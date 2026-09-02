#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenMeridianShockTreatmentRecoveryStore.h"

#include "demo_mapAttributeComponent.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenCombatConditionComponent.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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
			TEXT("Dev.D.UE.0.0.10.P16.5.r0"),
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
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& OutProof)
		{
			OutProof =
				Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof();
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
			const Fdemo_mapShanmenCombatConditionTreatmentResult Treatment =
				bArranged
					? Conditions->TryTreatMeridianShock(Intent, Sample)
					: Fdemo_mapShanmenCombatConditionTreatmentResult();
			const bool bCaptured = bArranged
				&& Treatment.IsSuccess()
				&& Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::
					TryCapture(Treatment.Receipt, OutProof);

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
	Json.ReplaceInline(TEXT("\"SchemaVersion\":1"), TEXT("\"SchemaVersion\":2"));
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

#endif
