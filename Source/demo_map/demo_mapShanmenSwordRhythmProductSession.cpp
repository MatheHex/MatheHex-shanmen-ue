#include "demo_mapShanmenSwordRhythmProductSession.h"

#include "ShanmenBasicSwordExecution.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool TryCreateCanonicalEvaluationPolicy(
		const FShanmenContentStamp& Content,
		FShanmenSwordRhythmEvaluationPolicy& OutPolicy)
	{
		FShanmenSwordRhythmEvaluationPolicyCapture Capture;
		Capture.PolicyDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalEvaluationPolicyDefinitionId();
		Capture.Content = Content;

		FShanmenSwordRhythmEffectSpecificationCapture PreciseLink;
		PreciseLink.ContributionKind =
			EShanmenSwordRhythmContributionKind::PreciseSwordLink;
		PreciseLink.EffectDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalPreciseLinkEffectDefinitionId();
		FShanmenSwordRhythmEffectSpecificationCapture PerfectGuard;
		PerfectGuard.ContributionKind =
			EShanmenSwordRhythmContributionKind::PerfectWeaponGuard;
		PerfectGuard.EffectDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalPerfectGuardEffectDefinitionId();
		FShanmenSwordRhythmEffectSpecificationCapture SpiritEvasion;
		SpiritEvasion.ContributionKind =
			EShanmenSwordRhythmContributionKind::SpiritEvasion;
		SpiritEvasion.EffectDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalSpiritEvasionEffectDefinitionId();
		Capture.Specifications = {
			PreciseLink,
			PerfectGuard,
			SpiritEvasion
		};
		return FShanmenSwordRhythmEvaluationPolicy::TryCapture(
			Capture,
			OutPolicy);
	}

	bool TryCreateCanonicalEffectCuePolicy(
		const FShanmenContentStamp& Content,
		Fdemo_mapShanmenSwordRhythmEffectCuePolicy& OutPolicy)
	{
		Fdemo_mapShanmenSwordRhythmEffectCuePolicyCapture Capture;
		Capture.PolicyDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalEffectCuePolicyDefinitionId();
		Capture.Content = Content;

		Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture PreciseLink;
		PreciseLink.EffectDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalPreciseLinkEffectDefinitionId();
		PreciseLink.VisualCueDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalPreciseLinkVisualCueDefinitionId();
		PreciseLink.AudioCueDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalPreciseLinkAudioCueDefinitionId();

		Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture PerfectGuard;
		PerfectGuard.EffectDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalPerfectGuardEffectDefinitionId();
		PerfectGuard.VisualCueDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalPerfectGuardVisualCueDefinitionId();
		PerfectGuard.AudioCueDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalPerfectGuardAudioCueDefinitionId();

		Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture SpiritEvasion;
		SpiritEvasion.EffectDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalSpiritEvasionEffectDefinitionId();
		SpiritEvasion.VisualCueDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalSpiritEvasionVisualCueDefinitionId();
		SpiritEvasion.AudioCueDefinitionId =
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalSpiritEvasionAudioCueDefinitionId();

		Capture.Bindings = {
			PreciseLink,
			PerfectGuard,
			SpiritEvasion
		};
		return Fdemo_mapShanmenSwordRhythmEffectCuePolicy::TryCapture(
			Capture,
			OutPolicy);
	}
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::CanonicalContentVersion()
{
	return TEXT("0.0.10.P12.11");
}

FString Fdemo_mapShanmenSwordRhythmProductConfig::CanonicalContentDigest()
{
	return TEXT("Shanmen.SwordRhythm.ProductConfig.r3.SymbolicEffectCues");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::CanonicalRuleId()
{
	return TEXT("Combat.Style.Sword.Taiji01.BasicLinkWindow.r1");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalEvaluationPolicyDefinitionId()
{
	return TEXT("Combat.Style.Sword.Taiji01.SymbolicEffects.r1");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalPreciseLinkEffectDefinitionId()
{
	return TEXT("Combat.Style.Sword.Taiji01.Effect.PreciseFlow");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalPerfectGuardEffectDefinitionId()
{
	return TEXT("Combat.Style.Sword.Taiji01.Effect.BorrowedForce");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalSpiritEvasionEffectDefinitionId()
{
	return TEXT("Combat.Style.Sword.Taiji01.Effect.RedirectedMomentum");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalEffectCuePolicyDefinitionId()
{
	return TEXT("Presentation.Sword.Taiji01.SymbolicEffectCues.r1");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalPreciseLinkVisualCueDefinitionId()
{
	return TEXT("Presentation.Sword.Taiji01.Visual.PreciseFlow.r1");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalPreciseLinkAudioCueDefinitionId()
{
	return TEXT("Presentation.Sword.Taiji01.Audio.PreciseFlow.r1");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalPerfectGuardVisualCueDefinitionId()
{
	return TEXT("Presentation.Sword.Taiji01.Visual.BorrowedForce.r1");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalPerfectGuardAudioCueDefinitionId()
{
	return TEXT("Presentation.Sword.Taiji01.Audio.BorrowedForce.r1");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalSpiritEvasionVisualCueDefinitionId()
{
	return TEXT("Presentation.Sword.Taiji01.Visual.RedirectedMomentum.r1");
}

FName Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalSpiritEvasionAudioCueDefinitionId()
{
	return TEXT("Presentation.Sword.Taiji01.Audio.RedirectedMomentum.r1");
}

int64 Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalLinkOpenOffsetTicks()
{
	return 8;
}

int64 Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalLinkCloseOffsetTicks()
{
	return 13;
}

int64 Fdemo_mapShanmenSwordRhythmProductConfig::
	CanonicalTimelineTicksPerSecond()
{
	return Fdemo_mapShanmenCombatRunFixedTimeline::
		CanonicalTicksPerSecond();
}

FGuid Fdemo_mapShanmenSwordRhythmProductConfig::CanonicalConfigId()
{
	FShanmenContentStamp Content;
	Content.Version = CanonicalContentVersion();
	Content.Digest = CanonicalContentDigest();
	FShanmenSwordRhythmEvaluationPolicy EvaluationPolicy;
	if (!TryCreateCanonicalEvaluationPolicy(Content, EvaluationPolicy))
	{
		return FGuid();
	}
	Fdemo_mapShanmenSwordRhythmEffectCuePolicy EffectCuePolicy;
	if (!TryCreateCanonicalEffectCuePolicy(Content, EffectCuePolicy))
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Combat.SwordRhythm.ProductConfig.r3"),
		{
			CanonicalContentVersion().ToString(),
			CanonicalContentDigest(),
			FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId()
				.ToString(),
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId()
				.ToString(),
			CanonicalRuleId().ToString(),
			FString::Printf(
				TEXT("%lld"),
				static_cast<long long>(CanonicalLinkOpenOffsetTicks())),
			FString::Printf(
				TEXT("%lld"),
				static_cast<long long>(CanonicalLinkCloseOffsetTicks())),
			FString::Printf(
				TEXT("%lld"),
				static_cast<long long>(CanonicalTimelineTicksPerSecond())),
			GuidDigits(EvaluationPolicy.GetPolicyId()),
			GuidDigits(EffectCuePolicy.GetPolicyId())
		});
}

bool Fdemo_mapShanmenSwordRhythmProductConfig::TryCreateCanonical(
	Fdemo_mapShanmenSwordRhythmProductConfig& OutConfig)
{
	OutConfig = Fdemo_mapShanmenSwordRhythmProductConfig();
	Fdemo_mapShanmenSwordRhythmProductConfig Candidate;
	Candidate.Content.Version = CanonicalContentVersion();
	Candidate.Content.Digest = CanonicalContentDigest();

	FShanmenSwordRhythmDefinitionCapture DefinitionCapture;
	DefinitionCapture.StyleDefinitionId =
		FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId();
	DefinitionCapture.RuleId = CanonicalRuleId();
	DefinitionCapture.LinkOpenOffsetTicks =
		CanonicalLinkOpenOffsetTicks();
	DefinitionCapture.LinkCloseOffsetTicks =
		CanonicalLinkCloseOffsetTicks();
	if (!FShanmenSwordRhythmDefinition::TryCapture(
			DefinitionCapture,
			Candidate.Definition))
	{
		return false;
	}
	if (!TryCreateCanonicalEvaluationPolicy(
			Candidate.Content,
			Candidate.EvaluationPolicy))
	{
		return false;
	}
	if (!TryCreateCanonicalEffectCuePolicy(
			Candidate.Content,
			Candidate.EffectCuePolicy))
	{
		return false;
	}

	Candidate.TimelineTicksPerSecond =
		CanonicalTimelineTicksPerSecond();
	Candidate.ConfigId = CanonicalConfigId();
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutConfig = Candidate;
	return true;
}

bool Fdemo_mapShanmenSwordRhythmProductConfig::IsValid() const
{
	FShanmenSwordRhythmEvaluationPolicy ExpectedPolicy;
	if (!TryCreateCanonicalEvaluationPolicy(Content, ExpectedPolicy))
	{
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCuePolicy ExpectedCuePolicy;
	if (!TryCreateCanonicalEffectCuePolicy(Content, ExpectedCuePolicy))
	{
		return false;
	}
	return ConfigId.IsValid()
		&& ConfigId == CanonicalConfigId()
		&& Content.IsValid()
		&& Content.Version == CanonicalContentVersion()
		&& Content.Digest == CanonicalContentDigest()
		&& Definition.IsValid()
		&& Definition.GetStyleDefinitionId()
			== FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId()
		&& Definition.GetSupportedActionDefinitionId()
			== FShanmenBasicSwordDefinition::CanonicalActionDefinitionId()
		&& Definition.GetRuleId() == CanonicalRuleId()
		&& Definition.GetLinkOpenOffsetTicks()
			== CanonicalLinkOpenOffsetTicks()
		&& Definition.GetLinkCloseOffsetTicks()
			== CanonicalLinkCloseOffsetTicks()
		&& EvaluationPolicy.IsValid()
		&& EvaluationPolicy.GetPolicyId() == ExpectedPolicy.GetPolicyId()
		&& EffectCuePolicy.IsValid()
		&& EffectCuePolicy.GetPolicyId()
			== ExpectedCuePolicy.GetPolicyId()
		&& TimelineTicksPerSecond == CanonicalTimelineTicksPerSecond();
}

bool Fdemo_mapShanmenSwordRhythmProductSession::TryBegin(
	const FGuid& RequestedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !RequestedRunId.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Session requires valid empty state and Run identity.");
		return false;
	}

	Fdemo_mapShanmenSwordRhythmProductConfig CanonicalConfig;
	if (!Fdemo_mapShanmenSwordRhythmProductConfig::TryCreateCanonical(
			CanonicalConfig))
	{
		OutDiagnostic =
			TEXT("Canonical sword-rhythm product config failed closed.");
		return false;
	}
	if (!IsEmpty())
	{
		if (Host.GetRunId() == RequestedRunId
			&& Config.GetConfigId() == CanonicalConfig.GetConfigId())
		{
			OutDiagnostic =
				TEXT("Sword-rhythm product Session is already active for this exact Run and config.");
			return true;
		}
		OutDiagnostic =
			TEXT("Sword-rhythm product Session rejects a second active Run or config.");
		return false;
	}

	Fdemo_mapShanmenSwordRhythmProductHost CandidateHost;
	if (!CandidateHost.TryBegin(
			RequestedRunId,
			CanonicalConfig.GetDefinition(),
			OutDiagnostic))
	{
		return false;
	}
	Fdemo_mapShanmenSwordRhythmProductSession Candidate;
	Candidate.Config = CanonicalConfig;
	Candidate.Host = CandidateHost;
	if (!Candidate.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Session rejected its canonical candidate state.");
		return false;
	}

	*this = Candidate;
	OutDiagnostic =
		TEXT("Canonical sword-rhythm product config was installed for this Run.");
	return true;
}

bool Fdemo_mapShanmenSwordRhythmProductSession::
	TryObserveExecutedBasicSword(
		const Fdemo_mapBasicSwordProductExecutionResult& ProductResult,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmReceipt& OutReceipt,
		FString& OutDiagnostic)
{
	FShanmenSwordRhythmContributionBindingReceipt IgnoredBindingReceipt;
	FShanmenSwordRhythmEvaluationInput IgnoredEvaluationInput;
	return TryObserveExecutedBasicSword(
		ProductResult,
		TimelineSample,
		OutReceipt,
		IgnoredBindingReceipt,
		IgnoredEvaluationInput,
		OutDiagnostic);
}

bool Fdemo_mapShanmenSwordRhythmProductSession::
	TryObserveExecutedBasicSword(
		const Fdemo_mapBasicSwordProductExecutionResult& ProductResult,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmReceipt& OutReceipt,
		FShanmenSwordRhythmContributionBindingReceipt& OutBindingReceipt,
		FString& OutDiagnostic)
{
	FShanmenSwordRhythmEvaluationInput IgnoredEvaluationInput;
	return TryObserveExecutedBasicSword(
		ProductResult,
		TimelineSample,
		OutReceipt,
		OutBindingReceipt,
		IgnoredEvaluationInput,
		OutDiagnostic);
}

bool Fdemo_mapShanmenSwordRhythmProductSession::
	TryObserveExecutedBasicSword(
		const Fdemo_mapBasicSwordProductExecutionResult& ProductResult,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmReceipt& OutReceipt,
		FShanmenSwordRhythmContributionBindingReceipt& OutBindingReceipt,
		FShanmenSwordRhythmEvaluationInput& OutEvaluationInput,
		FString& OutDiagnostic)
{
	FShanmenSwordRhythmEvaluationReceipt IgnoredEvaluationReceipt;
	return TryObserveExecutedBasicSword(
		ProductResult,
		TimelineSample,
		OutReceipt,
		OutBindingReceipt,
		OutEvaluationInput,
		IgnoredEvaluationReceipt,
		OutDiagnostic);
}

bool Fdemo_mapShanmenSwordRhythmProductSession::
	TryObserveExecutedBasicSword(
		const Fdemo_mapBasicSwordProductExecutionResult& ProductResult,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmReceipt& OutReceipt,
		FShanmenSwordRhythmContributionBindingReceipt& OutBindingReceipt,
		FShanmenSwordRhythmEvaluationInput& OutEvaluationInput,
		FShanmenSwordRhythmEvaluationReceipt& OutEvaluationReceipt,
		FString& OutDiagnostic)
{
	OutReceipt = FShanmenSwordRhythmReceipt();
	OutBindingReceipt =
		FShanmenSwordRhythmContributionBindingReceipt();
	OutEvaluationInput = FShanmenSwordRhythmEvaluationInput();
	OutEvaluationReceipt = FShanmenSwordRhythmEvaluationReceipt();
	OutDiagnostic.Reset();
	if (!IsValid() || IsEmpty())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm observation requires one valid active product Session.");
		return false;
	}

	Fdemo_mapShanmenSwordRhythmProductHost CandidateHost = Host;
	FShanmenSwordRhythmReceipt CandidateReceipt;
	if (!CandidateHost.TryObserveExecutedBasicSword(
			ProductResult,
			TimelineSample,
			CandidateReceipt,
			OutDiagnostic))
	{
		return false;
	}

	FShanmenSwordRhythmContributionBindingLedger CandidateBindings =
		ContributionBindings;
	const FShanmenSwordRhythmObservation& CurrentObservation =
		CandidateReceipt.GetCurrentObservation();
	if (!TryEnsureContributionBindingScope(
			CurrentObservation.GetAction(),
			CurrentObservation.GetTimelineId(),
			CandidateBindings,
			OutDiagnostic))
	{
		return false;
	}
	FShanmenSwordRhythmContributionBindingReceipt CandidateBindingReceipt;
	if (!CandidateBindings.TryObserveBasicSword(
			CurrentObservation,
			CandidateBindingReceipt))
	{
		OutDiagnostic =
			TEXT("Sword-rhythm contribution ledger rejected the accepted BasicSword observation.");
		return false;
	}
	if (CandidateReceipt.GetBand()
		== EShanmenSwordRhythmBand::PreciseLinked)
	{
		FShanmenSwordRhythmContribution PreciseContribution;
		if (!FShanmenSwordRhythmContribution::TryCapturePreciseSwordLink(
				CandidateReceipt,
				PreciseContribution)
			|| !CandidateBindings.TryRecordContribution(
				PreciseContribution))
		{
			OutDiagnostic =
				TEXT("Sword-rhythm product Session could not preserve precise-link contribution evidence for the next action.");
			return false;
		}
	}
	FShanmenSwordRhythmEvaluationInput CandidateEvaluationInput;
	if (!FShanmenSwordRhythmEvaluationInput::TryCapture(
			CandidateReceipt,
			CandidateBindingReceipt,
			CandidateEvaluationInput))
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Session could not freeze the evaluator handoff input.");
		return false;
	}
	const FShanmenSwordRhythmEvaluationResult Evaluation =
		FShanmenSwordRhythmEvaluator::Evaluate(
			CandidateEvaluationInput,
			Config.GetEvaluationPolicy());
	if (!Evaluation.IsSuccess())
	{
		OutDiagnostic = FString::Printf(
			TEXT("Canonical sword-rhythm evaluation failed closed: %s"),
			*Evaluation.GetDiagnostic());
		return false;
	}
	const FShanmenSwordRhythmEvaluationReceipt CandidateEvaluationReceipt =
		Evaluation.GetReceipt();
	const auto Projection =
		Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
			Config,
			Host.GetRunId(),
			CandidateReceipt,
			CandidateEvaluationReceipt,
			CandidateHost.GetChain().NumRecordedObservations());
	if (!Projection.IsProjected())
	{
		OutDiagnostic = Projection.Diagnostic;
		return false;
	}

	Fdemo_mapShanmenSwordRhythmProductSession Candidate = *this;
	Candidate.Host = CandidateHost;
	Candidate.LastReceipt = CandidateReceipt;
	Candidate.LastEvaluationInput = CandidateEvaluationInput;
	Candidate.LastEvaluationReceipt = CandidateEvaluationReceipt;
	Candidate.PresentationState = Projection.State;
	Candidate.ContributionBindings = CandidateBindings;
	if (!Candidate.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Session rejected an invalid post-observation state.");
		return false;
	}

	*this = Candidate;
	OutReceipt = CandidateReceipt;
	OutBindingReceipt = CandidateBindingReceipt;
	OutEvaluationInput = CandidateEvaluationInput;
	OutEvaluationReceipt = CandidateEvaluationReceipt;
	OutDiagnostic =
		TEXT("Completed BasicSword action was atomically accepted, evaluated and projected by the canonical sword-rhythm Session.");
	return true;
}

bool Fdemo_mapShanmenSwordRhythmProductSession::
	TryRecordPerfectWeaponGuardContribution(
		const FShanmenWeaponGuardTimingProjectionReceipt& Receipt,
		FShanmenSwordRhythmContribution& OutContribution,
		FString& OutDiagnostic)
{
	OutContribution = FShanmenSwordRhythmContribution();
	OutDiagnostic.Reset();
	FShanmenSwordRhythmContribution CandidateContribution;
	if (!FShanmenSwordRhythmContribution::
			TryCapturePerfectWeaponGuard(
				Receipt,
				CandidateContribution))
	{
		OutDiagnostic =
			TEXT("Only a valid perfect weapon-guard projection can contribute to sword rhythm.");
		return false;
	}
	if (!TryRecordContribution(CandidateContribution, OutDiagnostic))
	{
		return false;
	}
	OutContribution = CandidateContribution;
	OutDiagnostic =
		TEXT("Perfect weapon-guard evidence is pending for the next BasicSword action.");
	return true;
}

bool Fdemo_mapShanmenSwordRhythmProductSession::
	TryRecordSpiritEvasionContribution(
		const FShanmenSpiritEvasionProjectionReceipt& Receipt,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmContribution& OutContribution,
		FString& OutDiagnostic)
{
	OutContribution = FShanmenSwordRhythmContribution();
	OutDiagnostic.Reset();
	if (!TimelineSample.IsValid())
	{
		OutDiagnostic =
			TEXT("Spirit-evasion contribution requires one valid Run timeline sample.");
		return false;
	}
	FShanmenSwordRhythmContribution CandidateContribution;
	if (!FShanmenSwordRhythmContribution::TryCaptureSpiritEvasion(
			Receipt,
			TimelineSample.GetTimelineId(),
			TimelineSample.GetCurrentTick(),
			CandidateContribution))
	{
		OutDiagnostic =
			TEXT("Only a valid spirit-evasion projection can contribute to sword rhythm.");
		return false;
	}
	if (!TryRecordContribution(CandidateContribution, OutDiagnostic))
	{
		return false;
	}
	OutContribution = CandidateContribution;
	OutDiagnostic =
		TEXT("Spirit-evasion evidence is pending for the next BasicSword action.");
	return true;
}

bool Fdemo_mapShanmenSwordRhythmProductSession::TryRecordContribution(
	const FShanmenSwordRhythmContribution& Contribution,
	FString& OutDiagnostic)
{
	if (!IsValid() || IsEmpty() || !Contribution.IsValid())
	{
		OutDiagnostic =
			TEXT("Contribution recording requires one valid active sword-rhythm Session and immutable evidence.");
		return false;
	}

	FShanmenSwordRhythmContributionBindingLedger CandidateBindings =
		ContributionBindings;
	if (!TryEnsureContributionBindingScope(
			Contribution.GetAction(),
			Contribution.GetTimelineId(),
			CandidateBindings,
			OutDiagnostic)
		|| !CandidateBindings.TryRecordContribution(Contribution))
	{
		if (OutDiagnostic.IsEmpty())
		{
			OutDiagnostic =
				TEXT("Contribution evidence conflicts with the active Run, owner, timeline or ledger history.");
		}
		return false;
	}

	Fdemo_mapShanmenSwordRhythmProductSession Candidate = *this;
	Candidate.ContributionBindings = CandidateBindings;
	if (!Candidate.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Session rejected invalid contribution state.");
		return false;
	}
	*this = Candidate;
	return true;
}

bool Fdemo_mapShanmenSwordRhythmProductSession::
	TryEnsureContributionBindingScope(
		const FShanmenCombatActionSnapshot& Action,
		const FGuid& TimelineId,
		FShanmenSwordRhythmContributionBindingLedger& InOutLedger,
		FString& OutDiagnostic) const
{
	if (!Action.IsValid()
		|| Action.GetRunId() != Host.GetRunId()
		|| !TimelineId.IsValid()
		|| TimelineId
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
				Host.GetRunId()))
	{
		OutDiagnostic =
			TEXT("Contribution evidence must belong to this Session's Run and canonical timeline.");
		return false;
	}
	if (InOutLedger.IsValid())
	{
		const auto& Scope = InOutLedger.GetScope();
		if (Scope.GetRunId() != Action.GetRunId()
			|| Scope.GetOwnerId() != Action.GetOwnerId()
			|| Scope.GetTimelineId() != TimelineId)
		{
			OutDiagnostic =
				TEXT("Contribution evidence conflicts with the Session's frozen Run, owner or timeline scope.");
			return false;
		}
		return true;
	}

	FShanmenSwordRhythmContributionBindingScope Scope;
	if (!FShanmenSwordRhythmContributionBindingScope::TryCapture(
			Action.GetRunId(),
			Action.GetOwnerId(),
			TimelineId,
			Scope)
		|| !FShanmenSwordRhythmContributionBindingLedger::TryCreate(
			Scope,
			InOutLedger))
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Session could not freeze its contribution binding scope.");
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenSwordRhythmProductSession::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Session end requires valid state and Run identity.");
		return false;
	}
	if (IsEmpty())
	{
		OutDiagnostic = TEXT("Sword-rhythm product Session is already empty.");
		return true;
	}
	if (Host.GetRunId() != ExpectedRunId)
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Session rejects mismatched Run teardown.");
		return false;
	}

	Fdemo_mapShanmenSwordRhythmProductHost CandidateHost = Host;
	if (!CandidateHost.TryEnd(ExpectedRunId, OutDiagnostic))
	{
		return false;
	}
	Reset();
	OutDiagnostic = TEXT("Sword-rhythm product Session ended with its Run.");
	return true;
}

void Fdemo_mapShanmenSwordRhythmProductSession::Reset()
{
	*this = Fdemo_mapShanmenSwordRhythmProductSession();
}

bool Fdemo_mapShanmenSwordRhythmProductSession::IsValid() const
{
	if (!Host.IsValid())
	{
		return false;
	}
	if (Host.IsEmpty())
	{
		return !Config.IsValid()
			&& !LastReceipt.IsValid()
			&& !LastEvaluationInput.IsValid()
			&& !LastEvaluationReceipt.IsValid()
			&& !PresentationState.IsValid()
			&& !ContributionBindings.IsValid();
	}
	if (!Config.IsValid()
		|| Host.GetDefinition().GetDefinitionId()
			!= Config.GetDefinition().GetDefinitionId())
	{
		return false;
	}

	const int32 ObservationCount =
		Host.GetChain().NumRecordedObservations();
	if (ContributionBindings.IsValid())
	{
		const auto& Scope = ContributionBindings.GetScope();
		if (Scope.GetRunId() != Host.GetRunId()
			|| Scope.GetTimelineId()
				!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
					Host.GetRunId())
			|| ContributionBindings.NumObservedActions()
				!= ObservationCount
			|| (ObservationCount > 0
				&& ContributionBindings.GetLastObservation()
					.GetObservationId()
					!= Host.GetChain().GetLastObservation()
						.GetObservationId()))
		{
			return false;
		}
	}
	else if (ObservationCount > 0)
	{
		return false;
	}
	if (ObservationCount == 0)
	{
		return !LastReceipt.IsValid()
			&& !LastEvaluationInput.IsValid()
			&& !LastEvaluationReceipt.IsValid()
			&& !PresentationState.IsValid();
	}
	TArray<FName> ExpectedEffectDefinitionIds;
	if (LastEvaluationReceipt.IsValid())
	{
		ExpectedEffectDefinitionIds.Reserve(
			LastEvaluationReceipt.NumEffects());
		for (const FShanmenSwordRhythmEvaluatedEffect& Effect
			: LastEvaluationReceipt.GetEffects())
		{
			ExpectedEffectDefinitionIds.Add(
				Effect.GetSpecification().GetEffectDefinitionId());
		}
	}
	return LastReceipt.IsValid()
		&& LastEvaluationInput.IsValid()
		&& LastEvaluationReceipt.IsValid()
		&& LastEvaluationInput.GetRhythmReceipt().GetReceiptId()
			== LastReceipt.GetReceiptId()
		&& LastEvaluationReceipt.GetInput().GetInputId()
			== LastEvaluationInput.GetInputId()
		&& LastEvaluationReceipt.GetInput().GetRhythmReceipt().GetReceiptId()
			== LastReceipt.GetReceiptId()
		&& LastEvaluationReceipt.GetPolicy().GetPolicyId()
			== Config.GetEvaluationPolicy().GetPolicyId()
		&& PresentationState.IsValid()
		&& LastReceipt.GetDefinition().GetDefinitionId()
			== Config.GetDefinition().GetDefinitionId()
		&& LastReceipt.GetCurrentObservation().GetObservationId()
			== Host.GetChain().GetLastObservation().GetObservationId()
		&& LastReceipt.GetResultingChainCount()
			== Host.GetChain().GetCurrentChainCount()
		&& PresentationState.GetRunId() == Host.GetRunId()
		&& PresentationState.GetConfigId() == Config.GetConfigId()
		&& PresentationState.GetReceiptId() == LastReceipt.GetReceiptId()
		&& PresentationState.GetEvaluationReceiptId()
			== LastEvaluationReceipt.GetReceiptId()
		&& PresentationState.GetEvaluationPolicyId()
			== Config.GetEvaluationPolicy().GetPolicyId()
		&& PresentationState.GetEffectDefinitionIds()
			== ExpectedEffectDefinitionIds
		&& PresentationState.GetObservationRevision() == ObservationCount
		&& PresentationState.GetResultingChainCount()
			== LastReceipt.GetResultingChainCount();
}

bool Fdemo_mapShanmenSwordRhythmProductSession::IsEmpty() const
{
	return IsValid() && Host.IsEmpty();
}
