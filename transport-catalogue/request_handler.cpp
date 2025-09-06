#include "request_handler.h"
#include "transport_router.h"
#include "json_reader.h"
#include <unordered_set>

struct Stop_Hasher {
    size_t operator()(const domain::Stop* stop) const {
        return (size_t)stop;
    }
};

std::optional<domain::BusStat> RequestHandler::GetBusStat(const std::string& bus_name) const {
    const domain::Bus* bus = db_.GetBus(bus_name);
    if (!bus) return std::nullopt;

    domain::BusStat stat;
    stat.stop_count = db_.GetCountStopsOnRouts(bus);

    std::unordered_set<const domain::Stop*> unique_stops(bus->route.begin(), bus->route.end());
    stat.unique_stop_count = static_cast<int>(unique_stops.size());

    stat.route_length = db_.GetLengthRoute(bus);

    // std::cerr << "Bus " << bus_name << " route length: " << stat.route_length << "\n";
    // std::cerr << "Stops: ";
     /*for (const auto& stop : bus->route) {
         std::cerr << stop->name << " ";
     }
     std::cerr << "\n";*/

    stat.curvature = db_.GetCurvature(bus, stat.route_length);

    return stat;
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

json::Node RequestHandler::ProcessRouteRequest(const json_reader::StatRequest& request) const {
    //std::cout << "Processing route from " << request.from << " to " << request.to << std::endl;

    const auto& route_info = router_.FindRoute(request.from, request.to);
    if (!route_info) {
        //std::cout << "Route not found between " << request.from << " and " << request.to << std::endl;
        return json::Builder{}
            .StartDict()
            .Key("request_id").Value(request.id)
            .Key("error_message").Value("not found")
            .EndDict()
            .Build();
    }

    json::Array items;
    double total_time = 0.0;

    for (const auto& edge_id : route_info->edges) {
        const auto& edge = router_.GetGraph().GetEdge(edge_id);
        if (edge.type == graph::EdgeType::WAIT) {
            items.push_back(
                json::Builder{}
                .StartDict()
                .Key("type").Value("Wait")
                .Key("stop_name").Value(edge.name)
                .Key("time").Value(edge.weight)
                .EndDict()
                .Build()
            );
            total_time += edge.weight;
        }
        else if (edge.type == graph::EdgeType::BUS) {
            items.push_back(
                json::Builder{}
                .StartDict()
                .Key("type").Value("Bus")
                .Key("bus").Value(edge.name)
                .Key("span_count").Value(edge.span_count)
                .Key("time").Value(edge.weight)
                .EndDict()
                .Build()
            );
            total_time += edge.weight;
        }
    }

    return json::Builder{}
        .StartDict()
        .Key("request_id").Value(request.id)
        .Key("total_time").Value(total_time)
        .Key("items").Value(items)
        .EndDict()
        .Build();
}