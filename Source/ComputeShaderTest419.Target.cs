// Copyright (c) 2018 Hirofumi Seo, M.D. at SCIEMENT, Inc.
// http://www.sciement.com
// http://www.sciement.com/tech-blog/

using UnrealBuildTool;
using System.Collections.Generic;

public class ComputeShaderTest419Target : TargetRules
{
	public ComputeShaderTest419Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "ComputeShaderTest419" } );
	}
}
