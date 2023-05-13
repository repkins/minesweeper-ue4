using UnrealBuildTool;

public class MinesweeperTests : ModuleRules
{
	public MinesweeperTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Minesweeper" });
	}
}
