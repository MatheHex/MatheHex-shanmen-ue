#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSensePulseCoordinator.h"

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
	const FGuid DivinePulseRunId(0xD5200001, 0, 0, 1);
	const FGuid DivinePulseOtherRunId(0xD5200002, 0, 0, 1);
	const FGuid DivinePulseOwnerId(0xD5200003, 0, 0, 1);
	const FGuid DivinePulseSourceId(0xD5200004, 0, 0, 1);
	const FGuid DivinePulseOtherSourceId(0xD5200005, 0, 0, 1);
	const FGuid DivinePulseSubjectId(0xD5200010, 0, 0, 1);

	FGameplayTagContainer LivingTags()
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		return Tags;
	}

	FShanmenCombatActionSnapshot MakePulseAction(
		int32 ActivationSequence = 1,
		const FGuid& RunId = DivinePulseRunId,
		const FGuid& SourceEntityId = DivinePulseSourceId,
		FName ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId())
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = DivinePulseOwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content.Version = TEXT("0.0.10.P19.2");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P19.2-PULSE");
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenDivineSenseDefinition MakePulseDefinition()
	{
		FShanmenDivineSenseDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.ScanRuleId = TEXT("Spell.DivineSense.Pulse.Product01");
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

	FShanmenActionResourceCost MakePulseCost(
		float Amount = 10.0f,
		FGameplayTag Channel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy())
	{
		FShanmenActionResourceCostCapture Capture;
		Capture.RuleId = TEXT("Spell.DivineSense.Pulse.SpiritEnergy01");
		Capture.ResourceChannel = Channel;
		Capture.Amount = Amount;
		FShanmenActionResourceCost Cost;
		check(FShanmenActionResourceCost::TryCapture(Capture, Cost));
		return Cost;
	}

	FShanmenActionResourceAuthority MakePulseAuthority(
		float CurrentAmount = 100.0f,
		float MaximumAmount = 100.0f,
		const FGuid& OwnerEntityId = DivinePulseSourceId,
		FGameplayTag Channel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy())
	{
		FShanmenActionResourceAuthority Authority;
		check(FShanmenActionResourceAuthority::TryCreate(
			OwnerEntityId,
			Channel,
			CurrentAmount,
			MaximumAmount,
			0,
			Authority));
		return Authority;
	}

	struct FPulseEvidenceEntry
	{
		FGameplayTagContainer Tags;
		bool bHasLineOfSight = true;
		int64 AuthorityRevision = 0;
	};

	class FFixedPulseEvidenceProvider final
		: public Idemo_mapShanmenDivineSenseWorldEvidenceProvider
	{
	public:
		TMap<FGuid, FPulseEvidenceEntry> Entries;
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
			const FPulseEvidenceEntry* Entry = Entries.Find(
				SubjectEntityId);
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

	struct FDivinePulseWorldFixture
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

		~FDivinePulseWorldFixture()
		{
			Stop();
		}
	};

	bool BindPulseActor(
		FShanmenWorldEntityRegistry& Registry,
		AActor* Actor,
		const FGuid& EntityId)
	{
		return Registry.BindObject(
			DivinePulseRunId, Actor, EntityId)
			== EShanmenWorldBindingResult::Bound;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSensePulseAtomicCommitTest,
	"Shanmen.0_0_10.Product.DivineSensePulseCoordinator.AtomicCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSensePulseAtomicCommitTest::RunTest(const FString&)
{
	FDivinePulseWorldFixture Fixture;
	if (!TestTrue(TEXT("World fixture starts"), Fixture.Start()))
	{
		return false;
	}
	AActor* Source = Fixture.Spawn(FVector::ZeroVector);
	AActor* Subject = Fixture.Spawn(FVector(30.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Source exists"), Source)
		|| !TestNotNull(TEXT("Subject exists"), Subject))
	{
		return false;
	}

	FShanmenWorldEntityRegistry Registry;
	TestTrue(TEXT("Registry begins"), Registry.TryBeginRun(DivinePulseRunId));
	TestTrue(TEXT("Source binds"), BindPulseActor(
		Registry, Source, DivinePulseSourceId));
	TestTrue(TEXT("Subject binds"), BindPulseActor(
		Registry, Subject, DivinePulseSubjectId));
	FFixedPulseEvidenceProvider Provider;
	Provider.Add(DivinePulseSubjectId, LivingTags(), true, 7);

	const FShanmenCombatActionSnapshot Action = MakePulseAction();
	const FShanmenDivineSenseDefinition Definition = MakePulseDefinition();
	const FShanmenActionResourceCost Cost = MakePulseCost();
	FShanmenActionResourceAuthority Authority = MakePulseAuthority();
	Fdemo_mapShanmenDivineSensePulseCoordinator Coordinator;
	TestTrue(TEXT("Coordinator opens"),
		Fdemo_mapShanmenDivineSensePulseCoordinator::TryCreate(
			DivinePulseRunId, 2, Coordinator));

	const Fdemo_mapShanmenDivineSensePulseResult Result =
		Coordinator.Execute(
			Fixture.World,
			Registry,
			Source,
			Action,
			Definition,
			Cost,
			0,
			4,
			{ Subject },
			Provider,
			Authority);

	TestTrue(TEXT("Pulse result is valid"), Result.IsValid());
	TestTrue(TEXT("Pulse applies"), Result.IsSuccess());
	TestEqual(TEXT("Pulse status"), Result.Status,
		Edemo_mapShanmenDivineSensePulseStatus::Applied);
	TestFalse(TEXT("First apply is not replay"), Result.IsReplay());
	TestTrue(TEXT("Receipt is valid"), Result.Receipt.IsValid());
	TestEqual(TEXT("One provider call"), Provider.Calls.Num(), 1);
	TestEqual(TEXT("Provider sees subject"), Provider.Calls[0],
		DivinePulseSubjectId);
	TestEqual(TEXT("One reveal"),
		Result.Receipt.GetWorldObservation().Receipt.NumReveals(), 1);
	TestEqual(TEXT("Reveal identity"),
		Result.Receipt.GetWorldObservation().Receipt.GetReveals()[0]
			.GetObservation().GetSubjectEntityId(),
		DivinePulseSubjectId);
	TestEqual(TEXT("Startup sequence"),
		Result.Receipt.GetStartup().GetSequence(), int64(0));
	TestEqual(TEXT("Commit sequence"),
		Result.Receipt.GetActiveCommit().GetSequence(), int64(1));
	TestEqual(TEXT("Recovery sequence"),
		Result.Receipt.GetRecovery().GetSequence(), int64(2));
	TestEqual(TEXT("Completion sequence"),
		Result.Receipt.GetCompletion().GetSequence(), int64(3));
	TestEqual(TEXT("Completion reason"),
		Result.Receipt.GetCompletion().GetTerminalReason(),
		EShanmenActionTerminalReason::Completed);
	TestEqual(TEXT("Resource committed once"),
		Authority.GetCurrentAmount(), 90.0f);
	TestEqual(TEXT("No reservation remains"),
		Authority.GetReservedAmount(), 0.0f);
	TestEqual(TEXT("Reserve and commit advance revision"),
		Authority.GetAuthorityRevision(), int64(2));
	TestEqual(TEXT("One pulse recorded"),
		Coordinator.NumProcessedPulses(), 1);
	TestTrue(TEXT("Coordinator remains valid"), Coordinator.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSensePulseReplayAndCapacityTest,
	"Shanmen.0_0_10.Product.DivineSensePulseCoordinator.ReplayAndCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSensePulseReplayAndCapacityTest::RunTest(const FString&)
{
	FDivinePulseWorldFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	AActor* Source = Fixture.Spawn(FVector::ZeroVector);
	AActor* Subject = Fixture.Spawn(FVector(20.0, 0.0, 0.0));
	if (!Source || !Subject)
	{
		return false;
	}
	FShanmenWorldEntityRegistry Registry;
	check(Registry.TryBeginRun(DivinePulseRunId));
	check(BindPulseActor(Registry, Source, DivinePulseSourceId));
	check(BindPulseActor(Registry, Subject, DivinePulseSubjectId));
	FFixedPulseEvidenceProvider Provider;
	Provider.Add(DivinePulseSubjectId, LivingTags(), true, 3);

	const FShanmenCombatActionSnapshot Action = MakePulseAction();
	const FShanmenDivineSenseDefinition Definition = MakePulseDefinition();
	const FShanmenActionResourceCost Cost = MakePulseCost();
	FShanmenActionResourceAuthority Authority = MakePulseAuthority();
	Fdemo_mapShanmenDivineSensePulseCoordinator Coordinator;
	check(Fdemo_mapShanmenDivineSensePulseCoordinator::TryCreate(
		DivinePulseRunId, 1, Coordinator));
	const auto Applied = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		Cost,
		2,
		4,
		{ Subject },
		Provider,
		Authority);
	if (!TestTrue(TEXT("Initial pulse applies"), Applied.IsSuccess()))
	{
		return false;
	}
	const int32 CallsAfterApply = Provider.Calls.Num();
	const float AmountAfterApply = Authority.GetCurrentAmount();
	const int64 RevisionAfterApply = Authority.GetAuthorityRevision();

	Provider.UnavailableSubjectId = DivinePulseSubjectId;
	const auto Replay = Coordinator.Execute(
		nullptr,
		Registry,
		nullptr,
		Action,
		Definition,
		Cost,
		2,
		4,
		{},
		Provider,
		Authority);
	TestTrue(TEXT("Replay succeeds"), Replay.IsSuccess());
	TestTrue(TEXT("Replay is marked"), Replay.IsReplay());
	TestEqual(TEXT("Replay status"), Replay.Status,
		Edemo_mapShanmenDivineSensePulseStatus::AlreadyApplied);
	TestEqual(TEXT("Replay receipt is identical"),
		Replay.Receipt.GetReceiptId(), Applied.Receipt.GetReceiptId());
	TestEqual(TEXT("Replay performs no provider I/O"),
		Provider.Calls.Num(), CallsAfterApply);
	TestEqual(TEXT("Replay does not consume again"),
		Authority.GetCurrentAmount(), AmountAfterApply);
	TestEqual(TEXT("Replay does not advance revision"),
		Authority.GetAuthorityRevision(), RevisionAfterApply);

	const auto Conflict = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		Cost,
		2,
		5,
		{ Subject },
		Provider,
		Authority);
	TestTrue(TEXT("Changed replay is structured rejection"),
		Conflict.IsValid());
	TestEqual(TEXT("Changed budget conflicts"), Conflict.Error,
		Edemo_mapShanmenDivineSensePulseError::ActivationConflict);

	const FShanmenCombatActionSnapshot NextAction = MakePulseAction(2);
	const auto Capacity = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		NextAction,
		Definition,
		Cost,
		0,
		4,
		{ Subject },
		Provider,
		Authority);
	TestTrue(TEXT("Capacity rejection is valid"), Capacity.IsValid());
	TestEqual(TEXT("Capacity is bounded"), Capacity.Error,
		Edemo_mapShanmenDivineSensePulseError::
			ProcessedCapacityExceeded);
	TestEqual(TEXT("Rejected requests perform no provider I/O"),
		Provider.Calls.Num(), CallsAfterApply);

	FShanmenActionResourceAuthority FreshAuthority = MakePulseAuthority();
	const auto DesynchronizedReplay = Coordinator.Execute(
		nullptr,
		Registry,
		nullptr,
		Action,
		Definition,
		Cost,
		2,
		4,
		{},
		Provider,
		FreshAuthority);
	TestTrue(TEXT("Desynchronized replay is structured"),
		DesynchronizedReplay.IsValid());
	TestEqual(TEXT("Replay requires matching resource ledger"),
		DesynchronizedReplay.Error,
		Edemo_mapShanmenDivineSensePulseError::StateDesynchronized);
	TestEqual(TEXT("Replay check does not mutate foreign ledger"),
		FreshAuthority.GetAuthorityRevision(), int64(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSensePulseAtomicRollbackTest,
	"Shanmen.0_0_10.Product.DivineSensePulseCoordinator.AtomicRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSensePulseAtomicRollbackTest::RunTest(const FString&)
{
	FDivinePulseWorldFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	AActor* Source = Fixture.Spawn(FVector::ZeroVector);
	AActor* Subject = Fixture.Spawn(FVector(25.0, 0.0, 0.0));
	if (!Source || !Subject)
	{
		return false;
	}
	FShanmenWorldEntityRegistry Registry;
	check(Registry.TryBeginRun(DivinePulseRunId));
	check(BindPulseActor(Registry, Source, DivinePulseSourceId));
	check(BindPulseActor(Registry, Subject, DivinePulseSubjectId));
	FFixedPulseEvidenceProvider Provider;
	Provider.Add(DivinePulseSubjectId, LivingTags(), true, 11);

	const FShanmenCombatActionSnapshot Action = MakePulseAction();
	const FShanmenDivineSenseDefinition Definition = MakePulseDefinition();
	const FShanmenActionResourceCost Cost = MakePulseCost();
	Fdemo_mapShanmenDivineSensePulseCoordinator Coordinator;
	check(Fdemo_mapShanmenDivineSensePulseCoordinator::TryCreate(
		DivinePulseRunId, 2, Coordinator));

	FShanmenActionResourceAuthority Insufficient =
		MakePulseAuthority(5.0f, 5.0f);
	const auto NoEnergy = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		Cost,
		0,
		4,
		{ Subject },
		Provider,
		Insufficient);
	TestTrue(TEXT("Insufficient result is structured"), NoEnergy.IsValid());
	TestEqual(TEXT("Insufficient resource rejects"), NoEnergy.Error,
		Edemo_mapShanmenDivineSensePulseError::ResourceReservationRejected);
	TestEqual(TEXT("Resource cause is retained"), NoEnergy.ResourceError,
		EShanmenActionResourceTransactionError::InsufficientAvailable);
	TestEqual(TEXT("Insufficient balance unchanged"),
		Insufficient.GetCurrentAmount(), 5.0f);
	TestEqual(TEXT("Insufficient revision unchanged"),
		Insufficient.GetAuthorityRevision(), int64(0));
	TestEqual(TEXT("No evidence requested"), Provider.Calls.Num(), 0);
	TestEqual(TEXT("No failed pulse recorded"),
		Coordinator.NumProcessedPulses(), 0);

	FShanmenActionResourceAuthority Authority = MakePulseAuthority();
	Provider.UnavailableSubjectId = DivinePulseSubjectId;
	const auto EvidenceFailure = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		Cost,
		0,
		4,
		{ Subject },
		Provider,
		Authority);
	TestTrue(TEXT("Evidence failure is structured"),
		EvidenceFailure.IsValid());
	TestEqual(TEXT("World rejection is classified"),
		EvidenceFailure.Error,
		Edemo_mapShanmenDivineSensePulseError::WorldObservationRejected);
	TestEqual(TEXT("World cause is retained"),
		EvidenceFailure.WorldFailure.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SubjectEvidenceUnavailable);
	TestEqual(TEXT("Staged resource commit rolls back"),
		Authority.GetCurrentAmount(), 100.0f);
	TestEqual(TEXT("Rollback clears reservation"),
		Authority.GetReservedAmount(), 0.0f);
	TestEqual(TEXT("Rollback preserves revision"),
		Authority.GetAuthorityRevision(), int64(0));
	TestEqual(TEXT("Failed pulse is not recorded"),
		Coordinator.NumProcessedPulses(), 0);

	Provider.UnavailableSubjectId.Invalidate();
	const auto Retry = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		Cost,
		0,
		4,
		{ Subject },
		Provider,
		Authority);
	TestTrue(TEXT("Same activation may retry after atomic rollback"),
		Retry.IsSuccess());
	TestEqual(TEXT("Successful retry commits once"),
		Authority.GetCurrentAmount(), 90.0f);
	TestEqual(TEXT("Successful retry records once"),
		Coordinator.NumProcessedPulses(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSensePulseAdmissionFencesTest,
	"Shanmen.0_0_10.Product.DivineSensePulseCoordinator.AdmissionFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSensePulseAdmissionFencesTest::RunTest(const FString&)
{
	FDivinePulseWorldFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	AActor* Source = Fixture.Spawn(FVector::ZeroVector);
	AActor* Subject = Fixture.Spawn(FVector(10.0, 0.0, 0.0));
	if (!Source || !Subject)
	{
		return false;
	}
	FShanmenWorldEntityRegistry Registry;
	check(Registry.TryBeginRun(DivinePulseRunId));
	check(BindPulseActor(Registry, Source, DivinePulseSourceId));
	check(BindPulseActor(Registry, Subject, DivinePulseSubjectId));
	FFixedPulseEvidenceProvider Provider;
	Provider.Add(DivinePulseSubjectId, LivingTags(), true, 1);
	const FShanmenCombatActionSnapshot Action = MakePulseAction();
	const FShanmenDivineSenseDefinition Definition = MakePulseDefinition();
	const FShanmenActionResourceCost Cost = MakePulseCost();
	FShanmenActionResourceAuthority Authority = MakePulseAuthority();

	Fdemo_mapShanmenDivineSensePulseCoordinator InvalidCoordinator;
	const auto NotReady = InvalidCoordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		Cost,
		0,
		1,
		{ Subject },
		Provider,
		Authority);
	TestEqual(TEXT("Default coordinator rejects"), NotReady.Error,
		Edemo_mapShanmenDivineSensePulseError::CoordinatorNotReady);
	TestFalse(TEXT("Invalid Run cannot create coordinator"),
		Fdemo_mapShanmenDivineSensePulseCoordinator::TryCreate(
			FGuid(), 1, InvalidCoordinator));
	TestFalse(TEXT("Zero ledger capacity rejects"),
		Fdemo_mapShanmenDivineSensePulseCoordinator::TryCreate(
			DivinePulseRunId, 0, InvalidCoordinator));

	Fdemo_mapShanmenDivineSensePulseCoordinator Coordinator;
	check(Fdemo_mapShanmenDivineSensePulseCoordinator::TryCreate(
		DivinePulseRunId, 2, Coordinator));
	const auto WrongRun = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		MakePulseAction(1, DivinePulseOtherRunId),
		Definition,
		Cost,
		0,
		1,
		{ Subject },
		Provider,
		Authority);
	TestEqual(TEXT("Foreign Run rejects"), WrongRun.Error,
		Edemo_mapShanmenDivineSensePulseError::RunMismatch);

	const auto WrongAction = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		MakePulseAction(1, DivinePulseRunId, DivinePulseSourceId,
			TEXT("Combat.Action.Sword.Basic01")),
		Definition,
		Cost,
		0,
		1,
		{ Subject },
		Provider,
		Authority);
	TestEqual(TEXT("Non-Divine action rejects"), WrongAction.Error,
		Edemo_mapShanmenDivineSensePulseError::InvalidInput);

	const FShanmenActionResourceCost WrongCost = MakePulseCost(
		10.0f, FShanmenCombatRuntimeNativeTags::Resource());
	const auto WrongCostResult = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		WrongCost,
		0,
		1,
		{ Subject },
		Provider,
		Authority);
	TestEqual(TEXT("Non-SpiritEnergy cost rejects"),
		WrongCostResult.Error,
		Edemo_mapShanmenDivineSensePulseError::InvalidInput);

	FShanmenActionResourceAuthority WrongOwner = MakePulseAuthority(
		100.0f, 100.0f, DivinePulseOtherSourceId);
	const auto WrongOwnerResult = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		Cost,
		0,
		1,
		{ Subject },
		Provider,
		WrongOwner);
	TestEqual(TEXT("Foreign resource owner rejects"),
		WrongOwnerResult.Error,
		Edemo_mapShanmenDivineSensePulseError::ResourceOwnerMismatch);

	FShanmenActionResourceAuthority WrongChannel = MakePulseAuthority(
		100.0f,
		100.0f,
		DivinePulseSourceId,
		FShanmenCombatRuntimeNativeTags::Resource());
	const auto WrongChannelResult = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		Cost,
		0,
		1,
		{ Subject },
		Provider,
		WrongChannel);
	TestEqual(TEXT("Resource channel mismatch rejects"),
		WrongChannelResult.Error,
		Edemo_mapShanmenDivineSensePulseError::ResourceChannelMismatch);

	const auto OverBudget = Coordinator.Execute(
		Fixture.World,
		Registry,
		Source,
		Action,
		Definition,
		Cost,
		0,
		0,
		{ Subject },
		Provider,
		Authority);
	TestTrue(TEXT("World admission rejection is structured"),
		OverBudget.IsValid());
	TestEqual(TEXT("Over-budget failure remains visible"),
		OverBudget.WorldFailure.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SampleBudgetExceeded);
	TestEqual(TEXT("Over-budget pulse does not consume"),
		Authority.GetCurrentAmount(), 100.0f);
	TestEqual(TEXT("Over-budget pulse does not revise"),
		Authority.GetAuthorityRevision(), int64(0));
	TestEqual(TEXT("Admission failures perform no evidence I/O"),
		Provider.Calls.Num(), 0);
	TestEqual(TEXT("Admission failures are not recorded"),
		Coordinator.NumProcessedPulses(), 0);
	return true;
}

#endif
