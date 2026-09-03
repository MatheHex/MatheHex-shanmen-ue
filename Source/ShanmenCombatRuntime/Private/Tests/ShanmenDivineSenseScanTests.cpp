#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDivineSenseScan.h"

#include <limits>

namespace
{
	const FGuid DivineSenseRunId(0x59100001, 0, 0, 1);
	const FGuid DivineSenseOwnerId(0x59100002, 0, 0, 1);
	const FGuid DivineSenseSourceId(0x59100003, 0, 0, 1);
	const FGuid SubjectA(0x59100004, 0, 0, 1);
	const FGuid SubjectB(0x59100005, 0, 0, 1);
	const FGuid SubjectC(0x59100006, 0, 0, 1);
	const FGuid SubjectD(0x59100007, 0, 0, 1);
	const FGuid SubjectE(0x59100008, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeDivineSenseAction(
		uint64 ActivationSequence = 1,
		FName ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId())
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = DivineSenseRunId;
		Capture.OwnerId = DivineSenseOwnerId;
		Capture.SourceEntityId = DivineSenseSourceId;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content.Version = TEXT("0.0.10.P19.0");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P19.0");
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenDivineSenseDefinitionCapture MakeDefinitionCapture(
		EShanmenDivineSenseOcclusionPolicy OcclusionPolicy =
			EShanmenDivineSenseOcclusionPolicy::VisibleOnly,
		double Radius = 100.0,
		int32 MaximumResults = 4)
	{
		FShanmenDivineSenseDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.ScanRuleId = TEXT("Spell.DivineSense.Pulse.Basic01");
		Capture.Radius = Radius;
		Capture.MaximumResults = MaximumResults;
		Capture.OcclusionPolicy = OcclusionPolicy;
		Capture.RequiredSubjectTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.BlockedSubjectTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.bRejectSelf = true;
		return Capture;
	}

	FShanmenDivineSenseDefinition MakeDefinition(
		EShanmenDivineSenseOcclusionPolicy OcclusionPolicy =
			EShanmenDivineSenseOcclusionPolicy::VisibleOnly,
		double Radius = 100.0,
		int32 MaximumResults = 4)
	{
		FShanmenDivineSenseDefinition Definition;
		check(FShanmenDivineSenseDefinition::TryCapture(
			MakeDefinitionCapture(
				OcclusionPolicy, Radius, MaximumResults),
			Definition));
		return Definition;
	}

	FShanmenDivineSenseScanRequest MakeRequest(
		const FShanmenDivineSenseDefinition& Definition,
		int32 ScanOrdinal = 0,
		const FVector& Origin = FVector::ZeroVector,
		uint64 ActivationSequence = 1)
	{
		FShanmenDivineSenseScanRequest Request;
		check(FShanmenDivineSenseScanRequest::TryCapture(
			MakeDivineSenseAction(ActivationSequence),
			Definition,
			Origin,
			ScanOrdinal,
			Request));
		return Request;
	}

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

	FShanmenDivineSenseObservation MakeObservation(
		const FShanmenDivineSenseScanRequest& Request,
		const FGuid& SubjectId,
		const FVector& Location,
		bool bHasLineOfSight,
		int64 AuthorityRevision = 1,
		const FGameplayTagContainer& Tags = LivingTags())
	{
		FShanmenDivineSenseObservationCapture Capture;
		Capture.SubjectEntityId = SubjectId;
		Capture.WorldLocation = Location;
		Capture.SubjectTags = Tags;
		Capture.bHasLineOfSight = bHasLineOfSight;
		Capture.AuthorityRevision = AuthorityRevision;

		FShanmenDivineSenseObservation Observation;
		check(FShanmenDivineSenseObservation::TryCapture(
			Request, Capture, Observation));
		return Observation;
	}

	bool ContainsSubject(
		const FShanmenDivineSenseScanReceipt& Receipt,
		const FGuid& SubjectId)
	{
		for (const FShanmenDivineSenseReveal& Reveal :
			Receipt.GetReveals())
		{
			if (Reveal.GetObservation().GetSubjectEntityId() == SubjectId)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenDivineSenseDefinitionAndRequestTest,
	"Shanmen.0_0_10.CombatRuntime.DivineSense.DefinitionAndRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenDivineSenseDefinitionAndRequestTest::RunTest(const FString&)
{
	FShanmenDivineSenseDefinition Definition;
	TestTrue(TEXT("Authored pulse definition captures immutably"),
		FShanmenDivineSenseDefinition::TryCapture(
			MakeDefinitionCapture(), Definition));
	TestTrue(TEXT("Definition preserves policy without freezing final balance"),
		Definition.IsValid()
			&& Definition.GetDefinitionId().IsValid()
			&& FMath::IsNearlyEqual(Definition.GetRadius(), 100.0)
			&& Definition.GetMaximumResults() == 4
			&& Definition.GetOcclusionPolicy()
				== EShanmenDivineSenseOcclusionPolicy::VisibleOnly
			&& Definition.GetRequiredSubjectTags().HasTagExact(
				FShanmenCombatNativeTags::TargetLiving()));

	FShanmenDivineSenseDefinition ReplayDefinition;
	TestTrue(TEXT("Equivalent authored values replay one definition identity"),
		FShanmenDivineSenseDefinition::TryCapture(
			MakeDefinitionCapture(), ReplayDefinition)
			&& ReplayDefinition.GetDefinitionId()
				== Definition.GetDefinitionId());

	FShanmenDivineSenseDefinitionCapture Invalid =
		MakeDefinitionCapture();
	Invalid.ActionDefinitionId = TEXT("Combat.Action.Projectile.Generic");
	TestFalse(TEXT("Unrelated action cannot masquerade as Divine Sense"),
		FShanmenDivineSenseDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.ScanRuleId = NAME_None;
	TestFalse(TEXT("Definition requires an authored scan rule"),
		FShanmenDivineSenseDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.Radius = 0.0;
	TestFalse(TEXT("Non-positive radius fails closed"),
		FShanmenDivineSenseDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.Radius = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("Non-finite radius fails closed"),
		FShanmenDivineSenseDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.MaximumResults = 0;
	TestFalse(TEXT("A scan must allow at least one result"),
		FShanmenDivineSenseDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.BlockedSubjectTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	TestFalse(TEXT("Conflicting subject filters fail closed"),
		FShanmenDivineSenseDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.OcclusionPolicy =
		static_cast<EShanmenDivineSenseOcclusionPolicy>(255);
	TestFalse(TEXT("Unknown occlusion policy fails closed"),
		FShanmenDivineSenseDefinition::TryCapture(Invalid, Definition));
	TestFalse(TEXT("Default definition is not implicit content"),
		FShanmenDivineSenseDefinition().IsValid());

	Definition = MakeDefinition();
	FShanmenDivineSenseScanRequest Request;
	TestTrue(TEXT("Valid action freezes one logical scan"),
		FShanmenDivineSenseScanRequest::TryCapture(
			MakeDivineSenseAction(),
			Definition,
			FVector(-0.0, 20.0, 5.0),
			0,
			Request));
	TestTrue(TEXT("Origin zero is canonicalized and request is valid"),
		Request.IsValid()
			&& Request.GetOrigin().X == 0.0);
	const FShanmenDivineSenseScanRequest Replay = MakeRequest(
		Definition, 0, FVector(0.0, 20.0, 5.0));
	TestEqual(TEXT("Equivalent scan request replays one identity"),
		Request.GetScanId(), Replay.GetScanId());
	const FShanmenDivineSenseScanRequest Next = MakeRequest(
		Definition, 1, FVector(0.0, 20.0, 5.0));
	TestNotEqual(TEXT("Scan ordinal separates repeated pulses"),
		Request.GetScanId(), Next.GetScanId());

	TestFalse(TEXT("Wrong action definition cannot enter scan contract"),
		FShanmenDivineSenseScanRequest::TryCapture(
			MakeDivineSenseAction(1, TEXT("Combat.Action.Sword.Basic01")),
			Definition,
			FVector::ZeroVector,
			0,
			Request));
	TestFalse(TEXT("Negative ordinal fails closed"),
		FShanmenDivineSenseScanRequest::TryCapture(
			MakeDivineSenseAction(),
			Definition,
			FVector::ZeroVector,
			-1,
			Request));
	TestFalse(TEXT("Non-finite origin fails closed"),
		FShanmenDivineSenseScanRequest::TryCapture(
			MakeDivineSenseAction(),
			Definition,
			FVector(std::numeric_limits<double>::infinity(), 0.0, 0.0),
			0,
			Request));
	TestFalse(TEXT("Rejected request clears caller output"),
		Request.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenDivineSenseObservationEvidenceTest,
	"Shanmen.0_0_10.CombatRuntime.DivineSense.ObservationEvidence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenDivineSenseObservationEvidenceTest::RunTest(const FString&)
{
	const FShanmenDivineSenseScanRequest Request =
		MakeRequest(MakeDefinition());
	FShanmenDivineSenseObservationCapture Capture;
	Capture.SubjectEntityId = SubjectA;
	Capture.WorldLocation = FVector(25.0, -0.0, 2.0);
	Capture.SubjectTags = LivingTags();
	Capture.bHasLineOfSight = true;
	Capture.AuthorityRevision = 7;

	FShanmenDivineSenseObservation Observation;
	TestTrue(TEXT("World evidence binds to one exact scan"),
		FShanmenDivineSenseObservation::TryCapture(
			Request, Capture, Observation));
	TestTrue(TEXT("Observation freezes identity, tags, visibility, and revision"),
		Observation.IsValid()
			&& Observation.GetScanId() == Request.GetScanId()
			&& Observation.GetSubjectEntityId() == SubjectA
			&& Observation.GetSubjectTags().HasTagExact(
				FShanmenCombatNativeTags::TargetLiving())
			&& Observation.HasLineOfSight()
			&& Observation.GetAuthorityRevision() == 7
			&& Observation.GetWorldLocation().Y == 0.0);

	FShanmenDivineSenseObservation Replay;
	TestTrue(TEXT("Equivalent evidence replays one observation identity"),
		FShanmenDivineSenseObservation::TryCapture(
			Request, Capture, Replay)
			&& Replay.GetObservationId()
				== Observation.GetObservationId());
	Capture.AuthorityRevision = 8;
	TestTrue(TEXT("Authority revision participates in evidence identity"),
		FShanmenDivineSenseObservation::TryCapture(
			Request, Capture, Replay)
			&& Replay.GetObservationId()
				!= Observation.GetObservationId());
	Capture.AuthorityRevision = 7;
	Capture.bHasLineOfSight = false;
	TestTrue(TEXT("Occlusion evidence participates in identity"),
		FShanmenDivineSenseObservation::TryCapture(
			Request, Capture, Replay)
			&& Replay.GetObservationId()
				!= Observation.GetObservationId());

	Capture = FShanmenDivineSenseObservationCapture();
	Capture.WorldLocation = FVector::ZeroVector;
	Capture.SubjectTags = LivingTags();
	TestFalse(TEXT("Missing subject identity fails closed"),
		FShanmenDivineSenseObservation::TryCapture(
			Request, Capture, Observation));
	Capture.SubjectEntityId = SubjectA;
	Capture.SubjectTags.Reset();
	TestFalse(TEXT("Unclassified subject evidence fails closed"),
		FShanmenDivineSenseObservation::TryCapture(
			Request, Capture, Observation));
	Capture.SubjectTags = LivingTags();
	Capture.AuthorityRevision = -1;
	TestFalse(TEXT("Negative authority revision fails closed"),
		FShanmenDivineSenseObservation::TryCapture(
			Request, Capture, Observation));
	Capture.AuthorityRevision = 0;
	Capture.WorldLocation = FVector(
		std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0);
	TestFalse(TEXT("Non-finite location fails closed"),
		FShanmenDivineSenseObservation::TryCapture(
			Request, Capture, Observation));
	TestFalse(TEXT("Rejected evidence clears caller output"),
		Observation.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenDivineSenseFilteringAndVisibilityTest,
	"Shanmen.0_0_10.CombatRuntime.DivineSense.FilteringAndVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenDivineSenseFilteringAndVisibilityTest::RunTest(
	const FString&)
{
	const FShanmenDivineSenseScanRequest Request =
		MakeRequest(MakeDefinition());
	FGameplayTagContainer NonLivingTags;
	NonLivingTags.AddTag(FShanmenCombatNativeTags::DamageSpirit());

	const FShanmenDivineSenseObservation NearVisible = MakeObservation(
		Request, SubjectA, FVector(10.0, 0.0, 0.0), true);
	const FShanmenDivineSenseObservation EdgeVisible = MakeObservation(
		Request, SubjectB, FVector(100.0, 0.0, 0.0), true);
	const TArray<FShanmenDivineSenseObservation> Observations = {
		MakeObservation(
			Request,
			DivineSenseSourceId,
			FVector(5.0, 0.0, 0.0),
			true),
		NearVisible,
		EdgeVisible,
		MakeObservation(
			Request, SubjectC, FVector(101.0, 0.0, 0.0), true),
		MakeObservation(
			Request,
			SubjectD,
			FVector(20.0, 0.0, 0.0),
			true,
			1,
			LivingTags(true)),
		MakeObservation(
			Request,
			SubjectE,
			FVector(30.0, 0.0, 0.0),
			false),
		MakeObservation(
			Request,
			FGuid(0x59100009, 0, 0, 1),
			FVector(15.0, 0.0, 0.0),
			true,
			1,
			NonLivingTags)
	};

	FShanmenDivineSenseScanReceipt Receipt;
	TestTrue(TEXT("Visible-only pulse resolves sampled evidence"),
		FShanmenDivineSenseResolver::TryResolve(
			Request, Observations, Receipt));
	TestTrue(TEXT("Only allowed, visible, in-range subjects are exposed"),
		Receipt.IsValid()
			&& Receipt.NumReveals() == 2
			&& ContainsSubject(Receipt, SubjectA)
			&& ContainsSubject(Receipt, SubjectB)
			&& !ContainsSubject(Receipt, DivineSenseSourceId)
			&& !ContainsSubject(Receipt, SubjectC)
			&& !ContainsSubject(Receipt, SubjectD)
			&& !ContainsSubject(Receipt, SubjectE));
	TestTrue(TEXT("Range boundary is inclusive and results are distance ordered"),
		Receipt.GetReveals()[0].GetObservation().GetSubjectEntityId()
			== SubjectA
			&& FMath::IsNearlyEqual(
				Receipt.GetReveals()[0].GetDistanceSquared(), 100.0)
			&& Receipt.GetReveals()[1].GetObservation().GetSubjectEntityId()
				== SubjectB
			&& FMath::IsNearlyEqual(
				Receipt.GetReveals()[1].GetDistanceSquared(), 10000.0)
			&& !Receipt.GetReveals()[0].WasOccluded());

	FShanmenDivineSenseScanReceipt Empty;
	TestTrue(TEXT("No observed subjects is a valid empty scan result"),
		FShanmenDivineSenseResolver::TryResolve(Request, {}, Empty)
			&& Empty.IsValid()
			&& Empty.NumReveals() == 0
			&& Empty.GetReceiptId().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenDivineSenseDeterminismAndAmbiguityTest,
	"Shanmen.0_0_10.CombatRuntime.DivineSense.DeterminismAndAmbiguity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenDivineSenseDeterminismAndAmbiguityTest::RunTest(
	const FString&)
{
	const FShanmenDivineSenseScanRequest Request = MakeRequest(
		MakeDefinition(
			EShanmenDivineSenseOcclusionPolicy::RevealOccluded,
			100.0,
			2));
	const FShanmenDivineSenseObservation Far = MakeObservation(
		Request, SubjectA, FVector(30.0, 0.0, 0.0), true);
	const FShanmenDivineSenseObservation NearOccluded = MakeObservation(
		Request, SubjectB, FVector(10.0, 0.0, 0.0), false);
	const FShanmenDivineSenseObservation Middle = MakeObservation(
		Request, SubjectC, FVector(20.0, 0.0, 0.0), true);

	FShanmenDivineSenseScanReceipt First;
	TestTrue(TEXT("Reveal-occluded policy admits hidden sampled evidence"),
		FShanmenDivineSenseResolver::TryResolve(
			Request, { Far, NearOccluded, Middle }, First));
	TestTrue(TEXT("Capacity keeps the nearest deterministic subjects"),
		First.IsValid()
			&& First.NumReveals() == 2
			&& First.GetReveals()[0].GetObservation().GetSubjectEntityId()
				== SubjectB
			&& First.GetReveals()[0].WasOccluded()
			&& First.GetReveals()[1].GetObservation().GetSubjectEntityId()
				== SubjectC
			&& !ContainsSubject(First, SubjectA));

	FShanmenDivineSenseScanReceipt Permuted;
	TestTrue(TEXT("Candidate order cannot change consumer result identity"),
		FShanmenDivineSenseResolver::TryResolve(
			Request, { Middle, Far, NearOccluded }, Permuted)
			&& Permuted.GetReceiptId() == First.GetReceiptId()
			&& Permuted.GetReveals()[0].GetRevealId()
				== First.GetReveals()[0].GetRevealId()
			&& Permuted.GetReveals()[1].GetRevealId()
				== First.GetReveals()[1].GetRevealId());

	FShanmenDivineSenseScanReceipt Deduplicated;
	TestTrue(TEXT("Exact duplicate collider evidence is idempotently deduplicated"),
		FShanmenDivineSenseResolver::TryResolve(
			Request,
			{ NearOccluded, Middle, NearOccluded, Far },
			Deduplicated)
			&& Deduplicated.GetReceiptId() == First.GetReceiptId());

	const FShanmenDivineSenseObservation Conflicting = MakeObservation(
		Request,
		SubjectB,
		FVector(10.0, 0.0, 0.0),
		false,
		2);
	TestFalse(TEXT("Conflicting evidence for one subject fails closed"),
		FShanmenDivineSenseResolver::TryResolve(
			Request, { NearOccluded, Conflicting }, Deduplicated));
	TestFalse(TEXT("Ambiguous resolution clears caller output"),
		Deduplicated.IsValid());

	const FShanmenDivineSenseScanRequest OtherRequest = MakeRequest(
		MakeDefinition(
			EShanmenDivineSenseOcclusionPolicy::RevealOccluded,
			100.0,
			2),
		1);
	const FShanmenDivineSenseObservation Foreign = MakeObservation(
		OtherRequest, SubjectD, FVector(5.0, 0.0, 0.0), true);
	TestFalse(TEXT("Observation from another scan cannot be mixed in"),
		FShanmenDivineSenseResolver::TryResolve(
			Request, { NearOccluded, Foreign }, Deduplicated));
	TestFalse(TEXT("Foreign evidence rejection leaves no receipt"),
		Deduplicated.IsValid());
	return true;
}

#endif
