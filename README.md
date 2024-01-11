# UnrealYAML: A Unreal-Compatible Wrapper for the yaml-cpp Library

Welcome to the UnrealYAML Plugin, a Plugin that allows the parsing and emitting of YAML files, based on the [yaml-cpp Library](https://github.com/jbeder/yaml-cpp)

**Current yaml-cpp base commit:** [328d2d8](https://github.com/jbeder/yaml-cpp/commit/328d2d85e833be7cb5a0ab246cc3f5d7e16fc67a)

**Important Node:** The Plugin is far from finished and needs more testing, bug fixing and features to become fully usable and to work natively in Unreal! Feel free to contribute

## Features
- Basic Functionality
	- Assigment
	- Conversion to and from most frequently used Unreal Types
	- Iterators
	- Loading and Saving Files
- Usable in Blueprint and C++
- Automatic Parsing to Unreal Struct using the Unreal Reflection System
	- `UPROPERTY` support:
		- Simple scalar values, such as int, float, FString and FText
		- `TMap` and `TArray`, and nested `USTRUCT`s 
		- Object references: `TSoftObjectPtr` and `TSubclassOf`
	- Configurable validation and detailed error reporting.

### TODO
- Node into Struct Parsing:
	- With Blueprint Structs (if possible)
	- Option to respect case on key values (currently always case insensitive)
	- Parse Struct into Node (Reverse Direction)
- Sequences and Maps into TArray\<Node> and TMap\<FString, Node> for Iteration in Blueprints
- Interfacing with Unreal JSON Plugin?
- Wrapper class for the Emitter?
- Schema Verification?


## Tutorial
You can find some examples in the [corresponding wiki page](https://github.com/jwindgassen/UnrealYAML/wiki/Examples).
Since this is a thin wrapper around the *yaml-cpp* library, many of the things in their [wiki](https://github.com/jbeder/yaml-cpp/wiki/Tutorial) can also be applied to this Plugin too.


## Patches
I used some patches ontop of the *yaml-cpp* library to ease the integration:
- **enum-class**: Changed enums to enum classes to remove *-Wshadow* error
