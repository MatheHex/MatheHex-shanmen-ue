#include "ShanmenDemo20Sources.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenDemo20WorldCheckpoint.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapDeterministicRewardRandom.h"
#include "demo_mapRewardGenerator.h"

namespace
{
	bool Same(const FShanmenContentStamp& A,const FShanmenContentStamp& B) { return A.Version==B.Version && A.Digest==B.Digest; }
	struct FPool { const TCHAR* Id; int32 Weight, MaxQuantity, Value; };
	// Fixed definitions only. Score records generation accounting, not spendable currency.
	const FPool Pool[]={{TEXT("Heal.Pill"),24,3,12},{TEXT("Material.Herb"),20,7,3},{TEXT("Material.Ore"),16,4,7},
		{TEXT("Trophy.Jade"),14,3,18},{TEXT("Trophy.Scroll"),8,2,32},{TEXT("Sword.Plain"),6,1,40},
		{TEXT("Sword.Heavy"),4,1,70},{TEXT("Armor.Robe"),5,1,35},{TEXT("Armor.Leather"),3,1,65}};
	int32 RoleIndex(FName Role)
	{
		for (int32 I=0;I<3;++I) { if (Role==FShanmenDemo20Sources::ChestRole(I)) return I;
			if (Role==FShanmenDemo20Sources::EnemyRole(I)) return I+3; }
		return INDEX_NONE;
	}
	uint64 Seed(const FGuid& Run,uint64 RunSeed,FName Role,const TCHAR* Channel)
	{
		const auto Content=FShanmenDemo20Sources::ContentStamp();
		const FGuid Domain=FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Source.Random.r1"),
			{Run.ToString(EGuidFormats::Digits),LexToString(RunSeed),Content.Version.ToString(),Content.Digest,Role.ToString().ToLower(),Channel});
		return Fdemo_mapRewardGenerator::ComputeStableSeed(Domain,TEXT("Demo20.Source"),TEXT("Xorshift64Star.r1"));
	}
	bool Scope(const FShanmenItemGeneratedSourceReadResult& R,uint64 RunSeed)
	{
		return R.OwnerId==FShanmenDemo20Catalog::OwnerId() && R.RunId.IsValid() && FShanmenDemo20Sources::IsRegistered(R.SourceRoleId)
			&& RunSeed!=0 && RunSeed==FShanmenDemo20WorldCheckpoint::SeedForRun(R.RunId)
			&& Same(R.ItemContent,FShanmenDemo20Catalog::ContentStamp()) && R.AuthorityRevision>=0
			&& R.AcceptedSequence>=0 && R.AcceptedSequence<MAX_int64 && R.PityState>=0
			&& (Same(R.SourceContent,FShanmenDemo20Sources::ContentStamp()) || (R.SourceContent.Version.IsNone() && R.SourceContent.Digest.IsEmpty()));
	}
	bool ValidReceipt(const FShanmenItemGeneratedSourceReceipt& Receipt,const FShanmenItemGeneratedSourceReadResult& R,uint64 RunSeed)
	{
		if (!Receipt.IsValid() || !Scope(R,RunSeed) || !Same(R.SourceContent,FShanmenDemo20Sources::ContentStamp())) return false;
		const auto& P=Receipt.GetPlan();
		if (P.OwnerId!=R.OwnerId || P.RunId!=R.RunId || P.SourceRoleId!=R.SourceRoleId || !Same(P.Content,FShanmenDemo20Sources::ContentStamp())
			|| P.EffectiveSeed!=Seed(R.RunId,RunSeed,R.SourceRoleId,TEXT("Loot")) || P.ExpectedSequence>=R.AcceptedSequence
			|| P.Entries.Num()>4 || P.bLegacyCompatibilityView || P.bPityCommitRequired || P.PityStateBefore!=P.PityStateAfter
			|| P.ProjectionId!=TEXT("Demo20.SourceProjection.r1") || P.DistributionProfileId!=TEXT("Demo20.FixedSourcePool.r1")
			|| P.BudgetProfileId!=TEXT("Demo20.SourceBudget.r1")) return false;
		int64 Total=0;
		for (int32 I=0;I<P.Entries.Num();++I)
		{
			const auto& E=P.Entries[I]; FShanmenItemDefinition D; const FPool* Match=nullptr;
			for (const auto& Candidate:Pool) if (E.Definition.DefinitionId==FName(Candidate.Id)) Match=&Candidate;
			FShanmenItemRewardMetadata Metadata; Metadata.RewardSourceRoleId=P.SourceRoleId;
			if (!Match || !FShanmenDemo20Catalog::Definition(E.Definition.DefinitionId,D) || !(E.Definition==D) || !(E.RewardMetadata==Metadata)
				|| E.Quantity>Match->MaxQuantity || E.UnitValue!=Match->Value || E.TotalValue!=E.UnitValue*E.Quantity
				|| E.SectionId!=TEXT("Loot") || E.SlotIndex!=I || !E.ChildContainerType.IsNone() || E.ChildContainerCapacity!=0) return false;
			Total+=E.TotalValue;
		}
		const bool Elite=P.SourceRoleId==FShanmenDemo20Sources::EnemyRole(2),Chest=RoleIndex(P.SourceRoleId)<3;
		return P.Entries.Num()>=(Elite?3:Chest?2:1) && P.Entries.Num()<=(Elite?4:Chest?3:2)
			&& Total==P.GeneratedTotalValue && Total==P.RandomizedBudget && P.ResidualValue==0;
	}
}

FShanmenContentStamp FShanmenDemo20Sources::ContentStamp()
{
	FShanmenContentStamp C; C.Version=TEXT("Demo20.Sources.r1");
	C.Digest=TEXT("JadePass.6Roles.ChestX800+2100i.Y+-620.Z40.CountChest2-3Enemy1-2Elite3-4.9Pool.W24-20-16-14-8-6-4-5-3.Q3-7-4-3-2-1-1-1-1.V12-3-7-18-32-40-70-35-65.Xorshift64Star.r1"); return C;
}
FName FShanmenDemo20Sources::ChestRole(int32 I)
{
	const FName Roles[]={TEXT("Chest.StonePath"),TEXT("Chest.Bamboo"),TEXT("Chest.Altar")}; return I>=0 && I<3?Roles[I]:NAME_None;
}
FName FShanmenDemo20Sources::EnemyRole(int32 I)
{
	const FName Roles[]={TEXT("Enemy.Melee"),TEXT("Enemy.Ranged"),TEXT("Enemy.Elite")}; return I>=0 && I<3?Roles[I]:NAME_None;
}
bool FShanmenDemo20Sources::IsRegistered(FName R) { return RoleIndex(R)!=INDEX_NONE; }
FString FShanmenDemo20Sources::Name(FName R)
{
	// Enemy role keys are historical slot identities, not the new random archetype.
	const TCHAR* Names[]={TEXT("石径宝匣"),TEXT("竹林宝匣"),TEXT("遗坛宝匣"),TEXT("石径守卫遗物"),TEXT("竹林守卫遗物"),TEXT("遗坛精英遗物")};
	const int32 I=RoleIndex(R); return I==INDEX_NONE?TEXT("未注册来源"):Names[I];
}
bool FShanmenDemo20Sources::ChestPosition(const FGuid& Run,uint64 RunSeed,int32 Index,FVector& Out)
{
	if (!Run.IsValid() || RunSeed==0 || RunSeed!=FShanmenDemo20WorldCheckpoint::SeedForRun(Run) || Index<0 || Index>=3) return false;
	Fdemo_mapDeterministicRewardRandom Random(Seed(Run,RunSeed,ChestRole(Index),TEXT("Position")));
	// Both choices stay inside their zone, away from doors, spawn, extraction and pillars.
	Out=FVector(800.f+2100.f*Index,Random.RangeInclusive(0,1)?620.f:-620.f,40.f); return true;
}
bool FShanmenDemo20Sources::Build(const FShanmenItemGeneratedSourceReadResult& Read,uint64 RunSeed,
	FShanmenItemGeneratedSourceRequest& Out,FString& Reason)
{
	if (Read.Status!=EShanmenItemGeneratedSourceReadStatus::Absent || Read.RunState!=EShanmenItemGeneratedSourceRunState::Active || !Scope(Read,RunSeed))
	{ Reason=TEXT("来源、内容或原局状态未确认，没有生成物品。"); return false; }
	FShanmenItemGeneratedSourceRequest R; R.ItemContent=Read.ItemContent; auto& P=R.Plan;
	P.OwnerId=Read.OwnerId; P.RunId=Read.RunId; P.SourceRoleId=Read.SourceRoleId; P.Content=ContentStamp();
	P.SlotId=Read.SourceRoleId; P.ProjectionId=TEXT("Demo20.SourceProjection.r1"); P.DistributionProfileId=TEXT("Demo20.FixedSourcePool.r1");
	P.BudgetProfileId=TEXT("Demo20.SourceBudget.r1"); P.EffectiveSeed=Seed(P.RunId,RunSeed,P.SourceRoleId,TEXT("Loot"));
	P.ExpectedSequence=Read.AcceptedSequence; P.PityStateBefore=P.PityStateAfter=Read.PityState;
	const bool Elite=Read.SourceRoleId==EnemyRole(2); const bool Chest=RoleIndex(Read.SourceRoleId)<3;
	Fdemo_mapDeterministicRewardRandom Random(P.EffectiveSeed);
	const int32 Count=static_cast<int32>(Random.RangeInclusive(Elite?3:Chest?2:1,Elite?4:Chest?3:2));
	for (int32 I=0;I<Count;++I)
	{
		int32 Choice=static_cast<int32>(Random.RangeInclusive(1,100)); const FPool* Selected=nullptr;
		for (const auto& E:Pool) { Choice-=E.Weight; if (Choice<=0) { Selected=&E; break; } }
		if (!Selected) { Reason=TEXT("来源目录校验失败。"); return false; }
		FShanmenItemGeneratedSourceEntry E; if (!FShanmenDemo20Catalog::Definition(Selected->Id,E.Definition)) return false;
		E.Quantity=static_cast<int32>(Random.RangeInclusive(1,Selected->MaxQuantity)); E.SectionId=TEXT("Loot"); E.SlotIndex=I;
		E.UnitValue=Selected->Value; E.TotalValue=E.UnitValue*E.Quantity; E.RewardMetadata.RewardSourceRoleId=P.SourceRoleId;
		P.GeneratedTotalValue+=E.TotalValue; P.Entries.Add(E);
	}
	P.RandomizedBudget=P.GeneratedTotalValue;
	if (!P.IsValid()) { Reason=TEXT("来源计划校验失败，未接纳。"); return false; }
	Out=MoveTemp(R); return true;
}
bool FShanmenDemo20Sources::Resolve(const FGuid& Run,uint64 RunSeed,FName Role,const FShanmenDemo20SourcePorts& Ports,
	FShanmenItemGeneratedSourceReceipt& Out,FString& Reason)
{
	if (!IsRegistered(Role) || !Ports.Read || !Ports.Accept) { Reason=TEXT("搜索端口尚未就绪。"); return false; }
	const FGuid Owner=FShanmenDemo20Catalog::OwnerId(); auto Read=Ports.Read(Owner,Run,Role);
	if (Read.RunId!=Run || Read.OwnerId!=Owner || Read.SourceRoleId!=Role || Read.RunState!=EShanmenItemGeneratedSourceRunState::Active || !Scope(Read,RunSeed))
	{ Reason=TEXT("来源读取未确认，请重试或重启恢复原局。"); return false; }
	if (Read.Status==EShanmenItemGeneratedSourceReadStatus::Accepted)
	{
		if (!ValidReceipt(Read.Receipt,Read,RunSeed)) { Reason=TEXT("已保存的来源不兼容，未重抽物品。"); return false; }
		Out=Read.Receipt; return true;
	}
	FShanmenItemGeneratedSourceRequest R; if (!Build(Read,RunSeed,R,Reason)) return false;
	const auto Result=Ports.Accept(R);
	if (!Result.IsCommandSuccess()) { Reason=TEXT("搜索结果尚未确认保存。没有领取或重抽；可重试原来源，恢复中请重启。"); return false; }
	Read=Ports.Read(Owner,Run,Role);
	if (Read.Status!=EShanmenItemGeneratedSourceReadStatus::Accepted || !ValidReceipt(Read.Receipt,Read,RunSeed) || !(Read.Receipt.GetPlan()==R.Plan))
	{ Reason=TEXT("搜索回读未确认。请恢复原局，不另建来源。"); return false; }
	Out=Read.Receipt; return true;
}
FString FShanmenDemo20Sources::Preview(const FShanmenItemGeneratedSourceReceipt& R)
{
	if (!R.IsValid()) return TEXT("来源结果尚未确认。");
	FString S; for (const auto& E:R.GetPlan().Entries)
		S+=FString::Printf(TEXT("%s × %d\n"),*FShanmenDemo20Catalog::ItemName(E.Definition.DefinitionId),E.Quantity);
	return S+TEXT("\n搜索结果已保存；当前为只读预览，尚不能领取。\n重新打开或恢复不会重抽。世界继续运行，请留意敌人。");
}
bool FShanmenDemo20Search::Begin(FName InRole,const FVector& Player,float InHealth)
{
	if (IsActive() || !FShanmenDemo20Sources::IsRegistered(InRole) || Player.ContainsNaN() || !FMath::IsFinite(InHealth) || InHealth<=0) return false;
	Role=InRole; Origin=Player; Health=InHealth; Clock=0; return true;
}
EShanmenDemo20SearchStep FShanmenDemo20Search::Advance(float Delta,const FVector& Player,float InHealth,bool Available)
{
	if (!IsActive() || !Available || !FMath::IsFinite(Delta) || Delta<0 || Player.ContainsNaN() || !FMath::IsFinite(InHealth)
		|| InHealth<Health || FVector::Dist2D(Player,Origin)>20.f)
	{ Cancel(); return EShanmenDemo20SearchStep::Interrupted; }
	Clock=FMath::Min(1.f,Clock+Delta); return Clock>=1.f?EShanmenDemo20SearchStep::Complete:EShanmenDemo20SearchStep::Pending;
}
