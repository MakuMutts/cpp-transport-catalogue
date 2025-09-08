#pragma once

#include "router.h"
#include "transport_catalogue.h"
#include "domain.h"

#include <memory>
#include <chrono>
#include <variant>
#include <unordered_map>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

namespace transport {

    using Minutes = std::chrono::duration<double, std::chrono::minutes::period>;

    struct RoutingSettings {
        int bus_wait_time = 0;
        double bus_velocity = 0.0;
    };

    struct RouteInfo_ {
        Minutes total_time{};
        std::vector<graph::EdgeId> edges;

        struct BusItem {
            const domain::Bus* bus_ptr = nullptr;
            Minutes time{};
            size_t span_count = 0;
            std::string_view bus_name{};
        };

        struct WaitItem {
            const domain::Stop* stop_ptr = nullptr;
            Minutes time{};
            std::string_view stop_name{};
        };

        using Item = std::variant<BusItem, WaitItem>;
        std::vector<Item> items;
    };

    class Router {
    public:
        Router(RoutingSettings settings, const transport_catalogue::TransportCatalogue& catalog);

        std::optional<RouteInfo_> FindRoute(std::string_view stop_from, std::string_view stop_to) const;

    private:
        void BuildGraph(const transport_catalogue::TransportCatalogue& catalog);
        RouteInfo_ ConvertRouteInfo(const graph::Router<double>::RouteInfo& route_info) const;

        RoutingSettings settings_;
        graph::DirectedWeightedGraph<double> graph_;
        std::map<std::string_view, graph::VertexId> stop_ids_;
        std::unique_ptr<graph::Router<double>> router_;
        std::unordered_map<graph::EdgeId, RouteInfo_::Item> edge_info_;
    };

} // namespace transport
