#include "transport_catalogue.h"
#include <optional>
#include <cmath>

namespace transport_catalogue {

    void TransportCatalogue::AddStop(std::string name, geo::Coordinates coordinates) {
        stops_.push_back({ std::move(name), coordinates,{} });
        const Stop* stop_ptr = &stops_.back();
        stopname_to_stop_[stop_ptr->name] = stop_ptr;
        stop_to_buses_[stop_ptr];
    }

    void TransportCatalogue::AddBus(std::string name, const std::vector<std::string_view>& stop_names, bool is_roundtrip) {
        std::vector<const Stop*> stop_ptrs;
        stop_ptrs.reserve(stop_names.size());

        for (std::string_view stop_name : stop_names) {
            auto it = stopname_to_stop_.find(stop_name);
            if (it != stopname_to_stop_.end()) {
                stop_ptrs.push_back(it->second);
            }
        }

        buses_.push_back({ std::move(name), std::move(stop_ptrs), is_roundtrip });
        const Bus* bus_ptr = &buses_.back();
        busname_to_bus_[bus_ptr->name] = bus_ptr;

        for (const Stop* stop : bus_ptr->stops) {
            if (stop) {
                stop_to_buses_[stop].insert(bus_ptr->name);
                const_cast<Stop*>(stop)->buses.insert(bus_ptr->name);
            }
        }
    }

    void TransportCatalogue::AddDistance(std::string_view from, std::string_view to, double distance) {
        const Stop* from_stop = FindStop(from);
        const Stop* to_stop = FindStop(to);

        if (from_stop && to_stop) {
            distances_[{from_stop, to_stop}] = distance;
        }
    }

    const Bus* TransportCatalogue::FindBus(std::string_view name) const {
        if (auto it = busname_to_bus_.find(name); it != busname_to_bus_.end()) {
            return it->second;
        }
        return nullptr;
    }

    const Stop* TransportCatalogue::FindStop(std::string_view name) const {
        if (auto it = stopname_to_stop_.find(name); it != stopname_to_stop_.end()) {
            return it->second;
        }
        return nullptr;
    }

    double TransportCatalogue::GetDistance(std::string_view from, std::string_view to) const {
        const Stop* from_stop = FindStop(from);
        const Stop* to_stop = FindStop(to);

        if (!from_stop || !to_stop) {
            return 0.0;
        }

        if (auto it = distances_.find({ from_stop, to_stop }); it != distances_.end()) {
            return it->second;
        }

        if (auto it = distances_.find({ to_stop, from_stop }); it != distances_.end()) {
            return it->second;
        }

        return 0.0;
    }

    std::optional<TransportCatalogue::BusInfo> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
        const Bus* bus = FindBus(bus_name);
        if (!bus) return std::nullopt;

        BusInfo info;
        info.stop_count = bus->stops.size();

        std::unordered_set<const Stop*> unique_stops;
        for (const Stop* stop : bus->stops) {
            if (stop) unique_stops.insert(stop);
        }
        info.unique_stop_count = unique_stops.size();

        double geo_distance = 0.0;
        double real_distance = 0.0;

        for (size_t i = 1; i < bus->stops.size(); ++i) {
            const Stop* from = bus->stops[i - 1];
            const Stop* to = bus->stops[i];
            if (from && to) {
                geo_distance += geo::ComputeDistance(from->coordinates, to->coordinates);
                real_distance += GetDistance(from->name, to->name);
            }
        }


        info.route_length = real_distance;
        info.curvature = real_distance / geo_distance;

        return info;
    }

    std::optional<const std::unordered_set<std::string_view>*> TransportCatalogue::GetBusesByStop(const Stop* stop) const {
        auto it = stop_to_buses_.find(stop);
        if (it != stop_to_buses_.end()) {
            return &it->second;
        }
        return std::nullopt;
    }

} 
