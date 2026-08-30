#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceConsumerCommandHost.h"
#include "demo_mapShanmenFormationProductHost.h"

class Udemo_mapAttributeComponent;

enum class Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus
	: uint8
{
	Bound,
	BindingReplayed,
	BridgeInvalid,
	ProductHostInvalid,
	ProductIdentityMismatch,
	ProductTerminal,
	BindingRejected,
	StateInvalid
};

/** Product-scoped result around one explicit P8.29 subject binding. */
struct Fdemo_mapShanmenFormationInfluenceConsumerProductBindingResult
{
	Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
			BridgeInvalid;
	FString Diagnostic;
	FGuid BridgeId;
	bool bBridgeStateChanged = false;
	Fdemo_mapShanmenFormationInfluenceConsumerBindingResult Binding;

	bool IsSuccess() const;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus
	: uint8
{
	Routed,
	TransactionReplayed,
	BridgeInvalid,
	ProductHostInvalid,
	ProductIdentityMismatch,
	ProductTerminal,
	CommandProductIdentityMismatch,
	RouteRejected,
	StateInvalid
};

/** Product-scoped result around one direct P8.29 command route. */
struct Fdemo_mapShanmenFormationInfluenceConsumerProductRouteResult
{
	Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
			BridgeInvalid;
	FString Diagnostic;
	FGuid BridgeId;
	bool bBridgeStateChanged = false;
	Fdemo_mapShanmenFormationInfluenceConsumerRouteResult Route;

	bool IsSuccess() const;
};

/**
 * Explicit bridge between one existing formation ProductHost lifecycle and
 * the P8.29 run-scoped consumer command host.
 *
 * Opening freezes product correlation, action, deployment, and content
 * identity. Every bind/route call must present that exact caller-owned
 * ProductHost. The bridge never discovers components, retries commands,
 * schedules work, owns World/Actor state, or mirrors coordinator history.
 */
class Fdemo_mapShanmenFormationInfluenceConsumerProductBridge
{
public:
	static bool TryOpen(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		Fdemo_mapShanmenFormationInfluenceConsumerProductBridge& OutBridge);

	Fdemo_mapShanmenFormationInfluenceConsumerProductBindingResult
	TryBindSubject(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		const FGuid& SubjectEntityId,
		Udemo_mapAttributeComponent* AttributeComponent);
	Fdemo_mapShanmenFormationInfluenceConsumerProductRouteResult TryRoute(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command);

	bool IsValid() const;
	bool MatchesProductHost(
		const Fdemo_mapShanmenFormationProductHost& ProductHost) const;
	bool MatchesCommandProductIdentity(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command) const;
	bool IsDrained() const { return IsValid() && CommandHost.IsDrained(); }
	int32 GetBindingCount() const
	{
		return IsValid() ? CommandHost.GetBindingCount() : INDEX_NONE;
	}
	int32 GetActiveApplicationCount() const
	{
		return IsValid()
			? CommandHost.GetActiveApplicationCount()
			: INDEX_NONE;
	}
	int32 GetCompletedTransactionCount() const
	{
		return IsValid()
			? CommandHost.GetCompletedTransactionCount()
			: INDEX_NONE;
	}

	const FGuid& GetBridgeId() const { return BridgeId; }
	const Fdemo_mapShanmenRunCorrelation& GetCorrelation() const
	{
		return Correlation;
	}
	const FGuid& GetActionActivationId() const
	{
		return ActionActivationId;
	}
	const FGuid& GetDeploymentId() const { return DeploymentId; }
	const Fdemo_mapShanmenFormationInfluenceConsumerCommandHost&
	GetCommandHost() const
	{
		return CommandHost;
	}

private:
	Fdemo_mapShanmenRunCorrelation Correlation;
	FGuid ActionActivationId;
	FGuid ActionSourceEntityId;
	FGuid DeploymentId;
	FShanmenContentStamp Content;
	FGuid BridgeId;
	Fdemo_mapShanmenFormationInfluenceConsumerCommandHost CommandHost;
};
