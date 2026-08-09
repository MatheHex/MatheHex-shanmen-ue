#pragma once

#include "CoreMinimal.h"

/** Central lightweight sizing policy shared by the 0.0.7 product UI surfaces. */
struct Fdemo_mapUILayoutProfile
{
	int32 ViewportWidth = 1920;
	int32 ViewportHeight = 1080;
	int32 UnifiedGridColumns = 5;
	float OuterPadding = 10.0f;
	float SectionSpacing = 8.0f;
	bool bCompact = false;
};

struct Fdemo_mapUILayoutPolicy
{
	static Fdemo_mapUILayoutProfile Resolve(FIntPoint ViewportSize);
	static bool IsSupportedCommon16By9(FIntPoint ViewportSize);
};
