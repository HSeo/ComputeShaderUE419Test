// Copyright (c) 2018 Hirofumi Seo, M.D. at SCIEMENT, Inc.
// http://www.sciement.com
// http://www.sciement.com/tech-blog/

using UnrealBuildTool;
using System.Collections.Generic;

public class ComputeShaderTest419EditorTarget : TargetRules
{
	public ComputeShaderTest419EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "ComputeShaderTest419" } );
	}
}
