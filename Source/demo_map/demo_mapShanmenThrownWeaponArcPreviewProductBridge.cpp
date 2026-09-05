#include "demo_mapShanmenThrownWeaponArcPreviewProductBridge.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapProfilePreparationTypes.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString FloatBits(float Value)
	{
		Value = Value == 0.0f ? 0.0f : Value;
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	FString DoubleBits(double Value)
	{
		Value = Value == 0.0 ? 0.0 : Value;
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	void AppendCanonicalTags(
		TArray<FString>& Parts,
		const TCHAR* Prefix,
		const FGameplayTagContainer& Container)
	{
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.ToString() < Right.ToString();
		});
		Parts.Add(FString::Printf(TEXT("%s.Count=%d"), Prefix, Tags.Num()));
		for (const FGameplayTag& Tag : Tags)
		{
			Parts.Add(FString::Printf(TEXT("%s.%s"), Prefix, *Tag.ToString()));
		}
	}

	bool IsValidSlot(const int32 HotbarSlotNumber)
	{
		return HotbarSlotNumber >= 1
			&& HotbarSlotNumber
				<= Fdemo_mapPersistentPreparationLayout::HotbarSlotCount;
	}

	bool IsPreviewChoiceValid(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState)
	{
		return ChoiceState.IsValid()
			&& ChoiceState.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& ChoiceState.HasArcTargetIntent();
	}

	bool IsValidSequence(const uint64 Sequence)
	{
		return Sequence > 0 && Sequence < MAX_uint64;
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version && Left.Digest == Right.Digest;
	}

	FGuid MakePreviewRequestId(
		const int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		const int32 SegmentCount,
		const FShanmenContentStamp& AuthorityContent,
		const Fdemo_mapShanmenThrownWeaponSessionConfig& SessionConfig,
		const FGuid& RunId,
		const FGuid& PlayerEntityId,
		const FGuid& SourceItemInstanceId,
		const uint64 ObservedNextActivationSequence)
	{
		if (!IsValidSlot(HotbarSlotNumber)
			|| !IsPreviewChoiceValid(ChoiceState)
			|| !ChoicePolicy.IsValid()
			|| !AuthorityContent.IsValid()
			|| !SessionConfig.IsValid()
			|| SessionConfig.GetTrajectoryKind() != ETrajectory::BallisticArc
			|| !RunId.IsValid() || !PlayerEntityId.IsValid()
			|| !SourceItemInstanceId.IsValid()
			|| !IsValidSequence(ObservedNextActivationSequence))
		{
			return FGuid();
		}

		const FShanmenThrownWeaponDefinitionCapture& Definition =
			SessionConfig.GetDefinition();
		const Fdemo_mapShanmenThrownWeaponArcProductPolicy& ArcPolicy =
			SessionConfig.GetArcPolicy();
		TArray<FString> Parts = {
			FString::FromInt(HotbarSlotNumber),
			GuidDigits(ChoiceState.GetStateId()),
			GuidDigits(ChoiceState.GetLastCommandId()),
			FString::Printf(
				TEXT("%llu"),
				static_cast<unsigned long long>(ChoiceState.GetRevision())),
			DoubleBits(ChoiceState.GetArcTargetIntent().X),
			DoubleBits(ChoiceState.GetArcTargetIntent().Y),
			DoubleBits(ChoiceState.GetArcApexAdjustment()),
			GuidDigits(ChoicePolicy.GetPolicyId()),
			FString::FromInt(SegmentCount),
			AuthorityContent.Version.ToString(),
			AuthorityContent.Digest,
			GuidDigits(RunId),
			GuidDigits(PlayerEntityId),
			GuidDigits(SourceItemInstanceId),
			FString::Printf(
				TEXT("%llu"),
				static_cast<unsigned long long>(
					ObservedNextActivationSequence)),
			Definition.ActionDefinitionId.ToString(),
			Definition.DetectorId.ToString(),
			Definition.FormulaId.ToString(),
			FloatBits(Definition.BaseDamage),
			FloatBits(Definition.TechniquePowerCoefficient),
			FloatBits(Definition.LaunchSpeed),
			Definition.bRejectSelf ? TEXT("RejectSelf=1") : TEXT("RejectSelf=0"),
			FString::FromInt(static_cast<int32>(ArcPolicy.GetTechniqueTier())),
			DoubleBits(ArcPolicy.GetGravityMagnitude()),
			DoubleBits(ArcPolicy.GetMaximumFlightTime())
		};
		AppendCanonicalTags(Parts, TEXT("Damage"), Definition.DamageTags);
		AppendCanonicalTags(
			Parts, TEXT("RequiredTarget"), Definition.RequiredTargetTags);
		AppendCanonicalTags(
			Parts, TEXT("Source"), SessionConfig.GetSourceTags());
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT(
				"demo_map.ShanmenThrownWeapon.ArcPreviewProductRequest.r1")),
			Parts);
	}

	bool CaptureMatchesBridge(
		const Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult& Result)
	{
		const auto& Capture = Result.GetCapture();
		if (!Capture.IsCaptured())
		{
			return false;
		}
		const auto& Request = Capture.GetRequest();
		return Request.GetPreviewRequestId() == Result.GetPreviewRequestId()
			&& Request.GetRunId() == Result.GetLifecycleRunId()
			&& Request.GetPlayerEntityId() == Result.GetPlayerEntityId()
			&& Request.GetSourceItemInstanceId()
				== Result.GetSourceItemInstanceId()
			&& SameContent(
				Request.GetAuthorityContent(), Result.GetAuthorityContent())
			&& Request.GetObservedNextActivationSequence()
				== Result.GetSequenceBefore()
			&& Request.GetSessionConfig().Matches(Result.GetSessionConfig())
			&& Request.GetChoicePolicy().Matches(Result.GetChoicePolicy())
			&& Request.GetSegmentCount() == Result.GetSegmentCount();
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult::
	IsCaptured() const
{
	if (Status != EStatus::Captured
		|| Diagnostic.IsEmpty()
		|| !IsValidSlot(HotbarSlotNumber)
		|| !IsPreviewChoiceValid(ChoiceState)
		|| !ChoicePolicy.IsValid()
		|| !AuthorityContent.IsValid()
		|| !SessionConfig.IsValid()
		|| SessionConfig.GetTrajectoryKind() != ETrajectory::BallisticArc
		|| !LifecycleRunId.IsValid()
		|| CoordinatorRunId != LifecycleRunId
		|| !PlayerEntityId.IsValid()
		|| !SourceItemInstanceId.IsValid()
		|| SequenceReadCount != 2
		|| !IsValidSequence(SequenceBefore)
		|| SequenceAfter != SequenceBefore)
	{
		return false;
	}
	const FGuid ExpectedRequestId = MakePreviewRequestId(
		HotbarSlotNumber,
		ChoiceState,
		ChoicePolicy,
		SegmentCount,
		AuthorityContent,
		SessionConfig,
		LifecycleRunId,
		PlayerEntityId,
		SourceItemInstanceId,
		SequenceBefore);
	return PreviewRequestId.IsValid()
		&& PreviewRequestId == ExpectedRequestId
		&& CaptureMatchesBridge(*this);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult& Other)
	const
{
	return IsCaptured() && Other.IsCaptured()
		&& HotbarSlotNumber == Other.HotbarSlotNumber
		&& ChoiceState.Matches(Other.ChoiceState)
		&& ChoicePolicy.Matches(Other.ChoicePolicy)
		&& SegmentCount == Other.SegmentCount
		&& SameContent(AuthorityContent, Other.AuthorityContent)
		&& SessionConfig.Matches(Other.SessionConfig)
		&& LifecycleRunId == Other.LifecycleRunId
		&& CoordinatorRunId == Other.CoordinatorRunId
		&& PlayerEntityId == Other.PlayerEntityId
		&& SourceItemInstanceId == Other.SourceItemInstanceId
		&& SequenceBefore == Other.SequenceBefore
		&& SequenceAfter == Other.SequenceAfter
		&& SequenceReadCount == Other.SequenceReadCount
		&& PreviewRequestId == Other.PreviewRequestId
		&& Capture.Matches(Other.Capture);
}

Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult
Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
	const int32 HotbarSlotNumber,
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
	const int32 SegmentCount,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult Result;
	Result.HotbarSlotNumber = HotbarSlotNumber;
	Result.ChoiceState = ChoiceState;
	Result.ChoicePolicy = ChoicePolicy;
	Result.SegmentCount = SegmentCount;
	Result.AuthorityContent =
		Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp();

	if (!IsValidSlot(HotbarSlotNumber)
		|| !ChoicePolicy.IsValid()
		|| SegmentCount
			< FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount
		|| SegmentCount
			> FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount)
	{
		Result.Status = EStatus::InputRejected;
		Result.Diagnostic =
			TEXT("Arc preview bridge requires one valid slot, policy, and segment count.");
		return Result;
	}
	if (!IsPreviewChoiceValid(ChoiceState))
	{
		Result.Status = EStatus::ChoiceRejected;
		Result.Diagnostic =
			TEXT("Arc preview bridge requires one valid Arc choice with target intent.");
		return Result;
	}
	if (!Result.AuthorityContent.IsValid()
		|| !Lifecycle.IsActive()
		|| !Lifecycle.IsValid()
		|| Lifecycle.GetTrajectoryKind() != ETrajectory::BallisticArc
		|| !Lifecycle.TryCaptureReadOnlyHotbarBinding(
			HotbarSlotNumber,
			Result.SourceItemInstanceId,
			Result.SessionConfig))
	{
		Result.Status = EStatus::ProductUnavailable;
		Result.Diagnostic =
			TEXT("Arc preview bridge could not read one exact active product hotbar binding.");
		return Result;
	}

	Result.LifecycleRunId = Lifecycle.GetRunId();
	Result.CoordinatorRunId = Coordinator.GetRunId();
	Result.PlayerEntityId = Coordinator.GetPlayerEntityId();
	if (!Coordinator.IsReady()
		|| !Result.LifecycleRunId.IsValid()
		|| Result.CoordinatorRunId != Result.LifecycleRunId
		|| !Result.PlayerEntityId.IsValid())
	{
		Result.Status = EStatus::RunUnavailable;
		Result.Diagnostic =
			TEXT("Arc preview product lifecycle and coordinator must name one ready player Run.");
		return Result;
	}

	Result.SequenceBefore =
		Coordinator.GetNextPlayerThrownWeaponActivationSequence();
	Result.SequenceReadCount = 1;
	if (!IsValidSequence(Result.SequenceBefore))
	{
		Result.Status = EStatus::SequenceUnavailable;
		Result.Diagnostic =
			TEXT("Arc preview cannot observe an exhausted thrown-weapon sequence.");
		return Result;
	}

	Result.PreviewRequestId = MakePreviewRequestId(
		Result.HotbarSlotNumber,
		Result.ChoiceState,
		Result.ChoicePolicy,
		Result.SegmentCount,
		Result.AuthorityContent,
		Result.SessionConfig,
		Result.LifecycleRunId,
		Result.PlayerEntityId,
		Result.SourceItemInstanceId,
		Result.SequenceBefore);
	Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest Request;
	const bool bRequestCaptured =
		Result.PreviewRequestId.IsValid()
		&& Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::TryCapture(
			Result.PreviewRequestId,
			Result.LifecycleRunId,
			Result.PlayerEntityId,
			Result.SourceItemInstanceId,
			Result.AuthorityContent,
			Result.SequenceBefore,
			Result.SessionConfig,
			Result.ChoicePolicy,
			Result.SegmentCount,
			Request);
	Result.Capture =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(Request);
	Result.SequenceAfter =
		Coordinator.GetNextPlayerThrownWeaponActivationSequence();
	Result.SequenceReadCount = 2;

	if (!bRequestCaptured)
	{
		Result.Status = EStatus::RequestRejected;
		Result.Diagnostic =
			TEXT("Live product evidence could not enter the immutable preview request.");
		return Result;
	}
	if (Result.SequenceAfter != Result.SequenceBefore)
	{
		Result.Status = EStatus::SequenceChanged;
		Result.Diagnostic =
			TEXT("Thrown-weapon action sequence changed during preview capture.");
		return Result;
	}
	if (!Result.Capture.IsCaptured())
	{
		Result.Status = EStatus::CaptureRejected;
		Result.Diagnostic = Result.Capture.GetDiagnostic();
		return Result;
	}

	Result.Status = EStatus::Captured;
	Result.Diagnostic =
		TEXT("Captured one live Arc preview without reserving product or inventory state.");
	return Result;
}
