#pragma once

class FYamlNode;

/** Parses a String into an Yaml-Node.
 *
 * @returns If the Parsing was successful */
UNREALYAML_API bool ParseYaml(const FString String, FYamlNode& Out);

/** Opens a Document and Parses the Contents into a Yaml Structure.
 *
 * @returns If the File Exists and the Parsing was successful */
UNREALYAML_API bool LoadYamlFromFile(const FString Path, FYamlNode& Out);

/** Writes the Contents of a YAML-Node to an File.
 * This will overwrite the existing File if it exists! */
UNREALYAML_API void WriteYamlToFile(const FString Path, FYamlNode Node);