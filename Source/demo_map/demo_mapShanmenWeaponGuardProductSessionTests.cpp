#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenWeaponGuardProductSession.h"

namespace
{
	const EAutomationTestFlags ProductSessionFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ProductSessionRun(
		0xDFA20001, 0xDFA20002, 0xDFA20003, 0xDFA20004);
	const FGuid ProductSessionTimeline(
		0xDFA30001, 0xDFA30002, 0xDFA30003, 0xDFA30004);

	struct FProductSessionFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* Root = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapItemAuthority Items;
		Fdemo_mapShanmenWeaponGuardProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FProductSessionFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			Root = Pawn
				? NewObject<UBoxComponent>(
					Pawn,
					TEXT("WeaponGuardProductSessionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("WeaponGuardProductSessionHealth"))
				: nullptr;
			if (Pawn && Root)
			{
				Pawn->SetRootComponent(Root);
			}
			bReady = Pawn && Root && Health
				&& Coordinator.TryBeginRun(
					ProductSessionRun,
					Pawn,
					Health,
					Diagnostic);
		}

		~FProductSessionFixture()
		{
			Session.TryInterruptAndReset();
			Coordinator.Reset();
		}

		FGuid Equip(FName DefinitionId)
		{
			TArray<FGuid> Affected;
			const Fdemo_mapItemOperationResult Added =
				Items.AddDefinition(DefinitionId, 1, &Affected);
			if (!Added.bSuccess || Affected.Num() != 1)
			{
				return FGuid();
			}
			return Items.Equip(
					Affected[0],
					Fdemo_mapItemIds::WeaponSlot).bSuccess
				? Affected[0]
				: FGuid();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductSessionStartFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductSession.StartFences",
	ProductSessionFlags)

bool Fdemo_mapWeaponGuardProductSessionStartFenceTest::RunTest(
	const FString&)
{
	FProductSessionFixture Fixture;
	const auto MissingAuthority = Fixture.Session.TryStart(
		nullptr,
		Fixture.Coordinator,
		ProductSessionTimeline,
		10);
	const auto EmptyWeapon = Fixture.Session.TryStart(
		&Fixture.Items,
		Fixture.Coordinator,
		ProductSessionTimeline,
		10);
	TestTrue(TEXT("fixture Run is ready"), Fixture.bReady);
	TestTrue(TEXT("missing item authority is a valid rejection"),
		MissingAuthority.IsValid()
			&& MissingAuthority.Error
				== Edemo_mapShanmenWeaponGuardSessionStartError::
					ItemAuthorityUnavailable);
	TestTrue(TEXT("empty WeaponSlot preserves route rejection"),
		EmptyWeapon.IsValid()
			&& EmptyWeapon.Error
				== Edemo_mapShanmenWeaponGuardSessionStartError::
					RouteRejected
			&& EmptyWeapon.Route.Status
				== Edemo_mapShanmenWeaponGuardProductRouteStatus::
					ItemAuthorizationRejected);
	TestTrue(TEXT("all start fences preserve valid empty Session"),
		Fixture.Session.IsEmpty());
	TestEqual(TEXT("start fences do not consume Run sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductSessionOwnershipTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductSession.Ownership",
	ProductSessionFlags)

bool Fdemo_mapWeaponGuardProductSessionOwnershipTest::RunTest(
	const FString&)
{
	FProductSessionFixture Fixture;
	const FGuid WeaponId = Fixture.Equip(Fdemo_mapItemIds::TrainingBlade);
	const auto Started = Fixture.Session.TryStart(
		&Fixture.Items,
		Fixture.Coordinator,
		ProductSessionTimeline,
		40);
	const Fdemo_mapShanmenWeaponGuardProductHost* Host =
		Fixture.Session.GetActiveHost();
	TestTrue(TEXT("equipped weapon starts one Session Host"),
		Fixture.bReady
			&& WeaponId.IsValid()
			&& Started.IsStarted()
			&& Fixture.Session.HasActive());
	TestTrue(TEXT("Session retains exact item and Host identity"),
		Host
			&& Host->IsActive()
			&& Host->GetHostId() == Started.HostId
			&& Started.SourceItemInstanceId == WeaponId);
	TestTrue(TEXT("equipped authorization remains current"),
		Fixture.Session.IsCurrentAuthorization(Fixture.Items));
	TestEqual(TEXT("one Session start consumes one Run sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductSessionActiveFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductSession.ActiveFence",
	ProductSessionFlags)

bool Fdemo_mapWeaponGuardProductSessionActiveFenceTest::RunTest(
	const FString&)
{
	FProductSessionFixture Fixture;
	const FGuid WeaponId = Fixture.Equip(Fdemo_mapItemIds::TrainingBlade);
	const auto First = Fixture.Session.TryStart(
		&Fixture.Items,
		Fixture.Coordinator,
		ProductSessionTimeline,
		10);
	const auto Duplicate = Fixture.Session.TryStart(
		&Fixture.Items,
		Fixture.Coordinator,
		ProductSessionTimeline,
		20);
	TestTrue(TEXT("fixture acquires initial Host"),
		Fixture.bReady && WeaponId.IsValid() && First.IsStarted());
	TestTrue(TEXT("duplicate start reports existing sole Host"),
		Duplicate.IsAlreadyActive()
			&& Duplicate.HostId == First.HostId
			&& Duplicate.SourceItemInstanceId == WeaponId);
	TestTrue(TEXT("duplicate start leaves Session active"),
		Fixture.Session.HasActive());
	TestEqual(TEXT("duplicate start does not consume Run sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductSessionReleaseTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductSession.Release",
	ProductSessionFlags)

bool Fdemo_mapWeaponGuardProductSessionReleaseTest::RunTest(
	const FString&)
{
	FProductSessionFixture Fixture;
	const FGuid WeaponId = Fixture.Equip(Fdemo_mapItemIds::TrainingBlade);
	const auto Started = Fixture.Session.TryStart(
		&Fixture.Items,
		Fixture.Coordinator,
		ProductSessionTimeline,
		10);
	const auto Released = Fixture.Session.TryRelease();
	const auto Repeated = Fixture.Session.TryRelease();
	TestTrue(TEXT("fixture starts before release"),
		Fixture.bReady && WeaponId.IsValid() && Started.IsStarted());
	TestTrue(TEXT("release proves Recovery then Completed"),
		Released.IsSuccess()
			&& Released.Status
				== Edemo_mapShanmenWeaponGuardSessionTransitionStatus::
					Completed
			&& Released.Recovery.Status
				== Edemo_mapShanmenWeaponGuardHostTransitionStatus::
					EnteredRecovery
			&& Released.Terminal.Status
				== Edemo_mapShanmenWeaponGuardHostTransitionStatus::
					Completed);
	TestTrue(TEXT("completed release retires Session ownership"),
		Fixture.Session.IsEmpty()
			&& Fixture.Session.GetActiveHost() == nullptr);
	TestTrue(TEXT("repeated release is an accepted no-op"),
		Repeated.IsSuccess() && Repeated.IsNoOp());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductSessionInterruptTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductSession.Interrupt",
	ProductSessionFlags)

bool Fdemo_mapWeaponGuardProductSessionInterruptTest::RunTest(
	const FString&)
{
	FProductSessionFixture Fixture;
	const FGuid WeaponId = Fixture.Equip(Fdemo_mapItemIds::TrainingBlade);
	const auto Started = Fixture.Session.TryStart(
		&Fixture.Items,
		Fixture.Coordinator,
		ProductSessionTimeline,
		10);
	const auto Interrupted = Fixture.Session.TryInterruptAndReset();
	const auto Repeated = Fixture.Session.TryInterruptAndReset();
	TestTrue(TEXT("fixture starts before Run teardown"),
		Fixture.bReady && WeaponId.IsValid() && Started.IsStarted());
	TestTrue(TEXT("Run teardown proves Host interruption"),
		Interrupted.IsSuccess()
			&& Interrupted.Status
				== Edemo_mapShanmenWeaponGuardSessionTransitionStatus::
					Interrupted
			&& Interrupted.Terminal.Status
				== Edemo_mapShanmenWeaponGuardHostTransitionStatus::
					Interrupted);
	TestTrue(TEXT("interruption clears sole ownership"),
		Fixture.Session.IsEmpty());
	TestTrue(TEXT("repeated teardown is an accepted no-op"),
		Repeated.IsSuccess() && Repeated.IsNoOp());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductSessionStaleItemTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductSession.StaleItem",
	ProductSessionFlags)

bool Fdemo_mapWeaponGuardProductSessionStaleItemTest::RunTest(
	const FString&)
{
	FProductSessionFixture Fixture;
	const FGuid FirstId = Fixture.Equip(Fdemo_mapItemIds::TrainingBlade);
	const auto Started = Fixture.Session.TryStart(
		&Fixture.Items,
		Fixture.Coordinator,
		ProductSessionTimeline,
		10);
	const FGuid ReplacementId =
		Fixture.Equip(Fdemo_mapItemIds::WeaponLevel1);
	TestTrue(TEXT("fixture starts with exact first item"),
		Fixture.bReady
			&& FirstId.IsValid()
			&& ReplacementId.IsValid()
			&& Started.IsStarted());
	TestFalse(TEXT("replacement makes captured authorization stale"),
		Fixture.Session.IsCurrentAuthorization(Fixture.Items));
	TestTrue(TEXT("item mutation does not orphan or rewrite Host"),
		Fixture.Session.HasActive()
			&& Fixture.Session.GetActiveRoute()
			&& Fixture.Session.GetActiveRoute()->ItemAuthorization
				.Authorization.GetSourceItemInstanceId() == FirstId);
	const auto Released = Fixture.Session.TryRelease();
	TestTrue(TEXT("stale item identity still permits orderly cleanup"),
		Released.IsSuccess() && Fixture.Session.IsEmpty());
	return true;
}

#endif
