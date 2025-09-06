#pragma once
#include <iostream>

#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

namespace json_reader {
    enum class TypeRequest {
        Bus,
        Stop,
        Map
    };

    struct StatRequest {
        int id = 0;
        TypeRequest type = TypeRequest::Bus;
        std::string name = "";
    };

    class JsonReader {
    public:
        JsonReader(std::istream& input) : document_(json::Load(input)) {};

        void FillDataBase(transport_catalogue::TransportCatalogue& db) const;
        void Out(transport_catalogue::TransportCatalogue& db, const RequestHandler& request_handler, std::ostream& output) const;
        renderer::RenderSettings GetRenderSettings() const;

    private:
        void AddStops(transport_catalogue::TransportCatalogue& db) const;
        void AddBuses(transport_catalogue::TransportCatalogue& db) const;

        std::vector<StatRequest> GetRequest(void) const;
        json::Document document_;
    };
}  // namespace json_reader
