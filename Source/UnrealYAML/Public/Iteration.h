#pragma once

#include "FYamlNode.h"

/** The Iterator Base class. */
class FYamlIterator {
	friend class FYamlNode;
	
	YAML::iterator Iterator;
	
	explicit FYamlIterator(const YAML::iterator Iter) : Iterator(Iter) {}

	// Proxystruct returned by the -> operator
	struct FProxy {
		FYamlNode Ref;
		
		explicit FProxy(FYamlNode& Node) : Ref(Node) {}
		FYamlNode* operator->() { return &Ref; }
		operator FYamlNode*() { return &Ref; }
	};
	
public:	
	FYamlNode& operator*() const {
		return *new FYamlNode(*Iterator);
	}
	
	FYamlIterator& operator++() {
		++Iterator;
		return *this;
	}

	FYamlIterator& operator++(int) {
		FYamlIterator Pre(*this);
		++(*this);
		return Pre;
	}

	FProxy& operator->() const {
		return *new FProxy(**this);
	}

	bool operator ==(const FYamlIterator Other) const {return Iterator == Other.Iterator;}
	bool operator !=(const FYamlIterator Other) const {return Iterator != Other.Iterator;}
};