#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationMasteryAuthorityAdapter.h"

#include "Misc/AutomationTest.h"

namespace
{
	using EProjectionStatus =
		Edemo_mapShanmenFormationMasteryProjectionStatus;

	const FGuid MasteryOwner(0xF8F15001, 0, 0, 1);
	const FGuid ForeignOwner(0xF8F15002, 0, 0, 2);

	FShanmenContentStamp MakeContent(const TCHAR* Digest)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P27.15.Test");
		Content.Digest = Digest;
		return Content;
	}

	Fdemo_mapShanmenFormationMasteryAuthorityCapture MakeCapture(
		const FShanmenContentStamp& Content,
		const FGuid& OwnerId,
		const int64 AuthorityRevision,
		const EShanmenFormationMasteryTier Tier)
	{
		Fdemo_mapShanmenFormationMasteryAuthorityCapture Capture;
		Capture.OwnerId = OwnerId;
		Capture.AuthorityRevision = AuthorityRevision;
		Capture.Content = Content;
		Capture.MasteryTier = Tier;
		return Capture;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMasteryAuthorityDeterminismTest,
	"Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter.DeterministicSingleReadProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMasteryAuthorityDeterminismTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("determinism"));
	int32 TotalReads = 0;
	FGuid LastRequestedOwner;
	auto ReadAuthority = [&TotalReads, &LastRequestedOwner, &Content](
		const FGuid& RequestedOwner,
		Fdemo_mapShanmenFormationMasteryAuthorityCapture& OutCapture,
		FString& OutDiagnostic)
	{
		++TotalReads;
		LastRequestedOwner = RequestedOwner;
		OutCapture = MakeCapture(
			Content,
			MasteryOwner,
			17,
			EShanmenFormationMasteryTier::Intermediate);
		OutDiagnostic = TEXT("Authority read completed.");
		return true;
	};

	const auto First =
		Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, MasteryOwner, ReadAuthority);
	const auto Replay =
		Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, MasteryOwner, ReadAuthority);
	TestTrue(TEXT("Each projection performs exactly one authority read"),
		TotalReads == 2
			&& First.AuthorityReadCount == 1
			&& Replay.AuthorityReadCount == 1
			&& LastRequestedOwner == MasteryOwner);
	TestTrue(TEXT("Exact authority replay has deterministic evidence"),
		First.IsProjected()
			&& Replay.IsProjected()
			&& First.AuthorityRead.GetReadId()
				== Replay.AuthorityRead.GetReadId()
			&& First.AuthorityRead.GetOwnerId() == MasteryOwner
			&& First.AuthorityRead.GetAuthorityRevision() == 17
			&& First.AuthorityRead.GetMasteryPolicy().GetTier()
				== EShanmenFormationMasteryTier::Intermediate);
	TestTrue(TEXT("Projected policy carries the intermediate operation"),
		First.AuthorityRead.GetMasteryPolicy().CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::ProximityFill)
		&& First.AuthorityRead.GetMasteryPolicy().CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::RemoteThrow)
		&& !First.AuthorityRead.GetMasteryPolicy().CanUseDeliveryMode(
			EShanmenFormationMaterialDeliveryMode::ScatterFormation));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMasteryAuthorityPreflightTest,
	"Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter.PreflightAndAvailabilityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMasteryAuthorityPreflightTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("preflight"));
	int32 ReadCount = 0;
	auto UnexpectedRead = [&ReadCount](
		const FGuid&,
		Fdemo_mapShanmenFormationMasteryAuthorityCapture&,
		FString&)
	{
		++ReadCount;
		return true;
	};
	const auto InvalidContent =
		Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			FShanmenContentStamp(), MasteryOwner, UnexpectedRead);
	const auto InvalidOwner =
		Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, FGuid(), UnexpectedRead);
	TestTrue(TEXT("Invalid preflight evidence never reaches the authority"),
		ReadCount == 0
			&& InvalidContent.IsValid()
			&& InvalidContent.Status == EProjectionStatus::ContentInvalid
			&& InvalidOwner.IsValid()
			&& InvalidOwner.Status == EProjectionStatus::OwnerInvalid);

	auto UnavailableRead = [&ReadCount](
		const FGuid&,
		Fdemo_mapShanmenFormationMasteryAuthorityCapture&,
		FString& OutDiagnostic)
	{
		++ReadCount;
		OutDiagnostic = TEXT("Test mastery authority unavailable.");
		return false;
	};
	const auto Unavailable =
		Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, MasteryOwner, UnavailableRead);
	auto MalformedRead = [&ReadCount](
		const FGuid&,
		Fdemo_mapShanmenFormationMasteryAuthorityCapture&,
		FString&)
	{
		++ReadCount;
		return true;
	};
	const auto Malformed =
		Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, MasteryOwner, MalformedRead);
	TestTrue(TEXT("Unavailable and malformed reads fail closed after one call"),
		ReadCount == 2
			&& Unavailable.IsValid()
			&& Unavailable.Status
				== EProjectionStatus::AuthorityUnavailable
			&& Unavailable.AuthorityReadCount == 1
			&& !Unavailable.AuthorityRead.IsValid()
			&& Malformed.IsValid()
			&& Malformed.Status
				== EProjectionStatus::AuthorityReadInvalid
			&& Malformed.AuthorityReadCount == 1
			&& !Malformed.AuthorityRead.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMasteryAuthorityIdentityFenceTest,
	"Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter.OwnerContentAndTierFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMasteryAuthorityIdentityFenceTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("current"));
	const FShanmenContentStamp StaleContent = MakeContent(TEXT("stale"));
	int32 ReadCount = 0;
	auto ForeignRead = [&ReadCount, &Content](
		const FGuid&,
		Fdemo_mapShanmenFormationMasteryAuthorityCapture& OutCapture,
		FString&)
	{
		++ReadCount;
		OutCapture = MakeCapture(
			Content,
			ForeignOwner,
			3,
			EShanmenFormationMasteryTier::Beginner);
		return true;
	};
	const auto Foreign =
		Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, MasteryOwner, ForeignRead);

	auto StaleRead = [&ReadCount, &StaleContent](
		const FGuid&,
		Fdemo_mapShanmenFormationMasteryAuthorityCapture& OutCapture,
		FString&)
	{
		++ReadCount;
		OutCapture = MakeCapture(
			StaleContent,
			MasteryOwner,
			4,
			EShanmenFormationMasteryTier::Intermediate);
		return true;
	};
	const auto Stale =
		Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, MasteryOwner, StaleRead);

	auto ForgedTierRead = [&ReadCount, &Content](
		const FGuid&,
		Fdemo_mapShanmenFormationMasteryAuthorityCapture& OutCapture,
		FString&)
	{
		++ReadCount;
		OutCapture = MakeCapture(
			Content,
			MasteryOwner,
			5,
			static_cast<EShanmenFormationMasteryTier>(255));
		return true;
	};
	const auto Forged =
		Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, MasteryOwner, ForgedTierRead);

	TestTrue(TEXT("Owner content and tier boundaries fail closed"),
		ReadCount == 3
			&& Foreign.IsValid()
			&& Foreign.Status == EProjectionStatus::OwnerMismatch
			&& Foreign.AuthorityRead.IsValid()
			&& Stale.IsValid()
			&& Stale.Status == EProjectionStatus::ContentMismatch
			&& Stale.AuthorityRead.IsValid()
			&& Forged.IsValid()
			&& Forged.Status
				== EProjectionStatus::AuthorityReadInvalid
			&& !Forged.AuthorityRead.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMasteryAuthorityReadIdentityTest,
	"Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter.RevisionAndTierIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMasteryAuthorityReadIdentityTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("identity"));
	FString Diagnostic;
	Fdemo_mapShanmenFormationMasteryAuthorityRead First;
	Fdemo_mapShanmenFormationMasteryAuthorityRead Replay;
	Fdemo_mapShanmenFormationMasteryAuthorityRead Revised;
	Fdemo_mapShanmenFormationMasteryAuthorityRead Master;
	TestTrue(TEXT("Valid authority reads capture immutable evidence"),
		Fdemo_mapShanmenFormationMasteryAuthorityRead::TryCapture(
			MakeCapture(
				Content,
				MasteryOwner,
				7,
				EShanmenFormationMasteryTier::Beginner),
			First,
			Diagnostic)
		&& Fdemo_mapShanmenFormationMasteryAuthorityRead::TryCapture(
			MakeCapture(
				Content,
				MasteryOwner,
				7,
				EShanmenFormationMasteryTier::Beginner),
			Replay,
			Diagnostic)
		&& Fdemo_mapShanmenFormationMasteryAuthorityRead::TryCapture(
			MakeCapture(
				Content,
				MasteryOwner,
				8,
				EShanmenFormationMasteryTier::Beginner),
			Revised,
			Diagnostic)
		&& Fdemo_mapShanmenFormationMasteryAuthorityRead::TryCapture(
			MakeCapture(
				Content,
				MasteryOwner,
				7,
				EShanmenFormationMasteryTier::Master),
			Master,
			Diagnostic));
	TestTrue(TEXT("Read identity binds revision and mastery tier"),
		First.IsValid()
			&& First.GetReadId() == Replay.GetReadId()
			&& First.GetReadId() != Revised.GetReadId()
			&& First.GetReadId() != Master.GetReadId());

	TestFalse(TEXT("Rejected recapture clears prior authority evidence"),
		Fdemo_mapShanmenFormationMasteryAuthorityRead::TryCapture(
			MakeCapture(
				Content,
				MasteryOwner,
				7,
				EShanmenFormationMasteryTier::Invalid),
			Master,
			Diagnostic));
	TestFalse(TEXT("Cleared authority evidence remains invalid"),
		Master.IsValid());
	return true;
}

#endif
