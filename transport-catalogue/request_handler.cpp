#include "request_handler.h"

#include <unordered_set>

struct Stop_Hasher {
    size_t operator()(const domain::Stop* stop) const {
        return (size_t)stop;
    }
};

std::optional<domain::BusStat> RequestHandler::GetBusStat(const std::string& bus_name) const {
    domain::BusStat bus_stat;
    const domain::Bus* bus = db_.GetBus(bus_name);
    if (bus == nullptr) {
        return std::nullopt;
    }
    bus_stat.stop_count = db_.GetCountStopsOnRouts(bus);
    std::unordered_set<const domain::Stop*, Stop_Hasher> unique_stops;
    for (const domain::Stop* stop : bus->route) {
        unique_stops.insert(stop);
    }
    bus_stat.unique_stop_count = static_cast<int>(unique_stops.size());
    int real_distance = db_.GetLengthRoute(bus);
    bus_stat.route_length = real_distance;
    bus_stat.curvature = db_.GetCurvature(bus, real_distance);

    return bus_stat;
}

std::set<std::string> RequestHandler::GetBusesByStop(const std::string& stop_name) const {
    const domain::Stop* stop = db_.GetStop(stop_name);
    return db_.GetBusesContainingStop(stop);
}

static std::vector<geo::Coordinates> GetStopCoordinates(const std::map<std::string, const domain::Stop*>& stops) {
    std::vector<geo::Coordinates> stop_coordinates;
    for (const auto& [name, stop] : stops) {
        stop_coordinates.push_back(stop->coord);
    }
    return stop_coordinates;
}

svg::Document RequestHandler::RenderMap() const {
    std::vector<const domain::Bus*> buses = db_.GetBuses();
    std::vector<renderer::BusColor> bus_colors = renderer_.GetBusLineColor(buses);
    std::map<std::string, const domain::Stop*> stops_containing_bus = db_.GetStopsContainingAnyBus();
    std::vector<geo::Coordinates> stop_coordinates = GetStopCoordinates(stops_containing_bus);
    //Создаем проектор координат
    renderer::SphereProjector sphere_projector(stop_coordinates.begin(), stop_coordinates.end(),
        renderer_.GetRenderSetings().svg.width,
        renderer_.GetRenderSetings().svg.height,
        renderer_.GetRenderSetings().svg.padding);
    std::vector<svg::Polyline> route_lines = renderer_.GetRouteLines(bus_colors, sphere_projector);
    std::vector<svg::Text> route_names = renderer_.GetRouteNames(bus_colors, sphere_projector);

    std::vector<svg::Circle> stop_symbols = renderer_.GetStopSymbols(stops_containing_bus, sphere_projector);
    std::vector<svg::Text> stop_names = renderer_.GetStopNames(stops_containing_bus, sphere_projector);

    svg::Document doc;
    for (const svg::Polyline& line : route_lines) {
        doc.Add(line);
    }
   for (const svg::Text& text : route_names) {
        doc.Add(text);
    }
    for (const svg::Circle& circle : stop_symbols) {
        doc.Add(circle);
    }
    for (const svg::Text& stop_name : stop_names) {
        doc.Add(stop_name);
    }
    return doc;
}
