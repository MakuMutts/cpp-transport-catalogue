#pragma once

#include "json.h"
#include "json_reader.h"
#include "transport_router.h"
#include "transport_catalogue.h"

#include <sstream>

using namespace renderer;
using namespace transport;

class RequestHandler {
public:
	explicit RequestHandler(const transport::Catalog& catalog, const renderer::MapRenderer& renderer, const transport::Router& router)
		: catalog_(catalog)
		, renderer_(renderer)
		, router_(router)
	{
	}

	std::optional<transport::Route> GetBusStat(const std::string_view bus_number) const;
	const std::set<std::string> GetBusesByStop(std::string_view stop_name) const;
	bool IsBusNumber(const std::string_view bus_number) const;
	bool IsStopName(const std::string_view stop_name) const;

	svg::Document RenderMap() const;
	const std::optional<graph::Router<double>::RouteInfo> GetOptimalRoute(const std::string_view stop_from, const std::string_view stop_to) const;
	const graph::DirectedWeightedGraph<double>& GetRouterGraph() const;
private:

	const Catalog& catalog_;
	const MapRenderer& renderer_;
	const transport::Router& router_;


};