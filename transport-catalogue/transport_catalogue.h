#pragma once

#include "geo.h"

#include <set>
#include <map>
#include <deque>
#include <vector>
#include <string>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>


namespace transport {

	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
		std::set<std::string> buses_by_stop;
	};

	struct Bus {
		std::string number;
		std::vector<const Stop*> stops;
		bool circular_route;
	};

	struct Route {
		size_t stops_count;
		size_t unique_stops_count;
		double route_length;
		double curvature;
	};

	struct HashStopPtr {
		size_t operator()(const std::pair<const Stop*, const Stop*>& stops) const {
			static const std::hash<const Stop*> hasher{};
			return hasher(stops.first) ^ hasher(stops.second);
		}
	};

	class Catalog {
	public:

		void AddRoute(std::string_view bus_number, const std::vector<const Stop*> stops, bool circular_route);
		void AddStop(std::string_view stop_name, const geo::Coordinates coordinates);
		const Bus* FindRoute(std::string_view bus_number) const;
		const Stop* FindStop(std::string_view stop_name) const;
		size_t UniqueStopsCount(std::string_view bus_number) const;
		void SetDistance(const Stop* from_there, const Stop* there, int distance);
		int GetDistance(const Stop* from_there, const Stop* there) const;
		const std::map<std::string_view, const Bus*> GetSortedAllBuses() const;
		const std::map<std::string_view, const Stop*> GetSortedAllStops() const;
		std::optional<transport::Route> GetBusStat(const std::string_view bus_number) const;
		const std::set<std::string> GetBusesByStop(std::string_view stop_name) const;

	private:
		std::deque<Bus> all_buses_;
		std::deque<Stop> all_stops_;
		std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
		std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
		std::unordered_map<std::pair<const Stop*, const Stop*>, int, HashStopPtr> distances_between_stops_;
	};
}