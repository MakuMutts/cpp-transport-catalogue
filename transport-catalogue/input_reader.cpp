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
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
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

    std::string command = std::string(line.substr(0, space_pos));
    std::string id = std::string(Trim(line.substr(space_pos + 1, colon_pos - space_pos - 1)));
    std::string description = std::string(line.substr(colon_pos + 1));

    return {std::move(command), std::move(id), std::move(description)};
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

} 
