#include "transport_catalogue.h"

namespace transport_catalogue {
    void TransportCatalogue::AddStop(const std::string& name, geo::Coordinates coordinates) {
        domain::Stop stop = { name, coordinates };
        stops_.push_back(stop);
        names_stops_[name] = &stops_[stops_.size() - 1];
    }

    void TransportCatalogue::AddBus(const std::string& name, const std::vector<std::string>& names_stops, domain::TypeRoute type) {
        domain::Bus bus;
        bus.name = name;
        for (std::string name : names_stops) {
            bus.route.push_back(names_stops_[name]);
        }
        bus.type = type;
        buses_.push_back(bus);
        names_buses_[name] = &buses_[buses_.size() - 1];
        for (std::string name : names_stops) {
            stop_to_buses_[names_stops_.at(name)].push_back(&buses_[buses_.size() - 1]);
        }
    }

    int TransportCatalogue::GetCountStopsOnRouts(const domain::Bus* bus) const {
        if (bus->type == domain::TypeRoute::linear) {
            return static_cast<int>(bus->route.size()) * 2 - 1;
        }
        return static_cast<int>(bus->route.size());
    }

    void TransportCatalogue::AddDistanceToStops(const domain::Stop* first_stop, const domain::Stop* second_stop, int distance) {
        distance_to_stops_[std::pair(first_stop, second_stop)] = distance;
    }

    const domain::Bus* TransportCatalogue::GetBus(const std::string& name) const {
        if (names_buses_.count(name))
            return names_buses_.at(name);
        else
            return nullptr;
    }

    const domain::Stop* TransportCatalogue::GetStop(const std::string& name) const {
        if (names_stops_.count(name))
            return names_stops_.at(name);
        return nullptr;
    }

    std::set<std::string> TransportCatalogue::GetBusesContainingStop(const domain::Stop* stop) const {
        std::set<std::string> result;
        if (stop_to_buses_.count(stop) > 0) {
            for (const domain::Bus* bus : stop_to_buses_.at(stop)) {
                result.insert(bus->name);
            }
        }
        return result;
    }

    int TransportCatalogue::GetRealLengthRoute(const domain::Stop* from, const domain::Stop* to) const {
        if (distance_to_stops_.count(std::pair(from, to)) == 0) {
            return distance_to_stops_.at(std::pair(to, from));
        }
        return distance_to_stops_.at(std::pair(from, to));
    }

    int TransportCatalogue::GetLengthRoute(const domain::Bus* bus) const {
        int length = 0;
        for (size_t i = 0; i < bus->route.size() - 1; ++i) {
            length += GetRealLengthRoute(bus->route[i], bus->route[i + 1]);
        }
        if (bus->type == domain::TypeRoute::linear) {
            for (size_t i = bus->route.size(); i > 1; --i) {
                length += GetRealLengthRoute(bus->route[i - 1], bus->route[i - 2]);
            }
        }
        return length;
    }

    double TransportCatalogue::GetCurvature(const domain::Bus* bus, int real_distance) const {
        double length = 0;
        for (size_t i = 0; i < bus->route.size() - 1; ++i) {
            length += geo::ComputeDistance(bus->route[i]->coord, bus->route[i + 1]->coord);
        }
        if (bus->type == domain::TypeRoute::linear) {
            length *= 2;
        }
        return real_distance / length;
    }

    std::vector<const domain::Bus*> TransportCatalogue::GetBuses() const {
        std::vector<const domain::Bus*> result;
        for (const domain::Bus& bus : buses_) {
            result.push_back(&bus);
        }
        return result;
    }

    std::map<std::string, const domain::Stop*> TransportCatalogue::GetStopsContainingAnyBus() const {
        std::map<std::string, const domain::Stop*> result;
        for (const auto& [name, stop] : names_stops_) {
            if (!GetBusesContainingStop(stop).empty())
                result.insert({ name, stop });
        }
        return result;
    }
}  // namespace transport_catalogue
