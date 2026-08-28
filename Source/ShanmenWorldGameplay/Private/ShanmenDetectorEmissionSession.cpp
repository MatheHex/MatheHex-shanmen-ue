#include "ShanmenDetectorEmissionSession.h"

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
	return Action.IsValid()
		&& !DetectorId.IsNone()
		&& NextEmissionOrdinal >= 0
		&& NextEmissionOrdinal <= MAX_int32;
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

	AcceptedTargetIds.Reset();
	bEmissionActive = true;
	return true;
}

bool FShanmenDetectorEmissionSession::TryAcceptCandidate(const FShanmenHitCandidate& Candidate)
{
	if (!bEmissionActive
		|| !Candidate.IsValid()
		|| Candidate.ActivationId != Action.GetActivationId()
		|| Candidate.SourceEntityId != Action.GetSourceEntityId()
		|| Candidate.DetectorId != DetectorId
		|| Candidate.DetectorKind != DetectorKind
		|| Candidate.HitOrdinal != static_cast<int32>(NextEmissionOrdinal)
		|| AcceptedTargetIds.Contains(Candidate.TargetEntityId))
	{
		return false;
	}

	AcceptedTargetIds.Add(Candidate.TargetEntityId);
	return true;
}

bool FShanmenDetectorEmissionSession::TryEndEmission()
{
	if (!bEmissionActive)
	{
		return false;
	}

	bEmissionActive = false;
	AcceptedTargetIds.Reset();
	++NextEmissionOrdinal;
	return true;
}

void FShanmenDetectorEmissionSession::Reset()
{
	*this = FShanmenDetectorEmissionSession();
}
