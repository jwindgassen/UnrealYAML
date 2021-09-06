#pragma once

#include "node/convert.h"

namespace YAML{
	// encode and decode an FString
	template <>
	struct convert<FString> {
		static Node encode(const FString& String) {
			return Node(std::string(TCHAR_TO_UTF8(*String)));
		}

		static bool decode(const Node& Node, FString& Out) {
			if(!Node.IsScalar()) {
				return false;
			}

			Out = UTF8_TO_TCHAR(Node.as<std::string>().c_str());
			return true;
		}
	};

	// encode and decode an FText
	template <>
	struct convert<FText> {
		static Node encode(const FText& Text) {
			return Node(Text.ToString());
		}

		static bool decode(const Node& Node, FText& Out) {
			if(!Node.IsScalar()) {
				return false;
			}

			Out = FText::FromString(Node.as<FString>());
			return true;
		}
	};
	
	// encode and decode an FVector
	template <>
	struct convert<FVector> {
		static Node encode(const FVector& Vector) {
			Node Node;
			Node.SetStyle(EmitterStyle::Flow);
			Node.push_back(Vector.X);
			Node.push_back(Vector.Y);
			Node.push_back(Vector.Z);
			return Node;
		}

		static bool decode(const Node& Node, FVector& Out) {
			if(!Node.IsSequence() || Node.size() != 3) {
				return false;
			}

			Out.X = Node[0].as<double>();
			Out.Y = Node[1].as<double>();
			Out.Z = Node[2].as<double>();
			return true;
		}
	};


	// encode and decode an FQuad
	template <>
	struct convert<FQuat> {
		static Node encode(const FQuat& Quad) {
			Node Node;
			Node.SetStyle(EmitterStyle::Flow);
			Node.push_back(Quad.X);
			Node.push_back(Quad.Y);
			Node.push_back(Quad.Z);
			Node.push_back(Quad.W);
			return Node;
		}

		static bool decode(const Node& Node, FQuat& Out) {
			if(!Node.IsSequence() || Node.size() != 4) {
				return false;
			}

			Out.X = Node[0].as<double>();
			Out.Y = Node[1].as<double>();
			Out.Z = Node[2].as<double>();
			Out.W = Node[3].as<double>();
			return true;
		}
	};


	// encode and decode an FTransform
	template <>
	struct convert<FTransform> {
		static Node encode(const FTransform& Transform) {
			Node Node;
			Node.SetStyle(EmitterStyle::Flow);
			Node.push_back(Transform.GetLocation());
			Node.push_back(Transform.GetRotation());
			Node.push_back(Transform.GetScale3D());
			return Node;
		}

		static bool decode(const Node& Node, FTransform& Out) {
			if(!Node.IsSequence() || Node.size() != 3) {
				return false;
			}
			
			Out.SetTranslation(Node[0].as<FVector>());
			Out.SetRotation(Node[1].as<FQuat>());
			Out.SetScale3D(Node[2].as<FVector>());
			return true;
		}
	};


	// encode and decode an TArray to a sequence
	template<class T>
	struct convert<TArray<T>> {
		static Node encode(const TArray<T> Array) {
			Node Node(NodeType::Sequence);
			for (T& Element : Array) {
				Node.push_back(Element);
			}
			return Node;
		}

		static bool decode(const Node& Node, TArray<T>& Out) {
			if (!(Node.Type() == NodeType::Map || Node.Type() == NodeType::Sequence)) {
				return false;
			}

			Out = {};
			for(const_iterator Iterator = Node.begin(); Iterator != Node.end(); ++Iterator) {
				Out.Add(Iterator->as<T>());
			}

			return true;
		}
	};


	// encode and decode an TArray to a sequence
	template<class T>
	struct convert<TSet<T>> {
		static Node encode(const TSet<T> Array) {
			Node Node(NodeType::Sequence);
			for (T& Element : Array) {
				Node.push_back(Element);
			}
			return Node;
		}

		static bool decode(const Node& Node, TSet<T>& Out) {
			if (!(Node.Type() == NodeType::Map || Node.Type() == NodeType::Sequence)) {
				return false;
			}

			Out = {};
			for(const_iterator Iterator = Node.begin(); Iterator != Node.end(); ++Iterator) {
				Out.Add(Iterator->as<T>());
			}

			return true;
		}
	};


	// encode and decode an TMap to a Map
	template<class TKey, class TValue>
	struct convert<TMap<TKey, TValue>> {
		static Node encode(const TMap<TKey, TValue> Map) {
			Node Node(NodeType::Map);
			
			for (TTuple<TKey, TValue> Element : Map) {
				Node[Element.Key] = Element.Value;
			} 
			return Node;
		}

		static bool decode(const Node& Node, TMap<TKey, TValue>& Out) {
			if (Node.Type() != NodeType::Map) {
				return false;
			}

			Out = {};
			for(const_iterator Iterator = Node.begin(); Iterator != Node.end(); ++Iterator) {
				Out.Add(Iterator->first.as<TKey>(), Iterator->second.as<TValue>());
			}

			return true;
		}
	};
}
