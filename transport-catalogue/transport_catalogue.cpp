#include "transport_catalogue.h"

namespace transport_catalogue {

    void TransportCatalogue::AddStop(const std::string& name, geo::Coordinates coordinates) {
        stops_.push_back({ name, coordinates });
        names_stops_[stops_.back().name] = &stops_.back();
    }

    void TransportCatalogue::AddBus(const std::string& name,
        const std::vector<std::string>& names_stops,
        domain::TypeRoute type) {
        buses_.emplace_back();
        auto* bus_ptr = &buses_.back();
        bus_ptr->name = name;
        bus_ptr->type = type;

        bus_ptr->route.reserve(names_stops.size());
        for (const std::string& stop_name : names_stops) {
            auto it = names_stops_.find(stop_name);
            if (it != names_stops_.end()) {
                bus_ptr->route.push_back(it->second);
                stop_to_buses_[it->second].push_back(bus_ptr);
            }
        }
        names_buses_[name] = bus_ptr;
    }

    int TransportCatalogue::GetCountStopsOnRouts(const domain::Bus* bus) const {
        return (bus->type == domain::TypeRoute::linear)
            ? static_cast<int>(bus->route.size()) * 2 - 1
            : static_cast<int>(bus->route.size());
    }

    void TransportCatalogue::AddDistanceToStops(const domain::Stop* from,
        const domain::Stop* to,
        int distance) {
        distance_to_stops_[{from, to}] = distance;
    }

    const domain::Bus* TransportCatalogue::GetBus(const std::string& name) const {
        auto it = names_buses_.find(name);
        return (it != names_buses_.end()) ? it->second : nullptr;
    }

    const domain::Stop* TransportCatalogue::GetStop(const std::string& name) const {
        auto it = names_stops_.find(name);
        return (it != names_stops_.end()) ? it->second : nullptr;
    }

    std::set<std::string> TransportCatalogue::GetBusesContainingStop(const domain::Stop* stop) const {
        auto it = stop_to_buses_.find(stop);
        if (it == stop_to_buses_.end()) return {};
        std::set<std::string> result;
        for (const auto* bus : it->second) {
            result.insert(bus->name);
        }
        return result;
    }

    int TransportCatalogue::GetDistance(const domain::Stop* from, const domain::Stop* to) const {
        if (auto it = distance_to_stops_.find({ from, to }); it != distance_to_stops_.end())
            return it->second;
        if (auto it = distance_to_stops_.find({ to, from }); it != distance_to_stops_.end())
            return it->second;
        return 0;
    }

    int TransportCatalogue::GetLengthRoute(const domain::Bus* bus) const {
        int length = 0;
        for (size_t i = 0; i + 1 < bus->route.size(); ++i) {
            auto from = bus->route[i];
            auto to = bus->route[i + 1];
            length += GetDistance(from, to);
            if (bus->type == domain::TypeRoute::linear) {
                length += GetDistance(to, from);
            }
        }
        return length;
    }

    double TransportCatalogue::GetCurvature(const domain::Bus* bus, int real_distance) const {
        double geo_length = 0.0;
        for (size_t i = 0; i + 1 < bus->route.size(); ++i) {
            double dist = geo::ComputeDistance(bus->route[i]->coord, bus->route[i + 1]->coord);
            geo_length += (bus->type == domain::TypeRoute::linear) ? dist * 2 : dist;
        }
        return geo_length > 0 ? real_distance / geo_length : 0.0;
    }

    std::vector<const domain::Bus*> TransportCatalogue::GetBuses() const {
        std::vector<const domain::Bus*> result;
        result.reserve(buses_.size());
        for (const auto& bus : buses_) {
            result.push_back(&bus);
        }
        return result;
    }

    std::map<std::string, const domain::Stop*> TransportCatalogue::GetStopsContainingAnyBus() const {
        std::map<std::string, const domain::Stop*> result;
        for (const auto& [name, stop] : names_stops_) {
            if (auto it = stop_to_buses_.find(stop); it != stop_to_buses_.end() && !it->second.empty()) {
                result.emplace(name, stop);
            }
        }
        return result;
    }

    const std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, domain::StopPairHasher>&
        TransportCatalogue::GetDistanceToStops() const {
        return distance_to_stops_;
    }

    

    

    /*void TransportCatalogue::SetRoutingSettings(int bus_wait_time, double bus_velocity) {
        settings_.bus_wait_time = bus_wait_time;
        settings_.bus_velocity_ = bus_velocity;
    }*/

    const std::map<std::string_view, const domain::Bus*> TransportCatalogue::GetSortedAllBuses() const {
        return { names_buses_.begin(), names_buses_.end() };
    }

    const std::map<std::string_view, const domain::Stop*> TransportCatalogue::GetSortedAllStops() const {
        return { names_stops_.begin(), names_stops_.end() };
    }

} // namespace transport_catalogue
