#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseProductHost.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid DivineHostRunId(0xD5300001, 0, 0, 1);
	const FGuid DivineHostOtherRunId(0xD5300002, 0, 0, 1);
	const FGuid DivineHostOwnerId(0xD5300003, 0, 0, 1);
	const FGuid DivineHostSourceId(0xD5300004, 0, 0, 1);
	const FGuid DivineHostOtherSourceId(0xD5300005, 0, 0, 1);
	const FGuid DivineHostSubjectId(0xD5300010, 0, 0, 1);

	FGameplayTagContainer HostLivingTags()
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		return Tags;
	}

	FShanmenCombatActionSnapshot MakeHostAction(
		int32 ActivationSequence = 1,
		const FGuid& RunId = DivineHostRunId,
		const FGuid& SourceEntityId = DivineHostSourceId,
		FName ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId())
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = DivineHostOwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content.Version = TEXT("0.0.10.P19.3");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P19.3-HOST");
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

	FShanmenDivineSenseDefinition MakeHostDefinition()
	{
		FShanmenDivineSenseDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.ScanRuleId = TEXT("Spell.DivineSense.Pulse.ProductHost01");
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

	FShanmenActionResourceCost MakeHostCost(
		float Amount = 10.0f,
		FGameplayTag Channel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy())
	{
		FShanmenActionResourceCostCapture Capture;
		Capture.RuleId = TEXT("Spell.DivineSense.Pulse.HostSpiritEnergy01");
		Capture.ResourceChannel = Channel;
		Capture.Amount = Amount;
		FShanmenActionResourceCost Cost;
		check(FShanmenActionResourceCost::TryCapture(Capture, Cost));
		return Cost;
	}

	Fdemo_mapShanmenDivineSensePulseCommand MakeHostCommand(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		int32 ScanOrdinal = 0,
		int32 SubjectActorBudget = 4)
	{
		Fdemo_mapShanmenDivineSensePulseCommand Command;
		check(Fdemo_mapShanmenDivineSensePulseCommand::TryCapture(
			Action,
			Definition,
			Cost,
			ScanOrdinal,
			SubjectActorBudget,
			Command));
		return Command;
	}

	struct FHostEvidenceEntry
	{
		FGameplayTagContainer Tags;
		bool bHasLineOfSight = true;
		int64 AuthorityRevision = 0;
	};

	class FFixedHostEvidenceProvider final
		: public Idemo_mapShanmenDivineSenseWorldEvidenceProvider
	{
	public:
		TMap<FGuid, FHostEvidenceEntry> Entries;
		FGuid UnavailableSubjectId;
		mutable TArray<FGuid> Calls;

		void Add(
			const FGuid& SubjectEntityId,
			const FGameplayTagContainer& Tags,
			bool bHasLineOfSight,
			int64 AuthorityRevision)
		{
			Entries.Add(
				SubjectEntityId,
				{ Tags, bHasLineOfSight, AuthorityRevision });
		}

		virtual bool TryCaptureSubjectEvidence(
			UWorld*,
			const FShanmenDivineSenseScanRequest&,
			const AActor*,
			const AActor*,
			const FGuid& SubjectEntityId,
			const FVector&,
			const FVector&,
			Fdemo_mapShanmenDivineSenseWorldSubjectEvidence&
				OutEvidence) const override
		{
			OutEvidence =
				Fdemo_mapShanmenDivineSenseWorldSubjectEvidence();
			Calls.Add(SubjectEntityId);
			if (SubjectEntityId == UnavailableSubjectId)
			{
				return false;
			}
			const FHostEvidenceEntry* Entry = Entries.Find(SubjectEntityId);
			if (!Entry)
			{
				return false;
			}
			OutEvidence.SubjectTags = Entry->Tags;
			OutEvidence.bHasLineOfSight = Entry->bHasLineOfSight;
			OutEvidence.AuthorityRevision = Entry->AuthorityRevision;
			return true;
		}
	};

	struct FDivineHostWorldFixture
	{
		UWorld* World = nullptr;

		bool Start()
		{
			if (!GEngine)
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
			return true;
		}

		AActor* Spawn(const FVector& Location) const
		{
			if (!World)
			{
				return nullptr;
			}
			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Actor = World->SpawnActor<AActor>(
				AActor::StaticClass(), FTransform::Identity, Parameters);
			if (!Actor)
			{
				return nullptr;
			}
			USceneComponent* Root = NewObject<USceneComponent>(
				Actor, NAME_None, RF_Transient);
			if (!Root)
			{
				World->DestroyActor(Actor, true, true);
				return nullptr;
			}
			Actor->SetRootComponent(Root);
			Root->SetWorldLocation(Location);
			return Actor;
		}

		void Stop()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
				CollectGarbage(RF_NoFlags);
			}
		}

		~FDivineHostWorldFixture()
		{
			Stop();
		}
	};

	bool BindHostActor(
		FShanmenWorldEntityRegistry& Registry,
		AActor* Actor,
		const FGuid& EntityId)
	{
		return Registry.BindObject(DivineHostRunId, Actor, EntityId)
			== EShanmenWorldBindingResult::Bound;
	}

	bool StartHostWorld(
		FDivineHostWorldFixture& Fixture,
		FShanmenWorldEntityRegistry& Registry,
		AActor*& OutSource,
		AActor*& OutSubject)
	{
		if (!Fixture.Start())
		{
			return false;
		}
		OutSource = Fixture.Spawn(FVector::ZeroVector);
		OutSubject = Fixture.Spawn(FVector(25.0, 0.0, 0.0));
		return OutSource && OutSubject
			&& Registry.TryBeginRun(DivineHostRunId)
			&& BindHostActor(Registry, OutSource, DivineHostSourceId)
			&& BindHostActor(Registry, OutSubject, DivineHostSubjectId);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductHostOpenAndApplyTest,
	"Shanmen.0_0_10.Product.DivineSenseProductHost.OpenAndApply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductHostOpenAndApplyTest::RunTest(const FString&)
{
	FDivineHostWorldFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	AActor* Source = nullptr;
	AActor* Subject = nullptr;
	if (!TestTrue(TEXT("World and registry start"),
		StartHostWorld(Fixture, Registry, Source, Subject)))
	{
		return false;
	}
	FFixedHostEvidenceProvider Provider;
	Provider.Add(DivineHostSubjectId, HostLivingTags(), true, 7);

	Fdemo_mapShanmenDivineSenseProductHost Host;
	Fdemo_mapShanmenDivineSenseProductHost SameHost;
	TestTrue(TEXT("Host opens"),
		Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			DivineHostRunId,
			DivineHostSourceId,
			100.0f,
			100.0f,
			0,
			2,
			Host));
	TestTrue(TEXT("Equivalent host opens"),
		Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			DivineHostRunId,
			DivineHostSourceId,
			100.0f,
			100.0f,
			0,
			2,
			SameHost));
	TestEqual(TEXT("Host identity is deterministic"),
		Host.GetHostId(), SameHost.GetHostId());

	const FShanmenCombatActionSnapshot Action = MakeHostAction();
	const FShanmenDivineSenseDefinition Definition = MakeHostDefinition();
	const FShanmenActionResourceCost Cost = MakeHostCost();
	const Fdemo_mapShanmenDivineSensePulseCommand Command =
		MakeHostCommand(Action, Definition, Cost);
	const auto Result = Host.ExecutePulse(
		Fixture.World,
		Registry,
		Source,
		Command,
		{ Subject },
		Provider);

	TestTrue(TEXT("Host result is valid"), Result.IsValid());
	TestTrue(TEXT("Pulse applies"), Result.IsSuccess());
	TestFalse(TEXT("First application is not replay"), Result.IsReplay());
	TestEqual(TEXT("Applied status"), Result.Status,
		Edemo_mapShanmenDivineSenseHostPulseStatus::Applied);
	TestEqual(TEXT("Result binds host"), Result.HostId, Host.GetHostId());
	TestEqual(TEXT("Result binds command"),
		Result.Command.GetCommandId(), Command.GetCommandId());
	TestEqual(TEXT("Resource before"),
		Result.ResourceBefore.GetCurrentAmount(), 100.0f);
	TestEqual(TEXT("Resource after"),
		Result.ResourceAfter.GetCurrentAmount(), 90.0f);
	TestEqual(TEXT("Revision advances twice"),
		Result.ResourceAfter.GetAuthorityRevision(), int64(2));
	TestEqual(TEXT("Host owns committed balance"),
		Host.GetCurrentSpiritEnergy(), 90.0f);
	TestEqual(TEXT("Host owns one pulse"), Host.NumProcessedPulses(), 1);
	TestEqual(TEXT("One evidence call"), Provider.Calls.Num(), 1);
	TestTrue(TEXT("Host remains valid"), Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductHostReplayAndCapacityTest,
	"Shanmen.0_0_10.Product.DivineSenseProductHost.ReplayAndCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductHostReplayAndCapacityTest::RunTest(
	const FString&)
{
	FDivineHostWorldFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	AActor* Source = nullptr;
	AActor* Subject = nullptr;
	if (!StartHostWorld(Fixture, Registry, Source, Subject))
	{
		return false;
	}
	FFixedHostEvidenceProvider Provider;
	Provider.Add(DivineHostSubjectId, HostLivingTags(), true, 3);
	Fdemo_mapShanmenDivineSenseProductHost Host;
	check(Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
		DivineHostRunId, DivineHostSourceId, 100.0f, 100.0f, 0, 2, Host));
	const FShanmenDivineSenseDefinition Definition = MakeHostDefinition();
	const FShanmenActionResourceCost Cost = MakeHostCost();
	const auto FirstCommand = MakeHostCommand(
		MakeHostAction(1), Definition, Cost, 2, 4);
	const auto Applied = Host.ExecutePulse(
		Fixture.World,
		Registry,
		Source,
		FirstCommand,
		{ Subject },
		Provider);
	if (!TestTrue(TEXT("Initial command applies"), Applied.IsSuccess()))
	{
		return false;
	}
	const int32 CallsAfterApply = Provider.Calls.Num();
	const FGuid SnapshotAfterApply =
		Applied.ResourceAfter.GetSnapshotId();

	Provider.UnavailableSubjectId = DivineHostSubjectId;
	const auto Replay = Host.ExecutePulse(
		nullptr,
		Registry,
		nullptr,
		FirstCommand,
		{},
		Provider);
	TestTrue(TEXT("Exact replay succeeds"), Replay.IsSuccess());
	TestTrue(TEXT("Exact replay is marked"), Replay.IsReplay());
	TestEqual(TEXT("Replay returns same pulse proof"),
		Replay.Pulse.Receipt.GetReceiptId(),
		Applied.Pulse.Receipt.GetReceiptId());
	TestEqual(TEXT("Replay resource is unchanged"),
		Replay.ResourceBefore.GetSnapshotId(), SnapshotAfterApply);
	TestEqual(TEXT("Replay after equals before"),
		Replay.ResourceAfter.GetSnapshotId(),
		Replay.ResourceBefore.GetSnapshotId());
	TestEqual(TEXT("Replay performs no evidence I/O"),
		Provider.Calls.Num(), CallsAfterApply);

	const auto ChangedCommand = MakeHostCommand(
		MakeHostAction(1), Definition, Cost, 2, 3);
	TestNotEqual(TEXT("Changed payload changes command identity"),
		ChangedCommand.GetCommandId(), FirstCommand.GetCommandId());
	const auto Conflict = Host.ExecutePulse(
		Fixture.World,
		Registry,
		Source,
		ChangedCommand,
		{ Subject },
		Provider);
	TestTrue(TEXT("Changed replay is structured"), Conflict.IsValid());
	TestEqual(TEXT("Changed activation is rejected"), Conflict.Error,
		Edemo_mapShanmenDivineSenseHostPulseError::PulseRejected);
	TestEqual(TEXT("Nested conflict remains visible"), Conflict.Pulse.Error,
		Edemo_mapShanmenDivineSensePulseError::ActivationConflict);
	TestEqual(TEXT("Conflict performs no evidence I/O"),
		Provider.Calls.Num(), CallsAfterApply);

	Provider.UnavailableSubjectId.Invalidate();
	const auto SecondCommand = MakeHostCommand(
		MakeHostAction(2), Definition, Cost);
	const auto Second = Host.ExecutePulse(
		Fixture.World,
		Registry,
		Source,
		SecondCommand,
		{ Subject },
		Provider);
	TestTrue(TEXT("Second capacity slot applies"), Second.IsSuccess());
	const int32 CallsAtCapacity = Provider.Calls.Num();
	Provider.UnavailableSubjectId = DivineHostSubjectId;
	const auto Capacity = Host.ExecutePulse(
		Fixture.World,
		Registry,
		Source,
		MakeHostCommand(MakeHostAction(3), Definition, Cost),
		{ Subject },
		Provider);
	TestEqual(TEXT("Host exposes bounded coordinator rejection"),
		Capacity.Pulse.Error,
		Edemo_mapShanmenDivineSensePulseError::
			ProcessedCapacityExceeded);
	TestEqual(TEXT("Capacity rejection performs no evidence I/O"),
		Provider.Calls.Num(), CallsAtCapacity);
	TestEqual(TEXT("Two pulses consumed exactly twice"),
		Host.GetCurrentSpiritEnergy(), 80.0f);
	TestEqual(TEXT("Host reaches exact capacity"),
		Host.NumProcessedPulses(), 2);
	TestTrue(TEXT("Host remains valid at capacity"), Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductHostRollbackAndRecoveryTest,
	"Shanmen.0_0_10.Product.DivineSenseProductHost.RollbackAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductHostRollbackAndRecoveryTest::RunTest(
	const FString&)
{
	FDivineHostWorldFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	AActor* Source = nullptr;
	AActor* Subject = nullptr;
	if (!StartHostWorld(Fixture, Registry, Source, Subject))
	{
		return false;
	}
	FFixedHostEvidenceProvider Provider;
	Provider.Add(DivineHostSubjectId, HostLivingTags(), true, 11);
	Provider.UnavailableSubjectId = DivineHostSubjectId;
	const FShanmenDivineSenseDefinition Definition = MakeHostDefinition();
	const FShanmenActionResourceCost Cost = MakeHostCost();
	const auto Command = MakeHostCommand(
		MakeHostAction(), Definition, Cost);
	Fdemo_mapShanmenDivineSenseProductHost Host;
	check(Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
		DivineHostRunId, DivineHostSourceId, 100.0f, 100.0f, 0, 2, Host));

	const auto Failed = Host.ExecutePulse(
		Fixture.World,
		Registry,
		Source,
		Command,
		{ Subject },
		Provider);
	TestTrue(TEXT("Evidence failure is structured"), Failed.IsValid());
	TestEqual(TEXT("Host classifies nested failure"), Failed.Error,
		Edemo_mapShanmenDivineSenseHostPulseError::PulseRejected);
	TestEqual(TEXT("World failure remains visible"), Failed.Pulse.Error,
		Edemo_mapShanmenDivineSensePulseError::WorldObservationRejected);
	TestEqual(TEXT("Rejected result proves unchanged state"),
		Failed.ResourceBefore.GetSnapshotId(),
		Failed.ResourceAfter.GetSnapshotId());
	TestEqual(TEXT("Failed staging preserves balance"),
		Host.GetCurrentSpiritEnergy(), 100.0f);
	TestEqual(TEXT("Failed staging preserves revision"),
		Host.GetResourceRevision(), int64(0));
	TestEqual(TEXT("Failed staging records no pulse"),
		Host.NumProcessedPulses(), 0);

	Provider.UnavailableSubjectId.Invalidate();
	const auto Retry = Host.ExecutePulse(
		Fixture.World,
		Registry,
		Source,
		Command,
		{ Subject },
		Provider);
	TestTrue(TEXT("Same command can recover"), Retry.IsSuccess());
	TestEqual(TEXT("Recovery consumes once"),
		Host.GetCurrentSpiritEnergy(), 90.0f);
	TestEqual(TEXT("Recovery records once"), Host.NumProcessedPulses(), 1);

	Fdemo_mapShanmenDivineSenseProductHost Insufficient;
	check(Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
		DivineHostRunId, DivineHostSourceId, 5.0f, 5.0f, 0, 2,
		Insufficient));
	const int32 CallsBeforeInsufficient = Provider.Calls.Num();
	const auto NoEnergy = Insufficient.ExecutePulse(
		Fixture.World,
		Registry,
		Source,
		Command,
		{ Subject },
		Provider);
	TestTrue(TEXT("Insufficient result is structured"), NoEnergy.IsValid());
	TestEqual(TEXT("Resource cause remains visible"),
		NoEnergy.Pulse.ResourceError,
		EShanmenActionResourceTransactionError::InsufficientAvailable);
	TestEqual(TEXT("Insufficient state is unchanged"),
		NoEnergy.ResourceBefore.GetSnapshotId(),
		NoEnergy.ResourceAfter.GetSnapshotId());
	TestEqual(TEXT("Insufficient path performs no evidence I/O"),
		Provider.Calls.Num(), CallsBeforeInsufficient);
	TestEqual(TEXT("Insufficient host remains untouched"),
		Insufficient.GetCurrentSpiritEnergy(), 5.0f);
	TestTrue(TEXT("Both hosts remain valid"),
		Host.IsValid() && Insufficient.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductHostAdmissionFencesTest,
	"Shanmen.0_0_10.Product.DivineSenseProductHost.AdmissionFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductHostAdmissionFencesTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenDivineSenseProductHost RejectedHost;
	TestFalse(TEXT("Invalid Run rejects"),
		Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			FGuid(), DivineHostSourceId, 10.0f, 10.0f, 0, 1,
			RejectedHost));
	TestFalse(TEXT("Invalid source rejects"),
		Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			DivineHostRunId, FGuid(), 10.0f, 10.0f, 0, 1,
			RejectedHost));
	TestFalse(TEXT("Negative balance rejects"),
		Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			DivineHostRunId, DivineHostSourceId, -1.0f, 10.0f, 0, 1,
			RejectedHost));
	TestFalse(TEXT("Balance above maximum rejects"),
		Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			DivineHostRunId, DivineHostSourceId, 11.0f, 10.0f, 0, 1,
			RejectedHost));
	TestFalse(TEXT("Negative revision rejects"),
		Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			DivineHostRunId, DivineHostSourceId, 10.0f, 10.0f, -1, 1,
			RejectedHost));
	TestFalse(TEXT("Zero pulse capacity rejects"),
		Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			DivineHostRunId, DivineHostSourceId, 10.0f, 10.0f, 0, 0,
			RejectedHost));
	TestFalse(TEXT("Revision budget must cover declared capacity"),
		Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			DivineHostRunId,
			DivineHostSourceId,
			10.0f,
			10.0f,
			MAX_int64 - 1,
			1,
			RejectedHost));

	const FShanmenDivineSenseDefinition Definition = MakeHostDefinition();
	const FShanmenActionResourceCost Cost = MakeHostCost();
	const FShanmenCombatActionSnapshot Action = MakeHostAction();
	Fdemo_mapShanmenDivineSensePulseCommand Command;
	TestFalse(TEXT("Default command is invalid"), Command.IsValid());
	TestFalse(TEXT("Non-Divine action rejects command capture"),
		Fdemo_mapShanmenDivineSensePulseCommand::TryCapture(
			MakeHostAction(
				1,
				DivineHostRunId,
				DivineHostSourceId,
				TEXT("Combat.Action.Sword.Basic01")),
			Definition,
			Cost,
			0,
			1,
			Command));
	TestFalse(TEXT("Non-SpiritEnergy cost rejects command capture"),
		Fdemo_mapShanmenDivineSensePulseCommand::TryCapture(
			Action,
			Definition,
			MakeHostCost(
				10.0f, FShanmenCombatRuntimeNativeTags::Resource()),
			0,
			1,
			Command));
	TestFalse(TEXT("Negative scan ordinal rejects"),
		Fdemo_mapShanmenDivineSensePulseCommand::TryCapture(
			Action, Definition, Cost, -1, 1, Command));
	TestFalse(TEXT("Negative Actor budget rejects"),
		Fdemo_mapShanmenDivineSensePulseCommand::TryCapture(
			Action, Definition, Cost, 0, -1, Command));

	Fdemo_mapShanmenDivineSenseProductHost Host;
	check(Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
		DivineHostRunId, DivineHostSourceId, 100.0f, 100.0f, 0, 2, Host));
	FFixedHostEvidenceProvider Provider;
	FShanmenWorldEntityRegistry EmptyRegistry;
	const auto InvalidCommand = Host.ExecutePulse(
		nullptr, EmptyRegistry, nullptr, Command, {}, Provider);
	TestTrue(TEXT("Invalid command rejection is structured"),
		InvalidCommand.IsValid());
	TestEqual(TEXT("Invalid command is classified"), InvalidCommand.Error,
		Edemo_mapShanmenDivineSenseHostPulseError::InvalidCommand);
	TestEqual(TEXT("Invalid command does not mutate resource"),
		InvalidCommand.ResourceBefore.GetSnapshotId(),
		InvalidCommand.ResourceAfter.GetSnapshotId());

	const auto ForeignRunCommand = MakeHostCommand(
		MakeHostAction(1, DivineHostOtherRunId), Definition, Cost);
	const auto ForeignRun = Host.ExecutePulse(
		nullptr, EmptyRegistry, nullptr, ForeignRunCommand, {}, Provider);
	TestEqual(TEXT("Foreign Run rejects at host boundary"), ForeignRun.Error,
		Edemo_mapShanmenDivineSenseHostPulseError::RunMismatch);
	const auto ForeignSourceCommand = MakeHostCommand(
		MakeHostAction(1, DivineHostRunId, DivineHostOtherSourceId),
		Definition,
		Cost);
	const auto ForeignSource = Host.ExecutePulse(
		nullptr, EmptyRegistry, nullptr, ForeignSourceCommand, {}, Provider);
	TestEqual(TEXT("Foreign source rejects at host boundary"),
		ForeignSource.Error,
		Edemo_mapShanmenDivineSenseHostPulseError::SourceMismatch);

	const auto ValidCommand = MakeHostCommand(Action, Definition, Cost);
	Fdemo_mapShanmenDivineSenseProductHost DefaultHost;
	const auto NotReady = DefaultHost.ExecutePulse(
		nullptr, EmptyRegistry, nullptr, ValidCommand, {}, Provider);
	TestTrue(TEXT("Default host rejection is structured"),
		NotReady.IsValid());
	TestEqual(TEXT("Default host is not ready"), NotReady.Error,
		Edemo_mapShanmenDivineSenseHostPulseError::HostNotReady);

	FDivineHostWorldFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	AActor* Source = nullptr;
	AActor* Subject = nullptr;
	if (!StartHostWorld(Fixture, Registry, Source, Subject))
	{
		return false;
	}
	Provider.Add(DivineHostSubjectId, HostLivingTags(), true, 1);
	const auto OverBudgetCommand = MakeHostCommand(
		Action, Definition, Cost, 0, 0);
	const auto OverBudget = Host.ExecutePulse(
		Fixture.World,
		Registry,
		Source,
		OverBudgetCommand,
		{ Subject },
		Provider);
	TestTrue(TEXT("Over-budget rejection is structured"),
		OverBudget.IsValid());
	TestEqual(TEXT("World budget cause remains visible"),
		OverBudget.Pulse.WorldFailure.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SampleBudgetExceeded);
	TestEqual(TEXT("Admission paths perform no evidence I/O"),
		Provider.Calls.Num(), 0);
	TestEqual(TEXT("All admission paths preserve balance"),
		Host.GetCurrentSpiritEnergy(), 100.0f);
	TestEqual(TEXT("All admission paths preserve ledger"),
		Host.NumProcessedPulses(), 0);
	TestTrue(TEXT("Host remains valid"), Host.IsValid());
	return true;
}

#endif
