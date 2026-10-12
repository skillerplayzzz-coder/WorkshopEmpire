using UnrealBuildTool;

// WorkshopEmpireTests: editor-only automation tests (Session Frontend > Automation, filter "WorkshopEmpire").
public class WorkshopEmpireTests : ModuleRules
{
	public WorkshopEmpireTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"WECore",
			"WorkshopEmpire"
		});
	}
}
