#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationProductSession.h"

#include "ShanmenCombatResolver.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenItemCutover.h"
#include "demo_mapShanmenPreparationAdapter.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid AttemptA(0xF8200001, 0, 0, 1);
	const FGuid AttemptB(0xF8200002, 0, 0, 1);
	const FGuid AttemptOther(0xF8200003, 0, 0, 1);
	const FGuid SourceEntityId(0xF8200010, 0, 0, 1);
	const FName AnchorA(TEXT("Formation.Anchor.ProductSession.A"));
	const FName AnchorB(TEXT("Formation.Anchor.ProductSession.B"));

	FString NewFormationSessionRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P8.2.r0"), Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenFormationDiagramDefinition MakeDiagram()
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId =
			TEXT("Formation.Diagram.ProductSession.P8.2");
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.ActivationEnergy.ProductSession.P8.2");
		Capture.ActivationEnergyCost.ResourceChannel =
			FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel();
		Capture.ActivationEnergyCost.Amount = 10.0f;
		FShanmenFormationAnchorCapture& First =
			Capture.Anchors.AddDefaulted_GetRef();
		First.Order = 0;
		First.AnchorDefinitionId = AnchorA;
		First.RelativeOffset = FVector(100.0, -50.0, 0.0);
		FShanmenFormationMaterialRequirementCapture& FirstWood =
			First.Requirements.AddDefaulted_GetRef();
		FirstWood.Order = 0;
		FirstWood.MaterialDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
		FirstWood.Quantity = 1;
		FShanmenFormationAnchorCapture& Second =
			Capture.Anchors.AddDefaulted_GetRef();
		Second.Order = 1;
		Second.AnchorDefinitionId = AnchorB;
		Second.RelativeOffset = FVector(100.0, 50.0, 0.0);
		FShanmenFormationMaterialRequirementCapture& SecondWood =
			Second.Requirements.AddDefaulted_GetRef();
		SecondWood.Order = 0;
		SecondWood.MaterialDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
		SecondWood.Quantity = 2;
		FShanmenFormationDiagramDefinition Diagram;
		check(FShanmenFormationDiagramDefinition::TryCapture(
			Capture, Diagram));
		return Diagram;
	}

	struct FFormationSessionFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid WoodAId;
		FGuid WoodBId;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		Udemo_mapProfileSessionSubsystem* ProfileSession = nullptr;
		Fdemo_mapShanmenRunCorrelation Correlation;
		Fdemo_mapShanmenFormationProductSession Session;

		bool Start(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewFormationSessionRoot(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			Fdemo_mapPersistentItemRecord WoodA;
			WoodAId = WoodA.ItemInstanceId = FGuid::NewGuid();
			WoodA.ItemDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
			WoodA.StackCount = 2;
			WoodA.PersistentDomain =
				Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(WoodA);
			Fdemo_mapPersistentItemRecord WoodB = WoodA;
			WoodBId = WoodB.ItemInstanceId = FGuid::NewGuid();
			SeedProfile.PermanentStash.Add(WoodB);
			const Fdemo_mapProfileSaveResult Saved =
				Repository.SaveProfile(SeedProfile, Storage);
			if (!Saved.IsSuccess() || !GEngine)
			{
				Test.AddError(FString::Printf(
					TEXT("P8.2 isolated seed failed: %s"),
					*Saved.Diagnostic));
				return false;
			}

			GameInstance = NewObject<UGameInstance>(
				GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			Authority = GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			ProfileSession = GameInstance->GetSubsystem<
				Udemo_mapProfileSessionSubsystem>();
			if (!Authority || !ProfileSession)
			{
				return false;
			}
			const Fdemo_mapProfileSessionInitializeResult Initialized =
				ProfileSession->InitializeSession(Storage);
			Fdemo_map0909BSectWarehouseService Warehouse;
			Fdemo_map0909BWarehousePresentation Presentation;
			FString Diagnostic;
			if (!Initialized.IsReady()
				|| !Warehouse.OpenForSect(
					Root, Initialized.Snapshot,
					Edemo_map0909BTopState::AtSect,
					Presentation, Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P8.2 migration source failed: %s"),
					*Diagnostic));
				return false;
			}
			const Fdemo_mapShanmenItemCutoverResult Cutover =
				Fdemo_mapShanmenItemCutoverCoordinator::Execute(
					Storage, SeedProfile.ProfileId, *Authority,
					*ProfileSession, Warehouse);
			if (!Cutover.IsReady()
				|| !ProfileSession->SetPreparationMaterial(
					WoodAId, true).IsAccepted()
				|| !ProfileSession->SetPreparationMaterial(
					WoodBId, true).IsAccepted())
			{
				Test.AddError(FString::Printf(
					TEXT("P8.2 preparation failed: %s"),
					*Cutover.Diagnostic));
				return false;
			}
			const Fdemo_mapShanmenPreparedLoadoutResult Started =
				Fdemo_mapShanmenPreparationAdapter::
					StartPreparedLoadout(*Authority);
			if (!Started.IsCommitted()
				|| !Fdemo_mapShanmenRunLifecycleAdapter::
					TryGetActiveRunCorrelation(
						*Authority, Correlation, &Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P8.2 active Run failed: %s"),
					*Diagnostic));
				return false;
			}

			FShanmenItemAuthoritySnapshot Snapshot;
			if (!Authority->TryCaptureSnapshot(Snapshot))
			{
				return false;
			}
			FShanmenCombatActionCapture ActionCapture;
			ActionCapture.RunId = Correlation.ActiveRunId;
			ActionCapture.OwnerId = Correlation.OwnerId;
			ActionCapture.SourceEntityId = SourceEntityId;
			ActionCapture.ActionDefinitionId =
				FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
			ActionCapture.Content = Snapshot.Content;
			ActionCapture.ActivationId =
				FShanmenCombatIdFactory::MakeActivationId(
					ActionCapture.RunId,
					ActionCapture.SourceEntityId,
					ActionCapture.ActionDefinitionId,
					82);
			FShanmenCombatActionSnapshot Action;
			FShanmenActionTransitionReceipt Startup;
			FShanmenActionTransitionReceipt Active;
			FShanmenFormationDeploymentReceipt Begin;
			if (!FShanmenCombatActionSnapshot::TryCapture(
					ActionCapture, Action)
				|| !Fdemo_mapShanmenFormationProductSession::TryStart(
					Correlation, Action, MakeDiagram(), FVector::ZeroVector,
					FVector::ForwardVector, Session,
					Startup, Active, Begin))
			{
				Test.AddError(TEXT("P8.2 product session failed to start."));
				return false;
			}
			return true;
		}

		void Stop()
		{
			if (GameInstance)
			{
				GameInstance->Shutdown();
				Authority = nullptr;
				ProfileSession = nullptr;
				GameInstance->RemoveFromRoot();
				GameInstance->MarkAsGarbage();
				GameInstance = nullptr;
				CollectGarbage(RF_NoFlags);
			}
			if (!Root.IsEmpty())
			{
				IFileManager::Get().DeleteDirectory(*Root, false, true);
				Root.Reset();
			}
		}

		~FFormationSessionFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationSessionPrepareTest,
	"Shanmen.0_0_10.Product.FormationSession.PrepareAndSinglePendingFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationSessionPrepareTest::RunTest(const FString&)
{
	FFormationSessionFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Prepare")))
	{
		return false;
	}
	Fdemo_mapShanmenRunCorrelation Foreign = Fixture.Correlation;
	Foreign.CorrelationId = FGuid(0xF8200100, 0, 0, 1);
	const Fdemo_mapShanmenFormationSessionResult Rejected =
		Fixture.Session.TryPrepareAnchor(
			*Fixture.Authority, Foreign, AnchorA, AttemptA);
	const Fdemo_mapShanmenFormationSessionResult Prepared =
		Fixture.Session.TryPrepareAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorA, AttemptA);
	const Fdemo_mapShanmenFormationSessionResult Replay =
		Fixture.Session.TryPrepareAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorA, AttemptA);
	const Fdemo_mapShanmenFormationSessionResult Conflict =
		Fixture.Session.TryPrepareAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorB, AttemptB);
	TestTrue(TEXT("Foreign Run identity fails before durable preparation"),
		Rejected.Status
			== Edemo_mapShanmenFormationSessionStatus::CorrelationMismatch
			&& !Fixture.Session.GetAnchorAudits().Num());
	TestTrue(TEXT("One exact material attempt becomes the sole pending value"),
		Prepared.Status == Edemo_mapShanmenFormationSessionStatus::Prepared
			&& Prepared.Material.IsPrepared()
			&& Fixture.Session.HasPendingMaterial());
	TestTrue(TEXT("Exact prepare replay preserves deterministic transaction identity"),
		Replay.Status == Edemo_mapShanmenFormationSessionStatus::Replayed
			&& Replay.Material.TransactionId
				== Prepared.Material.TransactionId);
	TestTrue(TEXT("A second anchor cannot overlap the pending inventory intent"),
		Conflict.Status
			== Edemo_mapShanmenFormationSessionStatus::PendingConflict
			&& Fixture.Session.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationSessionCommitTest,
	"Shanmen.0_0_10.Product.FormationSession.CommitReplayAndEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationSessionCommitTest::RunTest(const FString&)
{
	FFormationSessionFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Commit")))
	{
		return false;
	}
	check(Fixture.Session.TryPrepareAnchor(
		*Fixture.Authority, Fixture.Correlation,
		AnchorA, AttemptA).IsSuccess());
	const Fdemo_mapShanmenFormationSessionResult First =
		Fixture.Session.TryCommitPreparedAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorA, AttemptA);
	const Fdemo_mapShanmenFormationSessionResult Replay =
		Fixture.Session.TryCommitPreparedAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorA, AttemptA);
	const Fdemo_mapShanmenFormationSessionResult Conflict =
		Fixture.Session.TryCommitPreparedAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorA, AttemptOther);
	TestTrue(TEXT("First anchor couples durable material and deployment receipts"),
		First.Status == Edemo_mapShanmenFormationSessionStatus::Committed
			&& First.Material.IsCommitted()
			&& First.DeploymentReceipt.IsValid()
			&& Fixture.Session.GetAnchorAudits().Num() == 1
			&& Fixture.Session.GetState()
				== Edemo_mapShanmenFormationSessionState::Deploying);
	TestTrue(TEXT("Commit replay returns the exact audit without another consume"),
		Replay.Status == Edemo_mapShanmenFormationSessionStatus::Replayed
			&& Replay.DeploymentReceipt.GetReceiptId()
				== First.DeploymentReceipt.GetReceiptId()
			&& Conflict.Status
				== Edemo_mapShanmenFormationSessionStatus::AttemptConflict);

	check(Fixture.Session.TryPrepareAnchor(
		*Fixture.Authority, Fixture.Correlation,
		AnchorB, AttemptB).IsSuccess());
	const Fdemo_mapShanmenFormationSessionResult Second =
		Fixture.Session.TryCommitPreparedAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorB, AttemptB);
	const Fdemo_mapShanmenFormationSessionResult Ended =
		Fixture.Session.TryEnd(Fixture.Correlation);
	const Fdemo_mapShanmenFormationSessionResult HistoricalPrepare =
		Fixture.Session.TryPrepareAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorB, AttemptB);
	TestTrue(TEXT("Final anchor activates the formation and exact material total"),
		Second.Status == Edemo_mapShanmenFormationSessionStatus::Committed
			&& Second.Material.FinalizeReceipts.Last().ResourceAfter == 1
			&& Fixture.Session.GetAnchorAudits().Num() == 2
			&& Ended.Status
				== Edemo_mapShanmenFormationSessionStatus::Ended
			&& HistoricalPrepare.Status
				== Edemo_mapShanmenFormationSessionStatus::Replayed);
	TestTrue(TEXT("End closes deployment and completes the action lifecycle"),
		Fixture.Session.IsTerminal()
			&& Fixture.Session.GetState()
				== Edemo_mapShanmenFormationSessionState::Ended
			&& Fixture.Session.GetDeployment().GetState()
				== EShanmenFormationDeploymentState::Ended
			&& Fixture.Session.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationSessionCancelTest,
	"Shanmen.0_0_10.Product.FormationSession.CancelReleasesPending",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationSessionCancelTest::RunTest(const FString&)
{
	FFormationSessionFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Cancel")))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationSessionResult Prepared =
		Fixture.Session.TryPrepareAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorA, AttemptA);
	const Fdemo_mapShanmenFormationSessionResult Cancelled =
		Fixture.Session.TryCancel(*Fixture.Authority, Fixture.Correlation);
	TestTrue(TEXT("Cancellation terminalizes every prepared line without consume"),
		Prepared.Material.IsPrepared()
			&& Cancelled.Status
				== Edemo_mapShanmenFormationSessionStatus::Cancelled
			&& Cancelled.Material.IsCancelled()
			&& Cancelled.Material.FinalizeReceipts[0].ResourceBefore
				== Cancelled.Material.FinalizeReceipts[0].ResourceAfter);
	TestTrue(TEXT("Material release precedes deployment and action cancellation"),
		Fixture.Session.IsTerminal()
			&& !Fixture.Session.HasPendingMaterial()
			&& Fixture.Session.GetAnchorAudits().IsEmpty()
			&& Fixture.Session.GetDeployment().GetState()
				== EShanmenFormationDeploymentState::Cancelled
			&& Fixture.Session.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationSessionRecoveryTest,
	"Shanmen.0_0_10.Product.FormationSession.ForwardCommitRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationSessionRecoveryTest::RunTest(const FString&)
{
	FFormationSessionFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Recovery")))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationSessionResult Prepared =
		Fixture.Session.TryPrepareAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorA, AttemptA);
	const Fdemo_mapShanmenFormationMaterialResult ExternalCommit =
		Fdemo_mapShanmenFormationMaterialAdapter::CommitMaterials(
			*Fixture.Authority,
			Fixture.Correlation,
			Fixture.Session.GetDeployment(),
			*Fixture.Session.GetPendingMaterial());
	const Fdemo_mapShanmenFormationSessionResult CancelRejected =
		Fixture.Session.TryCancel(*Fixture.Authority, Fixture.Correlation);
	const Fdemo_mapShanmenFormationSessionResult Recovered =
		Fixture.Session.TryCommitPreparedAnchor(
			*Fixture.Authority, Fixture.Correlation, AnchorA, AttemptA);
	TestTrue(TEXT("Durable commit can exist before the transient deployment edge"),
		Prepared.Material.IsPrepared() && ExternalCommit.IsCommitted()
			&& Fixture.Session.IsValid());
	TestTrue(TEXT("Cancellation fails forward once durable material is observed"),
		CancelRejected.Status
			== Edemo_mapShanmenFormationSessionStatus::MaterialCommitRecoveryRequired
			&& Fixture.Session.GetState()
				== Edemo_mapShanmenFormationSessionState::Deploying);
	TestTrue(TEXT("Exact commit replay reconstructs receipts and finishes the anchor"),
		Recovered.Status == Edemo_mapShanmenFormationSessionStatus::Committed
			&& Recovered.Material.Evidence.FulfillmentId
				== ExternalCommit.Evidence.FulfillmentId
			&& Fixture.Session.GetAnchorAudits().Num() == 1
			&& !Fixture.Session.HasPendingMaterial()
			&& Fixture.Session.IsValid());
	return true;
}

#endif
