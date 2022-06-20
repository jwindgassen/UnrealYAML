#pragma once

#include "yaml.h"

// An Emitter used to create new YAML-Structures. You can use the "<<"-operator to add Elements to the Structure.
using FYamlEmitter = YAML::Emitter;
