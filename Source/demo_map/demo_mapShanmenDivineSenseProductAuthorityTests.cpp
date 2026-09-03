#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseProductAuthority.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const EAutomationTestFlags ProductAuthorityFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid DivineSenseAuthorityRunA(
		0xD5700001, 0xD5700002, 0xD5700003, 0xD5700004);
	const FGuid DivineSenseAuthorityRunB(
		0xD5710001, 0xD5710002, 0xD5710003, 0xD5710004);

	struct FDivineSenseAuthorityFixture
	{
		UWorld* World = nullptr;
		APawn* Player = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;

		bool Start(const FGuid& RunId = DivineSenseAuthorityRunA)
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
					TEXT("P197DivineSensePlayerRoot"),
					RF_Transient)
				: nullptr;
			Health = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player,
					TEXT("P197DivineSensePlayerHealth"),
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

		~FDivineSenseAuthorityFixture()
		{
			Stop();
		}
	};

	Fdemo_mapShanmenDivineSenseProductConfig MakeCanonicalConfig()
	{
		Fdemo_mapShanmenDivineSenseProductConfig Config;
		check(Fdemo_mapShanmenDivineSenseProductAuthority::
			TryCreateCanonicalConfig(Config));
		return Config;
	}

	Fdemo_mapShanmenDivineSenseProductConfig MakeAlternateConfig()
	{
		FShanmenDivineSenseDefinitionCapture DefinitionCapture;
		DefinitionCapture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		DefinitionCapture.ScanRuleId =
			TEXT("Spell.DivineSense.Pulse.NonCanonical");
		DefinitionCapture.Radius =
			Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalRadius();
		DefinitionCapture.MaximumResults =
			Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalMaximumResults();
		DefinitionCapture.OcclusionPolicy =
			Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalOcclusionPolicy();
		DefinitionCapture.RequiredSubjectTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		DefinitionCapture.bRejectSelf = true;
		FShanmenDivineSenseDefinition Definition;
		check(FShanmenDivineSenseDefinition::TryCapture(
			DefinitionCapture, Definition));

		FShanmenActionResourceCostCapture CostCapture;
		CostCapture.RuleId =
			Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalCostRuleId();
		CostCapture.ResourceChannel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy();
		CostCapture.Amount =
			Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalSpiritEnergyCost();
		FShanmenActionResourceCost Cost;
		check(FShanmenActionResourceCost::TryCapture(CostCapture, Cost));

		Fdemo_mapShanmenDivineSenseProductConfig Config;
		check(Fdemo_mapShanmenDivineSenseProductConfig::TryCapture(
			Definition,
			Cost,
			Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalPulseCapacity(),
			Config));
		return Config;
	}

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

	class FNoReadEvidenceProvider final
		: public Idemo_mapShanmenDivineSenseWorldEvidenceProvider
	{
	public:
		mutable int32 CallCount = 0;

		virtual bool TryCaptureSubjectEvidence(
			UWorld*,
			const FShanmenDivineSenseScanRequest&,
			const AActor*,
			const AActor*,
			const FGuid&,
			const FVector&,
			const FVector&,
			Fdemo_mapShanmenDivineSenseWorldSubjectEvidence&) const override
		{
			++CallCount;
			return false;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductAuthorityCanonicalConfigTest,
	"Shanmen.0_0_10.Product.DivineSenseProductAuthority.CanonicalConfig",
	ProductAuthorityFlags)

bool Fdemo_mapDivineSenseProductAuthorityCanonicalConfigTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenDivineSenseProductConfig Empty;
	TestFalse(TEXT("default config is invalid"), Empty.IsValid());
	const Fdemo_mapShanmenDivineSenseProductConfig First =
		MakeCanonicalConfig();
	const Fdemo_mapShanmenDivineSenseProductConfig Replay =
		MakeCanonicalConfig();
	const FShanmenDivineSenseDefinition& Definition =
		First.GetDefinition();
	TestTrue(TEXT("canonical config is valid and recognized"),
		First.IsValid()
			&& Fdemo_mapShanmenDivineSenseProductAuthority::
				IsCanonicalConfig(First));
	TestEqual(TEXT("canonical config identity is replay stable"),
		First.GetConfigId(), Replay.GetConfigId());
	TestEqual(TEXT("canonical config id is exposed by authority"),
		First.GetConfigId(),
		Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalConfigId());
	TestEqual(TEXT("scan rule is product-owned"),
		Definition.GetScanRuleId(),
		Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalScanRuleId());
	TestEqual(TEXT("scan radius is product-owned"),
		Definition.GetRadius(), 1200.0);
	TestEqual(TEXT("result capacity is product-owned"),
		Definition.GetMaximumResults(), 8);
	TestEqual(TEXT("occluded evidence is explicitly revealable"),
		Definition.GetOcclusionPolicy(),
		EShanmenDivineSenseOcclusionPolicy::RevealOccluded);
	TestTrue(TEXT("only living subjects are required"),
		Definition.GetRequiredSubjectTags().HasTagExact(
			FShanmenCombatNativeTags::TargetLiving())
			&& Definition.GetRequiredSubjectTags().Num() == 1
			&& Definition.GetBlockedSubjectTags().IsEmpty()
			&& Definition.RejectsSelf());
	TestEqual(TEXT("SpiritEnergy rule is product-owned"),
		First.GetCost().GetRuleId(),
		Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalCostRuleId());
	TestEqual(TEXT("SpiritEnergy amount is product-owned"),
		First.GetCost().GetAmount(), 10.0f);
	TestEqual(TEXT("Run pulse capacity is product-owned"),
		First.GetPulseCapacity(), 16);
	TestEqual(TEXT("subject budget is product-owned"),
		Fdemo_mapShanmenDivineSenseProductAuthority::
			CanonicalSubjectActorBudget(), 32);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductAuthorityReservationFencesTest,
	"Shanmen.0_0_10.Product.DivineSenseProductAuthority.ReservationFences",
	ProductAuthorityFlags)

bool Fdemo_mapDivineSenseProductAuthorityReservationFencesTest::RunTest(
	const FString&)
{
	Fdemo_mapCombatRunCoordinator Unready;
	Fdemo_mapPlayerDivineSenseActionReservation Reservation;
	FString Diagnostic;
	TestFalse(TEXT("unready Run cannot reserve Divine Sense identity"),
		Unready.TryReservePlayerDivineSenseAction(
			MakeCanonicalConfig(), Reservation, Diagnostic));
	TestFalse(TEXT("unready rejection returns no reservation"),
		Reservation.IsValid());
	const auto UnreadyPrepared =
		Fdemo_mapShanmenDivineSenseProductAuthority::PrepareIntent(Unready);
	TestEqual(TEXT("authority classifies unready Run rejection"),
		UnreadyPrepared.Status,
		Edemo_mapShanmenDivineSenseProductPrepareStatus::
			ReservationRejected);

	FDivineSenseAuthorityFixture Fixture;
	TestTrue(TEXT("fixture starts"), Fixture.Start());
	const Fdemo_mapShanmenDivineSenseProductConfig Alternate =
		MakeAlternateConfig();
	TestTrue(TEXT("alternate config is structurally valid"),
		Alternate.IsValid());
	TestFalse(TEXT("alternate values are not canonical"),
		Fdemo_mapShanmenDivineSenseProductAuthority::
			IsCanonicalConfig(Alternate));
	TestFalse(TEXT("valid but noncanonical config cannot reserve"),
		Fixture.Coordinator.TryReservePlayerDivineSenseAction(
			Alternate, Reservation, Diagnostic));
	TestEqual(TEXT("rejected config leaves sequence untouched"),
		Fixture.Coordinator.GetNextPlayerDivineSenseActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductAuthoritySequentialReservationTest,
	"Shanmen.0_0_10.Product.DivineSenseProductAuthority.SequentialReservation",
	ProductAuthorityFlags)

bool Fdemo_mapDivineSenseProductAuthoritySequentialReservationTest::RunTest(
	const FString&)
{
	FDivineSenseAuthorityFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const auto Config = MakeCanonicalConfig();
	Fdemo_mapPlayerDivineSenseActionReservation First;
	Fdemo_mapPlayerDivineSenseActionReservation Second;
	TestTrue(TEXT("first identity is reserved"),
		Fixture.Coordinator.TryReservePlayerDivineSenseAction(
			Config, First, Fixture.Diagnostic));
	TestTrue(TEXT("first reservation validates its own provenance"),
		First.IsValid());
	TestEqual(TEXT("first sequence begins at one"),
		First.GetActivationSequence(), static_cast<uint64>(1));
	TestTrue(TEXT("action is Run- and player-owned without item forgery"),
		First.GetAction().GetRunId() == Fixture.Coordinator.GetRunId()
			&& First.GetAction().GetOwnerId()
				== Fixture.Coordinator.GetPlayerEntityId()
			&& First.GetAction().GetSourceEntityId()
				== Fixture.Coordinator.GetPlayerEntityId()
			&& !First.GetAction().GetSourceItemInstanceId().IsValid());
	TestTrue(TEXT("action carries exact product content and player tag"),
		First.GetAction().GetContent().Version
			== Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalContentVersion()
			&& First.GetAction().GetContent().Digest
				== Fdemo_mapShanmenDivineSenseProductAuthority::
					CanonicalContentDigest()
			&& First.GetAction().GetSourceTags().HasTagExact(
				FShanmenCombatNativeTags::SourcePlayer()));
	TestEqual(TEXT("activation id is derived from the Run sequence"),
		First.GetActivationId(),
		FShanmenCombatIdFactory::MakeActivationId(
			First.GetAction().GetRunId(),
			First.GetAction().GetSourceEntityId(),
			First.GetAction().GetActionDefinitionId(),
			First.GetActivationSequence()));
	TestTrue(TEXT("second identity is reserved"),
		Fixture.Coordinator.TryReservePlayerDivineSenseAction(
			Config, Second, Fixture.Diagnostic));
	TestEqual(TEXT("second sequence is monotonic"),
		Second.GetActivationSequence(), static_cast<uint64>(2));
	TestNotEqual(TEXT("separate attempts cannot share identity"),
		Second.GetActivationId(), First.GetActivationId());
	TestEqual(TEXT("next sequence advances after two acceptances"),
		Fixture.Coordinator.GetNextPlayerDivineSenseActivationSequence(),
		static_cast<uint64>(3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductAuthorityRunResetTest,
	"Shanmen.0_0_10.Product.DivineSenseProductAuthority.RunReset",
	ProductAuthorityFlags)

bool Fdemo_mapDivineSenseProductAuthorityRunResetTest::RunTest(
	const FString&)
{
	FDivineSenseAuthorityFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const auto Config = MakeCanonicalConfig();
	Fdemo_mapPlayerDivineSenseActionReservation FirstRun;
	TestTrue(TEXT("first Run reserves one identity"),
		Fixture.Coordinator.TryReservePlayerDivineSenseAction(
			Config, FirstRun, Fixture.Diagnostic));
	TestTrue(TEXT("first Run closes cleanly"),
		Fixture.Coordinator.TryEndRun(
			DivineSenseAuthorityRunA, Fixture.Diagnostic));
	TestEqual(TEXT("closed Run resets its local sequence"),
		Fixture.Coordinator.GetNextPlayerDivineSenseActivationSequence(),
		static_cast<uint64>(1));
	TestTrue(TEXT("same player binds a distinct next Run"),
		Fixture.Coordinator.TryBeginRun(
			DivineSenseAuthorityRunB,
			Fixture.Player,
			Fixture.Health,
			Fixture.Diagnostic));
	Fdemo_mapPlayerDivineSenseActionReservation SecondRun;
	TestTrue(TEXT("next Run starts at sequence one"),
		Fixture.Coordinator.TryReservePlayerDivineSenseAction(
			Config, SecondRun, Fixture.Diagnostic)
			&& SecondRun.GetActivationSequence() == 1);
	TestNotEqual(TEXT("Run identity separates sequence-one activations"),
		FirstRun.GetActivationId(), SecondRun.GetActivationId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductAuthorityIntentCompositionTest,
	"Shanmen.0_0_10.Product.DivineSenseProductAuthority.IntentComposition",
	ProductAuthorityFlags)

bool Fdemo_mapDivineSenseProductAuthorityIntentCompositionTest::RunTest(
	const FString&)
{
	FDivineSenseAuthorityFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const auto First =
		Fdemo_mapShanmenDivineSenseProductAuthority::PrepareIntent(
			Fixture.Coordinator);
	const auto Second =
		Fdemo_mapShanmenDivineSenseProductAuthority::PrepareIntent(
			Fixture.Coordinator);
	TestTrue(TEXT("both complete product proofs are ready"),
		First.IsReady() && Second.IsReady());
	TestTrue(TEXT("Intent retains the exact Run reservation action"),
		First.Intent.GetAction().GetActivationId()
			== First.Reservation.GetActivationId()
			&& First.Intent.GetRunId()
				== Fixture.Coordinator.GetRunId());
	TestEqual(TEXT("IntentId is authority-derived and replay stable"),
		First.Intent.GetIntentId(),
		Fdemo_mapShanmenDivineSenseProductAuthority::MakeIntentId(
			First.Reservation));
	TestEqual(TEXT("scan ordinal cannot be selected by input"),
		First.Intent.GetScanOrdinal(), 0);
	TestEqual(TEXT("subject budget cannot be selected by input"),
		First.Intent.GetSubjectActorBudget(), 32);
	TestEqual(TEXT("attempts share one canonical config"),
		First.Config.GetConfigId(), Second.Config.GetConfigId());
	TestNotEqual(TEXT("attempts receive different activation identities"),
		First.Reservation.GetActivationId(),
		Second.Reservation.GetActivationId());
	TestNotEqual(TEXT("attempts receive different Intent identities"),
		First.Intent.GetIntentId(), Second.Intent.GetIntentId());
	TestEqual(TEXT("two prepared attempts consume two sequences"),
		Fixture.Coordinator.GetNextPlayerDivineSenseActivationSequence(),
		static_cast<uint64>(3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductAuthorityControllerCompatibilityTest,
	"Shanmen.0_0_10.Product.DivineSenseProductAuthority.ControllerCompatibility",
	ProductAuthorityFlags)

bool Fdemo_mapDivineSenseProductAuthorityControllerCompatibilityTest::RunTest(
	const FString&)
{
	FDivineSenseAuthorityFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const auto Prepared =
		Fdemo_mapShanmenDivineSenseProductAuthority::PrepareIntent(
			Fixture.Coordinator);
	if (!TestTrue(TEXT("authority prepares Controller input"),
		Prepared.IsReady()))
	{
		return false;
	}
	FShanmenActionResourceSnapshot Opening;
	if (!TestTrue(TEXT("opening SpiritEnergy snapshot captures"),
		MakeOpeningSnapshot(
			Fixture.Coordinator.GetPlayerEntityId(),
			100.0f,
			100.0f,
			7,
			Opening)))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	TestTrue(TEXT("P19.6 Controller accepts canonical config"),
		Controller.TryBegin(
			Fixture.Coordinator,
			Opening,
			Prepared.Config,
			Fixture.Diagnostic));
	FNoReadEvidenceProvider Provider;
	const auto Applied = Controller.TrySubmit(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		Prepared.Intent,
		{},
		Provider);
	TestTrue(TEXT("authority Intent crosses the canonical Controller route"),
		Applied.IsAccepted() && !Applied.IsReplay());
	TestEqual(TEXT("empty canonical batch performs no evidence reads"),
		Provider.CallCount, 0);
	TestTrue(TEXT("exact cost and capacity mutate once"),
		Controller.NumCapturedIntents() == 1
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 90.0f
			&& Controller.GetSession().GetRouter().NumProcessedCommands()
				== 1);
	const FGuid ControllerId = Controller.GetControllerId();
	const auto Ended = Controller.TryEnd(
		Fixture.Coordinator, ControllerId);
	TestTrue(TEXT("Controller closes before its Run"),
		Ended.IsSuccess() && Controller.IsEnded());
	TestTrue(TEXT("Run closes after Controller proof"),
		Fixture.Coordinator.TryEndRun(
			DivineSenseAuthorityRunA, Fixture.Diagnostic));
	return true;
}

#endif
