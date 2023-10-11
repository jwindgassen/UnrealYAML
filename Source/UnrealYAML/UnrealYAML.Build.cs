using System.IO;
using UnrealBuildTool;

public class UnrealYAML : ModuleRules {
	public UnrealYAML(ReadOnlyTargetRules Target) : base(Target) {
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		Type = ModuleType.CPlusPlus;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });

		bEnableExceptions = true;

		// Replace the source ExportHeader with our ExportHeader
		PublicDefinitions.Add("YAML_CPP_API=UNREALYAML_API");

		PublicIncludePaths.Add(Path.Combine(PluginDirectory, "Source", "UnrealYAML", "yaml-cpp", "include"));
		PrivateIncludePaths.Add(Path.Combine(PluginDirectory, "Source", "UnrealYAML", "yaml-cpp", "src"));
	}
}
