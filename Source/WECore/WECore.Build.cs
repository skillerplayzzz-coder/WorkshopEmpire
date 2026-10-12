using UnrealBuildTool;

// WECore: engine-light foundation shared by every Workshop Empire system
// (money type, log categories, common enums). Must not depend on gameplay modules.
public class WECore : ModuleRules
{
	public WECore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
