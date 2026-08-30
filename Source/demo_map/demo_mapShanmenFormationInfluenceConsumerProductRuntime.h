#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceConsumerProductBridge.h"

class Udemo_mapAttributeComponent;

enum class Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus
	: uint8
{
	Activated,
	ActivationReplayed,
	Deactivated,
	DeactivationReplayed,
	TeardownReady,
	RuntimeInvalid,
	ProductHostInvalid,
	ProductIdentityMismatch,
	ProductTerminal,
	CommandInvalid,
	CommandOperationMismatch,
	CommandSubjectMismatch,
	CommandProductIdentityMismatch,
	BindingRejected,
	RouteRejected,
	ActiveApplicationsRemain,
	StateInvalid
};

/** One caller-driven consumer lifecycle operation with nested bridge evidence. */
struct Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
{
	Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
			RuntimeInvalid;
	FString Diagnostic;
	FGuid RuntimeId;
	bool bRuntimeStateChanged = false;
	Fdemo_mapShanmenFormationInfluenceConsumerProductBindingResult Binding;
	Fdemo_mapShanmenFormationInfluenceConsumerProductRouteResult Route;

	bool IsSuccess() const;
};

/**
 * Explicit Apply/Remove lifecycle facade over one P8.31 product bridge.
 *
 * The caller supplies the exact ProductHost, subject component, and frozen
 * command on every operation. Activation preflights identity and operation
 * before the append-only binding is created. Deactivation routes one explicit
 * Remove. Teardown readiness is an explicit drained check; this runtime never
 * discovers components, synthesizes commands, auto-removes, retries, schedules,
 * persists, or owns ProductHost/World/Actor state. The readiness check accepts
 * the same terminal ProductHost solely for exact forward World-teardown
 * recovery; activation and deactivation continue to reject terminal products.
 */
class Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime
{
public:
	static bool TryOpen(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime& OutRuntime);

	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult TryActivate(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		const FGuid& SubjectEntityId,
		Udemo_mapAttributeComponent* AttributeComponent,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& ApplyCommand);
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult TryDeactivate(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& RemoveCommand);
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
	CheckTeardownReady(
		const Fdemo_mapShanmenFormationProductHost& ProductHost) const;

	bool IsValid() const;
	bool MatchesProductHost(
		const Fdemo_mapShanmenFormationProductHost& ProductHost) const;
	bool IsDrained() const { return IsValid() && Bridge.IsDrained(); }
	int32 GetBindingCount() const
	{
		return IsValid() ? Bridge.GetBindingCount() : INDEX_NONE;
	}
	int32 GetActiveApplicationCount() const
	{
		return IsValid() ? Bridge.GetActiveApplicationCount() : INDEX_NONE;
	}
	int32 GetCompletedTransactionCount() const
	{
		return IsValid() ? Bridge.GetCompletedTransactionCount() : INDEX_NONE;
	}
	const FGuid& GetRuntimeId() const { return RuntimeId; }
	const Fdemo_mapShanmenFormationInfluenceConsumerProductBridge&
	GetBridge() const
	{
		return Bridge;
	}

private:
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
	CheckProductHost(
		const Fdemo_mapShanmenFormationProductHost& ProductHost) const;

	FGuid RuntimeId;
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge Bridge;
};
