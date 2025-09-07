#pragma once

#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <math.h>
namespace transport {
	struct RoutingSettings {
		int bus_wait_time = 0;
		double bus_velocity = 0.0;
	};
	class Router {
	public:
		Router(RoutingSettings settings, const transport_catalogue::TransportCatalogue& catalog);

		
		const std::optional<graph::Router<double>::RouteInfo> FindRoute(const std::string_view stop_from, const std::string_view stop_to) const;


		const graph::DirectedWeightedGraph<double>& GetGraph() const;
		const int GetBusWaitTime() const;
		const double GetBusVelocity() const;

		//const Router GetRouterSettings() const;
	private:
		
		RoutingSettings settings_;
		const graph::DirectedWeightedGraph<double>& BuildGraph(const transport_catalogue::TransportCatalogue& catalog);
		graph::DirectedWeightedGraph<double> graph_;
		std::map<std::string_view, graph::VertexId> stop_ids_;
		std::unique_ptr<graph::Router<double>> router_;
	};

}