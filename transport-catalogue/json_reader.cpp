#include "json_reader.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>

using namespace std::string_literals;

namespace json_reader {
    // Быстрая карта типов запросов
    static const std::unordered_map<std::string, TypeRequest> type_map = {
        {"Bus", TypeRequest::Bus},
        {"Stop", TypeRequest::Stop},
        {"Map", TypeRequest::Map},
        {"Route", TypeRequest::Route}
    };

    void JsonReader::AddStops(transport_catalogue::TransportCatalogue& db) const {
        const json::Node& root = document_.GetRoot();
        if (!root.IsDict()) return;

        const json::Dict& dict = root.AsDict();
        const json::Array& requests = dict.at("base_requests").AsArray();

        // Первый проход: добавление остановок
        for (const json::Node& request : requests) {
            const json::Dict& request_dict = request.AsDict();
            std::string type = request_dict.at("type").AsString();

            if (type == "Stop") {
                std::string name = request_dict.at("name").AsString();
                double latitude = request_dict.at("latitude").AsDouble();
                double longitude = request_dict.at("longitude").AsDouble();
                db.AddStop(name, { latitude, longitude });
            }
        }

        // Второй проход: добавление расстояний
        for (const json::Node& request : requests) {
            const json::Dict& request_dict = request.AsDict();
            std::string type = request_dict.at("type").AsString();

            if (type == "Stop") {
                std::string from = request_dict.at("name").AsString();
                if (request_dict.at("road_distances").IsDict()) {
                    for (const auto& [to, dist] : request_dict.at("road_distances").AsDict()) {
                        int distance = dist.AsInt();
                        db.AddDistanceToStops(db.GetStop(from), db.GetStop(to), distance);
                    }
                }
            }
        }
    }

    void JsonReader::AddBuses(transport_catalogue::TransportCatalogue& db) const {
        const json::Node& root = document_.GetRoot();
        if (!root.IsDict()) return;

        const json::Dict& dict = root.AsDict();
        const json::Array& requests = dict.at("base_requests").AsArray();

        for (const json::Node& request : requests) {
            const json::Dict& request_dict = request.AsDict();
            std::string type = request_dict.at("type").AsString();

            if (type == "Bus") {
                std::string bus_name = request_dict.at("name").AsString();
                const json::Array& stops_array = request_dict.at("stops").AsArray();

                std::vector<std::string> stops;
                stops.reserve(stops_array.size());

                for (const json::Node& stop : stops_array) {
                    stops.push_back(stop.AsString());
                }

                domain::TypeRoute type_route = request_dict.at("is_roundtrip").AsBool()
                    ? domain::TypeRoute::circular
                    : domain::TypeRoute::linear;

                db.AddBus(bus_name, stops, type_route);
            }
        }
    }

    void JsonReader::FillDataBase(transport_catalogue::TransportCatalogue& db) const {
        AddStops(db);
        AddBuses(db);
    }

    std::vector<StatRequest> JsonReader::GetRequest() const {
        std::vector<StatRequest> result;
        const json::Node& root = document_.GetRoot();

        if (!root.IsDict()) return result;

        const json::Dict& dict = root.AsDict();
        const json::Array& requests = dict.at("stat_requests").AsArray();
        result.reserve(requests.size());

        for (const json::Node& request : requests) {
            const json::Dict& request_dict = request.AsDict();
            StatRequest req;
            req.id = request_dict.at("id").AsInt();

            std::string type_str = request_dict.at("type").AsString();
            if (auto it = type_map.find(type_str); it != type_map.end()) {
                req.type = it->second;
            }
            else {
                continue; // Пропускаем неизвестные типы
            }

            if (req.type == TypeRequest::Bus || req.type == TypeRequest::Stop) {
                req.name = request_dict.at("name").AsString();
            }
            else if (req.type == TypeRequest::Route) {
                req.from = request_dict.at("from").AsString();
                req.to = request_dict.at("to").AsString();
            }

            result.push_back(std::move(req));
        }

        return result;
    }

    static json::Dict GetErrorMessage(const json_reader::StatRequest& request) {
        return json::Builder{}.StartDict()
            .Key("request_id"s).Value(request.id)
            .Key("error_message"s).Value("not found"s)
            .EndDict()
            .Build()
            .AsDict();
    }

    static json::Dict GetStop(const transport_catalogue::TransportCatalogue& db, const json_reader::StatRequest& request, const RequestHandler& request_handler) {
        const domain::Stop* stop = db.GetStop(request.name);
        if (stop == nullptr) {
            return GetErrorMessage(request);
        }

        std::set<std::string> buses = request_handler.GetBusesByStop(request.name);
        json::Array buses_arr;
        buses_arr.reserve(buses.size());

        for (const std::string& bus : buses) {
            buses_arr.push_back(bus);
        }

        return json::Builder{}.StartDict()
            .Key("buses"s).Value(std::move(buses_arr))
            .Key("request_id"s).Value(request.id)
            .EndDict()
            .Build()
            .AsDict();
    }

    static json::Dict GetBus(const json_reader::StatRequest& request, const RequestHandler& request_handler) {
        std::optional<domain::BusStat> bus = request_handler.GetBusStat(request.name);
        if (!bus) {
            return GetErrorMessage(request);
        }

        const domain::BusStat& b = *bus;
        return json::Builder{}.StartDict()
            .Key("curvature"s).Value(b.curvature)
            .Key("request_id"s).Value(request.id)
            .Key("route_length"s).Value(static_cast<double>(b.route_length)) 
            .Key("stop_count"s).Value(b.stop_count)
            .Key("unique_stop_count"s).Value(b.unique_stop_count)
            .EndDict()
            .Build()
            .AsDict();
    }

    static json::Dict GetMap(const json_reader::StatRequest& request, const RequestHandler& request_handler) {
        std::stringstream sstrm;
        svg::Document doc = request_handler.RenderMap();
        doc.Render(sstrm);

        return json::Builder{}.StartDict()
            .Key("map"s).Value(sstrm.str())
            .Key("request_id"s).Value(request.id)
            .EndDict().Build().AsDict();
    }

    void JsonReader::Out(transport_catalogue::TransportCatalogue& db, const RequestHandler& request_handler, std::ostream& output) const {
        std::vector<StatRequest> stat_requests = GetRequest();
        json::Array arr;
        arr.reserve(stat_requests.size());

        for (const StatRequest& request : stat_requests) {
            switch (request.type) {
            case TypeRequest::Stop:
                arr.emplace_back(GetStop(db, request, request_handler));
                break;
            case TypeRequest::Bus:
                arr.emplace_back(GetBus(request, request_handler));
                break;
            case TypeRequest::Map:
                arr.emplace_back(GetMap(request, request_handler));
                break;
            case TypeRequest::Route:
                arr.emplace_back(request_handler.ProcessRouteRequest(request));
                break;
            }
        }

        json::Print(json::Document{ std::move(arr) }, output);
    }

    // Вспомогательные функции для чтения настроек рендеринга
    namespace {
        svg::Color ParseColor(const json::Node& color_node) {
            if (color_node.IsString()) {
                return color_node.AsString();
            }
            else if (color_node.IsArray()) {
                const auto& arr = color_node.AsArray();
                if (arr.size() == 3) {
                    return svg::Rgb{
                        static_cast<uint8_t>(arr[0].AsInt()),
                        static_cast<uint8_t>(arr[1].AsInt()),
                        static_cast<uint8_t>(arr[2].AsInt())
                    };
                }
                else if (arr.size() == 4) {
                    return svg::Rgba{
                        static_cast<uint8_t>(arr[0].AsInt()),
                        static_cast<uint8_t>(arr[1].AsInt()),
                        static_cast<uint8_t>(arr[2].AsInt()),
                        arr[3].AsDouble()
                    };
                }
            }
            return "black"s; // fallback
        }

        svg::Point ParsePoint(const json::Array& arr) {
            if (arr.size() >= 2) {
                return { arr[0].AsDouble(), arr[1].AsDouble() };
            }
            return { 0.0, 0.0 };
        }
    }
    static void AddSvgSettings(const json::Dict& settings, renderer::SvgRenderSettings& svg) {
        svg.width = settings.at("width"s).AsDouble();
        svg.height = settings.at("height"s).AsDouble();
        svg.padding = settings.at("padding"s).AsDouble();
    }

    static void AddBusSettings(const json::Dict& settings, renderer::BusRenderSettings& bus) {
        bus.line_width = settings.at("line_width"s).AsDouble();
        bus.label.font_size = settings.at("bus_label_font_size"s).AsInt();
        json::Array bus_label_offset = settings.at("bus_label_offset"s).AsArray();
        bus.label.offset = { bus_label_offset.at(0).AsDouble(), bus_label_offset.at(1).AsDouble() };
    }

    static void AddStopSettings(const json::Dict& settings, renderer::StopRenderSettings& stop) {
        stop.radius = settings.at("stop_radius"s).AsDouble();
        stop.label.font_size = settings.at("stop_label_font_size"s).AsInt();
        json::Array stop_label_offset = settings.at("stop_label_offset"s).AsArray();
        stop.label.offset = { stop_label_offset.at(0).AsDouble(), stop_label_offset.at(1).AsDouble() };
    }
    static svg::Rgb GetRgb(const json::Node& color) {
        svg::Rgb underlayer_color;
        underlayer_color.red = color.AsArray().at(0).AsInt();
        underlayer_color.green = color.AsArray().at(1).AsInt();
        underlayer_color.blue = color.AsArray().at(2).AsInt();
        return underlayer_color;
    }
    static svg::Rgba GetRgba(const json::Node& color) {
        svg::Rgba underlayer_color;
        underlayer_color.red = color.AsArray().at(0).AsInt();
        underlayer_color.green = color.AsArray().at(1).AsInt();
        underlayer_color.blue = color.AsArray().at(2).AsInt();
        underlayer_color.opacity = color.AsArray().at(3).AsDouble();
        return underlayer_color;
    }

    static void AddUnderlayerSettings(const json::Dict& settings, renderer::UnderlayerSettings& underlayer) {
        json::Node underlayer_color_node = settings.at("underlayer_color"s);
        if (underlayer_color_node.IsArray()) {
            if (underlayer_color_node.AsArray().size() == 3) {  //Rgb
                svg::Rgb underlayer_color = GetRgb(underlayer_color_node);
                underlayer.color = underlayer_color;
            }
            else if (underlayer_color_node.AsArray().size() == 4) {  //Rgba
                svg::Rgba underlayer_color = GetRgba(underlayer_color_node);
                underlayer.color = underlayer_color;
            }
        }
        else if (underlayer_color_node.IsString()) {
            underlayer.color = underlayer_color_node.AsString();
        }

        underlayer.width = settings.at("underlayer_width"s).AsDouble();
    }

    static void AddColorPalette(const json::Dict& settings, std::vector<svg::Color>& color_palette) {
        json::Node color_palette_node = settings.at("color_palette");
        json::Array colors_palette = color_palette_node.AsArray();
        for (const json::Node& color : colors_palette) {
            if (color.IsString()) {
                color_palette.push_back(color.AsString());
            }
            if (color.IsArray()) {
                if (color.AsArray().size() == 3) {  //Rgb
                    svg::Rgb rgb_color = GetRgb(color);
                    color_palette.push_back(rgb_color);
                }
                else if (color.AsArray().size() == 4) {  //Rgba
                    svg::Rgba rgba_color = GetRgba(color);
                    color_palette.push_back(rgba_color);
                }
            }
        }
    }
   
    renderer::RenderSettings JsonReader::GetRenderSettings() const {
        renderer::RenderSettings render_setting;
        const json::Node& root = document_.GetRoot();

        if (!root.IsDict()) return render_setting;

        const json::Dict& dict = root.AsDict();
        if (!dict.count("render_settings"s)) return render_setting;

        const json::Dict& settings = dict.at("render_settings"s).AsDict();

        // SVG settings
        render_setting.svg.width = settings.at("width"s).AsDouble();
        render_setting.svg.height = settings.at("height"s).AsDouble();
        render_setting.svg.padding = settings.at("padding"s).AsDouble();

        // Bus settings
        render_setting.bus.line_width = settings.at("line_width"s).AsDouble();
        render_setting.bus.label.font_size = settings.at("bus_label_font_size"s).AsInt();
        render_setting.bus.label.offset = ParsePoint(settings.at("bus_label_offset"s).AsArray());

        // Stop settings
        render_setting.stop.radius = settings.at("stop_radius"s).AsDouble();
        render_setting.stop.label.font_size = settings.at("stop_label_font_size"s).AsInt();
        render_setting.stop.label.offset = ParsePoint(settings.at("stop_label_offset"s).AsArray());

        // Underlayer settings
        render_setting.underlayer.color = ParseColor(settings.at("underlayer_color"s));
        render_setting.underlayer.width = settings.at("underlayer_width"s).AsDouble();

        // Color palette
        const json::Array& colors_palette = settings.at("color_palette").AsArray();
        render_setting.color_palette.reserve(colors_palette.size());

        for (const json::Node& color : colors_palette) {
            render_setting.color_palette.push_back(ParseColor(color));
        }

        return render_setting;
    }

    const json::Node& JsonReader::GetStatRequests() const {
        static const json::Node empty_node = nullptr;
        const json::Node& root = document_.GetRoot();

        if (!root.IsDict() || !root.AsDict().count("stat_requests")) {
            return empty_node;
        }

        return root.AsDict().at("stat_requests");
    }

    const json::Node& JsonReader::GetRoutingSettings() const {
        static const json::Node empty_node = nullptr;
        const json::Node& root = document_.GetRoot();

        if (!root.IsDict() || !root.AsDict().count("routing_settings")) {
            return empty_node;
        }

        return root.AsDict().at("routing_settings");
    }

    void JsonReader::ParseRoutingSettings(transport_catalogue::TransportCatalogue& db) const {
        const json::Node& root = document_.GetRoot();

        if (!root.IsDict() || !root.AsDict().count("routing_settings")) {
            return;
        }

        const json::Dict& routing_settings = root.AsDict().at("routing_settings").AsDict();
        int bus_wait_time = routing_settings.at("bus_wait_time").AsInt();
        double bus_velocity = routing_settings.at("bus_velocity").AsDouble();

        db.SetRoutingSettings(bus_wait_time, bus_velocity);
    }

}  // namespace json_reader