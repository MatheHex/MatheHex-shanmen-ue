#include "demo_mapM01Extraction.h"

const FName Fdemo_mapM01Ids::Map(TEXT("M01"));
const FName Fdemo_mapM01Ids::RiskLow(TEXT("M01.Risk.LOW"));
const FName Fdemo_mapM01Ids::RiskMid(TEXT("M01.Risk.MID"));
const FName Fdemo_mapM01Ids::RiskHigh(TEXT("M01.Risk.HIGH"));
const FName Fdemo_mapM01Ids::ResourceTier1(TEXT("M01.Resource.TIER_1"));
const FName Fdemo_mapM01Ids::ResourceTier2(TEXT("M01.Resource.TIER_2"));
const FName Fdemo_mapM01Ids::ResourceTier3(TEXT("M01.Resource.TIER_3"));
const FName Fdemo_mapM01Ids::ExitDiscardSpatial(TEXT("M01.Exit.DiscardSpatial"));
const FName Fdemo_mapM01Ids::ExitRegular(TEXT("M01.Exit.Regular"));
const FName Fdemo_mapM01Ids::ExitBoss(TEXT("M01.Exit.Boss"));
const FName Fdemo_mapM01Ids::MainBoss(TEXT("M01.Boss.Main"));

bool Fdemo_mapM01Ids::Validate(FString* OutError)
{
	auto Fail = [OutError](const TCHAR* Message)
	{
		if (OutError) *OutError = Message;
		return false;
	};
	const TArray<FName> Ids = {
		Map, RiskLow, RiskMid, RiskHigh,
		ResourceTier1, ResourceTier2, ResourceTier3,
		ExitDiscardSpatial, ExitRegular, ExitBoss, MainBoss
	};
	TSet<FName> Unique;
	for (const FName Id : Ids)
	{
		if (Id.IsNone() || Unique.Contains(Id)) return Fail(TEXT("M01 stable IDs must be non-empty and unique."));
		Unique.Add(Id);
		if (Id != Map && !Id.ToString().StartsWith(TEXT("M01."))) return Fail(TEXT("M01 content IDs must use the M01.* prefix."));
	}
	return true;
}

Fdemo_mapM01ExtractionAuthority::Fdemo_mapM01ExtractionAuthority()
{
	ResetForNewRun(false);
}

void Fdemo_mapM01ExtractionAuthority::ResetForNewRun(bool bInSpatialItemEquipped)
{
	Exits.Reset();
	Exits.Add(Edemo_mapM01ExitType::DiscardSpatial);
	Exits.Add(Edemo_mapM01ExitType::Regular);
	Exits.Add(Edemo_mapM01ExitType::Boss);
	CountingExit.Reset();
	bSpatialItemEquipped = bInSpatialItemEquipped;
	bBossDefeated = false;
	bRunTerminal = false;
	bCompletionIssued = false;
	Runtime(Edemo_mapM01ExitType::Regular).State = Edemo_mapM01ExtractionState::Available;
	Runtime(Edemo_mapM01ExitType::Boss).State = Edemo_mapM01ExtractionState::Locked;
	Runtime(Edemo_mapM01ExitType::DiscardSpatial).State = bSpatialItemEquipped
		? Edemo_mapM01ExtractionState::Locked
		: Edemo_mapM01ExtractionState::Available;
}

bool Fdemo_mapM01ExtractionAuthority::NotifyBossDefeated(FName BossId)
{
	if (bRunTerminal || bBossDefeated || BossId != Fdemo_mapM01Ids::MainBoss) return false;
	bBossDefeated = true;
	FRuntime& Boss = Runtime(Edemo_mapM01ExitType::Boss);
	if (Boss.State != Edemo_mapM01ExtractionState::Completed)
	{
		Boss.State = Edemo_mapM01ExtractionState::Available;
		Boss.CancelReason = Edemo_mapM01ExtractionCancelReason::None;
	}
	return true;
}

void Fdemo_mapM01ExtractionAuthority::SetSpatialItemEquipped(bool bEquipped)
{
	if (bSpatialItemEquipped == bEquipped) return;
	bSpatialItemEquipped = bEquipped;
	FRuntime& Discard = Runtime(Edemo_mapM01ExitType::DiscardSpatial);
	if (bRunTerminal || Discard.State == Edemo_mapM01ExtractionState::Completed) return;
	if (bEquipped)
	{
		if (CountingExit.IsSet() && CountingExit.GetValue() == Edemo_mapM01ExitType::DiscardSpatial)
		{
			CancelCounting(Edemo_mapM01ExtractionCancelReason::ConditionInvalidated);
		}
		else
		{
			Discard.State = Edemo_mapM01ExtractionState::Locked;
			Discard.CancelReason = Edemo_mapM01ExtractionCancelReason::None;
		}
	}
	else if (Discard.State == Edemo_mapM01ExtractionState::Locked
		|| Discard.State == Edemo_mapM01ExtractionState::Cancelled)
	{
		Discard.State = Edemo_mapM01ExtractionState::Available;
		Discard.CancelReason = Edemo_mapM01ExtractionCancelReason::None;
	}
}

void Fdemo_mapM01ExtractionAuthority::SetInRange(Edemo_mapM01ExitType ExitType, bool bInRange)
{
	FRuntime& Exit = Runtime(ExitType);
	if (Exit.bInRange == bInRange) return;
	Exit.bInRange = bInRange;
	if (!bInRange && CountingExit.IsSet() && CountingExit.GetValue() == ExitType)
	{
		CancelCounting(Edemo_mapM01ExtractionCancelReason::LeftRange);
	}
}

bool Fdemo_mapM01ExtractionAuthority::BeginInteraction(Edemo_mapM01ExitType ExitType, FString* OutDiagnostic)
{
	auto Reject = [OutDiagnostic](const TCHAR* Message)
	{
		if (OutDiagnostic) *OutDiagnostic = Message;
		return false;
	};
	FRuntime& Exit = Runtime(ExitType);
	if (bRunTerminal || bCompletionIssued) return Reject(TEXT("The current Run is already terminal."));
	if (!Exit.bInRange) return Reject(TEXT("Enter the extraction range before interacting."));
	if (!IsConditionSatisfied(ExitType)) return Reject(ExitType == Edemo_mapM01ExitType::Boss
		? TEXT("Boss extraction is locked until the current Run boss dies.")
		: TEXT("Discard-spatial extraction requires an empty spatial-item slot."));
	if (CountingExit.IsSet()) return Reject(TEXT("An extraction countdown is already active."));
	if (Exit.State == Edemo_mapM01ExtractionState::Completed) return Reject(TEXT("This extraction already completed."));
	Exit.State = Edemo_mapM01ExtractionState::CountingDown;
	Exit.CancelReason = Edemo_mapM01ExtractionCancelReason::None;
	Exit.RemainingSeconds = CountdownSeconds;
	CountingExit = ExitType;
	return true;
}

void Fdemo_mapM01ExtractionAuthority::NotifyEffectiveDamage()
{
	CancelCounting(Edemo_mapM01ExtractionCancelReason::Damaged);
}

void Fdemo_mapM01ExtractionAuthority::NotifyPlayerDefeated()
{
	CancelCounting(Edemo_mapM01ExtractionCancelReason::PlayerDefeated);
}

void Fdemo_mapM01ExtractionAuthority::NotifyRunTerminal()
{
	if (bRunTerminal) return;
	CancelCounting(Edemo_mapM01ExtractionCancelReason::RunTerminal);
	bRunTerminal = true;
	for (TPair<Edemo_mapM01ExitType, FRuntime>& Pair : Exits)
	{
		if (Pair.Value.State != Edemo_mapM01ExtractionState::Completed)
		{
			Pair.Value.State = Edemo_mapM01ExtractionState::Unavailable;
			Pair.Value.RemainingSeconds = 0.0f;
		}
	}
}

bool Fdemo_mapM01ExtractionAuthority::Advance(float DeltaSeconds, Edemo_mapM01ExitType& OutCompletedExit)
{
	if (!CountingExit.IsSet() || bRunTerminal || bCompletionIssued || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return false;
	const Edemo_mapM01ExitType Type = CountingExit.GetValue();
	FRuntime& Exit = Runtime(Type);
	if (!Exit.bInRange)
	{
		CancelCounting(Edemo_mapM01ExtractionCancelReason::LeftRange);
		return false;
	}
	if (!IsConditionSatisfied(Type))
	{
		CancelCounting(Edemo_mapM01ExtractionCancelReason::ConditionInvalidated);
		return false;
	}
	Exit.RemainingSeconds = FMath::Max(0.0f, Exit.RemainingSeconds - DeltaSeconds);
	if (Exit.RemainingSeconds > KINDA_SMALL_NUMBER) return false;
	Exit.State = Edemo_mapM01ExtractionState::Completed;
	Exit.CancelReason = Edemo_mapM01ExtractionCancelReason::None;
	CountingExit.Reset();
	bCompletionIssued = true;
	OutCompletedExit = Type;
	return true;
}

Fdemo_mapM01ExtractionSnapshot Fdemo_mapM01ExtractionAuthority::GetSnapshot(Edemo_mapM01ExitType ExitType) const
{
	const FRuntime& Exit = Runtime(ExitType);
	Fdemo_mapM01ExtractionSnapshot Result;
	Result.ExitType = ExitType;
	Result.ExitId = ExitId(ExitType);
	Result.State = Exit.State;
	Result.CancelReason = Exit.CancelReason;
	Result.RemainingSeconds = Exit.RemainingSeconds;
	Result.bInRange = Exit.bInRange;
	Result.bConditionSatisfied = IsConditionSatisfied(ExitType);
	return Result;
}

FString Fdemo_mapM01ExtractionAuthority::FormatStatus(Edemo_mapM01ExitType ExitType) const
{
	const Fdemo_mapM01ExtractionSnapshot Snapshot = GetSnapshot(ExitType);
	const FString Label = ExitLabel(ExitType);
	if (Snapshot.State == Edemo_mapM01ExtractionState::CountingDown)
	{
		return FString::Printf(TEXT("%s · 撤离 %.1f 秒"), *Label, Snapshot.RemainingSeconds);
	}
	if (!Snapshot.bConditionSatisfied)
	{
		return ExitType == Edemo_mapM01ExitType::Boss
			? FString::Printf(TEXT("%s · 锁定：击败本局主 Boss"), *Label)
			: FString::Printf(TEXT("%s · 条件不足：先弃置空间道具"), *Label);
	}
	if (Snapshot.State == Edemo_mapM01ExtractionState::Cancelled)
	{
		return FString::Printf(TEXT("%s · 倒计时已取消，可重新交互"), *Label);
	}
	if (Snapshot.State == Edemo_mapM01ExtractionState::Completed)
	{
		return FString::Printf(TEXT("%s · 已完成"), *Label);
	}
	return FString::Printf(TEXT("%s · 可用"), *Label);
}

FName Fdemo_mapM01ExtractionAuthority::ExitId(Edemo_mapM01ExitType ExitType)
{
	switch (ExitType)
	{
	case Edemo_mapM01ExitType::DiscardSpatial: return Fdemo_mapM01Ids::ExitDiscardSpatial;
	case Edemo_mapM01ExitType::Boss: return Fdemo_mapM01Ids::ExitBoss;
	default: return Fdemo_mapM01Ids::ExitRegular;
	}
}

FString Fdemo_mapM01ExtractionAuthority::ExitLabel(Edemo_mapM01ExitType ExitType)
{
	switch (ExitType)
	{
	case Edemo_mapM01ExitType::DiscardSpatial: return TEXT("弃置空间道具撤离");
	case Edemo_mapM01ExitType::Boss: return TEXT("Boss撤离");
	default: return TEXT("常规撤离");
	}
}

bool Fdemo_mapM01ExtractionAuthority::IsConditionSatisfied(Edemo_mapM01ExitType ExitType) const
{
	if (bRunTerminal) return false;
	if (ExitType == Edemo_mapM01ExitType::Boss) return bBossDefeated;
	if (ExitType == Edemo_mapM01ExitType::DiscardSpatial) return !bSpatialItemEquipped;
	return true;
}

void Fdemo_mapM01ExtractionAuthority::CancelCounting(Edemo_mapM01ExtractionCancelReason Reason)
{
	if (!CountingExit.IsSet()) return;
	FRuntime& Exit = Runtime(CountingExit.GetValue());
	Exit.State = Edemo_mapM01ExtractionState::Cancelled;
	Exit.CancelReason = Reason;
	Exit.RemainingSeconds = 0.0f;
	CountingExit.Reset();
}

Fdemo_mapM01ExtractionAuthority::FRuntime& Fdemo_mapM01ExtractionAuthority::Runtime(Edemo_mapM01ExitType ExitType)
{
	return Exits.FindChecked(ExitType);
}

const Fdemo_mapM01ExtractionAuthority::FRuntime& Fdemo_mapM01ExtractionAuthority::Runtime(Edemo_mapM01ExitType ExitType) const
{
	return Exits.FindChecked(ExitType);
}

