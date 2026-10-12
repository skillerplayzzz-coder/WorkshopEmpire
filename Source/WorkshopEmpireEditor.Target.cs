using UnrealBuildTool;
using System.Collections.Generic;

public class WorkshopEmpireEditorTarget : TargetRules
{
	public WorkshopEmpireEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "WECore", "WorkshopEmpire", "WorkshopEmpireTests" });
	}
}
