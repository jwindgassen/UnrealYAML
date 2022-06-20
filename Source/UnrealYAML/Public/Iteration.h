#pragma once

#include "Node.h"

/** The Iterator Base class. */
class FYamlIterator {
	friend struct FYamlNode;
	
	YAML::iterator Iterator;
	int32 Index;
	
	explicit FYamlIterator(const YAML::iterator Iter) : Iterator(Iter), Index(0) {}

	// Proxystruct returned by the -> operator
	struct FProxy {
		FYamlNode Ref;
		
		explicit FProxy(FYamlNode& Node) : Ref(Node) {}
		FYamlNode* operator->() { return &Ref; }
		operator FYamlNode*() { return &Ref; }
	};
	
public:
	/** Returns the <b>Key</b> Element of the Key-Value-Pair if the Iterated Node is a <b>Map</b>
	 * or a Node containing the <b>Index</b> of the Value if the Iterated Node is a <b>List</b>!
	 *
	 * The corresponding Value can be retrieved via Value() */
	FYamlNode Key() {
		if (Iterator->first.IsDefined()) {
			return FYamlNode(Iterator->first);
		}

		return FYamlNode(Index);
	}

	
	/** Returns the <b>Value</b> Element of the Key-Value-Pair if the Iterated Node is a <b>Map</b>
	* or a Node containing the <b>Value</b> if the Iterated Node is a <b>List</b>!
	*
	* The corresponding Key (for a Map) or Index (for a List) can be retrieved via Key() */
	FYamlNode Value() {
		if (Iterator->second.IsDefined()) {
			return FYamlNode(Iterator->second);
		}

		return **this;
	}

	
	/** Dereferencing the Iterator yields the Value */
	FYamlNode& operator*() const {
		if (Iterator->second.IsDefined()) {
			return *new FYamlNode(Iterator->second);
		}
		
		return *new FYamlNode(*Iterator);
	}

	
	/** The Arrow Operator yields a Pointer to the Value */
	FProxy& operator->() const {
		return *new FProxy(**this);
	}

	
	FYamlIterator& operator++() {
		++Iterator;
		Index++;
		return *this;
	}

	
	FYamlIterator operator++(int) {
		FYamlIterator Pre(*this);
		++(*this);
		Index++;
		return Pre;
	}
	
	bool operator ==(const FYamlIterator Other) const {return Iterator == Other.Iterator;}
	bool operator !=(const FYamlIterator Other) const {return Iterator != Other.Iterator;}
};