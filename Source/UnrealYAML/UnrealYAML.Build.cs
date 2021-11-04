using System.IO;
using Tools.DotNETCommon;
using UnrealBuildTool;

public class UnrealYAML : ModuleRules {
	public UnrealYAML(ReadOnlyTargetRules Target) : base(Target) {
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		Type = ModuleType.CPlusPlus;
		PublicDependencyModuleNames.AddRange(new[] {"Core", "Projects"});

		bEnableExceptions = true;
		bUseRTTI = true;

		if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64) {
			PublicDefinitions.Add("YAML_CPP_DLL=1");
			PublicDefinitions.Add("yaml_cpp_EXPORTS=1");
		}

		PublicIncludePaths.Add(Path.Combine(PluginDirectory, "Source", "UnrealYAML", "yaml-cpp", "include"));
		PrivateIncludePaths.Add(Path.Combine(PluginDirectory, "Source", "UnrealYAML","yaml-cpp", "src"));
	}
}
