#include "ShanmenDemo20WorldCheckpoint.h"
#include "ShanmenDeterministicId.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr uint32 Magic = 0x44325731; // D2W1; independent product checkpoint, not an item schema.
	constexpr int32 MaxBytes = 1024;
	bool Position(const FVector& V)
	{
		return !V.ContainsNaN() && FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z)
			&& V.X >= -1100 && V.X <= 6300 && FMath::Abs(V.Y) <= 1200 && V.Z >= -10 && V.Z <= 400;
	}
	void Payload(FArchive& A, FShanmenDemo20WorldCheckpoint& C)
	{
		A << C.Generation << C.ContentId << C.RunSeed << C.Combat.RunId;
		if (A.IsLoading()) C.Combat.EncounterRevision=FShanmenDemo20Encounters::RevisionForContent(C.ContentId);
		uint8 Phase = static_cast<uint8>(C.Combat.Phase); A << Phase; C.Combat.Phase = static_cast<EShanmenDemo20Phase>(Phase);
		A << C.Combat.Sequence;
		for (int32 I = 0; I < 4; ++I) A << C.Combat.Health[I] << C.Combat.Revisions[I];
		A << C.Combat.Elapsed << C.Combat.AttackCooldown << C.Combat.EvadeCooldown << C.Combat.EvadeWindow
			<< C.Combat.SwordDamage << C.Combat.ArmorFraction << C.PlayerPosition << C.PlayerYaw;
		for (int32 I = 0; I < 3; ++I) A << C.EnemyPositions[I] << C.WarningTargets[I] << C.EnemyClocks[I];
		// Legacy r1 records end here. Empty intents keep their exact encoding, so
		// CAS against an existing 333-byte record remains valid without migration.
		if ((A.IsLoading() && !A.AtEnd()) || (A.IsSaving() && C.Medicine.IsSet()))
		{
			uint32 Extension = 0x484C3031; A << Extension;
			uint8 Origin = static_cast<uint8>(C.Medicine.Origin);
			A << C.Medicine.ItemId << C.Medicine.ExpectedQuantity << C.Medicine.ExpectedItemRevision << Origin;
			C.Medicine.Origin = static_cast<EShanmenDemo20MedicineOrigin>(Origin);
			if (Extension != 0x484C3031 || !C.Medicine.IsSet()) A.SetError();
		}
	}
	FGuid Digest(const TArray<uint8>& Bytes)
	{
		return FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.WorldCheckpoint.Integrity.r1"), {BytesToHex(Bytes.GetData(), Bytes.Num())});
	}
	bool Encode(const FShanmenDemo20WorldCheckpoint& C, TArray<uint8>& Out)
	{
		if (!C.IsValid() || C.Generation < 1) return false;
		auto Copy = C; TArray<uint8> Body; FMemoryWriter Writer(Body); Payload(Writer, Copy);
		if (Writer.IsError() || Body.Num() > MaxBytes - 24) return false;
		Out.Reset(); FMemoryWriter Envelope(Out); uint32 Header = Magic; int32 Size = Body.Num(); FGuid Hash = Digest(Body);
		Envelope << Header << Size << Hash; Envelope.Serialize(Body.GetData(), Body.Num()); return !Envelope.IsError();
	}
	bool Decode(const TArray<uint8>& Bytes, FShanmenDemo20WorldCheckpoint& Out)
	{
		if (Bytes.Num() < 24 || Bytes.Num() > MaxBytes) return false;
		FMemoryReader Envelope(Bytes); uint32 Header = 0; int32 Size = 0; FGuid Hash;
		Envelope << Header << Size << Hash;
		if (Envelope.IsError() || Header != Magic || Size != Bytes.Num()-24) return false;
		TArray<uint8> Body; Body.Append(Bytes.GetData()+24, Size);
		if (Hash != Digest(Body)) return false;
		FMemoryReader Reader(Body); FShanmenDemo20WorldCheckpoint C; Payload(Reader,C);
		if (Reader.IsError() || Reader.Tell() != Body.Num() || C.Generation < 1 || !C.IsValid()) return false;
		Out = C; return true;
	}
	bool Read(const FString& P, TArray<uint8>& Out, int32 Minimum = 24)
	{
		TUniquePtr<IFileHandle> H(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*P));
		if (!H || H->Size() < Minimum || H->Size() > MaxBytes) return false;
		Out.SetNumUninitialized(static_cast<int32>(H->Size()));
		// Windows readers reject Read(...,0), even for an opened empty file.
		// Only damage preservation opts into Minimum=0; checkpoints still need 24 bytes.
		return (Out.IsEmpty() || H->Read(Out.GetData(),Out.Num())) && H->Size() == Out.Num();
	}
	bool WriteTemp(const FString& P, const TArray<uint8>& Bytes)
	{
		{
			TUniquePtr<IFileHandle> H(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*P,false,false));
			if (!H || (!Bytes.IsEmpty() && !H->Write(Bytes.GetData(),Bytes.Num())) || !H->Flush(true)) return false;
		}
		TArray<uint8> Verify; return Read(P,Verify,0) && Verify == Bytes;
	}
	bool WriteAtomic(const FString& P, const TArray<uint8>& Bytes)
	{
		const FString Temp=P+TEXT(".tmp"); TArray<uint8> Verify;
		return WriteTemp(Temp,Bytes) && IFileManager::Get().Move(*P,*Temp,true,false,true,true)
			&& Read(P,Verify,0) && Verify == Bytes;
	}
	// Metadata names an exact admitted transaction, not another live world or inventory.
	// The predecessor hash binds a retry to the original CAS input. A newer witness
	// must never be silently replaced with a different intent at the same generation.
	struct FWitness
	{
		FGuid Run, CheckpointHash, PreviousHash;
		int32 Generation=0;
		bool IsValid() const { return Run.IsValid() && CheckpointHash.IsValid() && Generation>0 && Generation<MAX_int32
			&& ((Generation==1 && !PreviousHash.IsValid()) || (Generation>1 && PreviousHash.IsValid())); }
	};
	bool EncodeWitness(const FWitness& W, TArray<uint8>& Out)
	{
		if (!W.IsValid()) return false;
		auto C=W; TArray<uint8> Body; FMemoryWriter Writer(Body);
		Writer << C.Run << C.Generation << C.CheckpointHash << C.PreviousHash;
		Out.Reset(); FMemoryWriter Envelope(Out); uint32 Header=0x44324831; int32 Size=Body.Num(); FGuid Hash=Digest(Body);
		Envelope << Header << Size << Hash; Envelope.Serialize(Body.GetData(),Body.Num());
		return !Writer.IsError() && !Envelope.IsError();
	}
	bool ReadWitness(const FString& P, const FGuid& Run, FWitness& Out, TArray<uint8>& Raw)
	{
		if (!Read(P,Raw) || Raw.Num()!=76) return false;
		FMemoryReader Envelope(Raw); uint32 Header=0; int32 Size=0; FGuid Hash;
		Envelope << Header << Size << Hash;
		if (Envelope.IsError() || Header!=0x44324831 || Size!=52) return false;
		TArray<uint8> Body; Body.Append(Raw.GetData()+24,Size);
		if (Hash!=Digest(Body)) return false;
		FMemoryReader Reader(Body); FWitness W; Reader << W.Run << W.Generation << W.CheckpointHash << W.PreviousHash;
		if (Reader.IsError() || Reader.Tell()!=Body.Num() || !W.IsValid() || W.Run!=Run) return false;
		Out=W; return true;
	}
	bool Matches(const FWitness& W, const TArray<uint8>& Bytes, FShanmenDemo20WorldCheckpoint& Out)
	{
		return Digest(Bytes)==W.CheckpointHash && Decode(Bytes,Out)
			&& Out.Combat.RunId==W.Run && Out.Generation==W.Generation;
	}
	bool Replicate(const FString& P, const TArray<uint8>& Bytes)
	{
#if WITH_DEV_AUTOMATION_TESTS
		if (FShanmenDemo20WorldCheckpointStore::bFailReplicaWrite) return false;
#endif
		TArray<uint8> Existing;
		return (Read(P+TEXT(".replica"),Existing) && Existing==Bytes) || WriteAtomic(P+TEXT(".replica"),Bytes);
	}
}

FGuid FShanmenDemo20WorldCheckpoint::CurrentContentId()
{
	return FShanmenDemo20Encounters::ContentId(FShanmenDemo20Encounters::CurrentRevision);
}
FGuid FShanmenDemo20WorldCheckpoint::LegacyContentId()
{
	return FShanmenDemo20Encounters::ContentId(1);
}
uint64 FShanmenDemo20WorldCheckpoint::SeedForRun(const FGuid& Run)
{
	const FGuid S = FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Expedition.RunSeed.r1"), {Run.ToString()});
	return (static_cast<uint64>(S.A)<<32) | S.B;
}
bool FShanmenDemo20WorldCheckpoint::IsValid() const
{
	if (Generation < 0 || Generation == MAX_int32 || FShanmenDemo20Encounters::RevisionForContent(ContentId)!=Combat.EncounterRevision || !Combat.IsValid()
		|| RunSeed != SeedForRun(Combat.RunId) || !Position(PlayerPosition) || !FMath::IsFinite(PlayerYaw)) return false;
	for (int32 I = 0; I < 3; ++I) if (!Position(EnemyPositions[I]) || !Position(WarningTargets[I])
		|| !FMath::IsFinite(EnemyClocks[I]) || EnemyClocks[I] < -2.f || EnemyClocks[I] > 2.f) return false;
	if (Medicine.IsSet())
	{
		if (Medicine.ExpectedQuantity < 1 || Medicine.ExpectedQuantity > 10 || Medicine.ExpectedItemRevision < 0
			|| Medicine.ExpectedItemRevision == MAX_int64 || static_cast<uint8>(Medicine.Origin)>2
			|| Combat.Phase != EShanmenDemo20Phase::Active || Combat.Health[0]<=0.f || Combat.Health[0]>=100.f
			|| Combat.AttackCooldown>0.f || Combat.EvadeWindow>0.f || Combat.Sequence>=MAX_uint64-1) return false;
	}
	else if (Medicine.ExpectedQuantity != 0 || Medicine.ExpectedItemRevision != 0
		|| Medicine.Origin != EShanmenDemo20MedicineOrigin::PreparedCarry) return false;
	return true;
}

#if WITH_DEV_AUTOMATION_TESTS
bool FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace = false;
bool FShanmenDemo20WorldCheckpointStore::bFailFirstReadAfterReplace = false;
bool FShanmenDemo20WorldCheckpointStore::bFailAfterWitnessBeforeReplace = false;
bool FShanmenDemo20WorldCheckpointStore::bFailReplicaWrite = false;
bool FShanmenDemo20WorldCheckpointStore::bFailBeforeRepairReplace = false;
#endif
FString FShanmenDemo20WorldCheckpointStore::Path(const FString& Root, const FGuid& Run)
{
	return FPaths::Combine(Root,TEXT("Demo20World"),Run.ToString(EGuidFormats::Digits)+TEXT(".world"));
}
bool FShanmenDemo20WorldCheckpointStore::Load(const FString& Root, const FGuid& Run, FShanmenDemo20WorldCheckpoint& Out, FString& Reason)
{
	TArray<uint8> Bytes, Head; FShanmenDemo20WorldCheckpoint C; FWitness W;
	const FString P=Path(Root,Run); const bool HasHead=IFileManager::Get().FileExists(*(P+TEXT(".head")));
	const bool Valid=Root.Len()>0 && Run.IsValid() && Read(P,Bytes) && Decode(Bytes,C) && C.Combat.RunId==Run
		&& (HasHead ? (ReadWitness(P+TEXT(".head"),Run,W,Head) && Matches(W,Bytes,C))
			: !IFileManager::Get().FileExists(*(P+TEXT(".replica"))));
	if (!Valid)
	{ Reason = TEXT("探索记录缺失、待确认或校验失败。可尝试修复并恢复原局；无精确凭证时保留原档，不会重置生命或重生敌人。"); return false; }
	Out=C; Reason.Reset(); return true;
}
bool FShanmenDemo20WorldCheckpointStore::Recover(const FString& Root, const FGuid& Run,
	FShanmenDemo20WorldCheckpoint& Out, FString& Reason)
{
	auto Fail=[&]() { Reason=TEXT("原局尚未修复：未找到最新代的精确凭证与副本，或替换未确认。原档保留；不会选择旧备份、重生敌人或返还丹药。可重试或保留日志交人工检查。"); return false; };
	if (Root.IsEmpty() || !Run.IsValid()) return Fail();
	const FString P=Path(Root,Run); TArray<uint8> Head; FWitness W;
	if (!ReadWitness(P+TEXT(".head"),Run,W,Head)) return Fail();
	TArray<uint8> Bytes; FShanmenDemo20WorldCheckpoint C; bool Found=false;
	for (const FString& Source : {P,P+TEXT(".replica"),P+TEXT(".tmp")})
		if (Read(Source,Bytes) && Matches(W,Bytes,C)) { Found=true; break; }
	if (!Found) return Fail(); // .bak intentionally cannot establish the latest generation.
	TArray<uint8> Original; const bool HadPrimary=IFileManager::Get().FileExists(*P);
	if (HadPrimary)
	{
		if (!Read(P,Original,0)) return Fail(); // Do not overwrite an oversized/unreadable damaged file.
		if (Original!=Bytes)
		{
			const FString Archived=P+TEXT(".damaged.")+Digest(Original).ToString(EGuidFormats::Digits);
			TArray<uint8> Kept;
			if (IFileManager::Get().FileExists(*Archived)) { if (!Read(Archived,Kept,0) || Kept!=Original) return Fail(); }
			else if (!WriteAtomic(Archived,Original)) return Fail();
		}
	}
	TArray<uint8> CurrentHead, CurrentPrimary; FWitness Check;
	if (!ReadWitness(P+TEXT(".head"),Run,Check,CurrentHead) || CurrentHead!=Head
		|| IFileManager::Get().FileExists(*P)!=HadPrimary || (HadPrimary && (!Read(P,CurrentPrimary,0) || CurrentPrimary!=Original))) return Fail();
#if WITH_DEV_AUTOMATION_TESTS
	if (bFailBeforeRepairReplace) return Fail();
#endif
	if (Original!=Bytes && !WriteAtomic(P,Bytes)) return Fail();
	if (!Replicate(P,Bytes) || !Read(P,CurrentPrimary) || CurrentPrimary!=Bytes
		|| !ReadWitness(P+TEXT(".head"),Run,Check,CurrentHead) || CurrentHead!=Head || !Matches(W,CurrentPrimary,C)) return Fail();
	Out=C; Reason=TEXT("原局精确记录已修复；生命、敌人、种子与待确认动作按同一代继续，未重开或返还物品。"); return true;
}
bool FShanmenDemo20WorldCheckpointStore::Save(const FString& Root, FShanmenDemo20WorldCheckpoint& Before,
	const FShanmenDemo20WorldCheckpoint& Intent, FString& Reason)
{
	auto Fail = [&]() { Reason = TEXT("探索进度未确认保存，游戏已暂停。可重试；不会显示已带回或已保存。"); return false; };
	if (Root.IsEmpty() || !Intent.IsValid() || Before.Generation < 0 || Before.Generation >= MAX_int32-1
		|| (Before.Generation > 0 && (Before.Combat.RunId != Intent.Combat.RunId || Before.ContentId != Intent.ContentId))) return Fail();
	auto C = Intent; C.Generation = Before.Generation+1;
	TArray<uint8> Bytes; if (!Encode(C,Bytes)) return Fail();
	const FString P=Path(Root,C.Combat.RunId), Temp=P+TEXT(".tmp"), Backup=P+TEXT(".bak");
	TArray<uint8> Old, Head; FWitness W; const bool HasHead=IFileManager::Get().FileExists(*(P+TEXT(".head")));
	if (HasHead && !ReadWitness(P+TEXT(".head"),C.Combat.RunId,W,Head)) return Fail();
	if (!HasHead && IFileManager::Get().FileExists(*(P+TEXT(".replica")))) return Fail();
	if (Read(P,Old) && Old == Bytes)
	{
		FShanmenDemo20WorldCheckpoint Verified;
		if (HasHead && !Matches(W,Bytes,Verified)) return Fail();
		if (HasHead && Before.Generation>0)
		{
			TArray<uint8> Previous;
			if (!Encode(Before,Previous) || W.PreviousHash!=Digest(Previous)) return Fail();
		}
		if (!HasHead)
		{
			FWitness Adopted; Adopted.Run=C.Combat.RunId; Adopted.Generation=C.Generation; Adopted.CheckpointHash=Digest(Bytes);
			TArray<uint8> Previous; if (Before.Generation>0) { if (!Encode(Before,Previous)) return Fail(); Adopted.PreviousHash=Digest(Previous); }
			if (!EncodeWitness(Adopted,Head) || !WriteAtomic(P+TEXT(".head"),Head)) return Fail();
		}
		if (!Replicate(P,Bytes)) return Fail();
		Before=C; Reason.Reset(); return true;
	} // Exact ambiguous post-replace retry, without another generation.
	TArray<uint8> Expected;
	if (Before.Generation > 0)
	{
		if (!Encode(Before,Expected) || !Read(P,Old) || Old != Expected) return Fail();
		if (HasHead && !((W.Generation==Before.Generation && W.CheckpointHash==Digest(Expected))
			|| (W.Generation==C.Generation && W.CheckpointHash==Digest(Bytes) && W.PreviousHash==Digest(Expected)))) return Fail();
	}
	else if (IFileManager::Get().FileExists(*P) || IFileManager::Get().FileExists(*Backup)
		|| (HasHead && (W.Generation!=1 || W.CheckpointHash!=Digest(Bytes)))) return Fail();
	if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(P),true)) return Fail();
	if (!WriteTemp(Temp,Bytes)) return Fail();
	TArray<uint8> Verify; FShanmenDemo20WorldCheckpoint Decoded;
	if (!Read(Temp,Verify) || Verify != Bytes || !Decode(Verify,Decoded)) return Fail();
	if (Before.Generation > 0)
	{
		if (IFileManager::Get().Copy(*Backup,*P,true,true) != COPY_OK || !Read(Backup,Verify) || Verify != Old) return Fail();
	}
#if WITH_DEV_AUTOMATION_TESTS
	if (bFailBeforeReplace) return Fail();
#endif
	FWitness Next; Next.Run=C.Combat.RunId; Next.Generation=C.Generation; Next.CheckpointHash=Digest(Bytes);
	if (Before.Generation>0) Next.PreviousHash=Digest(Expected);
	if (!EncodeWitness(Next,Head) || !WriteAtomic(P+TEXT(".head"),Head)) return Fail();
#if WITH_DEV_AUTOMATION_TESTS
	if (bFailAfterWitnessBeforeReplace) return Fail();
#endif
	if (!IFileManager::Get().Move(*P,*Temp,true,false,true,true)) return Fail();
#if WITH_DEV_AUTOMATION_TESTS
	if (bFailFirstReadAfterReplace) { bFailFirstReadAfterReplace=false; return Fail(); }
#endif
	if (!Read(P,Verify) || Verify != Bytes || !Decode(Verify,Decoded)) return Fail();
	if (!Replicate(P,Bytes)) return Fail();
	Before = C; Reason.Reset(); return true;
}
