#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationInfluenceConsumerWorldResolution.h"

#include "ShanmenWorldEntityRegistry.h"
#include "demo_mapAttributeComponent.h"

#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerWorldResolutionTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerWorldResolution.RegistryBacked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerWorldResolutionTest::RunTest(
	const FString&)
{
	const FGuid RunId(0xF8370001, 0, 0, 1);
	const FGuid ForeignRunId(0xF8370002, 0, 0, 1);
	const FGuid EntityId(0xF8370003, 0, 0, 1);
	FShanmenWorldEntityRegistry EmptyRegistry;
	FShanmenWorldEntityRegistry Registry;
	Udemo_mapAttributeComponent* GenericAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* ExactBodyAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* UnboundAttributes =
		NewObject<Udemo_mapAttributeComponent>();

	TestTrue(TEXT("Registry opens for the exact product Run"),
		Registry.TryBeginRun(RunId));
	TestTrue(TEXT("Caller binds the generic attribute capability explicitly"),
		Registry.BindObject(RunId, GenericAttributes, EntityId)
			== EShanmenWorldBindingResult::Bound);
	TestTrue(TEXT("Caller may bind an exact body capability explicitly"),
		Registry.BindObject(RunId, ExactBodyAttributes, EntityId, 7)
			== EShanmenWorldBindingResult::Bound);

	const auto InvalidExpectedRun =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, FGuid(), GenericAttributes);
	const auto RegistryUnavailable =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			EmptyRegistry, RunId, GenericAttributes);
	const auto ForeignRun =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, ForeignRunId, GenericAttributes);
	const auto MissingComponent =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, RunId, nullptr);
	const auto InvalidBody =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, RunId, GenericAttributes, INDEX_NONE - 1);
	const auto Unbound =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, RunId, UnboundAttributes);
	const auto WrongBody =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, RunId, ExactBodyAttributes, 8);
	const auto GenericResolved =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, RunId, GenericAttributes);
	const auto GenericReplay =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, RunId, GenericAttributes);
	const auto ExactBodyResolved =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, RunId, ExactBodyAttributes, 7);

	TestTrue(TEXT("Invalid inputs expose exact fail-closed states"),
		InvalidExpectedRun.Status
			== Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				ExpectedRunIdInvalid
			&& !InvalidExpectedRun.bRegistryChecked
			&& RegistryUnavailable.Status
				== Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
					RegistryUnavailable
			&& RegistryUnavailable.bRegistryChecked
			&& ForeignRun.Status
				== Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
					RegistryRunMismatch
			&& MissingComponent.Status
				== Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
					AttributeComponentUnavailable
			&& InvalidBody.Status
				== Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
					BodyIndexInvalid);
	TestTrue(TEXT("Unbound and wrong-body capabilities cannot mint identity"),
		Unbound.Status
			== Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
				EntityNotFound
			&& !Unbound.ResolvedEntityId.IsValid()
			&& WrongBody.Status
				== Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
					EntityNotFound
			&& !WrongBody.Resolution.IsValid());
	TestTrue(TEXT("Generic binding creates pointer-free resolution evidence"),
		GenericResolved.IsSuccess()
			&& GenericResolved.bRegistryChecked
			&& GenericResolved.ExpectedRunId == RunId
			&& GenericResolved.RegistryRunId == RunId
			&& GenericResolved.ResolvedEntityId == EntityId
			&& GenericResolved.Resolution.SubjectEntityId == EntityId
			&& GenericResolved.Resolution.AttributeComponentUniqueId
				== GenericAttributes->GetUniqueID());
	TestTrue(TEXT("Repeated resolution is deterministic and read-only"),
		GenericReplay.IsSuccess()
			&& GenericReplay.Resolution.ResolutionId
				== GenericResolved.Resolution.ResolutionId
			&& Registry.NumObjectBindings() == 2);
	TestTrue(TEXT("Exact body binding preserves body-specific identity"),
		ExactBodyResolved.IsSuccess()
			&& ExactBodyResolved.BodyIndex == 7
			&& ExactBodyResolved.ResolvedEntityId == EntityId
			&& ExactBodyResolved.Resolution.AttributeComponentUniqueId
				== ExactBodyAttributes->GetUniqueID());
	return true;
}

#endif
