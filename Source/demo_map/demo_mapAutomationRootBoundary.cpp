#include "demo_mapAutomationRootBoundary.h"

#if !UE_BUILD_SHIPPING

#include "Misc/Paths.h"

namespace
{
	bool CanonicalizeAbsolutePath(
		const FString& Input,
		FString& OutCanonical,
		const bool bRejectTrailingSeparator)
	{
		OutCanonical.Reset();
		if (Input.IsEmpty()
			|| !Input.Equals(Input.TrimStartAndEnd(), ESearchCase::CaseSensitive)
			|| FPaths::IsRelative(Input))
		{
			return false;
		}

		FString Slashed = Input;
		Slashed.ReplaceInline(TEXT("\\"), TEXT("/"));
		if (bRejectTrailingSeparator
			&& Slashed.Len() > 3
			&& Slashed.EndsWith(TEXT("/"), ESearchCase::CaseSensitive))
		{
			return false;
		}
		TArray<FString> Segments;
		Slashed.ParseIntoArray(Segments, TEXT("/"), true);
		if (Segments.Contains(TEXT(".")) || Segments.Contains(TEXT("..")))
		{
			return false;
		}

		OutCanonical = FPaths::ConvertRelativePathToFull(Input);
		if (!FPaths::CollapseRelativeDirectories(OutCanonical))
		{
			return false;
		}
		FPaths::NormalizeFilename(OutCanonical);
		while (OutCanonical.Len() > 3 && OutCanonical.EndsWith(TEXT("/")))
		{
			OutCanonical.LeftChopInline(1, EAllowShrinking::No);
		}
		return !OutCanonical.IsEmpty() && !FPaths::IsRelative(OutCanonical);
	}

	bool SamePath(const FString& A, const FString& B)
	{
		return !A.IsEmpty() && A.Equals(B, ESearchCase::IgnoreCase);
	}

	bool SameOrChildPath(const FString& Candidate, const FString& Parent)
	{
		return SamePath(Candidate, Parent)
			|| (!Candidate.IsEmpty()
				&& !Parent.IsEmpty()
				&& Candidate.StartsWith(Parent + TEXT("/"), ESearchCase::IgnoreCase));
	}

	bool StrictChildPath(const FString& Candidate, const FString& Parent)
	{
		return !SamePath(Candidate, Parent) && SameOrChildPath(Candidate, Parent);
	}

	TArray<FString> CanonicalizePathArray(const TArray<FString>& Inputs)
	{
		TArray<FString> Result;
		for (const FString& Input : Inputs)
		{
			FString Canonical;
			if (CanonicalizeAbsolutePath(Input, Canonical, false))
			{
				Result.AddUnique(Canonical);
			}
		}
		return Result;
	}

	bool AnyOverlap(
		const TArray<FString>& Candidates,
		const TArray<FString>& ProtectedRoots)
	{
		for (const FString& Candidate : Candidates)
		{
			for (const FString& Protected : ProtectedRoots)
			{
				if (SameOrChildPath(Candidate, Protected)
					|| SameOrChildPath(Protected, Candidate))
				{
					return true;
				}
			}
		}
		return false;
	}
}

FString Fdemo_mapAutomationRootBoundaryResult::Diagnostic() const
{
	return FString::Printf(
		TEXT("root=%s root_absolute=%d leaf_match=%d parent_automation=%d parent_saved=%d project_root_leaf_match=%d uproject_present=%d root_reparse=%d storage_child=%d user_child=%d user_match=%d config_child=%d production_overlap=%d external_delivery_overlap=%d accepted=%d"),
		*CanonicalRoot,
		bRootAbsolute,
		bLeafMatch,
		bParentAutomation,
		bParentSaved,
		bProjectRootLeafMatch,
		bUprojectPresent,
		bRootReparse,
		bStorageChild,
		bUserChild,
		bUserMatch,
		bConfigChild,
		bProductionOverlap,
		bExternalDeliveryOverlap,
		bAccepted);
}

bool Fdemo_mapAutomationRootBoundary::DeriveCanonicalProjectRoot(
	const FString& AutomationRoot,
	FString& OutCanonicalRoot,
	FString& OutCanonicalProjectRoot)
{
	OutCanonicalProjectRoot.Reset();
	if (!CanonicalizeAbsolutePath(AutomationRoot, OutCanonicalRoot, true))
	{
		return false;
	}

	const FString AutomationParent = FPaths::GetPath(OutCanonicalRoot);
	const FString SavedParent = FPaths::GetPath(AutomationParent);
	OutCanonicalProjectRoot = FPaths::GetPath(SavedParent);
	return !OutCanonicalProjectRoot.IsEmpty();
}

Fdemo_mapAutomationRootBoundaryResult Fdemo_mapAutomationRootBoundary::Evaluate(
	const Fdemo_mapAutomationRootBoundaryPaths& Paths,
	const TSet<FString>& ExactTaskLeaves)
{
	Fdemo_mapAutomationRootBoundaryResult Result;
	Result.bRootAbsolute = DeriveCanonicalProjectRoot(
		Paths.AutomationRoot,
		Result.CanonicalRoot,
		Result.CanonicalProjectRoot);

	bool bAllRequiredPathsCanonical = Result.bRootAbsolute;
	bAllRequiredPathsCanonical &= CanonicalizeAbsolutePath(
		Paths.StorageRoot,
		Result.CanonicalStorageRoot,
		false);
	bAllRequiredPathsCanonical &= CanonicalizeAbsolutePath(
		Paths.UserConfigRoot,
		Result.CanonicalUserConfigRoot,
		false);
	bAllRequiredPathsCanonical &= CanonicalizeAbsolutePath(
		Paths.UserDirRoot,
		Result.CanonicalUserDirRoot,
		false);
	bAllRequiredPathsCanonical &= CanonicalizeAbsolutePath(
		Paths.GeneratedConfigRoot,
		Result.CanonicalGeneratedConfigRoot,
		false);

	const FString AutomationParent = FPaths::GetPath(Result.CanonicalRoot);
	const FString SavedParent = FPaths::GetPath(AutomationParent);
	const FString RootLeaf = FPaths::GetCleanFilename(Result.CanonicalRoot);
	for (const FString& ExactLeaf : ExactTaskLeaves)
	{
		Result.bLeafMatch |= RootLeaf.Equals(ExactLeaf, ESearchCase::IgnoreCase);
	}
	Result.bParentAutomation = FPaths::GetCleanFilename(AutomationParent).Equals(
		TEXT("Automation"),
		ESearchCase::IgnoreCase);
	Result.bParentSaved = FPaths::GetCleanFilename(SavedParent).Equals(
		TEXT("Saved"),
		ESearchCase::IgnoreCase);
	const FString ProjectRootLeaf = FPaths::GetCleanFilename(Result.CanonicalProjectRoot);
	Result.bProjectRootLeafMatch =
		ProjectRootLeaf.Equals(TEXT("Dev.D.UE.0.0.5"), ESearchCase::IgnoreCase)
		|| ProjectRootLeaf.Equals(TEXT("Dev.D.UE.0.0.6"), ESearchCase::IgnoreCase)
		|| ProjectRootLeaf.Equals(TEXT("Dev.D.UE.0.0.7"), ESearchCase::IgnoreCase)
		|| ProjectRootLeaf.Equals(TEXT("Dev.D.UE.0.0.9-XFix1"), ESearchCase::IgnoreCase);

	FString ExpectedUproject;
	CanonicalizeAbsolutePath(
		FPaths::Combine(Result.CanonicalProjectRoot, TEXT("demo_map.uproject")),
		ExpectedUproject,
		false);
	const TArray<FString> ExistingFiles = CanonicalizePathArray(Paths.ExistingOrdinaryFiles);
	const TArray<FString> ReparsePaths = CanonicalizePathArray(Paths.ReparsePaths);
	Result.bUprojectPresent = ExistingFiles.ContainsByPredicate(
		[&ExpectedUproject](const FString& Path)
		{
			return SamePath(Path, ExpectedUproject);
		})
		&& !ReparsePaths.ContainsByPredicate(
			[&ExpectedUproject](const FString& Path)
			{
				return SamePath(Path, ExpectedUproject);
			});

	Result.bStorageChild = StrictChildPath(
		Result.CanonicalStorageRoot,
		Result.CanonicalRoot);
	Result.bUserChild = StrictChildPath(
		Result.CanonicalUserConfigRoot,
		Result.CanonicalRoot);
	Result.bUserMatch = SamePath(
		Result.CanonicalUserDirRoot,
		Result.CanonicalUserConfigRoot);
	Result.bConfigChild = StrictChildPath(
		Result.CanonicalGeneratedConfigRoot,
		Result.CanonicalUserConfigRoot);

	const TArray<FString> EvidencePaths = CanonicalizePathArray(Paths.EvidencePaths);
	const bool bAllEvidenceCanonical = EvidencePaths.Num() == Paths.EvidencePaths.Num();
	const bool bEvidenceContained = bAllEvidenceCanonical
		&& !EvidencePaths.ContainsByPredicate(
			[&Result](const FString& Path)
			{
				return !SameOrChildPath(Path, Result.CanonicalRoot);
			});

	TArray<FString> RelevantPaths = {
		Result.CanonicalRoot,
		Result.CanonicalStorageRoot,
		Result.CanonicalUserConfigRoot,
		Result.CanonicalUserDirRoot,
		Result.CanonicalGeneratedConfigRoot,
		Result.CanonicalProjectRoot,
		AutomationParent,
		SavedParent,
		ExpectedUproject
	};
	RelevantPaths.Append(EvidencePaths);
	Result.bRootReparse = ReparsePaths.ContainsByPredicate(
		[&RelevantPaths](const FString& ReparsePath)
		{
			return RelevantPaths.ContainsByPredicate(
				[&ReparsePath](const FString& RelevantPath)
				{
					return SameOrChildPath(RelevantPath, ReparsePath);
				});
		});

	TArray<FString> IsolatedPaths = {
		Result.CanonicalRoot,
		Result.CanonicalStorageRoot,
		Result.CanonicalUserConfigRoot,
		Result.CanonicalUserDirRoot,
		Result.CanonicalGeneratedConfigRoot
	};
	IsolatedPaths.Append(EvidencePaths);
	Result.bProductionOverlap = AnyOverlap(
		IsolatedPaths,
		CanonicalizePathArray(Paths.ProductionRoots));
	Result.bExternalDeliveryOverlap = !bEvidenceContained
		|| AnyOverlap(
			IsolatedPaths,
			CanonicalizePathArray(Paths.ExternalDeliveryRoots));

	Result.bAccepted = bAllRequiredPathsCanonical
		&& Result.bLeafMatch
		&& Result.bParentAutomation
		&& Result.bParentSaved
		&& Result.bProjectRootLeafMatch
		&& Result.bUprojectPresent
		&& !Result.bRootReparse
		&& Result.bStorageChild
		&& Result.bUserChild
		&& Result.bUserMatch
		&& Result.bConfigChild
		&& !Result.bProductionOverlap
		&& !Result.bExternalDeliveryOverlap;
	return Result;
}

#endif
