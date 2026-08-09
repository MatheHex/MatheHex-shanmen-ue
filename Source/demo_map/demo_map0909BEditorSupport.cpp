#include "demo_map0909BEditorSupport.h"

#include "demo_map.h"
#include "demo_map0909BM01RuntimeAdapter.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapGameMode.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"

bool Fdemo_map0909BEditorSupport::ValidateDefaultEntry(
	const Ademo_mapGameMode& GameMode,
	FString& OutDiagnostic)
{
	const FString ExpectedMap = Fdemo_map0909BM01RuntimeAdapter::ExpectedM01MapDescriptor();
	FString GameDefault;
	FString EditorDefault;
	FString GameModePath;
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("GameDefaultMap"), GameDefault, GEngineIni);
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("EditorStartupMap"), EditorDefault, GEngineIni);
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("GlobalDefaultGameMode"), GameModePath, GEngineIni);
	const FString GamePackage = FPackageName::ObjectPathToPackageName(GameDefault);
	const bool bGameMap = GameDefault.StartsWith(ExpectedMap);
	const bool bEditorMap = EditorDefault.StartsWith(ExpectedMap);
	const bool bPackageExists = !GamePackage.IsEmpty()
		&& FPackageName::DoesPackageExist(GamePackage);
	const bool bRuntimeMap = GameMode.IsM01ExpeditionMap();
	const bool bRuntimeReady = GameMode.Is0909BRuntimeReady();
	const bool bNoRetiredSectWidget = !GameMode.HasRetired0909BDefaultWidget();
	const bool bMode = GameModePath.Contains(TEXT("demo_mapGameMode"));
	FString AdapterDiagnostic;
	const bool bAdapterDescriptor =
		Fdemo_map0909BM01RuntimeAdapter::ValidateM01DescriptorConfiguration(AdapterDiagnostic);
	const bool bValid = bGameMap && bEditorMap && bPackageExists
		&& bRuntimeMap && bRuntimeReady && bNoRetiredSectWidget && bMode
		&& bAdapterDescriptor;
	OutDiagnostic = FString::Printf(
		TEXT("GameDefault=%s EditorStartup=%s Package=%d RuntimeM01=%d RuntimeReady=%d RetiredCTA=%d GameMode=%s Adapter={%s}"),
		*GameDefault, *EditorDefault, bPackageExists ? 1 : 0,
		bRuntimeMap ? 1 : 0, bRuntimeReady ? 1 : 0,
		bNoRetiredSectWidget ? 0 : 1, *GameModePath, *AdapterDiagnostic);
	if (bValid)
	{
		UE_LOG(Logdemo_map, Log,
		TEXT("0_0_9BFIX_EDITOR_SUPPORT Event=DefaultEntryValidation Valid=1 %s"),
			*OutDiagnostic);
	}
	else
	{
		UE_LOG(Logdemo_map, Error,
		TEXT("0_0_9BFIX_EDITOR_SUPPORT Event=DefaultEntryValidation Valid=0 %s"),
			*OutDiagnostic);
	}
	return bValid;
}

bool Fdemo_map0909BEditorSupport::ValidateWarehouseProjection(
	const Fdemo_map0909BWarehousePresentation& Presentation,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!Presentation.bOpen || !Presentation.OwnerId.IsValid()
		|| !Presentation.LoadoutSelection.IsUsable()
		|| Presentation.OwnerId != Presentation.LoadoutSelection.OwnerId
		|| Presentation.PersistentRevision != Presentation.LoadoutSelection.PersistentRevision
		|| Presentation.Projection.Revision != Presentation.LoadoutSelection.GraphRevision)
	{
		OutDiagnostic = TEXT("P2 warehouse projection lacks one coherent Owner/revision/digest-bound P5 graph.");
		return false;
	}
	for (const FCodeBLoadoutSelectionRoot& Root : Presentation.LoadoutSelection.Roots)
	{
		const demo_map_code_b::FCodeBP2ContainerView* Container =
			Presentation.Projection.Containers.FindByPredicate([&Root](const demo_map_code_b::FCodeBP2ContainerView& Value)
			{
				return Value.ContainerId == Root.ContainerId;
			});
		if (!Container || !Container->Slots.IsValidIndex(Root.SlotIndex)
			|| Container->Slots[Root.SlotIndex].ItemId != Root.RootItemId
			|| !Root.GraphClosureItemIds.Contains(Root.RootItemId))
		{
			OutDiagnostic = TEXT("P2 warehouse projection diverges from its read-only Code B LoadoutSelection.");
			return false;
		}
	}
	OutDiagnostic = TEXT("P2 warehouse projection is one Owner-matched P5 graph with a read-only LoadoutSelection.");
	return true;
}

bool Fdemo_map0909BEditorSupport::ValidateStartAttemptAudit(
	const Fdemo_map0909BStartDiagnostic& Diagnostic,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!Diagnostic.StartAttemptId.IsValid() || Diagnostic.AttemptSequence <= 0
		|| Diagnostic.RequestedAtUtc.IsEmpty()
		|| Diagnostic.M01MapDescriptor
			!= Fdemo_map0909BM01RuntimeAdapter::ExpectedM01MapDescriptor()
		|| Diagnostic.LoadoutPersistentRevision == INDEX_NONE
		|| Diagnostic.LoadoutGraphRevision == INDEX_NONE
		|| Diagnostic.LoadoutSelectionDigest.IsEmpty())
	{
		OutDiagnostic = TEXT("P3 StartAttempt audit is missing immutable attempt, map or LoadoutSelection correlation evidence.");
		return false;
	}
	if (Diagnostic.AfterState == Edemo_map0909BTopState::InRun
		&& (!Diagnostic.OwnerId.IsValid() || !Diagnostic.RunId.IsValid()
			|| Diagnostic.RuntimeReceiptClass != TEXT("RuntimeReady")
			|| Diagnostic.M01MapIdentity.IsEmpty()))
	{
		OutDiagnostic = TEXT("P3 InRun audit lacks one correlated RuntimeReady identity fact.");
		return false;
	}
	if (Diagnostic.AfterState == Edemo_map0909BTopState::AtSect
		&& Diagnostic.FailureClass != TEXT("None")
		&& Diagnostic.RunId.IsValid())
	{
		OutDiagnostic = TEXT("P3 technical failure left an active Run identity after returning to AtSect.");
		return false;
	}
	OutDiagnostic = TEXT("P3 StartAttempt audit is read-only and correlation-complete for the current coordinator state.");
	return true;
}
