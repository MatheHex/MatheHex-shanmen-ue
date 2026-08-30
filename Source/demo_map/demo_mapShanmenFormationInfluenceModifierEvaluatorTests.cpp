#if WITH_DEV_AUTOMATION_TESTS

#include "demo_mapShanmenFormationInfluenceModifierEvaluator.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"

namespace
{
	using FSpecification =
		Fdemo_mapShanmenFormationInfluenceModifierSpecification;
	using FContext = Fdemo_mapShanmenFormationInfluenceEvaluationContext;
	using FDecision =
		Fdemo_mapShanmenFormationInfluenceModifierDecisionRecord;
	using EDecision = Edemo_mapShanmenFormationInfluenceModifierDecision;
	using EStackPolicy = Edemo_mapShanmenFormationInfluenceStackPolicy;
	using EStatus = Edemo_mapShanmenFormationInfluenceEvaluationStatus;

	FShanmenContentStamp MakeContent(const TCHAR* Digest = TEXT("P8.23-CONTENT"))
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.23");
		Content.Digest = Digest;
		return Content;
	}

	Fdemo_mapShanmenFormationInfluencePolicy MakePolicy(
		const FShanmenContentStamp& Content = MakeContent())
	{
		Fdemo_mapShanmenFormationInfluencePolicy Policy;
		Policy.PolicyDefinitionId = TEXT("Formation.Policy.TestAura");
		Policy.InfluenceDefinitionId = TEXT("Formation.Influence.TestAura");
		Policy.Content = Content;
		check(Policy.IsValid());
		return Policy;
	}

	FGameplayTagContainer MakeTags(
		const TArray<FGameplayTag>& Tags)
	{
		FGameplayTagContainer Result;
		for (const FGameplayTag& Tag : Tags)
		{
			Result.AddTag(Tag);
		}
		return Result;
	}

	FContext MakeContext(
		const FGameplayTag& Channel =
			FShanmenCombatNativeTags::InfluenceOffensePower())
	{
		FContext Context;
		Context.RunId = FGuid(1, 2, 3, 4);
		Context.SubjectEntityId = FGuid(5, 6, 7, 8);
		Context.Channel = Channel;
		Context.SubjectTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		Context.Content = MakeContent();
		check(Context.IsValid());
		return Context;
	}

	FSpecification MakeSpecification(
		const FName ModifierDefinitionId,
		const FGameplayTag& Channel,
		const int32 MagnitudeUnits,
		const FName StackGroupId,
		const EStackPolicy StackPolicy,
		const int32 Priority = 0,
		const FGameplayTagContainer& Required = FGameplayTagContainer(),
		const FGameplayTagContainer& Blocked = FGameplayTagContainer(),
		const Fdemo_mapShanmenFormationInfluencePolicy& Policy = MakePolicy())
	{
		FSpecification Specification;
		check(FSpecification::TryCreate(
			Policy, ModifierDefinitionId, Channel, Required, Blocked,
			MagnitudeUnits, StackGroupId, StackPolicy, Priority,
			Specification));
		return Specification;
	}

	const FDecision* FindDecision(
		const Fdemo_mapShanmenFormationInfluenceEvaluationReceipt& Receipt,
		const FName ModifierDefinitionId)
	{
		return Receipt.Decisions.FindByPredicate(
			[ModifierDefinitionId](const FDecision& Decision)
			{
				return Decision.Specification.GetModifierDefinitionId()
					== ModifierDefinitionId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenFormationInfluenceModifierFilteringTest,
	"Shanmen.0_0_10.Product.FormationInfluenceModifierEvaluator.FilteringAndAdditive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenFormationInfluenceModifierFilteringTest::RunTest(
	const FString&)
{
	const FGameplayTag Offense =
		FShanmenCombatNativeTags::InfluenceOffensePower();
	const FGameplayTag Defense =
		FShanmenCombatNativeTags::InfluenceDefenseGuard();
	const FGameplayTagContainer Living = MakeTags(
		{ FShanmenCombatNativeTags::TargetLiving() });
	const FGameplayTagContainer Player = MakeTags(
		{ FShanmenCombatNativeTags::SourcePlayer() });

	const FSpecification Base = MakeSpecification(
		TEXT("Modifier.Power.Base"), Offense, 120,
		TEXT("Stack.Power.Additive"), EStackPolicy::Additive, 0, Living);
	const FSpecification Penalty = MakeSpecification(
		TEXT("Modifier.Power.Penalty"), Offense, -20,
		TEXT("Stack.Power.Additive"), EStackPolicy::Additive, 0, Living);
	const FSpecification OtherChannel = MakeSpecification(
		TEXT("Modifier.Guard.OtherChannel"), Defense, 500,
		TEXT("Stack.Guard"), EStackPolicy::Additive);
	const FSpecification MissingRequired = MakeSpecification(
		TEXT("Modifier.Power.PlayerOnly"), Offense, 40,
		TEXT("Stack.Power.Player"), EStackPolicy::Additive, 0, Player);
	const FSpecification Blocked = MakeSpecification(
		TEXT("Modifier.Power.NotLiving"), Offense, 60,
		TEXT("Stack.Power.Blocked"), EStackPolicy::Additive, 0,
		FGameplayTagContainer(), Living);

	const auto Result =
		Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
			MakeContext(),
			{ OtherChannel, MissingRequired, Base, Blocked, Penalty });
	TestTrue(TEXT("Tagged modifier set evaluates"), Result.IsSuccess());
	TestEqual(TEXT("Signed additive magnitude is exact"),
		Result.Receipt.FinalMagnitudeUnits, static_cast<int64>(100));
	TestEqual(TEXT("Exactly two modifiers contribute"),
		Result.Receipt.GetAppliedCount(), 2);
	TestEqual(TEXT("Every authored specification remains visible"),
		Result.Receipt.Decisions.Num(), 5);

	const FDecision* BaseDecision = FindDecision(
		Result.Receipt, TEXT("Modifier.Power.Base"));
	const FDecision* PenaltyDecision = FindDecision(
		Result.Receipt, TEXT("Modifier.Power.Penalty"));
	const FDecision* ChannelDecision = FindDecision(
		Result.Receipt, TEXT("Modifier.Guard.OtherChannel"));
	const FDecision* RequiredDecision = FindDecision(
		Result.Receipt, TEXT("Modifier.Power.PlayerOnly"));
	const FDecision* BlockedDecision = FindDecision(
		Result.Receipt, TEXT("Modifier.Power.NotLiving"));
	TestTrue(TEXT("Base decision exists"), BaseDecision != nullptr);
	TestTrue(TEXT("Penalty decision exists"), PenaltyDecision != nullptr);
	TestTrue(TEXT("Channel decision exists"), ChannelDecision != nullptr);
	TestTrue(TEXT("Required decision exists"), RequiredDecision != nullptr);
	TestTrue(TEXT("Blocked decision exists"), BlockedDecision != nullptr);
	if (BaseDecision && PenaltyDecision && ChannelDecision
		&& RequiredDecision && BlockedDecision)
	{
		TestTrue(TEXT("Base applies"),
			BaseDecision->Decision == EDecision::Applied);
		TestTrue(TEXT("Penalty applies"),
			PenaltyDecision->Decision == EDecision::Applied);
		TestTrue(TEXT("Other channel is explicit"),
			ChannelDecision->Decision == EDecision::ChannelMismatch);
		TestTrue(TEXT("Missing required tags are explicit"),
			RequiredDecision->Decision == EDecision::RequiredTagsMissing);
		TestTrue(TEXT("Blocked tags are explicit"),
			BlockedDecision->Decision == EDecision::BlockedByTags);
	}
	TestTrue(TEXT("Offense power retains parent influence hierarchy"),
		Offense.MatchesTag(FShanmenCombatNativeTags::InfluenceOffense())
		&& Offense.MatchesTag(FShanmenCombatNativeTags::Influence()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenFormationInfluenceModifierStackingTest,
	"Shanmen.0_0_10.Product.FormationInfluenceModifierEvaluator.StackingAndOrderIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenFormationInfluenceModifierStackingTest::RunTest(
	const FString&)
{
	const FGameplayTag Channel =
		FShanmenCombatNativeTags::InfluenceOffensePower();
	const FSpecification StrongSmall = MakeSpecification(
		TEXT("Modifier.Strong.Small"), Channel, 80,
		TEXT("Stack.Strong"), EStackPolicy::StrongestMagnitude);
	const FSpecification StrongLarge = MakeSpecification(
		TEXT("Modifier.Strong.Large"), Channel, -120,
		TEXT("Stack.Strong"), EStackPolicy::StrongestMagnitude, 1);
	const FSpecification StrongTie = MakeSpecification(
		TEXT("Modifier.Strong.Tie"), Channel, 120,
		TEXT("Stack.Strong"), EStackPolicy::StrongestMagnitude, 5);
	const FSpecification PriorityLow = MakeSpecification(
		TEXT("Modifier.Priority.Low"), Channel, 25,
		TEXT("Stack.Priority"), EStackPolicy::HighestPriority, 1);
	const FSpecification PriorityHigh = MakeSpecification(
		TEXT("Modifier.Priority.High"), Channel, 10,
		TEXT("Stack.Priority"), EStackPolicy::HighestPriority, 5);
	const FSpecification PriorityTie = MakeSpecification(
		TEXT("Modifier.Priority.Tie"), Channel, -30,
		TEXT("Stack.Priority"), EStackPolicy::HighestPriority, 5);

	const FContext Context = MakeContext();
	const auto Forward =
		Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
			Context,
			{ StrongSmall, StrongLarge, StrongTie,
				PriorityLow, PriorityHigh, PriorityTie });
	const auto Reordered =
		Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
			Context,
			{ PriorityTie, PriorityHigh, PriorityLow,
				StrongTie, StrongLarge, StrongSmall });
	TestTrue(TEXT("Forward stack evaluation succeeds"), Forward.IsSuccess());
	TestTrue(TEXT("Reordered stack evaluation succeeds"), Reordered.IsSuccess());
	TestTrue(TEXT("Input order cannot change canonical receipt"),
		Forward.Receipt.Matches(Reordered.Receipt));
	TestEqual(TEXT("Explicit tie-break winners sum exactly"),
		Forward.Receipt.FinalMagnitudeUnits, static_cast<int64>(90));
	TestEqual(TEXT("One winner per non-additive group"),
		Forward.Receipt.GetAppliedCount(), 2);

	const FDecision* SmallDecision = FindDecision(
		Forward.Receipt, TEXT("Modifier.Strong.Small"));
	const FDecision* LargeDecision = FindDecision(
		Forward.Receipt, TEXT("Modifier.Strong.Large"));
	const FDecision* StrongTieDecision = FindDecision(
		Forward.Receipt, TEXT("Modifier.Strong.Tie"));
	const FDecision* LowDecision = FindDecision(
		Forward.Receipt, TEXT("Modifier.Priority.Low"));
	const FDecision* HighDecision = FindDecision(
		Forward.Receipt, TEXT("Modifier.Priority.High"));
	const FDecision* PriorityTieDecision = FindDecision(
		Forward.Receipt, TEXT("Modifier.Priority.Tie"));
	if (SmallDecision && LargeDecision && StrongTieDecision
		&& LowDecision && HighDecision && PriorityTieDecision)
	{
		TestTrue(TEXT("Small magnitude is suppressed"),
			SmallDecision->Decision == EDecision::StackSuppressed
			&& SmallDecision->WinningSpecificationId
				== StrongTie.GetSpecificationId());
		TestTrue(TEXT("Equal absolute magnitude uses priority"),
			LargeDecision->Decision == EDecision::StackSuppressed
			&& StrongTieDecision->Decision == EDecision::Applied
			&& LargeDecision->WinningSpecificationId
				== StrongTie.GetSpecificationId());
		TestTrue(TEXT("Low priority is suppressed"),
			LowDecision->Decision == EDecision::StackSuppressed
			&& LowDecision->WinningSpecificationId
				== PriorityTie.GetSpecificationId());
		TestTrue(TEXT("Equal priority uses strongest absolute magnitude"),
			HighDecision->Decision == EDecision::StackSuppressed
			&& PriorityTieDecision->Decision == EDecision::Applied
			&& HighDecision->WinningSpecificationId
				== PriorityTie.GetSpecificationId());
	}
	else
	{
		AddError(TEXT("Expected all four canonical stack decisions."));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenFormationInfluenceModifierFailClosedTest,
	"Shanmen.0_0_10.Product.FormationInfluenceModifierEvaluator.FailClosedContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenFormationInfluenceModifierFailClosedTest::RunTest(
	const FString&)
{
	const FGameplayTag Channel =
		FShanmenCombatNativeTags::InfluenceOffensePower();
	const FSpecification Valid = MakeSpecification(
		TEXT("Modifier.Valid"), Channel, 10,
		TEXT("Stack.Valid"), EStackPolicy::Additive);

	FContext InvalidContext = MakeContext();
	InvalidContext.SubjectEntityId.Invalidate();
	auto Result =
		Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
			InvalidContext, { Valid });
	TestTrue(TEXT("Invalid context fails closed"),
		Result.Status == EStatus::ContextInvalid && !Result.IsSuccess());

	Result = Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
		MakeContext(), { FSpecification() });
	TestTrue(TEXT("Invalid specification fails closed"),
		Result.Status == EStatus::SpecificationInvalid);

	Result = Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
		MakeContext(), { Valid, Valid });
	TestTrue(TEXT("Duplicate authored identity fails closed"),
		Result.Status == EStatus::DuplicateSpecification);
	const FSpecification ConflictingDefinition = MakeSpecification(
		TEXT("Modifier.Valid"), Channel, 20,
		TEXT("Stack.Other"), EStackPolicy::Additive);
	Result = Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
		MakeContext(), { Valid, ConflictingDefinition });
	TestTrue(TEXT("One definition name cannot carry two payloads"),
		Result.Status == EStatus::DuplicateSpecification);

	const FSpecification ForeignContent = MakeSpecification(
		TEXT("Modifier.Foreign"), Channel, 10,
		TEXT("Stack.Foreign"), EStackPolicy::Additive, 0,
		FGameplayTagContainer(), FGameplayTagContainer(),
		MakePolicy(MakeContent(TEXT("FOREIGN-CONTENT"))));
	Result = Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
		MakeContext(), { ForeignContent });
	TestTrue(TEXT("Mixed content fails closed"),
		Result.Status == EStatus::ContentMismatch);

	const FSpecification Additive = MakeSpecification(
		TEXT("Modifier.Conflict.Add"), Channel, 10,
		TEXT("Stack.Conflict"), EStackPolicy::Additive);
	const FSpecification Strongest = MakeSpecification(
		TEXT("Modifier.Conflict.Strong"), Channel, 20,
		TEXT("Stack.Conflict"), EStackPolicy::StrongestMagnitude);
	Result = Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
		MakeContext(), { Additive, Strongest });
	TestTrue(TEXT("One stack domain cannot mix policies"),
		Result.Status == EStatus::StackPolicyConflict);

	FSpecification InvalidAuthored;
	const FGameplayTagContainer Living = MakeTags(
		{ FShanmenCombatNativeTags::TargetLiving() });
	TestFalse(TEXT("Contradictory exact filters are rejected at capture"),
		FSpecification::TryCreate(
			MakePolicy(), TEXT("Modifier.Contradictory"), Channel,
			Living, Living, 10, TEXT("Stack.Invalid"),
			EStackPolicy::Additive, 0, InvalidAuthored));
	TestTrue(TEXT("Rejected capture leaves default output"),
		!InvalidAuthored.GetSpecificationId().IsValid()
		&& InvalidAuthored.GetModifierDefinitionId().IsNone());
	TestFalse(TEXT("Zero magnitude is not an authored modifier"),
		FSpecification::TryCreate(
			MakePolicy(), TEXT("Modifier.Zero"), Channel,
			FGameplayTagContainer(), FGameplayTagContainer(), 0,
			TEXT("Stack.Invalid"), EStackPolicy::Additive, 0,
			InvalidAuthored));
	TestTrue(TEXT("Repeated rejected capture also leaves default output"),
		!InvalidAuthored.GetSpecificationId().IsValid()
		&& InvalidAuthored.GetModifierDefinitionId().IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenFormationInfluenceModifierReceiptIntegrityTest,
	"Shanmen.0_0_10.Product.FormationInfluenceModifierEvaluator.ReceiptIntegrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenFormationInfluenceModifierReceiptIntegrityTest::RunTest(
	const FString&)
{
	const FContext Context = MakeContext();
	const auto Empty =
		Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
			Context, {});
	TestTrue(TEXT("Empty authored set has a canonical zero receipt"),
		Empty.IsSuccess() && Empty.Receipt.Decisions.IsEmpty()
		&& Empty.Receipt.FinalMagnitudeUnits == 0);

	const FSpecification Valid = MakeSpecification(
		TEXT("Modifier.Integrity"), Context.Channel, 75,
		TEXT("Stack.Integrity"), EStackPolicy::Additive);
	const auto Evaluated =
		Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
			Context, { Valid });
	TestTrue(TEXT("Reference receipt is valid"), Evaluated.IsSuccess());
	TestTrue(TEXT("Specification is sealed to its source policy"),
		Valid.MatchesPolicy(MakePolicy()));

	auto TamperedMagnitude = Evaluated.Receipt;
	++TamperedMagnitude.FinalMagnitudeUnits;
	TestFalse(TEXT("Final magnitude tampering is detected"),
		TamperedMagnitude.IsValid());

	auto TamperedDecision = Evaluated.Receipt;
	TamperedDecision.Decisions[0].ContributionMagnitudeUnits = 0;
	TestFalse(TEXT("Decision contribution tampering is detected"),
		TamperedDecision.IsValid());

	auto TamperedContext = Evaluated.Receipt;
	TamperedContext.Context.SubjectTags.AddTag(
		FShanmenCombatNativeTags::SourcePlayer());
	TestFalse(TEXT("Context tampering invalidates receipt identity"),
		TamperedContext.IsValid());

	auto TamperedWinner = Evaluated.Receipt;
	TamperedWinner.Decisions[0].WinningSpecificationId = FGuid(9, 9, 9, 9);
	TestFalse(TEXT("Fabricated stack winner is detected"),
		TamperedWinner.IsValid());
	return true;
}

#endif
