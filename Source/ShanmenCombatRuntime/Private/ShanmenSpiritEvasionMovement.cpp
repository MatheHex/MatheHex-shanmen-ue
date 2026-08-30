#include "ShanmenSpiritEvasionMovement.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString DoubleBits(double Value)
	{
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(
			TEXT("%016llX"),
			static_cast<unsigned long long>(Bits));
	}

	void AppendCanonicalTags(
		TArray<FString>& Parts,
		const FGameplayTagContainer& Container)
	{
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.GetTagName().LexicalLess(Right.GetTagName());
		});
		Parts.Add(FString::FromInt(Tags.Num()));
		for (const FGameplayTag& Tag : Tags)
		{
			Parts.Add(Tag.ToString());
		}
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId() == Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool IsCanonicalPlanarDirection(const FVector& Direction)
	{
		return FMath::IsFinite(Direction.X)
			&& FMath::IsFinite(Direction.Y)
			&& FMath::IsFinite(Direction.Z)
			&& Direction.Z == 0.0
			&& !Direction.IsNearlyZero()
			&& FMath::IsNearlyEqual(
				Direction.SizeSquared2D(), 1.0, 1.e-8);
	}

	bool TryCanonicalPlanarDirection(
		const FVector& Candidate,
		FVector& OutDirection)
	{
		OutDirection = FVector::ZeroVector;
		if (!FMath::IsFinite(Candidate.X)
			|| !FMath::IsFinite(Candidate.Y)
			|| !FMath::IsFinite(Candidate.Z))
		{
			return false;
		}

		OutDirection = FVector(Candidate.X, Candidate.Y, 0.0);
		if (!OutDirection.Normalize())
		{
			OutDirection = FVector::ZeroVector;
			return false;
		}

		if (OutDirection.X == 0.0) OutDirection.X = 0.0;
		if (OutDirection.Y == 0.0) OutDirection.Y = 0.0;
		OutDirection.Z = 0.0;
		return IsCanonicalPlanarDirection(OutDirection);
	}

	FGuid MakeIntentId(
		const FShanmenCombatActionSnapshot& Action,
		FName MovementPolicyId,
		const FVector& PlanarDirection)
	{
		if (!Action.IsValid()
			|| Action.GetActionDefinitionId()
				!= FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
			|| MovementPolicyId.IsNone()
			|| !IsCanonicalPlanarDirection(PlanarDirection))
		{
			return FGuid();
		}

		TArray<FString> Parts = {
			GuidDigits(Action.GetRunId()),
			GuidDigits(Action.GetOwnerId()),
			GuidDigits(Action.GetActivationId()),
			GuidDigits(Action.GetSourceEntityId()),
			GuidDigits(Action.GetSourceItemInstanceId()),
			Action.GetActionDefinitionId().ToString(),
			Action.GetContent().Version.ToString(),
			Action.GetContent().Digest,
			MovementPolicyId.ToString(),
			DoubleBits(PlanarDirection.X),
			DoubleBits(PlanarDirection.Y),
			DoubleBits(PlanarDirection.Z)
		};
		AppendCanonicalTags(Parts, Action.GetSourceTags());
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritEvasion.MovementIntent.r1"), Parts);
	}

	FGuid MakeRequestId(
		const FShanmenSpiritEvasionMovementIntent& Intent,
		const FShanmenSpiritEvasionWindowReceipt& Window)
	{
		if (!Intent.IsValid()
			|| !Window.IsValid()
			|| !ActionsMatch(Intent.GetAction(), Window.GetAction()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritEvasion.MovementRequest.r1"),
			{
				GuidDigits(Intent.GetIntentId()),
				GuidDigits(Window.GetReceiptId())
			});
	}
}

bool FShanmenSpiritEvasionMovementIntent::TryCapture(
	const FShanmenSpiritEvasionMovementIntentCapture& Capture,
	FShanmenSpiritEvasionMovementIntent& OutIntent)
{
	OutIntent = FShanmenSpiritEvasionMovementIntent();
	FVector PlanarDirection;
	if (!Capture.Action.IsValid()
		|| Capture.Action.GetActionDefinitionId()
			!= FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
		|| Capture.MovementPolicyId.IsNone()
		|| !TryCanonicalPlanarDirection(
			Capture.CandidateDirection, PlanarDirection))
	{
		return false;
	}

	FShanmenSpiritEvasionMovementIntent Candidate;
	Candidate.Action = Capture.Action;
	Candidate.MovementPolicyId = Capture.MovementPolicyId;
	Candidate.PlanarDirection = PlanarDirection;
	Candidate.IntentId = MakeIntentId(
		Candidate.Action,
		Candidate.MovementPolicyId,
		Candidate.PlanarDirection);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutIntent = Candidate;
	return true;
}

bool FShanmenSpiritEvasionMovementIntent::IsValid() const
{
	return IntentId.IsValid()
		&& Action.IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
		&& !MovementPolicyId.IsNone()
		&& IsCanonicalPlanarDirection(PlanarDirection)
		&& IntentId == MakeIntentId(
			Action, MovementPolicyId, PlanarDirection);
}

bool FShanmenSpiritEvasionMovementRequest::IsValid() const
{
	return RequestId.IsValid()
		&& Intent.IsValid()
		&& Window.IsValid()
		&& ActionsMatch(Intent.GetAction(), Window.GetAction())
		&& RequestId == MakeRequestId(Intent, Window);
}

bool FShanmenSpiritEvasionMovementPlanner::TryCreateRequest(
	const FShanmenSpiritEvasionMovementIntent& Intent,
	const FShanmenSpiritEvasionWindow& Window,
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenSpiritEvasionMovementRequest& OutRequest)
{
	OutRequest = FShanmenSpiritEvasionMovementRequest();
	if (!Intent.IsValid()
		|| !Window.IsActiveFor(ActionRuntime)
		|| !ActionsMatch(
			Intent.GetAction(), Window.GetOpenReceipt().GetAction()))
	{
		return false;
	}

	FShanmenSpiritEvasionMovementRequest Candidate;
	Candidate.Intent = Intent;
	Candidate.Window = Window.GetOpenReceipt();
	Candidate.RequestId = MakeRequestId(
		Candidate.Intent, Candidate.Window);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutRequest = Candidate;
	return true;
}

bool FShanmenSpiritEvasionMovementLedger::TryAccept(
	const FShanmenSpiritEvasionMovementRequest& Request)
{
	if (!Request.IsValid()
		|| AcceptedRequestIds.Contains(Request.GetRequestId()))
	{
		return false;
	}
	AcceptedRequestIds.Add(Request.GetRequestId());
	return true;
}

bool FShanmenSpiritEvasionMovementLedger::Contains(
	const FGuid& RequestId) const
{
	return RequestId.IsValid() && AcceptedRequestIds.Contains(RequestId);
}

int32 FShanmenSpiritEvasionMovementLedger::Num() const
{
	return AcceptedRequestIds.Num();
}

void FShanmenSpiritEvasionMovementLedger::Reset()
{
	AcceptedRequestIds.Reset();
}
