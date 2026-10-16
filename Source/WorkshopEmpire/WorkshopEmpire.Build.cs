using UnrealBuildTool;

// WorkshopEmpire: primary game module (game framework classes and, from Phase 1 on, gameplay systems).
public class WorkshopEmpire : ModuleRules
{
	public WorkshopEmpire(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"NetCore",
			"AIModule",
			"NavigationSystem",
			"WECore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UMG",
			"CoreOnline",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"EngineSettings",
			"Sockets",
			"Slate",
			"SlateCore"
		});
	}
}
