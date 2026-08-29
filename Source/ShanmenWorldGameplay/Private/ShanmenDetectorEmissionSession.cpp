#include "ShanmenDetectorEmissionSession.h"

namespace
{
	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return Left.ToString(EGuidFormats::Digits)
			< Right.ToString(EGuidFormats::Digits);
	}

	bool CandidateMatchesContext(
		const FShanmenHitCandidate& Candidate,
		const FShanmenWorldHitContext& Context)
	{
		return Candidate.IsValid()
			&& Context.IsValid()
			&& Candidate.ActivationId
				== Context.GetAction().GetActivationId()
			&& Candidate.SourceEntityId
				== Context.GetAction().GetSourceEntityId()
			&& Candidate.DetectorId == Context.GetDetectorId()
			&& Candidate.DetectorKind == Context.GetDetectorKind()
			&& Candidate.HitOrdinal == Context.GetHitOrdinal();
	}
}

bool FShanmenDetectorEmissionReceipt::IsValid() const
{
	if (!Context.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FShanmenHitCandidate& Candidate = Candidates[Index];
		if (!CandidateMatchesContext(Candidate, Context)
			|| (Index > 0
				&& !GuidLess(
					Candidates[Index - 1].TargetEntityId,
					Candidate.TargetEntityId)))
		{
			return false;
		}
	}
	return true;
}

bool FShanmenDetectorEmissionSession::TryStart(
	const FShanmenCombatActionSnapshot& Action,
	FName DetectorId,
	EShanmenHitDetectorKind DetectorKind,
	FShanmenDetectorEmissionSession& OutSession)
{
	OutSession = FShanmenDetectorEmissionSession();
	FShanmenWorldHitContext Probe;
	if (!FShanmenWorldHitContext::TryCreate(
			Action,
			DetectorId,
			DetectorKind,
			0,
			Probe))
	{
		return false;
	}

	OutSession.Action = Action;
	OutSession.DetectorId = DetectorId;
	OutSession.DetectorKind = DetectorKind;
	return true;
}

bool FShanmenDetectorEmissionSession::IsValid() const
{
	if (!Action.IsValid()
		|| DetectorId.IsNone()
		|| NextEmissionOrdinal < 0
		|| NextEmissionOrdinal > MAX_int32)
	{
		return false;
	}

	if (!bEmissionActive)
	{
		return !ActiveContext.IsValid() && AcceptedCandidates.IsEmpty();
	}
	if (!ActiveContext.IsValid()
		|| ActiveContext.GetAction().GetActivationId()
			!= Action.GetActivationId()
		|| ActiveContext.GetAction().GetSourceEntityId()
			!= Action.GetSourceEntityId()
		|| ActiveContext.GetDetectorId() != DetectorId
		|| ActiveContext.GetDetectorKind() != DetectorKind
		|| ActiveContext.GetHitOrdinal()
			!= static_cast<int32>(NextEmissionOrdinal))
	{
		return false;
	}
	for (const TPair<FGuid, FShanmenHitCandidate>& Entry : AcceptedCandidates)
	{
		if (Entry.Key != Entry.Value.TargetEntityId
			|| !CandidateMatchesContext(Entry.Value, ActiveContext))
		{
			return false;
		}
	}
	return true;
}

bool FShanmenDetectorEmissionSession::TryBeginEmission(FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	if (!IsValid() || bEmissionActive)
	{
		return false;
	}

	if (!FShanmenWorldHitContext::TryCreate(
			Action,
			DetectorId,
			DetectorKind,
			static_cast<int32>(NextEmissionOrdinal),
			OutContext))
	{
		return false;
	}

	AcceptedCandidates.Reset();
	ActiveContext = OutContext;
	bEmissionActive = true;
	return true;
}

bool FShanmenDetectorEmissionSession::TryAcceptCandidate(const FShanmenHitCandidate& Candidate)
{
	if (!IsValid()
		|| !bEmissionActive
		|| !CandidateMatchesContext(Candidate, ActiveContext)
		|| AcceptedCandidates.Contains(Candidate.TargetEntityId))
	{
		return false;
	}

	AcceptedCandidates.Add(Candidate.TargetEntityId, Candidate);
	return true;
}

bool FShanmenDetectorEmissionSession::TryEndEmission(
	FShanmenDetectorEmissionReceipt& OutReceipt)
{
	OutReceipt = FShanmenDetectorEmissionReceipt();
	if (!IsValid() || !bEmissionActive)
	{
		return false;
	}

	OutReceipt.Context = ActiveContext;
	AcceptedCandidates.GenerateValueArray(OutReceipt.Candidates);
	OutReceipt.Candidates.Sort(
		[](const FShanmenHitCandidate& Left,
			const FShanmenHitCandidate& Right)
		{
			return GuidLess(Left.TargetEntityId, Right.TargetEntityId);
		});
	if (!OutReceipt.IsValid())
	{
		OutReceipt = FShanmenDetectorEmissionReceipt();
		return false;
	}

	bEmissionActive = false;
	ActiveContext = FShanmenWorldHitContext();
	AcceptedCandidates.Reset();
	++NextEmissionOrdinal;
	return true;
}

bool FShanmenDetectorEmissionSession::TryEndEmission()
{
	FShanmenDetectorEmissionReceipt Ignored;
	return TryEndEmission(Ignored);
}

void FShanmenDetectorEmissionSession::Reset()
{
	*this = FShanmenDetectorEmissionSession();
}
