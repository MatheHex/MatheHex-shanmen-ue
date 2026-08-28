#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponAdapter.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid OwnerId(0xD3610001, 0, 0, 1);
	const FGuid ScopeId(0xD3610002, 0, 0, 1);
	const FGuid SourceEntityId(0xD3610003, 0, 0, 1);
	const FGuid SwordId(0xD3610004, 0, 0, 1);
	const FGuid ContainerId(0xD3610005, 0, 0, 1);
	const FGuid ReserveRequestId(0xD3610006, 0, 0, 1);
	const FGuid StartRequestId(0xD3610007, 0, 0, 1);
	const FName FlyingSwordDefinitionId(TEXT("Item.Test.FlyingSword.P6.1"));

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P6.1");
		Content.Digest = TEXT("P6.1.ControlledWeaponAuthorityAdapter.v1");
		return Content;
	}

	FShanmenOperationContext MakeContext(
		const FShanmenContentStamp& Content,
		const FGuid& RequestId)
	{
		FShanmenOperationContext Context;
		Context.RunId = ScopeId;
		Context.OwnerId = OwnerId;
		Context.RequestId = RequestId;
		Context.Content = Content;
		return Context;
	}

	struct FControlledWeaponFixture
	{
		FShanmenItemAuthoritySnapshot Snapshot;
		Fdemo_mapShanmenRunCorrelation Correlation;
		Fdemo_mapShanmenControlledWeaponPrepareRequest Request;

		bool Build()
		{
			const FShanmenContentStamp Content = MakeContent();
			FShanmenItemAuthoritySnapshot Initial;
			Initial.AuthorityRevision = 0;
			Initial.Content = Content;

			FShanmenItemDefinition Definition;
			Definition.DefinitionId = FlyingSwordDefinitionId;
			Definition.MaxStack = 1;
			Definition.ItemTags.AddTag(
				FShanmenItemNativeTags::CapabilityDeploy());
			Definition.ItemTags.AddTag(
				FShanmenItemNativeTags::ItemWeaponFlyingSword());
			Initial.Definitions.Add(Definition);

			FShanmenItemContainer Container;
			Container.ContainerId = ContainerId;
			Container.RunId = ScopeId;
			Container.OwnerId = OwnerId;
			Container.ContainerType = TEXT("Container.Test.P6.1.Equipment");
			Container.Slots.Add(SwordId);
			Initial.Containers.Add(Container);

			FShanmenItemInstance Sword;
			Sword.ItemInstanceId = SwordId;
			Sword.DefinitionId = FlyingSwordDefinitionId;
			Sword.RunId = ScopeId;
			Sword.OwnerId = OwnerId;
			Sword.ParentContainerId = ContainerId;
			Sword.SlotIndex = 0;
			Sword.Quantity = 1;
			Sword.Revision = 0;
			Sword.State = EShanmenItemInstanceState::Stored;
			Initial.Items.Add(Sword);

			FShanmenItemRepository Repository;
			if (!Repository.TryLoadSnapshot(Initial))
			{
				return false;
			}
			FShanmenItemReserveRequest Reserve;
			Reserve.Context = MakeContext(Content, ReserveRequestId);
			Reserve.ItemInstanceId = SwordId;
			Reserve.ResourceKind = EShanmenItemResourceKind::DeploymentLock;
			Reserve.Amount = 1;
			Reserve.ExpectedItemRevision = 0;
			Reserve.PurposeId = TEXT("Prepare.Weapon.FlyingSword.P6.1");
			const FShanmenItemTransactionReceipt Reserved =
				Repository.Reserve(Reserve);
			if (!Reserved.IsSuccess())
			{
				return false;
			}
			FShanmenItemRunStartRequest Start;
			Start.Context = MakeContext(Content, StartRequestId);
			Start.ReservationIds.Add(Reserved.ReservationId);
			const FShanmenItemTransactionReceipt Started =
				Repository.StartPreparedRun(Start);
			if (!Started.IsSuccess())
			{
				return false;
			}
			Snapshot = Repository.CaptureSnapshot();

			Correlation.CorrelationId = FGuid(0xD3610010, 0, 0, 1);
			Correlation.OwnerId = OwnerId;
			Correlation.ScopeId = ScopeId;
			Correlation.ActiveRunId = Started.ReservationId;
			Correlation.PreparedRequestId = ReserveRequestId;
			Correlation.PreparedReceiptId = Reserved.ReceiptId;
			Correlation.LifecycleRequestId = Started.RequestId;
			Correlation.LifecycleReceiptId = Started.ReceiptId;
			Correlation.PreparedAuthorityRevision = Reserved.AuthorityRevision;
			Correlation.LifecycleAuthorityRevision = Started.AuthorityRevision;
			Correlation.WeaponItemInstanceId = SwordId;
			Correlation.OrderedPreparedItemInstanceIds.Add(SwordId);
			Correlation.HotbarItemInstanceIds.SetNum(
				Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);

			Request.SourceEntityId = SourceEntityId;
			Request.SourceItemInstanceId = SwordId;
			Request.ActivationSequence = 7;
			Request.Definition.ActionDefinitionId =
				FShanmenControlledWeaponDefinition::
					CanonicalActionDefinitionId();
			Request.Definition.DetectorId =
				TEXT("Detector.ControlledWeapon.P6.1.FlyingSword");
			Request.Definition.FormulaId =
				TEXT("Formula.ControlledWeapon.P6.1.Test");
			Request.Definition.BaseDamage = 10.0f;
			Request.Definition.ControlPowerCoefficient = 0.25f;
			Request.Definition.DamageTags.AddTag(
				FShanmenCombatNativeTags::DamagePhysicalSlash());
			Request.Definition.RequiredTargetTags.AddTag(
				FShanmenCombatNativeTags::TargetLiving());
			Request.ControlPower = 40.0f;
			return Correlation.IsValid() && Request.IsValid();
		}
	};

	template <typename TValue, typename TPredicate>
	TValue* FindMutable(TArray<TValue>& Values, TPredicate Predicate)
	{
		return Values.FindByPredicate(Predicate);
	}

	void TestRejected(
		FAutomationTestBase& Test,
		const TCHAR* What,
		const Fdemo_mapShanmenControlledWeaponPrepareResult& Result,
		const Edemo_mapShanmenControlledWeaponPrepareStatus Expected)
	{
		Test.TestTrue(What,
			Result.Status == Expected
			&& !Result.IsPrepared()
			&& !Result.Execution.IsValid()
			&& !Result.Evidence.IsValid());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponAuthorityGateTest,
	"Shanmen.0_0_10.Product.ControlledWeaponAdapter.AuthorityGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponAuthorityGateTest::RunTest(const FString&)
{
	FControlledWeaponFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P6.1 deployed flying-sword fixture."));
		return false;
	}
	const FShanmenItemAuthoritySnapshot Before = Fixture.Snapshot;
	const Fdemo_mapShanmenRunCorrelation CorrelationBefore =
		Fixture.Correlation;
	const Fdemo_mapShanmenControlledWeaponPrepareResult Result =
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Request);

	TestTrue(TEXT("Exact deployed weapon-slot flying sword enters P6.0"),
		Result.IsPrepared()
		&& Result.Evidence.ItemInstanceId == SwordId
		&& Result.Evidence.ItemDefinitionId == FlyingSwordDefinitionId
		&& Result.Evidence.ActiveRunId == Fixture.Correlation.ActiveRunId
		&& Result.Action.GetSourceItemInstanceId() == SwordId
		&& Result.Action.GetRunId() == Fixture.Correlation.ActiveRunId
		&& Result.Action.GetOwnerId() == OwnerId
		&& Result.Execution.GetState()
			== EShanmenControlledWeaponState::Orbiting);
	TestTrue(TEXT("Authority item tags are frozen into combat source tags"),
		Result.Action.GetSourceTags().HasTagExact(
			FShanmenItemNativeTags::CapabilityDeploy())
		&& Result.Action.GetSourceTags().HasTagExact(
			FShanmenItemNativeTags::ItemWeaponFlyingSword()));
	TestTrue(TEXT("Preparation is read-only across both evidence values"),
		Fixture.Snapshot == Before
		&& Fixture.Correlation == CorrelationBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponFailClosedTest,
	"Shanmen.0_0_10.Product.ControlledWeaponAdapter.FailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponFailClosedTest::RunTest(const FString&)
{
	FControlledWeaponFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P6.1 fail-closed fixture."));
		return false;
	}

	Fdemo_mapShanmenControlledWeaponPrepareRequest WrongItem =
		Fixture.Request;
	WrongItem.SourceItemInstanceId = FGuid(0xD3610090, 0, 0, 1);
	TestRejected(*this, TEXT("A different item cannot borrow the weapon slot"),
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			Fixture.Snapshot, Fixture.Correlation, WrongItem),
		Edemo_mapShanmenControlledWeaponPrepareStatus::SourceItemMismatch);

	FShanmenItemAuthoritySnapshot Stored = Fixture.Snapshot;
	FShanmenItemInstance* StoredItem = FindMutable(
		Stored.Items, [](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == SwordId;
		});
	check(StoredItem);
	StoredItem->State = EShanmenItemInstanceState::Stored;
	TestRejected(*this, TEXT("A stored sword cannot be controlled in the Run"),
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			Stored, Fixture.Correlation, Fixture.Request),
		Edemo_mapShanmenControlledWeaponPrepareStatus::ItemNotDeployed);

	FShanmenItemAuthoritySnapshot GenericWeapon = Fixture.Snapshot;
	FShanmenItemDefinition* GenericDefinition = FindMutable(
		GenericWeapon.Definitions,
		[](const FShanmenItemDefinition& Definition)
		{
			return Definition.DefinitionId == FlyingSwordDefinitionId;
		});
	check(GenericDefinition);
	GenericDefinition->ItemTags.RemoveTag(
		FShanmenItemNativeTags::ItemWeaponFlyingSword());
	TestRejected(*this, TEXT("A generic deployable weapon is not a flying sword"),
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			GenericWeapon, Fixture.Correlation, Fixture.Request),
		Edemo_mapShanmenControlledWeaponPrepareStatus::DefinitionNotFlyingSword);

	FShanmenItemAuthoritySnapshot Uncommitted = Fixture.Snapshot;
	FShanmenItemReservationSnapshot* Pending = FindMutable(
		Uncommitted.Reservations,
		[](const FShanmenItemReservationSnapshot& Reservation)
		{
			return Reservation.ItemInstanceId == SwordId;
		});
	check(Pending);
	Pending->State = EShanmenItemReservationState::Reserved;
	TestRejected(*this, TEXT("An uncommitted DeploymentLock cannot source combat"),
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			Uncommitted, Fixture.Correlation, Fixture.Request),
		Edemo_mapShanmenControlledWeaponPrepareStatus::DeploymentEvidenceInvalid);

	FShanmenItemAuthoritySnapshot Stale = Fixture.Snapshot;
	Stale.AuthorityRevision = Fixture.Correlation.LifecycleAuthorityRevision - 1;
	TestRejected(*this, TEXT("A pre-lifecycle snapshot cannot source combat"),
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			Stale, Fixture.Correlation, Fixture.Request),
		Edemo_mapShanmenControlledWeaponPrepareStatus::SnapshotStale);

	FShanmenItemAuthoritySnapshot RevisionMismatch = Fixture.Snapshot;
	FShanmenItemInstance* UnadvancedItem = FindMutable(
		RevisionMismatch.Items,
		[](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == SwordId;
		});
	const FShanmenItemReservationSnapshot* Deployment =
		RevisionMismatch.Reservations.FindByPredicate(
			[](const FShanmenItemReservationSnapshot& Reservation)
			{
				return Reservation.ItemInstanceId == SwordId;
			});
	check(UnadvancedItem && Deployment);
	UnadvancedItem->Revision = Deployment->ItemRevisionAtReserve;
	TestRejected(*this, TEXT("Deployment must advance the exact item revision"),
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			RevisionMismatch, Fixture.Correlation, Fixture.Request),
		Edemo_mapShanmenControlledWeaponPrepareStatus::DeploymentEvidenceInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponAdapterReplayTest,
	"Shanmen.0_0_10.Product.ControlledWeaponAdapter.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponAdapterReplayTest::RunTest(const FString&)
{
	FControlledWeaponFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P6.1 replay fixture."));
		return false;
	}
	const Fdemo_mapShanmenControlledWeaponPrepareResult First =
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Request);
	const Fdemo_mapShanmenControlledWeaponPrepareResult Replay =
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			Fixture.Snapshot, Fixture.Correlation, Fixture.Request);
	TestTrue(TEXT("Exact evidence and sequence reproduce the activation"),
		First.IsPrepared() && Replay.IsPrepared()
		&& First.Action.GetActivationId() == Replay.Action.GetActivationId()
		&& First.Action.GetContent().Version
			== Replay.Action.GetContent().Version
		&& First.Action.GetContent().Digest
			== Replay.Action.GetContent().Digest
		&& First.Evidence.DeploymentReservationId
			== Replay.Evidence.DeploymentReservationId
		&& First.Evidence.AuthorityRevision
			== Replay.Evidence.AuthorityRevision);

	Fdemo_mapShanmenControlledWeaponPrepareRequest Later = Fixture.Request;
	++Later.ActivationSequence;
	const Fdemo_mapShanmenControlledWeaponPrepareResult Next =
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareFromEvidence(
			Fixture.Snapshot, Fixture.Correlation, Later);
	TestTrue(TEXT("A later activation sequence owns a distinct identity"),
		Next.IsPrepared()
		&& Next.Action.GetActivationId() != First.Action.GetActivationId()
		&& Next.Action.GetSourceItemInstanceId()
			== First.Action.GetSourceItemInstanceId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponFacadeBoundaryTest,
	"Shanmen.0_0_10.Product.ControlledWeaponAdapter.ProductFacadeBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponFacadeBoundaryTest::RunTest(const FString&)
{
	FControlledWeaponFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P6.1 facade fixture."));
		return false;
	}
	if (!GEngine)
	{
		AddError(TEXT("GEngine is unavailable for the P6.1 facade fixture."));
		return false;
	}
	UGameInstance* GameInstance = NewObject<UGameInstance>(
		GEngine, NAME_None, RF_Transient);
	if (!GameInstance)
	{
		AddError(TEXT("Could not allocate the P6.1 product GameInstance."));
		return false;
	}
	GameInstance->AddToRoot();
	GameInstance->Init();
	Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		GameInstance->GetSubsystem<
			Udemo_mapShanmenItemAuthoritySubsystem>();
	Udemo_mapItemSubsystem* Runtime =
		GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	if (!Authority || !Runtime)
	{
		GameInstance->Shutdown();
		GameInstance->RemoveFromRoot();
		AddError(TEXT("Could not resolve the P6.1 product facade inputs."));
		return false;
	}
	TestRejected(*this,
		TEXT("The product facade cannot bypass an unbound authority"),
		Fdemo_mapShanmenControlledWeaponAdapter::PrepareActiveRun(
			*Authority, *Runtime, Fixture.Request),
		Edemo_mapShanmenControlledWeaponPrepareStatus::AuthorityNotReady);
	GameInstance->Shutdown();
	GameInstance->RemoveFromRoot();
	GameInstance->MarkAsGarbage();
	return true;
}

#endif
