#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenItemGeneratedSource.h"
#include "ShanmenItemTags.h"

namespace
{
	using FPlan = FShanmenItemGeneratedSourcePlan;
	using FView = FShanmenItemGeneratedSourceView;
	using FReceipt = FShanmenItemGeneratedSourceReceipt;
	using FContract = FShanmenItemGeneratedSourceContract;
	using EDecision = EShanmenItemGeneratedSourceDecision;
	using EError = EShanmenItemGeneratedSourceError;
	using ERun = EShanmenItemGeneratedSourceRunState;

	FPlan MakePlan()
	{
		FPlan Plan;
		Plan.OwnerId = FGuid(10, 20, 30, 40);
		Plan.RunId = FGuid(50, 60, 70, 80);
		Plan.SourceRoleId = TEXT("Source.Contract.Nonzero");
		Plan.Content.Version = TEXT("Manifest.Contract.1");
		Plan.Content.Digest = TEXT("resolved-manifest-content");
		Plan.SlotId = TEXT("Slot.Contract");
		Plan.ProjectionId = TEXT("Projection.Contract");
		Plan.DistributionProfileId = TEXT("Distribution.Contract");
		Plan.BudgetProfileId = TEXT("Budget.Contract");
		Plan.MarkerId = TEXT("Marker.Contract");
		Plan.EncounterId = TEXT("Encounter.Contract");
		Plan.JackpotPolicyId = TEXT("Jackpot.Contract");
		Plan.RareExtremePolicyId = TEXT("Rare.Contract");
		Plan.AffixPolicyId = TEXT("Affix.Contract");
		Plan.EffectiveSeed = 123456789;
		Plan.RandomizedBudget = 900;
		Plan.GeneratedTotalValue = 830;
		Plan.ResidualValue = 70;
		Plan.ExpectedSequence = 11;
		Plan.PityStateBefore = 7; // Domain contract must not bake in product's 0..3 policy.
		Plan.PityStateAfter = 8;
		Plan.bPityCommitRequired = true;
		FShanmenItemGeneratedSourceEntry Entry;
		Entry.Definition.DefinitionId = TEXT("Item.Contract.Stack");
		Entry.Definition.MaxStack = 20;
		Entry.Definition.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityConsumeQuantity());
		Entry.Quantity = 3;
		Entry.SectionId = TEXT("Section.Body");
		Entry.SlotIndex = 2;
		Entry.UnitValue = 10;
		Entry.TotalValue = 30;
		Plan.Entries.Add(Entry);
		Entry.Definition = FShanmenItemDefinition();
		Entry.Definition.DefinitionId = TEXT("Item.Contract.Equipment");
		Entry.Definition.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityDurability());
		Entry.Definition.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityCharges());
		Entry.Definition.MaxDurability = 100;
		Entry.Definition.MaxCharges = 3;
		Entry.Quantity = 1;
		Entry.SectionId = TEXT("Section.Equipment");
		Entry.SlotIndex = 1;
		Entry.UnitValue = 100;
		Entry.TotalValue = 800;
		Entry.ChildContainerType = TEXT("Container.Contract");
		Entry.ChildContainerCapacity = 4;
		Entry.RewardMetadata.RewardEventKind = EShanmenItemRewardEventKind::Jackpot;
		Entry.RewardMetadata.RewardEventId = FGuid(100, 1, 1, 1);
		Entry.RewardMetadata.RewardValueMultiplierBps = FShanmenItemRewardMetadata::JackpotMultiplierBps;
		Entry.RewardMetadata.RewardSourceRoleId = Plan.SourceRoleId;
		Entry.RewardMetadata.RareRewardEventId = FGuid(100, 1, 1, 2);
		Entry.RewardMetadata.RareRewardPolicyId = Plan.RareExtremePolicyId;
		Entry.RewardMetadata.RareRewardTierId = TEXT("Rare.Tier.Contract");
		Entry.RewardMetadata.RareRewardBonusValue = 150;
		Entry.RewardMetadata.AffixSetEventId = FGuid(100, 1, 1, 3);
		Entry.RewardMetadata.AffixPolicyId = Plan.AffixPolicyId;
		Entry.RewardMetadata.AffixAcquisition = EShanmenItemRewardAffixAcquisition::Natural;
		FShanmenItemResolvedRewardAffix Affix;
		Affix.AffixId = TEXT("Affix.Resolved.Contract");
		Affix.Tier = EShanmenItemRewardAffixTier::Tier1;
		Affix.ResolvedMagnitudeScaled = 123;
		Affix.ResolvedValue = 50;
		Entry.RewardMetadata.Affixes.Add(Affix);
		Plan.Entries.Add(Entry);
		return Plan;
	}

	FView MakeView(const FPlan& Plan)
	{
		FView View;
		View.OwnerId = Plan.OwnerId;
		View.RunId = Plan.RunId;
		View.SourceContent = Plan.Content;
		View.State = ERun::Active;
		View.AcceptedSequence = Plan.ExpectedSequence;
		View.PityState = Plan.PityStateBefore;
		return View;
	}

	FView After(const FReceipt& Receipt)
	{
		FView View = MakeView(Receipt.GetPlan());
		View.AcceptedSequence = Receipt.GetAcceptedSequence();
		View.PityState = Receipt.GetPlan().PityStateAfter;
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGeneratedSourceIdentityTest,
	"Shanmen.0_0_10.Items.GeneratedSource.DeterministicIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenGeneratedSourceIdentityTest::RunTest(const FString&)
{
	const FPlan Plan = MakePlan();
	const FView View = MakeView(Plan);
	const auto First = FContract::Evaluate(View, Plan);
	const auto Again = FContract::Evaluate(View, Plan);
	TestTrue(TEXT("nonzero fixture is valid"), Plan.IsValid());
	TestTrue(TEXT("preparation is not durable acceptance"), First.Decision == EDecision::CandidatePrepared);
	TestTrue(TEXT("deterministic entire value"), First.Receipt == Again.Receipt);
	TestTrue(TEXT("validated immutable receipt"), First.Receipt.IsValid());
	TestEqual(TEXT("sequence increments once"), First.Receipt.GetAcceptedSequence(), int64(12));
	TestEqual(TEXT("view remains unchanged"), View.AcceptedSequence, int64(11));
	TestEqual(TEXT("view pity remains unchanged"), View.PityState, 7);
	const auto& Ids = First.Receipt.GetItemIds();
	TestEqual(TEXT("one identity per ordered entry"), Ids.Num(), 2);
	TSet<FGuid> Unique { First.Receipt.GetSourceId(), First.Receipt.GetContainerId() };
	for (const auto& Id : Ids) { Unique.Add(Id); }
	TestEqual(TEXT("namespace and ordinal separation"), Unique.Num(), 4);
	TestTrue(TEXT("FName equivalent casing same identity"),
		FContract::MakeSourceId(Plan.OwnerId, Plan.RunId, TEXT("SOURCE.CONTRACT.NONZERO")) == First.Receipt.GetSourceId());
	TestTrue(TEXT("owner separates identity"),
		FContract::MakeSourceId(FGuid(99, 0, 0, 1), Plan.RunId, Plan.SourceRoleId) != First.Receipt.GetSourceId());
	TestTrue(TEXT("run separates identity"),
		FContract::MakeSourceId(Plan.OwnerId, FGuid(99, 0, 0, 2), Plan.SourceRoleId) != First.Receipt.GetSourceId());
	TestTrue(TEXT("source separates identity"),
		FContract::MakeSourceId(Plan.OwnerId, Plan.RunId, TEXT("Source.Other")) != First.Receipt.GetSourceId());
	TestFalse(TEXT("invalid owner cannot derive identity"), FContract::MakeSourceId({}, Plan.RunId, Plan.SourceRoleId).IsValid());
	TestFalse(TEXT("empty source cannot derive identity"), FContract::MakeSourceId(Plan.OwnerId, Plan.RunId, NAME_None).IsValid());
	FPlan Changed = Plan;
	++Changed.EffectiveSeed;
	const auto ChangedResult = FContract::Evaluate(View, Changed);
	TestTrue(TEXT("changing plan does not evade source key"), ChangedResult.Receipt.GetSourceId() == First.Receipt.GetSourceId());
	TestTrue(TEXT("changing plan does not re-key items"), ChangedResult.Receipt.GetItemIds() == Ids);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGeneratedSourceReplayTest,
	"Shanmen.0_0_10.Items.GeneratedSource.ExactReplayAndTerminal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenGeneratedSourceReplayTest::RunTest(const FString&)
{
	const FPlan Plan = MakePlan();
	const FReceipt Original = FContract::Evaluate(MakeView(Plan), Plan).Receipt;
	FView View = After(Original);
	for (const ERun State : { ERun::Active, ERun::Finalized })
	{
		View.State = State;
		const auto Replay = FContract::Evaluate(View, Plan, &Original);
		TestTrue(TEXT("existing exact source replay"), Replay.Decision == EDecision::ExactReplay);
		TestTrue(TEXT("replay retains entire original receipt"), Replay.Receipt == Original);
	}
	TestTrue(TEXT("terminal new acceptance rejected"), FContract::Evaluate(View, Plan).Error == EError::RunClosed);
	View.AcceptedSequence = 99;
	View.PityState = 23;
	TestTrue(TEXT("historical exact replay ignores later cursor changes"),
		FContract::Evaluate(View, Plan, &Original).Decision == EDecision::ExactReplay);
	const FReceipt Copy = Original;
	TestTrue(TEXT("value copy keeps full recovery payload"), Copy == Original && Copy.IsValid());
	TestEqual(TEXT("affix nonzero survives"), Copy.GetPlan().Entries[1].RewardMetadata.Affixes[0].ResolvedMagnitudeScaled, 123);
	FPlan Next = Plan;
	Next.SourceRoleId = TEXT("Source.Next");
	Next.Entries[1].RewardMetadata.RewardSourceRoleId = Next.SourceRoleId;
	TestTrue(TEXT("closed run new source rejected"), FContract::Evaluate(View, Next).Error == EError::RunClosed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGeneratedSourceConflictTest,
	"Shanmen.0_0_10.Items.GeneratedSource.CompletePayloadConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenGeneratedSourceConflictTest::RunTest(const FString&)
{
	const FPlan Original = MakePlan();
	const FReceipt Receipt = FContract::Evaluate(MakeView(Original), Original).Receipt;
	const FView View = After(Receipt);
	int32 Cases = 0;
	auto RejectChange = [&](const TCHAR* Label, TFunctionRef<void(FPlan&)> Change)
	{
		FPlan Changed = Original;
		Change(Changed);
		const auto Result = FContract::Evaluate(View, Changed, &Receipt);
		TestTrue(Label, !(Changed == Original) && Result.Decision == EDecision::Rejected);
		TestFalse(TEXT("rejection exposes no reusable receipt"), Result.Receipt.IsValid());
		TestTrue(TEXT("original receipt preserved"), Receipt.GetPlan() == Original);
		++Cases;
	};
#define CHECK_CHANGE(Expression) RejectChange(TEXT(#Expression), [](FPlan& P) { Expression; })
	CHECK_CHANGE(P.OwnerId.D++);
	CHECK_CHANGE(P.RunId.D++);
	CHECK_CHANGE(P.SourceRoleId = TEXT("Source.Conflict"));
	CHECK_CHANGE(P.Content.Version = TEXT("Manifest.Other"));
	CHECK_CHANGE(P.Content.Digest += TEXT(".other"));
	CHECK_CHANGE(P.SlotId = TEXT("Slot.Other"));
	CHECK_CHANGE(P.ProjectionId = TEXT("Projection.Other"));
	CHECK_CHANGE(P.DistributionProfileId = TEXT("Distribution.Other"));
	CHECK_CHANGE(P.BudgetProfileId = TEXT("Budget.Other"));
	CHECK_CHANGE(P.MarkerId = TEXT("Marker.Other"));
	CHECK_CHANGE(P.EncounterId = TEXT("Encounter.Other"));
	CHECK_CHANGE(P.JackpotPolicyId = TEXT("Jackpot.Other"));
	CHECK_CHANGE(P.RareExtremePolicyId = TEXT("Rare.Other"));
	CHECK_CHANGE(P.AffixPolicyId = TEXT("Affix.Other"));
	CHECK_CHANGE(P.EffectiveSeed++);
	CHECK_CHANGE(P.RandomizedBudget++);
	CHECK_CHANGE(P.GeneratedTotalValue++);
	CHECK_CHANGE(P.ResidualValue++);
	CHECK_CHANGE(P.ExpectedSequence++);
	CHECK_CHANGE(P.PityStateBefore++);
	CHECK_CHANGE(P.PityStateAfter++);
	CHECK_CHANGE(P.bPityCommitRequired = false);
	CHECK_CHANGE(P.bFallbackUsed = true);
	CHECK_CHANGE(P.bLegacyCompatibilityView = true);
	CHECK_CHANGE(P.Entries.Swap(0, 1));
	CHECK_CHANGE(P.Entries.RemoveAt(1));
	CHECK_CHANGE(P.Entries[0].Definition.DefinitionId = TEXT("Item.Other"));
	CHECK_CHANGE(P.Entries[0].Definition.MaxStack++);
	CHECK_CHANGE(P.Entries[1].Definition.MaxDurability++);
	CHECK_CHANGE(P.Entries[1].Definition.MaxCharges++);
	CHECK_CHANGE(P.Entries[1].Definition.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityDeploy()));
	CHECK_CHANGE(P.Entries[0].Quantity++);
	CHECK_CHANGE(P.Entries[0].SectionId = TEXT("Section.Other"));
	CHECK_CHANGE(P.Entries[0].SlotIndex++);
	CHECK_CHANGE(P.Entries[0].UnitValue++);
	CHECK_CHANGE(P.Entries[0].TotalValue++);
	CHECK_CHANGE(P.Entries[1].ChildContainerType = TEXT("Container.Other"));
	CHECK_CHANGE(P.Entries[1].ChildContainerCapacity++);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.RewardEventKind = EShanmenItemRewardEventKind::None);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.RewardEventId.D++);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.RewardValueMultiplierBps++);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.RewardSourceRoleId = TEXT("Source.Other"));
	CHECK_CHANGE(P.Entries[1].RewardMetadata.RareRewardEventId.D++);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.RareRewardPolicyId = TEXT("Rare.Other"));
	CHECK_CHANGE(P.Entries[1].RewardMetadata.RareRewardTierId = TEXT("Tier.Other"));
	CHECK_CHANGE(P.Entries[1].RewardMetadata.RareRewardBonusValue++);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.AffixSetEventId.D++);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.AffixPolicyId = TEXT("Affix.Other"));
	CHECK_CHANGE(P.Entries[1].RewardMetadata.AffixAcquisition = EShanmenItemRewardAffixAcquisition::PityGuaranteed);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.Affixes[0].AffixId = TEXT("Affix.Other"));
	CHECK_CHANGE(P.Entries[1].RewardMetadata.Affixes[0].Tier = EShanmenItemRewardAffixTier::Tier2);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.Affixes[0].ResolvedMagnitudeScaled++);
	CHECK_CHANGE(P.Entries[1].RewardMetadata.Affixes[0].ResolvedValue++);
#undef CHECK_CHANGE
	TestEqual(TEXT("all payload mutations exercised"), Cases, 53);
	FPlan Changed = Original;
	Changed.EffectiveSeed++;
	TestTrue(TEXT("valid conflicting plan has explicit conflict result"),
		FContract::Evaluate(View, Changed, &Receipt).Error == EError::SourceConflict);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGeneratedSourceGuardsTest,
	"Shanmen.0_0_10.Items.GeneratedSource.AuthorityScopeAndCursor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenGeneratedSourceGuardsTest::RunTest(const FString&)
{
	const FPlan Plan = MakePlan();
	FView View = MakeView(Plan);
	const FReceipt Receipt = FContract::Evaluate(View, Plan).Receipt;
	View.OwnerId.D++;
	TestTrue(TEXT("wrong owner fails"), FContract::Evaluate(View, Plan).Error == EError::ScopeMismatch);
	View = MakeView(Plan); View.RunId.D++;
	TestTrue(TEXT("wrong run fails"), FContract::Evaluate(View, Plan).Error == EError::ScopeMismatch);
	View = MakeView(Plan); View.SourceContent.Digest += TEXT("drift");
	TestTrue(TEXT("wrong content fails"), FContract::Evaluate(View, Plan).Error == EError::ContentMismatch);
	View = MakeView(Plan); ++View.AcceptedSequence;
	TestTrue(TEXT("out-of-order source fails"), FContract::Evaluate(View, Plan).Error == EError::CursorConflict);
	View = MakeView(Plan); ++View.PityState;
	TestTrue(TEXT("stale pity fails"), FContract::Evaluate(View, Plan).Error == EError::CursorConflict);
	View = MakeView(Plan); View.State = static_cast<ERun>(255);
	TestTrue(TEXT("unknown run state fails closed"), FContract::Evaluate(View, Plan).Error == EError::InvalidView);
	View = MakeView(Plan);
	TestTrue(TEXT("future receipt not silently accepted"),
		FContract::Evaluate(View, Plan, &Receipt).Error == EError::InvalidExistingReceipt);
	View = After(Receipt); ++View.PityState;
	TestTrue(TEXT("last receipt must agree with current pity"),
		FContract::Evaluate(View, Plan, &Receipt).Error == EError::InvalidExistingReceipt);
	View = After(Receipt);
	FReceipt Invalid;
	TestTrue(TEXT("invalid existing receipt not treated as absent"),
		FContract::Evaluate(View, Plan, &Invalid).Error == EError::InvalidExistingReceipt);
	FPlan Other = Plan;
	Other.SourceRoleId = TEXT("Source.Other");
	Other.Entries[1].RewardMetadata.RewardSourceRoleId = Other.SourceRoleId;
	const FReceipt OtherReceipt = FContract::Evaluate(MakeView(Other), Other).Receipt;
	TestTrue(TEXT("foreign source lookup fails"),
		FContract::Evaluate(View, Plan, &OtherReceipt).Error == EError::InvalidExistingReceipt);
	FPlan NoPity = Plan;
	NoPity.bPityCommitRequired = false;
	NoPity.PityStateAfter = NoPity.PityStateBefore;
	TestTrue(TEXT("non-pity source preserves nonzero pity"),
		FContract::Evaluate(MakeView(NoPity), NoPity).Receipt.GetPlan().PityStateAfter == 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGeneratedSourceBoundsTest,
	"Shanmen.0_0_10.Items.GeneratedSource.BoundedPlanValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShanmenGeneratedSourceBoundsTest::RunTest(const FString&)
{
	const FPlan Base = MakePlan();
	auto Invalid = [&](const TCHAR* Label, TFunctionRef<void(FPlan&)> Change)
	{
		FPlan P = Base;
		Change(P);
		TestFalse(Label, P.IsValid());
		const auto Result = FContract::Evaluate(MakeView(Base), P);
		TestTrue(TEXT("invalid plan rejected with no receipt"),
			Result.Error == EError::InvalidPlan && !Result.Receipt.IsValid());
	};
	Invalid(TEXT("empty entries"), [](FPlan& P) { P.Entries.Reset(); });
	Invalid(TEXT("zero seed"), [](FPlan& P) { P.EffectiveSeed = 0; });
	Invalid(TEXT("negative value"), [](FPlan& P) { P.Entries[0].TotalValue = -1; });
	Invalid(TEXT("zero quantity"), [](FPlan& P) { P.Entries[0].Quantity = 0; });
	Invalid(TEXT("over stack"), [](FPlan& P) { P.Entries[0].Quantity = 21; });
	Invalid(TEXT("negative slot"), [](FPlan& P) { P.Entries[0].SlotIndex = -1; });
	Invalid(TEXT("over slot"), [](FPlan& P) { P.Entries[0].SlotIndex = FPlan::MaxSlots; });
	Invalid(TEXT("missing section"), [](FPlan& P) { P.Entries[0].SectionId = NAME_None; });
	Invalid(TEXT("duplicate address"), [](FPlan& P) {
		P.Entries[1].SectionId = P.Entries[0].SectionId; P.Entries[1].SlotIndex = P.Entries[0].SlotIndex;
	});
	Invalid(TEXT("inconsistent same definition"), [](FPlan& P) {
		auto Duplicate = P.Entries[0];
		Duplicate.SlotIndex = 6;
		Duplicate.Definition.MaxStack++;
		P.Entries.Add(MoveTemp(Duplicate));
	});
	Invalid(TEXT("child missing type"), [](FPlan& P) { P.Entries[1].ChildContainerType = NAME_None; });
	Invalid(TEXT("child over capacity"), [](FPlan& P) { P.Entries[1].ChildContainerCapacity = FPlan::MaxSlots + 1; });
	Invalid(TEXT("foreign reward provenance"), [](FPlan& P) { P.Entries[1].RewardMetadata.RewardSourceRoleId = TEXT("Other"); });
	Invalid(TEXT("incomplete reward metadata"), [](FPlan& P) { P.Entries[1].RewardMetadata.AffixSetEventId.Invalidate(); });
	Invalid(TEXT("sequence overflow"), [](FPlan& P) { P.ExpectedSequence = MAX_int64; });
	Invalid(TEXT("negative sequence"), [](FPlan& P) { P.ExpectedSequence = -1; });
	Invalid(TEXT("oversized digest"), [](FPlan& P) { P.Content.Digest = FString::ChrN(FPlan::MaxDigestLength + 1, TEXT('a')); });
	FPlan Max = Base;
	Max.Entries.Reset();
	for (int32 Index = 0; Index < FPlan::MaxEntries; ++Index)
	{
		auto Entry = Base.Entries[0];
		Entry.SlotIndex = Index;
		Max.Entries.Add(Entry);
	}
	const auto MaxResult = FContract::Evaluate(MakeView(Max), Max);
	TestTrue(TEXT("exact entry limit admitted"), MaxResult.Receipt.IsValid());
	TestEqual(TEXT("bounded receipt count"), MaxResult.Receipt.GetItemIds().Num(), FPlan::MaxEntries);
	Max.Entries.Add(Base.Entries[1]);
	TestFalse(TEXT("limit plus one rejected"), Max.IsValid());
	FPlan Sections = Base;
	Sections.Entries[1].SlotIndex = Sections.Entries[0].SlotIndex;
	TestTrue(TEXT("same slot in different sections allowed"), Sections.IsValid());
	FPlan Legacy = Base;
	Legacy.bLegacyCompatibilityView = true;
	Legacy.ProjectionId = NAME_None;
	Legacy.DistributionProfileId = NAME_None;
	TestTrue(TEXT("resolved compatibility plan needs no projection policy"), Legacy.IsValid());
	Legacy.bLegacyCompatibilityView = false;
	TestFalse(TEXT("current projection requires policy identity"), Legacy.IsValid());
	return true;
}

#endif
