#pragma once

#include <iostream>

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

class RequestHandler;

class JsonReader {
public:
	JsonReader(std::istream& input)
		: input_(json::Load(input))
	{
	}


	const json::Node& GetStatRequests() const;
	const json::Node& GetRenderSettings() const;
	const json::Node& GetRoutingSettings() const;

	void ProcessRequests(const json::Node& stat_requests, RequestHandler& rh) const;

	void FillCatalogue(transport::Catalog& catalog);
	renderer::MapRenderer FillRenderSettings(const json::Node& settings) const;
	transport::Router FillRoutingSettings(const json::Node& settingrs) const;

	const json::Node RendererPrintRoute(const json::Dict& request_map, RequestHandler& rh) const;
	const json::Node RendererPrintStop(const json::Dict& request_map, RequestHandler& rh) const;
	const json::Node RendererPrintMap(const json::Dict& request_map, RequestHandler& rh) const;
	const json::Node RendererPrintRouting(const json::Dict& request_map, RequestHandler& rh) const;



private:
	json::Document input_;
	json::Node dummy_ = nullptr;

	const json::Node& GetBaseRequests() const;

	std::tuple<std::string_view, geo::Coordinates, std::map<std::string_view, int>> FillStop(const json::Dict& request_map) const;
	void FillStopDistances(transport::Catalog& catalog) const;
	std::tuple<std::string_view, std::vector<const transport::Stop*>, bool> FillRoute(const json::Dict& request_map, transport::Catalog& catalog) const;
};