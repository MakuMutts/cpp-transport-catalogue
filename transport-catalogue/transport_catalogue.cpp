#include "transport_catalogue.h"

namespace transport {

	void Catalog::AddRoute(std::string_view bus_number, const std::vector<const Stop*> stops, bool circular_route) {
		all_buses_.push_back({ std::string(bus_number), stops, circular_route });
		busname_to_bus_[all_buses_.back().number] = &all_buses_.back();
		for (const auto& route_stop : stops) {
			for (auto& stop_ : all_stops_) {
				if (stop_.name == route_stop->name) stop_.buses_by_stop.insert(std::string(bus_number));
			}
		}
	}

	void Catalog::AddStop(std::string_view stop_name, const geo::Coordinates coordinates) {
		all_stops_.push_back({ std::string(stop_name), coordinates, {} });
		stopname_to_stop_[all_stops_.back().name] = &all_stops_.back();
	}

	const Bus* Catalog::FindRoute(std::string_view bus_number) const {
		return busname_to_bus_.count(bus_number) ? busname_to_bus_.at(bus_number) : nullptr;
	}

	const Stop* Catalog::FindStop(std::string_view stop_name) const {
		return stopname_to_stop_.count(stop_name) ? stopname_to_stop_.at(stop_name) : nullptr;
	}

	size_t Catalog::UniqueStopsCount(std::string_view bus_number) const {
		const auto& stops = busname_to_bus_.at(bus_number)->stops;
		std::unordered_set<std::string_view> unique_stops;
		for (const auto& stop : stops) {
			unique_stops.insert(stop->name);
		}
		return unique_stops.size();
	}

	void Catalog::SetDistance(const Stop* from_there, const Stop* there, int distance) {
		distances_between_stops_[{from_there, there}] = distance;
	}

	int Catalog::GetDistance(const Stop* from_there, const Stop* there) const {
		if (distances_between_stops_.count({ from_there,there }) != 0) {
			return distances_between_stops_.at({ from_there, there });
		}
		else if (distances_between_stops_.count({ there,from_there }) != 0) {
			return distances_between_stops_.at({ there, from_there });
		}
		else {
			return 0;
		}
	}

	const std::map<std::string_view, const Bus*> Catalog::GetSortedAllBuses() const {
		return std::map<std::string_view, const Bus*>(busname_to_bus_.begin(), busname_to_bus_.end());
	}

	const std::map<std::string_view, const Stop*> Catalog::GetSortedAllStops() const {
		return std::map<std::string_view, const Stop*>(stopname_to_stop_.begin(), stopname_to_stop_.end());
	}

	std::optional<transport::Route> Catalog::GetBusStat(const std::string_view bus_number) const {
		transport::Route bus_stat{};
		const transport::Bus* bus = FindRoute(bus_number);

		if (!bus) throw std::invalid_argument("Can't find bus");
		if (bus->circular_route) bus_stat.stops_count = bus->stops.size();
		else bus_stat.stops_count = bus->stops.size() * 2 - 1;

		int route_length = 0;
		double geographic_length = 0.0;

		for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
			auto from_there = bus->stops[i];
			auto there = bus->stops[i + 1];
			if (bus->circular_route) {
				route_length += GetDistance(from_there, there);
				geographic_length += geo::ComputeDistance(from_there->coordinates,
					there->coordinates);
			}
			else {
				route_length += GetDistance(from_there, there) + GetDistance(there, from_there);
				geographic_length += geo::ComputeDistance(from_there->coordinates,
					there->coordinates) * 2;
			}
		}

		bus_stat.unique_stops_count = UniqueStopsCount(bus_number);
		bus_stat.route_length = route_length;
		bus_stat.curvature = route_length / geographic_length;

		return bus_stat;
	}

	const std::set<std::string> Catalog::GetBusesByStop(std::string_view stop_name) const {
		return FindStop(stop_name)->buses_by_stop;
	}
}