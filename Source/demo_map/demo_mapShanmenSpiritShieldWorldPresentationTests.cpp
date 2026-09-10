#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSpiritShieldWorldPresentation.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenDivineSenseProductAuthority.h"
#include "demo_mapShanmenSpiritShieldProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SpiritShieldWorldPresentationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SpiritShieldWorldPresentationRun(
		0x25600001, 0x25600002, 0x25600003, 0x25600004);
	const FGuid SpiritShieldWorldPresentationAttackSource(
		0x25610001, 0x25610002, 0x25610003, 0x25610004);

	struct FSpiritShieldWorldFixture
	{
		UWorld* World = nullptr;
		ACharacter* Character = nullptr;
		UStaticMesh* ShellMesh = nullptr;
		UMaterialInterface* ShellMaterial = nullptr;
		FString Diagnostic;

		FSpiritShieldWorldFixture()
		{
			if (!GEngine)
			{
				Diagnostic = TEXT("Engine was unavailable.");
				return;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				Diagnostic = TEXT("Transient World allocation failed.");
				return;
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
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Character = World->SpawnActor<ACharacter>(
				ACharacter::StaticClass(),
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParameters);
			ShellMesh = LoadObject<UStaticMesh>(
				nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
			ShellMaterial = LoadObject<UMaterialInterface>(
				nullptr,
				TEXT("/Game/LevelPrototyping/Interactable/JumpPad/Assets/Materials/M_SimpleGlow.M_SimpleGlow"));
			if (!Character || !ShellMesh || !ShellMaterial)
			{
				Diagnostic = TEXT(
					"Character or authored shield assets were unavailable.");
			}
		}

		~FSpiritShieldWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		bool IsReady() const
		{
			return World && Character && ShellMesh && ShellMaterial;
		}

		bool Synchronize(
			const Fdemo_mapShanmenSpiritShieldProductSession* Session) const
		{
			return Fdemo_mapShanmenSpiritShieldWorldPresentation::Synchronize(
				Character, Session, ShellMesh, ShellMaterial);
		}
	};

	struct FSpiritShieldPresentationProductFixture
	{
		APawn* Player = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenDivineSenseProductController SpiritEnergy;
		Fdemo_mapShanmenSpiritShieldProductSession Session;
		FString Diagnostic;

		bool Start()
		{
			Player = NewObject<APawn>(GetTransientPackage());
			Health = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player, TEXT("P256SpiritShieldHealth"))
				: nullptr;
			if (!Player || !Health
				|| !Coordinator.TryBeginRun(
					SpiritShieldWorldPresentationRun,
					Player,
					Health,
					Diagnostic)
				|| !Timeline.TryBegin(
					SpiritShieldWorldPresentationRun, Diagnostic))
			{
				return false;
			}

			FShanmenActionResourceAuthority OpeningAuthority;
			FShanmenActionResourceSnapshot OpeningSnapshot;
			Fdemo_mapShanmenDivineSenseProductConfig Config;
			if (!FShanmenActionResourceAuthority::TryCreate(
					Coordinator.GetPlayerEntityId(),
					FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
					100.0f,
					100.0f,
					0,
					OpeningAuthority)
				|| !OpeningAuthority.TryCaptureSnapshot(OpeningSnapshot)
				|| !Fdemo_mapShanmenDivineSenseProductAuthority::
					TryCreateCanonicalConfig(Config)
				|| !SpiritEnergy.TryBegin(
					Coordinator, OpeningSnapshot, Config, Diagnostic))
			{
				return false;
			}

			return Session.TryActivate(
				Coordinator,
				SpiritEnergy,
				CaptureTimeline(),
				[this]()
				{
					return Fdemo_mapShanmenPlayerActionGateResult::
						FromArbitration(
							Coordinator.TryAuthorizePlayerAction(
								Edemo_mapShanmenPlayerActionKind::SpiritShield,
								Fdemo_mapShanmenPlayerActionOccupancySnapshot()));
				}).IsAccepted();
		}

		Fdemo_mapShanmenCombatRunTimelineSample CaptureTimeline()
		{
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			Timeline.TryCapture(Sample);
			return Sample;
		}
	};

	struct FSpiritShieldPresentationImpactIdentity
	{
		FShanmenCombatActionSnapshot Action;
		FShanmenHitCandidate Candidate;
		FGuid ImpactId;
	};

	FSpiritShieldPresentationImpactIdentity MakeImpactIdentity(
		const FGuid& TargetEntityId)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = SpiritShieldWorldPresentationRun;
		Capture.OwnerId = SpiritShieldWorldPresentationAttackSource;
		Capture.SourceEntityId = SpiritShieldWorldPresentationAttackSource;
		Capture.ActionDefinitionId =
			TEXT("Combat.Action.Test.P25_6ShieldPresentation");
		Capture.Content.Version = TEXT("0.0.10.P25.6");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P25.6-SHIELD-PRESENTATION");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			256);

		FSpiritShieldPresentationImpactIdentity Identity;
		check(FShanmenCombatActionSnapshot::TryCapture(
			Capture, Identity.Action));
		Identity.Candidate.ActivationId = Identity.Action.GetActivationId();
		Identity.Candidate.SourceEntityId = Identity.Action.GetSourceEntityId();
		Identity.Candidate.TargetEntityId = TargetEntityId;
		Identity.Candidate.DetectorId =
			TEXT("Detector.Test.P25_6ShieldPresentation");
		Identity.Candidate.DetectorKind = EShanmenHitDetectorKind::Shape;
		Identity.Candidate.HitNormal = FVector::BackwardVector;
		Identity.Candidate.HitOrdinal = 0;
		Identity.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Identity.Action.GetRunId(),
			Identity.Candidate.ActivationId,
			Identity.Candidate.DetectorId,
			Identity.Candidate.TargetEntityId,
			Identity.Candidate.HitOrdinal);
		return Identity;
	}

	FShanmenDefenseSnapshot MakeBaseDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		check(Defense.IsValid());
		return Defense;
	}

	FShanmenImpactRequest MakeDepletingImpactRequest(
		const FSpiritShieldPresentationImpactIdentity& Identity,
		const FShanmenDefenseSnapshot& Defense)
	{
		FShanmenImpactRequest Request;
		Request.ImpactId = Identity.ImpactId;
		Request.Action = Identity.Action;
		Request.Candidate = Identity.Candidate;
		Request.Damage.FormulaId =
			TEXT("Combat.Formula.Test.P25_6ShieldPresentation");
		Request.Damage.RawDamage = 40.0f;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 0;
		Request.Defense = Defense;
		check(Request.IsValid());
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldWorldPresentationGeometryTest,
	"Shanmen.0_0_10.Product.SpiritShieldWorldPresentation.GeometryFailClosed",
	SpiritShieldWorldPresentationFlags)

bool Fdemo_mapSpiritShieldWorldPresentationGeometryTest::RunTest(
	const FString&)
{
	FSpiritShieldWorldFixture World;
	if (!World.IsReady())
	{
		AddError(World.Diagnostic);
		return false;
	}

	TestTrue(TEXT("empty authority installs hidden collisionless cues"),
		World.Synchronize(nullptr)
			&& Fdemo_mapShanmenSpiritShieldWorldPresentation::
				IsGeometryValid(World.Character)
			&& !Fdemo_mapShanmenSpiritShieldWorldPresentation::
				IsVisible(World.Character));
	UStaticMeshComponent* Shell = nullptr;
	UPointLightComponent* CueLight = nullptr;
	TInlineComponentArray<UStaticMeshComponent*> MeshComponents;
	World.Character->GetComponents(MeshComponents);
	for (UStaticMeshComponent* Component : MeshComponents)
	{
		if (Component
			&& Component->GetFName() == TEXT("SpiritShieldShellVisual"))
		{
			Shell = Component;
		}
	}
	TInlineComponentArray<UPointLightComponent*> LightComponents;
	World.Character->GetComponents(LightComponents);
	for (UPointLightComponent* Component : LightComponents)
	{
		if (Component
			&& Component->GetFName() == TEXT("SpiritShieldCueLight"))
		{
			CueLight = Component;
		}
	}
	TestTrue(TEXT("authored components use mesh material and no collision"),
		Shell && CueLight
			&& Shell->GetStaticMesh() == World.ShellMesh
			&& Shell->GetCollisionEnabled()
				== ECollisionEnabled::NoCollision
			&& !Shell->GetGenerateOverlapEvents()
			&& Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0)));
	TestTrue(TEXT("dynamic shell material receives the canonical energy color"),
		Fdemo_mapShanmenSpiritShieldWorldPresentation::
			HasCanonicalMaterialColor(World.Character));
	TestTrue(TEXT("cue light receives the exact linear energy color"),
		Fdemo_mapShanmenSpiritShieldWorldPresentation::
			GetCueColor(World.Character).Equals(
				FLinearColor(
					FLinearColor(0.08f, 0.68f, 1.0f).ToFColor(false)),
				KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldWorldPresentationActivationReleaseTest,
	"Shanmen.0_0_10.Product.SpiritShieldWorldPresentation.ActivationAndRelease",
	SpiritShieldWorldPresentationFlags)

bool Fdemo_mapSpiritShieldWorldPresentationActivationReleaseTest::RunTest(
	const FString&)
{
	FSpiritShieldWorldFixture World;
	FSpiritShieldPresentationProductFixture Product;
	if (!World.IsReady() || !Product.Start())
	{
		AddError(!World.IsReady() ? World.Diagnostic : Product.Diagnostic);
		return false;
	}

	TestTrue(TEXT("accepted active Session reveals both player shield cues"),
		World.Synchronize(&Product.Session)
			&& Fdemo_mapShanmenSpiritShieldWorldPresentation::
				IsVisible(World.Character)
			&& Fdemo_mapShanmenSpiritShieldWorldPresentation::
				GetCueColor(World.Character).Equals(
					FLinearColor(
						FLinearColor(0.08f, 0.68f, 1.0f)
							.ToFColor(false)),
					KINDA_SMALL_NUMBER));
	TestTrue(TEXT("owner release clears the sole Session"),
		Product.Session.TryReleaseOwner(Product.Diagnostic)
			&& Product.Session.IsEmpty());
	TestTrue(TEXT("released Session hides both world cues"),
		World.Synchronize(&Product.Session)
			&& !Fdemo_mapShanmenSpiritShieldWorldPresentation::
				IsVisible(World.Character));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldWorldPresentationDeadlineTest,
	"Shanmen.0_0_10.Product.SpiritShieldWorldPresentation.DeadlineClosure",
	SpiritShieldWorldPresentationFlags)

bool Fdemo_mapSpiritShieldWorldPresentationDeadlineTest::RunTest(
	const FString&)
{
	FSpiritShieldWorldFixture World;
	FSpiritShieldPresentationProductFixture Product;
	if (!World.IsReady() || !Product.Start())
	{
		AddError(!World.IsReady() ? World.Diagnostic : Product.Diagnostic);
		return false;
	}
	if (!World.Synchronize(&Product.Session))
	{
		AddError(TEXT("Active Session failed initial world synchronization."));
		return false;
	}

	int64 AdvancedTicks = 0;
	TestTrue(TEXT("fixed Run timeline reaches canonical shield deadline"),
		Product.Timeline.TryAdvance(
			3.0, AdvancedTicks, Product.Diagnostic)
			&& AdvancedTicks == 90);
	const auto Closed =
		Product.Session.ObserveTimeline(Product.CaptureTimeline());
	TestTrue(TEXT("deadline observation closes authoritative shield state"),
		Closed.IsSuccess()
			&& Closed.Status
				== Edemo_mapShanmenSpiritShieldProductTimelineStatus::Closed
			&& Product.Session.IsClosed());
	TestTrue(TEXT("deadline-closed Session hides both world cues"),
		World.Synchronize(&Product.Session)
			&& !Fdemo_mapShanmenSpiritShieldWorldPresentation::
				IsVisible(World.Character));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldWorldPresentationDepletionTest,
	"Shanmen.0_0_10.Product.SpiritShieldWorldPresentation.CapacityDepletion",
	SpiritShieldWorldPresentationFlags)

bool Fdemo_mapSpiritShieldWorldPresentationDepletionTest::RunTest(
	const FString&)
{
	FSpiritShieldWorldFixture World;
	FSpiritShieldPresentationProductFixture Product;
	if (!World.IsReady() || !Product.Start())
	{
		AddError(!World.IsReady() ? World.Diagnostic : Product.Diagnostic);
		return false;
	}
	if (!World.Synchronize(&Product.Session))
	{
		AddError(TEXT("Active Session failed initial world synchronization."));
		return false;
	}

	const FSpiritShieldPresentationImpactIdentity Identity =
		MakeImpactIdentity(Product.Coordinator.GetPlayerEntityId());
	const auto Defense = Product.Session.TryComposeImpactDefense(
		Identity.ImpactId,
		Product.CaptureTimeline(),
		MakeBaseDefense());
	const FShanmenImpactRequest Request = MakeDepletingImpactRequest(
		Identity, Defense.Defense);
	const FShanmenImpactResult Resolution =
		FShanmenDefenseResolver::Resolve(Request);
	const auto Committed = Product.Session.CommitImpact(
		Defense, Request, Resolution, []() { return true; });
	TestTrue(TEXT("depleting impact consumes exact remaining shield capacity"),
		Committed.DidConsumeCapacity()
			&& FMath::IsNearlyEqual(Resolution.PreventedDamage, 30.0f)
			&& FMath::IsNearlyEqual(Product.Session.GetAvailableCapacity(), 0.0f));
	TestTrue(TEXT("zero-capacity active Session hides both world cues"),
		World.Synchronize(&Product.Session)
			&& !Fdemo_mapShanmenSpiritShieldWorldPresentation::
				IsVisible(World.Character));
	return true;
}

#endif
