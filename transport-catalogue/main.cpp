#include <iostream>
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

using namespace std;
int main() {
    transport_catalogue::TransportCatalogue db;
    renderer::MapRenderer renderer;

    json_reader::JsonReader reader(cin);
    reader.FillDataBase(db);
    transport::RoutingSettings routing_settings = reader.ParseRoutingSettings(db);
    // db.SetRoutingSettings(routing_settings.bus_wait_time, routing_settings.bus_velocity);

     // Инициализация маршрутизатора после загрузки всех данных
    transport::Router router(routing_settings, db);

    renderer::RenderSettings render_setting = reader.GetRenderSettings();
    //router.BuildGraph(db);
    renderer.SetRenderSettings(render_setting);
    RequestHandler request_handler(db, renderer, router);
    reader.Out(db, request_handler, cout);

    return 0;
}