#include "transport_router.h"

namespace transport {
    Router::Router(RoutingSettings settings, const transport_catalogue::TransportCatalogue& catalog)
        : settings_(std::move(settings)) {
        BuildGraph(catalog);
    }
    const graph::DirectedWeightedGraph<double>& Router::BuildGraph(const transport_catalogue::TransportCatalogue& catalog) {
        const auto& all_stops = catalog.GetSortedAllStops();
        const auto& all_buses = catalog.GetSortedAllBuses();
        graph::DirectedWeightedGraph<double> stops_graph(all_stops.size() * 2);

        // Создаем вершины графа и ребра ожидания
        graph::VertexId vertex_id = 0;
        for (const auto& [stop_name, stop_info] : all_stops) {
            stop_ids_[stop_info->name] = vertex_id;

            stops_graph.AddEdge({
                vertex_id,
                vertex_id + 1,
                static_cast<double>(settings_.bus_wait_time),
                graph::EdgeType::WAIT,
                stop_info->name,
                0
                });
            vertex_id += 2;
        }

        // Функция для перевода расстояния в минуты
        auto CalcTime = [this](int distance) {
            return distance / (settings_.bus_velocity * 1000.0 / 60.0);
            };

        // Обрабатываем маршруты автобусов
        for (const auto& [bus_name, bus] : all_buses) {
            const auto& stops = bus->route;
            const size_t n = stops.size();

            // Префиксные суммы расстояний для прямого и обратного направлений
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
                    stops_graph.AddEdge({
                        from_id + 1,
                        to_id,
                        CalcTime(dist_sum),
                        graph::EdgeType::BUS,
                        bus->name,
                        static_cast<int>(j - i)
                        });

                    // Обратное направление (если маршрут не кольцевой)
                    if (bus->type != domain::TypeRoute::circular) {
                        stops_graph.AddEdge({
                            to_id + 1,
                            from_id,
                            CalcTime(dist_sum_inverse),
                            graph::EdgeType::BUS,
                            bus->name,
                            static_cast<int>(j - i)
                            });
                    }
                }
            }
        }

        graph_ = std::move(stops_graph);
        router_ = std::make_unique<graph::Router<double>>(graph_);
        return graph_;
    }

    const std::optional<graph::Router<double>::RouteInfo> Router::FindRoute(const std::string_view stop_from, const std::string_view stop_to) const {
        if (!router_) {
            return std::nullopt;
        }
        return router_->BuildRoute(stop_ids_.at(stop_from), stop_ids_.at(stop_to));
    }

    const graph::DirectedWeightedGraph<double>& Router::GetGraph() const {
        return graph_;
    }
    const int Router::GetBusWaitTime() const {
        return settings_.bus_wait_time;
    }

    const double Router::GetBusVelocity() const {
        return settings_.bus_velocity;
    }
    /*const Router Router::GetRouterSettings() const {
        return { settings_.bus_wait_time, settings_.bus_velocity};
    }*/

} // namespace transport
