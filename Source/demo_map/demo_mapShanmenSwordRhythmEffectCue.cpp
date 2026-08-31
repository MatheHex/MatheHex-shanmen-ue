#include "demo_mapShanmenSwordRhythmEffectCue.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool IsKnownChannel(
		const Edemo_mapShanmenSwordRhythmEffectCueChannel Channel)
	{
		return Channel
			== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
			|| Channel
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio;
	}

	bool BindingComesBefore(
		const Fdemo_mapShanmenSwordRhythmEffectCueBinding& Left,
		const Fdemo_mapShanmenSwordRhythmEffectCueBinding& Right)
	{
		return Left.GetEffectDefinitionId().ToString().Compare(
			Right.GetEffectDefinitionId().ToString(),
			ESearchCase::CaseSensitive) < 0;
	}

	FGuid MakeBindingId(
		const FName EffectDefinitionId,
		const FName VisualCueDefinitionId,
		const FName AudioCueDefinitionId,
		const FShanmenContentStamp& Content)
	{
		if (EffectDefinitionId.IsNone()
			|| (VisualCueDefinitionId.IsNone()
				&& AudioCueDefinitionId.IsNone())
			|| !Content.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueBinding.r1"),
			{
				EffectDefinitionId.ToString(),
				VisualCueDefinitionId.IsNone()
					? TEXT("NO_VISUAL_CUE")
					: VisualCueDefinitionId.ToString(),
				AudioCueDefinitionId.IsNone()
					? TEXT("NO_AUDIO_CUE")
					: AudioCueDefinitionId.ToString(),
				Content.Version.ToString(),
				Content.Digest
			});
	}

	FGuid MakePolicyId(
		const FName PolicyDefinitionId,
		const FShanmenContentStamp& Content,
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueBinding>& Bindings)
	{
		if (PolicyDefinitionId.IsNone()
			|| !Content.IsValid()
			|| Bindings.IsEmpty())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			PolicyDefinitionId.ToString(),
			Content.Version.ToString(),
			Content.Digest,
			FString::FromInt(Bindings.Num())
		};
		for (const Fdemo_mapShanmenSwordRhythmEffectCueBinding& Binding
			: Bindings)
		{
			if (!Binding.IsValid()
				|| !ContentMatches(Content, Binding.GetContent()))
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Binding.GetBindingId()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCuePolicy.r1"),
			Parts);
	}

	FName CueDefinitionForChannel(
		const Fdemo_mapShanmenSwordRhythmEffectCueBinding& Binding,
		const Edemo_mapShanmenSwordRhythmEffectCueChannel Channel)
	{
		switch (Channel)
		{
		case Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual:
			return Binding.GetVisualCueDefinitionId();
		case Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio:
			return Binding.GetAudioCueDefinitionId();
		default:
			return NAME_None;
		}
	}

	FGuid MakeCommandId(
		const Fdemo_mapShanmenSwordRhythmEffectCueCommand& Command)
	{
		if (!Command.GetPresentationStateId().IsValid()
			|| !Command.GetEvaluationReceiptId().IsValid()
			|| !Command.GetCuePolicyId().IsValid()
			|| !Command.GetBinding().IsValid()
			|| Command.GetEffectOrdinal() < 0
			|| !IsKnownChannel(Command.GetChannel())
			|| Command.GetCueDefinitionId().IsNone())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueCommand.r1"),
			{
				GuidDigits(Command.GetPresentationStateId()),
				GuidDigits(Command.GetEvaluationReceiptId()),
				GuidDigits(Command.GetCuePolicyId()),
				GuidDigits(Command.GetBinding().GetBindingId()),
				FString::FromInt(Command.GetEffectOrdinal()),
				FString::FromInt(static_cast<uint8>(Command.GetChannel())),
				Command.GetCueDefinitionId().ToString()
			});
	}

	bool BuildCanonicalCommands(
		const Fdemo_mapShanmenSwordRhythmPresentationState& State,
		const Fdemo_mapShanmenSwordRhythmEffectCuePolicy& Policy,
		TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand>& OutCommands)
	{
		OutCommands.Reset();
		if (!State.IsValid() || !Policy.IsValid())
		{
			return false;
		}
		OutCommands.Reserve(State.NumEffectDefinitions() * 2);
		for (int32 EffectOrdinal = 0;
			EffectOrdinal < State.NumEffectDefinitions();
			++EffectOrdinal)
		{
			Fdemo_mapShanmenSwordRhythmEffectCueBinding Binding;
			if (!Policy.TryFindBinding(
					State.GetEffectDefinitionIds()[EffectOrdinal], Binding))
			{
				OutCommands.Reset();
				return false;
			}
			for (const auto Channel : {
					Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
					Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio})
			{
				if (CueDefinitionForChannel(Binding, Channel).IsNone())
				{
					continue;
				}
				Fdemo_mapShanmenSwordRhythmEffectCueCommand Command;
				if (!Fdemo_mapShanmenSwordRhythmEffectCueCommand::TryCreate(
						State,
						Policy,
						Binding,
						EffectOrdinal,
						Channel,
						Command))
				{
					OutCommands.Reset();
					return false;
				}
				OutCommands.Add(MoveTemp(Command));
			}
		}
		return true;
	}

	FGuid MakeEventId(
		const Fdemo_mapShanmenSwordRhythmPresentationState& State,
		const Fdemo_mapShanmenSwordRhythmEffectCuePolicy& Policy,
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand>& Commands)
	{
		if (!State.IsValid() || !Policy.IsValid())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(State.GetPresentationStateId()),
			GuidDigits(State.GetEvaluationReceiptId()),
			GuidDigits(Policy.GetPolicyId()),
			FString::FromInt(Commands.Num())
		};
		for (const Fdemo_mapShanmenSwordRhythmEffectCueCommand& Command
			: Commands)
		{
			if (!Command.IsValid())
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Command.GetCommandId()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueEvent.r1"),
			Parts);
	}

	Fdemo_mapShanmenSwordRhythmEffectCueAdaptResult Reject(
		const Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueAdaptResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueBinding::TryCapture(
	const Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture& Capture,
	const FShanmenContentStamp& InContent,
	Fdemo_mapShanmenSwordRhythmEffectCueBinding& OutBinding)
{
	OutBinding = Fdemo_mapShanmenSwordRhythmEffectCueBinding();
	Fdemo_mapShanmenSwordRhythmEffectCueBinding Candidate;
	Candidate.EffectDefinitionId = Capture.EffectDefinitionId;
	Candidate.VisualCueDefinitionId = Capture.VisualCueDefinitionId;
	Candidate.AudioCueDefinitionId = Capture.AudioCueDefinitionId;
	Candidate.Content = InContent;
	Candidate.BindingId = MakeBindingId(
		Candidate.EffectDefinitionId,
		Candidate.VisualCueDefinitionId,
		Candidate.AudioCueDefinitionId,
		Candidate.Content);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutBinding = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueBinding::IsValid() const
{
	return BindingId.IsValid()
		&& BindingId == MakeBindingId(
			EffectDefinitionId,
			VisualCueDefinitionId,
			AudioCueDefinitionId,
			Content);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePolicy::TryCapture(
	const Fdemo_mapShanmenSwordRhythmEffectCuePolicyCapture& Capture,
	Fdemo_mapShanmenSwordRhythmEffectCuePolicy& OutPolicy)
{
	OutPolicy = Fdemo_mapShanmenSwordRhythmEffectCuePolicy();
	if (Capture.PolicyDefinitionId.IsNone()
		|| !Capture.Content.IsValid()
		|| Capture.Bindings.IsEmpty())
	{
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCuePolicy Candidate;
	Candidate.PolicyDefinitionId = Capture.PolicyDefinitionId;
	Candidate.Content = Capture.Content;
	Candidate.Bindings.Reserve(Capture.Bindings.Num());
	for (const Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture& BindingCapture
		: Capture.Bindings)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueBinding Binding;
		if (!Fdemo_mapShanmenSwordRhythmEffectCueBinding::TryCapture(
				BindingCapture, Candidate.Content, Binding))
		{
			return false;
		}
		Candidate.Bindings.Add(MoveTemp(Binding));
	}
	Candidate.Bindings.Sort(BindingComesBefore);
	Candidate.PolicyId = MakePolicyId(
		Candidate.PolicyDefinitionId,
		Candidate.Content,
		Candidate.Bindings);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutPolicy = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePolicy::IsValid() const
{
	if (!PolicyId.IsValid()
		|| PolicyDefinitionId.IsNone()
		|| !Content.IsValid()
		|| Bindings.IsEmpty())
	{
		return false;
	}
	TSet<FName> EffectDefinitionIds;
	for (int32 Index = 0; Index < Bindings.Num(); ++Index)
	{
		const Fdemo_mapShanmenSwordRhythmEffectCueBinding& Binding =
			Bindings[Index];
		if (!Binding.IsValid()
			|| !ContentMatches(Content, Binding.GetContent())
			|| EffectDefinitionIds.Contains(Binding.GetEffectDefinitionId())
			|| (Index > 0
				&& !BindingComesBefore(Bindings[Index - 1], Binding)))
		{
			return false;
		}
		EffectDefinitionIds.Add(Binding.GetEffectDefinitionId());
	}
	return PolicyId == MakePolicyId(
		PolicyDefinitionId, Content, Bindings);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePolicy::TryFindBinding(
	const FName EffectDefinitionId,
	Fdemo_mapShanmenSwordRhythmEffectCueBinding& OutBinding) const
{
	OutBinding = Fdemo_mapShanmenSwordRhythmEffectCueBinding();
	if (!IsValid() || EffectDefinitionId.IsNone())
	{
		return false;
	}
	for (const Fdemo_mapShanmenSwordRhythmEffectCueBinding& Binding
		: Bindings)
	{
		if (Binding.GetEffectDefinitionId() == EffectDefinitionId)
		{
			OutBinding = Binding;
			return true;
		}
	}
	return false;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueCommand::TryCreate(
	const Fdemo_mapShanmenSwordRhythmPresentationState& State,
	const Fdemo_mapShanmenSwordRhythmEffectCuePolicy& Policy,
	const Fdemo_mapShanmenSwordRhythmEffectCueBinding& InBinding,
	const int32 InEffectOrdinal,
	const Edemo_mapShanmenSwordRhythmEffectCueChannel InChannel,
	Fdemo_mapShanmenSwordRhythmEffectCueCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenSwordRhythmEffectCueCommand();
	if (!State.IsValid()
		|| !Policy.IsValid()
		|| !InBinding.IsValid()
		|| !State.GetEffectDefinitionIds().IsValidIndex(InEffectOrdinal)
		|| State.GetEffectDefinitionIds()[InEffectOrdinal]
			!= InBinding.GetEffectDefinitionId()
		|| !IsKnownChannel(InChannel))
	{
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueBinding PolicyBinding;
	if (!Policy.TryFindBinding(
			InBinding.GetEffectDefinitionId(), PolicyBinding)
		|| PolicyBinding.GetBindingId() != InBinding.GetBindingId())
	{
		return false;
	}
	const FName CueDefinitionId =
		CueDefinitionForChannel(InBinding, InChannel);
	if (CueDefinitionId.IsNone())
	{
		return false;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueCommand Candidate;
	Candidate.PresentationStateId = State.GetPresentationStateId();
	Candidate.EvaluationReceiptId = State.GetEvaluationReceiptId();
	Candidate.CuePolicyId = Policy.GetPolicyId();
	Candidate.Binding = InBinding;
	Candidate.EffectOrdinal = InEffectOrdinal;
	Candidate.Channel = InChannel;
	Candidate.CueDefinitionId = CueDefinitionId;
	Candidate.CommandId = MakeCommandId(Candidate);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCommand = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueCommand::IsValid() const
{
	return CommandId.IsValid()
		&& CueDefinitionId
			== CueDefinitionForChannel(Binding, Channel)
		&& CommandId == MakeCommandId(*this);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueEvent::TryCreate(
	const Fdemo_mapShanmenSwordRhythmPresentationState& InState,
	const Fdemo_mapShanmenSwordRhythmEffectCuePolicy& InPolicy,
	Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent)
{
	OutEvent = Fdemo_mapShanmenSwordRhythmEffectCueEvent();
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand> Commands;
	if (!BuildCanonicalCommands(InState, InPolicy, Commands))
	{
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Candidate;
	Candidate.State = InState;
	Candidate.Policy = InPolicy;
	Candidate.Commands = MoveTemp(Commands);
	Candidate.EventId = MakeEventId(
		Candidate.State,
		Candidate.Policy,
		Candidate.Commands);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutEvent = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueEvent::IsValid() const
{
	if (!EventId.IsValid() || !State.IsValid() || !Policy.IsValid())
	{
		return false;
	}
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand> ExpectedCommands;
	if (!BuildCanonicalCommands(State, Policy, ExpectedCommands)
		|| ExpectedCommands.Num() != Commands.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Commands.Num(); ++Index)
	{
		if (!Commands[Index].IsValid()
			|| Commands[Index].GetCommandId()
				!= ExpectedCommands[Index].GetCommandId())
		{
			return false;
		}
	}
	return EventId == MakeEventId(State, Policy, Commands);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueEvent::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Other) const
{
	return IsValid() && Other.IsValid() && EventId == Other.EventId;
}

Fdemo_mapShanmenSwordRhythmEffectCueAdaptResult
Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
	const Fdemo_mapShanmenSwordRhythmPresentationState& State,
	const Fdemo_mapShanmenSwordRhythmEffectCuePolicy& Policy)
{
	if (!State.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::StateInvalid,
			TEXT("Effect-cue adaptation requires one valid presentation state."));
	}
	if (!Policy.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::PolicyInvalid,
			TEXT("Effect-cue adaptation requires one valid authored policy."));
	}
	for (const FName EffectDefinitionId : State.GetEffectDefinitionIds())
	{
		Fdemo_mapShanmenSwordRhythmEffectCueBinding Binding;
		if (!Policy.TryFindBinding(EffectDefinitionId, Binding))
		{
			return Reject(
				Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::EffectUnmapped,
				TEXT("Presentation state contains an effect outside the authored cue policy."));
		}
	}

	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueEvent::TryCreate(
			State, Policy, Event))
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::EventRejected,
			TEXT("Effect-cue event failed deterministic self-validation."));
	}
	Fdemo_mapShanmenSwordRhythmEffectCueAdaptResult Result;
	Result.Status =
		Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::Adapted;
	Result.Diagnostic =
		TEXT("Read-only symbolic effects were adapted to typed cue commands.");
	Result.Event = MoveTemp(Event);
	return Result;
}
