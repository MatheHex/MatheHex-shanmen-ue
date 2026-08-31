#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenWeaponGuardProductRoute.h"

namespace
{
	const EAutomationTestFlags ProductRouteFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ProductRouteRun(
		0xDFA00001, 0xDFA00002, 0xDFA00003, 0xDFA00004);
	const FGuid ProductRouteTimeline(
		0xDFA10001, 0xDFA10002, 0xDFA10003, 0xDFA10004);

	struct FProductRouteFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* Root = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapItemAuthority Items;
		FString Diagnostic;
		bool bReady = false;

		FProductRouteFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			Root = Pawn
				? NewObject<UBoxComponent>(
					Pawn,
					TEXT("WeaponGuardProductRouteRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("WeaponGuardProductRouteHealth"))
				: nullptr;
			if (Pawn && Root)
			{
				Pawn->SetRootComponent(Root);
			}
			bReady = Pawn && Root && Health
				&& Coordinator.TryBeginRun(
					ProductRouteRun,
					Pawn,
					Health,
					Diagnostic);
		}

		~FProductRouteFixture()
		{
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
	Fdemo_mapWeaponGuardProductRouteItemFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductRoute.ItemFences",
	ProductRouteFlags)

bool Fdemo_mapWeaponGuardProductRouteItemFenceTest::RunTest(
	const FString&)
{
	FProductRouteFixture Fixture;
	const auto Empty = Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
		Fixture.Items,
		Fixture.Coordinator,
		ProductRouteTimeline,
		10);
	TestTrue(TEXT("fixture Run is ready"), Fixture.bReady);
	TestEqual(TEXT("empty equipment is rejected before product start"),
		Empty.Status,
		Edemo_mapShanmenWeaponGuardProductRouteStatus::
			ItemAuthorizationRejected);
	TestEqual(TEXT("nested item failure remains visible"),
		Empty.ItemAuthorization.Status,
		Edemo_mapShanmenWeaponGuardItemStatus::WeaponNotEquipped);
	TestFalse(TEXT("rejection has no product proof"),
		Empty.ProductStart.IsReady());
	TestEqual(TEXT("item fence preserves Run sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductRouteExactBindingTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductRoute.ExactBinding",
	ProductRouteFlags)

bool Fdemo_mapWeaponGuardProductRouteExactBindingTest::RunTest(
	const FString&)
{
	FProductRouteFixture Fixture;
	const FGuid WeaponId = Fixture.Equip(Fdemo_mapItemIds::TrainingBlade);
	const int32 Revision = Fixture.Items.GetAuthorityRevision();
	const auto Result = Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
		Fixture.Items,
		Fixture.Coordinator,
		ProductRouteTimeline,
		40);
	TestTrue(TEXT("equipped weapon starts canonical guard"),
		Fixture.bReady && WeaponId.IsValid() && Result.IsReady());
	TestEqual(TEXT("route retains exact authorized item"),
		Result.ItemAuthorization.Authorization.GetSourceItemInstanceId(),
		WeaponId);
	TestEqual(TEXT("reservation retains exact authorized item"),
		Result.ProductStart.Reservation.GetSourceItemInstanceId(),
		WeaponId);
	TestEqual(TEXT("host action retains exact authorized item"),
		Result.ProductStart.Host.GetActionRuntime().GetAction()
			.GetSourceItemInstanceId(),
		WeaponId);
	TestEqual(TEXT("route never mutates item authority"),
		Fixture.Items.GetAuthorityRevision(),
		Revision);
	TestTrue(TEXT("authorization remains current after synchronous start"),
		Fdemo_mapShanmenWeaponGuardItemAdapter::IsCurrentAuthorization(
			Fixture.Items,
			Result.ItemAuthorization.Authorization));
	TestEqual(TEXT("caller sample reaches timing policy"),
		Result.ProductStart.Host.GetTimingPolicy().GetActiveStartTick(),
		static_cast<int64>(40));
	TestEqual(TEXT("one accepted route consumes one sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductRouteTimelineFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductRoute.TimelineFences",
	ProductRouteFlags)

bool Fdemo_mapWeaponGuardProductRouteTimelineFenceTest::RunTest(
	const FString&)
{
	FProductRouteFixture Fixture;
	const FGuid WeaponId = Fixture.Equip(Fdemo_mapItemIds::TrainingBlade);
	const int32 Revision = Fixture.Items.GetAuthorityRevision();
	const auto Missing = Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
		Fixture.Items,
		Fixture.Coordinator,
		FGuid(),
		10);
	const auto Negative = Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
		Fixture.Items,
		Fixture.Coordinator,
		ProductRouteTimeline,
		-1);
	const auto Overflow = Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
		Fixture.Items,
		Fixture.Coordinator,
		ProductRouteTimeline,
		MAX_int64 - 4);
	TestTrue(TEXT("fixture weapon exists"),
		Fixture.bReady && WeaponId.IsValid());
	TestTrue(TEXT("all timeline fences preserve item authorization"),
		Missing.ItemAuthorization.IsAuthorized()
			&& Negative.ItemAuthorization.IsAuthorized()
			&& Overflow.ItemAuthorization.IsAuthorized());
	TestTrue(TEXT("all timeline fences expose product rejection"),
		Missing.Status
				== Edemo_mapShanmenWeaponGuardProductRouteStatus::
					ProductStartRejected
			&& Negative.Status
				== Edemo_mapShanmenWeaponGuardProductRouteStatus::
					ProductStartRejected
			&& Overflow.Status
				== Edemo_mapShanmenWeaponGuardProductRouteStatus::
					ProductStartRejected);
	TestTrue(TEXT("nested timeline classifications remain visible"),
		Missing.ProductStart.Status
				== Edemo_mapShanmenWeaponGuardProductStartStatus::InvalidTimeline
			&& Negative.ProductStart.Status
				== Edemo_mapShanmenWeaponGuardProductStartStatus::InvalidTimeline
			&& Overflow.ProductStart.Status
				== Edemo_mapShanmenWeaponGuardProductStartStatus::InvalidTimeline);
	TestEqual(TEXT("timeline fences preserve Run sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	TestEqual(TEXT("timeline fences do not mutate item authority"),
		Fixture.Items.GetAuthorityRevision(),
		Revision);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductRouteRunFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductRoute.RunFence",
	ProductRouteFlags)

bool Fdemo_mapWeaponGuardProductRouteRunFenceTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Items;
	TArray<FGuid> Affected;
	const Fdemo_mapItemOperationResult Added = Items.AddDefinition(
		Fdemo_mapItemIds::TrainingBlade,
		1,
		&Affected);
	const bool bEquipped = Added.bSuccess && Affected.Num() == 1
		&& Items.Equip(
			Affected[0],
			Fdemo_mapItemIds::WeaponSlot).bSuccess;
	Fdemo_mapCombatRunCoordinator Unready;
	const auto Result = Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
		Items,
		Unready,
		ProductRouteTimeline,
		10);
	TestTrue(TEXT("fixture item is equipped"), bEquipped);
	TestTrue(TEXT("item authorization succeeds without inventing Run state"),
		Result.ItemAuthorization.IsAuthorized());
	TestEqual(TEXT("unready Run is a product rejection"),
		Result.Status,
		Edemo_mapShanmenWeaponGuardProductRouteStatus::ProductStartRejected);
	TestEqual(TEXT("nested reservation rejection remains visible"),
		Result.ProductStart.Status,
		Edemo_mapShanmenWeaponGuardProductStartStatus::ReservationRejected);
	TestEqual(TEXT("unready Run sequence remains one"),
		Unready.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardProductRouteReplacementTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductRoute.Replacement",
	ProductRouteFlags)

bool Fdemo_mapWeaponGuardProductRouteReplacementTest::RunTest(
	const FString&)
{
	FProductRouteFixture Fixture;
	const FGuid FirstId = Fixture.Equip(Fdemo_mapItemIds::TrainingBlade);
	const auto First = Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
		Fixture.Items,
		Fixture.Coordinator,
		ProductRouteTimeline,
		10);
	const FGuid SecondId = Fixture.Equip(Fdemo_mapItemIds::WeaponLevel1);
	const int32 SecondRevision = Fixture.Items.GetAuthorityRevision();
	const auto Second = Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
		Fixture.Items,
		Fixture.Coordinator,
		ProductRouteTimeline,
		20);
	TestTrue(TEXT("both exact equipment states can start independently"),
		Fixture.bReady
			&& FirstId.IsValid()
			&& SecondId.IsValid()
			&& First.IsReady()
			&& Second.IsReady());
	TestFalse(TEXT("replacement invalidates first authorization"),
		Fdemo_mapShanmenWeaponGuardItemAdapter::IsCurrentAuthorization(
			Fixture.Items,
			First.ItemAuthorization.Authorization));
	TestTrue(TEXT("replacement route binds second exact item"),
		Second.ItemAuthorization.Authorization.GetSourceItemInstanceId()
				== SecondId
			&& Second.ProductStart.Reservation.GetSourceItemInstanceId()
				== SecondId);
	TestNotEqual(TEXT("replacement changes authorization identity"),
		First.ItemAuthorization.Authorization.GetAuthorizationId(),
		Second.ItemAuthorization.Authorization.GetAuthorizationId());
	TestNotEqual(TEXT("accepted routes have distinct activation identity"),
		First.ProductStart.Reservation.GetActivationId(),
		Second.ProductStart.Reservation.GetActivationId());
	TestEqual(TEXT("second route remains read-only"),
		Fixture.Items.GetAuthorityRevision(),
		SecondRevision);
	TestEqual(TEXT("two accepted routes consume two sequences"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(3));
	return true;
}

#endif
