#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace input_reader {

    namespace detail {

        geo::Coordinates ParseCoordinates(std::string_view str) {
            static const double nan = std::nan("");

            auto not_space = str.find_first_not_of(' ');
            auto comma = str.find(',');

            if (comma == str.npos) {
                return { nan, nan };
            }

            auto not_space2 = str.find_first_not_of(' ', comma + 1);
            auto comma2 = str.find_first_of(',', not_space2);

            double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
            double lng;
            if (comma2 == str.npos) {
                lng = std::stod(std::string(str.substr(not_space2)));
            }
            else {
                lng = std::stod(std::string(str.substr(not_space2, comma2 - not_space2)));
            }
            return { lat, lng };
        }

        std::string_view Trim(std::string_view string) {
            const auto start = string.find_first_not_of(' ');
            if (start == string.npos) {
                return {};
            }
            return string.substr(start, string.find_last_not_of(' ') + 1 - start);
        }

        std::vector<std::string_view> Split(std::string_view string, char delim) {
            std::vector<std::string_view> result;

            size_t pos = 0;
            while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
                auto delim_pos = string.find(delim, pos);
                if (delim_pos == string.npos) {
                    delim_pos = string.size();
                }
                if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
                    result.push_back(substr);
                }
                pos = delim_pos + 1;
            }

            return result;
        }

        std::vector<std::string_view> ParseRoute(std::string_view route) {
            if (route.find('>') != route.npos) {
                return Split(route, '>');
            }

            auto stops = Split(route, '-');
            std::vector<std::string_view> results(stops.begin(), stops.end());
            results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

            return results;
        }

        CommandDescription ParseCommandDescription(std::string_view line) {
            auto colon_pos = line.find(':');
            if (colon_pos == std::string_view::npos) return {};

            auto space_pos = line.find(' ');
            if (space_pos >= colon_pos) return {};

            auto not_space = line.find_first_not_of(' ', space_pos);
            if (not_space >= colon_pos) return {};

            return { std::string(line.substr(0, space_pos)),
                    std::string(line.substr(not_space, colon_pos - not_space)),
                    std::string(line.substr(colon_pos + 1)) };
        }

        std::vector<std::pair<int, std::string>> ParseDistance(std::string_view str) {
            std::vector<std::pair<int, std::string>> dist;
            str = str.substr(str.find(",") + 1);
            auto str_distance = str.substr(str.find_first_of(",") + 1);
            auto comma = str_distance.find(" to ") + 1;

            while (comma != 0) {
                auto meters = std::stoi(std::string(str_distance.substr(0, str_distance.find_first_of("m"))));
                auto stop = std::string(str_distance.substr(str_distance.find_first_of("m") + 5,
                    str_distance.find(",") - str_distance.find_first_of("m") - 5));
                dist.push_back({ meters, stop });
                comma = str_distance.find(",") + 1;
                str_distance = str_distance.substr(str_distance.find_first_of(",") + 1);
            }
            return dist;
        }

    } // namespace detail

    void InputReader::ParseLine(std::string_view line) {
        auto command_description = detail::ParseCommandDescription(line);
        if (command_description) {
            commands_.push_back(std::move(command_description));
        }
    }

    void InputReader::ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const {
        for (const auto& cmd : commands_) {
            if (cmd.command == "Stop") {
                geo::Coordinates coords = detail::ParseCoordinates(cmd.description);
                catalogue.AddStop(cmd.id, coords);
            }
        }

        for (const auto& cmd : commands_) {
            if (cmd.command == "Stop") {
                auto distances = detail::ParseDistance(cmd.description);
                for (const auto& [distance, to] : distances) {
                    catalogue.AddDistance(cmd.id, to, distance);
                }
            }
        }

        for (const auto& cmd : commands_) {
            if (cmd.command == "Bus") {
                std::vector<std::string_view> stops = detail::ParseRoute(cmd.description);
                bool is_roundtrip = cmd.description.find('>') != std::string::npos;

                bool all_stops_exist = true;
                for (std::string_view stop : stops) {
                    if (!catalogue.FindStop(stop)) {
                        all_stops_exist = false;
                        break;
                    }
                }

                if (all_stops_exist) {
                    catalogue.AddBus(cmd.id, stops, is_roundtrip);
                }
            }
        }
    }

} // namespace input_reader
