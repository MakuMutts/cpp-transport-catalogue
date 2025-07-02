#pragma once
#include <unordered_set>

#include "domain.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

class RequestHandler {
public:
    RequestHandler(const transport_catalogue::TransportCatalogue& db, const renderer::MapRenderer& renderer) : db_(db), renderer_(renderer) {};

    std::optional<domain::BusStat> GetBusStat(const std::string& bus_name) const;

    std::set<std::string> GetBusesByStop(const std::string& stop_name) const;

    svg::Document RenderMap() const;

private:
    const transport_catalogue::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
};
