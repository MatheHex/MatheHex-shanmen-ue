#include "demo_mapUILayoutPolicy.h"

Fdemo_mapUILayoutProfile Fdemo_mapUILayoutPolicy::Resolve(
	FIntPoint ViewportSize)
{
	Fdemo_mapUILayoutProfile Result;
	Result.ViewportWidth = FMath::Max(1, ViewportSize.X);
	Result.ViewportHeight = FMath::Max(1, ViewportSize.Y);
	Result.bCompact = Result.ViewportWidth < 1600;
	Result.UnifiedGridColumns = Result.bCompact ? 4 : 5;
	Result.OuterPadding = Result.bCompact ? 6.0f : 10.0f;
	Result.SectionSpacing = Result.bCompact ? 5.0f : 8.0f;
	return Result;
}

bool Fdemo_mapUILayoutPolicy::IsSupportedCommon16By9(
	FIntPoint ViewportSize)
{
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		return false;
	}
	const float Ratio =
		static_cast<float>(ViewportSize.X)
		/ static_cast<float>(ViewportSize.Y);
	return FMath::IsNearlyEqual(Ratio, 16.0f / 9.0f, 0.02f)
		&& ViewportSize.X >= 1280
		&& ViewportSize.Y >= 720;
}
