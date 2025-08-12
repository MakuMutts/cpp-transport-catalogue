#pragma once

#include "json.h"

#include <optional>

namespace json {

	class Builder {
	public:
		class DictItemContext;
		class DictKeyContext;
		class ArrayItemContext;

		Builder();
		DictKeyContext Key(std::string key);
		Builder& Value(Node::Value value);
		DictItemContext StartDict();
		Builder& EndDict();
		ArrayItemContext StartArray();
		Builder& EndArray();
		Node Build();
		Node GetNode(Node::Value value);

	private:
		Node root_{ nullptr };
		std::vector<Node*> nodes_stack_;
		std::optional<std::string> key_{ std::nullopt };
	};

	class Builder::DictItemContext {
	public:
		DictItemContext(Builder& builder);

		DictKeyContext Key(std::string key);
		Builder& EndDict();

	private:
		Builder& builder_;
	};

	class BuilderContext {
	public:
		explicit BuilderContext(Builder& builder) : builder_(builder) {}
	protected:
		Builder& builder_;

		Builder::ArrayItemContext StartArray();
		Builder::DictItemContext StartDict();
	};

	class Builder::ArrayItemContext : public BuilderContext {
	public:
		ArrayItemContext(Builder& builder) : BuilderContext(builder) {}

		ArrayItemContext Value(Node::Value value);
		Builder& EndArray();
	};

	class Builder::DictKeyContext : public BuilderContext {
	public:
		DictKeyContext(Builder& builder) : BuilderContext(builder) {}

		DictItemContext Value(Node::Value value);
	};


}