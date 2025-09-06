#pragma once
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "domain.h"
namespace transport_catalogue {
    class TransportCatalogue {
    public:
        void AddStop(const std::string& name, geo::Coordinates coordinates);
        void AddBus(const std::string& name, const std::vector<std::string>& names_stops, domain::TypeRoute type);
        void AddDistanceToStops(const domain::Stop* first_stop, const domain::Stop* second_stop, int distance);
        int GetCountStopsOnRouts(const domain::Bus* bus) const;
        const domain::Bus* GetBus(const std::string& name) const;
        const domain::Stop* GetStop(const std::string& name) const;
        std::set<std::string> GetBusesContainingStop(const domain::Stop* stop) const;
        int GetRealLengthRoute(const domain::Stop* from, const domain::Stop* to) const;
        int GetLengthRoute(const domain::Bus* bus) const;
        double GetCurvature(const domain::Bus* bus, int real_distance) const;
        std::vector<const domain::Bus*> GetBuses() const;
        std::map<std::string, const domain::Stop*> GetStopsContainingAnyBus() const;


        const std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, domain::StopPairHasher>& GetDistanceToStops() const;
        const std::unordered_map<std::string, const domain::Stop*>& GetStopsIndex() const;
        const std::unordered_map<std::string, const domain::Bus*>& GetBusesIndex() const;
        const std::map<std::string_view, const domain::Bus*> GetSortedAllBuses() const;
        const std::map<std::string_view, const domain::Stop*> GetSortedAllStops() const;
        int GetDistance(const domain::Stop* from_there, const domain::Stop* there) const;

        double GetBusVelocity() const;
        int GetBusWaitTime() const;
        void SetRoutingSettings(int bus_wait_time, double bus_velocity);


        std::deque<domain::Stop> stops_;
        std::unordered_map<std::string, const domain::Stop*> names_stops_;
        std::deque<domain::Bus> buses_;
        std::unordered_map<std::string, const domain::Bus*> names_buses_;
        std::unordered_map<const domain::Stop*, std::vector<const domain::Bus*>> stop_to_buses_;
        std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, domain::StopPairHasher> distance_to_stops_;
        int bus_wait_time_ = 0;
        double bus_velocity_ = 0.0;
    };



}  //namespace transport_catalogue