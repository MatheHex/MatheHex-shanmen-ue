#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapM01EnemyTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapM01EnemyConfigTest,
	"demo_map.M01.Enemy.Config",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapM01EnemyConfigTest::RunTest(const FString& Parameters)
{
	FString Error;
	TestTrue(TEXT("M01 enemy config validates"),
		Fdemo_mapM01EnemyConfig::Validate(&Error));
	const TArray<Fdemo_mapM01EnemyDefinition>& Definitions =
		Fdemo_mapM01EnemyConfig::GetDefinitions();
	int32 Standard = 0;
	int32 Elite = 0;
	int32 Boss = 0;
	TSet<FName> Encounters;
	TSet<FName> Archetypes;
	TSet<FName> SourceRoles;
	for (const Fdemo_mapM01EnemyDefinition& Definition : Definitions)
	{
		Encounters.Add(Definition.EncounterId);
		Archetypes.Add(Definition.EnemyArchetypeId);
		SourceRoles.Add(Definition.RewardSourceRoleId);
		if (Definition.IsBoss()) ++Boss;
		else if (Definition.IsElite()) ++Elite;
		else ++Standard;
		TestTrue(TEXT("Every record has P4 identity"),
			!Definition.RewardSourceRoleId.IsNone()
			&& !Definition.CorpseIdentity.IsNone()
			&& !Definition.RiskTierId.IsNone());
	}
	TestEqual(TEXT("Total actor count"), Definitions.Num(), 14);
	TestEqual(TEXT("Standard actor count"), Standard, 10);
	TestEqual(TEXT("Elite actor count"), Elite, 3);
	TestEqual(TEXT("Boss actor count"), Boss, 1);
	TestEqual(TEXT("Unique encounters"), Encounters.Num(), 14);
	TestEqual(TEXT("Distinct combat archetypes"), Archetypes.Num(), 6);
	TestEqual(TEXT("Distinct reward source roles"), SourceRoles.Num(), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapM01EnemyRunLedgerTest,
	"demo_map.M01.Enemy.RunLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapM01EnemyRunLedgerTest::RunTest(const FString& Parameters)
{
	Fdemo_mapM01RunEnemyLedger Ledger;
	const FGuid FirstRun = FGuid::NewGuid();
	Ledger.ResetForNewRun(FirstRun);
	TestTrue(TEXT("Run is active"), Ledger.IsActive());
	TestTrue(TEXT("First encounter claim succeeds"),
		Ledger.TryClaimEncounter(TEXT("M01.Encounter.LOW.Skirmisher.01")));
	TestFalse(TEXT("Duplicate encounter is rejected"),
		Ledger.TryClaimEncounter(TEXT("M01.Encounter.LOW.Skirmisher.01")));
	TestTrue(TEXT("First Boss death commits"),
		Ledger.TryCommitBossDeath(TEXT("M01.Boss.Main")));
	TestFalse(TEXT("Duplicate Boss death is rejected"),
		Ledger.TryCommitBossDeath(TEXT("M01.Boss.Main")));
	Ledger.MarkTerminal();
	TestFalse(TEXT("Terminal Run rejects encounters"),
		Ledger.TryClaimEncounter(TEXT("M01.Encounter.MID.Ranged.01")));
	Ledger.ResetForNewRun(FGuid::NewGuid());
	TestTrue(TEXT("New Run accepts the same stable encounter again"),
		Ledger.TryClaimEncounter(TEXT("M01.Encounter.LOW.Skirmisher.01")));
	TestTrue(TEXT("New Run resets Boss death"),
		Ledger.TryCommitBossDeath(TEXT("M01.Boss.Main")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapM01BossPlannerTest,
	"demo_map.M01.Enemy.BossPlanner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapM01BossPlannerTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Close range selects sweep"),
		Fdemo_mapM01BossCombatPlanner::SelectAttack(200.0f),
		Edemo_mapM01BossAttack::Sweep);
	TestEqual(TEXT("Mid range selects charge"),
		Fdemo_mapM01BossCombatPlanner::SelectAttack(600.0f),
		Edemo_mapM01BossAttack::Charge);
	TestEqual(TEXT("Long range selects volley"),
		Fdemo_mapM01BossCombatPlanner::SelectAttack(1100.0f),
		Edemo_mapM01BossAttack::Volley);
	return true;
}

#endif

