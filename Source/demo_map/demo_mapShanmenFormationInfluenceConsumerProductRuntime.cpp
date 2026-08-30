#include "demo_mapShanmenFormationInfluenceConsumerProductRuntime.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FGuid MakeRuntimeId(const FGuid& BridgeId)
	{
		if (!BridgeId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerProductRuntime.r1"),
			{ BridgeId.ToString(EGuidFormats::Digits) });
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult::
IsSuccess() const
{
	if (!RuntimeId.IsValid())
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
		Activated:
		return Binding.IsSuccess() && Route.IsSuccess()
			&& Route.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					Routed
			&& bRuntimeStateChanged;
	case Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
		ActivationReplayed:
		return Binding.IsSuccess() && Route.IsSuccess()
			&& Route.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					TransactionReplayed
			&& !bRuntimeStateChanged;
	case Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
		Deactivated:
		return Route.IsSuccess()
			&& Route.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					Routed
			&& bRuntimeStateChanged;
	case Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
		DeactivationReplayed:
		return Route.IsSuccess()
			&& Route.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					TransactionReplayed
			&& !bRuntimeStateChanged;
	case Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
		TeardownReady:
		return !bRuntimeStateChanged;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryOpen(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime& OutRuntime)
{
	OutRuntime = Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime();
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime Candidate;
	if (!Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryOpen(
			ProductHost, Candidate.Bridge))
	{
		return false;
	}
	Candidate.RuntimeId = MakeRuntimeId(Candidate.Bridge.GetBridgeId());
	if (!Candidate.IsValid() || !Candidate.MatchesProductHost(ProductHost))
	{
		return false;
	}
	OutRuntime = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::CheckProductHost(
	const Fdemo_mapShanmenFormationProductHost& ProductHost) const
{
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult Result;
	Result.RuntimeId = RuntimeId;
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				RuntimeInvalid;
		Result.Diagnostic = TEXT("Consumer product runtime is not open or valid.");
		return Result;
	}
	if (!ProductHost.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				ProductHostInvalid;
		Result.Diagnostic = TEXT("Consumer runtime requires one valid ProductHost.");
		return Result;
	}
	if (!MatchesProductHost(ProductHost))
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				ProductIdentityMismatch;
		Result.Diagnostic = TEXT("Consumer runtime rejected a foreign product identity.");
		return Result;
	}
	if (ProductHost.GetSession().IsTerminal())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				ProductTerminal;
		Result.Diagnostic = TEXT("A terminal formation product cannot run consumers.");
		return Result;
	}
	Result.Status =
		Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
			TeardownReady;
	return Result;
}

Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryActivate(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	const FGuid& SubjectEntityId,
	Udemo_mapAttributeComponent* AttributeComponent,
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& ApplyCommand)
{
	auto Result = CheckProductHost(ProductHost);
	if (Result.Status
		!= Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
			TeardownReady)
	{
		return Result;
	}
	if (!ApplyCommand.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				CommandInvalid;
		Result.Diagnostic = TEXT("Consumer activation requires one valid command.");
		return Result;
	}
	if (ApplyCommand.GetOperation()
		!= Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply)
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				CommandOperationMismatch;
		Result.Diagnostic = TEXT("Consumer activation requires an Apply command.");
		return Result;
	}
	if (!Bridge.MatchesCommandProductIdentity(ApplyCommand))
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				CommandProductIdentityMismatch;
		Result.Diagnostic =
			TEXT("Consumer activation rejected a foreign product command.");
		return Result;
	}
	if (!SubjectEntityId.IsValid()
		|| ApplyCommand.GetProjection().GetLease().Key.SubjectEntityId
			!= SubjectEntityId)
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				CommandSubjectMismatch;
		Result.Diagnostic =
			TEXT("Consumer activation subject does not match the frozen command.");
		return Result;
	}

	Result.Binding = Bridge.TryBindSubject(
		ProductHost, SubjectEntityId, AttributeComponent);
	Result.bRuntimeStateChanged = Result.Binding.bBridgeStateChanged;
	if (!Result.Binding.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				BindingRejected;
		Result.Diagnostic = Result.Binding.Diagnostic;
		return Result;
	}

	Result.Route = Bridge.TryRoute(ProductHost, ApplyCommand);
	Result.bRuntimeStateChanged = Result.bRuntimeStateChanged
		|| Result.Route.bBridgeStateChanged;
	if (!Result.Route.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				RouteRejected;
		Result.Diagnostic = Result.Route.Diagnostic;
		return Result;
	}
	Result.Status = Result.Route.Status
		== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::Routed
		? Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
			Activated
		: Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
			ActivationReplayed;
	if (!IsValid() || !Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				StateInvalid;
		Result.Diagnostic = TEXT("Consumer activation violated runtime invariants.");
	}
	return Result;
}

Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryDeactivate(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& RemoveCommand)
{
	auto Result = CheckProductHost(ProductHost);
	if (Result.Status
		!= Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
			TeardownReady)
	{
		return Result;
	}
	if (!RemoveCommand.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				CommandInvalid;
		Result.Diagnostic = TEXT("Consumer deactivation requires one valid command.");
		return Result;
	}
	if (RemoveCommand.GetOperation()
		!= Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove)
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				CommandOperationMismatch;
		Result.Diagnostic = TEXT("Consumer deactivation requires a Remove command.");
		return Result;
	}
	if (!Bridge.MatchesCommandProductIdentity(RemoveCommand))
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				CommandProductIdentityMismatch;
		Result.Diagnostic =
			TEXT("Consumer deactivation rejected a foreign product command.");
		return Result;
	}

	Result.Route = Bridge.TryRoute(ProductHost, RemoveCommand);
	Result.bRuntimeStateChanged = Result.Route.bBridgeStateChanged;
	if (!Result.Route.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				RouteRejected;
		Result.Diagnostic = Result.Route.Diagnostic;
		return Result;
	}
	Result.Status = Result.Route.Status
		== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::Routed
		? Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
			Deactivated
		: Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
			DeactivationReplayed;
	if (!IsValid() || !Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				StateInvalid;
		Result.Diagnostic = TEXT("Consumer deactivation violated runtime invariants.");
	}
	return Result;
}

Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::
CheckTeardownReady(
	const Fdemo_mapShanmenFormationProductHost& ProductHost) const
{
	auto Result = CheckProductHost(ProductHost);
	if (Result.Status
		!= Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
			TeardownReady)
	{
		return Result;
	}
	if (!Bridge.IsDrained())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				ActiveApplicationsRemain;
		Result.Diagnostic =
			TEXT("Consumer applications must be removed before product teardown.");
		return Result;
	}
	return Result;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::IsValid() const
{
	return Bridge.IsValid()
		&& RuntimeId == MakeRuntimeId(Bridge.GetBridgeId());
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::
MatchesProductHost(
	const Fdemo_mapShanmenFormationProductHost& ProductHost) const
{
	return IsValid() && Bridge.MatchesProductHost(ProductHost);
}
