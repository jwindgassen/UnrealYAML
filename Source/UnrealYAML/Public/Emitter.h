// Copyright (c) 2021-2026, Forschungszentrum Jülich GmbH. All rights reserved.
// Licensed under the MIT License. See LICENSE file for details.

#pragma once

#include "yaml.h"

// An Emitter used to create new YAML-Structures. You can use the "<<"-operator to add Elements to the Structure.
using FYamlEmitter = YAML::Emitter;
