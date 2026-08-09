#include "demo_mapAutomationRootBoundary.h"

#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapProfilePreparationFlow.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace
{
	const TArray<FString>& AcceptedLeaves()
	{
		static const TArray<FString> Leaves = {
			TEXT("Dev.D.UE.0.0.5.P8.0.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.2.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.3.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.4.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.6.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.7.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.8.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.9.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.10.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.11.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.12.r0"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r0"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r1"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r2"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r3"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r4"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r5"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r6"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r7"),
			TEXT("Dev.D.UE.0.0.5.P8.15.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.16.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.17.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.18.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.20.r0"),
			TEXT("Dev.D.UE.0.0.6.F0.0.r1"),
			TEXT("Dev.D.UE.0.0.6.F0.0.r2"),
			TEXT("Dev.D.UE.0.0.6.F0.0.r3"),
			TEXT("Dev.D.UE.0.0.6.F0.0.r4"),
			TEXT("Dev.D.UE.0.0.7.F0.0.r0")
		};
		return Leaves;
	}

	TSet<FString> ExactLeaves()
	{
		TSet<FString> Leaves;
		for (const FString& Leaf : AcceptedLeaves())
		{
			Leaves.Add(Leaf);
		}
		return Leaves;
	}

	const TArray<FString>& StableNegativeSentinels()
	{
		static const TArray<FString> Leaves = {
			TEXT("Dev.D.UE.0.0.5.P8.999.r0"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r999"),
			TEXT("Dev.D.UE.0.0.5.P8.20.r0.suffix"),
			TEXT("prefix.Dev.D.UE.0.0.5.P8.20.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.*.r0")
		};
		return Leaves;
	}

	FString Canonical(const FString& Path)
	{
		FString Result = FPaths::ConvertRelativePathToFull(Path);
		FPaths::CollapseRelativeDirectories(Result);
		FPaths::NormalizeFilename(Result);
		while (Result.Len() > 3 && Result.EndsWith(TEXT("/")))
		{
			Result.LeftChopInline(1, EAllowShrinking::No);
		}
		return Result;
	}

	bool SameOrChild(const FString& Candidate, const FString& Parent)
	{
		const FString CanonicalCandidate = Canonical(Candidate);
		const FString CanonicalParent = Canonical(Parent);
		return CanonicalCandidate.Equals(CanonicalParent, ESearchCase::IgnoreCase)
			|| CanonicalCandidate.StartsWith(
				CanonicalParent + TEXT("/"),
				ESearchCase::IgnoreCase);
	}

	bool ContainsPath(const TArray<FString>& Paths, const FString& Expected)
	{
		const FString CanonicalExpected = Canonical(Expected);
		return Paths.ContainsByPredicate(
			[&CanonicalExpected](const FString& Path)
			{
				return Canonical(Path).Equals(
					CanonicalExpected,
					ESearchCase::IgnoreCase);
			});
	}

	Fdemo_mapAutomationRootBoundaryPaths ValidPaths()
	{
		Fdemo_mapAutomationRootBoundaryPaths Paths;
		const FString ProjectRoot = Canonical(FPaths::ProjectDir());
		Paths.AutomationRoot = FPaths::Combine(
			ProjectRoot,
			TEXT("Saved"),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
		Paths.StorageRoot = FPaths::Combine(Paths.AutomationRoot, TEXT("Tests"), TEXT("Profile"));
		Paths.UserConfigRoot = FPaths::Combine(Paths.AutomationRoot, TEXT("Tests"), TEXT("User"));
		Paths.UserDirRoot = Paths.UserConfigRoot;
		Paths.GeneratedConfigRoot = FPaths::Combine(Paths.UserConfigRoot, TEXT("Config"));
		Paths.ExistingOrdinaryFiles.Add(FPaths::Combine(ProjectRoot, TEXT("demo_map.uproject")));
		Paths.ProductionRoots = {
			FPaths::Combine(ProjectRoot, TEXT("Saved"), TEXT("SaveGames"), TEXT("Shanmen")),
			FPaths::Combine(FPaths::GetPath(ProjectRoot), TEXT("FakeLocalAppData"), TEXT("demo_map"))
		};
		const FString WorkspaceRoot = FPaths::GetPath(ProjectRoot);
		Paths.ExternalDeliveryRoots = {
			FPaths::Combine(WorkspaceRoot, TEXT("Builds"), TEXT("demo_map"), TEXT("Candidate")),
			FPaths::Combine(WorkspaceRoot, TEXT("Builds"), TEXT("demo_map"), TEXT("ManualAcceptance")),
			FPaths::Combine(WorkspaceRoot, TEXT("Builds"), TEXT("demo_map"), TEXT("Latest")),
			FPaths::Combine(WorkspaceRoot, TEXT("Versions"))
		};
		Paths.EvidencePaths = {
			Paths.StorageRoot,
			Paths.UserConfigRoot,
			Paths.GeneratedConfigRoot,
			FPaths::Combine(Paths.AutomationRoot, TEXT("Tests"), TEXT("Boundary.log"))
		};
		return Paths;
	}

	void SetLeaf(Fdemo_mapAutomationRootBoundaryPaths& Paths, const FString& Leaf)
	{
		const FString AutomationParent = FPaths::GetPath(Paths.AutomationRoot);
		Paths.AutomationRoot = FPaths::Combine(AutomationParent, Leaf);
		Paths.StorageRoot = FPaths::Combine(Paths.AutomationRoot, TEXT("Tests"), TEXT("Profile"));
		Paths.UserConfigRoot = FPaths::Combine(Paths.AutomationRoot, TEXT("Tests"), TEXT("User"));
		Paths.UserDirRoot = Paths.UserConfigRoot;
		Paths.GeneratedConfigRoot = FPaths::Combine(Paths.UserConfigRoot, TEXT("Config"));
		Paths.EvidencePaths = {
			Paths.StorageRoot,
			Paths.UserConfigRoot,
			Paths.GeneratedConfigRoot,
			FPaths::Combine(Paths.AutomationRoot, TEXT("Tests"), TEXT("Boundary.log"))
		};
	}

	bool ContainsLeafIgnoreCase(const TArray<FString>& Leaves, const FString& Expected)
	{
		return Leaves.ContainsByPredicate(
			[&Expected](const FString& Leaf)
			{
				return Leaf.Equals(Expected, ESearchCase::IgnoreCase);
			});
	}

	bool HasCaseInsensitiveDuplicates(const TArray<FString>& Leaves)
	{
		for (int32 Left = 0; Left < Leaves.Num(); ++Left)
		{
			for (int32 Right = Left + 1; Right < Leaves.Num(); ++Right)
			{
				if (Leaves[Left].Equals(Leaves[Right], ESearchCase::IgnoreCase))
				{
					return true;
				}
			}
		}
		return false;
	}

	void TestRejectedLeaf(
		FAutomationTestBase& Test,
		Fdemo_mapAutomationRootBoundaryPaths& Paths,
		const FString& Leaf,
		const FString& Context)
	{
		SetLeaf(Paths, Leaf);
		const Fdemo_mapAutomationRootBoundaryResult Result =
			Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
		Test.TestTrue(
			*FString::Printf(TEXT("%s rejects leaf '%s' exactly"), *Context, *Leaf),
			!Result.bAccepted && !Result.bLeafMatch);
	}

	void TestAcceptedLeafContract(
		FAutomationTestBase& Test,
		Fdemo_mapAutomationRootBoundaryPaths& Paths,
		const FString& AcceptedLeaf,
		const FString& Context)
	{
		SetLeaf(Paths, AcceptedLeaf);
		const Fdemo_mapAutomationRootBoundaryResult AcceptedResult =
			Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
		Test.TestTrue(
			*FString::Printf(TEXT("%s accepts its exact leaf"), *Context),
			AcceptedResult.bAccepted && AcceptedResult.bLeafMatch);

		for (const FString& Sentinel : StableNegativeSentinels())
		{
			TestRejectedLeaf(Test, Paths, Sentinel, Context);
		}
		TestRejectedLeaf(Test, Paths, AcceptedLeaf + TEXT(".suffix"), Context);
		TestRejectedLeaf(Test, Paths, TEXT("prefix.") + AcceptedLeaf, Context);
		TestRejectedLeaf(Test, Paths, AcceptedLeaf + TEXT(".*"), Context);
	}

	bool RunBoundaryCase(FAutomationTestBase& Test, const int32 Case)
	{
		Fdemo_mapAutomationRootBoundaryPaths Paths = ValidPaths();
		Fdemo_mapAutomationRootBoundaryResult Result;
		switch (Case)
		{
		case 1:
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Active source Automation Root is accepted"), Result.bAccepted);
			break;
		case 2:
		{
			const auto EditorResult = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			const FString SimulatedPackagedProjectDir =
				FPaths::Combine(FPaths::GetPath(Paths.AutomationRoot), TEXT("Package"), TEXT("Windows"));
			Test.TestFalse(TEXT("Simulated packaged ProjectDir is intentionally outside evaluator inputs"), SimulatedPackagedProjectDir.IsEmpty());
			const auto PackagedContextResult = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Runtime context cannot change an explicit-root decision"),
				EditorResult.bAccepted
				&& PackagedContextResult.bAccepted
				&& EditorResult.Diagnostic() == PackagedContextResult.Diagnostic());
			break;
		}
		case 3:
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.P8.2.r0"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("P8.2 exact leaf remains accepted"), Result.bAccepted);
			break;
		case 4:
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("P8.3 exact leaf is accepted"), Result.bAccepted);
			break;
		case 5:
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.F0.0.r0"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("F0.0.r0 exact leaf remains accepted"), Result.bAccepted);
			break;
		case 6:
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.F0.0.r1"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("F0.0.r1 exact leaf remains accepted"), Result.bAccepted);
			break;
		case 7:
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.F0.0.r2"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("F0.0.r2 exact leaf remains accepted"), Result.bAccepted);
			break;
		case 8:
			TestRejectedLeaf(
				Test,
				Paths,
				StableNegativeSentinels()[0],
				TEXT("Stable P8 future sentinel contract"));
			break;
		case 9:
			TestRejectedLeaf(
				Test,
				Paths,
				StableNegativeSentinels()[2],
				TEXT("Stable suffix sentinel contract"));
			break;
		case 10:
			Paths.AutomationRoot = FPaths::Combine(
				FPaths::GetPath(FPaths::GetPath(Paths.AutomationRoot)),
				TEXT("AutomationWrong"),
				TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Wrong Automation parent is rejected"), !Result.bAccepted && !Result.bParentAutomation);
			break;
		case 11:
		{
			const FString ProjectRoot = Canonical(FPaths::ProjectDir());
			Paths.AutomationRoot = FPaths::Combine(ProjectRoot, TEXT("SavedWrong"), TEXT("Automation"), TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Wrong Saved grandparent is rejected"), !Result.bAccepted && !Result.bParentSaved);
			break;
		}
		case 12:
		{
			const FString WrongProject = FPaths::Combine(FPaths::GetPath(Canonical(FPaths::ProjectDir())), TEXT("WrongProject"));
			Paths.AutomationRoot = FPaths::Combine(WrongProject, TEXT("Saved"), TEXT("Automation"), TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
			Paths.ExistingOrdinaryFiles = { FPaths::Combine(WrongProject, TEXT("demo_map.uproject")) };
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Wrong ProjectRoot leaf is rejected"), !Result.bAccepted && !Result.bProjectRootLeafMatch);
			break;
		}
		case 13:
			Paths.ExistingOrdinaryFiles.Reset();
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Missing uproject is rejected"), !Result.bAccepted && !Result.bUprojectPresent);
			break;
		case 14:
			Paths.AutomationRoot = TEXT("Saved/Automation/Dev.D.UE.0.0.5.P8.3.r0");
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Relative root is rejected"), !Result.bAccepted && !Result.bRootAbsolute);
			break;
		case 15:
			Paths.ReparsePaths.Add(Paths.AutomationRoot);
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Root reparse point is rejected"), !Result.bAccepted && Result.bRootReparse);
			break;
		case 16:
			Paths.ReparsePaths.Add(Canonical(FPaths::ProjectDir()));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Ancestor reparse point is rejected"), !Result.bAccepted && Result.bRootReparse);
			break;
		case 17:
			Paths.StorageRoot = FPaths::Combine(Canonical(FPaths::ProjectDir()), TEXT("Saved"), TEXT("Escape"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Storage escape is rejected"), !Result.bAccepted && !Result.bStorageChild);
			break;
		case 18:
			Paths.UserConfigRoot = FPaths::Combine(Canonical(FPaths::ProjectDir()), TEXT("Saved"), TEXT("UserEscape"));
			Paths.UserDirRoot = Paths.UserConfigRoot;
			Paths.GeneratedConfigRoot = FPaths::Combine(Paths.UserConfigRoot, TEXT("Config"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("User root escape is rejected"), !Result.bAccepted && !Result.bUserChild);
			break;
		case 19:
			Paths.UserDirRoot = FPaths::Combine(Paths.AutomationRoot, TEXT("Tests"), TEXT("OtherUser"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("UserDir mismatch is rejected"), !Result.bAccepted && !Result.bUserMatch);
			break;
		case 20:
			Paths.GeneratedConfigRoot = FPaths::Combine(Paths.AutomationRoot, TEXT("Tests"), TEXT("Other"), TEXT("Config"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Generated Config escape is rejected"), !Result.bAccepted && !Result.bConfigChild);
			break;
		case 21:
			Paths.ProductionRoots.Add(Paths.StorageRoot);
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Production Save overlap is rejected"), !Result.bAccepted && Result.bProductionOverlap);
			break;
		case 22:
			Paths.ProductionRoots.Add(Paths.UserConfigRoot);
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Real user Config overlap is rejected"), !Result.bAccepted && Result.bProductionOverlap);
			break;
		case 23:
			Paths.EvidencePaths.Add(FPaths::Combine(Paths.ExternalDeliveryRoots[0], TEXT("Escaped.log")));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("External delivery evidence path is rejected"), !Result.bAccepted && Result.bExternalDeliveryOverlap);
			break;
		case 24:
		{
			const FString ProjectRoot = Canonical(FPaths::ProjectDir());
			Paths.AutomationRoot = FPaths::Combine(
				ProjectRoot,
				TEXT("Package"),
				TEXT("Windows"),
				TEXT("demo_map"),
				TEXT("Saved"),
				TEXT("Automation"),
				TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.P8.3.r0"));
			Paths.ExistingOrdinaryFiles.Reset();
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("Package-local Saved/Automation is rejected"),
				!Result.bAccepted
					&& !Result.bProjectRootLeafMatch
					&& !Result.bUprojectPresent);
			break;
		}
		case 25:
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.P8.4.r0"));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(TEXT("P8.4 exact leaf is accepted"), Result.bAccepted);
			break;
		case 26:
		{
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.7.F0.0.r0"));
			FString CommandUserDir;
			FParse::Value(FCommandLine::Get(), TEXT("UserDir="), CommandUserDir);
			Paths.UserConfigRoot = CommandUserDir;
			Paths.UserDirRoot = CommandUserDir;
			Paths.GeneratedConfigRoot = FPaths::GeneratedConfigDir();
			Paths.EvidencePaths = {
				Paths.StorageRoot,
				Paths.UserConfigRoot,
				Paths.GeneratedConfigRoot
			};

			const FString ProjectRoot = Canonical(FPaths::ProjectDir());
			Paths.ProductionRoots =
				Fdemo_mapProfilePreparationFlow::ProtectedProductionRootsForAutomation(ProjectRoot);
			const FString RedirectedRuntimeProduction =
				Fdemo_mapProfileStorageContext::Production().RootDirectory;
			const FString DerivedRealProduction = FPaths::Combine(
				ProjectRoot,
				TEXT("Saved"),
				TEXT("SaveGames"),
				TEXT("Shanmen"));
			const FString LocalAppData =
				FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA"));
			const FString RealUserConfig = FPaths::Combine(LocalAppData, TEXT("demo_map"));
			UE_LOG(
				LogTemp,
				Display,
				TEXT("F0_0_R7_RUNTIME_PRODUCTION_CONTEXT runtime=%s user=%s derived=%s"),
				*Canonical(RedirectedRuntimeProduction),
				*Canonical(CommandUserDir),
				*Canonical(DerivedRealProduction));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(
				TEXT("Redirected runtime Production context is task-local in this process"),
				!CommandUserDir.IsEmpty()
					&& SameOrChild(RedirectedRuntimeProduction, CommandUserDir));
			Test.TestTrue(
				TEXT("Redirected runtime Production context is not a protected production fact"),
				!ContainsPath(Paths.ProductionRoots, RedirectedRuntimeProduction));
			Test.TestTrue(
				TEXT("Generated Config remains below the same F0.0.r0 UserDir"),
				!CommandUserDir.IsEmpty()
					&& SameOrChild(Paths.GeneratedConfigRoot, CommandUserDir));
			Test.TestTrue(
				TEXT("Real production Save remains in the protected roots"),
				ContainsPath(Paths.ProductionRoots, DerivedRealProduction));
			Test.TestTrue(
				TEXT("Real local Config remains in the protected roots"),
				!LocalAppData.IsEmpty()
					&& ContainsPath(Paths.ProductionRoots, RealUserConfig));
			Test.TestTrue(
				TEXT("Adapter facts accept the isolated Dev.D.UE.0.0.7.F0.0.r0 task identity"),
				Result.bAccepted
					&& Result.bStorageChild
					&& Result.bUserChild
					&& Result.bUserMatch
					&& Result.bConfigChild
					&& !Result.bProductionOverlap);
			break;
		}
		case 27:
		{
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.F0.0.r5"));
			const FString ProjectRoot = Canonical(FPaths::ProjectDir());
			const FString DerivedProduction = FPaths::Combine(
				ProjectRoot,
				TEXT("Saved"),
				TEXT("SaveGames"),
				TEXT("Shanmen"));
			Paths.ProductionRoots =
				Fdemo_mapProfilePreparationFlow::ProtectedProductionRootsForAutomation(ProjectRoot);
			Test.TestTrue(
				TEXT("Active-project production Save is derived from CanonicalProjectRoot"),
				ContainsPath(Paths.ProductionRoots, DerivedProduction));
			Paths.StorageRoot = DerivedProduction;
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(
				TEXT("Derived active-project production Save remains protected"),
				!Result.bAccepted && Result.bProductionOverlap);
			break;
		}
		case 28:
		{
			SetLeaf(Paths, TEXT("Dev.D.UE.0.0.5.F0.0.r5"));
			const FString LocalAppData =
				FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA"));
			const FString RealUserConfig = FPaths::Combine(LocalAppData, TEXT("demo_map"));
			Paths.UserConfigRoot = RealUserConfig;
			Paths.UserDirRoot = RealUserConfig;
			Paths.GeneratedConfigRoot = FPaths::Combine(RealUserConfig, TEXT("Config"));
			Paths.EvidencePaths = {
				Paths.StorageRoot,
				Paths.UserConfigRoot,
				Paths.GeneratedConfigRoot
			};
			Paths.ProductionRoots =
				Fdemo_mapProfilePreparationFlow::ProtectedProductionRootsForAutomation(
					Canonical(FPaths::ProjectDir()));
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(Paths, ExactLeaves());
			Test.TestTrue(
				TEXT("Real user Config remains a protected root"),
				!LocalAppData.IsEmpty()
					&& !Result.bAccepted
					&& Result.bProductionOverlap);
			break;
		}
		case 29:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.F0.0.r3"),
				TEXT("F0.0.r3 contract"));
			break;
		case 30:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.6.r0"),
				TEXT("P8.6 contract"));
			break;
		case 31:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.7.r0"),
				TEXT("P8.7 contract"));
			break;
		case 32:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.8.r0"),
				TEXT("P8.8 contract"));
			break;
		case 33:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.9.r0"),
				TEXT("P8.9 contract"));
			break;
		case 34:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.10.r0"),
				TEXT("P8.10 contract"));
			break;
		case 35:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.11.r0"),
				TEXT("P8.11 contract"));
			break;
		case 36:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.F0.0.r4"),
				TEXT("F0.0.r4 contract"));
			break;
		case 37:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.12.r0"),
				TEXT("P8.12 contract"));
			break;
		case 38:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.F0.0.r5"),
				TEXT("F0.0.r5 contract"));
			break;
		case 39:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.15.r0"),
				TEXT("P8.15 contract"));
			break;
		case 40:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.16.r0"),
				TEXT("P8.16 contract"));
			break;
		case 41:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.17.r0"),
				TEXT("P8.17 contract"));
			break;
		case 42:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.18.r0"),
				TEXT("P8.18 contract"));
			break;
		case 43:
		{
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.F0.0.r6"),
				TEXT("F0.0.r6 contract"));

			Fdemo_mapAutomationRootBoundaryPaths PackagePaths = ValidPaths();
			const FString ProjectRoot = Canonical(FPaths::ProjectDir());
			PackagePaths.AutomationRoot = FPaths::Combine(
				ProjectRoot,
				TEXT("Package"),
				TEXT("Windows"),
				TEXT("demo_map"),
				TEXT("Saved"),
				TEXT("Automation"),
				TEXT("Dev.D.UE.0.0.5.F0.0.r6"));
			SetLeaf(PackagePaths, TEXT("Dev.D.UE.0.0.5.F0.0.r6"));
			PackagePaths.ExistingOrdinaryFiles.Reset();
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(PackagePaths, ExactLeaves());
			Test.TestTrue(
				TEXT("F0.0.r6 package-local Saved/Automation is rejected"),
				!Result.bAccepted
					&& !Result.bProjectRootLeafMatch
					&& !Result.bUprojectPresent);

			Fdemo_mapAutomationRootBoundaryPaths ProductionPaths = ValidPaths();
			SetLeaf(ProductionPaths, TEXT("Dev.D.UE.0.0.5.F0.0.r6"));
			ProductionPaths.ProductionRoots.Add(ProductionPaths.StorageRoot);
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(ProductionPaths, ExactLeaves());
			Test.TestTrue(
				TEXT("F0.0.r6 production Save overlap is rejected"),
				!Result.bAccepted && Result.bProductionOverlap);

			Fdemo_mapAutomationRootBoundaryPaths RealConfigPaths = ValidPaths();
			SetLeaf(RealConfigPaths, TEXT("Dev.D.UE.0.0.5.F0.0.r6"));
			RealConfigPaths.ProductionRoots.Add(RealConfigPaths.UserConfigRoot);
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(RealConfigPaths, ExactLeaves());
			Test.TestTrue(
				TEXT("F0.0.r6 real user Config overlap is rejected"),
				!Result.bAccepted && Result.bProductionOverlap);
			break;
		}
		case 44:
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.P8.20.r0"),
				TEXT("P8.20 exact-leaf contract"));
			break;
		case 45:
		{
			const TArray<FString>& Accepted = AcceptedLeaves();
			const TArray<FString>& Negative = StableNegativeSentinels();
			Test.TestFalse(
				TEXT("Accepted leaf table has no case-insensitive duplicates"),
				HasCaseInsensitiveDuplicates(Accepted));
			Test.TestFalse(
				TEXT("Negative sentinel table has no case-insensitive duplicates"),
				HasCaseInsensitiveDuplicates(Negative));

			bool bIntersectionFound = false;
			for (const FString& AcceptedLeaf : Accepted)
			{
				for (const FString& NegativeLeaf : Negative)
				{
					bIntersectionFound |= AcceptedLeaf.Equals(
						NegativeLeaf,
						ESearchCase::IgnoreCase);
				}
			}
			Test.TestFalse(
				TEXT("Accepted leaves and negative sentinels are disjoint"),
				bIntersectionFound);

			for (const FString& AcceptedLeaf : Accepted)
			{
				Fdemo_mapAutomationRootBoundaryPaths AcceptedPaths = ValidPaths();
				SetLeaf(AcceptedPaths, AcceptedLeaf);
				const Fdemo_mapAutomationRootBoundaryResult AcceptedResult =
					Fdemo_mapAutomationRootBoundary::Evaluate(
						AcceptedPaths,
						ExactLeaves());
				Test.TestTrue(
					*FString::Printf(
						TEXT("Accepted table leaf '%s' evaluates accepted"),
						*AcceptedLeaf),
					AcceptedResult.bAccepted && AcceptedResult.bLeafMatch);
			}
			for (const FString& NegativeLeaf : Negative)
			{
				Fdemo_mapAutomationRootBoundaryPaths NegativePaths = ValidPaths();
				SetLeaf(NegativePaths, NegativeLeaf);
				const Fdemo_mapAutomationRootBoundaryResult NegativeResult =
					Fdemo_mapAutomationRootBoundary::Evaluate(
						NegativePaths,
						ExactLeaves());
				Test.TestTrue(
					*FString::Printf(
						TEXT("Negative table leaf '%s' evaluates rejected"),
						*NegativeLeaf),
					!NegativeResult.bAccepted && !NegativeResult.bLeafMatch);
			}

			Test.TestTrue(
				TEXT("F0.0.r6 is explicitly present in the accepted table"),
				ContainsLeafIgnoreCase(
					Accepted,
					TEXT("Dev.D.UE.0.0.5.F0.0.r6")));
			Test.TestTrue(
				TEXT("P8.20 is explicitly present in the accepted table"),
				ContainsLeafIgnoreCase(
					Accepted,
					TEXT("Dev.D.UE.0.0.5.P8.20.r0")));
			Test.TestTrue(
				TEXT("F0.0.r7 is explicitly present in the accepted table"),
				ContainsLeafIgnoreCase(
					Accepted,
					TEXT("Dev.D.UE.0.0.5.F0.0.r7")));
			Test.TestTrue(
				TEXT("Stable P8 sentinel is explicitly present in the negative table"),
				ContainsLeafIgnoreCase(
					Negative,
					TEXT("Dev.D.UE.0.0.5.P8.999.r0")));
			Test.TestTrue(
				TEXT("Stable F sentinel is explicitly present in the negative table"),
				ContainsLeafIgnoreCase(
					Negative,
					TEXT("Dev.D.UE.0.0.5.F0.0.r999")));
			break;
		}
		case 46:
		{
			TestAcceptedLeafContract(
				Test,
				Paths,
				TEXT("Dev.D.UE.0.0.5.F0.0.r7"),
				TEXT("F0.0.r7 exact-leaf contract"));

			Fdemo_mapAutomationRootBoundaryPaths PackagePaths = ValidPaths();
			const FString ProjectRoot = Canonical(FPaths::ProjectDir());
			PackagePaths.AutomationRoot = FPaths::Combine(
				ProjectRoot,
				TEXT("Package"),
				TEXT("Windows"),
				TEXT("demo_map"),
				TEXT("Saved"),
				TEXT("Automation"),
				TEXT("Dev.D.UE.0.0.5.F0.0.r7"));
			SetLeaf(PackagePaths, TEXT("Dev.D.UE.0.0.5.F0.0.r7"));
			PackagePaths.ExistingOrdinaryFiles.Reset();
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(
				PackagePaths,
				ExactLeaves());
			Test.TestTrue(
				TEXT("F0.0.r7 package-local Saved/Automation is rejected"),
				!Result.bAccepted
					&& !Result.bProjectRootLeafMatch
					&& !Result.bUprojectPresent);

			Fdemo_mapAutomationRootBoundaryPaths ProductionPaths = ValidPaths();
			SetLeaf(ProductionPaths, TEXT("Dev.D.UE.0.0.5.F0.0.r7"));
			ProductionPaths.ProductionRoots.Add(ProductionPaths.StorageRoot);
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(
				ProductionPaths,
				ExactLeaves());
			Test.TestTrue(
				TEXT("F0.0.r7 production Save overlap is rejected"),
				!Result.bAccepted && Result.bProductionOverlap);

			Fdemo_mapAutomationRootBoundaryPaths RealConfigPaths = ValidPaths();
			SetLeaf(RealConfigPaths, TEXT("Dev.D.UE.0.0.5.F0.0.r7"));
			RealConfigPaths.ProductionRoots.Add(RealConfigPaths.UserConfigRoot);
			Result = Fdemo_mapAutomationRootBoundary::Evaluate(
				RealConfigPaths,
				ExactLeaves());
			Test.TestTrue(
				TEXT("F0.0.r7 real user Config overlap is rejected"),
				!Result.bAccepted && Result.bProductionOverlap);
			break;
		}
		default:
			Test.AddError(TEXT("Unknown AutomationRootBoundary test case."));
			break;
		}
		return true;
	}
}

#define DEMO_MAP_ROOT_TEST(ClassName, TestName, CaseNumber) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, TestName, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool ClassName::RunTest(const FString&) { return RunBoundaryCase(*this, CaseNumber); }

DEMO_MAP_ROOT_TEST(FRootBoundary01, "demo_map.AutomationRootBoundary.01.ActiveSourceRootAccepted", 1)
DEMO_MAP_ROOT_TEST(FRootBoundary02, "demo_map.AutomationRootBoundary.02.PackagedRuntimeContextDoesNotChangeDecision", 2)
DEMO_MAP_ROOT_TEST(FRootBoundary03, "demo_map.AutomationRootBoundary.03.P8_2ExactLeafPreserved", 3)
DEMO_MAP_ROOT_TEST(FRootBoundary04, "demo_map.AutomationRootBoundary.04.P8_3ExactLeafAccepted", 4)
DEMO_MAP_ROOT_TEST(FRootBoundary05, "demo_map.AutomationRootBoundary.05.F0_0_r0ExactLeafPreserved", 5)
DEMO_MAP_ROOT_TEST(FRootBoundary06, "demo_map.AutomationRootBoundary.06.F0_0_r1ExactLeafPreserved", 6)
DEMO_MAP_ROOT_TEST(FRootBoundary07, "demo_map.AutomationRootBoundary.07.F0_0_r2ExactLeafPreserved", 7)
DEMO_MAP_ROOT_TEST(FRootBoundary08, "demo_map.AutomationRootBoundary.08.FutureRevisionRejected", 8)
DEMO_MAP_ROOT_TEST(FRootBoundary09, "demo_map.AutomationRootBoundary.09.WildcardOrPrefixMatchRejected", 9)
DEMO_MAP_ROOT_TEST(FRootBoundary10, "demo_map.AutomationRootBoundary.10.WrongAutomationParentRejected", 10)
DEMO_MAP_ROOT_TEST(FRootBoundary11, "demo_map.AutomationRootBoundary.11.WrongSavedGrandparentRejected", 11)
DEMO_MAP_ROOT_TEST(FRootBoundary12, "demo_map.AutomationRootBoundary.12.WrongProjectRootLeafRejected", 12)
DEMO_MAP_ROOT_TEST(FRootBoundary13, "demo_map.AutomationRootBoundary.13.MissingUprojectRejected", 13)
DEMO_MAP_ROOT_TEST(FRootBoundary14, "demo_map.AutomationRootBoundary.14.RelativePathRejected", 14)
DEMO_MAP_ROOT_TEST(FRootBoundary15, "demo_map.AutomationRootBoundary.15.RootReparseRejected", 15)
DEMO_MAP_ROOT_TEST(FRootBoundary16, "demo_map.AutomationRootBoundary.16.AncestorReparseRejected", 16)
DEMO_MAP_ROOT_TEST(FRootBoundary17, "demo_map.AutomationRootBoundary.17.StorageEscapeRejected", 17)
DEMO_MAP_ROOT_TEST(FRootBoundary18, "demo_map.AutomationRootBoundary.18.UserRootEscapeRejected", 18)
DEMO_MAP_ROOT_TEST(FRootBoundary19, "demo_map.AutomationRootBoundary.19.UserDirMismatchRejected", 19)
DEMO_MAP_ROOT_TEST(FRootBoundary20, "demo_map.AutomationRootBoundary.20.GeneratedConfigEscapeRejected", 20)
DEMO_MAP_ROOT_TEST(FRootBoundary21, "demo_map.AutomationRootBoundary.21.ProductionSaveOverlapRejected", 21)
DEMO_MAP_ROOT_TEST(FRootBoundary22, "demo_map.AutomationRootBoundary.22.RealUserConfigRejected", 22)
DEMO_MAP_ROOT_TEST(FRootBoundary23, "demo_map.AutomationRootBoundary.23.ExternalDeliveryTreeRejected", 23)
DEMO_MAP_ROOT_TEST(FRootBoundary24, "demo_map.AutomationRootBoundary.24.PackageLocalSavedAutomationRejected", 24)
DEMO_MAP_ROOT_TEST(FRootBoundary25, "demo_map.AutomationRootBoundary.25.P8_4ExactLeafAccepted", 25)
DEMO_MAP_ROOT_TEST(FRootBoundary26, "demo_map.AutomationRootBoundary.26.RedirectedRuntimeProductionContextIgnoredAsProtectedFact", 26)
DEMO_MAP_ROOT_TEST(FRootBoundary27, "demo_map.AutomationRootBoundary.27.DerivedActiveProjectProductionSaveStillProtected", 27)
DEMO_MAP_ROOT_TEST(FRootBoundary28, "demo_map.AutomationRootBoundary.28.AdapterAcceptsTaskLocalUserDirAndRejectsRealUserConfig", 28)
DEMO_MAP_ROOT_TEST(FRootBoundary29, "demo_map.AutomationRootBoundary.29.F0_0_r3ExactLeafAccepted", 29)
DEMO_MAP_ROOT_TEST(FRootBoundary30, "demo_map.AutomationRootBoundary.30.P8_6ExactLeafAccepted", 30)
DEMO_MAP_ROOT_TEST(FRootBoundary31, "demo_map.AutomationRootBoundary.31.P8_7ExactLeafAccepted", 31)
DEMO_MAP_ROOT_TEST(FRootBoundary32, "demo_map.AutomationRootBoundary.32.P8_8ExactLeafAccepted", 32)
DEMO_MAP_ROOT_TEST(FRootBoundary33, "demo_map.AutomationRootBoundary.33.P8_9ExactLeafAccepted", 33)
DEMO_MAP_ROOT_TEST(FRootBoundary34, "demo_map.AutomationRootBoundary.34.P8_10ExactLeafAccepted", 34)
DEMO_MAP_ROOT_TEST(FRootBoundary35, "demo_map.AutomationRootBoundary.35.P8_11ExactLeafAccepted", 35)
DEMO_MAP_ROOT_TEST(FRootBoundary36, "demo_map.AutomationRootBoundary.36.F0_0_r4ExactLeafAccepted", 36)
DEMO_MAP_ROOT_TEST(FRootBoundary37, "demo_map.AutomationRootBoundary.37.P8_12ExactLeafAccepted", 37)
DEMO_MAP_ROOT_TEST(FRootBoundary38, "demo_map.AutomationRootBoundary.38.F0_0_r5ExactLeafAccepted", 38)
DEMO_MAP_ROOT_TEST(FRootBoundary39, "demo_map.AutomationRootBoundary.39.P8_15ExactLeafAccepted", 39)
DEMO_MAP_ROOT_TEST(FRootBoundary40, "demo_map.AutomationRootBoundary.40.P8_16ExactLeafAccepted", 40)
DEMO_MAP_ROOT_TEST(FRootBoundary41, "demo_map.AutomationRootBoundary.41.P8_17ExactLeafAccepted", 41)
DEMO_MAP_ROOT_TEST(FRootBoundary42, "demo_map.AutomationRootBoundary.42.P8_18ExactLeafAccepted", 42)
DEMO_MAP_ROOT_TEST(FRootBoundary43, "demo_map.AutomationRootBoundary.43.F0_0_r6ExactLeafAccepted", 43)
DEMO_MAP_ROOT_TEST(FRootBoundary44, "demo_map.AutomationRootBoundary.44.P8_20ExactLeafAccepted", 44)
DEMO_MAP_ROOT_TEST(FRootBoundary45, "demo_map.AutomationRootBoundary.45.AcceptedLeavesAndNegativeSentinelsAreDisjoint", 45)
DEMO_MAP_ROOT_TEST(FRootBoundary46, "demo_map.AutomationRootBoundary.46.F0_0_r7ExactLeafAccepted", 46)

#undef DEMO_MAP_ROOT_TEST

#endif
