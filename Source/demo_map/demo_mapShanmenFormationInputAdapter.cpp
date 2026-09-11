#include "demo_mapShanmenFormationInputAdapter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EStartStatus = Edemo_mapShanmenFormationStartInputStatus;
	using EAnchorStatus = Edemo_mapShanmenFormationAnchorInputStatus;

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool HasNoStartEvidence(
		const Fdemo_mapShanmenFormationStartInputResult& Result)
	{
		return !Result.IntentId.IsValid()
			&& !Result.Sample.IsValid()
			&& !Result.Intent.IsValid()
			&& !Result.Lifecycle.IsAccepted();
	}

	bool HasValidStartEnvelope(
		const Fdemo_mapShanmenFormationStartInputResult& Result)
	{
		if (!Result.RunId.IsValid()
			|| !Result.InputEventId.IsValid()
			|| !Result.Sample.IsValid()
			|| !Result.Intent.IsValid()
			|| Result.IntentId
				!= Fdemo_mapShanmenFormationInputAdapter::MakeIntentId(
					Result.RunId,
					Result.InputEventId))
		{
			return false;
		}

		Fdemo_mapShanmenFormationIntent Expected;
		return Fdemo_mapShanmenFormationIntent::TryCapture(
				Result.IntentId,
				Result.RunId,
				Result.Sample.GetDiagram(),
				Result.Sample.GetOrigin(),
				Result.Sample.GetForward(),
				Expected)
			&& Result.Intent.Matches(Expected);
	}

	bool HasNoAnchorEvidence(
		const Fdemo_mapShanmenFormationAnchorInputResult& Result)
	{
		return !Result.AttemptId.IsValid()
			&& !Result.Sample.IsValid()
			&& !Result.Operation.IsValid()
			&& !Result.Lifecycle.IsSuccess();
	}

	bool HasValidAnchorEnvelope(
		const Fdemo_mapShanmenFormationAnchorInputResult& Result)
	{
		return Result.RunId.IsValid()
			&& Result.InputEventId.IsValid()
			&& Result.Sample.IsValid()
			&& Result.AttemptId
				== Fdemo_mapShanmenFormationInputAdapter::
					MakeAnchorAttemptId(
						Result.RunId,
						Result.InputEventId)
			&& Result.Operation.IsValid()
			&& Result.Operation.GetRunId() == Result.RunId
			&& Result.Operation.GetAnchorDefinitionId()
				== Result.Sample.GetAnchorDefinitionId()
			&& Result.Operation.GetAttemptId() == Result.AttemptId;
	}
}

bool Fdemo_mapShanmenFormationStartInputSample::TryCapture(
	const FShanmenFormationDiagramDefinition& RequestedDiagram,
	const FVector& RequestedOrigin,
	const FVector& RequestedForward,
	Fdemo_mapShanmenFormationStartInputSample& OutSample)
{
	OutSample = Fdemo_mapShanmenFormationStartInputSample();
	const FVector PlanarForward(
		RequestedForward.X,
		RequestedForward.Y,
		0.0);
	if (!RequestedDiagram.IsValid()
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedForward)
		|| PlanarForward.IsNearlyZero())
	{
		return false;
	}

	OutSample.Diagram = RequestedDiagram;
	OutSample.Origin = RequestedOrigin;
	OutSample.Forward = PlanarForward.GetSafeNormal();
	if (!OutSample.IsValid())
	{
		OutSample = Fdemo_mapShanmenFormationStartInputSample();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenFormationStartInputSample::IsValid() const
{
	return Diagram.IsValid()
		&& IsFiniteVector(Origin)
		&& IsFiniteVector(Forward)
		&& Forward.Z == 0.0
		&& Forward.IsNormalized();
}

bool Fdemo_mapShanmenFormationAnchorInputSample::TryCapture(
	const FName RequestedAnchorDefinitionId,
	Fdemo_mapShanmenFormationAnchorInputSample& OutSample)
{
	OutSample = Fdemo_mapShanmenFormationAnchorInputSample();
	if (RequestedAnchorDefinitionId.IsNone())
	{
		return false;
	}
	OutSample.AnchorDefinitionId = RequestedAnchorDefinitionId;
	return OutSample.IsValid();
}

bool Fdemo_mapShanmenFormationStartInputResult::IsValid() const
{
	if (Status == EStartStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| SampleCount < 0 || SampleCount > 1
		|| LifecycleInvocationCount < 0
		|| LifecycleInvocationCount > 1)
	{
		return false;
	}

	switch (Status)
	{
	case EStartStatus::GameplayBlocked:
	case EStartStatus::LifecycleUnavailable:
		return SampleCount == 0
			&& LifecycleInvocationCount == 0
			&& HasNoStartEvidence(*this);

	case EStartStatus::RunUnavailable:
		return !RunId.IsValid()
			&& SampleCount == 0
			&& LifecycleInvocationCount == 0
			&& HasNoStartEvidence(*this);

	case EStartStatus::EventIdentityInvalid:
		return RunId.IsValid()
			&& !InputEventId.IsValid()
			&& SampleCount == 0
			&& LifecycleInvocationCount == 0
			&& HasNoStartEvidence(*this);

	case EStartStatus::SampleRejected:
		return RunId.IsValid()
			&& InputEventId.IsValid()
			&& SampleCount == 1
			&& LifecycleInvocationCount == 0
			&& HasNoStartEvidence(*this);

	case EStartStatus::IntentCaptureRejected:
		return RunId.IsValid()
			&& InputEventId.IsValid()
			&& SampleCount == 1
			&& LifecycleInvocationCount == 0
			&& Sample.IsValid()
			&& IntentId.IsValid()
			&& !Intent.IsValid()
			&& !Lifecycle.IsAccepted();

	case EStartStatus::LifecycleRejected:
		return SampleCount == 1
			&& LifecycleInvocationCount == 1
			&& HasValidStartEnvelope(*this)
			&& !Lifecycle.IsAccepted();

	case EStartStatus::Applied:
		return SampleCount == 1
			&& LifecycleInvocationCount == 1
			&& HasValidStartEnvelope(*this)
			&& Lifecycle.IsAccepted()
			&& Lifecycle.IntentId == IntentId
			&& Lifecycle.RunId == RunId;

	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationStartInputResult::IsAccepted() const
{
	return Status == EStartStatus::Applied && IsValid();
}

bool Fdemo_mapShanmenFormationAnchorInputResult::IsValid() const
{
	if (Status == EAnchorStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| SampleCount < 0 || SampleCount > 1
		|| LifecycleInvocationCount < 0
		|| LifecycleInvocationCount > 1)
	{
		return false;
	}

	switch (Status)
	{
	case EAnchorStatus::GameplayBlocked:
	case EAnchorStatus::LifecycleUnavailable:
		return SampleCount == 0
			&& LifecycleInvocationCount == 0
			&& HasNoAnchorEvidence(*this);

	case EAnchorStatus::RunUnavailable:
		return !RunId.IsValid()
			&& SampleCount == 0
			&& LifecycleInvocationCount == 0
			&& HasNoAnchorEvidence(*this);

	case EAnchorStatus::EventIdentityInvalid:
		return RunId.IsValid()
			&& !InputEventId.IsValid()
			&& SampleCount == 0
			&& LifecycleInvocationCount == 0
			&& HasNoAnchorEvidence(*this);

	case EAnchorStatus::SampleRejected:
		return RunId.IsValid()
			&& InputEventId.IsValid()
			&& SampleCount == 1
			&& LifecycleInvocationCount == 0
			&& HasNoAnchorEvidence(*this);

	case EAnchorStatus::OperationCaptureRejected:
		return RunId.IsValid()
			&& InputEventId.IsValid()
			&& SampleCount == 1
			&& LifecycleInvocationCount == 0
			&& Sample.IsValid()
			&& AttemptId.IsValid()
			&& !Operation.IsValid()
			&& !Lifecycle.IsSuccess();

	case EAnchorStatus::LifecycleRejected:
		return SampleCount == 1
			&& LifecycleInvocationCount == 1
			&& HasValidAnchorEnvelope(*this)
			&& !Lifecycle.IsSuccess();

	case EAnchorStatus::Applied:
		return SampleCount == 1
			&& LifecycleInvocationCount == 1
			&& HasValidAnchorEnvelope(*this)
			&& Lifecycle.IsSuccess()
			&& Lifecycle.Operation.GetRunId() == Operation.GetRunId()
			&& Lifecycle.Operation.GetAnchorDefinitionId()
				== Operation.GetAnchorDefinitionId()
			&& Lifecycle.Operation.GetAttemptId()
				== Operation.GetAttemptId();

	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationAnchorInputResult::IsAccepted() const
{
	return Status == EAnchorStatus::Applied && IsValid();
}

FGuid Fdemo_mapShanmenFormationInputAdapter::MakeIntentId(
	const FGuid& RunId,
	const FGuid& InputEventId)
{
	if (!RunId.IsValid() || !InputEventId.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("demo_map.Formation.StartInputIntent.r1")),
		{
			GuidDigits(RunId),
			GuidDigits(InputEventId)
		});
}

FGuid Fdemo_mapShanmenFormationInputAdapter::MakeAnchorAttemptId(
	const FGuid& RunId,
	const FGuid& InputEventId)
{
	if (!RunId.IsValid() || !InputEventId.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("demo_map.Formation.AnchorInputAttempt.r1")),
		{
			GuidDigits(RunId),
			GuidDigits(InputEventId)
		});
}

Fdemo_mapShanmenFormationStartInputResult
Fdemo_mapShanmenFormationInputAdapter::RouteStartInput(
	const bool bGameplayInputAllowed,
	const bool bLifecycleAvailable,
	const FGuid& RunId,
	const FGuid& InputEventId,
	FSampleStart SampleStart,
	FRouteStart RouteLifecycle)
{
	Fdemo_mapShanmenFormationStartInputResult Result;
	Result.RunId = RunId;
	Result.InputEventId = InputEventId;
	if (!bGameplayInputAllowed)
	{
		Result.Status = EStartStatus::GameplayBlocked;
		Result.Diagnostic =
			TEXT("Formation start is blocked by the current gameplay surface.");
		return Result;
	}
	if (!bLifecycleAvailable)
	{
		Result.Status = EStartStatus::LifecycleUnavailable;
		Result.Diagnostic =
			TEXT("Formation start requires the authoritative Run lifecycle route.");
		return Result;
	}
	if (!RunId.IsValid())
	{
		Result.Status = EStartStatus::RunUnavailable;
		Result.Diagnostic =
			TEXT("Formation start requires one valid Combat Run identity.");
		return Result;
	}
	if (!InputEventId.IsValid())
	{
		Result.Status = EStartStatus::EventIdentityInvalid;
		Result.Diagnostic =
			TEXT("Formation start requires one stable caller-owned event identity.");
		return Result;
	}

	Result.SampleCount = 1;
	Result.Sample = SampleStart();
	if (!Result.Sample.IsValid())
	{
		Result.Status = EStartStatus::SampleRejected;
		Result.Diagnostic =
			TEXT("Formation start received an invalid diagram or spatial sample.");
		return Result;
	}
	Result.IntentId = MakeIntentId(RunId, InputEventId);
	if (!Result.IntentId.IsValid()
		|| !Fdemo_mapShanmenFormationIntent::TryCapture(
			Result.IntentId,
			RunId,
			Result.Sample.GetDiagram(),
			Result.Sample.GetOrigin(),
			Result.Sample.GetForward(),
			Result.Intent))
	{
		Result.Status = EStartStatus::IntentCaptureRejected;
		Result.Diagnostic =
			TEXT("Formation start could not enter the immutable intent contract.");
		return Result;
	}

	Result.LifecycleInvocationCount = 1;
	Result.Lifecycle = RouteLifecycle(Result.Intent);
	Result.Status = Result.Lifecycle.IsAccepted()
		? EStartStatus::Applied
		: EStartStatus::LifecycleRejected;
	Result.Diagnostic = Result.Lifecycle.Diagnostic.IsEmpty()
		? TEXT("Formation lifecycle rejected the captured start intent.")
		: Result.Lifecycle.Diagnostic;
	return Result;
}

Fdemo_mapShanmenFormationAnchorInputResult
Fdemo_mapShanmenFormationInputAdapter::RouteAnchorInput(
	const bool bGameplayInputAllowed,
	const bool bLifecycleAvailable,
	const FGuid& RunId,
	const FGuid& InputEventId,
	FSampleAnchor SampleAnchor,
	FRouteAnchor RouteLifecycle)
{
	Fdemo_mapShanmenFormationAnchorInputResult Result;
	Result.RunId = RunId;
	Result.InputEventId = InputEventId;
	if (!bGameplayInputAllowed)
	{
		Result.Status = EAnchorStatus::GameplayBlocked;
		Result.Diagnostic =
			TEXT("Formation anchor is blocked by the current gameplay surface.");
		return Result;
	}
	if (!bLifecycleAvailable)
	{
		Result.Status = EAnchorStatus::LifecycleUnavailable;
		Result.Diagnostic =
			TEXT("Formation anchor requires the authoritative Run lifecycle route.");
		return Result;
	}
	if (!RunId.IsValid())
	{
		Result.Status = EAnchorStatus::RunUnavailable;
		Result.Diagnostic =
			TEXT("Formation anchor requires one valid Combat Run identity.");
		return Result;
	}
	if (!InputEventId.IsValid())
	{
		Result.Status = EAnchorStatus::EventIdentityInvalid;
		Result.Diagnostic =
			TEXT("Formation anchor requires one stable caller-owned event identity.");
		return Result;
	}

	Result.SampleCount = 1;
	Result.Sample = SampleAnchor();
	if (!Result.Sample.IsValid())
	{
		Result.Status = EAnchorStatus::SampleRejected;
		Result.Diagnostic =
			TEXT("Formation anchor received an invalid anchor selection sample.");
		return Result;
	}
	Result.AttemptId = MakeAnchorAttemptId(RunId, InputEventId);
	if (!Result.AttemptId.IsValid()
		|| !Fdemo_mapShanmenFormationAnchorOperation::TryCapture(
			RunId,
			Result.Sample.GetAnchorDefinitionId(),
			Result.AttemptId,
			Result.Operation))
	{
		Result.Status = EAnchorStatus::OperationCaptureRejected;
		Result.Diagnostic =
			TEXT("Formation anchor could not enter the immutable operation contract.");
		return Result;
	}

	Result.LifecycleInvocationCount = 1;
	Result.Lifecycle = RouteLifecycle(Result.Operation);
	Result.Status = Result.Lifecycle.IsSuccess()
		? EAnchorStatus::Applied
		: EAnchorStatus::LifecycleRejected;
	Result.Diagnostic = Result.Lifecycle.Diagnostic.IsEmpty()
		? TEXT("Formation lifecycle rejected the captured anchor operation.")
		: Result.Lifecycle.Diagnostic;
	return Result;
}
