#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseProductController.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSpiritShieldSession.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid ControllerRunId(0xD5600001, 0, 0, 1);
	const FGuid OtherControllerRunId(0xD5600002, 0, 0, 1);
	const FGuid FirstIntentId(0xD5600010, 0, 0, 1);
	const FGuid SecondIntentId(0xD5600011, 0, 0, 1);
	const FGuid SharedShieldTransactionId(0xD5600020, 0, 0, 1);
	const FGuid SharedShieldCommandId(0xD5600021, 0, 0, 1);
	const FGuid ConflictingShieldCommandId(0xD5600022, 0, 0, 1);
	const FGuid SharedShieldTimelineId(0xD5600023, 0, 0, 1);
	const FGuid PartialShieldTransactionId(0xD5600024, 0, 0, 1);
	const FGuid PartialShieldCommandId(0xD5600025, 0, 0, 1);

	struct FDivineSenseControllerFixture
	{
		UWorld* World = nullptr;
		APawn* Player = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;

		bool Start(const FGuid& RunId)
		{
			if (!GEngine || !RunId.IsValid())
			{
				return false;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				return false;
			}
			World->WorldType = EWorldType::GamePreview;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(EWorldType::GamePreview);
			Context.SetCurrentWorld(World);
			World->InitializeNewWorld(
				UWorld::InitializationValues()
					.InitializeScenes(false)
					.AllowAudioPlayback(false)
					.RequiresHitProxies(false)
					.CreatePhysicsScene(false)
					.CreateNavigation(false)
					.CreateAISystem(false)
					.ShouldSimulatePhysics(false)
					.EnableTraceCollision(false)
					.SetTransactional(false)
					.CreateFXSystem(false));

			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Player = World->SpawnActor<APawn>(
				APawn::StaticClass(), FTransform::Identity, Parameters);
			USceneComponent* Root = Player
				? NewObject<USceneComponent>(
					Player,
					TEXT("P196DivineSensePlayerRoot"),
					RF_Transient)
				: nullptr;
			Health = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player,
					TEXT("P196DivineSensePlayerHealth"),
					RF_Transient)
				: nullptr;
			if (!Player || !Root || !Health)
			{
				return false;
			}
			Player->SetRootComponent(Root);
			Root->SetWorldLocation(FVector::ZeroVector);
			return Coordinator.TryBeginRun(
				RunId, Player, Health, Diagnostic);
		}

		void Stop()
		{
			Coordinator.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
				Player = nullptr;
				Health = nullptr;
				CollectGarbage(RF_NoFlags);
			}
		}

		~FDivineSenseControllerFixture()
		{
			Stop();
		}
	};

	bool MakeOpeningSnapshot(
		const FGuid& OwnerEntityId,
		float CurrentAmount,
		float MaximumAmount,
		int64 AuthorityRevision,
		FShanmenActionResourceSnapshot& OutSnapshot)
	{
		FShanmenActionResourceAuthority Authority;
		return FShanmenActionResourceAuthority::TryCreate(
			OwnerEntityId,
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
			CurrentAmount,
			MaximumAmount,
			AuthorityRevision,
			Authority)
			&& Authority.TryCaptureSnapshot(OutSnapshot);
	}

	FShanmenCombatActionSnapshot MakeAction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		int32 ActivationSequence)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = Coordinator.GetRunId();
		Capture.OwnerId = Coordinator.GetPlayerEntityId();
		Capture.SourceEntityId = Coordinator.GetPlayerEntityId();
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P19.6");
		Capture.Content.Digest =
			TEXT("TEST-DIGEST-P19.6-PRODUCT-CONTROLLER");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenDivineSenseDefinition MakeDefinition(
		FName ScanRuleId = TEXT("Spell.DivineSense.ProductController01"))
	{
		FShanmenDivineSenseDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.ScanRuleId = ScanRuleId;
		Capture.Radius = 100.0;
		Capture.MaximumResults = 4;
		Capture.OcclusionPolicy =
			EShanmenDivineSenseOcclusionPolicy::VisibleOnly;
		Capture.RequiredSubjectTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.BlockedSubjectTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.bRejectSelf = true;
		FShanmenDivineSenseDefinition Definition;
		check(FShanmenDivineSenseDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenActionResourceCost MakeCost(
		float Amount,
		FGameplayTag Channel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
		FName RuleId = TEXT("Spell.DivineSense.ControllerSpiritEnergy01"))
	{
		FShanmenActionResourceCostCapture Capture;
		Capture.RuleId = RuleId;
		Capture.ResourceChannel = Channel;
		Capture.Amount = Amount;
		FShanmenActionResourceCost Cost;
		check(FShanmenActionResourceCost::TryCapture(Capture, Cost));
		return Cost;
	}

	Fdemo_mapShanmenDivineSenseProductConfig MakeConfig(
		int32 Capacity,
		float CostAmount = 10.0f,
		FName ScanRuleId = TEXT("Spell.DivineSense.ProductController01"))
	{
		Fdemo_mapShanmenDivineSenseProductConfig Config;
		check(Fdemo_mapShanmenDivineSenseProductConfig::TryCapture(
			MakeDefinition(ScanRuleId),
			MakeCost(CostAmount),
			Capacity,
			Config));
		return Config;
	}

	Fdemo_mapShanmenDivineSenseProductIntent MakeIntent(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& IntentId,
		int32 ActivationSequence,
		int32 ScanOrdinal = 0,
		int32 SubjectActorBudget = 1)
	{
		Fdemo_mapShanmenDivineSenseProductIntent Intent;
		check(Fdemo_mapShanmenDivineSenseProductIntent::TryCapture(
			IntentId,
			MakeAction(Coordinator, ActivationSequence),
			ScanOrdinal,
			SubjectActorBudget,
			Intent));
		return Intent;
	}

	FShanmenCombatActionSnapshot MakeShieldAction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		int32 ActivationSequence)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = Coordinator.GetRunId();
		Capture.OwnerId = Coordinator.GetPlayerEntityId();
		Capture.SourceEntityId = Coordinator.GetPlayerEntityId();
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P25.0");
		Capture.Content.Digest =
			TEXT("TEST-DIGEST-P25.0-SHARED-SPIRIT-ENERGY");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenSpiritShieldDefinition MakeShieldDefinition()
	{
		FShanmenSpiritShieldDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = TEXT("Defense.Spell.SpiritShield.P25_0");
		Capture.MaximumCapacity = 30.0f;
		Capture.RequiredDamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenSpiritShieldDefinition Definition;
		check(FShanmenSpiritShieldDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenSpiritShieldSchedule MakeShieldSchedule()
	{
		FShanmenSpiritShieldSchedule Schedule;
		check(FShanmenSpiritShieldSchedule::TryCapture(
			SharedShieldTimelineId, 100, 160, Schedule));
		return Schedule;
	}

	bool BeginController(
		FDivineSenseControllerFixture& Fixture,
		Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapShanmenDivineSenseProductConfig& Config,
		float CurrentAmount = 100.0f,
		float MaximumAmount = 100.0f,
		int64 AuthorityRevision = 0)
	{
		FShanmenActionResourceSnapshot Opening;
		return MakeOpeningSnapshot(
			Fixture.Coordinator.GetPlayerEntityId(),
			CurrentAmount,
			MaximumAmount,
			AuthorityRevision,
			Opening)
			&& Controller.TryBegin(
				Fixture.Coordinator,
				Opening,
				Config,
				Fixture.Diagnostic);
	}

	class FControllerEvidenceProvider final
		: public Idemo_mapShanmenDivineSenseWorldEvidenceProvider
	{
	public:
		bool bAvailable = true;
		mutable int32 CallCount = 0;

		virtual bool TryCaptureSubjectEvidence(
			UWorld*,
			const FShanmenDivineSenseScanRequest&,
			const AActor*,
			const AActor*,
			const FGuid&,
			const FVector&,
			const FVector&,
			Fdemo_mapShanmenDivineSenseWorldSubjectEvidence& OutEvidence)
			const override
		{
			OutEvidence =
				Fdemo_mapShanmenDivineSenseWorldSubjectEvidence();
			++CallCount;
			if (!bAvailable)
			{
				return false;
			}
			OutEvidence.SubjectTags.AddTag(
				FShanmenCombatNativeTags::TargetLiving());
			OutEvidence.bHasLineOfSight = true;
			OutEvidence.AuthorityRevision = 6;
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductControllerConfigLifecycleTest,
	"Shanmen.0_0_10.Product.DivineSenseProductController.ConfigLifecycleAndAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductControllerConfigLifecycleTest::RunTest(
	const FString&)
{
	FDivineSenseControllerFixture Fixture;
	if (!TestTrue(TEXT("Combat Run starts"),
		Fixture.Start(ControllerRunId)))
	{
		return false;
	}
	const Fdemo_mapShanmenDivineSenseProductConfig Config = MakeConfig(2);
	const Fdemo_mapShanmenDivineSenseProductConfig ReplayConfig =
		MakeConfig(2);
	TestTrue(TEXT("Equivalent product values freeze one config identity"),
		Config.IsValid() && Config.Matches(ReplayConfig)
			&& Config.GetConfigId() == ReplayConfig.GetConfigId());

	Fdemo_mapShanmenDivineSenseProductConfig WrongChannel;
	TestFalse(TEXT("Only SpiritEnergy can configure Divine Sense"),
		Fdemo_mapShanmenDivineSenseProductConfig::TryCapture(
			MakeDefinition(),
			MakeCost(
				10.0f,
				FShanmenCombatRuntimeNativeTags::Resource(),
				TEXT("Spell.DivineSense.InvalidRootResource")),
			2,
			WrongChannel));

	Fdemo_mapShanmenDivineSenseProductController Controller;
	TestTrue(TEXT("Default Controller is valid and empty"),
		Controller.IsValid() && Controller.IsEmpty());
	FShanmenActionResourceSnapshot Opening;
	check(MakeOpeningSnapshot(
		Fixture.Coordinator.GetPlayerEntityId(),
		100.0f,
		120.0f,
		7,
		Opening));
	TestTrue(TEXT("Controller freezes config and opens Session"),
		Controller.TryBegin(
			Fixture.Coordinator, Opening, Config, Fixture.Diagnostic));
	const FGuid ControllerId = Controller.GetControllerId();
	TestTrue(TEXT("Bound Controller identity is complete"),
		Controller.IsValid() && Controller.IsActive()
			&& ControllerId.IsValid()
			&& Controller.GetConfig().Matches(Config)
			&& Controller.GetSession().GetOpeningResourceSnapshot()
				.GetSnapshotId() == Opening.GetSnapshotId());
	TestTrue(TEXT("Exact begin is idempotent"),
		Controller.TryBegin(
			Fixture.Coordinator,
			Opening,
			ReplayConfig,
			Fixture.Diagnostic)
			&& Controller.GetControllerId() == ControllerId);
	TestFalse(TEXT("Active Controller cannot change frozen config"),
		Controller.TryBegin(
			Fixture.Coordinator,
			Opening,
			MakeConfig(3),
			Fixture.Diagnostic));
	TestFalse(TEXT("Active Controller cannot reset"), Controller.Reset());

	Fdemo_mapShanmenDivineSenseProductAvailability Availability;
	TestTrue(TEXT("Pointer-free product availability captures"),
		Controller.TryCaptureAvailability(
			Fixture.Coordinator, Availability, Fixture.Diagnostic));
	TestTrue(TEXT("Opening availability exposes both bounded authorities"),
		Availability.IsValid()
			&& Availability.GetControllerId() == ControllerId
			&& Availability.GetCapturedIntentCount() == 0
			&& Availability.GetRemainingIntentCapacity() == 2
			&& Availability.GetSessionAvailability()
				.GetRemainingCommandCapacity() == 2
			&& Availability.CanCaptureNewIntent()
			&& Controller.IsAvailabilityCurrent(
				Fixture.Coordinator, Availability));

	Fdemo_mapShanmenDivineSenseProductIntent InvalidIntent;
	TestFalse(TEXT("Intent requires a caller-stable identity"),
		Fdemo_mapShanmenDivineSenseProductIntent::TryCapture(
			FGuid(), MakeAction(Fixture.Coordinator, 1), 0, 1, InvalidIntent));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductControllerReplayTest,
	"Shanmen.0_0_10.Product.DivineSenseProductController.FrozenReplayWithoutLiveInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductControllerReplayTest::RunTest(
	const FString&)
{
	FDivineSenseControllerFixture Fixture;
	if (!Fixture.Start(ControllerRunId))
	{
		return false;
	}
	const Fdemo_mapShanmenDivineSenseProductConfig Config = MakeConfig(2);
	Fdemo_mapShanmenDivineSenseProductController Controller;
	check(BeginController(Fixture, Controller, Config));
	const Fdemo_mapShanmenDivineSenseProductIntent Intent =
		MakeIntent(Fixture.Coordinator, FirstIntentId, 1);
	FControllerEvidenceProvider Provider;

	const auto Applied = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		Intent,
		{ Fixture.Player },
		Provider);
	if (!TestTrue(TEXT("First Controller submission applies"),
		Applied.IsAccepted() && !Applied.IsReplay()
			&& !Applied.bReusedIntent))
	{
		return false;
	}
	const int32 CallsAfterApply = Provider.CallCount;
	const FGuid CommandId = Applied.Command.GetRouteCommandId();
	const FGuid ReceiptId =
		Applied.Route.Route.HostPulse.Pulse.Receipt.GetReceiptId();
	TestTrue(TEXT("Command uses only frozen Controller policy"),
		Applied.Command.GetPulseCommand().GetDefinition().GetDefinitionId()
			== Config.GetDefinition().GetDefinitionId()
			&& Applied.Command.GetPulseCommand().GetCost().GetCostId()
				== Config.GetCost().GetCostId()
			&& Controller.FindCapturedCommand(FirstIntentId)
				&& Controller.FindCapturedCommand(FirstIntentId)
					->GetRouteCommandId() == CommandId);
	TestTrue(TEXT("First submission changes product availability once"),
		Applied.AvailabilityBefore.GetCapturedIntentCount() == 0
			&& Applied.AvailabilityAfter.GetCapturedIntentCount() == 1
			&& Applied.AvailabilityAfter.GetSessionAvailability()
				.GetResourceSnapshot().GetCurrentAmount() == 90.0f
			&& Controller.NumCapturedIntents() == 1);

	Provider.bAvailable = false;
	const auto Replay = Controller.TrySubmit(
		Fixture.Coordinator,
		nullptr,
		nullptr,
		Intent,
		{},
		Provider);
	TestTrue(TEXT("Exact Controller submission replays"),
		Replay.IsAccepted() && Replay.IsReplay()
			&& Replay.bReusedIntent);
	TestTrue(TEXT("Replay returns identical frozen command and pulse proof"),
		Replay.Command.GetRouteCommandId() == CommandId
			&& Replay.Route.Route.HostPulse.Pulse.Receipt.GetReceiptId()
				== ReceiptId);
	TestEqual(TEXT("Replay performs no provider read"),
		Provider.CallCount, CallsAfterApply);
	TestTrue(TEXT("Replay preserves Controller and Session projections"),
		Replay.AvailabilityBefore.Matches(Replay.AvailabilityAfter)
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 90.0f
			&& Controller.GetSession().GetRouter().NumProcessedCommands()
				== 1
			&& Controller.NumCapturedIntents() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductControllerConflictRecoveryTest,
	"Shanmen.0_0_10.Product.DivineSenseProductController.ConflictRollbackAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductControllerConflictRecoveryTest::RunTest(
	const FString&)
{
	FDivineSenseControllerFixture Fixture;
	if (!Fixture.Start(ControllerRunId))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	check(BeginController(Fixture, Controller, MakeConfig(2)));
	const Fdemo_mapShanmenDivineSenseProductIntent Intent =
		MakeIntent(Fixture.Coordinator, FirstIntentId, 1);
	FControllerEvidenceProvider Provider;
	Provider.bAvailable = false;

	const auto Rejected = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		Intent,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("Provider failure remains a structured route rejection"),
		Rejected.IsValid()
			&& Rejected.Status
				== Edemo_mapShanmenDivineSenseProductControllerStatus::
					RouteRejected
			&& !Rejected.Route.IsAccepted());
	const int32 CallsAfterReject = Provider.CallCount;
	const FGuid FrozenCommandId =
		Rejected.Command.GetRouteCommandId();
	TestTrue(TEXT("Rejected first route retains one bounded frozen Intent"),
		Controller.IsValid() && Controller.NumCapturedIntents() == 1
			&& Rejected.AvailabilityBefore.GetCapturedIntentCount() == 0
			&& Rejected.AvailabilityAfter.GetCapturedIntentCount() == 1
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 100.0f
			&& Controller.GetSession().GetRouter().NumProcessedCommands()
				== 0);

	const Fdemo_mapShanmenDivineSenseProductIntent Conflict =
		MakeIntent(Fixture.Coordinator, FirstIntentId, 2, 1);
	const auto ConflictResult = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		Conflict,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("IntentId payload conflict fails before live evidence"),
		ConflictResult.IsValid()
			&& ConflictResult.Status
				== Edemo_mapShanmenDivineSenseProductControllerStatus::
					IntentIdConflict
			&& Provider.CallCount == CallsAfterReject
			&& Controller.NumCapturedIntents() == 1
			&& Controller.FindCapturedCommand(FirstIntentId)
				->GetRouteCommandId() == FrozenCommandId);

	Provider.bAvailable = true;
	const auto Recovered = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		Intent,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("Same frozen command recovers after provider repair"),
		Recovered.IsAccepted() && !Recovered.IsReplay()
			&& Recovered.bReusedIntent
			&& Recovered.Command.GetRouteCommandId() == FrozenCommandId
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 90.0f
			&& Controller.GetSession().GetRouter().NumProcessedCommands()
				== 1);

	FDivineSenseControllerFixture Foreign;
	if (!Foreign.Start(OtherControllerRunId))
	{
		return false;
	}
	const auto ForeignRun = Controller.TrySubmit(
		Foreign.Coordinator,
		Foreign.World,
		Foreign.Player,
		MakeIntent(Foreign.Coordinator, SecondIntentId, 1),
		{ Foreign.Player },
		Provider);
	TestTrue(TEXT("Foreign Run is rejected before provider access"),
		ForeignRun.IsValid()
			&& ForeignRun.Status
				== Edemo_mapShanmenDivineSenseProductControllerStatus::
					RunMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductControllerCapacityTeardownTest,
	"Shanmen.0_0_10.Product.DivineSenseProductController.CapacityAndTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductControllerCapacityTeardownTest::RunTest(
	const FString&)
{
	FDivineSenseControllerFixture Fixture;
	if (!Fixture.Start(ControllerRunId))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	check(BeginController(
		Fixture, Controller, MakeConfig(1), 10.0f, 20.0f, 3));
	const FGuid ControllerId = Controller.GetControllerId();
	const FGuid ConfigId = Controller.GetConfig().GetConfigId();
	const Fdemo_mapShanmenDivineSenseProductIntent First =
		MakeIntent(Fixture.Coordinator, FirstIntentId, 1);
	FControllerEvidenceProvider Provider;
	const auto Applied = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		First,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("Single product slot applies exact remaining cost"),
		Applied.IsAccepted()
			&& Applied.AvailabilityAfter.GetRemainingIntentCapacity() == 0
			&& !Applied.AvailabilityAfter.CanCaptureNewIntent()
			&& Applied.AvailabilityAfter.GetSessionAvailability()
				.GetRemainingCommandCapacity() == 0);

	const auto Exhausted = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		MakeIntent(Fixture.Coordinator, SecondIntentId, 2, 1),
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("New Intent fails at Controller capacity without recapture"),
		Exhausted.IsValid()
			&& Exhausted.Status
				== Edemo_mapShanmenDivineSenseProductControllerStatus::
					IntentCapacityExceeded
			&& Controller.NumCapturedIntents() == 1);

	const auto WrongEnd = Controller.TryEnd(
		Fixture.Coordinator, FGuid(0xD5600099, 0, 0, 1));
	TestTrue(TEXT("Wrong ControllerId cannot tear down"),
		WrongEnd.IsValid()
			&& WrongEnd.Status
				== Edemo_mapShanmenDivineSenseProductControllerEndStatus::
					ControllerMismatch
			&& Controller.IsActive());

	const auto Ended = Controller.TryEnd(
		Fixture.Coordinator, ControllerId);
	TestTrue(TEXT("Exact Controller teardown succeeds"),
		Ended.IsSuccess() && !Ended.IsReplay()
			&& Controller.IsEnded() && Controller.IsValid());
	TestTrue(TEXT("Terminal proof closes config Intent and Session identity"),
		Ended.Receipt.GetControllerId() == ControllerId
			&& Ended.Receipt.GetConfigId() == ConfigId
			&& Ended.Receipt.GetCapturedIntentCount() == 1
			&& Ended.Receipt.GetSessionEndReceiptId()
				== Ended.SessionEnd.Receipt.GetReceiptId());

	Fdemo_mapShanmenDivineSenseProductAvailability AfterEnd;
	TestFalse(TEXT("Ended Controller exposes no live availability"),
		Controller.TryCaptureAvailability(
			Fixture.Coordinator, AfterEnd, Fixture.Diagnostic));
	const auto SubmitAfterEnd = Controller.TrySubmit(
		Fixture.Coordinator,
		nullptr,
		nullptr,
		First,
		{},
		Provider);
	TestTrue(TEXT("Ended Controller rejects further submission"),
		SubmitAfterEnd.IsValid()
			&& SubmitAfterEnd.Status
				== Edemo_mapShanmenDivineSenseProductControllerStatus::
					ControllerNotActive);

	const auto ReplayEnd = Controller.TryEnd(
		Fixture.Coordinator, ControllerId);
	TestTrue(TEXT("Exact Controller teardown replays immutable proof"),
		ReplayEnd.IsSuccess() && ReplayEnd.IsReplay()
			&& ReplayEnd.Receipt.GetReceiptId()
				== Ended.Receipt.GetReceiptId());
	TestTrue(TEXT("Ended Controller resets to reusable empty state"),
		Controller.Reset() && Controller.IsEmpty() && Controller.IsValid());
	TestTrue(TEXT("Combat Run ends after Controller teardown"),
		Fixture.Coordinator.TryEndRun(
			ControllerRunId, Fixture.Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductControllerSharedSpiritEnergyLedgerTest,
	"Shanmen.0_0_10.Product.DivineSenseProductController.SharedSpiritEnergyLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductControllerSharedSpiritEnergyLedgerTest::
	RunTest(const FString&)
{
	FDivineSenseControllerFixture Fixture;
	if (!Fixture.Start(ControllerRunId))
	{
		return false;
	}

	Fdemo_mapShanmenDivineSenseProductController Controller;
	check(BeginController(Fixture, Controller, MakeConfig(3)));
	FControllerEvidenceProvider Provider;
	Provider.bAvailable = false;
	const Fdemo_mapShanmenDivineSenseProductIntent StaleIntent =
		MakeIntent(Fixture.Coordinator, FirstIntentId, 1);
	const auto InitialProviderReject = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		StaleIntent,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("Failed live evidence retains one immutable pre-shield command"),
		InitialProviderReject.IsValid()
			&& !InitialProviderReject.IsAccepted()
			&& Controller.NumCapturedIntents() == 1);

	FShanmenSpiritShieldSession ShieldSession;
	int32 MutationCalls = 0;
	const FShanmenCombatActionSnapshot ShieldAction =
		MakeShieldAction(Fixture.Coordinator, 25);
	const FShanmenSpiritShieldDefinition ShieldDefinition =
		MakeShieldDefinition();
	const FShanmenActionResourceCost ShieldCost = MakeCost(
		20.0f,
		FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
		TEXT("Cost.Spell.SpiritShield.P25_0"));
	const FShanmenSpiritShieldSchedule ShieldSchedule =
		MakeShieldSchedule();
	const auto PartialMutation =
		Controller.ApplySharedSpiritEnergyTransaction(
			Fixture.Coordinator,
			PartialShieldTransactionId,
			PartialShieldCommandId,
			[&](FShanmenActionResourceAuthority& Authority)
			{
				++MutationCalls;
				FShanmenSpiritShieldSession PartialShield;
				return FShanmenSpiritShieldSession::Begin(
					ShieldAction,
					ShieldDefinition,
					ShieldCost,
					ShieldSchedule,
					Authority,
					PartialShield).IsSuccess();
			});
	TestTrue(TEXT("Unfinalized external reservation rolls back atomically"),
		PartialMutation.IsValid() && !PartialMutation.IsSuccess()
			&& PartialMutation.Error
				== Edemo_mapShanmenSharedSpiritEnergyTransactionError::
					MutationRejected
			&& MutationCalls == 1
			&& Controller.IsValid()
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 100.0f
			&& Controller.GetSession().GetHost()
				.NumExternalSpiritEnergyTransactions() == 0
			&& Controller.GetSession().GetRouter()
				.NumExternalSpiritEnergyTransactions() == 0);

	const auto Applied = Controller.ApplySharedSpiritEnergyTransaction(
		Fixture.Coordinator,
		SharedShieldTransactionId,
		SharedShieldCommandId,
		[&](FShanmenActionResourceAuthority& Authority)
		{
			++MutationCalls;
			FShanmenSpiritShieldSession CandidateShield;
			const FShanmenSpiritShieldActionResult Begun =
				FShanmenSpiritShieldSession::Begin(
					ShieldAction,
					ShieldDefinition,
					ShieldCost,
					ShieldSchedule,
					Authority,
					CandidateShield);
			if (!Begun.IsSuccess()
				|| !CandidateShield.Commit(Authority).IsSuccess())
			{
				return false;
			}
			ShieldSession = MoveTemp(CandidateShield);
			return true;
		});
	TestTrue(TEXT("Shield activation commits through the sole Run SpiritEnergy owner"),
		Applied.IsSuccess() && !Applied.IsReplay()
			&& MutationCalls == 2
			&& ShieldSession.IsValid()
			&& Controller.IsValid()
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 80.0f
			&& Controller.GetSession().GetHost()
				.NumExternalSpiritEnergyTransactions() == 1
			&& Controller.GetSession().GetRouter()
				.NumExternalSpiritEnergyTransactions() == 1
			&& Controller.GetSession().GetRouter().NumProcessedCommands()
				== 0);

	const auto Replay = Controller.ApplySharedSpiritEnergyTransaction(
		Fixture.Coordinator,
		SharedShieldTransactionId,
		SharedShieldCommandId,
		[&](FShanmenActionResourceAuthority&)
		{
			++MutationCalls;
			return false;
		});
	TestTrue(TEXT("Exact shared transaction replay bypasses resource mutation"),
		Replay.IsSuccess() && Replay.IsReplay()
			&& MutationCalls == 2
			&& Replay.Receipt.GetReceiptId()
				== Applied.Receipt.GetReceiptId()
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 80.0f);

	const auto Conflict = Controller.ApplySharedSpiritEnergyTransaction(
		Fixture.Coordinator,
		SharedShieldTransactionId,
		ConflictingShieldCommandId,
		[&](FShanmenActionResourceAuthority&)
		{
			++MutationCalls;
			return true;
		});
	TestTrue(TEXT("Transaction identity conflict fails before mutation"),
		Conflict.IsValid() && !Conflict.IsSuccess()
			&& Conflict.Error
				== Edemo_mapShanmenSharedSpiritEnergyTransactionError::
					TransactionConflict
			&& MutationCalls == 2
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 80.0f);

	Provider.bAvailable = true;
	const int32 CallsBeforeStaleRetry = Provider.CallCount;
	const auto StaleRetry = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		StaleIntent,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("A pre-transaction optimistic command fails closed as stale"),
		StaleRetry.IsValid() && !StaleRetry.IsAccepted()
			&& StaleRetry.Route.Route.Status
				== Edemo_mapShanmenDivineSenseCommandRouteStatus::
					ResourceProjectionStale
			&& Provider.CallCount == CallsBeforeStaleRetry
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 80.0f);

	const auto FreshPulse = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		MakeIntent(Fixture.Coordinator, SecondIntentId, 2),
		{ Fixture.Player },
		Provider);
	Fdemo_mapShanmenDivineSenseProductAvailability Availability;
	TestTrue(TEXT("A fresh Divine Sense pulse continues from the shared balance"),
		FreshPulse.IsAccepted() && !FreshPulse.IsReplay()
			&& Controller.IsValid()
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 70.0f
			&& Controller.GetSession().GetRouter().NumProcessedCommands()
				== 1
			&& Controller.GetSession().GetRouter()
				.NumExternalSpiritEnergyTransactions() == 1
			&& Controller.TryCaptureAvailability(
				Fixture.Coordinator, Availability, Fixture.Diagnostic)
			&& Availability.GetSessionAvailability()
				.GetResourceSnapshot().GetCurrentAmount() == 70.0f);
	return true;
}

#endif
