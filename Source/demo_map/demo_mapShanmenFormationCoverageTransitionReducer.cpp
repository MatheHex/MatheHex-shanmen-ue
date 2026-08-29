#include "demo_mapShanmenFormationCoverageTransitionReducer.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsKnownKind(
		const Edemo_mapShanmenFormationCoverageTransitionKind Kind)
	{
		return Kind ==
				Edemo_mapShanmenFormationCoverageTransitionKind::Entered
			|| Kind ==
				Edemo_mapShanmenFormationCoverageTransitionKind::StayedCovered
			|| Kind ==
				Edemo_mapShanmenFormationCoverageTransitionKind::Left
			|| Kind ==
				Edemo_mapShanmenFormationCoverageTransitionKind::RemainedOutside;
	}

	Edemo_mapShanmenFormationCoverageTransitionKind DeriveKind(
		const Fdemo_mapShanmenFormationMembershipReceipt& Previous,
		const Fdemo_mapShanmenFormationMembershipReceipt& Current)
	{
		if (!Previous.IsCovered())
		{
			return Current.IsCovered()
				? Edemo_mapShanmenFormationCoverageTransitionKind::Entered
				: Edemo_mapShanmenFormationCoverageTransitionKind::RemainedOutside;
		}
		return Current.IsCovered()
			? Edemo_mapShanmenFormationCoverageTransitionKind::StayedCovered
			: Edemo_mapShanmenFormationCoverageTransitionKind::Left;
	}

	FGuid MakeFactId(
		const Fdemo_mapShanmenFormationMembershipTransitionFact& Fact)
	{
		if (!Fact.AreaId.IsValid() || !Fact.SubjectEntityId.IsValid()
			|| !Fact.PreviousMembershipReceiptId.IsValid()
			|| !Fact.CurrentMembershipReceiptId.IsValid()
			|| !IsKnownKind(Fact.Kind))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.CoverageTransitionFact.r1"),
			{
				GuidDigits(Fact.AreaId),
				GuidDigits(Fact.SubjectEntityId),
				GuidDigits(Fact.PreviousMembershipReceiptId),
				GuidDigits(Fact.CurrentMembershipReceiptId),
				FString::FromInt(static_cast<int32>(Fact.Kind))
			});
	}

	FGuid MakeBatchReceiptId(
		const Fdemo_mapShanmenFormationCoverageTransitionReceipt& Receipt)
	{
		if (!Receipt.PreviousCoverage.ReceiptId.IsValid()
			|| !Receipt.CurrentCoverage.ReceiptId.IsValid()
			|| Receipt.Facts.IsEmpty())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Receipt.PreviousCoverage.AreaId),
			GuidDigits(Receipt.PreviousCoverage.ReceiptId),
			GuidDigits(Receipt.CurrentCoverage.ReceiptId),
			FString::FromInt(Receipt.Facts.Num()),
			FString::FromInt(Receipt.EnteredCount),
			FString::FromInt(Receipt.StayedCoveredCount),
			FString::FromInt(Receipt.LeftCount),
			FString::FromInt(Receipt.RemainedOutsideCount)
		};
		for (const Fdemo_mapShanmenFormationMembershipTransitionFact& Fact :
			Receipt.Facts)
		{
			if (!Fact.IsValid())
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Fact.FactId));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.CoverageTransitionBatch.r1"), Parts);
	}

	Fdemo_mapShanmenFormationCoverageTransitionResult Reject(
		const Edemo_mapShanmenFormationCoverageTransitionStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationCoverageTransitionResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationMembershipTransitionFact::IsValid() const
{
	return FactId.IsValid() && AreaId.IsValid() && SubjectEntityId.IsValid()
		&& PreviousMembershipReceiptId.IsValid()
		&& CurrentMembershipReceiptId.IsValid() && IsKnownKind(Kind)
		&& FactId == MakeFactId(*this);
}

bool Fdemo_mapShanmenFormationCoverageTransitionReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !PreviousCoverage.IsValid()
		|| !CurrentCoverage.IsValid()
		|| PreviousCoverage.AreaId != CurrentCoverage.AreaId
		|| Facts.Num() != PreviousCoverage.Memberships.Num()
		|| Facts.Num() != CurrentCoverage.Memberships.Num()
		|| Facts.IsEmpty() || EnteredCount < 0 || StayedCoveredCount < 0
		|| LeftCount < 0 || RemainedOutsideCount < 0
		|| EnteredCount + StayedCoveredCount + LeftCount
			+ RemainedOutsideCount != Facts.Num())
	{
		return false;
	}

	int32 ExpectedEntered = 0;
	int32 ExpectedStayedCovered = 0;
	int32 ExpectedLeft = 0;
	int32 ExpectedRemainedOutside = 0;
	for (int32 Index = 0; Index < Facts.Num(); ++Index)
	{
		const Fdemo_mapShanmenFormationMembershipReceipt& Previous =
			PreviousCoverage.Memberships[Index];
		const Fdemo_mapShanmenFormationMembershipReceipt& Current =
			CurrentCoverage.Memberships[Index];
		const Fdemo_mapShanmenFormationMembershipTransitionFact& Fact =
			Facts[Index];
		const Edemo_mapShanmenFormationCoverageTransitionKind ExpectedKind =
			DeriveKind(Previous, Current);
		if (Previous.SubjectEntityId != Current.SubjectEntityId
			|| !Fact.IsValid()
			|| Fact.AreaId != PreviousCoverage.AreaId
			|| Fact.SubjectEntityId != Previous.SubjectEntityId
			|| Fact.PreviousMembershipReceiptId != Previous.ReceiptId
			|| Fact.CurrentMembershipReceiptId != Current.ReceiptId
			|| Fact.Kind != ExpectedKind)
		{
			return false;
		}
		switch (Fact.Kind)
		{
		case Edemo_mapShanmenFormationCoverageTransitionKind::Entered:
			++ExpectedEntered;
			break;
		case Edemo_mapShanmenFormationCoverageTransitionKind::StayedCovered:
			++ExpectedStayedCovered;
			break;
		case Edemo_mapShanmenFormationCoverageTransitionKind::Left:
			++ExpectedLeft;
			break;
		case Edemo_mapShanmenFormationCoverageTransitionKind::RemainedOutside:
			++ExpectedRemainedOutside;
			break;
		}
	}
	return ExpectedEntered == EnteredCount
		&& ExpectedStayedCovered == StayedCoveredCount
		&& ExpectedLeft == LeftCount
		&& ExpectedRemainedOutside == RemainedOutsideCount
		&& ReceiptId == MakeBatchReceiptId(*this);
}

bool Fdemo_mapShanmenFormationCoverageTransitionResult::IsSuccess() const
{
	return Status ==
			Edemo_mapShanmenFormationCoverageTransitionStatus::Reduced
		&& Receipt.IsValid();
}

Fdemo_mapShanmenFormationCoverageTransitionResult
Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
	const Fdemo_mapShanmenFormationCoverageReceipt& Previous,
	const Fdemo_mapShanmenFormationCoverageReceipt& Current)
{
	if (!Previous.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationCoverageTransitionStatus::PreviousInvalid,
			TEXT("Transition reduction requires one valid previous coverage receipt."));
	}
	if (!Current.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationCoverageTransitionStatus::CurrentInvalid,
			TEXT("Transition reduction requires one valid current coverage receipt."));
	}
	if (Previous.AreaId != Current.AreaId)
	{
		return Reject(
			Edemo_mapShanmenFormationCoverageTransitionStatus::AreaMismatch,
			TEXT("Area replacement is an explicit lifecycle event, not a coverage transition."));
	}
	if (Previous.Memberships.Num() != Current.Memberships.Num())
	{
		return Reject(
			Edemo_mapShanmenFormationCoverageTransitionStatus::SubjectSetMismatch,
			TEXT("Missing or newly sampled subjects require explicit caller lifecycle handling."));
	}
	for (int32 Index = 0; Index < Previous.Memberships.Num(); ++Index)
	{
		if (Previous.Memberships[Index].SubjectEntityId
			!= Current.Memberships[Index].SubjectEntityId)
		{
			return Reject(
				Edemo_mapShanmenFormationCoverageTransitionStatus::SubjectSetMismatch,
				TEXT("Coverage transition snapshots must contain the exact same canonical subject set."));
		}
	}

	Fdemo_mapShanmenFormationCoverageTransitionResult Result;
	Result.Receipt.PreviousCoverage = Previous;
	Result.Receipt.CurrentCoverage = Current;
	Result.Receipt.Facts.Reserve(Previous.Memberships.Num());
	for (int32 Index = 0; Index < Previous.Memberships.Num(); ++Index)
	{
		const Fdemo_mapShanmenFormationMembershipReceipt& PreviousMembership =
			Previous.Memberships[Index];
		const Fdemo_mapShanmenFormationMembershipReceipt& CurrentMembership =
			Current.Memberships[Index];
		Fdemo_mapShanmenFormationMembershipTransitionFact& Fact =
			Result.Receipt.Facts.AddDefaulted_GetRef();
		Fact.AreaId = Previous.AreaId;
		Fact.SubjectEntityId = PreviousMembership.SubjectEntityId;
		Fact.PreviousMembershipReceiptId = PreviousMembership.ReceiptId;
		Fact.CurrentMembershipReceiptId = CurrentMembership.ReceiptId;
		Fact.Kind = DeriveKind(PreviousMembership, CurrentMembership);
		Fact.FactId = MakeFactId(Fact);
		switch (Fact.Kind)
		{
		case Edemo_mapShanmenFormationCoverageTransitionKind::Entered:
			++Result.Receipt.EnteredCount;
			break;
		case Edemo_mapShanmenFormationCoverageTransitionKind::StayedCovered:
			++Result.Receipt.StayedCoveredCount;
			break;
		case Edemo_mapShanmenFormationCoverageTransitionKind::Left:
			++Result.Receipt.LeftCount;
			break;
		case Edemo_mapShanmenFormationCoverageTransitionKind::RemainedOutside:
			++Result.Receipt.RemainedOutsideCount;
			break;
		}
	}
	Result.Receipt.ReceiptId = MakeBatchReceiptId(Result.Receipt);
	if (!Result.Receipt.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationCoverageTransitionStatus::ReceiptRejected,
			TEXT("Coverage transition evidence failed deterministic self-validation."));
	}
	Result.Status = Edemo_mapShanmenFormationCoverageTransitionStatus::Reduced;
	Result.Diagnostic =
		TEXT("Two immutable coverage snapshots produced canonical transition facts.");
	return Result;
}
