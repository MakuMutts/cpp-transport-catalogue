#pragma once

#include <optional>
#include <string>

#include "json.h"

namespace json {

    class Builder;
    class KeyItemContext;
    class ArrayItemContext;
    class ValueDictItemContext;

    class Builder {
    public:
        using ValueVar = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;
        ArrayItemContext StartArray();
        Builder& EndArray();
        ValueDictItemContext StartDict();
        Builder& EndDict();
        KeyItemContext Key(std::string value);
        Builder& Value(ValueVar value);

        Node Build();

    private:
        Node root_;
        std::vector<Node*> nodes_stack_;
        std::optional<std::string> last_key_;
        bool is_empty_ = true;
    };

    class ItemContext {
    public:
        ItemContext(Builder& builder);

    protected:
        KeyItemContext Key(std::string value);
        Builder& EndDict();
        ValueDictItemContext Value(std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> value);
        ValueDictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndArray();
        Builder& builder_;

    private:
    };

    class ValueDictItemContext final : private ItemContext {
    public:
        using ItemContext::EndDict;
        using ItemContext::ItemContext;
        using ItemContext::Key;
    };

    class KeyItemContext final : private ItemContext {
    public:
        using ItemContext::ItemContext;
        using ItemContext::StartArray;
        using ItemContext::StartDict;
        using ItemContext::Value;
    };

    class ArrayItemContext final : private ItemContext {
    public:
        using ItemContext::EndArray;
        using ItemContext::ItemContext;
        using ItemContext::StartArray;
        using ItemContext::StartDict;
        ArrayItemContext Value(std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> value);
    };

}  // namespace json
