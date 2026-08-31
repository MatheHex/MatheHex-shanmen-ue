#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCue.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmEffectCueFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid CueRun(
		0xA6F60001, 0xA6F60002, 0xA6F60003, 0xA6F60004);
	const FGuid CueWeapon(
		0xA6F70001, 0xA6F70002, 0xA6F70003, 0xA6F70004);

	Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture MakeBindingCapture(
		const FName EffectDefinitionId,
		const FName VisualCueDefinitionId,
		const FName AudioCueDefinitionId)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture Capture;
		Capture.EffectDefinitionId = EffectDefinitionId;
		Capture.VisualCueDefinitionId = VisualCueDefinitionId;
		Capture.AudioCueDefinitionId = AudioCueDefinitionId;
		return Capture;
	}

	Fdemo_mapShanmenSwordRhythmEffectCuePolicyCapture MakeCanonicalCapture(
		const FShanmenContentStamp& Content)
	{
		Fdemo_mapShanmenSwordRhythmEffectCuePolicyCapture Capture;
		Capture.PolicyDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalEffectCuePolicyDefinitionId();
		Capture.Content = Content;
		Capture.Bindings = {
			MakeBindingCapture(
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPreciseLinkEffectDefinitionId(),
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPreciseLinkVisualCueDefinitionId(),
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPreciseLinkAudioCueDefinitionId()),
			MakeBindingCapture(
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPerfectGuardEffectDefinitionId(),
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPerfectGuardVisualCueDefinitionId(),
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPerfectGuardAudioCueDefinitionId()),
			MakeBindingCapture(
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalSpiritEvasionEffectDefinitionId(),
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalSpiritEvasionVisualCueDefinitionId(),
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalSpiritEvasionAudioCueDefinitionId())
		};
		return Capture;
	}

	struct FSwordRhythmEffectCueFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmEffectCueFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn,
					TEXT("EffectCuePlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("EffectCuePlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(
					CueRun,
					Pawn,
					Health,
					Diagnostic)
				&& Timeline.TryBegin(CueRun, Diagnostic)
				&& Session.TryBegin(CueRun, Diagnostic);
		}

		bool TryAdvanceTicks(const int64 RequestedTicks)
		{
			int64 AdvancedTicks = 0;
			const double Seconds = static_cast<double>(RequestedTicks)
				/ static_cast<double>(
					Fdemo_mapShanmenCombatRunFixedTimeline::
						CanonicalTicksPerSecond());
			return Timeline.TryAdvance(Seconds, AdvancedTicks, Diagnostic)
				&& AdvancedTicks == RequestedTicks;
		}

		bool TryExecute(
			Fdemo_mapBasicSwordProductExecutionResult& OutAction,
			FShanmenSwordRhythmReceipt& OutReceipt,
			Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent)
		{
			OutAction = Coordinator.ExecutePlayerBasicSwordSweep(
				CueWeapon,
				1.0f,
				{});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			if (!OutAction.IsExecuted()
				|| !Timeline.TryCapture(Sample)
				|| !Session.TryObserveExecutedBasicSword(
					OutAction,
					Sample,
					OutReceipt,
					Diagnostic))
			{
				return false;
			}
			const auto Adapted =
				Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
					Session.GetPresentationState(),
					Session.GetConfig().GetEffectCuePolicy());
			if (!Adapted.IsAdapted())
			{
				Diagnostic = Adapted.Diagnostic;
				return false;
			}
			OutEvent = Adapted.Event;
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmEffectCuePolicyTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCue.PolicyContract",
	SwordRhythmEffectCueFlags)

bool Fdemo_mapSwordRhythmEffectCuePolicyTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSwordRhythmProductConfig Config;
	TestTrue(TEXT("canonical product config captures"),
		Fdemo_mapShanmenSwordRhythmProductConfig::TryCreateCanonical(Config));
	if (!Config.IsValid())
	{
		return false;
	}

	const auto& Policy = Config.GetEffectCuePolicy();
	Fdemo_mapShanmenSwordRhythmEffectCueBinding Precise;
	Fdemo_mapShanmenSwordRhythmEffectCueBinding Guard;
	Fdemo_mapShanmenSwordRhythmEffectCueBinding Evasion;
	TestTrue(TEXT("canonical policy covers all three symbolic effects"),
		Policy.IsValid()
			&& Policy.GetBindings().Num() == 3
			&& Policy.TryFindBinding(
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPreciseLinkEffectDefinitionId(),
				Precise)
			&& Policy.TryFindBinding(
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPerfectGuardEffectDefinitionId(),
				Guard)
			&& Policy.TryFindBinding(
				Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalSpiritEvasionEffectDefinitionId(),
				Evasion));
	TestTrue(TEXT("each effect has typed visual and audio authored identities"),
		Precise.GetVisualCueDefinitionId()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPreciseLinkVisualCueDefinitionId()
			&& Precise.GetAudioCueDefinitionId()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPreciseLinkAudioCueDefinitionId()
			&& Guard.GetVisualCueDefinitionId()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPerfectGuardVisualCueDefinitionId()
			&& Guard.GetAudioCueDefinitionId()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPerfectGuardAudioCueDefinitionId()
			&& Evasion.GetVisualCueDefinitionId()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalSpiritEvasionVisualCueDefinitionId()
			&& Evasion.GetAudioCueDefinitionId()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalSpiritEvasionAudioCueDefinitionId());

	auto ForwardCapture = MakeCanonicalCapture(Config.GetContent());
	auto ReverseCapture = ForwardCapture;
	Algo::Reverse(ReverseCapture.Bindings);
	Fdemo_mapShanmenSwordRhythmEffectCuePolicy Forward;
	Fdemo_mapShanmenSwordRhythmEffectCuePolicy Reverse;
	TestTrue(TEXT("authored binding order does not change policy identity"),
		Fdemo_mapShanmenSwordRhythmEffectCuePolicy::TryCapture(
			ForwardCapture, Forward)
			&& Fdemo_mapShanmenSwordRhythmEffectCuePolicy::TryCapture(
				ReverseCapture, Reverse)
			&& Forward.GetPolicyId() == Reverse.GetPolicyId()
			&& Forward.GetPolicyId() == Policy.GetPolicyId());

	auto Duplicate = ForwardCapture;
	Duplicate.Bindings[1].EffectDefinitionId =
		Duplicate.Bindings[0].EffectDefinitionId;
	Fdemo_mapShanmenSwordRhythmEffectCuePolicy Rejected;
	TestFalse(TEXT("duplicate effect mappings fail closed"),
		Fdemo_mapShanmenSwordRhythmEffectCuePolicy::TryCapture(
			Duplicate, Rejected));
	auto NoCue = ForwardCapture;
	NoCue.Bindings[0].VisualCueDefinitionId = NAME_None;
	NoCue.Bindings[0].AudioCueDefinitionId = NAME_None;
	TestFalse(TEXT("binding without any presentation channel fails closed"),
		Fdemo_mapShanmenSwordRhythmEffectCuePolicy::TryCapture(
			NoCue, Rejected));
	TestFalse(TEXT("failed capture clears its output"), Rejected.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmEffectCueEmptyEventTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCue.EmptyAndPolling",
	SwordRhythmEffectCueFlags)

bool Fdemo_mapSwordRhythmEffectCueEmptyEventTest::RunTest(const FString&)
{
	FSwordRhythmEffectCueFixture Fixture;
	TestTrue(TEXT("real cue fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	Fdemo_mapBasicSwordProductExecutionResult Action;
	FShanmenSwordRhythmReceipt Receipt;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	TestTrue(TEXT("legal action without contributions creates a valid empty event"),
		Fixture.TryExecute(Action, Receipt, Event)
			&& Action.IsExecuted()
			&& !Action.AppliedDamage()
			&& Receipt.GetBand() == EShanmenSwordRhythmBand::Started
			&& Event.IsValid()
			&& Event.NumCommands() == 0
			&& Event.GetState().NumEffectDefinitions() == 0);
	const auto Repeated =
		Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
			Fixture.Session.GetPresentationState(),
			Fixture.Session.GetConfig().GetEffectCuePolicy());
	TestTrue(TEXT("polling identical state preserves empty event identity"),
		Repeated.IsAdapted()
			&& Event.Matches(Repeated.Event)
			&& Repeated.Event.NumCommands() == 0);

	const auto InvalidState =
		Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
			Fdemo_mapShanmenSwordRhythmPresentationState(),
			Fixture.Session.GetConfig().GetEffectCuePolicy());
	const auto InvalidPolicy =
		Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
			Fixture.Session.GetPresentationState(),
			Fdemo_mapShanmenSwordRhythmEffectCuePolicy());
	TestTrue(TEXT("invalid state and policy report distinct typed failures"),
		!InvalidState.IsAdapted()
			&& InvalidState.Status
				== Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::
					StateInvalid
			&& !InvalidPolicy.IsAdapted()
			&& InvalidPolicy.Status
				== Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::
					PolicyInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmEffectCuePreciseFlowTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCue.PreciseFlowCommands",
	SwordRhythmEffectCueFlags)

bool Fdemo_mapSwordRhythmEffectCuePreciseFlowTest::RunTest(const FString&)
{
	FSwordRhythmEffectCueFixture Fixture;
	TestTrue(TEXT("real precise-flow fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	Fdemo_mapBasicSwordProductExecutionResult Action;
	FShanmenSwordRhythmReceipt Receipt;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	TestTrue(TEXT("first action starts an empty effect route"),
		Fixture.TryExecute(Action, Receipt, Event));
	TestTrue(TEXT("second action reaches the precise-link boundary"),
		Fixture.TryAdvanceTicks(
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalLinkOpenOffsetTicks())
			&& Fixture.TryExecute(Action, Receipt, Event)
			&& Receipt.GetBand() == EShanmenSwordRhythmBand::PreciseLinked
			&& Event.NumCommands() == 0);
	TestTrue(TEXT("next action consumes precise evidence into two typed commands"),
		Fixture.TryExecute(Action, Receipt, Event)
			&& Event.IsValid()
			&& Event.GetState().NumEffectDefinitions() == 1
			&& Event.NumCommands() == 2);
	if (Event.NumCommands() != 2)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	const auto& Visual = Event.GetCommands()[0];
	const auto& Audio = Event.GetCommands()[1];
	TestTrue(TEXT("command order is effect ordinal then Visual and Audio"),
		Visual.GetEffectOrdinal() == 0
			&& Audio.GetEffectOrdinal() == 0
			&& Visual.GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
			&& Audio.GetChannel()
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio
			&& Visual.GetBinding().GetEffectDefinitionId()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPreciseLinkEffectDefinitionId()
			&& Audio.GetBinding().GetBindingId()
				== Visual.GetBinding().GetBindingId()
			&& Visual.GetCueDefinitionId()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPreciseLinkVisualCueDefinitionId()
			&& Audio.GetCueDefinitionId()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalPreciseLinkAudioCueDefinitionId()
			&& Visual.GetCommandId() != Audio.GetCommandId());

	Fdemo_mapShanmenSwordRhythmEffectCuePolicyCapture IncompleteCapture;
	IncompleteCapture.PolicyDefinitionId = TEXT("Test.IncompleteCuePolicy.r1");
	IncompleteCapture.Content = Fixture.Session.GetConfig().GetContent();
	IncompleteCapture.Bindings = {
		MakeBindingCapture(
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalPerfectGuardEffectDefinitionId(),
			TEXT("Test.Visual.Guard"),
			TEXT("Test.Audio.Guard"))
	};
	Fdemo_mapShanmenSwordRhythmEffectCuePolicy IncompletePolicy;
	TestTrue(TEXT("a structurally valid partial vocabulary can be authored"),
		Fdemo_mapShanmenSwordRhythmEffectCuePolicy::TryCapture(
			IncompleteCapture, IncompletePolicy));
	const auto Unmapped =
		Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
			Fixture.Session.GetPresentationState(),
			IncompletePolicy);
	TestTrue(TEXT("product state outside that vocabulary fails as EffectUnmapped"),
		!Unmapped.IsAdapted()
			&& Unmapped.Status
				== Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::
					EffectUnmapped
			&& !Unmapped.Event.IsValid());

	const Fdemo_mapShanmenSwordRhythmEffectCueEvent StableCopy = Event;
	TestTrue(TEXT("Run teardown invalidates source but not immutable event copy"),
		Fixture.Session.TryEnd(CueRun, Fixture.Diagnostic)
			&& Fixture.Session.IsEmpty()
			&& StableCopy.IsValid()
			&& !Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
				Fixture.Session.GetPresentationState(),
				Fixture.Session.GetConfig().GetEffectCuePolicy()).IsAdapted());
	return true;
}

#endif
