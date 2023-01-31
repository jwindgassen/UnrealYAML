# UnrealYAML: A Unreal-Compatible Wrapper for the yaml-cpp Library

Welcome to the UnrealYAML Plugin, a Plugin that allows the parsing and emitting of YAML files, based on the [yaml-cpp Library](https://github.com/jbeder/yaml-cpp)

**Current yaml-cpp base commit:** [328d2d8](https://github.com/jbeder/yaml-cpp/commit/328d2d85e833be7cb5a0ab246cc3f5d7e16fc67a)

**Important Node:** The Plugin is far from finished and needs more testing, bug fixing and features to become fully usable and to work natively in Unreal! Feel free to contribute

## Tutorial
For a Tutorial, please visit the official [yaml-cpp wiki](https://github.com/jbeder/yaml-cpp/wiki/Tutorial). All classes have been adjusted to match the Unreal Naming Convention, but overall stayed the same.

## Implemented Features
- Basic Functionality of the Node class (Assignment, Creating a YAML structure)
- Conversion to and from most frequently used Unreal Types
- Iterators

## Examples

### Blueprint

![image](https://user-images.githubusercontent.com/4656466/215745483-bc751ab4-e08a-4ab5-bd7e-ddfab56ef34a.png)
![image](https://user-images.githubusercontent.com/4656466/215745441-d23025be-f675-4e20-8d67-7a2b64b6dd85.png)

## TODO
- Wrapper class for the Emitter
- Better Stability
- More conversions


## Patches
- **enum-class**: Changed enums to enum classes to remove *-Wshadow* error
