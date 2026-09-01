#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfilePreparationPresenter.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileTradeTransaction.h"
#include "demo_mapRewardJackpot.h"
#include "demo_mapRewardProjectionTestSupport.h"
#include "demo_mapRewardSourceProjection.h"
#include "demo_mapSearchContainerPresenter.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	Fdemo_mapRewardJackpotTests,
	"demo_map.RewardJackpot",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

void Fdemo_mapRewardJackpotTests::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	static const TCHAR* Names[] = {
		TEXT("01.DefaultPolicyIdentity"),
		TEXT("02.DefaultChanceIsFivePercent"),
		TEXT("03.DefaultMultiplierIsSixX"),
		TEXT("04.RegistryValidates"),
		TEXT("05.AllEightProjectionsReferencePolicy"),
		TEXT("06.RollRangeIsBasisPoints"),
		TEXT("07.Roll499Hits"),
		TEXT("08.Roll500Misses"),
		TEXT("09.BoundedNaturalHitAndMiss"),
		TEXT("10.BoundedDualSourceHitAndMiss"),
		TEXT("11.SameIdentityIsDeterministic"),
		TEXT("12.RollAndSelectionDomainsAreSeparated"),
		TEXT("13.MissDoesNotAnnotate"),
		TEXT("14.HitAnnotatesExactlyOneEntry"),
		TEXT("15.SelectedEntryIndexIsEligible"),
		TEXT("16.EventIdIsDeterministic"),
		TEXT("17.SourceIdentityChangesEventId"),
		TEXT("18.BasePlanValueDoesNotExceedBudget"),
		TEXT("19.HitKeepsBaseGeneratedValue"),
		TEXT("20.HitBonusIsFiveTimesSelectedBase"),
		TEXT("21.HitFinalIsBasePlusBonus"),
		TEXT("22.ProjectionTraceCarriesDecision"),
		TEXT("23.ContainerSeedCarriesMetadata"),
		TEXT("24.NormalSeedDefaultsRemainNormal"),
		TEXT("25.FixedFallbackIsNotAnEligibleEntry"),
		TEXT("26.AllEightPlansRemainSuccessful"),
		TEXT("27.MaterializationPreservesMetadata"),
		TEXT("28.WholeTakePreservesGuid"),
		TEXT("29.NormalAndJackpotCannotStack"),
		TEXT("30.DifferentJackpotEventsCannotStack"),
		TEXT("31.SameJackpotEventCanStack"),
		TEXT("32.DifferentDefinitionsCannotStack"),
		TEXT("33.NormalSellValueIsUnchanged"),
		TEXT("34.JackpotSellValueIsSixX"),
		TEXT("35.StackSellValueUsesQuantity"),
		TEXT("36.UnsupportedMultiplierRejected"),
		TEXT("37.PersistentEqualityIncludesMetadata"),
		TEXT("38.ProfileValidationAcceptsJackpot"),
		TEXT("39.ProfileSaveReloadPreservesMetadata"),
		TEXT("40.OldMissingFieldsDefaultNormal"),
		TEXT("41.PartialMetadataIsRejected"),
		TEXT("42.EventIdConflictIsRejected"),
		TEXT("43.SameEventFragmentsAreAccepted"),
		TEXT("44.SplitConservesEffectiveValue"),
		TEXT("45.MergeConservesEffectiveValue"),
		TEXT("46.PreparationQuoteShowsJackpot"),
		TEXT("47.PreparationQuoteUsesEffectiveValue"),
		TEXT("48.NormalPreparationQuoteIsUnchanged"),
		TEXT("49.TradeCreditsExactlySixX"),
		TEXT("50.TradeFailureRollsBack"),
		TEXT("51.SearchContainerShowsJackpotAndValue"),
		TEXT("52.BuyCatalogIsUnchanged")
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		OutBeautifiedNames.Add(Names[Index]);
		OutTestCommands.Add(FString::FromInt(Index));
	}
}

namespace
{
	const Fdemo_mapRewardSourceProjection& HitProjection()
	{
		static const Fdemo_mapRewardSourceProjection Projection =
			demo_mapRewardProjectionTestSupport::FindBound(
				Fdemo_mapRewardProjectionIds::ChestMainWood);
		return Projection;
	}

	const Fdemo_mapRewardSourceProjection& MissProjection()
	{
		static const Fdemo_mapRewardSourceProjection Projection =
			demo_mapRewardProjectionTestSupport::FindBound(
				Fdemo_mapRewardProjectionIds::ChestMainOre);
		return Projection;
	}

	bool NaturalIds(FGuid& OutHit, FGuid& OutMiss)
	{
		return Fdemo_mapRewardJackpot::FindNaturalHitAndMiss(
			Fdemo_mapRewardJackpotPolicyRegistry::GetDefault(),
			HitProjection().StableSourceRoleId,
			HitProjection().ProjectionId,
			4096,
			OutHit,
			OutMiss);
	}

	Fdemo_mapRewardSourceProjectionResult NaturalHitPlan()
	{
		FGuid Hit;
		FGuid Miss;
		NaturalIds(Hit, Miss);
		return Fdemo_mapRewardSourceProjectionPlanner::Plan(
			HitProjection(),
			Hit);
	}

	FString NewRoot(int32 Case)
	{
		FString TaskId;
		FParse::Value(
			FCommandLine::Get(),
			TEXT("RewardAutomationTaskId="),
			TaskId);
		if (TaskId.IsEmpty()
			|| TaskId.Contains(TEXT("/"))
			|| TaskId.Contains(TEXT("\\"))
			|| TaskId.Contains(TEXT(" "))
			|| TaskId.Contains(TEXT("..")))
		{
			TaskId = TEXT("Dev.D.UE.0.0.6.P6.0.r3");
		}
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TaskId,
			TEXT("Tests"),
			TEXT("RewardJackpot"),
			FString::Printf(TEXT("%02d-%s"), Case + 1,
				*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	}

	Fdemo_mapPersistentItemRecord JackpotRecord(
		FName DefinitionId,
		int32 Quantity,
		const FGuid& EventId = FGuid::NewGuid())
	{
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = FGuid::NewGuid();
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = Quantity;
		Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		Item.RewardEventKind = Edemo_mapRewardEventKind::Jackpot;
		Item.RewardEventId = EventId;
		Item.RewardValueMultiplierBps =
			Fdemo_mapRewardEventRules::JackpotMultiplierBps;
		Item.RewardSourceRoleId = HitProjection().StableSourceRoleId;
		return Item;
	}

	bool SaveProfileWithJackpot(
		const FString& Root,
		Fdemo_mapPersistentItemRecord& OutItem)
	{
		Fdemo_mapProfileRepository Repository;
		Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
		OutItem = JackpotRecord(Fdemo_mapItemIds::SpiritDust, 2);
		Profile.PermanentStash.Add(OutItem);
		return Repository.SaveProfile(
			Profile,
			Fdemo_mapProfileStorageContext::ForRoot(Root)).IsSuccess();
	}

	bool RewriteRewardFields(
		const FString& Path,
		bool bRemoveAll)
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *Path))
		{
			return false;
		}
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader =
			TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Root)
			|| !Root.IsValid())
		{
			return false;
		}
		auto RewriteArray = [bRemoveAll](
			const TArray<TSharedPtr<FJsonValue>>* Values)
		{
			if (!Values)
			{
				return;
			}
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				const TSharedPtr<FJsonObject> Object =
					Value.IsValid() ? Value->AsObject() : nullptr;
				if (!Object)
				{
					continue;
				}
				Object->RemoveField(TEXT("RewardEventKind"));
				if (bRemoveAll)
				{
					Object->RemoveField(TEXT("RewardEventId"));
					Object->RemoveField(
						TEXT("RewardValueMultiplierBps"));
					Object->RemoveField(TEXT("RewardSourceRoleId"));
				}
			}
		};
		const TArray<TSharedPtr<FJsonValue>>* Stash = nullptr;
		Root->TryGetArrayField(TEXT("PermanentStash"), Stash);
		RewriteArray(Stash);
		const TSharedPtr<FJsonObject>* ActiveRun = nullptr;
		if (Root->TryGetObjectField(TEXT("ActiveRun"), ActiveRun)
			&& ActiveRun && ActiveRun->IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* RunItems = nullptr;
			(*ActiveRun)->TryGetArrayField(
				TEXT("ActiveRunItems"),
				RunItems);
			RewriteArray(RunItems);
		}
		FString Rewritten;
		const TSharedRef<
			TJsonWriter<
				TCHAR,
				TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<
				TCHAR,
				TCondensedJsonPrintPolicy<TCHAR>>::Create(&Rewritten);
		return FJsonSerializer::Serialize(Root.ToSharedRef(), Writer)
			&& FFileHelper::SaveStringToFile(
				Rewritten,
				*Path,
				FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	Fdemo_mapProfilePreparationViewState PreparationView(bool bJackpot)
	{
		Fdemo_mapProfilePreparationSnapshot Snapshot;
		Snapshot.SessionState =
			Edemo_mapProfileSessionState::ReadyForPreparation;
		Snapshot.ProfileId = FGuid::NewGuid();
		Fdemo_mapProfilePreparationStashRow Row;
		Row.ItemInstanceId = FGuid::NewGuid();
		Row.ItemDefinitionId = Fdemo_mapItemIds::SpiritDust;
		Row.StackCount = 2;
		Row.bSafeInPermanentStash = true;
		if (bJackpot)
		{
			const Fdemo_mapPersistentItemRecord Source =
				JackpotRecord(Row.ItemDefinitionId, Row.StackCount);
			Row.RewardEventKind = Source.RewardEventKind;
			Row.RewardEventId = Source.RewardEventId;
			Row.RewardValueMultiplierBps =
				Source.RewardValueMultiplierBps;
			Row.RewardSourceRoleId = Source.RewardSourceRoleId;
		}
		Snapshot.OrderedPermanentStashRows.Add(Row);
		return Fdemo_mapProfilePreparationPresenter::BuildViewState(
			Snapshot);
	}
}

bool Fdemo_mapRewardJackpotTests::RunTest(const FString& Parameters)
{
	const int32 Case = FCString::Atoi(*Parameters);
	const Fdemo_mapRewardJackpotPolicy& Policy =
		Fdemo_mapRewardJackpotPolicyRegistry::GetDefault();
	FGuid HitRunId;
	FGuid MissRunId;
	const bool bFoundNatural = NaturalIds(HitRunId, MissRunId);
	const Fdemo_mapRewardSourceProjectionResult HitPlan =
		NaturalHitPlan();
	const Fdemo_mapRewardSourceProjectionResult MissPlan =
		Fdemo_mapRewardSourceProjectionPlanner::Plan(
			HitProjection(),
			MissRunId);
	switch (Case)
	{
	case 0:
		TestEqual(TEXT("Default policy id"), Policy.PolicyId,
			FName(TEXT("Reward.Jackpot.Default")));
		break;
	case 1:
		TestEqual(TEXT("Chance basis points"), Policy.ChanceBps, 500);
		break;
	case 2:
		TestEqual(TEXT("Six-X basis points"),
			Policy.JackpotMultiplierBps, 60000);
		break;
	case 3:
	{
		FString Error;
		TestTrue(TEXT("Registry is valid"),
			Fdemo_mapRewardJackpotPolicyRegistry::Validate(&Error));
		break;
	}
	case 4:
		TestTrue(TEXT("Every projection names the central policy"),
			!Fdemo_mapRewardSourceProjectionRegistry::GetAll().
				ContainsByPredicate([&Policy](const auto& Projection)
				{
					return Projection.JackpotPolicyId
						!= Policy.PolicyId;
				}));
		break;
	case 5:
		TestTrue(TEXT("Natural rolls remain in [0,9999]"),
			HitPlan.Trace.JackpotRoll >= 0
				&& HitPlan.Trace.JackpotRoll <= 9999
				&& MissPlan.Trace.JackpotRoll >= 0
				&& MissPlan.Trace.JackpotRoll <= 9999);
		break;
	case 6:
		TestTrue(TEXT("499 is a hit"),
			Fdemo_mapRewardJackpot::IsHitRoll(499, 500));
		break;
	case 7:
		TestFalse(TEXT("500 is a miss"),
			Fdemo_mapRewardJackpot::IsHitRoll(500, 500));
		break;
	case 8:
		TestTrue(TEXT("Bounded search finds both outcomes"),
			bFoundNatural && HitRunId.IsValid() && MissRunId.IsValid());
		break;
	case 9:
	{
		TArray<TPair<FName, FName>> AllSources;
		for (const auto& Projection :
			Fdemo_mapRewardSourceProjectionRegistry::GetAll())
		{
			AllSources.Emplace(
				Projection.StableSourceRoleId,
				Projection.ProjectionId);
		}
		FGuid SingleHitRunId;
		TestTrue(TEXT("One natural identity has exactly one hit"),
			Fdemo_mapRewardJackpot::FindNaturalRunForSingleHitAcrossSources(
				Policy,
				HitProjection().StableSourceRoleId,
				HitProjection().ProjectionId,
				AllSources,
				4096,
				SingleHitRunId));
		break;
	}
	case 10:
		TestTrue(TEXT("Same identity repeats exactly"),
			Fdemo_mapRewardSourceProjectionPlanner::Plan(
				HitProjection(), HitRunId).PlannedStacks
			== HitPlan.PlannedStacks);
		break;
	case 11:
		TestTrue(TEXT("Roll and selection use separated domains"),
			HitPlan.Trace.JackpotRollSeed
				!= HitPlan.Trace.JackpotSelectionSeed);
		break;
	case 12:
		TestTrue(TEXT("Miss has no annotated entries"),
			MissPlan.IsSuccess()
				&& !MissPlan.Trace.bJackpotHit
				&& !MissPlan.PlannedStacks.ContainsByPredicate(
					[](const auto& Stack)
					{
						return Stack.RewardEventKind
							!= Edemo_mapRewardEventKind::None;
					}));
		break;
	case 13:
	{
		int32 Count = 0;
		for (const auto& Stack : HitPlan.PlannedStacks)
		{
			Count += Stack.RewardEventKind
				== Edemo_mapRewardEventKind::Jackpot ? 1 : 0;
		}
		TestEqual(TEXT("Exactly one planned entry is annotated"), Count, 1);
		break;
	}
	case 14:
		TestTrue(TEXT("Selected entry index resolves"),
			HitPlan.PlannedStacks.IsValidIndex(
				HitPlan.Trace.JackpotSelectedPlannedEntryIndex)
				&& HitPlan.PlannedStacks[
					HitPlan.Trace.JackpotSelectedPlannedEntryIndex].
						TotalValue > 0);
		break;
	case 15:
		TestEqual(TEXT("Event identity repeats"),
			Fdemo_mapRewardSourceProjectionPlanner::Plan(
				HitProjection(), HitRunId).Trace.JackpotEventId,
			HitPlan.Trace.JackpotEventId);
		break;
	case 16:
	{
		TArray<Fdemo_mapRewardPlannedStack> Copy =
			HitPlan.PlannedStacks;
		for (auto& Stack : Copy)
		{
			Stack.RewardEventKind = Edemo_mapRewardEventKind::None;
			Stack.RewardEventId.Invalidate();
			Stack.RewardValueMultiplierBps = 10000;
			Stack.RewardSourceRoleId = NAME_None;
		}
		const auto Other = Fdemo_mapRewardJackpot::Annotate(
			Policy,
			HitRunId,
			FName(TEXT("Other.Source.Role")),
			HitProjection().ProjectionId,
			Copy);
		TestNotEqual(TEXT("Source identity separates event ids"),
			Other.EventId, HitPlan.Trace.JackpotEventId);
		break;
	}
	case 17:
		TestTrue(TEXT("Base value obeys randomized budget"),
			HitPlan.Trace.GeneratedTotalValue
				<= HitPlan.Trace.RandomizedBudget);
		break;
	case 18:
		TestEqual(TEXT("Jackpot decision sees unchanged base value"),
			HitPlan.Trace.GeneratedTotalValue,
			HitPlan.Trace.FinalEffectiveRewardValue
				- HitPlan.Trace.JackpotBonusValue);
		break;
	case 19:
		TestEqual(TEXT("Bonus is selected base times five"),
			HitPlan.Trace.JackpotBonusValue,
			HitPlan.Trace.JackpotSelectedBaseValue * 5);
		break;
	case 20:
		TestEqual(TEXT("Final is base plus bonus"),
			HitPlan.Trace.FinalEffectiveRewardValue,
			HitPlan.Trace.GeneratedTotalValue
				+ HitPlan.Trace.JackpotBonusValue);
		break;
	case 21:
		TestTrue(TEXT("Trace has the complete central decision"),
			HitPlan.Trace.JackpotPolicyId == Policy.PolicyId
				&& HitPlan.Trace.JackpotRollSeed != 0
				&& HitPlan.Trace.JackpotSelectionSeed != 0
				&& HitPlan.Trace.JackpotEventId.IsValid());
		break;
	case 22:
	{
		const auto Seed =
			Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(
				HitPlan);
		const auto* Entry = Seed.FindByPredicate([](const auto& Item)
			{
				return Item.RewardEventKind
					== Edemo_mapRewardEventKind::Jackpot;
			});
		TestTrue(TEXT("Seed preserves all event metadata"),
			Entry
				&& Entry->RewardEventId
					== HitPlan.Trace.JackpotEventId
				&& Entry->RewardValueMultiplierBps == 60000
				&& !Entry->RewardSourceRoleId.IsNone());
		break;
	}
	case 23:
	{
		const auto Seed =
			Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(
				MissPlan);
		TestTrue(TEXT("Miss seed is normal by default"),
			!Seed.ContainsByPredicate([](const auto& Entry)
				{
					return Entry.RewardEventKind
							!= Edemo_mapRewardEventKind::None
						|| Entry.RewardEventId.IsValid()
						|| Entry.RewardValueMultiplierBps != 10000
						|| !Entry.RewardSourceRoleId.IsNone();
				}));
		break;
	}
	case 24:
		TestTrue(TEXT("Decision selects only planned stacks"),
			HitPlan.Trace.bJackpotHit
				&& HitPlan.Trace.JackpotSelectedPlannedEntryIndex >= 0
				&& !HitProjection().FixedFallbackTableId.IsNone()
				&& !HitPlan.Trace.bFallbackUsed);
		break;
	case 25:
	{
		bool bAll = true;
		for (const auto& Projection :
			Fdemo_mapRewardSourceProjectionRegistry::GetAll())
		{
			const auto Result =
				Fdemo_mapRewardSourceProjectionPlanner::Plan(
					demo_mapRewardProjectionTestSupport::
						BindPrototype(Projection),
					HitRunId);
			bAll &= Result.IsSuccess()
				&& Result.Trace.JackpotPolicyId == Policy.PolicyId;
		}
		TestTrue(TEXT("All eight projections remain valid"), bAll);
		break;
	}
	case 26:
	{
		Fdemo_mapItemAuthority Authority;
		const FGuid ItemId = FGuid::NewGuid();
		const auto Result = Authority.MaterializeDeployedInstance(
			ItemId,
			Fdemo_mapItemIds::SpiritDust,
			2,
			NAME_None,
			HitRunId,
			Edemo_mapRewardEventKind::Jackpot,
			HitPlan.Trace.JackpotEventId,
			60000,
			HitProjection().StableSourceRoleId);
		const auto* Item = Authority.FindInstance(ItemId);
		TestTrue(TEXT("Materialization preserves metadata"),
			Result.bSuccess && Item
				&& Item->RewardEventId
					== HitPlan.Trace.JackpotEventId
				&& Item->RewardValueMultiplierBps == 60000);
		break;
	}
	case 27:
	{
		Fdemo_mapItemAuthority Authority;
		const FGuid ContainerId = FGuid::NewGuid();
		FGuid ItemId;
		const auto Create = Authority.CreateContainerDefinition(
			Fdemo_mapItemIds::SpiritDust,
			2,
			ContainerId,
			HitRunId,
			ItemId,
			Edemo_mapRewardEventKind::Jackpot,
			HitPlan.Trace.JackpotEventId,
			60000,
			HitProjection().StableSourceRoleId);
		const auto Take = Authority.TransferContainerToInventoryWhole(
			ItemId, ContainerId);
		TestTrue(TEXT("Whole take keeps the same GUID"),
				Create.bSuccess && Take.bSuccess
				&& Take.RelatedInstanceId == ItemId
				&& Authority.FindInstance(ItemId) != nullptr);
		break;
	}
	case 28:
	case 29:
	case 30:
	case 31:
	{
		const FGuid A = FGuid::NewGuid();
		const FGuid B = Case == 30 ? A : FGuid::NewGuid();
		const bool bCompatible =
			Fdemo_mapRewardEventRules::AreStackCompatible(
				Fdemo_mapItemIds::SpiritDust,
				Case == 28
					? Edemo_mapRewardEventKind::None
					: Edemo_mapRewardEventKind::Jackpot,
				Case == 28 ? FGuid() : A,
				Case == 28 ? 10000 : 60000,
				Case == 28 ? NAME_None
					: HitProjection().StableSourceRoleId,
				Case == 31
					? Fdemo_mapItemIds::IronShard
					: Fdemo_mapItemIds::SpiritDust,
				Edemo_mapRewardEventKind::Jackpot,
				B,
				60000,
				HitProjection().StableSourceRoleId);
		TestEqual(TEXT("Stack compatibility includes full metadata"),
			bCompatible, Case == 30);
		break;
	}
	case 32:
	case 33:
	case 34:
	case 35:
	{
		int64 Value = 0;
		const int32 Quantity = Case == 34 ? 3 : 1;
		const int32 Multiplier =
			Case == 33 || Case == 34 ? 60000
				: Case == 35 ? 12345 : 10000;
		const bool bSuccess = Fdemo_mapItemSellValueRules::TryCompute(
			Fdemo_mapItemIds::SpiritDust,
			Quantity,
			Multiplier,
			Value);
		const auto* Definition = Fdemo_mapItemDefinitions::Find(
			Fdemo_mapItemIds::SpiritDust);
		const int64 Expected = Definition
			? Definition->SellPrice
				* static_cast<int64>(Quantity)
				* static_cast<int64>(
					Case == 33 || Case == 34 ? 6 : 1)
			: 0;
		TestTrue(TEXT("Unique checked sell valuation"),
			Case == 35 ? !bSuccess
				: bSuccess && Value == Expected);
		break;
	}
	case 36:
	{
		const auto A = JackpotRecord(
			Fdemo_mapItemIds::SpiritDust, 2);
		auto B = A;
		B.RewardValueMultiplierBps = 10000;
		TestFalse(TEXT("Persistent equality sees metadata"), A == B);
		break;
	}
	case 37:
	{
		Fdemo_mapProfileRepository Repository;
		auto Profile = Repository.CreateFreshProfile();
		Profile.PermanentStash.Add(JackpotRecord(
			Fdemo_mapItemIds::SpiritDust, 2));
		FString Error;
		TestTrue(TEXT("Valid jackpot profile accepted"),
			Repository.ValidateProfile(Profile, &Error));
		break;
	}
	case 38:
	{
		const FString Root = NewRoot(Case);
		Fdemo_mapPersistentItemRecord Item;
		const bool bSaved = SaveProfileWithJackpot(Root, Item);
		const auto Load = Fdemo_mapProfileRepository().
			LoadExistingProfile(
				Fdemo_mapProfileStorageContext::ForRoot(Root));
		const auto* Reloaded =
			Load.Profile.PermanentStash.FindByPredicate(
				[&Item](const auto& Candidate)
				{
					return Candidate.ItemInstanceId
						== Item.ItemInstanceId;
				});
		TestTrue(TEXT("Save/reload preserves metadata and GUID"),
			bSaved && Load.IsSuccess() && Reloaded
				&& *Reloaded == Item);
		break;
	}
	case 39:
	case 40:
	{
		const FString Root = NewRoot(Case);
		Fdemo_mapProfileRepository Repository;
		auto Profile = Repository.CreateFreshProfile();
		const auto Storage =
			Fdemo_mapProfileStorageContext::ForRoot(Root);
		const bool bSaved =
			Repository.SaveProfile(Profile, Storage).IsSuccess();
		const bool bRewritten =
			bSaved && RewriteRewardFields(
				Storage.PrimaryPath(),
				Case == 39);
		const auto Load = Repository.LoadExistingProfile(Storage);
		const bool bNormalDefaults =
			Load.IsSuccess()
			&& !Load.Profile.PermanentStash.ContainsByPredicate(
				[](const auto& Item)
				{
					return Item.RewardEventKind
							!= Edemo_mapRewardEventKind::None
						|| Item.RewardEventId.IsValid()
						|| Item.RewardValueMultiplierBps != 10000
						|| !Item.RewardSourceRoleId.IsNone();
				});
		TestTrue(TEXT("Backward compatibility is all-or-none"),
			bSaved && bRewritten
				&& (Case == 39
					? bNormalDefaults
					: !Load.IsSuccess()));
		break;
	}
	case 41:
	case 42:
	{
		Fdemo_mapProfileRepository Repository;
		auto Profile = Repository.CreateFreshProfile();
		const FGuid EventId = FGuid::NewGuid();
		Profile.PermanentStash.Add(JackpotRecord(
			Fdemo_mapItemIds::SpiritDust, 1, EventId));
		Profile.PermanentStash.Add(JackpotRecord(
			Case == 41
				? Fdemo_mapItemIds::IronShard
				: Fdemo_mapItemIds::SpiritDust,
			1,
			EventId));
		FString Error;
		TestEqual(TEXT("Event identity compatibility is enforced"),
			Repository.ValidateProfile(Profile, &Error),
			Case == 42);
		break;
	}
	case 43:
	case 44:
	{
		int64 Whole = 0;
		int64 First = 0;
		int64 Second = 0;
		const bool bValues =
			Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::SpiritDust, 5, 60000, Whole)
			&& Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::SpiritDust, 2, 60000, First)
			&& Fdemo_mapItemSellValueRules::TryCompute(
				Fdemo_mapItemIds::SpiritDust, 3, 60000, Second);
		TestTrue(TEXT("Split and merge conserve effective value"),
			bValues && Whole == First + Second);
		break;
	}
	case 45:
	case 46:
	case 47:
	{
		const auto View = PreparationView(Case != 47);
		const auto* Row =
			View.OrderedPermanentStashRows.IsEmpty()
				? nullptr
				: &View.OrderedPermanentStashRows[0];
		const auto* Definition = Fdemo_mapItemDefinitions::Find(
			Fdemo_mapItemIds::SpiritDust);
		const int64 Expected = Definition
			? Definition->SellPrice * 2
				* static_cast<int64>(Case == 47 ? 1 : 6)
			: 0;
		TestTrue(TEXT("Preparation quote exposes label and value"),
			Row && Row->TotalSellPrice == Expected
				&& (Case == 45
					? Row->RewardLabel == TEXT("JACKPOT ×6")
					: Case == 46
						? Row->SellDiagnostic.Contains(
							TEXT("Effective Sell"))
						: Row->RewardLabel.IsEmpty()));
		break;
	}
	case 48:
	case 49:
	{
		const FString Root = NewRoot(Case);
		Fdemo_mapPersistentItemRecord Item;
		if (!SaveProfileWithJackpot(Root, Item))
		{
			AddError(TEXT("Fixture save failed."));
			break;
		}
		Fdemo_mapProfileRepository Repository;
		Fdemo_mapProfileStorageContext Storage =
			Fdemo_mapProfileStorageContext::ForRoot(Root);
		const Fdemo_mapProfileLoadResult Load =
			Repository.LoadExistingProfile(Storage);
		Fdemo_mapPersistentProfile Profile = Load.Profile;
		const Fdemo_mapPersistentProfile Before = Profile;
		if (Case == 49)
		{
			Storage.InjectedFailure =
				Edemo_mapProfileFailureStage::WriteTemp;
		}
		Fdemo_mapProfileTradeIntent Intent;
		Intent.Kind = Edemo_mapProfileTradeKind::Sell;
		Intent.ExpectedProfileId = Profile.ProfileId;
		Intent.ExpectedSaveGeneration = Profile.SaveGeneration;
		Intent.ItemInstanceId = Item.ItemInstanceId;
		const Fdemo_mapProfileTradeTransactionResult Transaction =
			Fdemo_mapProfileTradeTransaction().Execute(
				Profile,
				Intent,
				{},
				Repository,
				Storage);
		const Fdemo_mapProfileTradeResult& Result =
			Transaction.Result;
		const auto* Definition = Fdemo_mapItemDefinitions::Find(
			Item.ItemDefinitionId);
		const int64 Expected = Definition
			? Definition->SellPrice * Item.StackCount * 6
			: 0;
		TestTrue(TEXT("Transaction uses one value path and is atomic"),
			Load.IsSuccess()
				&& (Case == 48
					? Result.IsCommitted()
						&& Result.TotalPrice == Expected
						&& Profile.PersistentSpiritStones
							== Before.PersistentSpiritStones
								+ Expected
					: !Result.IsCommitted()
						&& Profile == Before));
		break;
	}
	case 50:
	{
		Fdemo_mapRuntimeContainerSnapshot Snapshot;
		Snapshot.Kind = Edemo_mapRuntimeContainerKind::Chest;
		Snapshot.State = Edemo_mapRuntimeContainerState::Opened;
		Fdemo_mapRuntimeContainerSectionSnapshot Section;
		Section.Section = Edemo_mapRuntimeContainerSection::Chest;
		Section.Capacity = 1;
		Fdemo_mapRuntimeContainerEntrySnapshot Entry;
		Entry.EntryId = FGuid::NewGuid();
		Entry.SlotIndex = 0;
		Entry.State =
			Edemo_mapRuntimeContainerEntryState::Identified;
		Entry.DisplayName = FText::FromString(TEXT("Spirit Dust"));
		Entry.CategoryId = Fdemo_mapItemIds::MaterialCategory;
		Entry.StackCount = 2;
		Entry.RewardEventKind =
			Edemo_mapRewardEventKind::Jackpot;
		Entry.EffectiveStackSellValue = 60;
		Section.OrderedOccupiedEntries.Add(Entry);
		Snapshot.Sections.Add(Section);
		const auto View =
			Fdemo_mapSearchContainerPresenter::Build(Snapshot);
		TestTrue(TEXT("Search row visibly shows jackpot and effective value"),
			!View.Sections.IsEmpty()
				&& !View.Sections[0].Rows.IsEmpty()
				&& View.Sections[0].Rows[0].Text.Contains(
					TEXT("JACKPOT ×6"))
				&& View.Sections[0].Rows[0].Text.Contains(
					TEXT("SELL=60")));
		break;
	}
	case 51:
	{
		const auto Catalog =
			Fdemo_mapProfileTradeTransaction::BuildCatalog();
		const auto* Pill = Catalog.FindByPredicate([](const auto& Row)
			{
				return Row.ItemDefinitionId
					== Fdemo_mapItemIds::HealingPillLevel1;
			});
		TestTrue(TEXT("Buy catalog and price are unchanged"),
			Pill && Pill->BuyPrice == 30);
		break;
	}
	default:
		AddError(TEXT("Unknown RewardJackpot case."));
		break;
	}
	return true;
}

#endif
