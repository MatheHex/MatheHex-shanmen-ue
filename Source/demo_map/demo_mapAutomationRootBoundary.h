#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

struct Fdemo_mapAutomationRootBoundaryPaths
{
	FString AutomationRoot;
	FString StorageRoot;
	FString UserConfigRoot;
	FString UserDirRoot;
	FString GeneratedConfigRoot;
	TArray<FString> ExistingOrdinaryFiles;
	TArray<FString> ReparsePaths;
	TArray<FString> ProductionRoots;
	TArray<FString> ExternalDeliveryRoots;
	TArray<FString> EvidencePaths;
};

struct Fdemo_mapAutomationRootBoundaryResult
{
	FString CanonicalRoot;
	FString CanonicalStorageRoot;
	FString CanonicalUserConfigRoot;
	FString CanonicalUserDirRoot;
	FString CanonicalGeneratedConfigRoot;
	FString CanonicalProjectRoot;
	bool bRootAbsolute = false;
	bool bLeafMatch = false;
	bool bParentAutomation = false;
	bool bParentSaved = false;
	bool bProjectRootLeafMatch = false;
	bool bUprojectPresent = false;
	bool bRootReparse = false;
	bool bStorageChild = false;
	bool bUserChild = false;
	bool bUserMatch = false;
	bool bConfigChild = false;
	bool bProductionOverlap = false;
	bool bExternalDeliveryOverlap = false;
	bool bAccepted = false;

	FString Diagnostic() const;
};

/**
 * Pure Development-only evaluator. Every input is either a path or an exact
 * task-leaf set; runtime ProjectDir, process base, CWD, and package identity are
 * deliberately absent from the decision surface.
 */
class Fdemo_mapAutomationRootBoundary
{
public:
	static bool DeriveCanonicalProjectRoot(
		const FString& AutomationRoot,
		FString& OutCanonicalRoot,
		FString& OutCanonicalProjectRoot);

	static Fdemo_mapAutomationRootBoundaryResult Evaluate(
		const Fdemo_mapAutomationRootBoundaryPaths& Paths,
		const TSet<FString>& ExactTaskLeaves);
};

#endif
