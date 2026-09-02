#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenMeridianShockTreatmentProductRoute.h"

#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenCombatConditionComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenItemCutover.h"
#include "demo_mapShanmenMeridianShockTreatmentInputAdapter.h"
#include "demo_mapShanmenMeridianShockTreatmentProductLifecycle.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "ShanmenVitalityAuthority.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr EAutomationTestFlags RouteFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TargetEntityId(0xC1610001, 0, 0, 1);
	const FGuid ImpactId(0xC1610002, 0, 0, 1);
	const FGuid ResolutionId(0xC1610003, 0, 0, 1);

	FString NewRouteRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P16.1.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenVitalityCommitReceipt MakeCommittedReceipt()
	{
		FShanmenVitalityAuthority Authority;
		check(FShanmenVitalityAuthority::TryCreate(
			TargetEntityId,
			100.0f,
			100.0f,
			0,
			Authority));
		FShanmenVitalityCommitCommand Command;
		check(FShanmenVitalityCommitCommand::TryRestoreFromDurableIntent(
			ImpactId,
			ResolutionId,
			TargetEntityId,
			0,
			100.0f,
			100.0f,
			10.0f,
			0.0f,
			10.0f,
			EShanmenDefenseOutcome::Applied,
			Command));
		const FShanmenVitalityCommitResult Result = Authority.Commit(Command);
		check(Result.Status == EShanmenVitalityCommitStatus::Committed);
		return Result.Receipt;
	}

	struct FTreatmentRouteFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid TreatmentItemId;
		FGuid GenericPillId;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		Fdemo_map0909BSectWarehouseService Warehouse;
		Fdemo_mapShanmenRunCorrelation Correlation;
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Udemo_mapShanmenCombatConditionComponent* Conditions = nullptr;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
		Fdemo_mapShanmenMeridianShockTreatmentProductRoute Route;

		bool StartGameInstance(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for the P16.1 fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(
				GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Could not allocate the P16.1 GameInstance."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			Authority = GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			Session = GameInstance->GetSubsystem<
				Udemo_mapProfileSessionSubsystem>();
			return Authority && Session;
		}

		bool Build(
			FAutomationTestBase& Test,
			const TCHAR* Label,
			const bool bBindRoute = true)
		{
			Root = NewRouteRoot(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			FGuid TrainingBladeId;
			for (const Fdemo_mapPersistentItemRecord& Item :
				SeedProfile.PermanentStash)
			{
				if (Item.ItemDefinitionId
					== Fdemo_mapItemIds::TrainingBlade)
				{
					TrainingBladeId = Item.ItemInstanceId;
					break;
				}
			}
			Fdemo_mapPersistentItemRecord TreatmentItem;
			TreatmentItemId = TreatmentItem.ItemInstanceId = FGuid::NewGuid();
			TreatmentItem.ItemDefinitionId =
				Fdemo_mapItemIds::MeridianStabilizingPillLevel1;
			TreatmentItem.StackCount = 3;
			TreatmentItem.PersistentDomain =
				Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(TreatmentItem);
			Fdemo_mapPersistentItemRecord GenericPill;
			GenericPillId = GenericPill.ItemInstanceId = FGuid::NewGuid();
			GenericPill.ItemDefinitionId =
				Fdemo_mapItemIds::HealingPillLevel1;
			GenericPill.StackCount = 3;
			GenericPill.PersistentDomain =
				Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(GenericPill);
			SeedProfile.PreparationLayout.WeaponItemInstanceId =
				TrainingBladeId;
			const Fdemo_mapProfileSaveResult Saved =
				Repository.SaveProfile(SeedProfile, Storage);
			if (!TrainingBladeId.IsValid() || !Saved.IsSuccess()
				|| !StartGameInstance(Test))
			{
				Test.AddError(FString::Printf(
					TEXT("P16.1 seed failed: %s"), *Saved.Diagnostic));
				return false;
			}

			const Fdemo_mapProfileSessionInitializeResult Initialized =
				Session->InitializeSession(Storage);
			Fdemo_map0909BWarehousePresentation Presentation;
			FString Diagnostic;
			if (!Initialized.IsReady()
				|| !Warehouse.OpenForSect(
					Root,
					Initialized.Snapshot,
					Edemo_map0909BTopState::AtSect,
					Presentation,
					Diagnostic)
				|| !Fdemo_mapShanmenItemCutoverCoordinator::Execute(
					Storage,
					SeedProfile.ProfileId,
					*Authority,
					*Session,
					Warehouse).IsReady()
				|| !Session->SetPreparationMaterial(
					TreatmentItemId, true).IsAccepted()
				|| !Session->SetPreparationMaterial(
					GenericPillId, true).IsAccepted()
				|| !Session->SetPreparationHotbarSlot(
					1, TreatmentItemId).IsAccepted()
				|| !Session->SetPreparationHotbarSlot(
					2, GenericPillId).IsAccepted())
			{
				Test.AddError(FString::Printf(
					TEXT("P16.1 cutover or preparation failed: %s"),
					*Diagnostic));
				return false;
			}

			Udemo_mapItemSubsystem* Runtime =
				GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
			const Fdemo_mapShanmenRunStartResult Started = Runtime
				? Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
					*Authority, *Runtime)
				: Fdemo_mapShanmenRunStartResult();
			if (!Started.IsStarted() || !Started.RunCorrelation.IsValid())
			{
				Test.AddError(TEXT("P16.1 active Run could not start."));
				return false;
			}
			Correlation = Started.RunCorrelation;

			Attributes = NewObject<Udemo_mapAttributeComponent>(
				GetTransientPackage());
			Conditions = NewObject<
				Udemo_mapShanmenCombatConditionComponent>(
					GetTransientPackage());
			if (!Attributes || !Conditions)
			{
				Test.AddError(TEXT("P16.1 condition objects could not allocate."));
				return false;
			}
			Attributes->AddToRoot();
			Conditions->AddToRoot();
			const bool bConditionReady =
				Timeline.TryBegin(Correlation.ActiveRunId, Diagnostic)
				&& Conditions->TryBegin(
					Correlation.ActiveRunId,
					TargetEntityId,
					Timeline.GetTimelineId(),
					Attributes,
					Diagnostic)
				&& Timeline.TryCapture(TimelineSample)
				&& Conditions->TryApplyMeridianShock(
					MakeCommittedReceipt(),
					TimelineSample).IsSuccess();
			return bConditionReady
				&& (!bBindRoute
					|| Route.TryBegin(
						Correlation, Conditions, Diagnostic));
		}

		bool RestartAuthority(FAutomationTestBase& Test)
		{
			StopGameInstance();
			if (!StartGameInstance(Test)
				|| !Session->InitializeSession(Storage).IsReady())
			{
				return false;
			}
			const Fdemo_mapShanmenItemAuthorityBindResult Bound =
				Authority->BindExisting(Storage, SeedProfile.ProfileId);
			if (!Bound.IsReady())
			{
				Test.AddError(FString::Printf(
					TEXT("P16.1 authority restart failed: %s"),
					*Bound.Diagnostic));
				return false;
			}
			return true;
		}

		void StopGameInstance()
		{
			if (!GameInstance)
			{
				return;
			}
			GameInstance->Shutdown();
			Authority = nullptr;
			Session = nullptr;
			GameInstance->RemoveFromRoot();
			GameInstance->MarkAsGarbage();
			GameInstance = nullptr;
			CollectGarbage(RF_NoFlags);
		}

		int32 CountTreatmentFinalizations(bool bCommit) const
		{
			FShanmenItemAuthoritySnapshot Snapshot;
			if (!Authority || !Authority->TryCaptureSnapshot(Snapshot))
			{
				return INDEX_NONE;
			}
			int32 Count = 0;
			for (const FShanmenItemProcessedRequestSnapshot& Processed :
				Snapshot.ProcessedRequests)
			{
				Count += Processed.Receipt.IsSuccess()
					&& Processed.Receipt.Operation
						== EShanmenItemTransactionOperation::
							FinalizePreparedRunQuantityIntent
					&& Processed.Receipt.ItemInstanceId == TreatmentItemId
					&& (Processed.Receipt.Phase
							== EShanmenItemTransactionPhase::Committed)
						== bCommit
					? 1 : 0;
			}
			return Count;
		}

		~FTreatmentRouteFixture()
		{
			StopGameInstance();
			if (Conditions)
			{
				Conditions->RemoveFromRoot();
				Conditions = nullptr;
			}
			if (Attributes)
			{
				Attributes->RemoveFromRoot();
				Attributes = nullptr;
			}
			CollectGarbage(RF_NoFlags);
			if (!Root.IsEmpty())
			{
				IFileManager::Get().DeleteDirectory(*Root, false, true);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentRouteCommitTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Route.OrderedCommitReplay",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentRouteCommitTest::RunTest(const FString&)
{
	FTreatmentRouteFixture Fixture;
	if (!Fixture.Build(*this, TEXT("OrderedCommitReplay")))
	{
		AddError(TEXT("Could not build the P16.1 ordered route fixture."));
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentCommand Command;
	FString Diagnostic;
	TestTrue(TEXT("route captures one immutable live treatment command"),
		Fixture.Route.TryCaptureCommand(
			FGuid(0xC1610010, 0, 0, 1),
			Fixture.TreatmentItemId,
			Fixture.TimelineSample,
			Command,
			Diagnostic));
	const Fdemo_mapShanmenMeridianShockTreatmentRouteResult Committed =
		Fixture.Route.TryExecute(*Fixture.Authority, Command);
	FShanmenItemAuthoritySnapshot BeforeReplay;
	Fixture.Authority->TryCaptureSnapshot(BeforeReplay);
	const Fdemo_mapShanmenMeridianShockTreatmentRouteResult Replay =
		Fixture.Route.TryExecute(*Fixture.Authority, Command);
	AddInfo(FString::Printf(
		TEXT("P16.1 ordered status=%d error=%d item=%d treatment=%d/%d replay=%d diagnostic=%s"),
		static_cast<int32>(Committed.Status),
		static_cast<int32>(Committed.Error),
		static_cast<int32>(Committed.Item.Status),
		static_cast<int32>(Committed.Treatment.Status),
		static_cast<int32>(Committed.Treatment.Error),
		static_cast<int32>(Replay.Status),
		*Committed.Diagnostic));
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("sole route prepares, treats and commits exactly once"),
		Committed.IsCommitted()
			&& Committed.Status
				== Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Committed
			&& Committed.Item.FinalizeCommand.Receipt.ResourceBefore == 3
			&& Committed.Item.FinalizeCommand.Receipt.ResourceAfter == 2
			&& !Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Conditions->GetConditionRevision() == 2
			&& Fixture.CountTreatmentFinalizations(true) == 1
			&& Fixture.CountTreatmentFinalizations(false) == 0);
	TestTrue(TEXT("exact request replay mutates neither authority"),
		Replay.IsCommitted()
			&& Replay.Status
				== Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Replayed
			&& Replay.bReusedRequest
			&& BeforeReplay == AfterReplay
			&& Fixture.Route.NumJournaledCommands() == 1);
	TestTrue(TEXT("resolved route can end and forget only transient journal"),
		Fixture.Route.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Fixture.Route.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentRouteRecoveryTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Route.CommitOnlyRecovery",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentRouteRecoveryTest::RunTest(const FString&)
{
	FTreatmentRouteFixture Fixture;
	if (!Fixture.Build(*this, TEXT("CommitOnlyRecovery")))
	{
		AddError(TEXT("Could not build the P16.1 recovery route fixture."));
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentCommand Command;
	FString Diagnostic;
	if (!Fixture.Route.TryCaptureCommand(
		FGuid(0xC1610020, 0, 0, 1),
		Fixture.TreatmentItemId,
		Fixture.TimelineSample,
		Command,
		Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	Fixture.Route.SetInterruptAfterTreatmentForAutomation(true);
	const Fdemo_mapShanmenMeridianShockTreatmentRouteResult Interrupted =
		Fixture.Route.TryExecute(*Fixture.Authority, Command);
	AddInfo(FString::Printf(
		TEXT("P16.1 interrupted status=%d error=%d item=%d treatment=%d/%d diagnostic=%s"),
		static_cast<int32>(Interrupted.Status),
		static_cast<int32>(Interrupted.Error),
		static_cast<int32>(Interrupted.Item.Status),
		static_cast<int32>(Interrupted.Treatment.Status),
		static_cast<int32>(Interrupted.Treatment.Error),
		*Interrupted.Diagnostic));
	TestTrue(TEXT("post-treatment interruption retains commit-only recovery"),
		Interrupted.RequiresRecovery()
			&& Interrupted.Error
				== Edemo_mapShanmenMeridianShockTreatmentRouteError::
					InterruptedAfterTreatment
			&& Interrupted.Treatment.IsSuccess()
			&& !Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Route.HasUnresolvedRecovery()
			&& !Fixture.Route.TryEnd(
				Fixture.Correlation.ActiveRunId, Diagnostic));

	Fixture.Route.SetInterruptAfterTreatmentForAutomation(false);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapShanmenMeridianShockTreatmentRouteResult FailedCommit =
		Fixture.Route.TryExecute(*Fixture.Authority, Command);
	AddInfo(FString::Printf(
		TEXT("P16.1 failed commit status=%d error=%d item=%d treatment=%d/%d lifecycle=%d diagnostic=%s"),
		static_cast<int32>(FailedCommit.Status),
		static_cast<int32>(FailedCommit.Error),
		static_cast<int32>(FailedCommit.Item.Status),
		static_cast<int32>(FailedCommit.Treatment.Status),
		static_cast<int32>(FailedCommit.Treatment.Error),
		static_cast<int32>(Fixture.Authority->GetLifecycleState()),
		*FailedCommit.Diagnostic));
	TestTrue(TEXT("item persistence failure cannot reopen cancellation"),
		FailedCommit.RequiresRecovery()
			&& FailedCommit.Error
				== Edemo_mapShanmenMeridianShockTreatmentRouteError::
					CommitRejected
			&& FailedCommit.Treatment.IsSuccess()
			&& Fixture.CountTreatmentFinalizations(false) == 0);

	if (!Fixture.RestartAuthority(*this))
	{
		return false;
	}
	const Fdemo_mapShanmenMeridianShockTreatmentRouteResult Recovered =
		Fixture.Route.TryExecute(*Fixture.Authority, Command);
	AddInfo(FString::Printf(
		TEXT("P16.1 recovered status=%d error=%d item=%d treatment=%d/%d diagnostic=%s"),
		static_cast<int32>(Recovered.Status),
		static_cast<int32>(Recovered.Error),
		static_cast<int32>(Recovered.Item.Status),
		static_cast<int32>(Recovered.Treatment.Status),
		static_cast<int32>(Recovered.Treatment.Error),
		*Recovered.Diagnostic));
	TestTrue(TEXT("exact retry after authority restart commits one item"),
		Recovered.IsCommitted()
			&& Recovered.bReusedRequest
			&& Recovered.Item.FinalizeCommand.Receipt.ResourceBefore == 3
			&& Recovered.Item.FinalizeCommand.Receipt.ResourceAfter == 2
			&& Fixture.CountTreatmentFinalizations(true) == 1
			&& Fixture.CountTreatmentFinalizations(false) == 0
			&& !Fixture.Route.HasUnresolvedRecovery());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentRouteCancelConflictTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Route.CancelAndConflict",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentRouteCancelConflictTest::RunTest(
	const FString&)
{
	FTreatmentRouteFixture Fixture;
	if (!Fixture.Build(*this, TEXT("CancelAndConflict")))
	{
		AddError(TEXT("Could not build the P16.1 cancel route fixture."));
		return false;
	}
	const FGuid RequestId(0xC1610030, 0, 0, 1);
	FString Diagnostic;
	Fdemo_mapShanmenMeridianShockTreatmentCommand Original;
	if (!Fixture.Route.TryCaptureCommand(
		RequestId,
		Fixture.TreatmentItemId,
		Fixture.TimelineSample,
		Original,
		Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}

	int64 AdvancedTicks = 0;
	Fdemo_mapShanmenCombatRunTimelineSample CurrentSample;
	Fdemo_mapShanmenMeridianShockTreatmentCommand Conflicting;
	const bool bAdvanced = Fixture.Timeline.TryAdvance(
		1.0 / 30.0, AdvancedTicks, Diagnostic)
		&& Fixture.Timeline.TryCapture(CurrentSample)
		&& Fixture.Conditions->TryAdvance(CurrentSample).IsSuccess()
		&& Fixture.Route.TryCaptureCommand(
			RequestId,
			Fixture.TreatmentItemId,
			CurrentSample,
			Conflicting,
			Diagnostic);
	TestTrue(TEXT("same RequestId can expose a changed timeline payload"),
		bAdvanced && AdvancedTicks == 1
			&& !Original.Matches(Conflicting));

	const Fdemo_mapShanmenMeridianShockTreatmentRouteResult Cancelled =
		Fixture.Route.TryExecute(*Fixture.Authority, Original);
	const Fdemo_mapShanmenMeridianShockTreatmentRouteResult Conflict =
		Fixture.Route.TryExecute(*Fixture.Authority, Conflicting);
	const Fdemo_mapShanmenMeridianShockTreatmentRouteResult CancelReplay =
		Fixture.Route.TryExecute(*Fixture.Authority, Original);
	AddInfo(FString::Printf(
		TEXT("P16.1 cancel status=%d error=%d item=%d treatment=%d/%d conflict=%d/%d replay=%d diagnostic=%s"),
		static_cast<int32>(Cancelled.Status),
		static_cast<int32>(Cancelled.Error),
		static_cast<int32>(Cancelled.Item.Status),
		static_cast<int32>(Cancelled.Treatment.Status),
		static_cast<int32>(Cancelled.Treatment.Error),
		static_cast<int32>(Conflict.Status),
		static_cast<int32>(Conflict.Error),
		static_cast<int32>(CancelReplay.Status),
		*Cancelled.Diagnostic));
	TestTrue(TEXT("stale treatment cancels only while the same revision is live"),
		Cancelled.IsCancelled()
			&& Cancelled.Error
				== Edemo_mapShanmenMeridianShockTreatmentRouteError::
					TreatmentRejected
			&& Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Conditions->GetConditionRevision() == 1
			&& Fixture.CountTreatmentFinalizations(true) == 0
			&& Fixture.CountTreatmentFinalizations(false) == 1);
	TestTrue(TEXT("conflicting payload fails closed and exact cancellation replays"),
		Conflict.Status
				== Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Rejected
			&& Conflict.Error
				== Edemo_mapShanmenMeridianShockTreatmentRouteError::
					RequestIdConflict
			&& CancelReplay.IsCancelled()
			&& CancelReplay.bReusedRequest
			&& Fixture.CountTreatmentFinalizations(false) == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentHotbarOwnershipTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Session.HotbarOwnership",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentHotbarOwnershipTest::RunTest(
	const FString&)
{
	FTreatmentRouteFixture Fixture;
	if (!Fixture.Build(*this, TEXT("HotbarOwnership"), false))
	{
		AddError(TEXT("Could not build the P16.2 hotbar fixture."));
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle Lifecycle;
	Fdemo_mapShanmenMeridianShockTreatmentInputAdapter Input;
	FString Diagnostic;
	if (!Lifecycle.TryBegin(
			*Fixture.Authority, Fixture.Conditions, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}

	const Fdemo_mapShanmenMeridianShockTreatmentInputResult Generic =
		Input.RouteHotbarInput(
			Fixture.Authority, Lifecycle, Fixture.Timeline, 2);
	TestTrue(TEXT("ordinary consumable passes through without timeline capture"),
		Generic.ShouldPassThrough()
			&& !Generic.bTimelineSampled
			&& Input.GetNextRequestOrdinal() == 1
			&& Lifecycle.NumCapturedRequests() == 0);

	const Fdemo_mapShanmenMeridianShockTreatmentInputResult Applied =
		Input.RouteHotbarInput(
			Fixture.Authority, Lifecycle, Fixture.Timeline, 1);
	FShanmenItemAuthoritySnapshot BeforeRejectedReplay;
	Fixture.Authority->TryCaptureSnapshot(BeforeRejectedReplay);
	const Fdemo_mapShanmenMeridianShockTreatmentInputResult Inactive =
		Input.RouteHotbarInput(
			Fixture.Authority, Lifecycle, Fixture.Timeline, 1);
	FShanmenItemAuthoritySnapshot AfterRejectedReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterRejectedReplay);
	AddInfo(FString::Printf(
		TEXT("P16.2 hotbar generic=%d applied=%d/%d inactive=%d/%d ordinal=%llu diagnostic=%s"),
		static_cast<int32>(Generic.Status),
		static_cast<int32>(Applied.Status),
		static_cast<int32>(Applied.Route.Status),
		static_cast<int32>(Inactive.Status),
		static_cast<int32>(Inactive.Route.Error),
		Input.GetNextRequestOrdinal(),
		*Inactive.Diagnostic));
	TestTrue(TEXT("canonical treatment is consumed exactly once through the sole route"),
		Applied.IsAccepted()
			&& Applied.bTimelineSampled
			&& Applied.RequestOrdinal == 1
			&& Applied.Route.Item.FinalizeCommand.Receipt.ResourceBefore == 3
			&& Applied.Route.Item.FinalizeCommand.Receipt.ResourceAfter == 2
			&& !Fixture.Conditions->IsMeridianShockActive()
			&& Lifecycle.NumCapturedRequests() == 1);
	TestTrue(TEXT("inactive treatment remains handled and cannot fall through to healing"),
		Inactive.IsHandled()
			&& !Inactive.IsAccepted()
			&& Inactive.Status
				== Edemo_mapShanmenMeridianShockTreatmentInputStatus::
					ProductRejected
			&& BeforeRejectedReplay == AfterRejectedReplay
			&& Input.GetNextRequestOrdinal() == 3
			&& Lifecycle.NumCapturedRequests() == 1);
	TestTrue(TEXT("resolved treatment lifecycle ends cleanly"),
		Lifecycle.TryEnd(Diagnostic) && Lifecycle.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentLifecycleRecoveryTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Lifecycle.TeardownRecovery",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentLifecycleRecoveryTest::RunTest(
	const FString&)
{
	FTreatmentRouteFixture Fixture;
	if (!Fixture.Build(*this, TEXT("TeardownRecovery"), false))
	{
		AddError(TEXT("Could not build the P16.2 recovery fixture."));
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle Lifecycle;
	Fdemo_mapShanmenMeridianShockTreatmentInputAdapter Input;
	FString Diagnostic;
	if (!Lifecycle.TryBegin(
			*Fixture.Authority, Fixture.Conditions, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	Lifecycle.SetInterruptAfterTreatmentForAutomation(true);
	const Fdemo_mapShanmenMeridianShockTreatmentInputResult Interrupted =
		Input.RouteHotbarInput(
			Fixture.Authority, Lifecycle, Fixture.Timeline, 1);
	TestTrue(TEXT("post-treatment interruption remains owned by the active lifecycle"),
		Interrupted.Status
			== Edemo_mapShanmenMeridianShockTreatmentInputStatus::
				RecoveryRequired
			&& Interrupted.Route.RequiresRecovery()
			&& !Fixture.Conditions->IsMeridianShockActive()
			&& Lifecycle.HasUnresolvedRecovery()
			&& Lifecycle.NumPendingRecovery() == 1);

	Lifecycle.SetInterruptAfterTreatmentForAutomation(false);
	const bool bEnded = Lifecycle.TryEnd(Diagnostic);
	AddInfo(FString::Printf(
		TEXT("P16.2 teardown interrupted=%d/%d ended=%d pending=%d diagnostic=%s"),
		static_cast<int32>(Interrupted.Status),
		static_cast<int32>(Interrupted.Route.Error),
		bEnded ? 1 : 0,
		Lifecycle.NumPendingRecovery(),
		*Diagnostic));
	TestTrue(TEXT("Run teardown performs exact commit-only recovery before forgetting state"),
		bEnded
			&& Lifecycle.IsEmpty()
			&& Fixture.CountTreatmentFinalizations(true) == 1
			&& Fixture.CountTreatmentFinalizations(false) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentLedgerActiveRecoveryTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Recovery.ActiveCondition",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentLedgerActiveRecoveryTest::RunTest(
	const FString&)
{
	FTreatmentRouteFixture Fixture;
	if (!Fixture.Build(*this, TEXT("LedgerActiveRecovery"), false))
	{
		AddError(TEXT("Could not build the P16.3 active recovery fixture."));
		return false;
	}
	Fdemo_mapShanmenCombatConditionStatusSnapshot Status;
	FString Diagnostic;
	if (!Fixture.Conditions->TryCaptureMeridianShockStatus(Status))
	{
		AddError(TEXT("Could not capture active condition before durable prepare."));
		return false;
	}
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Prepared =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::PrepareActiveRun(
			*Fixture.Authority,
			Fixture.Correlation,
			Status,
			Fixture.TreatmentItemId);
	TestTrue(TEXT("durable prepare exists without a transient product session"),
		Prepared.IsPrepared()
			&& Fixture.CountTreatmentFinalizations(true) == 0
			&& Fixture.Conditions->IsMeridianShockActive());

	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle Reconstructed;
	if (!Reconstructed.TryBegin(
			*Fixture.Authority, Fixture.Conditions, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const bool bRecovered = Reconstructed.TryRecoverPending(Diagnostic);
	AddInfo(FString::Printf(
		TEXT("P16.3 active-ledger recovered=%d captured=%d commit=%d cancel=%d diagnostic=%s"),
		bRecovered ? 1 : 0,
		Reconstructed.NumCapturedRequests(),
		Fixture.CountTreatmentFinalizations(true),
		Fixture.CountTreatmentFinalizations(false),
		*Diagnostic));
	TestTrue(TEXT("active condition reconstructs treat then commit from durable ledger"),
		bRecovered
			&& !Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Conditions->NumProcessedTreatments() == 1
			&& Fixture.CountTreatmentFinalizations(true) == 1
			&& Fixture.CountTreatmentFinalizations(false) == 0
			&& Reconstructed.NumCapturedRequests() == 0
			&& !Reconstructed.HasUnresolvedRecovery());
	TestTrue(TEXT("reconstructed lifecycle ends after ledger resolution"),
		Reconstructed.TryEnd(Diagnostic) && Reconstructed.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentLedgerCommitOnlyRecoveryTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Recovery.CommitOnlyProof",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentLedgerCommitOnlyRecoveryTest::RunTest(
	const FString&)
{
	FTreatmentRouteFixture Fixture;
	if (!Fixture.Build(*this, TEXT("LedgerCommitOnlyRecovery"), false))
	{
		AddError(TEXT("Could not build the P16.3 commit-only recovery fixture."));
		return false;
	}
	{
		Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle LostSession;
		Fdemo_mapShanmenMeridianShockTreatmentInputAdapter Input;
		FString Diagnostic;
		if (!LostSession.TryBegin(
				*Fixture.Authority, Fixture.Conditions, Diagnostic))
		{
			AddError(Diagnostic);
			return false;
		}
		LostSession.SetInterruptAfterTreatmentForAutomation(true);
		const Fdemo_mapShanmenMeridianShockTreatmentInputResult Interrupted =
			Input.RouteHotbarInput(
				Fixture.Authority, LostSession, Fixture.Timeline, 1);
		TestTrue(TEXT("transient session is lost after condition commit"),
			Interrupted.Route.RequiresRecovery()
				&& !Fixture.Conditions->IsMeridianShockActive()
				&& Fixture.Conditions->NumProcessedTreatments() == 1
				&& Fixture.CountTreatmentFinalizations(true) == 0);
	}

	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle Reconstructed;
	FString Diagnostic;
	if (!Reconstructed.TryBegin(
			*Fixture.Authority, Fixture.Conditions, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const bool bRecovered = Reconstructed.TryRecoverPending(Diagnostic);
	AddInfo(FString::Printf(
		TEXT("P16.3 proof-ledger recovered=%d captured=%d commit=%d cancel=%d diagnostic=%s"),
		bRecovered ? 1 : 0,
		Reconstructed.NumCapturedRequests(),
		Fixture.CountTreatmentFinalizations(true),
		Fixture.CountTreatmentFinalizations(false),
		*Diagnostic));
	TestTrue(TEXT("runtime treatment proof selects commit-only ledger recovery"),
		bRecovered
			&& Fixture.Conditions->NumProcessedTreatments() == 1
			&& Fixture.CountTreatmentFinalizations(true) == 1
			&& Fixture.CountTreatmentFinalizations(false) == 0
			&& Reconstructed.NumCapturedRequests() == 0
			&& !Reconstructed.HasUnresolvedRecovery());
	TestTrue(TEXT("commit-only reconstructed lifecycle ends cleanly"),
		Reconstructed.TryEnd(Diagnostic) && Reconstructed.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentLedgerProofFenceTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Recovery.MissingProofFence",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentLedgerProofFenceTest::RunTest(
	const FString&)
{
	FTreatmentRouteFixture Fixture;
	if (!Fixture.Build(*this, TEXT("LedgerProofFence"), false))
	{
		AddError(TEXT("Could not build the P16.3 proof fence fixture."));
		return false;
	}
	Fdemo_mapShanmenCombatConditionStatusSnapshot Status;
	FString Diagnostic;
	if (!Fixture.Conditions->TryCaptureMeridianShockStatus(Status)
		|| !Fdemo_mapShanmenMeridianShockTreatmentAdapter::PrepareActiveRun(
			*Fixture.Authority,
			Fixture.Correlation,
			Status,
			Fixture.TreatmentItemId).IsPrepared()
		|| !Fixture.Conditions->TryEnd(
			Fixture.Correlation.ActiveRunId, Diagnostic)
		|| !Fixture.Conditions->TryBegin(
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Fixture.Timeline.GetTimelineId(),
			Fixture.Attributes,
			Diagnostic))
	{
		AddError(FString::Printf(
			TEXT("Could not arrange proof-loss recovery state: %s"),
			*Diagnostic));
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle Reconstructed;
	if (!Reconstructed.TryBegin(
			*Fixture.Authority, Fixture.Conditions, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const bool bRecovered = Reconstructed.TryRecoverPending(Diagnostic);
	Fixture.Authority->TryCaptureSnapshot(After);
	AddInfo(FString::Printf(
		TEXT("P16.3 missing-proof recovered=%d commit=%d cancel=%d unchanged=%d diagnostic=%s"),
		bRecovered ? 1 : 0,
		Fixture.CountTreatmentFinalizations(true),
		Fixture.CountTreatmentFinalizations(false),
		Before == After ? 1 : 0,
		*Diagnostic));
	TestTrue(TEXT("missing condition proof fails closed without item mutation"),
		!bRecovered
			&& !Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Conditions->NumProcessedTreatments() == 0
			&& Before == After
			&& Fixture.CountTreatmentFinalizations(true) == 0
			&& Fixture.CountTreatmentFinalizations(false) == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentInputIdentityTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Input.DeterministicIdentity",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentInputIdentityTest::RunTest(
	const FString&)
{
	const FGuid CorrelationId(0xC1620030, 0, 0, 1);
	const FGuid RunId(0xC1620030, 0, 0, 2);
	const FGuid ItemId(0xC1620030, 0, 0, 3);
	const FGuid SampleId(0xC1620030, 0, 0, 4);
	const FGuid First =
		Fdemo_mapShanmenMeridianShockTreatmentInputAdapter::MakeRequestId(
			CorrelationId, RunId, ItemId, 1, 17, SampleId, 9);
	const FGuid Replay =
		Fdemo_mapShanmenMeridianShockTreatmentInputAdapter::MakeRequestId(
			CorrelationId, RunId, ItemId, 1, 17, SampleId, 9);
	const FGuid Next =
		Fdemo_mapShanmenMeridianShockTreatmentInputAdapter::MakeRequestId(
			CorrelationId, RunId, ItemId, 1, 17, SampleId, 10);
	TestTrue(TEXT("equal canonical input identity replays exactly"),
		First.IsValid() && First == Replay && First != Next);
	TestFalse(TEXT("invalid hotbar identity fails closed"),
		Fdemo_mapShanmenMeridianShockTreatmentInputAdapter::MakeRequestId(
			CorrelationId, RunId, ItemId, 0, 17, SampleId, 9).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentSessionBindingFenceTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.Session.BindingFence",
	RouteFlags)

bool Fdemo_mapMeridianShockTreatmentSessionBindingFenceTest::RunTest(
	const FString&)
{
	FTreatmentRouteFixture Fixture;
	if (!Fixture.Build(*this, TEXT("SessionBindingFence"), false))
	{
		AddError(TEXT("Could not build the P16.2 binding fixture."));
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentProductSession Session;
	FString Diagnostic;
	if (!Session.TryBegin(
			Fixture.Correlation, Fixture.Conditions, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	TestTrue(TEXT("exact active binding is idempotent"),
		Session.TryBegin(
			Fixture.Correlation, Fixture.Conditions, Diagnostic));
	Fdemo_mapShanmenRunCorrelation Foreign = Fixture.Correlation;
	Foreign.CorrelationId = FGuid(0xC1620040, 0, 0, 1);
	TestFalse(TEXT("same RunId cannot hide a foreign full correlation"),
		Session.TryBegin(Foreign, Fixture.Conditions, Diagnostic));
	TestTrue(TEXT("binding fence preserves the original valid session"),
		Session.IsValid()
			&& Session.GetRunId() == Fixture.Correlation.ActiveRunId
			&& Session.TryEnd(
				*Fixture.Authority,
				Fixture.Correlation.ActiveRunId,
				Diagnostic));
	return true;
}

#endif
