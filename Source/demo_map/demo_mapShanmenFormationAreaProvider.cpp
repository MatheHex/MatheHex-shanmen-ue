#include "demo_mapShanmenFormationAreaProvider.h"

#include "ShanmenDeterministicId.h"

namespace
{
	constexpr double GeometryTolerance = KINDA_SMALL_NUMBER;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	FString CanonicalCoordinate(const double Value)
	{
		const double Canonical = Value == 0.0 ? 0.0 : Value;
		uint64 Bits = 0;
		FMemory::Memcpy(&Bits, &Canonical, sizeof(Bits));
		return FString::Printf(
			TEXT("%016llX"), static_cast<unsigned long long>(Bits));
	}

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool IsKnownRelation(
		const Edemo_mapShanmenFormationAreaRelation Relation)
	{
		return Relation == Edemo_mapShanmenFormationAreaRelation::Outside
			|| Relation == Edemo_mapShanmenFormationAreaRelation::Boundary
			|| Relation == Edemo_mapShanmenFormationAreaRelation::Inside;
	}

	bool AnchorLess(
		const Fdemo_mapShanmenFormationAreaAnchor& Left,
		const Fdemo_mapShanmenFormationAreaAnchor& Right)
	{
		if (Left.WorldLocation.X != Right.WorldLocation.X)
		{
			return Left.WorldLocation.X < Right.WorldLocation.X;
		}
		if (Left.WorldLocation.Y != Right.WorldLocation.Y)
		{
			return Left.WorldLocation.Y < Right.WorldLocation.Y;
		}
		if (Left.WorldLocation.Z != Right.WorldLocation.Z)
		{
			return Left.WorldLocation.Z < Right.WorldLocation.Z;
		}
		return GuidDigits(Left.PlacementId) < GuidDigits(Right.PlacementId);
	}

	double Cross(
		const FVector& Origin,
		const FVector& First,
		const FVector& Second)
	{
		return (First.X - Origin.X) * (Second.Y - Origin.Y)
			- (First.Y - Origin.Y) * (Second.X - Origin.X);
	}

	bool IsDuplicatePoint(
		const FVector& Left,
		const FVector& Right)
	{
		return FMath::IsNearlyEqual(
				Left.X, Right.X, GeometryTolerance)
			&& FMath::IsNearlyEqual(
				Left.Y, Right.Y, GeometryTolerance);
	}

	bool BuildHull(
		const TArray<Fdemo_mapShanmenFormationAreaAnchor>& Anchors,
		TArray<int32>& OutHull)
	{
		OutHull.Reset();
		if (Anchors.Num() < 3)
		{
			return false;
		}
		for (int32 Index = 1; Index < Anchors.Num(); ++Index)
		{
			if (IsDuplicatePoint(
				Anchors[Index - 1].WorldLocation,
				Anchors[Index].WorldLocation))
			{
				return false;
			}
		}

		TArray<int32> Lower;
		for (int32 Index = 0; Index < Anchors.Num(); ++Index)
		{
			while (Lower.Num() >= 2
				&& Cross(
					Anchors[Lower[Lower.Num() - 2]].WorldLocation,
					Anchors[Lower.Last()].WorldLocation,
					Anchors[Index].WorldLocation)
					<= GeometryTolerance)
			{
				Lower.Pop(EAllowShrinking::No);
			}
			Lower.Add(Index);
		}

		TArray<int32> Upper;
		for (int32 Index = Anchors.Num() - 1; Index >= 0; --Index)
		{
			while (Upper.Num() >= 2
				&& Cross(
					Anchors[Upper[Upper.Num() - 2]].WorldLocation,
					Anchors[Upper.Last()].WorldLocation,
					Anchors[Index].WorldLocation)
					<= GeometryTolerance)
			{
				Upper.Pop(EAllowShrinking::No);
			}
			Upper.Add(Index);
		}
		Lower.Pop(EAllowShrinking::No);
		Upper.Pop(EAllowShrinking::No);
		OutHull = MoveTemp(Lower);
		OutHull.Append(Upper);
		if (OutHull.Num() < 3)
		{
			OutHull.Reset();
			return false;
		}

		double TwiceArea = 0.0;
		for (int32 Index = 0; Index < OutHull.Num(); ++Index)
		{
			const FVector& A = Anchors[OutHull[Index]].WorldLocation;
			const FVector& B = Anchors[
				OutHull[(Index + 1) % OutHull.Num()]].WorldLocation;
			TwiceArea += A.X * B.Y - A.Y * B.X;
		}
		if (TwiceArea <= GeometryTolerance)
		{
			OutHull.Reset();
			return false;
		}
		return true;
	}

	FGuid MakeAreaId(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area)
	{
		if (!Area.RunId.IsValid() || !Area.OwnerId.IsValid()
			|| !Area.DeploymentId.IsValid() || !Area.Content.IsValid()
			|| Area.Anchors.Num() < 3)
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Area.RunId),
			GuidDigits(Area.OwnerId),
			GuidDigits(Area.DeploymentId),
			Area.Content.Version.ToString(),
			Area.Content.Digest,
			FString::FromInt(Area.Anchors.Num())
		};
		for (const Fdemo_mapShanmenFormationAreaAnchor& Anchor : Area.Anchors)
		{
			if (!Anchor.IsValid())
			{
				return FGuid();
			}
			Parts.Append({
				GuidDigits(Anchor.PlacementReceiptId),
				GuidDigits(Anchor.PlacementId),
				Anchor.AnchorDefinitionId.ToString(),
				GuidDigits(Anchor.AnchorInstanceId),
				CanonicalCoordinate(Anchor.WorldLocation.X),
				CanonicalCoordinate(Anchor.WorldLocation.Y),
				CanonicalCoordinate(Anchor.WorldLocation.Z)
			});
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.Area.r1"), Parts);
	}

	FGuid MakeMembershipReceiptId(
		const FGuid& AreaId,
		const FGuid& SubjectEntityId,
		const FVector& WorldLocation,
		const Edemo_mapShanmenFormationAreaRelation Relation)
	{
		if (!AreaId.IsValid() || !SubjectEntityId.IsValid()
			|| !IsFiniteVector(WorldLocation) || !IsKnownRelation(Relation))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.AreaMembership.r1"),
			{
				GuidDigits(AreaId),
				GuidDigits(SubjectEntityId),
				CanonicalCoordinate(WorldLocation.X),
				CanonicalCoordinate(WorldLocation.Y),
				CanonicalCoordinate(WorldLocation.Z),
				FString::FromInt(static_cast<int32>(Relation))
			});
	}

	FGuid MakeCoverageReceiptId(
		const Fdemo_mapShanmenFormationCoverageReceipt& Coverage)
	{
		if (!Coverage.AreaId.IsValid() || Coverage.Memberships.IsEmpty())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Coverage.AreaId),
			FString::FromInt(Coverage.Memberships.Num())
		};
		for (const Fdemo_mapShanmenFormationMembershipReceipt& Membership :
			Coverage.Memberships)
		{
			if (!Membership.IsValid()
				|| Membership.AreaId != Coverage.AreaId)
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Membership.ReceiptId));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.AreaCoverage.r1"), Parts);
	}

	Edemo_mapShanmenFormationAreaRelation Classify(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const FVector& Point)
	{
		bool bBoundary = false;
		for (int32 Index = 0; Index < Area.BoundaryAnchorIndices.Num(); ++Index)
		{
			const FVector& A = Area.Anchors[
				Area.BoundaryAnchorIndices[Index]].WorldLocation;
			const FVector& B = Area.Anchors[
				Area.BoundaryAnchorIndices[
					(Index + 1) % Area.BoundaryAnchorIndices.Num()]].WorldLocation;
			const double Side = Cross(A, B, Point);
			if (Side < -GeometryTolerance)
			{
				return Edemo_mapShanmenFormationAreaRelation::Outside;
			}
			if (FMath::Abs(Side) <= GeometryTolerance
				&& Point.X >= FMath::Min(A.X, B.X) - GeometryTolerance
				&& Point.X <= FMath::Max(A.X, B.X) + GeometryTolerance
				&& Point.Y >= FMath::Min(A.Y, B.Y) - GeometryTolerance
				&& Point.Y <= FMath::Max(A.Y, B.Y) + GeometryTolerance)
			{
				bBoundary = true;
			}
		}
		return bBoundary
			? Edemo_mapShanmenFormationAreaRelation::Boundary
			: Edemo_mapShanmenFormationAreaRelation::Inside;
	}

	Fdemo_mapShanmenFormationAreaBuildResult RejectBuild(
		const Edemo_mapShanmenFormationAreaBuildStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationAreaBuildResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationAreaAnchor::IsValid() const
{
	return PlacementReceiptId.IsValid() && PlacementId.IsValid()
		&& !AnchorDefinitionId.IsNone() && AnchorInstanceId.IsValid()
		&& IsFiniteVector(WorldLocation);
}

bool Fdemo_mapShanmenFormationAreaSnapshot::IsValid() const
{
	if (!AreaId.IsValid() || !RunId.IsValid() || !OwnerId.IsValid()
		|| !DeploymentId.IsValid() || !Content.IsValid()
		|| Anchors.Num() < 3 || BoundaryAnchorIndices.Num() < 3)
	{
		return false;
	}
	TSet<FGuid> ReceiptIds;
	TSet<FGuid> PlacementIds;
	TSet<FGuid> AnchorInstanceIds;
	TSet<FName> AnchorDefinitionIds;
	for (int32 Index = 0; Index < Anchors.Num(); ++Index)
	{
		const Fdemo_mapShanmenFormationAreaAnchor& Anchor = Anchors[Index];
		if (!Anchor.IsValid()
			|| (Index > 0 && !AnchorLess(Anchors[Index - 1], Anchor))
			|| ReceiptIds.Contains(Anchor.PlacementReceiptId)
			|| PlacementIds.Contains(Anchor.PlacementId)
			|| AnchorInstanceIds.Contains(Anchor.AnchorInstanceId)
			|| AnchorDefinitionIds.Contains(Anchor.AnchorDefinitionId))
		{
			return false;
		}
		ReceiptIds.Add(Anchor.PlacementReceiptId);
		PlacementIds.Add(Anchor.PlacementId);
		AnchorInstanceIds.Add(Anchor.AnchorInstanceId);
		AnchorDefinitionIds.Add(Anchor.AnchorDefinitionId);
	}
	TArray<int32> ExpectedHull;
	return BuildHull(Anchors, ExpectedHull)
		&& ExpectedHull == BoundaryAnchorIndices
		&& AreaId == MakeAreaId(*this);
}

bool Fdemo_mapShanmenFormationAreaBuildResult::IsSuccess() const
{
	return Status == Edemo_mapShanmenFormationAreaBuildStatus::Built
		&& Area.IsValid();
}

bool Fdemo_mapShanmenFormationAreaQuery::IsValid() const
{
	return SubjectEntityId.IsValid() && IsFiniteVector(WorldLocation);
}

bool Fdemo_mapShanmenFormationMembershipReceipt::IsValid() const
{
	return ReceiptId.IsValid() && AreaId.IsValid()
		&& SubjectEntityId.IsValid() && IsFiniteVector(WorldLocation)
		&& IsKnownRelation(Relation)
		&& ReceiptId == MakeMembershipReceiptId(
			AreaId, SubjectEntityId, WorldLocation, Relation);
}

bool Fdemo_mapShanmenFormationMembershipResult::IsSuccess() const
{
	return Status == Edemo_mapShanmenFormationMembershipStatus::Evaluated
		&& Receipt.IsValid();
}

bool Fdemo_mapShanmenFormationCoverageReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !AreaId.IsValid() || Memberships.IsEmpty()
		|| InsideCount < 0 || BoundaryCount < 0 || OutsideCount < 0
		|| InsideCount + BoundaryCount + OutsideCount != Memberships.Num())
	{
		return false;
	}
	TSet<FGuid> Subjects;
	int32 ExpectedInside = 0;
	int32 ExpectedBoundary = 0;
	int32 ExpectedOutside = 0;
	FString PreviousSubject;
	for (const Fdemo_mapShanmenFormationMembershipReceipt& Membership :
		Memberships)
	{
		const FString Subject = GuidDigits(Membership.SubjectEntityId);
		if (!Membership.IsValid() || Membership.AreaId != AreaId
			|| Subjects.Contains(Membership.SubjectEntityId)
			|| (!PreviousSubject.IsEmpty() && PreviousSubject >= Subject))
		{
			return false;
		}
		Subjects.Add(Membership.SubjectEntityId);
		PreviousSubject = Subject;
		switch (Membership.Relation)
		{
		case Edemo_mapShanmenFormationAreaRelation::Inside:
			++ExpectedInside;
			break;
		case Edemo_mapShanmenFormationAreaRelation::Boundary:
			++ExpectedBoundary;
			break;
		case Edemo_mapShanmenFormationAreaRelation::Outside:
			++ExpectedOutside;
			break;
		}
	}
	return ExpectedInside == InsideCount
		&& ExpectedBoundary == BoundaryCount
		&& ExpectedOutside == OutsideCount
		&& ReceiptId == MakeCoverageReceiptId(*this);
}

bool Fdemo_mapShanmenFormationCoverageResult::IsSuccess() const
{
	return Status == Edemo_mapShanmenFormationCoverageStatus::Evaluated
		&& Receipt.IsValid();
}

Fdemo_mapShanmenFormationAreaBuildResult
Fdemo_mapShanmenFormationAreaProvider::BuildArea(
	const TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt>&
		PlacementReceipts)
{
	if (PlacementReceipts.IsEmpty())
	{
		return RejectBuild(
			Edemo_mapShanmenFormationAreaBuildStatus::EmptyReceiptSet,
			TEXT("Formation area requires placed anchor receipts."));
	}
	if (PlacementReceipts.Num() < 3)
	{
		return RejectBuild(
			Edemo_mapShanmenFormationAreaBuildStatus::InsufficientAnchors,
			TEXT("A horizontal area requires at least three placed anchors."));
	}

	Fdemo_mapShanmenFormationAreaSnapshot Area;
	TSet<FGuid> ReceiptIds;
	TSet<FGuid> PlacementIds;
	TSet<FGuid> AnchorInstanceIds;
	TSet<FName> AnchorDefinitionIds;
	for (const Fdemo_mapShanmenFormationAnchorPlacementReceipt& Receipt :
		PlacementReceipts)
	{
		if (!Receipt.IsValid())
		{
			return RejectBuild(
				Edemo_mapShanmenFormationAreaBuildStatus::ReceiptInvalid,
				TEXT("Every area source must be one valid P8.3 placement receipt."));
		}
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent =
			Receipt.Intent;
		if (!Area.RunId.IsValid())
		{
			Area.RunId = Intent.RunId;
			Area.OwnerId = Intent.OwnerId;
			Area.DeploymentId = Intent.DeploymentId;
			Area.Content = Intent.Content;
		}
		else if (Area.RunId != Intent.RunId || Area.OwnerId != Intent.OwnerId
			|| Area.DeploymentId != Intent.DeploymentId
			|| !ContentMatches(Area.Content, Intent.Content))
		{
			return RejectBuild(
				Edemo_mapShanmenFormationAreaBuildStatus::ScopeMismatch,
				TEXT("One area cannot mix Run, owner, deployment, or content scopes."));
		}
		if (ReceiptIds.Contains(Receipt.ReceiptId)
			|| PlacementIds.Contains(Intent.PlacementId)
			|| AnchorInstanceIds.Contains(Intent.AnchorInstanceId)
			|| AnchorDefinitionIds.Contains(Intent.AnchorDefinitionId))
		{
			return RejectBuild(
				Edemo_mapShanmenFormationAreaBuildStatus::DuplicateIdentity,
				TEXT("Area sources must have unique receipt, placement, anchor-instance, and anchor-definition identities."));
		}
		ReceiptIds.Add(Receipt.ReceiptId);
		PlacementIds.Add(Intent.PlacementId);
		AnchorInstanceIds.Add(Intent.AnchorInstanceId);
		AnchorDefinitionIds.Add(Intent.AnchorDefinitionId);

		Fdemo_mapShanmenFormationAreaAnchor& Anchor =
			Area.Anchors.AddDefaulted_GetRef();
		Anchor.PlacementReceiptId = Receipt.ReceiptId;
		Anchor.PlacementId = Intent.PlacementId;
		Anchor.AnchorDefinitionId = Intent.AnchorDefinitionId;
		Anchor.AnchorInstanceId = Intent.AnchorInstanceId;
		Anchor.WorldLocation = Intent.WorldLocation;
	}
	Area.Anchors.Sort(AnchorLess);
	for (int32 Index = 1; Index < Area.Anchors.Num(); ++Index)
	{
		if (IsDuplicatePoint(
			Area.Anchors[Index - 1].WorldLocation,
			Area.Anchors[Index].WorldLocation))
		{
			return RejectBuild(
				Edemo_mapShanmenFormationAreaBuildStatus::DuplicatePoint,
				TEXT("Distinct placed anchors cannot occupy the same horizontal point."));
		}
	}
	if (!BuildHull(Area.Anchors, Area.BoundaryAnchorIndices))
	{
		return RejectBuild(
			Edemo_mapShanmenFormationAreaBuildStatus::DegenerateGeometry,
			TEXT("Placed anchors are collinear or have no positive horizontal area."));
	}
	Area.AreaId = MakeAreaId(Area);
	if (!Area.IsValid())
	{
		return RejectBuild(
			Edemo_mapShanmenFormationAreaBuildStatus::DegenerateGeometry,
			TEXT("Canonical formation area failed self-validation."));
	}

	Fdemo_mapShanmenFormationAreaBuildResult Result;
	Result.Status = Edemo_mapShanmenFormationAreaBuildStatus::Built;
	Result.Diagnostic =
		TEXT("Placed anchors produced one deterministic horizontal convex area.");
	Result.Area = MoveTemp(Area);
	return Result;
}

Fdemo_mapShanmenFormationMembershipResult
Fdemo_mapShanmenFormationAreaProvider::EvaluateMembership(
	const Fdemo_mapShanmenFormationAreaSnapshot& Area,
	const Fdemo_mapShanmenFormationAreaQuery& Query)
{
	Fdemo_mapShanmenFormationMembershipResult Result;
	if (!Area.IsValid())
	{
		Result.Status = Edemo_mapShanmenFormationMembershipStatus::AreaInvalid;
		Result.Diagnostic = TEXT("Membership requires one valid frozen area.");
		return Result;
	}
	if (!Query.IsValid())
	{
		Result.Status = Edemo_mapShanmenFormationMembershipStatus::QueryInvalid;
		Result.Diagnostic =
			TEXT("Membership requires one stable entity and finite location.");
		return Result;
	}
	Result.Receipt.AreaId = Area.AreaId;
	Result.Receipt.SubjectEntityId = Query.SubjectEntityId;
	Result.Receipt.WorldLocation = Query.WorldLocation;
	Result.Receipt.Relation = Classify(Area, Query.WorldLocation);
	Result.Receipt.ReceiptId = MakeMembershipReceiptId(
		Area.AreaId,
		Query.SubjectEntityId,
		Query.WorldLocation,
		Result.Receipt.Relation);
	if (!Result.Receipt.IsValid())
	{
		Result.Status = Edemo_mapShanmenFormationMembershipStatus::QueryInvalid;
		Result.Diagnostic =
			TEXT("Membership evidence failed deterministic self-validation.");
		Result.Receipt = Fdemo_mapShanmenFormationMembershipReceipt();
		return Result;
	}
	Result.Status = Edemo_mapShanmenFormationMembershipStatus::Evaluated;
	Result.Diagnostic =
		TEXT("Entity location was classified against the frozen horizontal area.");
	return Result;
}

Fdemo_mapShanmenFormationCoverageResult
Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(
	const Fdemo_mapShanmenFormationAreaSnapshot& Area,
	const TArray<Fdemo_mapShanmenFormationAreaQuery>& Queries)
{
	Fdemo_mapShanmenFormationCoverageResult Result;
	if (!Area.IsValid())
	{
		Result.Status = Edemo_mapShanmenFormationCoverageStatus::AreaInvalid;
		Result.Diagnostic = TEXT("Coverage requires one valid frozen area.");
		return Result;
	}
	if (Queries.IsEmpty())
	{
		Result.Status = Edemo_mapShanmenFormationCoverageStatus::QuerySetEmpty;
		Result.Diagnostic = TEXT("Coverage requires at least one entity query.");
		return Result;
	}
	TArray<Fdemo_mapShanmenFormationAreaQuery> Ordered = Queries;
	Ordered.Sort([](
		const Fdemo_mapShanmenFormationAreaQuery& Left,
		const Fdemo_mapShanmenFormationAreaQuery& Right)
	{
		return GuidDigits(Left.SubjectEntityId)
			< GuidDigits(Right.SubjectEntityId);
	});
	TSet<FGuid> Subjects;
	Result.Receipt.AreaId = Area.AreaId;
	for (const Fdemo_mapShanmenFormationAreaQuery& Query : Ordered)
	{
		if (!Query.IsValid())
		{
			Result.Status = Edemo_mapShanmenFormationCoverageStatus::QueryInvalid;
			Result.Diagnostic =
				TEXT("Every coverage query requires a stable entity and finite location.");
			Result.Receipt = Fdemo_mapShanmenFormationCoverageReceipt();
			return Result;
		}
		if (Subjects.Contains(Query.SubjectEntityId))
		{
			Result.Status =
				Edemo_mapShanmenFormationCoverageStatus::DuplicateSubject;
			Result.Diagnostic =
				TEXT("One coverage snapshot accepts one location per entity.");
			Result.Receipt = Fdemo_mapShanmenFormationCoverageReceipt();
			return Result;
		}
		Subjects.Add(Query.SubjectEntityId);
		const Fdemo_mapShanmenFormationMembershipResult Membership =
			EvaluateMembership(Area, Query);
		if (!Membership.IsSuccess())
		{
			Result.Status = Edemo_mapShanmenFormationCoverageStatus::QueryInvalid;
			Result.Diagnostic = Membership.Diagnostic;
			Result.Receipt = Fdemo_mapShanmenFormationCoverageReceipt();
			return Result;
		}
		Result.Receipt.Memberships.Add(Membership.Receipt);
		switch (Membership.Receipt.Relation)
		{
		case Edemo_mapShanmenFormationAreaRelation::Inside:
			++Result.Receipt.InsideCount;
			break;
		case Edemo_mapShanmenFormationAreaRelation::Boundary:
			++Result.Receipt.BoundaryCount;
			break;
		case Edemo_mapShanmenFormationAreaRelation::Outside:
			++Result.Receipt.OutsideCount;
			break;
		}
	}
	Result.Receipt.ReceiptId = MakeCoverageReceiptId(Result.Receipt);
	if (!Result.Receipt.IsValid())
	{
		Result.Status = Edemo_mapShanmenFormationCoverageStatus::QueryInvalid;
		Result.Diagnostic =
			TEXT("Coverage evidence failed deterministic self-validation.");
		Result.Receipt = Fdemo_mapShanmenFormationCoverageReceipt();
		return Result;
	}
	Result.Status = Edemo_mapShanmenFormationCoverageStatus::Evaluated;
	Result.Diagnostic =
		TEXT("Coverage produced canonically ordered membership evidence.");
	return Result;
}
