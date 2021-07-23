#include "UnrealYAML.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "yaml.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformProcess.h"
#endif

#define LOCTEXT_NAMESPACE "FUnrealYAMLModule"

// void FUnrealYAMLModule::StartupModule() {
// 	const FString BaseDir = IPluginManager::Get().FindPlugin("UnrealYAML")->GetBaseDir();
// 	const FString LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/Win64/yaml-cpp.lib"));
//
// 	YamlLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;
// }
//
// void FUnrealYAMLModule::ShutdownModule() {
// 	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
// 	// we call this function before unloading the module.
//
// 	// Free the dll handle
// 	//FPlatformProcess::FreeDllHandle(YAMLLibraryHandle);
// 	YamlLibraryHandle = nullptr;
// }

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealYAMLModule, UnrealYAML)
