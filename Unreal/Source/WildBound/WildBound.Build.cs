// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class WildBound : ModuleRules
{
	public WildBound(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// WildBound's runtime environment systems intentionally keep their implementation
		// helpers private to each .cpp file. Unreal unity builds concatenate several .cpp
		// files into a single translation unit, which can make otherwise-private anonymous
		// namespace helpers collide (TownTag, SpawnBox, GetCube, etc.). Compile the module
		// non-unity so every implementation file keeps its proper C++ translation-unit scope.
		bUseUnity = false;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
