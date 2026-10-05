#include "ShanmenDemo20Encounters.h"
#include "ShanmenDemo20WorldCheckpoint.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapDeterministicRewardRandom.h"
#include "demo_mapRewardGenerator.h"

namespace
{
	FShanmenDemo20EnemySpec Spec(EShanmenDemo20EnemyKind Kind, FVector Spawn)
	{
		FShanmenDemo20EnemySpec S; S.Kind=Kind; S.Spawn=Spawn;
		if (Kind==EShanmenDemo20EnemyKind::Ranged)
		{ S.Health=65; S.Damage=14; S.Range=800; S.WarningRadius=120; S.Color=FLinearColor(.29f,.27f,.51f); }
		else if (Kind==EShanmenDemo20EnemyKind::Elite)
		{ S.Health=156; S.Damage=28; S.Range=S.WarningRadius=310; S.Speed=210; S.Windup=1.1f;
			S.Scale=FVector(1.2f,1.2f,1.6f); S.Color=FLinearColor(.58f,.14f,.11f); }
		return S;
	}
}
FString FShanmenDemo20EnemySpec::Name() const
{
	return Kind==EShanmenDemo20EnemyKind::Ranged?TEXT("远程守卫"):Kind==EShanmenDemo20EnemyKind::Elite?TEXT("精英守卫"):TEXT("近战守卫");
}
FGuid FShanmenDemo20Encounters::ContentId(int32 Revision)
{
	if (Revision==1) return FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.WorldContent.r1"),
		{TEXT("JadePass.FixedThreeZones.r1"),TEXT("MeleeRangedElite.FixedGear.r1")});
	if (Revision==2) return FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.WorldContent.r2"),
		{TEXT("JadePass.FixedThreeZones.r1"),TEXT("OrdinaryZonesIndependent50Melee50Ranged.EliteFixed.X+-220.Y+-320.Xorshift64Star.r1")});
	return FGuid();
}
int32 FShanmenDemo20Encounters::RevisionForContent(const FGuid& Content)
{
	for (int32 R=1;R<=CurrentRevision;++R) if (Content==ContentId(R)) return R;
	return 0;
}
bool FShanmenDemo20Encounters::Build(const FGuid& Run, int32 Revision, FShanmenDemo20EnemySpec (&Out)[Count])
{
	if (!Run.IsValid() || Revision<1 || Revision>CurrentRevision) return false;
	FShanmenDemo20EnemySpec Candidate[Count];
	if (Revision==1)
	{
		Candidate[0]=Spec(EShanmenDemo20EnemyKind::Melee,FVector(1000,-350,65));
		Candidate[1]=Spec(EShanmenDemo20EnemyKind::Ranged,FVector(3100,200,65));
		Candidate[2]=Spec(EShanmenDemo20EnemyKind::Elite,FVector(5200,-300,80));
	}
	else
	{
		const uint64 Seed=FShanmenDemo20WorldCheckpoint::SeedForRun(Run);
		if (!Seed) return false;
		for (int32 I=0;I<Count;++I)
		{
			// A private stream per zone: chest searches, loot order and elapsed time cannot perturb it.
			const FGuid Domain=FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Encounter.Random.r2"),
				{Run.ToString(EGuidFormats::Digits),LexToString(Seed),ContentId(Revision).ToString(EGuidFormats::Digits),LexToString(I)});
			Fdemo_mapDeterministicRewardRandom Random(Fdemo_mapRewardGenerator::ComputeStableSeed(Domain,TEXT("Demo20.Encounter"),TEXT("Xorshift64Star.r1")));
			const auto Kind=I==2?EShanmenDemo20EnemyKind::Elite:Random.RangeInclusive(0,1)?EShanmenDemo20EnemyKind::Ranged:EShanmenDemo20EnemyKind::Melee;
			const FVector Position(1000.f+2100.f*I+(Random.RangeInclusive(0,1)?220.f:-220.f),
				Random.RangeInclusive(0,1)?320.f:-320.f,I==2?80.f:65.f);
			Candidate[I]=Spec(Kind,Position);
		}
	}
	for (int32 I=0;I<Count;++I) Out[I]=Candidate[I];
	return true;
}
