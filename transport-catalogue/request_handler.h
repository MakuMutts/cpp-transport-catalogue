#pragma once
#include <unordered_set>

#include "domain.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "json.h"

namespace json_reader {
    struct StatRequest;
}

class RequestHandler {
public:
    RequestHandler(const transport_catalogue::TransportCatalogue& db,
        const renderer::MapRenderer& renderer,
        const transport::Router& router)
        : db_(db), renderer_(renderer), router_(router) {
    };

    std::optional<domain::BusStat> GetBusStat(const std::string& bus_name) const;
    std::set<std::string> GetBusesByStop(const std::string& stop_name) const;
    svg::Document RenderMap() const;
    json::Node ProcessRouteRequest(const json_reader::StatRequest& request) const;
    //std::optional<domain::RouteStat> GetRoute(const std::string& from, const std::string& to) const;
private:
    const transport_catalogue::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    const transport::Router& router_;
};