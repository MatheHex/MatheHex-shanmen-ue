#include "demo_mapShanmenFormationInfluenceConsumerProductBridge.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version && Left.Digest == Right.Digest;
	}

	FGuid MakeBridgeId(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FGuid& ActionActivationId,
		const FGuid& ActionSourceEntityId,
		const FGuid& DeploymentId,
		const FShanmenContentStamp& Content)
	{
		if (!Correlation.IsValid() || !ActionActivationId.IsValid()
			|| !ActionSourceEntityId.IsValid() || !DeploymentId.IsValid()
			|| !Content.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerProductBridge.r1"),
			{
				GuidDigits(Correlation.CorrelationId),
				GuidDigits(Correlation.ActiveRunId),
				GuidDigits(Correlation.OwnerId),
				GuidDigits(ActionActivationId),
				GuidDigits(ActionSourceEntityId),
				GuidDigits(DeploymentId),
				Content.Version.ToString(), Content.Digest
			});
	}

	enum class EProductCheck : uint8
	{
		Accepted,
		BridgeInvalid,
		ProductHostInvalid,
		ProductIdentityMismatch,
		ProductTerminal
	};

	EProductCheck CheckProductHost(
		const Fdemo_mapShanmenFormationInfluenceConsumerProductBridge& Bridge,
		const Fdemo_mapShanmenFormationProductHost& ProductHost)
	{
		if (!Bridge.IsValid())
		{
			return EProductCheck::BridgeInvalid;
		}
		if (!ProductHost.IsValid())
		{
			return EProductCheck::ProductHostInvalid;
		}
		if (!Bridge.MatchesProductHost(ProductHost))
		{
			return EProductCheck::ProductIdentityMismatch;
		}
		if (ProductHost.GetSession().IsTerminal())
		{
			return EProductCheck::ProductTerminal;
		}
		return EProductCheck::Accepted;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductBindingResult::
IsSuccess() const
{
	if (!BridgeId.IsValid() || !Binding.IsSuccess())
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
		Bound:
		return Binding.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::Bound
			&& Binding.bHostStateCommitted && bBridgeStateChanged;
	case Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
		BindingReplayed:
		return Binding.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					BindingReplayed
			&& !Binding.bHostStateCommitted && !bBridgeStateChanged;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductRouteResult::
IsSuccess() const
{
	if (!BridgeId.IsValid() || !Route.IsSuccess())
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::Routed:
		return Route.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::Routed
			&& Route.bHostStateChanged && bBridgeStateChanged;
	case Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
		TransactionReplayed:
		return Route.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
					TransactionReplayed
			&& !Route.bHostStateChanged && !bBridgeStateChanged;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryOpen(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge& OutBridge)
{
	OutBridge =
		Fdemo_mapShanmenFormationInfluenceConsumerProductBridge();
	if (!ProductHost.IsValid() || ProductHost.GetSession().IsTerminal())
	{
		return false;
	}

	const auto& Session = ProductHost.GetSession();
	const auto& Action = Session.GetActionRuntime().GetAction();
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge Candidate;
	Candidate.Correlation = Session.GetCorrelation();
	Candidate.ActionActivationId = Action.GetActivationId();
	Candidate.ActionSourceEntityId = Action.GetSourceEntityId();
	Candidate.DeploymentId = Session.GetDeployment().GetDeploymentId();
	Candidate.Content = Action.GetContent();
	Candidate.BridgeId = MakeBridgeId(
		Candidate.Correlation, Candidate.ActionActivationId,
		Candidate.ActionSourceEntityId, Candidate.DeploymentId,
		Candidate.Content);
	if (!Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::TryOpen(
			Candidate.Correlation.ActiveRunId,
			Candidate.Content, Candidate.CommandHost)
		|| !Candidate.IsValid()
		|| !Candidate.MatchesProductHost(ProductHost))
	{
		return false;
	}
	OutBridge = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenFormationInfluenceConsumerProductBindingResult
Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryBindSubject(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	const FGuid& SubjectEntityId,
	Udemo_mapAttributeComponent* AttributeComponent)
{
	Fdemo_mapShanmenFormationInfluenceConsumerProductBindingResult Result;
	Result.BridgeId = BridgeId;
	switch (CheckProductHost(*this, ProductHost))
	{
	case EProductCheck::BridgeInvalid:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
				BridgeInvalid;
		Result.Diagnostic = TEXT("Consumer product bridge is not open or valid.");
		return Result;
	case EProductCheck::ProductHostInvalid:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
				ProductHostInvalid;
		Result.Diagnostic = TEXT("Consumer binding requires one valid ProductHost.");
		return Result;
	case EProductCheck::ProductIdentityMismatch:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
				ProductIdentityMismatch;
		Result.Diagnostic = TEXT("Consumer binding rejected a foreign product identity.");
		return Result;
	case EProductCheck::ProductTerminal:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
				ProductTerminal;
		Result.Diagnostic = TEXT("A terminal formation product cannot accept bindings.");
		return Result;
	default:
		break;
	}

	Result.Binding = CommandHost.TryBindSubject(
		SubjectEntityId, AttributeComponent);
	Result.Diagnostic = Result.Binding.Diagnostic;
	if (!Result.Binding.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
				BindingRejected;
		return Result;
	}
	Result.bBridgeStateChanged = Result.Binding.bHostStateCommitted;
	Result.Status = Result.Binding.Status
		== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::Bound
		? Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::Bound
		: Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
			BindingReplayed;
	if (!IsValid() || !Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
				StateInvalid;
		Result.Diagnostic =
			TEXT("Consumer binding failed product-bridge invariants.");
		Result.bBridgeStateChanged = false;
	}
	return Result;
}

Fdemo_mapShanmenFormationInfluenceConsumerProductRouteResult
Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryRoute(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command)
{
	Fdemo_mapShanmenFormationInfluenceConsumerProductRouteResult Result;
	Result.BridgeId = BridgeId;
	switch (CheckProductHost(*this, ProductHost))
	{
	case EProductCheck::BridgeInvalid:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
				BridgeInvalid;
		Result.Diagnostic = TEXT("Consumer product bridge is not open or valid.");
		return Result;
	case EProductCheck::ProductHostInvalid:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
				ProductHostInvalid;
		Result.Diagnostic = TEXT("Consumer routing requires one valid ProductHost.");
		return Result;
	case EProductCheck::ProductIdentityMismatch:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
				ProductIdentityMismatch;
		Result.Diagnostic = TEXT("Consumer routing rejected a foreign product identity.");
		return Result;
	case EProductCheck::ProductTerminal:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
				ProductTerminal;
		Result.Diagnostic = TEXT("A terminal formation product cannot route commands.");
		return Result;
	default:
		break;
	}
	if (Command.IsValid() && !MatchesCommandProductIdentity(Command))
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
				CommandProductIdentityMismatch;
		Result.Diagnostic =
			TEXT("Consumer routing rejected a command from a foreign product identity.");
		return Result;
	}

	Result.Route = CommandHost.TryRoute(Command);
	Result.Diagnostic = Result.Route.Diagnostic;
	if (!Result.Route.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
				RouteRejected;
		return Result;
	}
	Result.bBridgeStateChanged = Result.Route.bHostStateChanged;
	Result.Status = Result.Route.Status
		== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::Routed
		? Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::Routed
		: Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
			TransactionReplayed;
	if (!IsValid() || !Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
				StateInvalid;
		Result.Diagnostic =
			TEXT("Consumer route failed product-bridge invariants.");
		Result.bBridgeStateChanged = false;
	}
	return Result;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::IsValid() const
{
	return Correlation.IsValid() && ActionActivationId.IsValid()
		&& ActionSourceEntityId.IsValid() && DeploymentId.IsValid()
		&& Content.IsValid() && CommandHost.IsValid()
		&& CommandHost.GetRunId() == Correlation.ActiveRunId
		&& SameContent(CommandHost.GetContent(), Content)
		&& BridgeId == MakeBridgeId(
			Correlation, ActionActivationId, ActionSourceEntityId,
			DeploymentId, Content);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::
MatchesProductHost(
	const Fdemo_mapShanmenFormationProductHost& ProductHost) const
{
	if (!IsValid() || !ProductHost.IsValid())
	{
		return false;
	}
	const auto& Session = ProductHost.GetSession();
	const auto& Action = Session.GetActionRuntime().GetAction();
	return Session.GetCorrelation() == Correlation
		&& Action.GetActivationId() == ActionActivationId
		&& Action.GetSourceEntityId() == ActionSourceEntityId
		&& Session.GetDeployment().GetDeploymentId() == DeploymentId
		&& SameContent(Action.GetContent(), Content);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::
MatchesCommandProductIdentity(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command) const
{
	if (!IsValid() || !Command.IsValid())
	{
		return false;
	}
	const auto& LeaseKey = Command.GetProjection().GetLease().Key;
	return LeaseKey.RunId == Correlation.ActiveRunId
		&& LeaseKey.OwnerId == Correlation.OwnerId
		&& LeaseKey.SourceEntityId == ActionSourceEntityId
		&& LeaseKey.DeploymentId == DeploymentId
		&& SameContent(LeaseKey.Content, Content);
}
