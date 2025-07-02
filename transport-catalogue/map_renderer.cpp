
#include "map_renderer.h"

#include <algorithm>

using namespace std::string_literals;

namespace renderer {
    //Выходной вектор должен быть отсортирован по именам автобусов
    std::vector<BusColor> MapRenderer::GetBusLineColor(std::vector<const domain::Bus*>& buses) const {
        std::vector<BusColor> result;
        //sort vector by name
        std::sort(buses.begin(), buses.end(), [](const domain::Bus* lhs, const domain::Bus* rhs) { return lhs->name < rhs->name; });
        int count_buses = 0;
        for (const domain::Bus* bus : buses) {
            int index_color = (count_buses) % render_setings_.color_palette.size();
            if (!bus->route.empty()) {
                ++count_buses;
            }
            result.push_back({ bus, &render_setings_.color_palette[index_color] });
        }
        return result;
    }

    //Входной вектор должен быть отсортирован по именам автобусов
    std::vector<svg::Polyline> MapRenderer::GetRouteLines(const std::vector<BusColor>& sorted_by_name_buses_color, const SphereProjector& sphere_projector) const {
        std::vector<svg::Polyline> result;
        for (const BusColor& bus_color : sorted_by_name_buses_color) {
            if (!bus_color.bus->route.empty()) {  //Отрисовываем если есть остановки на маршруте
                svg::Polyline polyline = svg::Polyline();
                //Добавляем точки с координатами в Polyline
                for (const domain::Stop* stop : bus_color.bus->route) {
                    polyline.AddPoint(sphere_projector(stop->coord));
                }
                //Едем в обратную сторону, если маршрут линейны
                if (bus_color.bus->type == domain::TypeRoute::linear) {
                    if (bus_color.bus->route.size() > 1) {
                        for (auto stop_it = bus_color.bus->route.rbegin() + 1; stop_it != bus_color.bus->route.rend(); ++stop_it) {
                            polyline.AddPoint(sphere_projector((*stop_it)->coord));
                        }
                    }
                }
                result.push_back(polyline.SetFillColor("none"s)
                    .SetStrokeColor(*bus_color.color)
                    .SetStrokeWidth(render_setings_.bus.line_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));
            }
        }
        return result;
    }
    static svg::Text GetRouteText(const BusColor& bus_color,
        const svg::Point coord,
        const LabelRenderSetting& label) {
        svg::Text text = svg::Text();
        text.SetPosition(coord);      //Задаем x y
        text.SetOffset(label.offset)  //Задаем смещение dx dy
            .SetFontSize(label.font_size)
            .SetFontFamily("Verdana"s)
            .SetFontWeight("bold")
            .SetData(bus_color.bus->name)
            .SetFillColor(*bus_color.color);
        return text;
    }
    static svg::Text GetRouteUnderlayerText(const BusColor& bus_color,
        const svg::Point coord,
        const LabelRenderSetting& label,
        const UnderlayerSettings& underlayer) {
        svg::Text text_underlayer = svg::Text();  //Подложка
        text_underlayer.SetPosition(coord);       //Задаем x y
        text_underlayer.SetOffset(label.offset)   //Задаем смещение dx dy
            .SetFontSize(label.font_size)
            .SetFontFamily("Verdana"s)
            .SetFontWeight("bold")
            .SetData(bus_color.bus->name)
            .SetFillColor(underlayer.color)
            .SetStrokeColor(underlayer.color)
            .SetStrokeWidth(underlayer.width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        return text_underlayer;
    }
    static std::tuple<svg::Text, svg::Text> GetRouteName(const BusColor& bus_color,
        const svg::Point coord,
        const LabelRenderSetting& label,
        const UnderlayerSettings& underlayer) {
        svg::Text text = GetRouteText(bus_color, coord, label);
        svg::Text text_underlayer = GetRouteUnderlayerText(bus_color, coord, label, underlayer);
        return { text_underlayer, text };
    }
    std::vector<svg::Text> MapRenderer::GetRouteNames(const std::vector<BusColor>& buses, const SphereProjector& sphere_projector) const {
        std::vector<svg::Text> result;
        for (const BusColor& bus_color : buses) {
            if (!bus_color.bus->route.empty()) {  //Если у маршрута есть остановки, то рисуем его
                if (bus_color.bus->type == domain::TypeRoute::circular) {
                    auto [text_underlayer, text] = GetRouteName(bus_color,
                        sphere_projector(bus_color.bus->route.at(0)->coord),
                        render_setings_.bus.label,
                        render_setings_.underlayer);

                    result.push_back(text_underlayer);
                    result.push_back(text);
                }
                else {
                    const domain::Stop* first_end_stop = *(bus_color.bus->route.begin());
                    auto [text_first_end_stop_underlayer, text_first_end_stop] = GetRouteName(bus_color,
                        sphere_projector(first_end_stop->coord),
                        render_setings_.bus.label,
                        render_setings_.underlayer);
                    result.push_back(text_first_end_stop_underlayer);
                    result.push_back(text_first_end_stop);

                    const domain::Stop* second_end_stop = *(bus_color.bus->route.end() - 1);
                    if (second_end_stop != first_end_stop) {
                        auto [text_second_end_stop_underlayer, text_second_end_stop] = GetRouteName(bus_color,
                            sphere_projector(second_end_stop->coord),
                            render_setings_.bus.label,
                            render_setings_.underlayer);
                        result.push_back(text_second_end_stop_underlayer);
                        result.push_back(text_second_end_stop);
                    }
                }
            }
        }
        return result;
    }

    std::vector<svg::Circle> MapRenderer::GetStopSymbols(const std::map<std::string, const domain::Stop*>& stops, const SphereProjector& sphere_projector) const {
        std::vector<svg::Circle> result;
        for (const auto& [name, stop] : stops) {
            svg::Circle symbol_stop = svg::Circle();
            symbol_stop.SetCenter(sphere_projector(stop->coord))
                .SetRadius(render_setings_.stop.radius)
                .SetFillColor("white"s);
            result.push_back(symbol_stop);
        }
        return result;
    }

    std::vector<svg::Text> MapRenderer::GetStopNames(const std::map<std::string, const domain::Stop*>& stops, const SphereProjector& sphere_projector) const {
        std::vector<svg::Text> result;
        for (const auto& [name, stop] : stops) {
            svg::Point stop_coord = sphere_projector(stop->coord);

            svg::Text stop_symbol_under = svg::Text();
            stop_symbol_under.SetPosition(stop_coord)
                .SetOffset(render_setings_.stop.label.offset)
                .SetFontSize(render_setings_.stop.label.font_size)
                .SetFontFamily("Verdana"s)
                .SetData(stop->name)
                .SetFillColor(render_setings_.underlayer.color)
                .SetStrokeColor(render_setings_.underlayer.color)
                .SetStrokeWidth(render_setings_.underlayer.width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            svg::Text stop_symbol = svg::Text();
            stop_symbol.SetPosition(stop_coord)
                .SetOffset(render_setings_.stop.label.offset)
                .SetFontSize(render_setings_.stop.label.font_size)
                .SetFontFamily("Verdana"s)
                .SetData(stop->name)
                .SetFillColor("black"s);

            result.push_back(stop_symbol_under);
            result.push_back(stop_symbol);
        }
        return result;
    }
}  // namespace renderer
