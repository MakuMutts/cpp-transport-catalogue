#pragma once
#include <string>
#include <vector>

#include "geo.h"

namespace domain {
    enum class TypeRoute {
        circular,
        linear
    };
    struct Route {
        std::vector<std::string> stops;
        TypeRoute type;
    };

    struct Stop {
        std::string name;
        geo::Coordinates coord;
    };
    struct Bus {
        std::string name;
        TypeRoute type = TypeRoute::circular;
        std::vector<const Stop*> route;
    };
    struct StopPairHasher {
        size_t operator()(std::pair<const Stop*, const Stop*> stops) const {
            return std::hash<const void*>()(stops.first) ^ std::hash<const void*>()(stops.second);
        }
    };
    struct BusStat {
        double curvature;
        int route_length;
        int stop_count;
        int unique_stop_count;
    };
    typedef  const Bus* BusPtr;
}  // namespace domain
