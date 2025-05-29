
#pragma once
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

#include "geo.h"

namespace transport_catalogue {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip;
};

class TransportCatalogue {
public:
    void AddStop(std::string name, geo::Coordinates coordinates);
    void AddBus(std::string name, const std::vector<std::string_view>& stop_names, bool is_roundtrip);

    const Bus* FindBus(std::string_view name) const;
    const Stop* FindStop(std::string_view name) const;

    struct BusInfo {
        size_t stop_count = 0;
        size_t unique_stop_count = 0;
        double route_length = 0.0;
    };

    std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
    std::optional<const std::unordered_set<std::string_view>*> GetBusesByStop(const Stop* stop) const;

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;

    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
    std::unordered_map<const Stop*, std::unordered_set<std::string_view>> stop_to_buses_;
};

} 
