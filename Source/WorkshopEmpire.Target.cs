using UnrealBuildTool;
using System.Collections.Generic;

public class WorkshopEmpireTarget : TargetRules
{
	public WorkshopEmpireTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "WECore", "WorkshopEmpire" });
	}
}
