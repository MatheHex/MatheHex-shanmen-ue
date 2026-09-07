#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenMeridianShockTreatmentAdapter.h"

#include "Misc/AutomationTest.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "ShanmenVitalityAuthority.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	constexpr EAutomationTestFlags TreatmentFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid OwnerId(0xC1600001, 0, 0, 1);
	const FGuid ScopeId(0xC1600002, 0, 0, 1);
	const FGuid TreatmentItemId(0xC1600003, 0, 0, 1);
	const FGuid GenericPillId(0xC1600004, 0, 0, 1);
	const FGuid ContainerId(0xC1600005, 0, 0, 1);
	const FGuid TreatmentReserveRequestId(0xC1600006, 0, 0, 1);
	const FGuid GenericReserveRequestId(0xC1600007, 0, 0, 1);
	const FGuid StartRequestId(0xC1600008, 0, 0, 1);
	const FGuid TargetEntityId(0xC1600009, 0, 0, 1);
	const FGuid ImpactId(0xC160000A, 0, 0, 1);
	const FGuid ResolutionId(0xC160000B, 0, 0, 1);

	FShanmenContentStamp MakeContent()
	{
		return Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp();
	}

	FShanmenOperationContext MakeContext(const FGuid& RequestId)
	{
		FShanmenOperationContext Context;
		Context.RunId = ScopeId;
		Context.OwnerId = OwnerId;
		Context.RequestId = RequestId;
		Context.Content = MakeContent();
		return Context;
	}

	FShanmenVitalityCommitReceipt MakeCommittedReceipt()
	{
		FShanmenVitalityAuthority Authority;
		check(FShanmenVitalityAuthority::TryCreate(
			TargetEntityId,
			100.0f,
			100.0f,
			0,
			Authority));
		FShanmenVitalityCommitCommand Command;
		check(FShanmenVitalityCommitCommand::TryRestoreFromDurableIntent(
			ImpactId,
			ResolutionId,
			TargetEntityId,
			0,
			100.0f,
			100.0f,
			10.0f,
			0.0f,
			10.0f,
			EShanmenDefenseOutcome::Applied,
			Command));
		const FShanmenVitalityCommitResult Result = Authority.Commit(Command);
		check(Result.Status == EShanmenVitalityCommitStatus::Committed);
		return Result.Receipt;
	}

	FShanmenItemDefinition MakeQuantityDefinition(const FName DefinitionId)
	{
		const Fdemo_mapItemDefinition* Product =
			Fdemo_mapItemDefinitions::Find(DefinitionId);
		check(Product);
		FShanmenItemDefinition Definition;
		Definition.DefinitionId = DefinitionId;
		Definition.MaxStack = Product->MaxStackSize;
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityConsumeQuantity());
		return Definition;
	}

	struct FTreatmentFixture
	{
		FShanmenItemRepository Repository;
		FShanmenItemAuthoritySnapshot Snapshot;
		Fdemo_mapShanmenRunCorrelation Correlation;
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Udemo_mapShanmenCombatConditionComponent* Conditions = nullptr;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
		Fdemo_mapShanmenCombatConditionStatusSnapshot ActiveStatus;

		bool Build()
		{
			FShanmenItemAuthoritySnapshot Initial;
			Initial.Content = MakeContent();
			Initial.Definitions.Add(MakeQuantityDefinition(
				Fdemo_mapItemIds::MeridianStabilizingPillLevel1));
			Initial.Definitions.Add(MakeQuantityDefinition(
				Fdemo_mapItemIds::HealingPillLevel1));

			FShanmenItemContainer Container;
			Container.ContainerId = ContainerId;
			Container.RunId = ScopeId;
			Container.OwnerId = OwnerId;
			Container.ContainerType =
				TEXT("Container.Test.P16.0.RunInventory");
			Container.Slots = { TreatmentItemId, GenericPillId };
			Initial.Containers.Add(Container);

			FShanmenItemInstance TreatmentItem;
			TreatmentItem.ItemInstanceId = TreatmentItemId;
			TreatmentItem.DefinitionId =
				Fdemo_mapItemIds::MeridianStabilizingPillLevel1;
			TreatmentItem.RunId = ScopeId;
			TreatmentItem.OwnerId = OwnerId;
			TreatmentItem.ParentContainerId = ContainerId;
			TreatmentItem.SlotIndex = 0;
			TreatmentItem.Quantity = 4;
			Initial.Items.Add(TreatmentItem);

			FShanmenItemInstance GenericPill = TreatmentItem;
			GenericPill.ItemInstanceId = GenericPillId;
			GenericPill.DefinitionId = Fdemo_mapItemIds::HealingPillLevel1;
			GenericPill.SlotIndex = 1;
			GenericPill.Quantity = 2;
			Initial.Items.Add(GenericPill);
			if (!Repository.TryLoadSnapshot(Initial))
			{
				return false;
			}

			FShanmenItemReserveRequest TreatmentReserve;
			TreatmentReserve.Context = MakeContext(TreatmentReserveRequestId);
			TreatmentReserve.ItemInstanceId = TreatmentItemId;
			TreatmentReserve.ResourceKind = EShanmenItemResourceKind::Quantity;
			TreatmentReserve.Amount = 4;
			TreatmentReserve.ExpectedItemRevision = 0;
			TreatmentReserve.PurposeId =
				TEXT("Prepare.RunInventory.MeridianTreatment.P16.0");
			const FShanmenItemTransactionReceipt TreatmentReserved =
				Repository.Reserve(TreatmentReserve);

			FShanmenItemReserveRequest GenericReserve;
			GenericReserve.Context = MakeContext(GenericReserveRequestId);
			GenericReserve.ItemInstanceId = GenericPillId;
			GenericReserve.ResourceKind = EShanmenItemResourceKind::Quantity;
			GenericReserve.Amount = 2;
			GenericReserve.ExpectedItemRevision = 0;
			GenericReserve.PurposeId =
				TEXT("Prepare.RunInventory.GenericPill.P16.0");
			const FShanmenItemTransactionReceipt GenericReserved =
				TreatmentReserved.IsSuccess()
					? Repository.Reserve(GenericReserve)
					: FShanmenItemTransactionReceipt();

			FShanmenItemRunStartRequest Start;
			Start.Context = MakeContext(StartRequestId);
			Start.ReservationIds =
				{ TreatmentReserved.ReservationId, GenericReserved.ReservationId };
			const FShanmenItemTransactionReceipt Started =
				GenericReserved.IsSuccess()
					? Repository.StartPreparedRun(Start)
					: FShanmenItemTransactionReceipt();
			if (!Started.IsSuccess())
			{
				return false;
			}
			Snapshot = Repository.CaptureSnapshot();

			Correlation.CorrelationId = FGuid(0xC1600010, 0, 0, 1);
			Correlation.OwnerId = OwnerId;
			Correlation.ScopeId = ScopeId;
			Correlation.ActiveRunId = Started.ReservationId;
			Correlation.PreparedRequestId = GenericReserveRequestId;
			Correlation.PreparedReceiptId = GenericReserved.ReceiptId;
			Correlation.LifecycleRequestId = Started.RequestId;
			Correlation.LifecycleReceiptId = Started.ReceiptId;
			Correlation.PreparedAuthorityRevision =
				GenericReserved.AuthorityRevision;
			Correlation.LifecycleAuthorityRevision =
				Started.AuthorityRevision;
			Correlation.OrderedPreparedItemInstanceIds =
				{ TreatmentItemId, GenericPillId };
			Correlation.OrderedRunInventoryItemInstanceIds =
				{ TreatmentItemId, GenericPillId };
			Correlation.HotbarItemInstanceIds.SetNum(
				Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
			Correlation.HotbarItemInstanceIds[0] = TreatmentItemId;
			Correlation.HotbarItemInstanceIds[1] = GenericPillId;

			Attributes = NewObject<Udemo_mapAttributeComponent>(
				GetTransientPackage());
			Conditions = NewObject<
				Udemo_mapShanmenCombatConditionComponent>(
					GetTransientPackage());
			FString Diagnostic;
			return Correlation.IsValid()
				&& Attributes != nullptr
				&& Conditions != nullptr
				&& Timeline.TryBegin(Correlation.ActiveRunId, Diagnostic)
				&& Conditions->TryBegin(
					Correlation.ActiveRunId,
					TargetEntityId,
					Timeline.GetTimelineId(),
					Attributes,
					Diagnostic)
				&& Timeline.TryCapture(TimelineSample)
				&& Conditions->TryApplyMeridianShock(
					MakeCommittedReceipt(),
					TimelineSample).IsSuccess()
				&& Conditions->TryCaptureMeridianShockStatus(ActiveStatus)
				&& ActiveStatus.IsActive();
		}

		float MoveSpeed() const
		{
			float Value = -1.0f;
			check(Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::MoveSpeed,
				Value));
			return Value;
		}

		Fdemo_mapShanmenMeridianShockTreatmentItemResult Prepare(
			const FGuid& ItemId = TreatmentItemId)
		{
			Fdemo_mapShanmenMeridianShockTreatmentItemResult Result =
				Fdemo_mapShanmenMeridianShockTreatmentAdapter::
					BuildPrepareRequest(
						Snapshot,
						Correlation,
						ActiveStatus,
						ItemId);
			if (Result.Status
				!= Edemo_mapShanmenMeridianShockTreatmentStatus::
					RequestReady)
			{
				return Result;
			}
			Result.PrepareCommand.Status =
				EShanmenItemDurableCommandStatus::Persisted;
			Result.PrepareCommand.Receipt =
				Repository.PreparePreparedRunQuantityIntent(
					Result.PrepareRequest);
			Result.Status = Result.PrepareCommand.IsCommandSuccess()
				? Edemo_mapShanmenMeridianShockTreatmentStatus::Prepared
				: Edemo_mapShanmenMeridianShockTreatmentStatus::
					PrepareRejected;
			Snapshot = Repository.CaptureSnapshot();
			return Result;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentCatalogTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.CanonicalCatalog",
	TreatmentFlags)

bool Fdemo_mapMeridianShockTreatmentCatalogTest::RunTest(const FString&)
{
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(
			Fdemo_mapItemIds::MeridianStabilizingPillLevel1);
	TestTrue(TEXT("canonical treatment item has one exact product semantic"),
		Definition
			&& Definition->CategoryId == Fdemo_mapItemIds::ConsumableCategory
			&& Definition->MaxStackSize == 20
			&& Definition->bHotbarEligible
			&& Definition->bPurchasable
			&& Definition->BuyPrice == 45
			&& Definition->SellPrice == 22
			&& Definition->HasGameplaySemantic(
				Edemo_mapItemGameplaySemantic::MeridianShockTreatment)
			&& Definition->GameplaySemantics.Num() == 1);
	TestTrue(TEXT("P21.0 content identity is current"),
		Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
			TEXT("CodeB.Content.0.0.10.P21.0"),
			TEXT("A52AA4EEE9DBF314C017205BFC6E417B8C9A108D37471C7DDE088013B85FC685")));
	TestTrue(TEXT("P16.0 identity remains known historical evidence"),
		Fdemo_mapItemDefinitions::IsKnownContentIdentity(
			TEXT("CodeB.Content.0.0.10.P16.0"),
			TEXT("9B789DB381BB934F772326A5217094F19C654D5AF51D5623EDB0A108DBA4CC7B")));
	TestTrue(TEXT("P11.7 identity remains known historical evidence"),
		Fdemo_mapItemDefinitions::IsKnownContentIdentity(
			TEXT("CodeB.Content.0.0.10.P11.7"),
			TEXT("6D01652004E386DC469CB09FF0F3A77110C53841F6AF3F66F5600C3C9B9B4179")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentAuthorityTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.TreatmentAuthority",
	TreatmentFlags)

bool Fdemo_mapMeridianShockTreatmentAuthorityTest::RunTest(const FString&)
{
	FTreatmentFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P16.0 treatment authority fixture."));
		return false;
	}
	Fdemo_mapShanmenCombatConditionTreatmentIntent Intent;
	TestTrue(TEXT("active exact condition revision captures treatment intent"),
		Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
			Fixture.ActiveStatus.GetRunId(),
			Fixture.ActiveStatus.GetTargetEntityId(),
			Fixture.ActiveStatus.GetTimelineId(),
			TreatmentItemId,
			Fdemo_mapItemIds::MeridianStabilizingPillLevel1,
			Fixture.ActiveStatus.GetConditionRevision(),
			Intent)
			&& Intent.IsValid());
	const Fdemo_mapShanmenCombatConditionTreatmentResult Treated =
		Fixture.Conditions->TryTreatMeridianShock(
			Intent,
			Fixture.TimelineSample);
	TestTrue(TEXT("exact treatment clears modifier and advances once"),
		Treated.Status
				== Edemo_mapShanmenCombatConditionTreatmentStatus::Treated
			&& Treated.IsSuccess()
			&& Treated.Receipt.Matches(Intent)
			&& Treated.Receipt.GetConditionRevisionBefore() == 1
			&& Treated.Receipt.GetConditionRevisionAfter() == 2
			&& !Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Conditions->GetConditionRevision() == 2
			&& Fixture.Conditions->NumProcessedTreatments() == 1
			&& Fixture.Attributes->GetActiveModifierCount() == 0
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 600.0f));
	const Fdemo_mapShanmenCombatConditionTreatmentResult Replay =
		Fixture.Conditions->TryTreatMeridianShock(
			Intent,
			Fixture.TimelineSample);
	TestTrue(TEXT("exact treatment replay returns the original receipt"),
		Replay.Status
				== Edemo_mapShanmenCombatConditionTreatmentStatus::AlreadyTreated
			&& Replay.IsSuccess()
			&& Replay.Receipt.GetTreatmentId()
				== Treated.Receipt.GetTreatmentId()
			&& Fixture.Conditions->GetConditionRevision() == 2
			&& Fixture.Conditions->NumProcessedTreatments() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentCommitTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.CommitAndReplay",
	TreatmentFlags)

bool Fdemo_mapMeridianShockTreatmentCommitTest::RunTest(const FString&)
{
	FTreatmentFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P16.0 treatment commit fixture."));
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentItemResult Prepared =
		Fixture.Prepare();
	const Fdemo_mapShanmenCombatConditionTreatmentResult Treated =
		Fixture.Conditions->TryTreatMeridianShock(
			Prepared.TreatmentIntent,
			Fixture.TimelineSample);
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Commit =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::BuildCommitRequest(
			Fixture.Correlation,
			Prepared,
			Treated.Receipt);
	const FShanmenItemTransactionReceipt Committed =
		Commit.FinalizeRequest.IsValid()
			? Fixture.Repository.FinalizePreparedRunQuantityIntent(
				Commit.FinalizeRequest)
			: FShanmenItemTransactionReceipt();
	Fixture.Snapshot = Fixture.Repository.CaptureSnapshot();
	TestTrue(TEXT("condition proof is the only durable consumption commit point"),
		Prepared.IsPrepared()
			&& Treated.IsSuccess()
			&& Commit.Status
				== Edemo_mapShanmenMeridianShockTreatmentStatus::RequestReady
			&& Commit.FinalizeRequest.bCommit
			&& Committed.IsSuccess()
			&& Committed.Phase == EShanmenItemTransactionPhase::Committed
			&& Committed.ResourceBefore == 4
			&& Committed.ResourceAfter == 3
			&& Fixture.Repository.ValidateInvariants());

	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Rebuilt =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::BuildPrepareRequest(
			Fixture.Snapshot,
			Fixture.Correlation,
			Fixture.ActiveStatus,
			TreatmentItemId);
	const Fdemo_mapShanmenCombatConditionTreatmentResult TreatmentReplay =
		Fixture.Conditions->TryTreatMeridianShock(
			Prepared.TreatmentIntent,
			Fixture.TimelineSample);
	TestTrue(TEXT("immutable pre-treatment evidence reconstructs exact replay"),
		Rebuilt.HasPrepareRequest()
			&& Rebuilt.PrepareRequest.Context.RequestId
				== Prepared.PrepareRequest.Context.RequestId
			&& Rebuilt.PrepareRequest.ExpectedQuantityBefore == 4
			&& TreatmentReplay.Status
				== Edemo_mapShanmenCombatConditionTreatmentStatus::AlreadyTreated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentCancelTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.PreTreatmentCancel",
	TreatmentFlags)

bool Fdemo_mapMeridianShockTreatmentCancelTest::RunTest(const FString&)
{
	FTreatmentFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P16.0 treatment cancel fixture."));
		return false;
	}
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Prepared =
		Fixture.Prepare();
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Cancel =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::BuildCancelRequest(
			Fixture.Correlation,
			Prepared,
			Fixture.ActiveStatus);
	const FShanmenItemTransactionReceipt Cancelled =
		Cancel.FinalizeRequest.IsValid()
			? Fixture.Repository.FinalizePreparedRunQuantityIntent(
				Cancel.FinalizeRequest)
			: FShanmenItemTransactionReceipt();
	TestTrue(TEXT("pre-treatment cancellation preserves item and condition"),
		Prepared.IsPrepared()
			&& Cancel.Status
				== Edemo_mapShanmenMeridianShockTreatmentStatus::RequestReady
			&& !Cancel.FinalizeRequest.bCommit
			&& Cancelled.IsSuccess()
			&& Cancelled.Phase == EShanmenItemTransactionPhase::Cancelled
			&& Cancelled.ResourceBefore == 4
			&& Cancelled.ResourceAfter == 4
			&& Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Conditions->GetConditionRevision() == 1
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 450.0f)
			&& Fixture.Repository.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockTreatmentFailClosedTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.FailClosed",
	TreatmentFlags)

bool Fdemo_mapMeridianShockTreatmentFailClosedTest::RunTest(const FString&)
{
	FTreatmentFixture Fixture;
	if (!Fixture.Build())
	{
		AddError(TEXT("Could not build the P16.0 treatment fail-closed fixture."));
		return false;
	}
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult Generic =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::BuildPrepareRequest(
			Fixture.Snapshot,
			Fixture.Correlation,
			Fixture.ActiveStatus,
			GenericPillId);
	TestTrue(TEXT("generic healing pill cannot borrow treatment semantics"),
		Generic.Status
			== Edemo_mapShanmenMeridianShockTreatmentStatus::
				DefinitionNotTreatment
			&& !Generic.HasPrepareRequest());

	Fdemo_mapShanmenCombatConditionTreatmentIntent StaleRevision;
	TestTrue(TEXT("future revision still captures as immutable intent"),
		Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
			Fixture.ActiveStatus.GetRunId(),
			Fixture.ActiveStatus.GetTargetEntityId(),
			Fixture.ActiveStatus.GetTimelineId(),
			TreatmentItemId,
			Fdemo_mapItemIds::MeridianStabilizingPillLevel1,
			Fixture.ActiveStatus.GetConditionRevision() + 1,
			StaleRevision));
	TestTrue(TEXT("wrong condition revision fails without mutation"),
		Fixture.Conditions->TryTreatMeridianShock(
			StaleRevision,
			Fixture.TimelineSample).Error
				== Edemo_mapShanmenCombatConditionTreatmentError::
					ConditionRevisionMismatch
			&& Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Conditions->GetConditionRevision() == 1
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 450.0f));

	Fdemo_mapShanmenMeridianShockTreatmentItemResult Prepared =
		Fixture.Prepare();
	FTreatmentFixture Other;
	if (!Other.Build())
	{
		AddError(TEXT("Could not build the independent receipt fixture."));
		return false;
	}
	Fdemo_mapShanmenCombatConditionTreatmentIntent OtherIntent;
	check(Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
		Other.ActiveStatus.GetRunId(),
		Other.ActiveStatus.GetTargetEntityId(),
		Other.ActiveStatus.GetTimelineId(),
		FGuid(0xC16000FF, 0, 0, 1),
		Fdemo_mapItemIds::MeridianStabilizingPillLevel1,
		Other.ActiveStatus.GetConditionRevision(),
		OtherIntent));
	const Fdemo_mapShanmenCombatConditionTreatmentResult OtherTreatment =
		Other.Conditions->TryTreatMeridianShock(
			OtherIntent,
			Other.TimelineSample);
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult WrongReceipt =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::BuildCommitRequest(
			Fixture.Correlation,
			Prepared,
			OtherTreatment.Receipt);
	TestTrue(TEXT("another item receipt cannot consume this prepared intent"),
		Prepared.IsPrepared()
			&& OtherTreatment.IsSuccess()
			&& WrongReceipt.Status
				== Edemo_mapShanmenMeridianShockTreatmentStatus::
					TreatmentMismatch
			&& !WrongReceipt.FinalizeRequest.IsValid());

	const Fdemo_mapShanmenCombatConditionTreatmentResult Treated =
		Fixture.Conditions->TryTreatMeridianShock(
			Prepared.TreatmentIntent,
			Fixture.TimelineSample);
	Fdemo_mapShanmenCombatConditionStatusSnapshot TreatedStatus;
	const bool bCapturedTreatedStatus =
		Fixture.Conditions->TryCaptureMeridianShockStatus(TreatedStatus);
	const Fdemo_mapShanmenMeridianShockTreatmentItemResult LateCancel =
		Fdemo_mapShanmenMeridianShockTreatmentAdapter::BuildCancelRequest(
			Fixture.Correlation,
			Prepared,
			TreatedStatus);
	TestTrue(TEXT("treated condition cannot reuse old evidence to cancel consumption"),
		Treated.IsSuccess()
			&& bCapturedTreatedStatus
			&& TreatedStatus.IsValid()
			&& !TreatedStatus.IsActive()
			&& LateCancel.Status
				== Edemo_mapShanmenMeridianShockTreatmentStatus::
					ConditionStatusInvalid
			&& !LateCancel.FinalizeRequest.IsValid());
	return true;
}

#endif
