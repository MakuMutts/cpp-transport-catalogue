#include "json_reader.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>

using namespace std::string_literals;

namespace json_reader {
    void JsonReader::AddStops(transport_catalogue::TransportCatalogue& db) const {
        json::Node root = document_.GetRoot();
        json::Dict dict;

        if (root.IsDict()) {
            dict = root.AsDict();
            json::Node base_requests = dict.at("base_requests");

            json::Array requests = base_requests.AsArray();
            for (const json::Node& request : requests) {
                if (request.AsDict().at("type").AsString() == "Stop") {
                    std::string name = request.AsDict().at("name").AsString();
                    double latitude = request.AsDict().at("latitude").AsDouble();
                    double longitude = request.AsDict().at("longitude").AsDouble();
                    db.AddStop(name, { latitude, longitude });
                }
            }
            //Второй проход для добавления расстояния
            for (const json::Node& request : requests) {
                if (request.AsDict().at("type").AsString() == "Stop") {
                    if (request.AsDict().at("road_distances").IsDict()) {
                        json::Dict road_distances = request.AsDict().at("road_distances").AsDict();
                        for (const auto& [stop_name, distance] : road_distances) {
                            std::string name_this_stop = request.AsDict().at("name").AsString();
                            int distance_int = distance.AsInt();
                            db.AddDistanceToStops(db.GetStop(name_this_stop), db.GetStop(stop_name), distance_int);
                        }
                    }
                }
            }
        }
    }

    void JsonReader::AddBuses(transport_catalogue::TransportCatalogue& db) const {
        json::Node root = document_.GetRoot();
        json::Dict dict;

        dict = root.AsDict();
        json::Node base_requests = dict.at("base_requests");

        json::Array requests = base_requests.AsArray();
        for (const json::Node& request : requests) {
            if (request.AsDict().at("type").AsString() == "Bus") {
                std::string bus_name = request.AsDict().at("name").AsString();
                json::Array stops_array = request.AsDict().at("stops").AsArray();
                std::vector<std::string> stops;
                for (const json::Node& stop : stops_array) {
                    stops.push_back(stop.AsString());
                }
                domain::TypeRoute type;
                if (request.AsDict().at("is_roundtrip").AsBool()) {
                    type = domain::TypeRoute::circular;
                }
                else {
                    type = domain::TypeRoute::linear;
                }
                db.AddBus(bus_name, stops, type);
            }
        }
    }

    void JsonReader::FillDataBase(transport_catalogue::TransportCatalogue& db) const {
        AddStops(db);
        AddBuses(db);
    }

    std::vector<StatRequest> JsonReader::GetRequest(void) const {
        using namespace std::string_literals;

        std::vector<StatRequest> result;
        StatRequest req;
        json::Node root = document_.GetRoot();
        json::Dict dict = root.AsDict();
        json::Node base_requests = dict.at("stat_requests");
        json::Array requests = base_requests.AsArray();
        for (const json::Node& request : requests) {
            std::string type = request.AsDict().at("type").AsString();
            req.id = request.AsDict().at("id").AsInt();
            if (type == "Map") {
                req.type = TypeRequest::Map;
            }
            else if (type == "Bus"s) {
                req.type = TypeRequest::Bus;
            }
            else if (type == "Stop"s) {
                req.type = TypeRequest::Stop;
            }
            if (req.type == TypeRequest::Bus || req.type == TypeRequest::Stop) {
                req.name = request.AsDict().at("name").AsString();
            }
            result.push_back(req);
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
        else {
            std::set<std::string> buses = request_handler.GetBusesByStop(request.name);
            if (buses.size() == 0) {
                json::Array buses_arr;
                return json::Builder{}.StartDict()
                    .Key("buses"s).Value(buses_arr)
                    .Key("request_id"s).Value(request.id)
                    .EndDict()
                    .Build()
                    .AsDict();
            }
            else {
                json::Array buses_arr;
                for (const std::string& bus : buses) {
                    buses_arr.push_back(bus);
                }
                return json::Builder{}.StartDict()
                    .Key("buses"s).Value(buses_arr)
                    .Key("request_id"s).Value(request.id)
                    .EndDict()
                    .Build()
                    .AsDict();
            }
        }
    }

    static json::Dict GetBus(const json_reader::StatRequest& request, const RequestHandler& request_handler) {
        std::optional<domain::BusStat> bus = request_handler.GetBusStat(request.name);
        if (bus == std::nullopt) {
            return GetErrorMessage(request);
        }
        else {
            domain::BusStat b = *bus;
            return json::Builder{}.StartDict()
                .Key("curvature"s).Value(b.curvature)
                .Key("request_id"s).Value(request.id)
                .Key("route_length"s).Value(b.route_length)
                .Key("stop_count"s).Value(b.stop_count)
                .Key("unique_stop_count"s).Value(b.unique_stop_count)
                .EndDict()
                .Build()
                .AsDict();
        }
    }

    static json::Dict GetMap(const json_reader::StatRequest& request, const RequestHandler& request_handler) {
        std::stringstream sstrm;
        svg::Document doc = request_handler.RenderMap();
        doc.Render(sstrm);
        std::string test_str = sstrm.str();
        return json::Builder{}.StartDict()
            .Key("map"s).Value(sstrm.str())
            .Key("request_id"s).Value(request.id)
            .EndDict().Build().AsDict();
    }

    void JsonReader::Out(transport_catalogue::TransportCatalogue& db, const RequestHandler& request_handler, std::ostream& output) const {
        std::vector<StatRequest> stat_requests = GetRequest();  //Получаем Запросы
        json::Array arr;
        for (const json_reader::StatRequest& request : stat_requests) {
            if (request.type == json_reader::TypeRequest::Stop) {
                arr.emplace_back(GetStop(db, request, request_handler));
            }
            if (request.type == json_reader::TypeRequest::Bus) {
                arr.emplace_back(GetBus(request, request_handler));
            }
            if (request.type == json_reader::TypeRequest::Map) {
                arr.emplace_back(GetMap(request, request_handler));
            }
        }
        json::Print(json::Document{ arr }, output);
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
        json::Node root = document_.GetRoot();
        json::Dict dict;
        if (root.IsDict()) {
            dict = root.AsDict();
            if (dict.count("render_settings"s) > 0) {
                json::Node render_settings_node = dict.at("render_settings"s);
                json::Dict settings = render_settings_node.AsDict();
                AddSvgSettings(settings, render_setting.svg);
                AddBusSettings(settings, render_setting.bus);
                AddStopSettings(settings, render_setting.stop);
                AddUnderlayerSettings(settings, render_setting.underlayer);
                AddColorPalette(settings, render_setting.color_palette);
            }
        }
        return render_setting;
    }

}  // namespace json_reader
