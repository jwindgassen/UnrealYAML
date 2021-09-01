#pragma once

#include "yaml.h"
#include "YAMLAliases.h"
#include "UnrealYAML.h"

/** A wrapper for the Yaml Node class. Base YAML class. Stores a YAML-Structure in a Tree-like hierarchy.
 * Can therefore either hold a single value or be a Container for other Nodes.
 * Conversion from one Type to another will be done automatically as needed */
class UNREALYAML_API FYamlNode {
	friend void operator<<(std::ostream& Out, const FYamlNode& Node);
	friend void operator<<(FYamlEmitter& Out, const FYamlNode& Node);

	YAML::Node Node;

public:
	// Constructors --------------------------------------------------------------------
	/** Generate an Empty YAML Node */
	FYamlNode() = default;

	/** Generate an Empty YAML Node of a specific Type */
	explicit FYamlNode(const EYamlNodeType Type) : Node(Type) {}
	
	/** Generate a YAML Node that contains the given Data, which is implicitly Converted */
	template<typename T>
	explicit FYamlNode(const T& Data) : Node(Data) {}

	/** Generate an YAML Node from an Iterator Value */
	explicit FYamlNode(const YAML::detail::iterator_value& Value) : Node(YAML::Node(Value)) {}

	/** Generate an YAML Node from a Native YAML Node*/
	explicit FYamlNode(const YAML::Node Value) : Node(Value) {}
	
	// Types ---------------------------------------------------------------------------
	/** Returns the Type of the Contained Data */
	EYamlNodeType Type() const;

	/** If the Node has been Defined */
	bool IsDefined() const;

	/** Equivalent to Type() == Null (No Value) */
	bool IsNull() const;

	/** Equivalent to Type() == Scalar (Singular Value) */
	bool IsScalar() const;

	/** Equivalent to Type() == Sequence (Multiple Values without Keys) */
	bool IsSequence() const;

	/** Equivalent to Type() == Map (Multiple Values with Keys) */
	bool IsMap() const;

	
	// Conversion to bool and output to a Stream ---------------------------------------
	explicit operator bool() const;
	bool operator !() const;
	
	// Style ---------------------------------------------------------------------------
	/** Returns the Style of the Node, mostly relevant for Sequences */
	EYamlEmitterStyle Style() const;

	/** Sets the Style of the Node, mostly relevant for Sequences */
	void SetStyle(const EYamlEmitterStyle Style);


	// Assignment ----------------------------------------------------------------------
	/** Test if 2 Nodes are Equal */
	bool Is(const FYamlNode& Other) const;
	bool operator ==(const FYamlNode Other) const;
	
	/** Assign a Value to this Node. Will automatically converted */
	template <typename T>
	FYamlNode& operator=(const T& Value) {
		try {
			Node = Value;
		} catch (YAML::InvalidNode) {
			UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, won't assign any Value!"))
		}
		return *this;
	}
	
	/** Assign a Value to this Node. Will automatically converted */
	FYamlNode& operator=(const FYamlNode& Other) {
		Node = Other.Node;
		return *this;
	}
	
	/** Overwrite the Contents of this Node with the Content of another Node, or delete them if no Argument is given
	 *
	 * @returns If the Operation was successful
	 */
	bool Reset(const FYamlNode& Other = FYamlNode());

	
	// Access --------------------------------------------------------------------------
	/** Try to Convert the Contents of the Node to the Given Type */
	template <typename T>
	bool As(T& Out) const {
		try {
			Out = Node.as<T>();
			return true;
		} catch (YAML::Exception) {
			return false;
		}
	}

	/** Check if the given node can be converted to the given Type */
	template <typename T>
	bool CanConvertTo() const {
		try {
			Node.as<T>();
			return true;
		} catch (YAML::Exception) {
			return false;
		}
	}
	
	/** Try to Content of the Node if it is a Scalar */
	FString Scalar() const;

	/** Returns the whole Content of the Node as a single FString */
	FString GetContent() const;

	// Size and Iteration --------------------------------------------------------------
	/** Returns the Size of the Node if it is a Sequence or Map, 0 otherwise */
	int32 Size() const;

	/** Returns the start for a constant iterator. Use in combination with End() */
	FYamlConstIterator begin() const;

	/** Returns the start for an iterator. Use in combination with End() */
	FYamlIterator begin();

	/** Returns the end for a constant iterator. Use in combination with Start() */
	FYamlConstIterator end() const;

	/** Returns the end for a iterator. Use in combination with Start() */
	FYamlIterator end();

	// Sequence ------------------------------------------------------------------------
	/** Converts the Node to a Sequence and adds the Element to this list */
	template <typename T>
	void Push(const T& Element) {
		try {
			Node.push_back(Element);
		} catch (YAML::InvalidNode) {
			UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, can't Push any Value onto it!"))
		}
	}
	
	/** Converts the Node to a Sequence and adds the Node to this list */
	void Push(const FYamlNode& Element) {
		try {
			Node.push_back(Element.Node);
		} catch (YAML::InvalidNode) {
			UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, can't Push any Value onto it!"))
		}
	}

	// Map -----------------------------------------------------------------------------
	/** Forces a Conversion to a Map and adds the given Key-Value pair to the Map */
	template <typename K, typename V>
	void ForceInsert(const K& Key, const V& Value) {
		Node.force_insert(Key, Value);
	}
	
	// Indexing ------------------------------------------------------------------------
	/** Returns the Value at the given Key or Index */
	template <typename T>
	const FYamlNode operator[](const T& Key) const {
		return FYamlNode(Node[Key]);
	}

	/** Returns the Value at the given Key or Index */
	template <typename T>
	FYamlNode operator[](const T& Key) {
		return FYamlNode(Node[Key]);
	}

	/** Removes the Value at the given Key or Index */
	template <typename T>
	bool Remove(const T& Key) {
		return Node.remove(Key);
	}

	/** Returns the Value at the given Key or Index */
	const FYamlNode operator[](const FYamlNode& Key) const {
		return FYamlNode(Node[Key.Node]);
	}

	/** Returns the Value at the given Key or Index */
	FYamlNode operator[](const FYamlNode& Key) {
		return FYamlNode(Node[Key.Node]);
	}

	/** Removes the Value at the given Key or Index */
	bool Remove(const FYamlNode& Key) {
		return Node.remove(Key.Node);
	}
};

// Global Conversions ------------------------------------------------------------------

/** Write the Contents of the Node to an OutputStream */
inline void operator<<(std::ostream& Out, const FYamlNode& Node) {
	Out << Node.Node;
}

/** Write the Contents of the Node into an Emitter */
inline void operator<<(FYamlEmitter& Out, const FYamlNode& Node) {
	Out << Node.Node;
}
