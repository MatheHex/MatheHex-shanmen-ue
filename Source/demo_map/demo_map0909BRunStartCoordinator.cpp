#include "demo_map0909BRunStartCoordinator.h"

#include "demo_map.h"
#include "demo_mapGameMode.h"
#include "demo_mapPlayerController.h"

void Fdemo_map0909BRunStartCoordinator::Initialize(
	Ademo_mapGameMode* InGameMode,
	Ademo_mapPlayerController* InController)
{
	GameMode = InGameMode;
	Controller = InController;
	M01Adapter = MakeUnique<Fdemo_map0909BM01RuntimeAdapter>();
	M01Adapter->Initialize(InGameMode, InController);
	State = Edemo_map0909BTopState::AtSect;
	NextAttemptSequence = 0;
}

void Fdemo_map0909BRunStartCoordinator::BeginAttempt()
{
	LastDiagnostic = Fdemo_map0909BStartDiagnostic();
	AttemptRunCorrelation = Fdemo_mapShanmenRunCorrelation();
	LastDiagnostic.StartAttemptId = FGuid::NewGuid();
	LastDiagnostic.BeforeState = State;
	LastDiagnostic.AfterState = State;
	LastDiagnostic.AttemptSequence = ++NextAttemptSequence;
	LastDiagnostic.RequestedAtUtc = FDateTime::UtcNow().ToIso8601();
	LastDiagnostic.M01MapDescriptor =
		Fdemo_map0909BM01RuntimeAdapter::ExpectedM01MapDescriptor();
	LastDiagnostic.Sequence = TEXT("AtSect");
}

void Fdemo_map0909BRunStartCoordinator::Transition(
	const Edemo_map0909BTopState NewState,
	const FString& SequenceStep)
{
	State = NewState;
	LastDiagnostic.AfterState = NewState;
	LastDiagnostic.Sequence += TEXT("->") + SequenceStep;
	UE_LOG(Logdemo_map, Log, TEXT("0_0_9BFIX_RUN_COORDINATOR Event=Transition %s"),
		*LastDiagnostic.ToLogString());
}

void Fdemo_map0909BRunStartCoordinator::RecordRuntimeReceipt(
	const Fdemo_map0909BM01RuntimeReceipt& Receipt)
{
	LastDiagnostic.RuntimeReceiptSequence = Receipt.Sequence;
	LastDiagnostic.M01MapDescriptor = Receipt.MapDescriptor;
	LastDiagnostic.M01MapIdentity = Receipt.MapIdentity;
	LastDiagnostic.RuntimeReceiptClass =
		demo_map0909BM01RuntimeReceiptClassName(Receipt.ReceiptClass);
	if (Receipt.OwnerId.IsValid())
	{
		LastDiagnostic.OwnerId = Receipt.OwnerId;
	}
	if (Receipt.RunInstanceId.IsValid())
	{
		LastDiagnostic.RunId = Receipt.RunInstanceId;
	}
	if (Receipt.RunCorrelation.IsValid()
		&& !AttemptRunCorrelation.IsValid())
	{
		AttemptRunCorrelation = Receipt.RunCorrelation;
		LastDiagnostic.RunCorrelation = Receipt.RunCorrelation;
	}
	if (Receipt.ReceiptClass == Edemo_map0909BM01RuntimeReceiptClass::TechnicalFailure
		&& Receipt.FailureClass != TEXT("None"))
	{
		LastDiagnostic.FailureClass = Receipt.FailureClass;
	}
	if (!Receipt.Detail.IsEmpty())
	{
		LastDiagnostic.Detail = Receipt.Detail;
	}
}

bool Fdemo_map0909BRunStartCoordinator::ValidateRuntimeReady(
	const Fdemo_map0909BM01RuntimeReceipt& Receipt,
	FString& OutDiagnostic) const
{
	OutDiagnostic.Reset();
	if (State != Edemo_map0909BTopState::ActivatingWorld
		|| !Receipt.IsRuntimeReadyFor(LastDiagnostic.StartAttemptId))
	{
		OutDiagnostic = TEXT("RuntimeReady does not match the current ActivatingWorld StartAttempt.");
		return false;
	}
	if (!AttemptRunCorrelation.IsValid()
		|| Receipt.RunCorrelation != AttemptRunCorrelation
		|| Receipt.OwnerId != AttemptRunCorrelation.OwnerId
		|| !Receipt.RunInstanceId.IsValid()
		|| Receipt.RunInstanceId != AttemptRunCorrelation.ActiveRunId
		|| Receipt.MapDescriptor != Fdemo_map0909BM01RuntimeAdapter::ExpectedM01MapDescriptor()
		|| Receipt.MapIdentity.IsEmpty()
		|| !Receipt.bActivationRequestAccepted
		|| !Receipt.bWorldMatched
		|| !Receipt.bGameModeReady
		|| !Receipt.bWorldSettingsReady
		|| !Receipt.bControllerReady
		|| !Receipt.bPawnReady
		|| !Receipt.bInputRestored)
	{
		OutDiagnostic = TEXT("RuntimeReady lacks one correlated M01 World, player, input, map or authority-Run identity fact.");
		return false;
	}
	return true;
}

bool Fdemo_map0909BRunStartCoordinator::StartM01Run(
	FString& OutPlayerFeedback)
{
	OutPlayerFeedback.Reset();
	if (!GameMode.IsValid() || !Controller.IsValid() || !M01Adapter.IsValid())
	{
		OutPlayerFeedback = TEXT("远征启动器尚未完成初始化。请返回宗门后重试。");
		return false;
	}
	if (State != Edemo_map0909BTopState::AtSect)
	{
		OutPlayerFeedback = TEXT("当前部署尚未结束，不能重复启动 M01。 ");
		return false;
	}
	BeginAttempt();
	Transition(Edemo_map0909BTopState::PreparingStart, TEXT("PreparingStart"));
	Fdemo_map0909BM01RuntimeReceipt ActivationReceipt;
	if (!M01Adapter->BeginActivation(
		LastDiagnostic.StartAttemptId, ActivationReceipt))
	{
		RecordRuntimeReceipt(ActivationReceipt);
		return ReturnToSectAfterTechnicalFailure(
			ActivationReceipt.FailureClass.IsEmpty() ? TEXT("M01ActivationRejected")
				: ActivationReceipt.FailureClass,
			ActivationReceipt.Detail, OutPlayerFeedback);
	}
	RecordRuntimeReceipt(ActivationReceipt);
	Transition(Edemo_map0909BTopState::ActivatingWorld, TEXT("ActivationRequestAccepted"));
	if (!Controller->RestoreGameplayControlForNewRun())
	{
		return ReturnToSectAfterTechnicalFailure(
			TEXT("GameplayInputRestoreFailed"),
			TEXT("M01 activation completed but the player controller did not restore GameOnly input."),
			OutPlayerFeedback);
	}
	Fdemo_map0909BM01RuntimeReceipt ReadyReceipt;
	if (!M01Adapter->ConfirmRuntimeReady(LastDiagnostic.StartAttemptId, ReadyReceipt))
	{
		RecordRuntimeReceipt(ReadyReceipt);
		return ReturnToSectAfterTechnicalFailure(
			ReadyReceipt.FailureClass.IsEmpty() ? TEXT("RuntimeReadyRejected")
				: ReadyReceipt.FailureClass,
			ReadyReceipt.Detail, OutPlayerFeedback);
	}
	RecordRuntimeReceipt(ReadyReceipt);
	FString RuntimeValidation;
	if (!ValidateRuntimeReady(ReadyReceipt, RuntimeValidation))
	{
		return ReturnToSectAfterTechnicalFailure(
			TEXT("RuntimeReadyCorrelationInvalid"), RuntimeValidation, OutPlayerFeedback);
	}

	Transition(Edemo_map0909BTopState::InRun, TEXT("RuntimeReady->InRun"));
	// World confirmation is a read-only authority comparison. It cannot veto the
	// now-confirmed deployment and never writes the retired Code B document.
	GameMode->Observe0909BConfirmedRun(
		AttemptRunCorrelation, LastDiagnostic.AuthorityDiagnostic);
	LastDiagnostic.FailureClass = TEXT("None");
	LastDiagnostic.Detail = TEXT("M01 RuntimeReady was correlated before InRun; world confirmation then re-read the same Shanmen authority identity without a legacy write.");
	OutPlayerFeedback = TEXT("M01 已部署。真实世界、Run 身份与仓库锁定现在由同一协调器确认。");
	UE_LOG(Logdemo_map, Log, TEXT("0_0_9BFIX_RUN_COORDINATOR Event=DeploymentSucceeded %s"),
		*LastDiagnostic.ToLogString());
	return true;
}

bool Fdemo_map0909BRunStartCoordinator::ReturnToSectAfterTechnicalFailure(
	const FString& FailureClass,
	const FString& Detail,
	FString& OutPlayerFeedback)
{
	LastDiagnostic.FailureClass = FailureClass;
	LastDiagnostic.Detail = Detail;
	Transition(Edemo_map0909BTopState::TechnicalStartFailure,
		TEXT("TechnicalStartFailure"));

	FString RollbackDiagnostic;
	const FGuid AttemptRunId = LastDiagnostic.RunId;
	const bool bAttemptCreatedRun = AttemptRunId.IsValid();
	const bool bRolledBack = M01Adapter.IsValid()
		&& M01Adapter->CancelAttempt(LastDiagnostic.StartAttemptId, RollbackDiagnostic);
	const FGuid RecoverableRunId = bRolledBack && GameMode.IsValid()
		? GameMode->Get0909BRecoverableRunId() : FGuid();
	const bool bAuthorityIdentityConsistent = bAttemptCreatedRun
		? RecoverableRunId == AttemptRunId
		: !RecoverableRunId.IsValid();
	if (bRolledBack && bAuthorityIdentityConsistent)
	{
		if (RecoverableRunId.IsValid())
		{
			LastDiagnostic.RunId = RecoverableRunId;
		}
		else
		{
			LastDiagnostic.ReleasedRunId = LastDiagnostic.RunId;
			LastDiagnostic.RunId.Invalidate();
		}
		Transition(Edemo_map0909BTopState::AtSect, TEXT("AtSect"));
		LastDiagnostic.Detail += TEXT(" | ") + RollbackDiagnostic;
		LastDiagnostic.AuthorityDiagnostic = RecoverableRunId.IsValid()
			? TEXT("Durable Shanmen Run correlation retained for exact replay.")
			: TEXT("No durable Run was created before technical failure.");
		OutPlayerFeedback = RecoverableRunId.IsValid()
			? TEXT("M01 世界未能启动；瞬态 Runtime 已清理，原远征身份与物资仍由 ShanmenItems 保留。再次开始将恢复同一远征。")
			: TEXT("M01 未能启动，已作为技术失败安全返回宗门；没有生成玩家放弃记录。");
		UE_LOG(Logdemo_map, Warning,
			TEXT("0_0_9BFIX_RUN_COORDINATOR Event=TechnicalFailureRecovered %s"),
			*LastDiagnostic.ToLogString());
		return false;
	}

	LastDiagnostic.Detail += FString::Printf(
		TEXT(" | Rollback=%s ExpectedRunId=%s RecoverableRunId=%s"),
		*RollbackDiagnostic,
		*AttemptRunId.ToString(EGuidFormats::DigitsWithHyphens),
		*RecoverableRunId.ToString(EGuidFormats::DigitsWithHyphens));
	OutPlayerFeedback = TEXT("M01 启动遇到技术错误，自动回滚未完成；请保留诊断并停止继续部署。");
	UE_LOG(Logdemo_map, Error,
		TEXT("0_0_9BFIX_RUN_COORDINATOR Event=TechnicalFailureBlocked %s"),
		*LastDiagnostic.ToLogString());
	return false;
}
