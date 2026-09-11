#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenArmorResistanceItemAdapter.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid OwnerId(0xD3620001, 0, 0, 1);
	const FGuid ScopeId(0xD3620002, 0, 0, 1);
	const FGuid TargetEntityId(0xD3620003, 0, 0, 1);
	const FGuid ArmorId(0xD3620004, 0, 0, 1);
	const FGuid ContainerId(0xD3620005, 0, 0, 1);
	const FGuid ReserveRequestId(0xD3620006, 0, 0, 1);
	const FGuid StartRequestId(0xD3620007, 0, 0, 1);

	FShanmenContentStamp MakeContent()
	{
		return Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp();
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

	FShanmenDefenseSnapshot MakeBaseDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		FShanmenDefenseLayer& Shield = Defense.Layers.AddDefaulted_GetRef();
		Shield.LayerId = FGuid(0xD3620008, 0, 0, 1);
		Shield.RuleId = TEXT("Test.P26.1.ExistingShield");
		Shield.Operation = EShanmenDefenseOperation::AbsorbPoints;
		Shield.Order = FShanmenDefenseOrder::Shield;
		Shield.Magnitude = 7.0f;
		Shield.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseShield());
		Shield.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		return Defense;
	}

	void ConfigureAuthorityDefinition(
		FShanmenItemDefinition& OutDefinition,
		const Fdemo_mapItemDefinition& ProductDefinition)
	{
		OutDefinition = FShanmenItemDefinition();
		OutDefinition.DefinitionId = ProductDefinition.DefinitionId;
		OutDefinition.MaxStack = ProductDefinition.MaxStackSize;
		OutDefinition.MaxDurability = ProductDefinition.MaxDurability;
		OutDefinition.MaxCharges = ProductDefinition.MaxCharges;
		OutDefinition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityDeploy());
		if (OutDefinition.MaxDurability > 0)
		{
			OutDefinition.ItemTags.AddTag(
				FShanmenItemNativeTags::CapabilityDurability());
		}
		if (OutDefinition.MaxCharges > 0)
		{
			OutDefinition.ItemTags.AddTag(
				FShanmenItemNativeTags::CapabilityCharges());
		}
	}

	bool SameDefense(
		const FShanmenDefenseSnapshot& Left,
		const FShanmenDefenseSnapshot& Right)
	{
		if (Left.TargetTags != Right.TargetTags
			|| Left.Layers.Num() != Right.Layers.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Layers.Num(); ++Index)
		{
			const FShanmenDefenseLayer& A = Left.Layers[Index];
			const FShanmenDefenseLayer& B = Right.Layers[Index];
			if (A.LayerId != B.LayerId || A.RuleId != B.RuleId
				|| A.SourceInstanceId != B.SourceInstanceId
				|| A.Operation != B.Operation || A.Order != B.Order
				|| A.Magnitude != B.Magnitude
				|| A.bRequiresCommitOnTrigger != B.bRequiresCommitOnTrigger
				|| A.LayerTags != B.LayerTags
				|| A.RequiredDamageTags != B.RequiredDamageTags
				|| A.RequiredTargetTags != B.RequiredTargetTags)
			{
				return false;
			}
		}
		return true;
	}

	struct FArmorAuthorityFixture
	{
		FShanmenItemAuthoritySnapshot Snapshot;
		Fdemo_mapShanmenRunCorrelation Correlation;

		bool Build()
		{
			const Fdemo_mapItemDefinition* ProductDefinition =
				Fdemo_mapItemDefinitions::Find(
					Fdemo_mapItemIds::TrainingVest);
			if (!ProductDefinition)
			{
				return false;
			}
			const FShanmenContentStamp Content = MakeContent();
			FShanmenItemAuthoritySnapshot Initial;
			Initial.AuthorityRevision = 0;
			Initial.Content = Content;

			FShanmenItemDefinition AuthorityDefinition;
			ConfigureAuthorityDefinition(
				AuthorityDefinition, *ProductDefinition);
			Initial.Definitions.Add(AuthorityDefinition);

			FShanmenItemContainer Container;
			Container.ContainerId = ContainerId;
			Container.RunId = ScopeId;
			Container.OwnerId = OwnerId;
			Container.ContainerType =
				TEXT("Container.Test.P26.1.Equipment");
			Container.Slots.Add(ArmorId);
			Initial.Containers.Add(Container);

			FShanmenItemInstance Armor;
			Armor.ItemInstanceId = ArmorId;
			Armor.DefinitionId = ProductDefinition->DefinitionId;
			Armor.RunId = ScopeId;
			Armor.OwnerId = OwnerId;
			Armor.ParentContainerId = ContainerId;
			Armor.SlotIndex = 0;
			Armor.Quantity = 1;
			Armor.Durability = ProductDefinition->MaxDurability;
			Armor.Charges = ProductDefinition->MaxCharges;
			Armor.Revision = 0;
			Armor.State = EShanmenItemInstanceState::Stored;
			Initial.Items.Add(Armor);

			FShanmenItemRepository Repository;
			if (!Repository.TryLoadSnapshot(Initial))
			{
				return false;
			}
			FShanmenItemReserveRequest Reserve;
			Reserve.Context = MakeContext(Content, ReserveRequestId);
			Reserve.ItemInstanceId = ArmorId;
			Reserve.ResourceKind =
				EShanmenItemResourceKind::DeploymentLock;
			Reserve.Amount = 1;
			Reserve.ExpectedItemRevision = 0;
			Reserve.PurposeId = TEXT("Prepare.Armor.P26.1");
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

			Correlation.CorrelationId = FGuid(0xD3620010, 0, 0, 1);
			Correlation.OwnerId = OwnerId;
			Correlation.ScopeId = ScopeId;
			Correlation.ActiveRunId = Started.ReservationId;
			Correlation.PreparedRequestId = ReserveRequestId;
			Correlation.PreparedReceiptId = Reserved.ReceiptId;
			Correlation.LifecycleRequestId = Started.RequestId;
			Correlation.LifecycleReceiptId = Started.ReceiptId;
			Correlation.PreparedAuthorityRevision =
				Reserved.AuthorityRevision;
			Correlation.LifecycleAuthorityRevision =
				Started.AuthorityRevision;
			Correlation.ArmorItemInstanceId = ArmorId;
			Correlation.OrderedPreparedItemInstanceIds.Add(ArmorId);
			Correlation.HotbarItemInstanceIds.SetNum(
				Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
			return Correlation.IsValid();
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
		const Fdemo_mapShanmenArmorResistanceItemResult& Result,
		Edemo_mapShanmenArmorResistanceItemStatus Expected)
	{
		Test.TestTrue(What,
			Result.Status == Expected
			&& !Result.IsSuccess()
			&& !Result.HasArmorEvidence()
			&& !Result.HasProjection());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapArmorResistanceItemAuthoritySelectionTest,
	"Shanmen.0_0_10.Product.ArmorResistanceItemAdapter.AuthoritySelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapArmorResistanceItemAuthoritySelectionTest::RunTest(
	const FString&)
{
	FArmorAuthorityFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P26.1 deployed armor fixture."));
		return false;
	}
	const FShanmenItemAuthoritySnapshot Before = Fixture.Snapshot;
	const Fdemo_mapShanmenRunCorrelation CorrelationBefore =
		Fixture.Correlation;
	const FShanmenDefenseSnapshot Base = MakeBaseDefense();
	const Fdemo_mapShanmenArmorResistanceItemResult Result =
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Fixture.Snapshot,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base);

	TestTrue(TEXT("Exact active-Run ArmorSlot evidence is accepted"),
		Result.IsSuccess()
		&& Result.Status
			== Edemo_mapShanmenArmorResistanceItemStatus::NotApplicable
		&& Result.HasArmorEvidence()
		&& Result.Evidence.ArmorItemInstanceId == ArmorId
		&& Result.Evidence.ArmorDefinitionId
			== Fdemo_mapItemIds::TrainingVest
		&& Result.Evidence.ActiveRunId == Fixture.Correlation.ActiveRunId);
	TestTrue(TEXT("Unconfigured catalog armor remains an exact no-op"),
		!Result.HasProjection()
		&& SameDefense(Result.Projection.Defense, Base));
	TestTrue(TEXT("Authority selection and projection are read-only"),
		Fixture.Snapshot == Before
		&& Fixture.Correlation == CorrelationBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapArmorResistanceItemNoArmorReplayTest,
	"Shanmen.0_0_10.Product.ArmorResistanceItemAdapter.NoArmorReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapArmorResistanceItemNoArmorReplayTest::RunTest(const FString&)
{
	FArmorAuthorityFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P26.1 no-armor fixture."));
		return false;
	}
	Fixture.Correlation.ArmorItemInstanceId.Invalidate();
	const FShanmenDefenseSnapshot Base = MakeBaseDefense();
	const Fdemo_mapShanmenArmorResistanceItemResult First =
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Fixture.Snapshot,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base);
	const Fdemo_mapShanmenArmorResistanceItemResult Replay =
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Fixture.Snapshot,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base);
	TestTrue(TEXT("An empty ArmorSlot is a deterministic successful no-op"),
		First.IsSuccess() && Replay.IsSuccess()
		&& First.Status
			== Edemo_mapShanmenArmorResistanceItemStatus::NoArmorEquipped
		&& !First.HasArmorEvidence()
		&& SameDefense(First.Projection.Defense, Base)
		&& SameDefense(Replay.Projection.Defense, Base));

	FShanmenDefenseSnapshot InvalidDefense;
	TestRejected(*this,
		TEXT("An empty slot cannot conceal an invalid target defense"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Fixture.Snapshot,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			InvalidDefense),
		Edemo_mapShanmenArmorResistanceItemStatus::InputInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapArmorResistanceItemIdentityFencesTest,
	"Shanmen.0_0_10.Product.ArmorResistanceItemAdapter.IdentityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapArmorResistanceItemIdentityFencesTest::RunTest(const FString&)
{
	FArmorAuthorityFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P26.1 identity fixture."));
		return false;
	}
	const FShanmenDefenseSnapshot Base = MakeBaseDefense();
	TestRejected(*this, TEXT("Armor from another active Run is rejected"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Fixture.Snapshot,
			Fixture.Correlation,
			FGuid(0xD3620011, 0, 0, 1),
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::RunMismatch);

	FShanmenItemAuthoritySnapshot Missing = Fixture.Snapshot;
	Missing.Items.Reset();
	TestRejected(*this, TEXT("A missing prepared armor item is rejected"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Missing,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::ItemNotFound);

	FShanmenItemAuthoritySnapshot Stored = Fixture.Snapshot;
	FShanmenItemInstance* StoredItem = FindMutable(
		Stored.Items, [](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == ArmorId;
		});
	check(StoredItem);
	StoredItem->State = EShanmenItemInstanceState::Stored;
	TestRejected(*this, TEXT("A stored armor item cannot project Run defense"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Stored,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::ItemNotDeployed);

	FShanmenItemAuthoritySnapshot Unknown = Fixture.Snapshot;
	FShanmenItemInstance* UnknownItem = FindMutable(
		Unknown.Items, [](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == ArmorId;
		});
	check(UnknownItem);
	UnknownItem->DefinitionId = TEXT("Item.Unknown.P26.1");
	TestRejected(*this, TEXT("Unknown catalog identity is rejected"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Unknown,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::DefinitionUnavailable);

	const Fdemo_mapItemDefinition* WeaponDefinition =
		Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::TrainingBlade);
	if (!TestNotNull(TEXT("Training weapon fixture definition exists"),
		WeaponDefinition))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot WrongSlot = Fixture.Snapshot;
	FShanmenItemInstance* WrongSlotItem = FindMutable(
		WrongSlot.Items, [](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == ArmorId;
		});
	FShanmenItemDefinition* WrongSlotDefinition = FindMutable(
		WrongSlot.Definitions, [](const FShanmenItemDefinition& Definition)
		{
			return Definition.DefinitionId == Fdemo_mapItemIds::TrainingVest;
		});
	check(WrongSlotItem && WrongSlotDefinition);
	WrongSlotItem->DefinitionId = WeaponDefinition->DefinitionId;
	ConfigureAuthorityDefinition(*WrongSlotDefinition, *WeaponDefinition);
	TestRejected(*this,
		TEXT("A weapon definition cannot borrow the ArmorSlot identity"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			WrongSlot,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::DefinitionMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapArmorResistanceItemEvidenceFencesTest,
	"Shanmen.0_0_10.Product.ArmorResistanceItemAdapter.EvidenceFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapArmorResistanceItemEvidenceFencesTest::RunTest(const FString&)
{
	FArmorAuthorityFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P26.1 evidence fixture."));
		return false;
	}
	const FShanmenDefenseSnapshot Base = MakeBaseDefense();

	FShanmenItemAuthoritySnapshot Stale = Fixture.Snapshot;
	Stale.AuthorityRevision =
		Fixture.Correlation.LifecycleAuthorityRevision - 1;
	TestRejected(*this, TEXT("Pre-lifecycle item evidence is rejected"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Stale,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::SnapshotStale);

	FShanmenItemAuthoritySnapshot WrongContent = Fixture.Snapshot;
	WrongContent.Content.Digest += TEXT(".Drift");
	TestRejected(*this, TEXT("A different product content stamp is rejected"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			WrongContent,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::SnapshotStale);

	FShanmenItemAuthoritySnapshot Pending = Fixture.Snapshot;
	FShanmenItemReservationSnapshot* PendingDeployment = FindMutable(
		Pending.Reservations,
		[](const FShanmenItemReservationSnapshot& Reservation)
		{
			return Reservation.ItemInstanceId == ArmorId;
		});
	check(PendingDeployment);
	PendingDeployment->State = EShanmenItemReservationState::Reserved;
	TestRejected(*this,
		TEXT("A pending DeploymentLock cannot authorize passive defense"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Pending,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::DeploymentEvidenceInvalid);

	FShanmenItemAuthoritySnapshot Unadvanced = Fixture.Snapshot;
	FShanmenItemInstance* UnadvancedItem = FindMutable(
		Unadvanced.Items, [](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == ArmorId;
		});
	const FShanmenItemReservationSnapshot* Deployment =
		Unadvanced.Reservations.FindByPredicate(
			[](const FShanmenItemReservationSnapshot& Reservation)
			{
				return Reservation.ItemInstanceId == ArmorId;
			});
	check(UnadvancedItem && Deployment);
	UnadvancedItem->Revision = Deployment->ItemRevisionAtReserve;
	TestRejected(*this,
		TEXT("Deployment must advance the exact armor item revision"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Unadvanced,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::DeploymentEvidenceInvalid);

	FShanmenItemAuthoritySnapshot Drifted = Fixture.Snapshot;
	FShanmenItemDefinition* DriftedDefinition = FindMutable(
		Drifted.Definitions,
		[](const FShanmenItemDefinition& Definition)
		{
			return Definition.DefinitionId == Fdemo_mapItemIds::TrainingVest;
		});
	check(DriftedDefinition);
	++DriftedDefinition->MaxDurability;
	TestRejected(*this,
		TEXT("Authority and canonical armor resource shapes cannot drift"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
			Drifted,
			Fixture.Correlation,
			Fixture.Correlation.ActiveRunId,
			TargetEntityId,
			Base),
		Edemo_mapShanmenArmorResistanceItemStatus::DefinitionMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapArmorResistanceItemFacadeBoundaryTest,
	"Shanmen.0_0_10.Product.ArmorResistanceItemAdapter.ProductFacadeBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapArmorResistanceItemFacadeBoundaryTest::RunTest(const FString&)
{
	if (!GEngine)
	{
		AddError(TEXT("GEngine is unavailable for the P26.1 facade fixture."));
		return false;
	}
	UGameInstance* GameInstance = NewObject<UGameInstance>(
		GEngine, NAME_None, RF_Transient);
	if (!GameInstance)
	{
		AddError(TEXT("Could not allocate the P26.1 product GameInstance."));
		return false;
	}
	GameInstance->AddToRoot();
	GameInstance->Init();
	Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		GameInstance->GetSubsystem<
			Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!Authority)
	{
		GameInstance->Shutdown();
		GameInstance->RemoveFromRoot();
		AddError(TEXT("Could not resolve the P26.1 item authority."));
		return false;
	}
	TestRejected(*this,
		TEXT("Product facade cannot bypass an unbound authority"),
		Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectActiveRun(
			*Authority,
			ScopeId,
			TargetEntityId,
			MakeBaseDefense()),
		Edemo_mapShanmenArmorResistanceItemStatus::AuthorityNotReady);
	GameInstance->Shutdown();
	GameInstance->RemoveFromRoot();
	GameInstance->MarkAsGarbage();
	return true;
}

#endif
