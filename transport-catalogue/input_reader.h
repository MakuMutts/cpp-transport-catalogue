#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace input_reader {

namespace detail {

struct CommandDescription {
    explicit operator bool() const {
        return !command.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    std::string command;
    std::string id;
    std::string description;
};

} //detail

class InputReader {
public:
    void ParseLine(std::string_view line);
    void ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const;

private:
    std::vector<detail::CommandDescription> commands_;
};

} //input_reader
