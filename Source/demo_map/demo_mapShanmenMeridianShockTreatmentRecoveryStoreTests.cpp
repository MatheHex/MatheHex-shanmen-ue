#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenMeridianShockTreatmentRecoveryStore.h"

#include "demo_mapAttributeComponent.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenCombatConditionComponent.h"

#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ShanmenDeterministicId.h"
#include "ShanmenVitalityAuthority.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr EAutomationTestFlags RecoveryStoreFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid RecoveryOwnerId(0xC1650001, 0, 0, 1);
	const FGuid RecoveryRunId(0xC1650002, 0, 0, 1);

	FString NewRecoveryStoreRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P16.8.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenVitalityCommitReceipt MakeCommittedReceipt(
		const FGuid& TargetId,
		const FGuid& ImpactId,
		const FGuid& ResolutionId)
	{
		FShanmenVitalityAuthority Authority;
		if (!FShanmenVitalityAuthority::TryCreate(
				TargetId, 100.0f, 100.0f, 0, Authority))
		{
			return FShanmenVitalityCommitReceipt();
		}
		FShanmenVitalityCommitCommand Command;
		if (!FShanmenVitalityCommitCommand::TryRestoreFromDurableIntent(
				ImpactId,
				ResolutionId,
				TargetId,
				0,
				100.0f,
				100.0f,
				10.0f,
				0.0f,
				10.0f,
				EShanmenDefenseOutcome::Applied,
				Command))
		{
			return FShanmenVitalityCommitReceipt();
		}
		return Authority.Commit(Command).Receipt;
	}

	struct FRecoveryStoreFixture
	{
		FString Root;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;

		bool Begin(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewRecoveryStoreRoot(Label);
			FString Diagnostic;
			if (!Timeline.TryBegin(RecoveryRunId, Diagnostic))
			{
				Test.AddError(Diagnostic);
				return false;
			}
			return true;
		}

		Fdemo_mapShanmenTreatmentRecoveryStorageContext Storage() const
		{
			return Fdemo_mapShanmenTreatmentRecoveryStorageContext::ForRoot(
				Root, RecoveryOwnerId, RecoveryRunId);
		}

		bool CreateProof(
			FAutomationTestBase& Test,
			const FGuid& TargetId,
			const FGuid& ItemId,
			const FGuid& ImpactId,
			const FGuid& ResolutionId,
			const bool bAdvanceOneTick,
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& OutProof,
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent* OutIntent =
				nullptr)
		{
			OutProof =
				Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof();
			if (OutIntent)
			{
				*OutIntent =
					Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent();
			}
			FString Diagnostic;
			if (bAdvanceOneTick)
			{
				int64 AdvancedTicks = 0;
				if (!Timeline.TryAdvance(
						1.0 / static_cast<double>(
							Fdemo_mapShanmenCombatRunFixedTimeline::
								CanonicalTicksPerSecond()),
						AdvancedTicks,
						Diagnostic)
					|| AdvancedTicks != 1)
				{
					Test.AddError(FString::Printf(
						TEXT("Recovery fixture timeline advance failed: %s"),
						*Diagnostic));
					return false;
				}
			}

			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			Udemo_mapAttributeComponent* Attributes =
				NewObject<Udemo_mapAttributeComponent>(GetTransientPackage());
			Udemo_mapShanmenCombatConditionComponent* Conditions =
				NewObject<Udemo_mapShanmenCombatConditionComponent>(
					GetTransientPackage());
			if (!Attributes || !Conditions)
			{
				Test.AddError(
					TEXT("Recovery fixture could not allocate condition authority."));
				return false;
			}
			Attributes->AddToRoot();
			Conditions->AddToRoot();

			Fdemo_mapShanmenCombatConditionStatusSnapshot Status;
			Fdemo_mapShanmenCombatConditionTreatmentIntent Intent;
			const FShanmenVitalityCommitReceipt Vitality =
				MakeCommittedReceipt(TargetId, ImpactId, ResolutionId);
			const bool bArranged = Vitality.IsValid()
				&& Timeline.TryCapture(Sample)
				&& Conditions->TryBegin(
					RecoveryRunId,
					TargetId,
					Timeline.GetTimelineId(),
					Attributes,
					Diagnostic)
				&& Conditions->TryApplyMeridianShock(
					Vitality, Sample).IsSuccess()
				&& Conditions->TryCaptureMeridianShockStatus(Status)
				&& Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
					RecoveryRunId,
					TargetId,
					Timeline.GetTimelineId(),
					ItemId,
					Fdemo_mapItemIds::MeridianStabilizingPillLevel1,
					Status.GetConditionRevision(),
					Intent);
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent RecoveryIntent;
			const bool bIntentCaptured = !OutIntent
				|| (bArranged
					&& Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::
						TryCapture(Intent, Sample, RecoveryIntent));
			const Fdemo_mapShanmenCombatConditionTreatmentResult Treatment =
				bArranged && bIntentCaptured
					? Conditions->TryTreatMeridianShock(Intent, Sample)
					: Fdemo_mapShanmenCombatConditionTreatmentResult();
			const bool bCaptured = bArranged
				&& bIntentCaptured
				&& Treatment.IsSuccess()
				&& Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::
					TryCapture(Treatment.Receipt, OutProof);
			if (bCaptured && OutIntent)
			{
				*OutIntent = RecoveryIntent;
			}

			Conditions->TryEnd(RecoveryRunId, Diagnostic);
			Conditions->RemoveFromRoot();
			Conditions->MarkAsGarbage();
			Attributes->RemoveFromRoot();
			Attributes->MarkAsGarbage();
			if (!bCaptured)
			{
				Test.AddError(FString::Printf(
					TEXT("Recovery fixture could not capture proof: %s"),
					*Diagnostic));
			}
			return bCaptured;
		}

		~FRecoveryStoreFixture()
		{
			Timeline.Reset();
			CollectGarbage(RF_NoFlags);
			if (!Root.IsEmpty())
			{
				IFileManager::Get().DeleteDirectory(*Root, false, true);
			}
		}
	};

	bool ReadBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool WritePreviousSchemaDocument(
		const Fdemo_mapShanmenTreatmentRecoveryStorageContext& Storage,
		const FString& Path,
		TArray<Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof> Proofs,
		const int32 SaveGeneration)
	{
		Proofs.Sort([](
			const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Left,
			const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Right)
		{
			return GuidDigits(Left.GetTreatmentId())
				< GuidDigits(Right.GetTreatmentId());
		});
		TArray<FString> ProofSetParts =
			{
				TEXT("1"),
				GuidDigits(Storage.OwnerId),
				GuidDigits(Storage.RunId)
			};
		TArray<FString> JsonProofs;
		for (const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof :
			Proofs)
		{
			FString Encoded;
			if (!Proof.TryEncode(Encoded))
			{
				return false;
			}
			ProofSetParts.Add(Encoded);
			JsonProofs.Add(FString::Printf(TEXT("\"%s\""), *Encoded));
		}
		const FGuid DocumentId =
			FShanmenDeterministicId::FromCanonicalParts(
				TEXT("demo_map.Combat.Condition.MeridianShock.RecoveryStore.r1"),
				{ GuidDigits(Storage.OwnerId), GuidDigits(Storage.RunId) });
		const FGuid ProofSetId =
			FShanmenDeterministicId::FromCanonicalParts(
				TEXT("demo_map.Combat.Condition.MeridianShock.ProofSet.r1"),
				ProofSetParts);
		const FString Timestamp = FDateTime::UtcNow().ToIso8601();
		const FString Json = FString::Printf(
			TEXT("{\"SchemaVersion\":1,\"DocumentId\":\"%s\",\"OwnerId\":\"%s\",\"RunId\":\"%s\",\"SaveGeneration\":%d,\"CreatedUtc\":\"%s\",\"LastSavedUtc\":\"%s\",\"ProofSetId\":\"%s\",\"Proofs\":[%s]}"),
			*GuidDigits(DocumentId),
			*GuidDigits(Storage.OwnerId),
			*GuidDigits(Storage.RunId),
			SaveGeneration,
			*Timestamp,
			*Timestamp,
			*GuidDigits(ProofSetId),
			*FString::Join(JsonProofs, TEXT(",")));
		return (IFileManager::Get().DirectoryExists(
				*Storage.StorageDirectory())
				|| IFileManager::Get().MakeDirectory(
					*Storage.StorageDirectory(), true))
			&& FFileHelper::SaveStringToFile(
				Json,
				*Path,
				FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRecoveryStoreRoundTripTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.RoundTripReplayForget",
	RecoveryStoreFlags)

bool Fdemo_mapMeridianShockRecoveryStoreRoundTripTest::RunTest(
	const FString&)
{
	FRecoveryStoreFixture Fixture;
	if (!Fixture.Begin(*this, TEXT("RoundTripReplayForget")))
	{
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof FirstProof;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof SecondProof;
	if (!Fixture.CreateProof(
			*this,
			FGuid(0xC1650010, 0, 0, 1),
			FGuid(0xC1650010, 0, 0, 2),
			FGuid(0xC1650010, 0, 0, 3),
			FGuid(0xC1650010, 0, 0, 4),
			false,
			FirstProof)
		|| !Fixture.CreateProof(
			*this,
			FGuid(0xC1650020, 0, 0, 1),
			FGuid(0xC1650020, 0, 0, 2),
			FGuid(0xC1650020, 0, 0, 3),
			FGuid(0xC1650020, 0, 0, 4),
			false,
			SecondProof))
	{
		return false;
	}

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore Store;
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext Storage =
		Fixture.Storage();
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult RecordedSecond =
		Store.RecordProof(SecondProof, Storage);
	TArray<uint8> BeforeReplay;
	TestTrue(TEXT("first proof publishes generation one"),
		RecordedSecond.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Recorded
		&& RecordedSecond.Document.SaveGeneration == 1
		&& RecordedSecond.Document.Proofs.Num() == 1
		&& ReadBytes(Storage.PrimaryPath(), BeforeReplay));
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Replayed =
		Store.RecordProof(SecondProof, Storage);
	TArray<uint8> AfterReplay;
	TestTrue(TEXT("exact replay is byte- and generation-stable"),
		Replayed.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::AlreadyRecorded
		&& !Replayed.bDiskStateChanged
		&& Replayed.Document.SaveGeneration == 1
		&& ReadBytes(Storage.PrimaryPath(), AfterReplay)
		&& AfterReplay == BeforeReplay);

	const Fdemo_mapShanmenTreatmentRecoveryMutationResult RecordedFirst =
		Store.RecordProof(FirstProof, Storage);
	const bool bHasTwoProofs = RecordedFirst.Document.Proofs.Num() == 2;
	const FString FirstId = bHasTwoProofs
		? RecordedFirst.Document.Proofs[0].GetTreatmentId().ToString(
			EGuidFormats::Digits)
		: FString();
	const FString SecondId = bHasTwoProofs
		? RecordedFirst.Document.Proofs[1].GetTreatmentId().ToString(
			EGuidFormats::Digits)
		: FString();
	TestTrue(TEXT("second mutation persists canonical treatment-id order"),
		RecordedFirst.IsSuccess()
		&& RecordedFirst.Document.SaveGeneration == 2
		&& bHasTwoProofs
		&& FirstId < SecondId
		&& IFileManager::Get().FileExists(*Storage.BackupPath()));
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Loaded =
		Store.LoadExisting(Storage);
	TestTrue(TEXT("two-proof document reloads exactly"),
		Loaded.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::LoadedPrimary
		&& Loaded.Document == RecordedFirst.Document);

	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Forgotten =
		Store.ForgetProof(SecondProof.GetTreatmentId(), Storage);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult ForgottenReplay =
		Store.ForgetProof(SecondProof.GetTreatmentId(), Storage);
	TestTrue(TEXT("forget publishes an audit-safe document and replays"),
		Forgotten.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Forgotten
		&& Forgotten.Document.SaveGeneration == 3
		&& Forgotten.Document.Proofs.Num() == 1
		&& ForgottenReplay.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::AlreadyAbsent
		&& !ForgottenReplay.bDiskStateChanged
		&& ForgottenReplay.Document == Forgotten.Document);
	AddInfo(FString::Printf(
		TEXT("P16.5 store roundtrip generations=1/2/3 proofs=%d replayWrite=%d"),
		Forgotten.Document.Proofs.Num(),
		Replayed.bDiskStateChanged ? 1 : 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRecoveryStoreFailureIsolationTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.FailureIsolationRetry",
	RecoveryStoreFlags)

bool Fdemo_mapMeridianShockRecoveryStoreFailureIsolationTest::RunTest(
	const FString&)
{
	FRecoveryStoreFixture Fixture;
	if (!Fixture.Begin(*this, TEXT("FailureIsolationRetry")))
	{
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof FirstProof;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof SecondProof;
	if (!Fixture.CreateProof(
			*this,
			FGuid(0xC1650030, 0, 0, 1),
			FGuid(0xC1650030, 0, 0, 2),
			FGuid(0xC1650030, 0, 0, 3),
			FGuid(0xC1650030, 0, 0, 4),
			false,
			FirstProof)
		|| !Fixture.CreateProof(
			*this,
			FGuid(0xC1650040, 0, 0, 1),
			FGuid(0xC1650040, 0, 0, 2),
			FGuid(0xC1650040, 0, 0, 3),
			FGuid(0xC1650040, 0, 0, 4),
			false,
			SecondProof))
	{
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore Store;
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext Normal =
		Fixture.Storage();
	if (!Store.RecordProof(FirstProof, Normal).IsSuccess())
	{
		AddError(TEXT("Could not publish P16.5 failure-isolation fixture."));
		return false;
	}
	TArray<uint8> PrimaryBefore;
	ReadBytes(Normal.PrimaryPath(), PrimaryBefore);
	Fdemo_mapShanmenTreatmentRecoveryStorageContext Injected = Normal;
	Injected.InjectedFailure =
		Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::AtomicReplace;
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Failed =
		Store.RecordProof(SecondProof, Injected);
	TArray<uint8> PrimaryAfter;
	const bool bPrimaryPreserved =
		ReadBytes(Normal.PrimaryPath(), PrimaryAfter)
		&& PrimaryAfter == PrimaryBefore;
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Retried =
		Store.RecordProof(SecondProof, Normal);
	TestTrue(TEXT("pre-commit replacement failure preserves primary"),
		Failed.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::SaveFailed
		&& bPrimaryPreserved);
	TestTrue(TEXT("same proof retries to exactly one committed mutation"),
		Retried.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Recorded
		&& Retried.Document.SaveGeneration == 2
		&& Retried.Document.Proofs.Num() == 2);
	AddInfo(FString::Printf(
		TEXT("P16.5 failure isolation status=%d retryGeneration=%d"),
		static_cast<int32>(Failed.Status),
		Retried.Document.SaveGeneration));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRecoveryStoreBackupTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.BackupRecoveryFutureSchema",
	RecoveryStoreFlags)

bool Fdemo_mapMeridianShockRecoveryStoreBackupTest::RunTest(const FString&)
{
	FRecoveryStoreFixture Fixture;
	if (!Fixture.Begin(*this, TEXT("BackupRecoveryFutureSchema")))
	{
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof FirstProof;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof SecondProof;
	if (!Fixture.CreateProof(
			*this,
			FGuid(0xC1650050, 0, 0, 1),
			FGuid(0xC1650050, 0, 0, 2),
			FGuid(0xC1650050, 0, 0, 3),
			FGuid(0xC1650050, 0, 0, 4),
			false,
			FirstProof)
		|| !Fixture.CreateProof(
			*this,
			FGuid(0xC1650060, 0, 0, 1),
			FGuid(0xC1650060, 0, 0, 2),
			FGuid(0xC1650060, 0, 0, 3),
			FGuid(0xC1650060, 0, 0, 4),
			false,
			SecondProof))
	{
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore Store;
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext Storage =
		Fixture.Storage();
	if (!Store.RecordProof(FirstProof, Storage).IsSuccess()
		|| !Store.RecordProof(SecondProof, Storage).IsSuccess())
	{
		AddError(TEXT("Could not publish P16.5 backup fixture."));
		return false;
	}
	TArray<uint8> BackupBytes;
	ReadBytes(Storage.BackupPath(), BackupBytes);
	FFileHelper::SaveStringToFile(
		TEXT("{corrupt-primary"),
		*Storage.PrimaryPath(),
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Recovered =
		Store.LoadExisting(Storage);
	TArray<uint8> RecoveredBytes;
	TestTrue(TEXT("corrupt primary is quarantined and verified backup restored"),
		Recovered.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::RecoveredFromBackup
		&& Recovered.bDiskStateChanged
		&& !Recovered.QuarantinedPath.IsEmpty()
		&& IFileManager::Get().FileExists(*Recovered.QuarantinedPath)
		&& Recovered.Document.SaveGeneration == 1
		&& Recovered.Document.Proofs.Num() == 1
		&& ReadBytes(Storage.PrimaryPath(), RecoveredBytes)
		&& RecoveredBytes == BackupBytes);

	const FString FutureRoot = NewRecoveryStoreRoot(TEXT("FutureSchema"));
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext FutureStorage =
		Fdemo_mapShanmenTreatmentRecoveryStorageContext::ForRoot(
			FutureRoot, RecoveryOwnerId, RecoveryRunId);
	if (!Store.RecordProof(FirstProof, FutureStorage).IsSuccess())
	{
		AddError(TEXT("Could not publish P16.5 future-schema fixture."));
		IFileManager::Get().DeleteDirectory(*FutureRoot, false, true);
		return false;
	}
	FString Json;
	FFileHelper::LoadFileToString(Json, *FutureStorage.PrimaryPath());
	Json.ReplaceInline(TEXT("\"SchemaVersion\":2"), TEXT("\"SchemaVersion\":3"));
	FFileHelper::SaveStringToFile(
		Json,
		*FutureStorage.PrimaryPath(),
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Future =
		Store.LoadExisting(FutureStorage);
	TestTrue(TEXT("future schema rejects without reset or backup downgrade"),
		Future.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::FutureSchemaRejected
		&& !Future.bDiskStateChanged);
	const bool bMovedFutureToBackup = IFileManager::Get().Move(
		*FutureStorage.BackupPath(),
		*FutureStorage.PrimaryPath(),
		true,
		false,
		true,
		true);
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult FutureBackup =
		Store.LoadExisting(FutureStorage);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult FutureForget =
		Store.ForgetProof(FirstProof.GetTreatmentId(), FutureStorage);
	TestTrue(TEXT("future-schema backup alone is not misreported as absent"),
		bMovedFutureToBackup
		&& FutureBackup.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::
				FutureSchemaRejected
		&& FutureForget.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::LoadFailed
		&& !FutureBackup.bDiskStateChanged
		&& !FutureForget.bDiskStateChanged
		&& !IFileManager::Get().FileExists(*FutureStorage.PrimaryPath())
		&& IFileManager::Get().FileExists(*FutureStorage.BackupPath()));
	IFileManager::Get().DeleteDirectory(*FutureRoot, false, true);
	AddInfo(FString::Printf(
		TEXT("P16.5 recovery generation=%d quarantined=%d futureStatus=%d"),
		Recovered.Document.SaveGeneration,
		Recovered.QuarantinedPath.IsEmpty() ? 0 : 1,
		static_cast<int32>(Future.Status)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRecoveryStoreFenceTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.IdentityConflictFence",
	RecoveryStoreFlags)

bool Fdemo_mapMeridianShockRecoveryStoreFenceTest::RunTest(const FString&)
{
	FRecoveryStoreFixture Fixture;
	if (!Fixture.Begin(*this, TEXT("IdentityConflictFence")))
	{
		return false;
	}
	const FGuid TargetId(0xC1650070, 0, 0, 1);
	const FGuid ItemId(0xC1650070, 0, 0, 2);
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof FirstProof;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof ConflictingProof;
	if (!Fixture.CreateProof(
			*this,
			TargetId,
			ItemId,
			FGuid(0xC1650070, 0, 0, 3),
			FGuid(0xC1650070, 0, 0, 4),
			false,
			FirstProof)
		|| !Fixture.CreateProof(
			*this,
			TargetId,
			ItemId,
			FGuid(0xC1650070, 0, 0, 5),
			FGuid(0xC1650070, 0, 0, 6),
			true,
			ConflictingProof))
	{
		return false;
	}
	TestTrue(TEXT("fixture forms same treatment identity with different proof"),
		FirstProof.GetTreatmentId() == ConflictingProof.GetTreatmentId()
		&& FirstProof.GetProofId() != ConflictingProof.GetProofId());

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore Store;
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext Storage =
		Fixture.Storage();
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Recorded =
		Store.RecordProof(FirstProof, Storage);
	TArray<uint8> Before;
	ReadBytes(Storage.PrimaryPath(), Before);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Conflict =
		Store.RecordProof(ConflictingProof, Storage);
	TArray<uint8> After;
	Fdemo_mapShanmenTreatmentRecoveryStorageContext Foreign =
		Fdemo_mapShanmenTreatmentRecoveryStorageContext::ForRoot(
			Fixture.Root,
			RecoveryOwnerId,
			FGuid(0xC16500FF, 0, 0, 1));
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult CrossRun =
		Store.RecordProof(FirstProof, Foreign);
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext EmptyRoot =
		Fdemo_mapShanmenTreatmentRecoveryStorageContext::ForRoot(
			FString(), RecoveryOwnerId, RecoveryRunId);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult EmptyRootResult =
		Store.RecordProof(FirstProof, EmptyRoot);
	TestTrue(TEXT("same TreatmentId with different payload fails closed"),
		Recorded.IsSuccess()
		&& Conflict.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict
		&& !Conflict.bDiskStateChanged
		&& ReadBytes(Storage.PrimaryPath(), After)
		&& After == Before);
	TestTrue(TEXT("proof cannot cross its Owner/Run partition"),
		CrossRun.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest
		&& !IFileManager::Get().FileExists(*Foreign.PrimaryPath()));
	TestTrue(TEXT("empty storage root cannot degrade into the process directory"),
		!EmptyRoot.IsValid()
		&& EmptyRootResult.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest);
	AddInfo(FString::Printf(
		TEXT("P16.5 conflict treatment=%s ticks=%lld/%lld crossRun=%d"),
		*FirstProof.GetTreatmentId().ToString(EGuidFormats::Digits),
		static_cast<long long>(FirstProof.GetTreatedAtTick()),
		static_cast<long long>(ConflictingProof.GetTreatedAtTick()),
		static_cast<int32>(CrossRun.Status)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRecoveryStorePreviousSchemaMigrationTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.PreviousSchemaMigration",
	RecoveryStoreFlags)

bool Fdemo_mapMeridianShockRecoveryStorePreviousSchemaMigrationTest::RunTest(
	const FString&)
{
	FRecoveryStoreFixture Fixture;
	if (!Fixture.Begin(*this, TEXT("PreviousSchemaMigration")))
	{
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof Proof;
	if (!Fixture.CreateProof(
		*this,
		FGuid(0xC1680010, 0, 0, 1),
		FGuid(0xC1680010, 0, 0, 2),
		FGuid(0xC1680010, 0, 0, 3),
		FGuid(0xC1680010, 0, 0, 4),
		false,
		Proof))
	{
		return false;
	}

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore Store;
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext Storage =
		Fixture.Storage();
	if (!WritePreviousSchemaDocument(
		Storage, Storage.PrimaryPath(), { Proof }, 7))
	{
		AddError(TEXT("Could not write the schema-1 primary fixture."));
		return false;
	}
	TArray<uint8> PreviousPrimary;
	ReadBytes(Storage.PrimaryPath(), PreviousPrimary);
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Migrated =
		Store.LoadExisting(Storage);
	TArray<uint8> CurrentPrimary;
	TArray<uint8> PreviousBackup;
	FString CurrentJson;
	TestTrue(TEXT("schema-1 primary migrates once without losing proofs"),
		Migrated.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::
				MigratedPreviousSchema
		&& Migrated.IsSuccess()
		&& Migrated.bMigratedFromPreviousSchema
		&& !Migrated.bRecoveredFromBackup
		&& Migrated.bDiskStateChanged
		&& Migrated.SourceSchemaVersion
			== Fdemo_mapShanmenTreatmentRecoveryDocument::
				PreviousSchemaVersion
		&& Migrated.Document.SchemaVersion
			== Fdemo_mapShanmenTreatmentRecoveryDocument::CurrentSchemaVersion
		&& Migrated.Document.SaveGeneration == 8
		&& Migrated.Document.Intents.IsEmpty()
		&& Migrated.Document.Proofs.Num() == 1
		&& Migrated.Document.Proofs[0].Matches(Proof)
		&& ReadBytes(Storage.PrimaryPath(), CurrentPrimary)
		&& CurrentPrimary != PreviousPrimary
		&& ReadBytes(Storage.BackupPath(), PreviousBackup)
		&& PreviousBackup == PreviousPrimary
		&& FFileHelper::LoadFileToString(CurrentJson, *Storage.PrimaryPath())
		&& CurrentJson.Contains(TEXT("\"SchemaVersion\":2"))
		&& CurrentJson.Contains(TEXT("\"Intents\":[]")));

	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Reopened =
		Store.LoadExisting(Storage);
	TestTrue(TEXT("migrated primary reopens without another write"),
		Reopened.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::LoadedPrimary
		&& Reopened.SourceSchemaVersion
			== Fdemo_mapShanmenTreatmentRecoveryDocument::CurrentSchemaVersion
		&& !Reopened.bDiskStateChanged
		&& !Reopened.bMigratedFromPreviousSchema
		&& Reopened.Document.SaveGeneration == 8
		&& Reopened.Document == Migrated.Document);

	const FString BackupRoot = NewRecoveryStoreRoot(TEXT("PreviousBackupOnly"));
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext BackupStorage =
		Fdemo_mapShanmenTreatmentRecoveryStorageContext::ForRoot(
			BackupRoot, RecoveryOwnerId, RecoveryRunId);
	const bool bWroteBackup = WritePreviousSchemaDocument(
		BackupStorage, BackupStorage.BackupPath(), { Proof }, 11);
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult BackupMigrated =
		bWroteBackup
			? Store.LoadExisting(BackupStorage)
			: Fdemo_mapShanmenTreatmentRecoveryLoadResult();
	TestTrue(TEXT("backup-only schema-1 state restores and migrates"),
		bWroteBackup
		&& BackupMigrated.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::
				MigratedPreviousSchema
		&& BackupMigrated.bRecoveredFromBackup
		&& BackupMigrated.bMigratedFromPreviousSchema
		&& BackupMigrated.bDiskStateChanged
		&& BackupMigrated.SourceSchemaVersion
			== Fdemo_mapShanmenTreatmentRecoveryDocument::
				PreviousSchemaVersion
		&& BackupMigrated.Document.SaveGeneration == 12
		&& BackupMigrated.Document.Intents.IsEmpty()
		&& BackupMigrated.Document.Proofs.Num() == 1
		&& IFileManager::Get().FileExists(*BackupStorage.PrimaryPath())
		&& IFileManager::Get().FileExists(*BackupStorage.BackupPath()));
	IFileManager::Get().DeleteDirectory(*BackupRoot, false, true);

	const FString CorruptRoot = NewRecoveryStoreRoot(TEXT("PreviousCorrupt"));
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext CorruptStorage =
		Fdemo_mapShanmenTreatmentRecoveryStorageContext::ForRoot(
			CorruptRoot, RecoveryOwnerId, RecoveryRunId);
	const bool bWroteCorruptFixture = WritePreviousSchemaDocument(
		CorruptStorage, CorruptStorage.PrimaryPath(), { Proof }, 5);
	FString CorruptJson;
	FFileHelper::LoadFileToString(CorruptJson, *CorruptStorage.PrimaryPath());
	CorruptJson.ReplaceInline(
		TEXT("\"ProofSetId\":\""), TEXT("\"ProofSetId\":\"0"));
	const bool bTamperedPrevious = FFileHelper::SaveStringToFile(
		CorruptJson,
		*CorruptStorage.PrimaryPath(),
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult CorruptPrevious =
		bWroteCorruptFixture && bTamperedPrevious
			? Store.LoadExisting(CorruptStorage)
			: Fdemo_mapShanmenTreatmentRecoveryLoadResult();
	TestTrue(TEXT("invalid schema-1 integrity is rejected without migration"),
		bWroteCorruptFixture
		&& bTamperedPrevious
		&& CorruptPrevious.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::
				CorruptPrimaryNoValidBackup
		&& !CorruptPrevious.bDiskStateChanged
		&& !CorruptPrevious.bMigratedFromPreviousSchema
		&& IFileManager::Get().FileExists(*CorruptStorage.PrimaryPath())
		&& !IFileManager::Get().FileExists(*CorruptStorage.BackupPath()));
	IFileManager::Get().DeleteDirectory(*CorruptRoot, false, true);
	AddInfo(FString::Printf(
		TEXT("P16.8 schema migration generations=%d/%d backupRecovered=%d"),
		Migrated.Document.SaveGeneration,
		BackupMigrated.Document.SaveGeneration,
		BackupMigrated.bRecoveredFromBackup ? 1 : 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRecoveryStoreIntentPromotionTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.IntentPromotionAtomicity",
	RecoveryStoreFlags)

bool Fdemo_mapMeridianShockRecoveryStoreIntentPromotionTest::RunTest(
	const FString&)
{
	FRecoveryStoreFixture Fixture;
	if (!Fixture.Begin(*this, TEXT("IntentPromotionAtomicity")))
	{
		return false;
	}
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Intent;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent CancelledIntent;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof Proof;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof CancelledProof;
	if (!Fixture.CreateProof(
		*this,
		FGuid(0xC1680020, 0, 0, 1),
		FGuid(0xC1680020, 0, 0, 2),
		FGuid(0xC1680020, 0, 0, 3),
		FGuid(0xC1680020, 0, 0, 4),
		false,
		Proof,
		&Intent)
		|| !Fixture.CreateProof(
			*this,
			FGuid(0xC1680021, 0, 0, 1),
			FGuid(0xC1680021, 0, 0, 2),
			FGuid(0xC1680021, 0, 0, 3),
			FGuid(0xC1680021, 0, 0, 4),
			false,
			CancelledProof,
			&CancelledIntent))
	{
		return false;
	}

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore Store;
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext Storage =
		Fixture.Storage();
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Recorded =
		Store.RecordIntent(Intent, Storage);
	TArray<uint8> IntentPrimary;
	TestTrue(TEXT("prepared intent publishes generation one"),
		Recorded.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::IntentRecorded
		&& Recorded.Document.SaveGeneration == 1
		&& Recorded.Document.Intents.Num() == 1
		&& Recorded.Document.Proofs.IsEmpty()
		&& ReadBytes(Storage.PrimaryPath(), IntentPrimary));

	const Fdemo_mapShanmenTreatmentRecoveryMutationResult ReplayedIntent =
		Store.RecordIntent(Intent, Storage);
	TArray<uint8> ReplayedBytes;
	TestTrue(TEXT("exact intent replay is byte- and generation-stable"),
		ReplayedIntent.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
				IntentAlreadyRecorded
		&& !ReplayedIntent.bDiskStateChanged
		&& ReplayedIntent.Document.SaveGeneration == 1
		&& ReadBytes(Storage.PrimaryPath(), ReplayedBytes)
		&& ReplayedBytes == IntentPrimary);

	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Promoted =
		Store.PromoteIntentToProof(Intent, Proof, Storage);
	TArray<uint8> PromotedPrimary;
	TArray<uint8> PromotionBackup;
	FString IntentEncoded;
	FString ProofEncoded;
	FString PromotedJson;
	Intent.TryEncode(IntentEncoded);
	Proof.TryEncode(ProofEncoded);
	TestTrue(TEXT("one document mutation replaces intent with exact proof"),
		Promoted.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Promoted
		&& Promoted.Document.SaveGeneration == 2
		&& Promoted.Document.Intents.IsEmpty()
		&& Promoted.Document.Proofs.Num() == 1
		&& Promoted.Document.Proofs[0].Matches(Proof)
		&& ReadBytes(Storage.PrimaryPath(), PromotedPrimary)
		&& ReadBytes(Storage.BackupPath(), PromotionBackup)
		&& PromotionBackup == IntentPrimary
		&& PromotedPrimary != IntentPrimary
		&& FFileHelper::LoadFileToString(PromotedJson, *Storage.PrimaryPath())
		&& !PromotedJson.Contains(IntentEncoded)
		&& PromotedJson.Contains(ProofEncoded));

	const Fdemo_mapShanmenTreatmentRecoveryMutationResult ReplayedPromotion =
		Store.PromoteIntentToProof(Intent, Proof, Storage);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult LateIntent =
		Store.RecordIntent(Intent, Storage);
	TestTrue(TEXT("promotion replay and late intent cannot duplicate state"),
		ReplayedPromotion.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::AlreadyPromoted
		&& LateIntent.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::AlreadyPromoted
		&& !ReplayedPromotion.bDiskStateChanged
		&& !LateIntent.bDiskStateChanged
		&& ReplayedPromotion.Document == Promoted.Document
		&& LateIntent.Document == Promoted.Document);

	const Fdemo_mapShanmenTreatmentRecoveryMutationResult RecordedCancelled =
		Store.RecordIntent(CancelledIntent, Storage);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult ForgottenIntent =
		Store.ForgetIntent(CancelledIntent.GetTreatmentId(), Storage);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult ForgottenReplay =
		Store.ForgetIntent(CancelledIntent.GetTreatmentId(), Storage);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult ForgetPromoted =
		Store.ForgetIntent(Intent.GetTreatmentId(), Storage);
	TestTrue(TEXT("prepared intent cancellation is durable and idempotent"),
		RecordedCancelled.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::IntentRecorded
		&& RecordedCancelled.Document.SaveGeneration == 3
		&& RecordedCancelled.Document.Intents.Num() == 1
		&& RecordedCancelled.Document.Proofs.Num() == 1
		&& ForgottenIntent.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::IntentForgotten
		&& ForgottenIntent.Document.SaveGeneration == 4
		&& ForgottenIntent.Document.Intents.IsEmpty()
		&& ForgottenIntent.Document.Proofs.Num() == 1
		&& ForgottenReplay.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::
				IntentAlreadyAbsent
		&& ForgetPromoted.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::AlreadyPromoted
		&& !ForgottenReplay.bDiskStateChanged
		&& !ForgetPromoted.bDiskStateChanged);
	AddInfo(FString::Printf(
		TEXT("P16.8 intent promotion treatment=%s generations=1/2/3/4 replay=%d"),
		*Intent.GetTreatmentId().ToString(EGuidFormats::Digits),
		static_cast<int32>(ReplayedPromotion.Status)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRecoveryStoreIntentFailureFenceTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.IntentFailureConflictFence",
	RecoveryStoreFlags)

bool Fdemo_mapMeridianShockRecoveryStoreIntentFailureFenceTest::RunTest(
	const FString&)
{
	FRecoveryStoreFixture Fixture;
	if (!Fixture.Begin(*this, TEXT("IntentFailureConflictFence")))
	{
		return false;
	}
	const FGuid TargetId(0xC1680030, 0, 0, 1);
	const FGuid ItemId(0xC1680030, 0, 0, 2);
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Intent;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent ConflictingIntent;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof Proof;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof ConflictingProof;
	if (!Fixture.CreateProof(
		*this,
		TargetId,
		ItemId,
		FGuid(0xC1680030, 0, 0, 3),
		FGuid(0xC1680030, 0, 0, 4),
		false,
		Proof,
		&Intent)
		|| !Fixture.CreateProof(
			*this,
			TargetId,
			ItemId,
			FGuid(0xC1680030, 0, 0, 5),
			FGuid(0xC1680030, 0, 0, 6),
			true,
			ConflictingProof,
			&ConflictingIntent))
	{
		return false;
	}
	TestTrue(TEXT("fixture shares TreatmentId but changes sampled decision"),
		Intent.GetTreatmentId() == ConflictingIntent.GetTreatmentId()
		&& Intent.GetIntentId() != ConflictingIntent.GetIntentId()
		&& Proof.GetProofId() != ConflictingProof.GetProofId());

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore Store;
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext Storage =
		Fixture.Storage();
	if (!Store.RecordIntent(Intent, Storage).IsSuccess())
	{
		AddError(TEXT("Could not publish the P16.8 intent failure fixture."));
		return false;
	}
	TArray<uint8> Before;
	ReadBytes(Storage.PrimaryPath(), Before);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult IntentConflict =
		Store.RecordIntent(ConflictingIntent, Storage);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult ProofMismatch =
		Store.PromoteIntentToProof(Intent, ConflictingProof, Storage);
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Bypass =
		Store.RecordProof(Proof, Storage);
	TArray<uint8> AfterRejected;
	TestTrue(TEXT("conflict, mismatched proof and RecordProof bypass fail closed"),
		IntentConflict.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict
		&& ProofMismatch.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::InvalidRequest
		&& Bypass.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Conflict
		&& !IntentConflict.bDiskStateChanged
		&& !ProofMismatch.bDiskStateChanged
		&& !Bypass.bDiskStateChanged
		&& ReadBytes(Storage.PrimaryPath(), AfterRejected)
		&& AfterRejected == Before);

	Fdemo_mapShanmenTreatmentRecoveryStorageContext Injected = Storage;
	Injected.InjectedFailure =
		Edemo_mapShanmenTreatmentRecoveryStoreFailureStage::AtomicReplace;
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult FailedPromotion =
		Store.PromoteIntentToProof(Intent, Proof, Injected);
	TArray<uint8> AfterFailure;
	const bool bPrimaryPreserved = ReadBytes(
		Storage.PrimaryPath(), AfterFailure)
		&& AfterFailure == Before;
	const Fdemo_mapShanmenTreatmentRecoveryMutationResult Retried =
		Store.PromoteIntentToProof(Intent, Proof, Storage);
	TestTrue(TEXT("pre-commit promotion failure preserves durable intent"),
		FailedPromotion.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::SaveFailed
		&& bPrimaryPreserved);
	TestTrue(TEXT("same promotion retry commits one exact generation"),
		Retried.Status
			== Edemo_mapShanmenTreatmentRecoveryMutationStatus::Promoted
		&& Retried.Document.SaveGeneration == 2
		&& Retried.Document.Intents.IsEmpty()
		&& Retried.Document.Proofs.Num() == 1
		&& Retried.Document.Proofs[0].Matches(Proof));

	const FString CorruptRoot = NewRecoveryStoreRoot(TEXT("IntentCorruption"));
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext CorruptStorage =
		Fdemo_mapShanmenTreatmentRecoveryStorageContext::ForRoot(
			CorruptRoot, RecoveryOwnerId, RecoveryRunId);
	const bool bRecordedCorruptFixture =
		Store.RecordIntent(Intent, CorruptStorage).IsSuccess();
	FString CorruptJson;
	FFileHelper::LoadFileToString(CorruptJson, *CorruptStorage.PrimaryPath());
	CorruptJson.ReplaceInline(
		*Intent.GetIntentId().ToString(EGuidFormats::Digits),
		*FGuid(0xC16800FF, 0, 0, 1).ToString(EGuidFormats::Digits));
	const bool bWroteCorrupt = FFileHelper::SaveStringToFile(
		CorruptJson,
		*CorruptStorage.PrimaryPath(),
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const Fdemo_mapShanmenTreatmentRecoveryLoadResult Corrupt =
		bRecordedCorruptFixture && bWroteCorrupt
			? Store.LoadExisting(CorruptStorage)
			: Fdemo_mapShanmenTreatmentRecoveryLoadResult();
	TestTrue(TEXT("tampered intent without valid backup is never reset"),
		bRecordedCorruptFixture
		&& bWroteCorrupt
		&& Corrupt.Status
			== Edemo_mapShanmenTreatmentRecoveryLoadStatus::
				CorruptPrimaryNoValidBackup
		&& !Corrupt.bDiskStateChanged
		&& IFileManager::Get().FileExists(*CorruptStorage.PrimaryPath()));
	IFileManager::Get().DeleteDirectory(*CorruptRoot, false, true);
	AddInfo(FString::Printf(
		TEXT("P16.8 intent fences conflict=%d failure=%d retryGeneration=%d"),
		static_cast<int32>(IntentConflict.Status),
		static_cast<int32>(FailedPromotion.Status),
		Retried.Document.SaveGeneration));
	return true;
}

#endif
