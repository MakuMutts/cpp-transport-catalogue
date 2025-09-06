#include <iostream>

#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

using namespace std;
int main() {
    transport_catalogue::TransportCatalogue transport_catalogue;  //Создаем каталог
    renderer::MapRenderer map_renderer;                           //Создаем рендерер

    json_reader::JsonReader json_reader(cin);
    json_reader.FillDataBase(transport_catalogue);  //Заполняем транспортный каталог

    renderer::RenderSettings render_setting = json_reader.GetRenderSettings();  //Получаем настройки для рендера из json файла
    map_renderer.SetRenderSettings(render_setting);
    RequestHandler request_handler = RequestHandler(transport_catalogue, map_renderer);

    json_reader.Out(transport_catalogue, request_handler, cout);
    return 0;
}
