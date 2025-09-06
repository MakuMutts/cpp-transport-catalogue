#include "json_builder.h"

namespace json {

    ItemContext::ItemContext(Builder& builder) : builder_(builder) {}

    KeyItemContext ItemContext::Key(std::string value) {
        builder_.Key(value);
        KeyItemContext key_item_context(builder_);
        return key_item_context;
    }

    Builder& ItemContext::EndDict() {
        builder_.EndDict();
        return builder_;
    }

    ValueDictItemContext ItemContext::Value(std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> value) {
        builder_.Value(value);
        ValueDictItemContext value_item_context(builder_);
        return value_item_context;
    }

    ValueDictItemContext ItemContext::StartDict() {
        builder_.StartDict();
        ValueDictItemContext dict_item_context(builder_);
        return dict_item_context;
    }

    ArrayItemContext ItemContext::StartArray() {
        builder_.StartArray();
        ArrayItemContext array_item_context(builder_);
        return array_item_context;
    }

    Builder& ItemContext::EndArray() {
        builder_.EndArray();
        return builder_;
    }

    ArrayItemContext ArrayItemContext::Value(std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> value) {
        builder_.Value(value);
        ArrayItemContext value_array_item_context(builder_);
        return value_array_item_context;
    }

    ArrayItemContext Builder::StartArray() {
        if (is_empty_) {
            root_ = (Node(Array()));
            nodes_stack_.push_back(&root_);
            is_empty_ = false;
        }
        else {
            if (nodes_stack_.empty()) {
                throw std::logic_error("object complite");
            }
            if (nodes_stack_.back()->IsArray()) {
                Array new_node = Array();
                const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(new_node);
                nodes_stack_.push_back(&const_cast<Array&>(nodes_stack_.back()->AsArray()).back());
                is_empty_ = false;
            }
            else if (nodes_stack_.back()->IsDict()) {
                if (last_key_ == std::nullopt) {
                    throw std::logic_error("Not key for value");
                }
                Array new_node = Array();
                const_cast<Dict&>(nodes_stack_.back()->AsDict()).insert({ *last_key_, new_node });
                nodes_stack_.push_back(&const_cast<Dict&>(nodes_stack_.back()->AsDict()).at(*last_key_));
                last_key_ = std::nullopt;
                is_empty_ = false;
            }
        }
        ArrayItemContext array_item_context(*this);
        return array_item_context;
    }

    Builder& Builder::EndArray() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("object complite");
        }
        if (!nodes_stack_.back()->IsArray())
            throw std::logic_error("Its not end Array");
        nodes_stack_.pop_back();
        return *this;
    }

    ValueDictItemContext Builder::StartDict() {
        if (is_empty_) {
            root_ = (Node(Dict()));
            nodes_stack_.push_back(&root_);
            is_empty_ = false;
        }
        else {
            if (nodes_stack_.empty()) {
                throw std::logic_error("object complite");
            }
            if (nodes_stack_.back()->IsArray()) {
                Dict new_node = Dict();
                const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(new_node);
                nodes_stack_.push_back(&const_cast<Array&>(nodes_stack_.back()->AsArray()).back());
                is_empty_ = false;
            }
            else if (nodes_stack_.back()->IsDict()) {
                if (last_key_ == std::nullopt) {
                    throw std::logic_error("Not key for value");
                }
                Dict new_node = Dict();
                const_cast<Dict&>(nodes_stack_.back()->AsDict()).insert({ *last_key_, new_node });
                nodes_stack_.push_back(&const_cast<Dict&>(nodes_stack_.back()->AsDict()).at(*last_key_));
                last_key_ = std::nullopt;
                is_empty_ = false;
            }
        }
        ValueDictItemContext dict_item_context(*this);
        return dict_item_context;
    }
    Builder& Builder::EndDict() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("object complite");
        }
        if (!nodes_stack_.back()->IsDict())
            throw std::logic_error("Its not end Dict");
        nodes_stack_.pop_back();
        return *this;
    }

    KeyItemContext Builder::Key(std::string value) {
        if (nodes_stack_.empty()) {
            throw std::logic_error("object complite");
        }
        if (!nodes_stack_.back()->IsDict() || (last_key_ != std::nullopt)) {
            throw std::logic_error("Invalid Key");
        }
        last_key_ = value;

        KeyItemContext key_item_context(*this);
        return key_item_context;
    }

    Builder& Builder::Value(ValueVar value) {
        if (is_empty_) {
            Node new_node = std::visit([](auto val) {
                return Node(val);
                },
                value);
            root_ = new_node;
            is_empty_ = false;
        }
        else {
            if (nodes_stack_.empty()) {
                throw std::logic_error("object complite");
            }
            if (nodes_stack_.back()->IsArray()) {
                Node new_node = std::visit([](auto val) {
                    return Node(val);
                    },
                    value);
                const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(new_node);
                is_empty_ = false;
            }
            else if (nodes_stack_.back()->IsDict()) {
                if (last_key_ == std::nullopt) {
                    throw std::logic_error("Not key for value");
                }
                Node new_node_value = std::visit([](auto val) {
                    return Node(val);
                    },
                    value);
                const_cast<Dict&>(nodes_stack_.back()->AsDict()).insert({ *last_key_, new_node_value });
                last_key_ = std::nullopt;
                is_empty_ = false;
            }
            else {
                throw std::logic_error("Invalid Value");
            }
        }
        return *this;
    }

    Node Builder::Build() {
        if ((is_empty_) || (!nodes_stack_.empty())) {
            throw std::logic_error("Builder is empty or Array/Dict not end");
        }
        return root_;
    }

}  // namespace json