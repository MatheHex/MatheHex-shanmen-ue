#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenFormationMastery.h"

namespace
{
	FShanmenFormationMasteryPolicy MakePolicy(
		const EShanmenFormationMasteryTier Tier)
	{
		FShanmenFormationMasteryPolicy Policy;
		check(FShanmenFormationMasteryPolicy::TryCreate(Tier, Policy));
		return Policy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenFormationMasteryContractTest,
	"Shanmen.0_0_10.CombatRuntime.FormationMastery.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenFormationMasteryContractTest::RunTest(const FString&)
{
	FShanmenFormationMasteryPolicy Policy;
	TestFalse(TEXT("Default policy is invalid"), Policy.IsValid());

	for (const EShanmenFormationMasteryTier Tier : {
		EShanmenFormationMasteryTier::Beginner,
		EShanmenFormationMasteryTier::Intermediate,
		EShanmenFormationMasteryTier::Master })
	{
		TestTrue(TEXT("Each authored mastery tier captures immutably"),
			FShanmenFormationMasteryPolicy::TryCreate(Tier, Policy)
				&& Policy.IsValid()
				&& Policy.GetTier() == Tier);
	}

	const EShanmenFormationMasteryTier ForgedTier =
		static_cast<EShanmenFormationMasteryTier>(255);
	TestFalse(TEXT("Forged mastery tier fails closed"),
		FShanmenFormationMasteryPolicy::TryCreate(ForgedTier, Policy));
	TestFalse(TEXT("Rejected capture clears prior policy state"),
		Policy.IsValid());
	TestFalse(TEXT("Invalid tier sentinel is not an authored tier"),
		FShanmenFormationMasteryPolicy::IsTierValid(
			EShanmenFormationMasteryTier::Invalid));
	TestFalse(TEXT("Invalid delivery sentinel is not an operation"),
		FShanmenFormationMasteryPolicy::IsDeliveryModeValid(
			EShanmenFormationMaterialDeliveryMode::Invalid));
	TestFalse(TEXT("Forged delivery mode is rejected"),
		FShanmenFormationMasteryPolicy::IsDeliveryModeValid(
			static_cast<EShanmenFormationMaterialDeliveryMode>(255)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenFormationMasteryCapabilityMatrixTest,
	"Shanmen.0_0_10.CombatRuntime.FormationMastery.CapabilityMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenFormationMasteryCapabilityMatrixTest::RunTest(const FString&)
{
	const FShanmenFormationMasteryPolicy Beginner = MakePolicy(
		EShanmenFormationMasteryTier::Beginner);
	const FShanmenFormationMasteryPolicy Intermediate = MakePolicy(
		EShanmenFormationMasteryTier::Intermediate);
	const FShanmenFormationMasteryPolicy Master = MakePolicy(
		EShanmenFormationMasteryTier::Master);

	TestTrue(TEXT("Beginner walks to an anchor and fills it"),
		Beginner.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::ProximityFill));
	TestFalse(TEXT("Beginner cannot throw material remotely"),
		Beginner.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::RemoteThrow));
	TestFalse(TEXT("Beginner cannot scatter a whole formation"),
		Beginner.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::ScatterFormation));

	TestTrue(TEXT("Intermediate retains proximity fill"),
		Intermediate.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::ProximityFill));
	TestTrue(TEXT("Intermediate unlocks remote material throw"),
		Intermediate.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::RemoteThrow));
	TestFalse(TEXT("Intermediate cannot scatter a whole formation"),
		Intermediate.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::ScatterFormation));

	TestTrue(TEXT("Master retains proximity fill"),
		Master.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::ProximityFill));
	TestTrue(TEXT("Master retains remote material throw"),
		Master.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::RemoteThrow));
	TestTrue(TEXT("Master unlocks scatter formation"),
		Master.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::ScatterFormation));

	TestTrue(TEXT("Each tier exposes its highest unlocked operation"),
		Beginner.GetHighestUnlockedDeliveryMode()
			== EShanmenFormationMaterialDeliveryMode::ProximityFill
		&& Intermediate.GetHighestUnlockedDeliveryMode()
			== EShanmenFormationMaterialDeliveryMode::RemoteThrow
		&& Master.GetHighestUnlockedDeliveryMode()
			== EShanmenFormationMaterialDeliveryMode::ScatterFormation);
	TestFalse(TEXT("All valid tiers reject the invalid operation sentinel"),
		Beginner.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::Invalid)
		|| Intermediate.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::Invalid)
		|| Master.CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::Invalid));
	return true;
}

#endif
