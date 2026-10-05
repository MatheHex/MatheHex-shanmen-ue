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
	bool Read(const FString& P, TArray<uint8>& Out)
	{
		TUniquePtr<IFileHandle> H(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*P));
		if (!H || H->Size() < 24 || H->Size() > MaxBytes) return false;
		Out.SetNumUninitialized(static_cast<int32>(H->Size()));
		return H->Read(Out.GetData(),Out.Num()) && H->Size() == Out.Num();
	}
}

FGuid FShanmenDemo20WorldCheckpoint::CurrentContentId()
{
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.WorldContent.r1"), {TEXT("JadePass.FixedThreeZones.r1"),TEXT("MeleeRangedElite.FixedGear.r1")});
}
uint64 FShanmenDemo20WorldCheckpoint::SeedForRun(const FGuid& Run)
{
	const FGuid S = FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Expedition.RunSeed.r1"), {Run.ToString()});
	return (static_cast<uint64>(S.A)<<32) | S.B;
}
bool FShanmenDemo20WorldCheckpoint::IsValid() const
{
	if (Generation < 0 || Generation == MAX_int32 || ContentId != CurrentContentId() || !Combat.IsValid()
		|| RunSeed != SeedForRun(Combat.RunId) || !Position(PlayerPosition) || !FMath::IsFinite(PlayerYaw)) return false;
	for (int32 I = 0; I < 3; ++I) if (!Position(EnemyPositions[I]) || !Position(WarningTargets[I])
		|| !FMath::IsFinite(EnemyClocks[I]) || EnemyClocks[I] < -2.f || EnemyClocks[I] > 2.f) return false;
	if (Medicine.IsSet())
	{
		if (Medicine.ExpectedQuantity < 1 || Medicine.ExpectedQuantity > 10 || Medicine.ExpectedItemRevision < 0
			|| Medicine.ExpectedItemRevision == MAX_int64 || static_cast<uint8>(Medicine.Origin)>1
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
#endif
FString FShanmenDemo20WorldCheckpointStore::Path(const FString& Root, const FGuid& Run)
{
	return FPaths::Combine(Root,TEXT("Demo20World"),Run.ToString(EGuidFormats::Digits)+TEXT(".world"));
}
bool FShanmenDemo20WorldCheckpointStore::Load(const FString& Root, const FGuid& Run, FShanmenDemo20WorldCheckpoint& Out, FString& Reason)
{
	TArray<uint8> Bytes; FShanmenDemo20WorldCheckpoint C;
	if (Root.IsEmpty() || !Run.IsValid() || !Read(Path(Root,Run),Bytes) || !Decode(Bytes,C) || C.Combat.RunId != Run)
	{ Reason = TEXT("探索记录缺失、版本不匹配或校验失败。原物品档保留；不会重置生命或重生敌人。"); return false; }
	Out=C; Reason.Reset(); return true;
}
bool FShanmenDemo20WorldCheckpointStore::Save(const FString& Root, FShanmenDemo20WorldCheckpoint& Before,
	const FShanmenDemo20WorldCheckpoint& Intent, FString& Reason)
{
	auto Fail = [&]() { Reason = TEXT("探索进度未确认保存，游戏已暂停。可重试；不会显示已带回或已保存。"); return false; };
	if (Root.IsEmpty() || !Intent.IsValid() || Before.Generation < 0 || Before.Generation >= MAX_int32-1
		|| (Before.Generation > 0 && Before.Combat.RunId != Intent.Combat.RunId)) return Fail();
	auto C = Intent; C.Generation = Before.Generation+1;
	TArray<uint8> Bytes; if (!Encode(C,Bytes)) return Fail();
	const FString P=Path(Root,C.Combat.RunId), Temp=P+TEXT(".tmp"), Backup=P+TEXT(".bak");
	TArray<uint8> Old;
	if (Read(P,Old) && Old == Bytes)
	{ Before=C; Reason.Reset(); return true; } // Exact ambiguous post-replace retry, without another generation.
	if (Before.Generation > 0)
	{
		TArray<uint8> Expected; if (!Encode(Before,Expected) || !Read(P,Old) || Old != Expected) return Fail();
	}
	else if (IFileManager::Get().FileExists(*P) || IFileManager::Get().FileExists(*Backup)) return Fail();
	if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(P),true)) return Fail();
	{
		TUniquePtr<IFileHandle> H(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*Temp,false,false));
		if (!H || !H->Write(Bytes.GetData(),Bytes.Num()) || !H->Flush(true)) return Fail();
	}
	TArray<uint8> Verify; FShanmenDemo20WorldCheckpoint Decoded;
	if (!Read(Temp,Verify) || Verify != Bytes || !Decode(Verify,Decoded)) return Fail();
	if (Before.Generation > 0)
	{
		if (IFileManager::Get().Copy(*Backup,*P,true,true) != COPY_OK || !Read(Backup,Verify) || Verify != Old) return Fail();
	}
#if WITH_DEV_AUTOMATION_TESTS
	if (bFailBeforeReplace) return Fail();
#endif
	if (!IFileManager::Get().Move(*P,*Temp,true,false,true,true)) return Fail();
#if WITH_DEV_AUTOMATION_TESTS
	if (bFailFirstReadAfterReplace) { bFailFirstReadAfterReplace=false; return Fail(); }
#endif
	if (!Read(P,Verify) || Verify != Bytes || !Decode(Verify,Decoded)) return Fail();
	Before = C; Reason.Reset(); return true;
}
