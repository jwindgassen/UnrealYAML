using UnrealBuildTool;

public class UnrealYAMLTarget : TargetRules {
	public UnrealYAMLTarget(TargetInfo Target) : base(Target) {
		Type = UnrealBuildTool.TargetType.Editor;
		bForceEnableExceptions = true;
		bForceEnableObjCExceptions = true;
		bForceEnableRTTI = true;
	}
}
