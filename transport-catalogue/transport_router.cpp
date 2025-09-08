#include "transport_router.h"

namespace transport {

    Router::Router(RoutingSettings settings, const transport_catalogue::TransportCatalogue& catalog)
        : settings_(std::move(settings)) {
        BuildGraph(catalog);
    }

    void Router::BuildGraph(const transport_catalogue::TransportCatalogue& catalog) {
        const auto& all_stops = catalog.GetSortedAllStops();
        const auto& all_buses = catalog.GetSortedAllBuses();
        graph::DirectedWeightedGraph<double> stops_graph(all_stops.size() * 2);

        // Вершины для ожидания
        graph::VertexId vertex_id = 0;
        for (const auto& [stop_name, stop_info] : all_stops) {
            stop_ids_[stop_info->name] = vertex_id;

            // Ребро ожидания
            graph::EdgeId edge_id = stops_graph.AddEdge({
                vertex_id,
                vertex_id + 1,
                static_cast<double>(settings_.bus_wait_time),
                graph::EdgeType::WAIT,
                stop_info->name,
                0
                });

            edge_info_[edge_id] = RouteInfo_::WaitItem{
                stop_info,
                Minutes(settings_.bus_wait_time),
                stop_info->name
            };

            vertex_id += 2;
        }

        auto CalcTime = [this](int distance) {
            return distance / (settings_.bus_velocity * 1000.0 / 60.0);
            };

        // Автобусные рёбра
        for (const auto& [bus_name, bus] : all_buses) {
            const auto& stops = bus->route;
            const size_t n = stops.size();

            std::vector<int> prefix_dist(n, 0), prefix_dist_inv(n, 0);
            for (size_t i = 1; i < n; ++i) {
                prefix_dist[i] = prefix_dist[i - 1] + catalog.GetDistance(stops[i - 1], stops[i]);
                prefix_dist_inv[i] = prefix_dist_inv[i - 1] + catalog.GetDistance(stops[i], stops[i - 1]);
            }

            for (size_t i = 0; i < n; ++i) {
                const auto from_id = stop_ids_.at(stops[i]->name);

                for (size_t j = i + 1; j < n; ++j) {
                    const auto to_id = stop_ids_.at(stops[j]->name);

                    int dist_sum = prefix_dist[j] - prefix_dist[i];
                    int dist_sum_inverse = prefix_dist_inv[j] - prefix_dist_inv[i];

                    // Прямое направление
                    {
                        double time = CalcTime(dist_sum);
                        graph::EdgeId edge_id = stops_graph.AddEdge({
                            from_id + 1,
                            to_id,
                            time,
                            graph::EdgeType::BUS,
                            bus->name,
                            static_cast<int>(j - i)
                            });

                        edge_info_[edge_id] = RouteInfo_::BusItem{
                            bus,
                            Minutes(time),
                            j - i,
                            bus->name
                        };
                    }

                    // Обратное направление (если линейный маршрут)
                    if (bus->type != domain::TypeRoute::circular) {
                        double time = CalcTime(dist_sum_inverse);
                        graph::EdgeId edge_id = stops_graph.AddEdge({
                            to_id + 1,
                            from_id,
                            time,
                            graph::EdgeType::BUS,
                            bus->name,
                            static_cast<int>(j - i)
                            });

                        edge_info_[edge_id] = RouteInfo_::BusItem{
                            bus,
                            Minutes(time),
                            j - i,
                            bus->name
                        };
                    }
                }
            }
        }

        graph_ = std::move(stops_graph);
        router_ = std::make_unique<graph::Router<double>>(graph_);
    }

    std::optional<RouteInfo_> Router::FindRoute(std::string_view stop_from, std::string_view stop_to) const {
        if (!router_) {
            return std::nullopt;
        }
        auto route_info = router_->BuildRoute(stop_ids_.at(stop_from), stop_ids_.at(stop_to));
        if (!route_info) {
            return std::nullopt;
        }
        return ConvertRouteInfo(*route_info);
    }

    RouteInfo_ Router::ConvertRouteInfo(const graph::Router<double>::RouteInfo& route_info) const {
        RouteInfo_ result;
        result.total_time = Minutes(route_info.weight);

        for (auto edge_id : route_info.edges) {
            result.edges.push_back(edge_id);
            auto it = edge_info_.find(edge_id);
            if (it != edge_info_.end()) {
                result.items.push_back(it->second);
            }
        }

        return result;
    }

} // namespace transport
