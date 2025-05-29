#include <iomanip>
#include <iostream>
#include <algorithm>
#include "stat_reader.h"

namespace stat_reader {

void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& catalogue, 
                      std::string_view request, std::ostream& output) {
    if (request.starts_with("Bus ")) {
        std::string_view bus_name = request.substr(4);
        auto info_opt = catalogue.GetBusInfo(bus_name);
        if (!info_opt) {
            output << "Bus " << bus_name << ": not found" << std::endl;
            return;
        }
        const auto& info = *info_opt;
        output << "Bus " << bus_name << ": "
               << info.stop_count << " stops on route, "
               << info.unique_stop_count << " unique stops, "
               << std::setprecision(6) << info.route_length << " route length" << std::endl;
    } else if (request.starts_with("Stop ")) {
        std::string_view stop_name = request.substr(5);
        const transport_catalogue::Stop* stop = catalogue.FindStop(stop_name);
        if (!stop) {
            output << "Stop " << stop_name << ": not found" << std::endl;
            return;
        }
        auto buses_opt = catalogue.GetBusesByStop(stop);
        if (!buses_opt || (*buses_opt)->empty()) {
            output << "Stop " << stop_name << ": no buses" << std::endl;
            return;
        }

        output << "Stop " << stop_name << ": buses";

        std::vector<std::string_view> sorted_buses((*buses_opt)->begin(), (*buses_opt)->end());
        std::sort(sorted_buses.begin(), sorted_buses.end());

        for (const auto& bus : sorted_buses) {
            output << " " << bus;
        }
        output << std::endl;
    }
}

} 
