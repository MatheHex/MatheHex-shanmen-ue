#include "demo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	TArray<FGuid> OrderedCommandIds(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff)
	{
		TArray<FGuid> Result;
		if (!Handoff.IsValid())
		{
			return Result;
		}
		Result.Reserve(Handoff.GetCommands().Num());
		for (const Fdemo_mapShanmenSwordRhythmEffectCueCommand& Command
			: Handoff.GetCommands())
		{
			Result.Add(Command.GetCommandId());
		}
		return Result;
	}

	bool CommandIdsMatchExactly(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff,
		const TArray<FGuid>& CompletedCommandIds)
	{
		const TArray<FGuid> Expected = OrderedCommandIds(Handoff);
		if (Expected.IsEmpty() || Expected.Num() != CompletedCommandIds.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Expected.Num(); ++Index)
		{
			if (!CompletedCommandIds[Index].IsValid()
				|| CompletedCommandIds[Index] != Expected[Index])
			{
				return false;
			}
		}
		return true;
	}

	FGuid MakeAcknowledgementId(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff,
		const FName PresenterDefinitionId,
		const TArray<FGuid>& CompletedCommandIds)
	{
		if (!Handoff.IsValid() || PresenterDefinitionId.IsNone()
			|| !CommandIdsMatchExactly(Handoff, CompletedCommandIds))
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Handoff.GetHandoffId()),
			GuidDigits(Handoff.GetInvocation().GetInvocationId()),
			FString::FromInt(static_cast<uint8>(Handoff.GetChannel())),
			PresenterDefinitionId.ToString(),
			FString::FromInt(CompletedCommandIds.Num())
		};
		for (const FGuid& CommandId : CompletedCommandIds)
		{
			Parts.Add(GuidDigits(CommandId));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCuePresentationAcknowledgement.r1"),
			Parts);
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement::
	TryCapture(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& InHandoff,
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementCapture& Capture,
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement& OutAcknowledgement,
		FString& OutDiagnostic)
{
	OutAcknowledgement =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement();
	OutDiagnostic.Reset();
	if (!InHandoff.IsValid())
	{
		OutDiagnostic = TEXT(
			"Presentation acknowledgement requires one valid immutable handoff.");
		return false;
	}
	if (Capture.PresenterDefinitionId.IsNone())
	{
		OutDiagnostic = TEXT(
			"Presentation acknowledgement requires one stable authored presenter identity.");
		return false;
	}
	if (!CommandIdsMatchExactly(InHandoff, Capture.CompletedCommandIds))
	{
		OutDiagnostic = TEXT(
			"Presentation acknowledgement must contain every handoff command identity in exact order.");
		return false;
	}

	Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement Candidate;
	Candidate.Handoff = InHandoff;
	Candidate.PresenterDefinitionId = Capture.PresenterDefinitionId;
	Candidate.CompletedCommandIds = Capture.CompletedCommandIds;
	Candidate.AcknowledgementId = MakeAcknowledgementId(
		Candidate.Handoff,
		Candidate.PresenterDefinitionId,
		Candidate.CompletedCommandIds);
	if (!Candidate.IsValid())
	{
		OutDiagnostic = TEXT(
			"Presentation acknowledgement failed deterministic self-validation.");
		return false;
	}
	OutAcknowledgement = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Captured caller acknowledgement for one exact ordered presentation batch.");
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement::
	IsValid() const
{
	return Handoff.IsValid() && !PresenterDefinitionId.IsNone()
		&& CommandIdsMatchExactly(Handoff, CompletedCommandIds)
		&& AcknowledgementId.IsValid()
		&& AcknowledgementId == MakeAcknowledgementId(
			Handoff, PresenterDefinitionId, CompletedCommandIds);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OtherHandoff)
	const
{
	return IsValid() && Handoff.Matches(OtherHandoff);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& AcknowledgementId == Other.AcknowledgementId
		&& Handoff.Matches(Other.Handoff)
		&& PresenterDefinitionId == Other.PresenterDefinitionId
		&& CompletedCommandIds == Other.CompletedCommandIds;
}

TArray<FGuid>
Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
	GetOrderedCommandIds(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff)
{
	return OrderedCommandIds(Handoff);
}

bool Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary::
	TryAcknowledge(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff,
		const FName PresenterDefinitionId,
		const TArray<FGuid>& CompletedCommandIds,
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement& OutAcknowledgement,
		FString& OutDiagnostic)
{
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementCapture Capture;
	Capture.PresenterDefinitionId = PresenterDefinitionId;
	Capture.CompletedCommandIds = CompletedCommandIds;
	return Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement::
		TryCapture(Handoff, Capture, OutAcknowledgement, OutDiagnostic);
}
