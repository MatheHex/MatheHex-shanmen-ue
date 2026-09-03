#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseWorldObservationAdapter.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid DivineWorldRunId(0xD5100001, 0, 0, 1);
	const FGuid DivineWorldWrongRunId(0xD5100002, 0, 0, 1);
	const FGuid DivineWorldOwnerId(0xD5100003, 0, 0, 1);
	const FGuid DivineWorldSourceId(0xD5100004, 0, 0, 1);
	const FGuid DivineWorldWrongSourceId(0xD5100005, 0, 0, 1);
	const FGuid DivineWorldSubjectA(0xD5100010, 0, 0, 1);
	const FGuid DivineWorldSubjectB(0xD5100020, 0, 0, 1);
	const FGuid DivineWorldSubjectC(0xD5100030, 0, 0, 1);
	const FGuid DivineWorldSubjectD(0xD5100040, 0, 0, 1);
	const FGuid DivineWorldSubjectE(0xD5100050, 0, 0, 1);

	FGameplayTagContainer LivingTags(bool bBlocked = false)
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		if (bBlocked)
		{
			Tags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		}
		return Tags;
	}

	FGameplayTagContainer NonLivingTags()
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FShanmenCombatNativeTags::DamageSpirit());
		return Tags;
	}

	FShanmenCombatActionSnapshot MakeDivineWorldAction(
		const FGuid& RunId = DivineWorldRunId,
		const FGuid& SourceEntityId = DivineWorldSourceId)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = DivineWorldOwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P19.1");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P19.1-WORLD");
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1910);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenDivineSenseDefinition MakeDivineWorldDefinition(
		EShanmenDivineSenseOcclusionPolicy OcclusionPolicy =
			EShanmenDivineSenseOcclusionPolicy::VisibleOnly,
		double Radius = 100.0,
		int32 MaximumResults = 4)
	{
		FShanmenDivineSenseDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.ScanRuleId = TEXT("Spell.DivineSense.World.Basic01");
		Capture.Radius = Radius;
		Capture.MaximumResults = MaximumResults;
		Capture.OcclusionPolicy = OcclusionPolicy;
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

	struct FFixedEvidenceEntry
	{
		FGameplayTagContainer Tags;
		bool bHasLineOfSight = false;
		int64 AuthorityRevision = 0;
	};

	class FFixedDivineWorldEvidenceProvider final
		: public Idemo_mapShanmenDivineSenseWorldEvidenceProvider
	{
	public:
		TMap<FGuid, FFixedEvidenceEntry> Entries;
		FGuid UnavailableSubjectId;
		mutable TArray<FGuid> Calls;
		mutable FGuid LastScanId;
		mutable FVector LastSourceLocation = FVector::ZeroVector;
		mutable UWorld* LastWorld = nullptr;

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
			UWorld* World,
			const FShanmenDivineSenseScanRequest& Request,
			const AActor*,
			const AActor*,
			const FGuid& SubjectEntityId,
			const FVector& SourceLocation,
			const FVector&,
			Fdemo_mapShanmenDivineSenseWorldSubjectEvidence&
				OutEvidence) const override
		{
			OutEvidence =
				Fdemo_mapShanmenDivineSenseWorldSubjectEvidence();
			Calls.Add(SubjectEntityId);
			LastWorld = World;
			LastScanId = Request.GetScanId();
			LastSourceLocation = SourceLocation;
			if (SubjectEntityId == UnavailableSubjectId)
			{
				return false;
			}
			const FFixedEvidenceEntry* Entry = Entries.Find(
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

	struct FDivineWorldFixture
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

		~FDivineWorldFixture()
		{
			Stop();
		}
	};

	bool Bind(
		FShanmenWorldEntityRegistry& Registry,
		AActor* Actor,
		const FGuid& EntityId,
		const FGuid& RunId = DivineWorldRunId)
	{
		return Registry.BindObject(RunId, Actor, EntityId)
			== EShanmenWorldBindingResult::Bound;
	}

	bool ContainsSubject(
		const FShanmenDivineSenseScanReceipt& Receipt,
		const FGuid& SubjectEntityId)
	{
		for (const FShanmenDivineSenseReveal& Reveal : Receipt.GetReveals())
		{
			if (Reveal.GetObservation().GetSubjectEntityId()
				== SubjectEntityId)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseWorldCanonicalSampleTest,
	"Shanmen.0_0_10.Product.DivineSenseWorldObservation.CanonicalSample",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseWorldCanonicalSampleTest::RunTest(const FString&)
{
	FDivineWorldFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	if (!Fixture.Start() || !Registry.TryBeginRun(DivineWorldRunId))
	{
		AddError(TEXT("Could not start the P19.1 canonical World fixture."));
		return false;
	}
	AActor* Source = Fixture.Spawn(FVector(10.0, -0.0, 5.0));
	AActor* Far = Fixture.Spawn(FVector(70.0, 0.0, 5.0));
	AActor* Near = Fixture.Spawn(FVector(20.0, 0.0, 5.0));
	AActor* Edge = Fixture.Spawn(FVector(110.0, 0.0, 5.0));
	if (!Source || !Far || !Near || !Edge
		|| !Bind(Registry, Source, DivineWorldSourceId)
		|| !Bind(Registry, Far, DivineWorldSubjectA)
		|| !Bind(Registry, Near, DivineWorldSubjectB)
		|| !Bind(Registry, Edge, DivineWorldSubjectC))
	{
		AddError(TEXT("Could not bind the P19.1 canonical Actors."));
		return false;
	}

	FFixedDivineWorldEvidenceProvider Provider;
	Provider.Add(DivineWorldSubjectA, LivingTags(), true, 11);
	Provider.Add(DivineWorldSubjectB, LivingTags(), true, 12);
	Provider.Add(DivineWorldSubjectC, LivingTags(), true, 13);
	const FShanmenCombatActionSnapshot Action = MakeDivineWorldAction();
	const FShanmenDivineSenseDefinition Definition =
		MakeDivineWorldDefinition(
			EShanmenDivineSenseOcclusionPolicy::VisibleOnly, 100.0, 2);
	const Fdemo_mapShanmenDivineSenseWorldObservationResult First =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			Fixture.World,
			Registry,
			Source,
			Action,
			Definition,
			0,
			3,
			{ Edge, Far, Near },
			Provider);

	TestTrue(TEXT("Explicit registered Actor set resolves successfully"),
		First.IsSuccess());
	TestEqual(TEXT("All bounded subjects are observed once"),
		First.ObservedSubjectCount, 3);
	TestEqual(TEXT("Operational budget remains auditable"),
		First.SubjectActorBudget, 3);
	TestTrue(TEXT("Source transform becomes the immutable scan origin"),
		First.Receipt.GetRequest().GetOrigin()
			== FVector(10.0, 0.0, 5.0));
	TestEqual(TEXT("Capacity keeps two nearest reveals"),
		First.Receipt.NumReveals(), 2);
	TestEqual(TEXT("Nearest subject is first"),
		First.Receipt.GetReveals()[0].GetObservation().GetSubjectEntityId(),
		DivineWorldSubjectB);
	TestEqual(TEXT("Far subject is second"),
		First.Receipt.GetReveals()[1].GetObservation().GetSubjectEntityId(),
		DivineWorldSubjectA);
	TestEqual(TEXT("Evidence provider called once per subject"),
		Provider.Calls.Num(), 3);
	TestEqual(TEXT("Provider order is canonical A"),
		Provider.Calls[0], DivineWorldSubjectA);
	TestEqual(TEXT("Provider order is canonical B"),
		Provider.Calls[1], DivineWorldSubjectB);
	TestEqual(TEXT("Provider order is canonical C"),
		Provider.Calls[2], DivineWorldSubjectC);
	TestTrue(TEXT("Provider receives the exact World and Scan identity"),
		Provider.LastWorld == Fixture.World
			&& Provider.LastScanId == First.Receipt.GetRequest().GetScanId()
			&& Provider.LastSourceLocation == FVector(10.0, 0.0, 5.0));

	FFixedDivineWorldEvidenceProvider ReplayProvider;
	ReplayProvider.Entries = Provider.Entries;
	const Fdemo_mapShanmenDivineSenseWorldObservationResult Replay =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			Fixture.World,
			Registry,
			Source,
			Action,
			Definition,
			0,
			3,
			{ Near, Edge, Far },
			ReplayProvider);
	TestTrue(TEXT("Caller order cannot alter scan receipt identity"),
		Replay.IsSuccess()
			&& Replay.Receipt.GetReceiptId()
				== First.Receipt.GetReceiptId()
			&& ReplayProvider.Calls == Provider.Calls);

	FFixedDivineWorldEvidenceProvider EmptyProvider;
	const Fdemo_mapShanmenDivineSenseWorldObservationResult Empty =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			Fixture.World,
			Registry,
			Source,
			Action,
			Definition,
			0,
			0,
			{},
			EmptyProvider);
	TestTrue(TEXT("A zero-budget empty explicit sample is valid"),
		Empty.IsSuccess()
			&& Empty.ObservedSubjectCount == 0
			&& Empty.Receipt.NumReveals() == 0
			&& EmptyProvider.Calls.IsEmpty());
	TestEqual(TEXT("Sampling does not mutate the entity registry"),
		Registry.NumObjectBindings(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseWorldPolicyDelegationTest,
	"Shanmen.0_0_10.Product.DivineSenseWorldObservation.PolicyDelegation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseWorldPolicyDelegationTest::RunTest(const FString&)
{
	FDivineWorldFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	if (!Fixture.Start() || !Registry.TryBeginRun(DivineWorldRunId))
	{
		return false;
	}
	AActor* Source = Fixture.Spawn(FVector::ZeroVector);
	AActor* Visible = Fixture.Spawn(FVector(10.0, 0.0, 0.0));
	AActor* Hidden = Fixture.Spawn(FVector(20.0, 0.0, 0.0));
	AActor* Blocked = Fixture.Spawn(FVector(30.0, 0.0, 0.0));
	AActor* Outside = Fixture.Spawn(FVector(101.0, 0.0, 0.0));
	AActor* NonLiving = Fixture.Spawn(FVector(15.0, 0.0, 0.0));
	if (!Source || !Visible || !Hidden || !Blocked || !Outside
		|| !NonLiving
		|| !Bind(Registry, Source, DivineWorldSourceId)
		|| !Bind(Registry, Visible, DivineWorldSubjectA)
		|| !Bind(Registry, Hidden, DivineWorldSubjectB)
		|| !Bind(Registry, Blocked, DivineWorldSubjectC)
		|| !Bind(Registry, Outside, DivineWorldSubjectD)
		|| !Bind(Registry, NonLiving, DivineWorldSubjectE))
	{
		return false;
	}

	auto PopulateProvider = [](FFixedDivineWorldEvidenceProvider& Provider)
	{
		Provider.Add(DivineWorldSourceId, LivingTags(), true, 20);
		Provider.Add(DivineWorldSubjectA, LivingTags(), true, 21);
		Provider.Add(DivineWorldSubjectB, LivingTags(), false, 22);
		Provider.Add(DivineWorldSubjectC, LivingTags(true), true, 23);
		Provider.Add(DivineWorldSubjectD, LivingTags(), true, 24);
		Provider.Add(DivineWorldSubjectE, NonLivingTags(), true, 25);
	};
	const TArray<AActor*> Subjects = {
		Outside, Source, NonLiving, Hidden, Blocked, Visible
	};
	FFixedDivineWorldEvidenceProvider VisibleProvider;
	PopulateProvider(VisibleProvider);
	const Fdemo_mapShanmenDivineSenseWorldObservationResult VisibleOnly =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			Fixture.World,
			Registry,
			Source,
			MakeDivineWorldAction(),
			MakeDivineWorldDefinition(),
			1,
			6,
			Subjects,
			VisibleProvider);
	TestTrue(TEXT("World adapter delegates all filtering to P19.0"),
		VisibleOnly.IsSuccess()
			&& VisibleOnly.ObservedSubjectCount == 6
			&& VisibleOnly.Receipt.NumReveals() == 1
			&& ContainsSubject(
				VisibleOnly.Receipt, DivineWorldSubjectA)
			&& !ContainsSubject(
				VisibleOnly.Receipt, DivineWorldSourceId)
			&& !ContainsSubject(
				VisibleOnly.Receipt, DivineWorldSubjectB)
			&& !ContainsSubject(
				VisibleOnly.Receipt, DivineWorldSubjectC)
			&& !ContainsSubject(
				VisibleOnly.Receipt, DivineWorldSubjectD)
			&& !ContainsSubject(
				VisibleOnly.Receipt, DivineWorldSubjectE));

	FFixedDivineWorldEvidenceProvider RevealProvider;
	PopulateProvider(RevealProvider);
	const Fdemo_mapShanmenDivineSenseWorldObservationResult RevealOccluded =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			Fixture.World,
			Registry,
			Source,
			MakeDivineWorldAction(),
			MakeDivineWorldDefinition(
				EShanmenDivineSenseOcclusionPolicy::RevealOccluded),
			1,
			6,
			Subjects,
			RevealProvider);
	TestTrue(TEXT("Authored occlusion policy changes only P19.0 result"),
		RevealOccluded.IsSuccess()
			&& RevealOccluded.ObservedSubjectCount == 6
			&& RevealOccluded.Receipt.NumReveals() == 2
			&& ContainsSubject(
				RevealOccluded.Receipt, DivineWorldSubjectA)
			&& ContainsSubject(
				RevealOccluded.Receipt, DivineWorldSubjectB)
			&& RevealOccluded.Receipt.GetReveals()[1].WasOccluded());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseWorldAdmissionAndSourceFencesTest,
	"Shanmen.0_0_10.Product.DivineSenseWorldObservation.AdmissionAndSourceFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseWorldAdmissionAndSourceFencesTest::RunTest(
	const FString&)
{
	FDivineWorldFixture FirstWorld;
	FDivineWorldFixture SecondWorld;
	if (!FirstWorld.Start() || !SecondWorld.Start())
	{
		return false;
	}
	AActor* Source = FirstWorld.Spawn(FVector::ZeroVector);
	AActor* ForeignSource = SecondWorld.Spawn(FVector::ZeroVector);
	if (!Source || !ForeignSource)
	{
		return false;
	}
	const FShanmenCombatActionSnapshot Action = MakeDivineWorldAction();
	const FShanmenDivineSenseDefinition Definition =
		MakeDivineWorldDefinition();
	FFixedDivineWorldEvidenceProvider Provider;

	FShanmenWorldEntityRegistry Inactive;
	const auto InvalidRuntime =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			FirstWorld.World,
			Inactive,
			Source,
			FShanmenCombatActionSnapshot(),
			Definition,
			0,
			0,
			{},
			Provider);
	const auto InvalidBudget =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			FirstWorld.World,
			Inactive,
			Source,
			Action,
			Definition,
			0,
			-1,
			{},
			Provider);
	const auto OverBudget =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			FirstWorld.World,
			Inactive,
			Source,
			Action,
			Definition,
			0,
			0,
			{ Source },
			Provider);
	const auto NullWorld =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			nullptr,
			Inactive,
			Source,
			Action,
			Definition,
			0,
			0,
			{},
			Provider);
	const auto InactiveRegistry =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			FirstWorld.World,
			Inactive,
			Source,
			Action,
			Definition,
			0,
			0,
			{},
			Provider);

	FShanmenWorldEntityRegistry WrongRun;
	WrongRun.TryBeginRun(DivineWorldWrongRunId);
	const auto RunMismatch =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			FirstWorld.World,
			WrongRun,
			Source,
			Action,
			Definition,
			0,
			0,
			{},
			Provider);

	FShanmenWorldEntityRegistry Unregistered;
	Unregistered.TryBeginRun(DivineWorldRunId);
	const auto MissingSource =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			FirstWorld.World,
			Unregistered,
			Source,
			Action,
			Definition,
			0,
			0,
			{},
			Provider);
	const auto NullSource =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			FirstWorld.World,
			Unregistered,
			nullptr,
			Action,
			Definition,
			0,
			0,
			{},
			Provider);

	FShanmenWorldEntityRegistry WrongIdentity;
	WrongIdentity.TryBeginRun(DivineWorldRunId);
	Bind(WrongIdentity, Source, DivineWorldWrongSourceId);
	const auto SourceMismatch =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			FirstWorld.World,
			WrongIdentity,
			Source,
			Action,
			Definition,
			0,
			0,
			{},
			Provider);

	FShanmenWorldEntityRegistry ForeignRegistry;
	ForeignRegistry.TryBeginRun(DivineWorldRunId);
	Bind(ForeignRegistry, ForeignSource, DivineWorldSourceId);
	const auto SourceWorldMismatch =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			FirstWorld.World,
			ForeignRegistry,
			ForeignSource,
			Action,
			Definition,
			0,
			0,
			{},
			Provider);

	TestEqual(TEXT("Invalid Runtime input rejected first"),
		InvalidRuntime.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			RuntimeInputInvalid);
	TestEqual(TEXT("Negative sample budget rejected"),
		InvalidBudget.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SampleBudgetInvalid);
	TestEqual(TEXT("Oversized explicit set rejected before sampling"),
		OverBudget.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SampleBudgetExceeded);
	TestEqual(TEXT("Null World rejected"), NullWorld.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::WorldInvalid);
	TestEqual(TEXT("Inactive registry rejected"), InactiveRegistry.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::RegistryInactive);
	TestEqual(TEXT("Cross-Run registry rejected"), RunMismatch.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::RunMismatch);
	TestEqual(TEXT("Null source Actor rejected"), NullSource.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SourceActorUnavailable);
	TestEqual(TEXT("Unregistered source Actor rejected"),
		MissingSource.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SourceActorUnregistered);
	TestEqual(TEXT("Frozen source identity enforced"), SourceMismatch.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SourceIdentityMismatch);
	TestEqual(TEXT("Source Actor cannot cross Worlds"),
		SourceWorldMismatch.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SourceActorWorldMismatch);
	TestTrue(TEXT("Admission failures invoke no evidence callback"),
		Provider.Calls.IsEmpty());
	TestFalse(TEXT("Admission failure publishes no partial receipt"),
		SourceMismatch.Receipt.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseWorldSubjectEvidenceFencesTest,
	"Shanmen.0_0_10.Product.DivineSenseWorldObservation.SubjectEvidenceFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseWorldSubjectEvidenceFencesTest::RunTest(
	const FString&)
{
	FDivineWorldFixture FirstWorld;
	FDivineWorldFixture SecondWorld;
	FShanmenWorldEntityRegistry Registry;
	if (!FirstWorld.Start() || !SecondWorld.Start()
		|| !Registry.TryBeginRun(DivineWorldRunId))
	{
		return false;
	}
	AActor* Source = FirstWorld.Spawn(FVector::ZeroVector);
	AActor* Subject = FirstWorld.Spawn(FVector(10.0, 0.0, 0.0));
	AActor* Alias = FirstWorld.Spawn(FVector(20.0, 0.0, 0.0));
	AActor* Unregistered = FirstWorld.Spawn(FVector(30.0, 0.0, 0.0));
	AActor* Foreign = SecondWorld.Spawn(FVector(40.0, 0.0, 0.0));
	if (!Source || !Subject || !Alias || !Unregistered || !Foreign
		|| !Bind(Registry, Source, DivineWorldSourceId)
		|| !Bind(Registry, Subject, DivineWorldSubjectA)
		|| !Bind(Registry, Alias, DivineWorldSubjectA)
		|| !Bind(Registry, Foreign, DivineWorldSubjectB))
	{
		return false;
	}
	const FShanmenCombatActionSnapshot Action = MakeDivineWorldAction();
	const FShanmenDivineSenseDefinition Definition =
		MakeDivineWorldDefinition();
	FFixedDivineWorldEvidenceProvider Provider;
	Provider.Add(DivineWorldSubjectA, LivingTags(), true, 30);

	auto Sample = [&](const TArray<AActor*>& Actors)
	{
		return Fdemo_mapShanmenDivineSenseWorldObservationAdapter::
			SampleAndResolve(
				FirstWorld.World,
				Registry,
				Source,
				Action,
				Definition,
				2,
				Actors.Num(),
				Actors,
				Provider);
	};
	const auto NullSubject = Sample({ nullptr });
	const auto CrossWorld = Sample({ Foreign });
	const auto MissingBinding = Sample({ Unregistered });
	const auto Duplicate = Sample({ Subject, Alias });
	TestEqual(TEXT("Null subject Actor rejected"), NullSubject.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SubjectActorUnavailable);
	TestEqual(TEXT("Subject cannot cross Worlds"), CrossWorld.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SubjectActorWorldMismatch);
	TestEqual(TEXT("Unregistered subject rejected"), MissingBinding.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SubjectActorUnregistered);
	TestEqual(TEXT("Aliased entity transforms fail closed"), Duplicate.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			DuplicateSubjectEntity);
	TestTrue(TEXT("Whole Actor batch validates before provider calls"),
		Provider.Calls.IsEmpty());

	Provider.UnavailableSubjectId = DivineWorldSubjectA;
	const auto Unavailable = Sample({ Subject });
	TestEqual(TEXT("Provider rejection publishes no observation"),
		Unavailable.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SubjectEvidenceUnavailable);
	TestFalse(TEXT("Unavailable evidence leaves no receipt"),
		Unavailable.Receipt.IsValid());
	Provider.UnavailableSubjectId.Invalidate();
	Provider.Entries[DivineWorldSubjectA].Tags.Reset();
	const auto Invalid = Sample({ Subject });
	TestEqual(TEXT("Structurally invalid provider evidence rejected"),
		Invalid.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SubjectEvidenceInvalid);
	TestFalse(TEXT("Invalid evidence leaves no partial receipt"),
		Invalid.Receipt.IsValid());
	TestEqual(TEXT("Each accepted provider attempt occurs exactly once"),
		Provider.Calls.Num(), 2);
	return true;
}

#endif
