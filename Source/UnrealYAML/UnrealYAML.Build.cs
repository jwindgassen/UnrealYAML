using System.IO;
using Tools.DotNETCommon;
using UnrealBuildTool;

public class UnrealYAML : ModuleRules {
	public UnrealYAML(ReadOnlyTargetRules Target) : base(Target) {
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] {"Core", "Projects"});

		bEnableExceptions = true;
		bUseRTTI = true;

		PublicIncludePaths.Add(Path.Combine(PluginDirectory, "Source", "UnrealYAML", "yaml-cpp", "include"));
		PrivateIncludePaths.Add(Path.Combine(PluginDirectory, "Source", "UnrealYAML","yaml-cpp", "src"));
	}
}
