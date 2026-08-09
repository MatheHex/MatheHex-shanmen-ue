#include "demo_mapProfileRepository.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardShopStock.h"
#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	FString GuidString(const FGuid& Guid)
	{
		return Guid.IsValid() ? Guid.ToString(EGuidFormats::DigitsWithHyphensLower) : FString();
	}

	bool ParseGuid(const FString& Text, FGuid& OutGuid)
	{
		OutGuid.Invalidate();
		return Text.IsEmpty() || FGuid::Parse(Text, OutGuid);
	}

	const TCHAR* DomainString(Edemo_mapPersistentDomain Domain)
	{
		switch (Domain)
		{
		case Edemo_mapPersistentDomain::PermanentStash:
			return TEXT("PermanentStash");
		case Edemo_mapPersistentDomain::ActiveRun:
			return TEXT("ActiveRun");
		case Edemo_mapPersistentDomain::ShopStock:
			return TEXT("ShopStock");
		default:
			return TEXT("Invalid");
		}
	}

	bool ParseDomain(const FString& Text, Edemo_mapPersistentDomain& OutDomain)
	{
		if (Text == TEXT("PermanentStash")) { OutDomain = Edemo_mapPersistentDomain::PermanentStash; return true; }
		if (Text == TEXT("ActiveRun")) { OutDomain = Edemo_mapPersistentDomain::ActiveRun; return true; }
		if (Text == TEXT("ShopStock")) { OutDomain = Edemo_mapPersistentDomain::ShopStock; return true; }
		return false;
	}

	const TCHAR* ShopStockEntryStateString(
		Edemo_mapPersistentShopStockEntryState State)
	{
		return State == Edemo_mapPersistentShopStockEntryState::Sold
			? TEXT("Sold")
			: TEXT("Available");
	}

	bool ParseShopStockEntryState(
		const FString& Text,
		Edemo_mapPersistentShopStockEntryState& OutState)
	{
		if (Text == TEXT("Available"))
		{
			OutState = Edemo_mapPersistentShopStockEntryState::Available;
			return true;
		}
		if (Text == TEXT("Sold"))
		{
			OutState = Edemo_mapPersistentShopStockEntryState::Sold;
			return true;
		}
		return false;
	}

	const TCHAR* RewardEventKindString(Edemo_mapRewardEventKind Kind)
	{
		return Kind == Edemo_mapRewardEventKind::Jackpot
			? TEXT("Jackpot")
			: TEXT("None");
	}

	bool ParseRewardEventKind(
		const FString& Text,
		Edemo_mapRewardEventKind& OutKind)
	{
		if (Text == TEXT("None"))
		{
			OutKind = Edemo_mapRewardEventKind::None;
			return true;
		}
		if (Text == TEXT("Jackpot"))
		{
			OutKind = Edemo_mapRewardEventKind::Jackpot;
			return true;
		}
		return false;
	}

	const TCHAR* AffixAcquisitionString(
		Edemo_mapRewardAffixAcquisition Acquisition)
	{
		switch (Acquisition)
		{
		case Edemo_mapRewardAffixAcquisition::Natural:
			return TEXT("Natural");
		case Edemo_mapRewardAffixAcquisition::PityGuaranteed:
			return TEXT("PityGuaranteed");
		default:
			return TEXT("None");
		}
	}

	bool ParseAffixAcquisition(
		const FString& Text,
		Edemo_mapRewardAffixAcquisition& OutAcquisition)
	{
		if (Text == TEXT("Natural"))
		{
			OutAcquisition = Edemo_mapRewardAffixAcquisition::Natural;
			return true;
		}
		if (Text == TEXT("PityGuaranteed"))
		{
			OutAcquisition =
				Edemo_mapRewardAffixAcquisition::PityGuaranteed;
			return true;
		}
		if (Text == TEXT("None"))
		{
			OutAcquisition = Edemo_mapRewardAffixAcquisition::None;
			return true;
		}
		return false;
	}

	const TCHAR* RunStateString(Edemo_mapPersistentActiveRunState State)
	{
		switch (State)
		{
		case Edemo_mapPersistentActiveRunState::None: return TEXT("None");
		case Edemo_mapPersistentActiveRunState::Prepared: return TEXT("Prepared");
		case Edemo_mapPersistentActiveRunState::InProgress: return TEXT("InProgress");
		case Edemo_mapPersistentActiveRunState::Settled: return TEXT("Settled");
		case Edemo_mapPersistentActiveRunState::Extraction: return TEXT("Extraction");
		case Edemo_mapPersistentActiveRunState::Death: return TEXT("Death");
		case Edemo_mapPersistentActiveRunState::Abandon: return TEXT("Abandon");
		case Edemo_mapPersistentActiveRunState::RecoveredAbandon: return TEXT("RecoveredAbandon");
		case Edemo_mapPersistentActiveRunState::ActivationFailure: return TEXT("ActivationFailure");
		default: return TEXT("Invalid");
		}
	}

	bool ParseRunState(const FString& Text, Edemo_mapPersistentActiveRunState& OutState)
	{
		if (Text == TEXT("None")) { OutState = Edemo_mapPersistentActiveRunState::None; return true; }
		if (Text == TEXT("Prepared")) { OutState = Edemo_mapPersistentActiveRunState::Prepared; return true; }
		if (Text == TEXT("InProgress")) { OutState = Edemo_mapPersistentActiveRunState::InProgress; return true; }
		if (Text == TEXT("Settled")) { OutState = Edemo_mapPersistentActiveRunState::Settled; return true; }
		if (Text == TEXT("Extraction")) { OutState = Edemo_mapPersistentActiveRunState::Extraction; return true; }
		if (Text == TEXT("Death")) { OutState = Edemo_mapPersistentActiveRunState::Death; return true; }
		if (Text == TEXT("Abandon")) { OutState = Edemo_mapPersistentActiveRunState::Abandon; return true; }
		if (Text == TEXT("RecoveredAbandon")) { OutState = Edemo_mapPersistentActiveRunState::RecoveredAbandon; return true; }
		if (Text == TEXT("ActivationFailure")) { OutState = Edemo_mapPersistentActiveRunState::ActivationFailure; return true; }
		return false;
	}

	bool IsTerminalRunState(Edemo_mapPersistentActiveRunState State)
	{
		return State == Edemo_mapPersistentActiveRunState::Extraction
			|| State == Edemo_mapPersistentActiveRunState::Death
			|| State == Edemo_mapPersistentActiveRunState::Abandon
			|| State == Edemo_mapPersistentActiveRunState::RecoveredAbandon
			|| State == Edemo_mapPersistentActiveRunState::ActivationFailure;
	}

	TSharedRef<FJsonObject> ItemToJson(const Fdemo_mapPersistentItemRecord& Item)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("ItemInstanceId"), GuidString(Item.ItemInstanceId));
		Object->SetStringField(TEXT("ItemDefinitionId"), Item.ItemDefinitionId.ToString());
		Object->SetNumberField(TEXT("StackCount"), Item.StackCount);
		Object->SetStringField(TEXT("PersistentDomain"), DomainString(Item.PersistentDomain));
		if (Item.LegacySpatialParentItemInstanceId.IsValid())
		{
			Object->SetStringField(
				TEXT("LegacySpatialParentItemInstanceId"),
				GuidString(Item.LegacySpatialParentItemInstanceId));
		}
		Object->SetStringField(TEXT("EquipmentSlotId"), Item.EquipmentSlotId.IsNone() ? FString() : Item.EquipmentSlotId.ToString());
		Object->SetStringField(TEXT("OriginRunId"), GuidString(Item.OriginRunId));
		Object->SetStringField(
			TEXT("RewardEventKind"),
			RewardEventKindString(Item.RewardEventKind));
		Object->SetStringField(
			TEXT("RewardEventId"),
			GuidString(Item.RewardEventId));
		Object->SetNumberField(
			TEXT("RewardValueMultiplierBps"),
			Item.RewardValueMultiplierBps);
		Object->SetStringField(
			TEXT("RewardSourceRoleId"),
			Item.RewardSourceRoleId.IsNone()
				? FString()
				: Item.RewardSourceRoleId.ToString());
		Object->SetStringField(
			TEXT("RareRewardEventId"),
			GuidString(Item.RareRewardEventId));
		Object->SetStringField(
			TEXT("RareRewardPolicyId"),
			Item.RareRewardPolicyId.IsNone()
				? FString()
				: Item.RareRewardPolicyId.ToString());
		Object->SetStringField(
			TEXT("RareRewardTierId"),
			Item.RareRewardTierId.IsNone()
				? FString()
				: Item.RareRewardTierId.ToString());
		Object->SetNumberField(
			TEXT("RareRewardBonusValue"),
			static_cast<double>(Item.RareRewardBonusValue));
		if (!Item.AffixSet.IsEmpty())
		{
			Object->SetStringField(
				TEXT("AffixSetEventId"),
				GuidString(Item.AffixSet.AffixSetEventId));
			Object->SetStringField(
				TEXT("AffixPolicyId"),
				Item.AffixSet.AffixPolicyId.ToString());
			Object->SetStringField(
				TEXT("AffixAcquisitionKind"),
				AffixAcquisitionString(Item.AffixSet.Acquisition));
			TArray<TSharedPtr<FJsonValue>> AffixValues;
			for (const Fdemo_mapResolvedRewardAffix& Affix :
				Item.AffixSet.Affixes)
			{
				TSharedRef<FJsonObject> AffixObject =
					MakeShared<FJsonObject>();
				AffixObject->SetStringField(
					TEXT("AffixId"),
					Affix.AffixId.ToString());
				AffixObject->SetNumberField(
					TEXT("Tier"),
					static_cast<int32>(Affix.Tier));
				AffixObject->SetNumberField(
					TEXT("ResolvedMagnitudeScaled"),
					Affix.ResolvedMagnitudeScaled);
				AffixObject->SetNumberField(
					TEXT("ResolvedValue"),
					static_cast<double>(Affix.ResolvedValue));
				AffixValues.Add(
					MakeShared<FJsonValueObject>(AffixObject));
			}
			Object->SetArrayField(TEXT("Affixes"), AffixValues);
		}
		return Object;
	}

	bool JsonToItem(const TSharedPtr<FJsonObject>& Object, Fdemo_mapPersistentItemRecord& OutItem, FString& OutError)
	{
		if (!Object.IsValid()) { OutError = TEXT("Item record is not an object."); return false; }
		FString InstanceText, DefinitionText, DomainText, SlotText, OriginText;
		double StackNumber = 0.0;
		if (!Object->TryGetStringField(TEXT("ItemInstanceId"), InstanceText)
			|| !Object->TryGetStringField(TEXT("ItemDefinitionId"), DefinitionText)
			|| !Object->TryGetNumberField(TEXT("StackCount"), StackNumber)
			|| !Object->TryGetStringField(TEXT("PersistentDomain"), DomainText)
			|| !Object->TryGetStringField(TEXT("EquipmentSlotId"), SlotText)
			|| !Object->TryGetStringField(TEXT("OriginRunId"), OriginText))
		{
			OutError = TEXT("Item record is missing a required field.");
			return false;
		}
		if (!FMath::IsNearlyEqual(StackNumber, FMath::RoundToDouble(StackNumber)) || StackNumber < MIN_int32 || StackNumber > MAX_int32)
		{
			OutError = TEXT("Item StackCount is not an integer.");
			return false;
		}
		if (!ParseGuid(InstanceText, OutItem.ItemInstanceId) || !ParseGuid(OriginText, OutItem.OriginRunId) || !ParseDomain(DomainText, OutItem.PersistentDomain))
		{
			OutError = TEXT("Item GUID or persistent domain is invalid.");
			return false;
		}
		OutItem.ItemDefinitionId = FName(*DefinitionText);
		OutItem.StackCount = static_cast<int32>(StackNumber);
		OutItem.EquipmentSlotId = SlotText.IsEmpty() ? NAME_None : FName(*SlotText);
		FString LegacySpatialParentText;
		if (Object->TryGetStringField(TEXT("LegacySpatialParentItemInstanceId"), LegacySpatialParentText)
			&& (!ParseGuid(LegacySpatialParentText, OutItem.LegacySpatialParentItemInstanceId)
				|| !OutItem.LegacySpatialParentItemInstanceId.IsValid()))
		{
			OutError = TEXT("Item legacy spatial parent identity is invalid.");
			return false;
		}
		FString RewardKindText;
		if (Object->TryGetStringField(
			TEXT("RewardEventKind"),
			RewardKindText))
		{
			FString EventIdText;
			FString SourceRoleText;
			double MultiplierNumber = 0.0;
			if (!Object->TryGetStringField(
					TEXT("RewardEventId"),
					EventIdText)
				|| !Object->TryGetNumberField(
					TEXT("RewardValueMultiplierBps"),
					MultiplierNumber)
				|| !Object->TryGetStringField(
					TEXT("RewardSourceRoleId"),
					SourceRoleText)
				|| !ParseRewardEventKind(
					RewardKindText,
					OutItem.RewardEventKind)
				|| !ParseGuid(
					EventIdText,
					OutItem.RewardEventId)
				|| !FMath::IsNearlyEqual(
					MultiplierNumber,
					FMath::RoundToDouble(MultiplierNumber))
				|| MultiplierNumber < MIN_int32
				|| MultiplierNumber > MAX_int32)
			{
				OutError = TEXT("Item reward event metadata is malformed.");
				return false;
			}
			OutItem.RewardValueMultiplierBps =
				static_cast<int32>(MultiplierNumber);
			OutItem.RewardSourceRoleId = SourceRoleText.IsEmpty()
				? NAME_None
				: FName(*SourceRoleText);
		}
		else if (Object->HasField(TEXT("RewardEventId"))
			|| Object->HasField(TEXT("RewardValueMultiplierBps"))
			|| Object->HasField(TEXT("RewardSourceRoleId")))
		{
			OutError = TEXT("Item reward event metadata is partial.");
			return false;
		}
		FString RareEventIdText;
		if (Object->TryGetStringField(
			TEXT("RareRewardEventId"),
			RareEventIdText))
		{
			FString RarePolicyText;
			FString RareTierText;
			double RareBonusNumber = 0.0;
			if (!Object->TryGetStringField(
					TEXT("RareRewardPolicyId"),
					RarePolicyText)
				|| !Object->TryGetStringField(
					TEXT("RareRewardTierId"),
					RareTierText)
				|| !Object->TryGetNumberField(
					TEXT("RareRewardBonusValue"),
					RareBonusNumber)
				|| !ParseGuid(
					RareEventIdText,
					OutItem.RareRewardEventId)
				|| !FMath::IsNearlyEqual(
					RareBonusNumber,
					FMath::RoundToDouble(RareBonusNumber))
				|| RareBonusNumber < 0.0
				|| RareBonusNumber
					> static_cast<double>(MAX_int64))
			{
				OutError = TEXT("Item Rare reward metadata is malformed.");
				return false;
			}
			OutItem.RareRewardPolicyId = RarePolicyText.IsEmpty()
				? NAME_None : FName(*RarePolicyText);
			OutItem.RareRewardTierId = RareTierText.IsEmpty()
				? NAME_None : FName(*RareTierText);
			OutItem.RareRewardBonusValue =
				static_cast<int64>(RareBonusNumber);
		}
		else if (Object->HasField(TEXT("RareRewardPolicyId"))
			|| Object->HasField(TEXT("RareRewardTierId"))
			|| Object->HasField(TEXT("RareRewardBonusValue")))
		{
			OutError = TEXT("Item Rare reward metadata is partial.");
			return false;
		}
		FString AffixEventText;
		if (Object->TryGetStringField(
			TEXT("AffixSetEventId"),
			AffixEventText))
		{
			FString AffixPolicyText;
			FString AcquisitionText;
			const TArray<TSharedPtr<FJsonValue>>* AffixValues = nullptr;
			if (!Object->TryGetStringField(
					TEXT("AffixPolicyId"),
					AffixPolicyText)
				|| !Object->TryGetStringField(
					TEXT("AffixAcquisitionKind"),
					AcquisitionText)
				|| !Object->TryGetArrayField(
					TEXT("Affixes"),
					AffixValues)
				|| !ParseGuid(
					AffixEventText,
					OutItem.AffixSet.AffixSetEventId)
				|| !ParseAffixAcquisition(
					AcquisitionText,
					OutItem.AffixSet.Acquisition))
			{
				OutError = TEXT("Item Affix metadata is malformed.");
				return false;
			}
			OutItem.AffixSet.AffixPolicyId =
				AffixPolicyText.IsEmpty()
					? NAME_None : FName(*AffixPolicyText);
			for (const TSharedPtr<FJsonValue>& AffixValue : *AffixValues)
			{
				const TSharedPtr<FJsonObject>* AffixObject = nullptr;
				FString AffixIdText;
				double TierNumber = 0.0;
				double MagnitudeNumber = 0.0;
				double ValueNumber = 0.0;
				if (!AffixValue.IsValid()
					|| !AffixValue->TryGetObject(AffixObject)
					|| !AffixObject || !AffixObject->IsValid()
					|| !(*AffixObject)->TryGetStringField(
						TEXT("AffixId"), AffixIdText)
					|| !(*AffixObject)->TryGetNumberField(
						TEXT("Tier"), TierNumber)
					|| !(*AffixObject)->TryGetNumberField(
						TEXT("ResolvedMagnitudeScaled"),
						MagnitudeNumber)
					|| !(*AffixObject)->TryGetNumberField(
						TEXT("ResolvedValue"), ValueNumber)
					|| !FMath::IsNearlyEqual(
						TierNumber, FMath::RoundToDouble(TierNumber))
					|| !FMath::IsNearlyEqual(
						MagnitudeNumber,
						FMath::RoundToDouble(MagnitudeNumber))
					|| !FMath::IsNearlyEqual(
						ValueNumber,
						FMath::RoundToDouble(ValueNumber))
					|| TierNumber < 1 || TierNumber > 3
					|| MagnitudeNumber < MIN_int32
					|| MagnitudeNumber > MAX_int32
					|| ValueNumber <= 0.0
					|| ValueNumber > static_cast<double>(MAX_int64))
				{
					OutError = TEXT("Item Affix entry is malformed.");
					return false;
				}
				Fdemo_mapResolvedRewardAffix Affix;
				Affix.AffixId = FName(*AffixIdText);
				Affix.Tier =
					static_cast<Edemo_mapRewardAffixTier>(
						static_cast<int32>(TierNumber));
				Affix.ResolvedMagnitudeScaled =
					static_cast<int32>(MagnitudeNumber);
				Affix.ResolvedValue = static_cast<int64>(ValueNumber);
				OutItem.AffixSet.Affixes.Add(Affix);
			}
		}
		else if (Object->HasField(TEXT("AffixPolicyId"))
			|| Object->HasField(TEXT("AffixAcquisitionKind"))
			|| Object->HasField(TEXT("Affixes")))
		{
			OutError = TEXT("Item Affix metadata is partial.");
			return false;
		}
		return true;
	}

	TArray<TSharedPtr<FJsonValue>> GuidsToJson(const TArray<FGuid>& Guids)
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		Values.Reserve(Guids.Num());
		for (const FGuid& Id : Guids)
		{
			Values.Add(MakeShared<FJsonValueString>(GuidString(Id)));
		}
		return Values;
	}

	bool JsonToGuids(
		const TArray<TSharedPtr<FJsonValue>>* Values,
		TArray<FGuid>& OutGuids,
		const TCHAR* FieldName,
		FString& OutError)
	{
		if (!Values)
		{
			OutError = FString::Printf(TEXT("%s is missing."), FieldName);
			return false;
		}
		OutGuids.Reset();
		OutGuids.Reserve(Values->Num());
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			FGuid Id;
			if (!Value.IsValid() || !ParseGuid(Value->AsString(), Id))
			{
				OutError = FString::Printf(TEXT("%s contains a malformed GUID."), FieldName);
				return false;
			}
			OutGuids.Add(Id);
		}
		return true;
	}

	bool ExactCoreProfileMatch(const Fdemo_mapPersistentProfile& A, const Fdemo_mapPersistentProfile& B)
	{
		return A == B;
	}

	FString Int64String(int64 Value)
	{
		return FString::Printf(TEXT("%lld"), static_cast<long long>(Value));
	}

	bool ParseCanonicalNonNegativeInt64(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, int64& OutValue, FString& OutError)
	{
		FString Text;
		if (!Object.IsValid() || !Object->TryGetStringField(FieldName, Text))
		{
			OutError = FString::Printf(TEXT("%s must be a canonical decimal string."), FieldName);
			return false;
		}
		if (Text.IsEmpty() || (Text.Len() > 1 && Text[0] == TCHAR('0')))
		{
			OutError = FString::Printf(TEXT("%s is not canonical."), FieldName);
			return false;
		}
		for (TCHAR Character : Text)
		{
			if (Character < TCHAR('0') || Character > TCHAR('9'))
			{
				OutError = FString::Printf(TEXT("%s is not a non-negative integer."), FieldName);
				return false;
			}
		}
		int64 Parsed = 0;
		for (TCHAR Character : Text)
		{
			const int64 Digit = static_cast<int64>(Character - TCHAR('0'));
			if (Parsed > (MAX_int64 - Digit) / 10)
			{
				OutError = FString::Printf(TEXT("%s is outside the int64 contract."), FieldName);
				return false;
			}
			Parsed = Parsed * 10 + Digit;
		}
		OutValue = Parsed;
		if (Int64String(OutValue) != Text)
		{
			OutError = FString::Printf(TEXT("%s is outside the int64 contract."), FieldName);
			return false;
		}
		return true;
	}

	Fdemo_mapPersistentProfile PromoteLegacyProfile(const Fdemo_mapPersistentProfile& Legacy)
	{
		Fdemo_mapPersistentProfile Promoted = Legacy;
		Promoted.SchemaVersion = Fdemo_mapPersistentProfile::CurrentSchemaVersion;
		if (Legacy.SchemaVersion == 1)
		{
			Promoted.PersistentSpiritStones = 0;
			Promoted.ActiveRun.RiskSpiritStones = 0;
		}
		if (Legacy.SchemaVersion <= 2)
		{
			Promoted.PreparationLayout = Fdemo_mapPersistentPreparationLayout();
			Promoted.ActiveRun.ConsumedSpiritStoneSourceIds.Reset();
		}
		if (Legacy.SchemaVersion <= 5
			&& Promoted.PreparationLayout.AccessoryItemInstanceId.IsValid())
		{
			const TArray<Fdemo_mapPersistentItemRecord>& Domain =
				Promoted.ActiveRun.bHasActiveRun
					? Promoted.ActiveRun.ActiveRunItems
					: Promoted.PermanentStash;
			const Fdemo_mapPersistentItemRecord* RingRecord =
				Domain.FindByPredicate([&Promoted](
					const Fdemo_mapPersistentItemRecord& Candidate)
				{
					return Candidate.ItemInstanceId
						== Promoted.PreparationLayout.AccessoryItemInstanceId;
				});
			const Fdemo_mapItemDefinition* RingDefinition = RingRecord
				? Fdemo_mapItemDefinitions::Find(RingRecord->ItemDefinitionId)
				: nullptr;
			if (RingDefinition
				&& RingDefinition->CategoryId
					== Fdemo_mapItemIds::SpatialRingCategory)
			{
				Promoted.PreparationLayout.SpatialRingItemInstanceId =
					Promoted.PreparationLayout.AccessoryItemInstanceId;
				Promoted.PreparationLayout.AccessoryItemInstanceId.Invalidate();
			}
		}
		if (Legacy.SchemaVersion <= 5)
		{
			for (Fdemo_mapPersistentItemRecord& Item :
				Promoted.ActiveRun.ActiveRunItems)
			{
				const Fdemo_mapItemDefinition* Definition =
					Fdemo_mapItemDefinitions::Find(Item.ItemDefinitionId);
				if (Definition
					&& Definition->CategoryId
						== Fdemo_mapItemIds::SpatialRingCategory
					&& Item.EquipmentSlotId
						== Fdemo_mapItemIds::AccessorySlot)
				{
					Item.EquipmentSlotId = Fdemo_mapItemIds::SpatialRingSlot;
				}
			}
		}
		Promoted.TownLevel = 0;
		return Promoted;
	}

	bool IsExactLegacySourceFor(const Fdemo_mapPersistentProfile& Candidate, const Fdemo_mapPersistentProfile& Legacy)
	{
		return (Legacy.SchemaVersion >= 1 && Legacy.SchemaVersion <= 5)
			&& PromoteLegacyProfile(Legacy) == Candidate;
	}

	uint32 RotateRight(uint32 Value, uint32 Count)
	{
		return (Value >> Count) | (Value << (32u - Count));
	}

	FString Sha256Hex(const TArray<uint8>& Input)
	{
		static const uint32 K[64] = {
			0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
			0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
			0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
			0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
			0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
			0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
			0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
			0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
		};
		uint32 H[8] = { 0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u };
		TArray<uint8> Message = Input;
		const uint64 BitLength = static_cast<uint64>(Input.Num()) * 8ull;
		Message.Add(0x80u);
		while ((Message.Num() % 64) != 56) Message.Add(0u);
		for (int32 Shift = 56; Shift >= 0; Shift -= 8) Message.Add(static_cast<uint8>((BitLength >> Shift) & 0xffu));
		for (int32 Offset = 0; Offset < Message.Num(); Offset += 64)
		{
			uint32 W[64]{};
			for (int32 Index = 0; Index < 16; ++Index)
			{
				const int32 Base = Offset + Index * 4;
				W[Index] = (static_cast<uint32>(Message[Base]) << 24) | (static_cast<uint32>(Message[Base + 1]) << 16) | (static_cast<uint32>(Message[Base + 2]) << 8) | Message[Base + 3];
			}
			for (int32 Index = 16; Index < 64; ++Index)
			{
				const uint32 S0 = RotateRight(W[Index - 15], 7) ^ RotateRight(W[Index - 15], 18) ^ (W[Index - 15] >> 3);
				const uint32 S1 = RotateRight(W[Index - 2], 17) ^ RotateRight(W[Index - 2], 19) ^ (W[Index - 2] >> 10);
				W[Index] = W[Index - 16] + S0 + W[Index - 7] + S1;
			}
			uint32 A=H[0],B=H[1],C=H[2],D=H[3],E=H[4],F=H[5],G=H[6],HH=H[7];
			for (int32 Index = 0; Index < 64; ++Index)
			{
				const uint32 S1 = RotateRight(E, 6) ^ RotateRight(E, 11) ^ RotateRight(E, 25);
				const uint32 Ch = (E & F) ^ ((~E) & G);
				const uint32 Temp1 = HH + S1 + Ch + K[Index] + W[Index];
				const uint32 S0 = RotateRight(A, 2) ^ RotateRight(A, 13) ^ RotateRight(A, 22);
				const uint32 Maj = (A & B) ^ (A & C) ^ (B & C);
				const uint32 Temp2 = S0 + Maj;
				HH=G; G=F; F=E; E=D+Temp1; D=C; C=B; B=A; A=Temp1+Temp2;
			}
			H[0]+=A; H[1]+=B; H[2]+=C; H[3]+=D; H[4]+=E; H[5]+=F; H[6]+=G; H[7]+=HH;
		}
		return FString::Printf(TEXT("%08x%08x%08x%08x%08x%08x%08x%08x"), H[0],H[1],H[2],H[3],H[4],H[5],H[6],H[7]);
	}
}

Fdemo_mapPersistentProfile Fdemo_mapProfileRepository::CreateFreshProfile() const
{
	Fdemo_mapPersistentProfile Profile;
	Profile.SchemaVersion = Fdemo_mapPersistentProfile::CurrentSchemaVersion;
	Profile.ProfileId = FGuid::NewGuid();
	Profile.SaveGeneration = 0;
	Profile.ProfileMetadata.ProfileName = TEXT("Default");
	Profile.ProfileMetadata.CreatedUtc = FDateTime::UtcNow().ToIso8601();
	Profile.ProfileMetadata.LastSavedUtc = Profile.ProfileMetadata.CreatedUtc;
	Profile.PersistentSpiritStones = 0;
	Profile.ActiveRun.RiskSpiritStones = 0;
	for (FName DefinitionId : { Fdemo_mapItemIds::TrainingBlade, Fdemo_mapItemIds::TrainingVest, Fdemo_mapItemIds::WindTalisman })
	{
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = FGuid::NewGuid();
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = 1;
		Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		Profile.PermanentStash.Add(Item);
	}
	FString ShopError;
	if (!Fdemo_mapRewardShopStock::Generate(
			Profile.ProfileId,
			0,
			FGuid(),
			Profile.ShopStock,
			&ShopError))
	{
		Profile.ShopStock = Fdemo_mapPersistentShopStockState();
	}
	return Profile;
}

bool Fdemo_mapProfileRepository::ValidateProfile(const Fdemo_mapPersistentProfile& Profile, FString* OutError) const
{
	auto Fail = [OutError](const FString& Message) { if (OutError) *OutError = Message; return false; };
	if (Profile.SchemaVersion != Fdemo_mapPersistentProfile::CurrentSchemaVersion) return Fail(TEXT("Profile SchemaVersion is not current."));
	if (!Profile.ProfileId.IsValid()) return Fail(TEXT("ProfileId is invalid."));
	if (Profile.SaveGeneration < 0) return Fail(TEXT("SaveGeneration is negative."));
	if (Profile.PersistentSpiritStones < 0) return Fail(TEXT("PersistentSpiritStones is negative."));
	if (Profile.TownLevel < 0 || Profile.TownLevel > 5) return Fail(TEXT("TownLevel is outside the 0—5 contract."));
	if (Profile.ActiveRun.RiskSpiritStones < 0) return Fail(TEXT("RiskSpiritStones is negative."));
	if (Profile.ProfileMetadata.ProfileName.IsEmpty() || Profile.ProfileMetadata.CreatedUtc.IsEmpty() || Profile.ProfileMetadata.LastSavedUtc.IsEmpty()) return Fail(TEXT("Profile metadata is incomplete."));

	TSet<FGuid> ItemIds;
	TMap<FGuid, const Fdemo_mapPersistentItemRecord*> RewardEvents;
	TMap<FGuid, const Fdemo_mapPersistentItemRecord*> RareRewardEvents;
	auto ValidateItem = [&Fail, &ItemIds, &RewardEvents, &RareRewardEvents](const Fdemo_mapPersistentItemRecord& Item, Edemo_mapPersistentDomain ExpectedDomain)
	{
		if (!Item.ItemInstanceId.IsValid()) return Fail(TEXT("ItemInstanceId is invalid."));
		if (ItemIds.Contains(Item.ItemInstanceId)) return Fail(TEXT("Duplicate ItemInstanceId."));
		ItemIds.Add(Item.ItemInstanceId);
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Item.ItemDefinitionId);
		if (!Definition) return Fail(TEXT("Unknown ItemDefinitionId."));
		if (Item.StackCount <= 0 || Item.StackCount > Definition->MaxStackSize) return Fail(TEXT("Item StackCount violates its definition."));
		if (!Definition->CompatibleSlotIds.IsEmpty() && Item.StackCount != 1) return Fail(TEXT("Equipment definition quantity must equal one."));
		if (Item.PersistentDomain != ExpectedDomain) return Fail(TEXT("Item belongs to an unexpected persistent domain."));
		if (ExpectedDomain == Edemo_mapPersistentDomain::PermanentStash && (!Item.EquipmentSlotId.IsNone() || Item.OriginRunId.IsValid())) return Fail(TEXT("Permanent Stash item has run or equipment metadata."));
		if (!Item.EquipmentSlotId.IsNone() && !Definition->CompatibleSlotIds.Contains(Item.EquipmentSlotId)) return Fail(TEXT("EquipmentSlotId is incompatible with the definition."));
		FString RewardError;
		if (!Fdemo_mapRewardEventRules::IsValid(
			Item.RewardEventKind,
			Item.RewardEventId,
			Item.RewardValueMultiplierBps,
			Item.RewardSourceRoleId,
			Item.RareRewardEventId,
			Item.RareRewardPolicyId,
			Item.RareRewardTierId,
			Item.RareRewardBonusValue,
			&RewardError))
		{
			return Fail(RewardError);
		}
		if (!Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
			Item.ItemDefinitionId,
			Item.StackCount,
			Item.AffixSet,
			&RewardError))
		{
			return Fail(RewardError);
		}
		if (Item.RewardEventId.IsValid())
		{
			if (const Fdemo_mapPersistentItemRecord* const* Existing =
				RewardEvents.Find(Item.RewardEventId))
			{
				if (!Fdemo_mapRewardEventRules::AreStackCompatible(
					(*Existing)->ItemDefinitionId,
					(*Existing)->RewardEventKind,
					(*Existing)->RewardEventId,
					(*Existing)->RewardValueMultiplierBps,
					(*Existing)->RewardSourceRoleId,
					Item.ItemDefinitionId,
					Item.RewardEventKind,
					Item.RewardEventId,
					Item.RewardValueMultiplierBps,
					Item.RewardSourceRoleId))
				{
					return Fail(TEXT("RewardEventId conflicts across incompatible persistent items."));
				}
			}
			else
			{
				RewardEvents.Add(Item.RewardEventId, &Item);
			}
		}
		if (Item.RareRewardEventId.IsValid())
		{
			if (const Fdemo_mapPersistentItemRecord* const* Existing =
				RareRewardEvents.Find(Item.RareRewardEventId))
			{
				if ((*Existing)->RareRewardPolicyId
						!= Item.RareRewardPolicyId
					|| (*Existing)->RareRewardTierId
						!= Item.RareRewardTierId
					|| (*Existing)->RewardSourceRoleId
						!= Item.RewardSourceRoleId)
				{
					return Fail(TEXT("RareRewardEventId conflicts across persistent carrier provenance."));
				}
			}
			else
			{
				RareRewardEvents.Add(Item.RareRewardEventId, &Item);
			}
		}
		return true;
	};

	for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash)
	{
		if (!ValidateItem(Item, Edemo_mapPersistentDomain::PermanentStash)) return false;
	}
	FString ShopStockError;
	if (!Fdemo_mapRewardShopStock::ValidateState(
			Profile.ProfileId,
			Profile.ShopStock,
			&ShopStockError))
	{
		return Fail(ShopStockError);
	}
	if (Profile.ShopStock.bInitialized)
	{
		for (const Fdemo_mapPersistentShopStockEntry& Entry :
			Profile.ShopStock.Entries)
		{
			if (Entry.State
				== Edemo_mapPersistentShopStockEntryState::Available
				&& !ValidateItem(
					Entry.Item,
					Edemo_mapPersistentDomain::ShopStock))
			{
				return false;
			}
		}
	}

	const Fdemo_mapPersistentActiveRunRecord& Run = Profile.ActiveRun;
	if (!Run.bHasActiveRun)
	{
		if (Run.RiskSpiritStones != 0) return Fail(TEXT("Inactive ActiveRun contains risk spirit stones."));
		if (Run.ActiveRunState == Edemo_mapPersistentActiveRunState::None)
		{
			if (Run.ActiveRunId.IsValid() || !Run.DeployedItemIds.IsEmpty() || !Run.ActiveRunItems.IsEmpty()
				|| !Run.ConsumedSpiritStoneSourceIds.IsEmpty() || Run.CommittedSettlementId.IsValid())
				return Fail(TEXT("Idle ActiveRun record contains run, risk, or settlement data."));
		}
		else if (IsTerminalRunState(Run.ActiveRunState))
		{
			if (!Run.ActiveRunId.IsValid() || !Run.DeployedItemIds.IsEmpty() || !Run.ActiveRunItems.IsEmpty()
				|| !Run.ConsumedSpiritStoneSourceIds.IsEmpty()
				|| !Run.CommittedSettlementId.IsValid() || Run.CommittedSettlementId != Profile.LastSettlementId)
				return Fail(TEXT("Terminal ActiveRun tombstone is inconsistent."));
		}
		else
		{
			return Fail(TEXT("Inactive ActiveRun record has an unsupported state."));
		}
	}
	else
	{
		if (!Run.ActiveRunId.IsValid() || (Run.ActiveRunState != Edemo_mapPersistentActiveRunState::Prepared && Run.ActiveRunState != Edemo_mapPersistentActiveRunState::InProgress))
			return Fail(TEXT("ActiveRun identity/state combination is invalid."));
		if (Run.CommittedSettlementId.IsValid()) return Fail(TEXT("Unsettled ActiveRun contains a SettlementId."));
		TSet<FName> SourceIds;
		for (FName SourceId : Run.ConsumedSpiritStoneSourceIds)
		{
			if (SourceId.IsNone() || SourceIds.Contains(SourceId)) return Fail(TEXT("Consumed Spirit Stone SourceIds are invalid or duplicated."));
			SourceIds.Add(SourceId);
		}
		TSet<FGuid> DeployedIds;
		for (const FGuid& Id : Run.DeployedItemIds)
		{
			if (!Id.IsValid() || DeployedIds.Contains(Id)) return Fail(TEXT("ActiveRun deployed IDs are invalid or duplicated."));
			DeployedIds.Add(Id);
		}
		for (const Fdemo_mapPersistentItemRecord& Item : Run.ActiveRunItems)
		{
			if (!ValidateItem(Item, Edemo_mapPersistentDomain::ActiveRun)) return false;
			if ((Item.OriginRunId.IsValid() && Item.OriginRunId != Run.ActiveRunId) || !DeployedIds.Contains(Item.ItemInstanceId)) return Fail(TEXT("ActiveRun item identity or OriginRunId is inconsistent."));
		}
		if (DeployedIds.Num() != Run.ActiveRunItems.Num()) return Fail(TEXT("ActiveRun deployed IDs and item records differ."));
	}

	// P4x retires PreparationLayout from product Start Run.  It remains serialized
	// only for historical tools and is intentionally treated as opaque legacy
	// metadata here: stale equipment/ring/hotbar references must not make an
	// otherwise valid Profile unloadable or block a direct Start Run.  Explicit
	// legacy layout commits and prepared-run requests still validate their selected
	// layout inside their own transactions before any write is accepted.
	const Fdemo_mapPersistentWarehouseLayout& Warehouse =
		Profile.WarehouseLayout;
	if (!Warehouse.bInitialized)
	{
		if (!Warehouse.SlotItemInstanceIds.IsEmpty())
			return Fail(TEXT("Uninitialized WarehouseLayout must not contain slot hints."));
	}
	else
	{
		if (Warehouse.SlotItemInstanceIds.Num()
			!= Fdemo_mapPersistentWarehouseLayout::SlotCount)
			return Fail(TEXT("Initialized WarehouseLayout must contain exactly 30 slot hints."));
		TSet<FGuid> WarehouseHintIds;
		for (const FGuid& ItemId : Warehouse.SlotItemInstanceIds)
		{
			if (!ItemId.IsValid()) continue;
			if (WarehouseHintIds.Contains(ItemId))
				return Fail(TEXT("WarehouseLayout contains duplicate ItemInstanceId hints."));
			WarehouseHintIds.Add(ItemId);
		}
	}
	return true;
}

bool Fdemo_mapProfileRepository::SerializeProfile(const Fdemo_mapPersistentProfile& Profile, TArray<uint8>& OutBytes, FString& OutError) const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("SchemaVersion"), Profile.SchemaVersion);
	Root->SetStringField(TEXT("ProfileId"), GuidString(Profile.ProfileId));
	Root->SetNumberField(TEXT("SaveGeneration"), Profile.SaveGeneration);
	TSharedRef<FJsonObject> Metadata = MakeShared<FJsonObject>();
	Metadata->SetStringField(TEXT("ProfileName"), Profile.ProfileMetadata.ProfileName);
	Metadata->SetStringField(TEXT("CreatedUtc"), Profile.ProfileMetadata.CreatedUtc);
	Metadata->SetStringField(TEXT("LastSavedUtc"), Profile.ProfileMetadata.LastSavedUtc);
	Root->SetObjectField(TEXT("ProfileMetadata"), Metadata);
	Root->SetStringField(TEXT("PersistentSpiritStones"), Int64String(Profile.PersistentSpiritStones));
	Root->SetNumberField(TEXT("TownLevel"), Profile.TownLevel);
	TArray<TSharedPtr<FJsonValue>> Stash;
	for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash) Stash.Add(MakeShared<FJsonValueObject>(ItemToJson(Item)));
	Root->SetArrayField(TEXT("PermanentStash"), Stash);
	if (Profile.ShopStock.bInitialized)
	{
		TSharedRef<FJsonObject> ShopStock = MakeShared<FJsonObject>();
		ShopStock->SetStringField(
			TEXT("PolicyId"),
			Profile.ShopStock.PolicyId.ToString());
		ShopStock->SetNumberField(
			TEXT("Generation"),
			Profile.ShopStock.Generation);
		ShopStock->SetStringField(
			TEXT("ShopStockEventId"),
			GuidString(Profile.ShopStock.ShopStockEventId));
		ShopStock->SetStringField(
			TEXT("LastAppliedTerminalId"),
			GuidString(Profile.ShopStock.LastAppliedTerminalId));
		TArray<TSharedPtr<FJsonValue>> Entries;
		for (const Fdemo_mapPersistentShopStockEntry& Entry :
			Profile.ShopStock.Entries)
		{
			TSharedRef<FJsonObject> EntryObject = MakeShared<FJsonObject>();
			EntryObject->SetStringField(
				TEXT("SlotId"),
				Entry.SlotId.ToString());
			EntryObject->SetNumberField(
				TEXT("SlotOrdinal"),
				Entry.SlotOrdinal);
			EntryObject->SetStringField(
				TEXT("State"),
				ShopStockEntryStateString(Entry.State));
			if (Entry.State
				== Edemo_mapPersistentShopStockEntryState::Available)
			{
				EntryObject->SetObjectField(
					TEXT("Item"),
					ItemToJson(Entry.Item));
				EntryObject->SetStringField(
					TEXT("QuotedBuyValue"),
					Int64String(Entry.QuotedBuyValue));
			}
			else
			{
				EntryObject->SetStringField(
					TEXT("SoldItemInstanceId"),
					GuidString(Entry.SoldItemInstanceId));
			}
			Entries.Add(MakeShared<FJsonValueObject>(EntryObject));
		}
		ShopStock->SetArrayField(TEXT("Entries"), Entries);
		Root->SetObjectField(TEXT("ShopStock"), ShopStock);
	}
	TSharedRef<FJsonObject> Layout = MakeShared<FJsonObject>();
	Layout->SetStringField(TEXT("WeaponItemInstanceId"), GuidString(Profile.PreparationLayout.WeaponItemInstanceId));
	Layout->SetStringField(TEXT("ArmorItemInstanceId"), GuidString(Profile.PreparationLayout.ArmorItemInstanceId));
	Layout->SetStringField(TEXT("AccessoryItemInstanceId"), GuidString(Profile.PreparationLayout.AccessoryItemInstanceId));
	Layout->SetStringField(TEXT("SpatialRingItemInstanceId"), GuidString(Profile.PreparationLayout.SpatialRingItemInstanceId));
	Layout->SetStringField(TEXT("BackpackItemInstanceId"), GuidString(Profile.PreparationLayout.BackpackItemInstanceId));
	Layout->SetArrayField(TEXT("OrderedRunInventoryItemInstanceIds"), GuidsToJson(Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds));
	Layout->SetArrayField(TEXT("HotbarItemInstanceIds"), GuidsToJson(Profile.PreparationLayout.HotbarItemInstanceIds));
	Root->SetObjectField(TEXT("PreparationLayout"), Layout);
	if (Profile.WarehouseLayout.bInitialized)
	{
		TSharedRef<FJsonObject> WarehouseLayout =
			MakeShared<FJsonObject>();
		WarehouseLayout->SetArrayField(
			TEXT("SlotItemInstanceIds"),
			GuidsToJson(
				Profile.WarehouseLayout.SlotItemInstanceIds));
		Root->SetObjectField(
			TEXT("WarehouseLayout"),
			WarehouseLayout);
	}
	TSharedRef<FJsonObject> Run = MakeShared<FJsonObject>();
	Run->SetBoolField(TEXT("HasActiveRun"), Profile.ActiveRun.bHasActiveRun);
	Run->SetStringField(TEXT("ActiveRunId"), GuidString(Profile.ActiveRun.ActiveRunId));
	Run->SetStringField(TEXT("ActiveRunState"), RunStateString(Profile.ActiveRun.ActiveRunState));
	Run->SetStringField(TEXT("RiskSpiritStones"), Int64String(Profile.ActiveRun.RiskSpiritStones));
	TArray<TSharedPtr<FJsonValue>> Deployed;
	for (const FGuid& Id : Profile.ActiveRun.DeployedItemIds) Deployed.Add(MakeShared<FJsonValueString>(GuidString(Id)));
	Run->SetArrayField(TEXT("DeployedItemIds"), Deployed);
	TArray<TSharedPtr<FJsonValue>> RunItems;
	for (const Fdemo_mapPersistentItemRecord& Item : Profile.ActiveRun.ActiveRunItems) RunItems.Add(MakeShared<FJsonValueObject>(ItemToJson(Item)));
	Run->SetArrayField(TEXT("ActiveRunItems"), RunItems);
	TArray<TSharedPtr<FJsonValue>> ConsumedSources;
	for (FName SourceId : Profile.ActiveRun.ConsumedSpiritStoneSourceIds)
	{
		ConsumedSources.Add(MakeShared<FJsonValueString>(SourceId.ToString()));
	}
	Run->SetArrayField(TEXT("ConsumedSpiritStoneSourceIds"), ConsumedSources);
	Run->SetStringField(TEXT("CommittedSettlementId"), GuidString(Profile.ActiveRun.CommittedSettlementId));
	Root->SetObjectField(TEXT("ActiveRun"), Run);
	Root->SetStringField(TEXT("LastSettlementId"), GuidString(Profile.LastSettlementId));

	FString Json;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	if (!FJsonSerializer::Serialize(Root, Writer)) { OutError = TEXT("JSON serialization failed."); return false; }
	FTCHARToUTF8 Utf8(*Json);
	OutBytes.Reset(Utf8.Length());
	OutBytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	return true;
}

Fdemo_mapProfileRepository::FReadResult Fdemo_mapProfileRepository::DeserializeProfile(const TArray<uint8>& Bytes) const
{
	FReadResult Result;
	Result.Bytes = Bytes;
	if (Bytes.IsEmpty()) { Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("Profile file is empty."); return Result; }
	FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
	const FString Json(Converted.Length(), Converted.Get());
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) { Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("Profile JSON could not be parsed."); return Result; }
	double SchemaNumber = 0.0;
	if (!Root->TryGetNumberField(TEXT("SchemaVersion"), SchemaNumber)
		|| !FMath::IsNearlyEqual(SchemaNumber, FMath::RoundToDouble(SchemaNumber))
		|| SchemaNumber < MIN_int32
		|| SchemaNumber > MAX_int32)
	{
		Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("SchemaVersion is missing, non-integral, or out of range."); return Result;
	}
	const int32 Schema = static_cast<int32>(SchemaNumber);
	if (Schema > Fdemo_mapPersistentProfile::CurrentSchemaVersion) { Result.Kind = EReadKind::FutureSchema; Result.Diagnostic = TEXT("Profile uses a future SchemaVersion."); return Result; }
	if (Schema != 1 && Schema != 2 && Schema != 3 && Schema != 4 && Schema != 5 && Schema != Fdemo_mapPersistentProfile::CurrentSchemaVersion) { Result.Kind = EReadKind::InvalidData; Result.Diagnostic = TEXT("Profile uses an unsupported old SchemaVersion."); return Result; }

	Fdemo_mapPersistentProfile Profile;
	Profile.SchemaVersion = Schema;
	FString ProfileIdText, LastSettlementText;
	double GenerationNumber = 0.0;
	const TSharedPtr<FJsonObject>* Metadata = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Stash = nullptr;
	const TSharedPtr<FJsonObject>* Run = nullptr;
	if (!Root->TryGetStringField(TEXT("ProfileId"), ProfileIdText)
		|| !Root->TryGetNumberField(TEXT("SaveGeneration"), GenerationNumber)
		|| !Root->TryGetObjectField(TEXT("ProfileMetadata"), Metadata)
		|| !Root->TryGetArrayField(TEXT("PermanentStash"), Stash)
		|| !Root->TryGetObjectField(TEXT("ActiveRun"), Run)
		|| !Root->TryGetStringField(TEXT("LastSettlementId"), LastSettlementText)
		|| !FMath::IsNearlyEqual(GenerationNumber, FMath::RoundToDouble(GenerationNumber))
		|| GenerationNumber < MIN_int32 || GenerationNumber > MAX_int32
		|| !ParseGuid(ProfileIdText, Profile.ProfileId)
		|| !ParseGuid(LastSettlementText, Profile.LastSettlementId))
	{
		Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("Profile root fields are missing or malformed."); return Result;
	}
	Profile.SaveGeneration = static_cast<int32>(GenerationNumber);
	if (!(*Metadata)->TryGetStringField(TEXT("ProfileName"), Profile.ProfileMetadata.ProfileName)
		|| !(*Metadata)->TryGetStringField(TEXT("CreatedUtc"), Profile.ProfileMetadata.CreatedUtc)
		|| !(*Metadata)->TryGetStringField(TEXT("LastSavedUtc"), Profile.ProfileMetadata.LastSavedUtc))
	{
		Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("Profile metadata is malformed."); return Result;
	}
	if (Schema >= 2)
	{
		FString IntegerError;
		if (!ParseCanonicalNonNegativeInt64(Root, TEXT("PersistentSpiritStones"), Profile.PersistentSpiritStones, IntegerError))
		{
			Result.Kind = EReadKind::InvalidData; Result.Diagnostic = IntegerError; return Result;
		}
	}
	if (Schema >= 4)
	{
		double TownLevelNumber = 0.0;
		if (!Root->TryGetNumberField(TEXT("TownLevel"), TownLevelNumber)
			|| !FMath::IsNearlyEqual(TownLevelNumber, FMath::RoundToDouble(TownLevelNumber))
			|| TownLevelNumber < 0 || TownLevelNumber > 5)
		{
			Result.Kind = EReadKind::InvalidData; Result.Diagnostic = TEXT("TownLevel is missing or outside the 0—5 contract."); return Result;
		}
		Profile.TownLevel = static_cast<int32>(TownLevelNumber);
	}
	for (const TSharedPtr<FJsonValue>& Value : *Stash)
	{
		Fdemo_mapPersistentItemRecord Item;
		FString Error;
		if (!Value.IsValid() || !JsonToItem(Value->AsObject(), Item, Error)) { Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = Error; return Result; }
		Profile.PermanentStash.Add(Item);
	}
	if (Schema >= 3
		&& Root->HasField(TEXT("ShopStock")))
	{
		const TSharedPtr<FJsonObject>* ShopStock = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
		FString PolicyText;
		FString EventText;
		FString TerminalText;
		double ShopGenerationNumber = 0.0;
		if (!Root->TryGetObjectField(TEXT("ShopStock"), ShopStock)
			|| !(*ShopStock)->TryGetStringField(
				TEXT("PolicyId"), PolicyText)
			|| !(*ShopStock)->TryGetNumberField(
				TEXT("Generation"), ShopGenerationNumber)
			|| !(*ShopStock)->TryGetStringField(
				TEXT("ShopStockEventId"), EventText)
			|| !(*ShopStock)->TryGetStringField(
				TEXT("LastAppliedTerminalId"), TerminalText)
			|| !(*ShopStock)->TryGetArrayField(
				TEXT("Entries"), Entries)
			|| !FMath::IsNearlyEqual(
				ShopGenerationNumber,
				FMath::RoundToDouble(ShopGenerationNumber))
			|| ShopGenerationNumber < 0
			|| ShopGenerationNumber > MAX_int32
			|| !ParseGuid(
				EventText,
				Profile.ShopStock.ShopStockEventId)
			|| !ParseGuid(
				TerminalText,
				Profile.ShopStock.LastAppliedTerminalId))
		{
			Result.Kind = EReadKind::ParseFailure;
			Result.Diagnostic = TEXT("ShopStock root is malformed.");
			return Result;
		}
		Profile.ShopStock.bInitialized = true;
		Profile.ShopStock.PolicyId = FName(*PolicyText);
		Profile.ShopStock.Generation =
			static_cast<int32>(ShopGenerationNumber);
		for (const TSharedPtr<FJsonValue>& Value : *Entries)
		{
			const TSharedPtr<FJsonObject> EntryObject =
				Value.IsValid() ? Value->AsObject() : nullptr;
			Fdemo_mapPersistentShopStockEntry Entry;
			FString SlotText;
			FString StateText;
			double OrdinalNumber = 0.0;
			if (!EntryObject.IsValid()
				|| !EntryObject->TryGetStringField(
					TEXT("SlotId"), SlotText)
				|| !EntryObject->TryGetNumberField(
					TEXT("SlotOrdinal"), OrdinalNumber)
				|| !EntryObject->TryGetStringField(
					TEXT("State"), StateText)
				|| !FMath::IsNearlyEqual(
					OrdinalNumber,
					FMath::RoundToDouble(OrdinalNumber))
				|| OrdinalNumber < 0
				|| OrdinalNumber > MAX_int32
				|| !ParseShopStockEntryState(
					StateText,
					Entry.State))
			{
				Result.Kind = EReadKind::ParseFailure;
				Result.Diagnostic = TEXT("ShopStock entry identity is malformed.");
				return Result;
			}
			Entry.SlotId = FName(*SlotText);
			Entry.SlotOrdinal = static_cast<int32>(OrdinalNumber);
			if (Entry.State
				== Edemo_mapPersistentShopStockEntryState::Available)
			{
				const TSharedPtr<FJsonObject>* ItemObject = nullptr;
				FString ItemError;
				FString IntegerError;
				if (!EntryObject->TryGetObjectField(
						TEXT("Item"), ItemObject)
					|| !JsonToItem(*ItemObject, Entry.Item, ItemError)
					|| !ParseCanonicalNonNegativeInt64(
						EntryObject,
						TEXT("QuotedBuyValue"),
						Entry.QuotedBuyValue,
						IntegerError))
				{
					Result.Kind = EReadKind::ParseFailure;
					Result.Diagnostic = ItemError.IsEmpty()
						? IntegerError
						: ItemError;
					return Result;
				}
			}
			else
			{
				FString SoldText;
				if (!EntryObject->TryGetStringField(
						TEXT("SoldItemInstanceId"), SoldText)
					|| !ParseGuid(
						SoldText,
						Entry.SoldItemInstanceId))
				{
					Result.Kind = EReadKind::ParseFailure;
					Result.Diagnostic =
						TEXT("ShopStock SOLD tombstone is malformed.");
					return Result;
				}
			}
			Profile.ShopStock.Entries.Add(MoveTemp(Entry));
		}
	}
	if (Schema >= 3)
	{
		const TSharedPtr<FJsonObject>* Layout = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* RunInventory = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Hotbar = nullptr;
		FString WeaponText, ArmorText, AccessoryText, SpatialRingText, BackpackText, LayoutError;
		if (!Root->TryGetObjectField(TEXT("PreparationLayout"), Layout)
			|| !(*Layout)->TryGetStringField(TEXT("WeaponItemInstanceId"), WeaponText)
			|| !(*Layout)->TryGetStringField(TEXT("ArmorItemInstanceId"), ArmorText)
			|| !(*Layout)->TryGetStringField(TEXT("AccessoryItemInstanceId"), AccessoryText)
			|| (Schema >= 6 && !(*Layout)->TryGetStringField(TEXT("SpatialRingItemInstanceId"), SpatialRingText))
			|| !(*Layout)->TryGetStringField(TEXT("BackpackItemInstanceId"), BackpackText)
			|| !(*Layout)->TryGetArrayField(TEXT("OrderedRunInventoryItemInstanceIds"), RunInventory)
			|| !(*Layout)->TryGetArrayField(TEXT("HotbarItemInstanceIds"), Hotbar)
			|| !ParseGuid(WeaponText, Profile.PreparationLayout.WeaponItemInstanceId)
			|| !ParseGuid(ArmorText, Profile.PreparationLayout.ArmorItemInstanceId)
			|| !ParseGuid(AccessoryText, Profile.PreparationLayout.AccessoryItemInstanceId)
			|| (Schema >= 6 && !ParseGuid(SpatialRingText, Profile.PreparationLayout.SpatialRingItemInstanceId))
			|| !ParseGuid(BackpackText, Profile.PreparationLayout.BackpackItemInstanceId)
			|| !JsonToGuids(RunInventory, Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds, TEXT("OrderedRunInventoryItemInstanceIds"), LayoutError)
			|| !JsonToGuids(Hotbar, Profile.PreparationLayout.HotbarItemInstanceIds, TEXT("HotbarItemInstanceIds"), LayoutError))
		{
			Result.Kind = EReadKind::ParseFailure;
			Result.Diagnostic = LayoutError.IsEmpty() ? TEXT("PreparationLayout is malformed.") : LayoutError;
			return Result;
		}
	}
	if (Schema >= 3
		&& Root->HasField(TEXT("WarehouseLayout")))
	{
		const TSharedPtr<FJsonObject>* WarehouseLayout = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* WarehouseSlots = nullptr;
		FString WarehouseError;
		if (!Root->TryGetObjectField(
				TEXT("WarehouseLayout"),
				WarehouseLayout)
			|| !(*WarehouseLayout)->TryGetArrayField(
				TEXT("SlotItemInstanceIds"),
				WarehouseSlots)
			|| !JsonToGuids(
				WarehouseSlots,
				Profile.WarehouseLayout.SlotItemInstanceIds,
				TEXT("Warehouse SlotItemInstanceIds"),
				WarehouseError))
		{
			Result.Kind = EReadKind::ParseFailure;
			Result.Diagnostic = WarehouseError.IsEmpty()
				? TEXT("WarehouseLayout is malformed.")
				: WarehouseError;
			return Result;
		}
		Profile.WarehouseLayout.bInitialized = true;
	}
	FString ActiveRunIdText, RunStateText, CommittedText;
	const TArray<TSharedPtr<FJsonValue>>* Deployed = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* RunItems = nullptr;
	if (!(*Run)->TryGetBoolField(TEXT("HasActiveRun"), Profile.ActiveRun.bHasActiveRun)
		|| !(*Run)->TryGetStringField(TEXT("ActiveRunId"), ActiveRunIdText)
		|| !(*Run)->TryGetStringField(TEXT("ActiveRunState"), RunStateText)
		|| !(*Run)->TryGetArrayField(TEXT("DeployedItemIds"), Deployed)
		|| !(*Run)->TryGetArrayField(TEXT("ActiveRunItems"), RunItems)
		|| !(*Run)->TryGetStringField(TEXT("CommittedSettlementId"), CommittedText)
		|| !ParseGuid(ActiveRunIdText, Profile.ActiveRun.ActiveRunId)
		|| !ParseGuid(CommittedText, Profile.ActiveRun.CommittedSettlementId)
		|| !ParseRunState(RunStateText, Profile.ActiveRun.ActiveRunState))
	{
		Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("ActiveRun record is malformed."); return Result;
	}
	if (Schema >= 2)
	{
		FString IntegerError;
		if (!ParseCanonicalNonNegativeInt64(*Run, TEXT("RiskSpiritStones"), Profile.ActiveRun.RiskSpiritStones, IntegerError))
		{
			Result.Kind = EReadKind::InvalidData; Result.Diagnostic = IntegerError; return Result;
		}
	}
	for (const TSharedPtr<FJsonValue>& Value : *Deployed)
	{
		FGuid Id;
		if (!Value.IsValid() || !ParseGuid(Value->AsString(), Id)) { Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("DeployedItemId is malformed."); return Result; }
		Profile.ActiveRun.DeployedItemIds.Add(Id);
	}
	for (const TSharedPtr<FJsonValue>& Value : *RunItems)
	{
		Fdemo_mapPersistentItemRecord Item;
		FString Error;
		if (!Value.IsValid() || !JsonToItem(Value->AsObject(), Item, Error)) { Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = Error; return Result; }
		Profile.ActiveRun.ActiveRunItems.Add(Item);
	}
	if (Schema >= 3)
	{
		const TArray<TSharedPtr<FJsonValue>>* ConsumedSources = nullptr;
		if (!(*Run)->TryGetArrayField(TEXT("ConsumedSpiritStoneSourceIds"), ConsumedSources))
		{
			Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("ConsumedSpiritStoneSourceIds is missing."); return Result;
		}
		for (const TSharedPtr<FJsonValue>& Value : *ConsumedSources)
		{
			if (!Value.IsValid())
			{
				Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("ConsumedSpiritStoneSourceIds contains an invalid value."); return Result;
			}
			const FString SourceText = Value->AsString();
			if (SourceText.IsEmpty())
			{
				Result.Kind = EReadKind::ParseFailure; Result.Diagnostic = TEXT("ConsumedSpiritStoneSourceIds contains an empty source."); return Result;
			}
			Profile.ActiveRun.ConsumedSpiritStoneSourceIds.Add(FName(*SourceText));
		}
	}
	FString ValidationError;
	if (Schema <= 5)
	{
		const Fdemo_mapPersistentProfile Promoted = PromoteLegacyProfile(Profile);
		if (!ValidateProfile(Promoted, &ValidationError)) { Result.Kind = EReadKind::InvalidData; Result.Diagnostic = ValidationError; return Result; }
		Result.Kind = EReadKind::LegacySchema;
	}
	else
	{
		if (!ValidateProfile(Profile, &ValidationError)) { Result.Kind = EReadKind::InvalidData; Result.Diagnostic = ValidationError; return Result; }
		Result.Kind = EReadKind::Valid;
	}
	Result.Profile = MoveTemp(Profile);
	Result.Diagnostic = Schema < Fdemo_mapPersistentProfile::CurrentSchemaVersion
		? FString::Printf(TEXT("Profile is a valid Schema %d migration source."), Schema)
		: TEXT("Profile is valid.");
	return Result;
}

Fdemo_mapProfileRepository::FReadResult Fdemo_mapProfileRepository::ReadProfile(const FString& Path) const
{
	FReadResult Result;
	if (!IFileManager::Get().FileExists(*Path)) { Result.Kind = EReadKind::Missing; Result.Diagnostic = TEXT("Profile file does not exist."); return Result; }
	if (!FFileHelper::LoadFileToArray(Result.Bytes, *Path)) { Result.Kind = EReadKind::ReadFailure; Result.Diagnostic = TEXT("Profile file could not be read."); return Result; }
	return DeserializeProfile(Result.Bytes);
}

bool Fdemo_mapProfileRepository::ProfilesMatchForCommit(const Fdemo_mapPersistentProfile& Expected, const Fdemo_mapPersistentProfile& Actual) const
{
	return ExactCoreProfileMatch(Expected, Actual);
}

bool Fdemo_mapProfileRepository::ShouldFail(const Fdemo_mapProfileStorageContext& Storage, Edemo_mapProfileFailureStage Stage) const
{
#if WITH_DEV_AUTOMATION_TESTS
	return Storage.InjectedFailure == Stage;
#else
	return false;
#endif
}

Fdemo_mapProfileSaveResult Fdemo_mapProfileRepository::SaveProfile(Fdemo_mapPersistentProfile& Profile, const Fdemo_mapProfileStorageContext& Storage) const
{
	Fdemo_mapProfileSaveResult Result;
	Result.PrimaryPath = Storage.PrimaryPath(); Result.BackupPath = Storage.BackupPath(); Result.TempPath = Storage.TempPath();
	FString Error;
	if (!ValidateProfile(Profile, &Error)) { Result.Status = Edemo_mapProfileSaveStatus::ValidationRejected; Result.Diagnostic = Error; return Result; }
	if (Profile.SaveGeneration == MAX_int32) { Result.Status = Edemo_mapProfileSaveStatus::ValidationRejected; Result.Diagnostic = TEXT("SaveGeneration cannot be incremented."); return Result; }
	Fdemo_mapPersistentProfile Candidate = Profile;
	Candidate.SaveGeneration++;
	Candidate.ProfileMetadata.LastSavedUtc = FDateTime::UtcNow().ToIso8601();
	if (!ValidateProfile(Candidate, &Error)) { Result.Status = Edemo_mapProfileSaveStatus::ValidationRejected; Result.Diagnostic = Error; return Result; }
	TArray<uint8> Bytes;
	if (!SerializeProfile(Candidate, Bytes, Error)) { Result.Status = Edemo_mapProfileSaveStatus::SerializationFailed; Result.Diagnostic = Error; return Result; }
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::CreateDirectory) || (!IFileManager::Get().DirectoryExists(*Storage.RootDirectory) && !IFileManager::Get().MakeDirectory(*Storage.RootDirectory, true)))
	{
		Result.Status = Edemo_mapProfileSaveStatus::TempWriteFailed; Result.Diagnostic = TEXT("Storage directory could not be created."); return Result;
	}
	if (IFileManager::Get().FileExists(*Result.TempPath)) IFileManager::Get().Delete(*Result.TempPath, false, true, true);
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::WriteTemp)) { Result.Status = Edemo_mapProfileSaveStatus::TempWriteFailed; Result.Diagnostic = TEXT("Injected temporary write failure."); return Result; }
	TUniquePtr<IFileHandle> Handle(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*Result.TempPath, false, false));
	if (!Handle || !Handle->Write(Bytes.GetData(), Bytes.Num())) { Result.Status = Edemo_mapProfileSaveStatus::TempWriteFailed; Result.Diagnostic = TEXT("Temporary profile write failed."); return Result; }
	Result.bDiskStateChanged = true;
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::FlushOrCloseTemp)) { Handle.Reset(); Result.Status = Edemo_mapProfileSaveStatus::TempFlushOrCloseFailed; Result.Diagnostic = TEXT("Injected temporary flush/close failure."); return Result; }
	if (!Handle->Flush(true)) { Handle.Reset(); Result.Status = Edemo_mapProfileSaveStatus::TempFlushOrCloseFailed; Result.Diagnostic = TEXT("Temporary profile full flush failed."); return Result; }
	Handle.Reset();
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::ReadBackTemp)) { Result.Status = Edemo_mapProfileSaveStatus::TempVerificationFailed; Result.Diagnostic = TEXT("Injected temporary read-back failure."); return Result; }
	FReadResult TempRead = ReadProfile(Result.TempPath);
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::ValidateTemp) || TempRead.Kind != EReadKind::Valid || !ProfilesMatchForCommit(Candidate, TempRead.Profile))
	{
		Result.Status = Edemo_mapProfileSaveStatus::TempVerificationFailed; Result.Diagnostic = TEXT("Temporary profile verification failed."); return Result;
	}

	if (IFileManager::Get().FileExists(*Result.PrimaryPath))
	{
		const FReadResult PrimaryRead = ReadProfile(Result.PrimaryPath);
		const bool bCurrentPrimaryMatches = PrimaryRead.Kind == EReadKind::Valid
			&& PrimaryRead.Profile.ProfileId == Profile.ProfileId
			&& PrimaryRead.Profile.SaveGeneration == Profile.SaveGeneration;
		const bool bLegacyMigrationSourceMatches = PrimaryRead.Kind == EReadKind::LegacySchema
			&& IsExactLegacySourceFor(Profile, PrimaryRead.Profile);
		if (!bCurrentPrimaryMatches && !bLegacyMigrationSourceMatches)
		{
			Result.Status = Edemo_mapProfileSaveStatus::BackupPreparationFailed; Result.Diagnostic = TEXT("Existing primary is invalid or does not match the caller generation."); return Result;
		}
		if (ShouldFail(Storage, Edemo_mapProfileFailureStage::PrepareBackup)) { Result.Status = Edemo_mapProfileSaveStatus::BackupPreparationFailed; Result.Diagnostic = TEXT("Injected backup preparation failure."); return Result; }
		if (IFileManager::Get().Copy(*Result.BackupPath, *Result.PrimaryPath, true, true) != COPY_OK)
		{
			Result.Status = Edemo_mapProfileSaveStatus::BackupPreparationFailed; Result.Diagnostic = TEXT("Primary could not be copied to backup."); return Result;
		}
		const FReadResult BackupRead = ReadProfile(Result.BackupPath);
		if (BackupRead.Kind != PrimaryRead.Kind || BackupRead.Bytes != PrimaryRead.Bytes)
		{
			Result.Status = Edemo_mapProfileSaveStatus::BackupPreparationFailed; Result.Diagnostic = TEXT("Backup verification failed."); return Result;
		}
	}
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::AtomicReplace)) { Result.Status = Edemo_mapProfileSaveStatus::AtomicReplaceFailed; Result.Diagnostic = TEXT("Injected atomic replace failure."); return Result; }
	if (!IFileManager::Get().Move(*Result.PrimaryPath, *Result.TempPath, true, false, true, true))
	{
		Result.Status = Edemo_mapProfileSaveStatus::AtomicReplaceFailed; Result.Diagnostic = TEXT("Same-volume primary replacement failed."); return Result;
	}
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::ReadBackCommittedPrimary))
	{
		Result.Status = Edemo_mapProfileSaveStatus::PostCommitVerificationFailed; Result.Diagnostic = TEXT("Injected committed-primary read-back failure; caller generation was not advanced."); return Result;
	}
	const FReadResult Committed = ReadProfile(Result.PrimaryPath);
	if (Committed.Kind != EReadKind::Valid || !ProfilesMatchForCommit(Candidate, Committed.Profile))
	{
		Result.Status = Edemo_mapProfileSaveStatus::PostCommitVerificationFailed; Result.Diagnostic = TEXT("Committed primary verification failed; verified backup remains available when a prior primary existed."); return Result;
	}
	Profile = Candidate;
	Result.Status = Edemo_mapProfileSaveStatus::Saved;
	Result.Diagnostic = TEXT("Profile committed and verified.");
	Result.CommittedGeneration = Candidate.SaveGeneration;
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::CleanupTemp))
	{
		Result.bCleanupSucceeded = false;
		Result.Diagnostic += TEXT(" Injected cleanup warning after the rename consumed the temporary file.");
	}
	else if (IFileManager::Get().FileExists(*Result.TempPath))
	{
		Result.bCleanupSucceeded = IFileManager::Get().Delete(*Result.TempPath, false, true, true);
	}
	return Result;
}

FString Fdemo_mapProfileRepository::PreserveCorruptPrimary(const Fdemo_mapProfileStorageContext& Storage, const TArray<uint8>& Bytes, FString& OutError) const
{
	if (!IFileManager::Get().DirectoryExists(*Storage.CorruptDirectory()) && !IFileManager::Get().MakeDirectory(*Storage.CorruptDirectory(), true))
	{
		OutError = TEXT("Corrupt preservation directory could not be created."); return FString();
	}
	const FString Signature = Sha256Hex(Bytes);
	const FString Name = FString::Printf(TEXT("Profile_Default_%s_%s.corrupt.json"), *Signature, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	const FString Path = FPaths::Combine(Storage.CorruptDirectory(), Name);
	if (!FFileHelper::SaveArrayToFile(Bytes, *Path)) { OutError = TEXT("Corrupt primary bytes could not be preserved."); return FString(); }
	TArray<uint8> Verified;
	if (!FFileHelper::LoadFileToArray(Verified, *Path) || Verified != Bytes) { OutError = TEXT("Preserved corrupt bytes did not verify."); return FString(); }
	return Path;
}

Fdemo_mapProfileLoadResult Fdemo_mapProfileRepository::RecoverFromBackup(const Fdemo_mapProfileStorageContext& Storage, const FReadResult& Backup, Edemo_mapProfileLoadStatus SuccessStatus, const FString& QuarantinedPath) const
{
	Fdemo_mapProfileLoadResult Result;
	Result.PrimaryPath = Storage.PrimaryPath(); Result.BackupPath = Storage.BackupPath(); Result.TempPath = Storage.TempPath(); Result.QuarantinedPath = QuarantinedPath;
	if (Backup.Kind != EReadKind::Valid) { Result.Status = Edemo_mapProfileLoadStatus::WriteRecoveryFailed; Result.Diagnostic = TEXT("Backup is not valid for recovery."); return Result; }
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::WriteTemp) || !FFileHelper::SaveArrayToFile(Backup.Bytes, *Result.TempPath))
	{
		Result.Status = Edemo_mapProfileLoadStatus::WriteRecoveryFailed; Result.Diagnostic = TEXT("Recovery temporary write failed."); return Result;
	}
	Result.bDiskStateChanged = true;
	const FReadResult Temp = ReadProfile(Result.TempPath);
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::ReadBackTemp) || ShouldFail(Storage, Edemo_mapProfileFailureStage::ValidateTemp) || Temp.Kind != EReadKind::Valid || !ProfilesMatchForCommit(Backup.Profile, Temp.Profile))
	{
		Result.Status = Edemo_mapProfileLoadStatus::WriteRecoveryFailed; Result.Diagnostic = TEXT("Recovery temporary verification failed."); return Result;
	}
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::AtomicReplace) || !IFileManager::Get().Move(*Result.PrimaryPath, *Result.TempPath, true, false, true, true))
	{
		Result.Status = Edemo_mapProfileLoadStatus::WriteRecoveryFailed; Result.Diagnostic = TEXT("Recovery primary replacement failed."); return Result;
	}
	const FReadResult Primary = ReadProfile(Result.PrimaryPath);
	if (ShouldFail(Storage, Edemo_mapProfileFailureStage::ReadBackCommittedPrimary) || Primary.Kind != EReadKind::Valid || !ProfilesMatchForCommit(Backup.Profile, Primary.Profile))
	{
		Result.Status = Edemo_mapProfileLoadStatus::WriteRecoveryFailed; Result.Diagnostic = TEXT("Recovered primary verification failed."); return Result;
	}
	Result.Status = SuccessStatus;
	Result.Diagnostic = TEXT("Valid backup restored to primary without changing its generation.");
	Result.Profile = Primary.Profile;
	Result.CommittedGeneration = Primary.Profile.SaveGeneration;
	return Result;
}

Fdemo_mapProfileLoadResult Fdemo_mapProfileRepository::LoadExistingProfile(const Fdemo_mapProfileStorageContext& Storage) const
{
	Fdemo_mapProfileLoadResult Result;
	Result.PrimaryPath = Storage.PrimaryPath(); Result.BackupPath = Storage.BackupPath(); Result.TempPath = Storage.TempPath();
	const FReadResult Primary = ReadProfile(Result.PrimaryPath);
	if (Primary.Kind == EReadKind::Valid)
	{
		Result.Status = Edemo_mapProfileLoadStatus::LoadedPrimary; Result.Diagnostic = TEXT("Primary profile loaded and validated without a write."); Result.Profile = Primary.Profile; Result.CommittedGeneration = Primary.Profile.SaveGeneration; return Result;
	}
	if (Primary.Kind == EReadKind::LegacySchema)
	{
		const int32 SourceSchema = Primary.Profile.SchemaVersion;
		Fdemo_mapPersistentProfile Migrated = PromoteLegacyProfile(Primary.Profile);
		const Fdemo_mapProfileSaveResult Save = SaveProfile(Migrated, Storage);
		Result.bDiskStateChanged = Save.bDiskStateChanged;
		Result.CommittedGeneration = Save.CommittedGeneration;
		if (!Save.IsSuccess())
		{
			Result.Status = Edemo_mapProfileLoadStatus::WriteRecoveryFailed;
			Result.Diagnostic = FString::Printf(TEXT("Schema %d migration did not commit: %s"), SourceSchema, *Save.Diagnostic);
			return Result;
		}
		Result.Status = Edemo_mapProfileLoadStatus::LoadedPrimary;
		Result.Diagnostic = FString::Printf(
			TEXT("Schema %d profile atomically migrated to the current nested-container schema; ItemInstance GUIDs, carried ordering, quantities, and existing bag contents were preserved."),
			SourceSchema);
		Result.Profile = MoveTemp(Migrated);
		return Result;
	}
	if (Primary.Kind == EReadKind::FutureSchema)
	{
		Result.Status = Edemo_mapProfileLoadStatus::FutureSchemaRejected; Result.Diagnostic = Primary.Diagnostic; return Result;
	}
	const FReadResult Backup = ReadProfile(Result.BackupPath);
	if (Primary.Kind == EReadKind::Missing)
	{
		if (Backup.Kind == EReadKind::Valid) return RecoverFromBackup(Storage, Backup, Edemo_mapProfileLoadStatus::PrimaryMissingBackupRecovered, FString());
		Result.Status = Backup.Kind == EReadKind::ReadFailure ? Edemo_mapProfileLoadStatus::ReadFailed : Edemo_mapProfileLoadStatus::InvalidProfileData;
		Result.Diagnostic = TEXT("Primary is missing and no valid backup exists."); return Result;
	}
	if (Primary.Kind == EReadKind::ReadFailure)
	{
		Result.Status = Edemo_mapProfileLoadStatus::ReadFailed; Result.Diagnostic = Primary.Diagnostic; return Result;
	}
	if (Backup.Kind != EReadKind::Valid)
	{
		Result.Status = Edemo_mapProfileLoadStatus::CorruptPrimaryNoValidBackup; Result.Diagnostic = TEXT("Primary is corrupt or invalid and no valid backup exists; original files were left untouched."); return Result;
	}
	FString PreserveError;
	const FString PreservedPath = PreserveCorruptPrimary(Storage, Primary.Bytes, PreserveError);
	if (PreservedPath.IsEmpty())
	{
		Result.Status = Edemo_mapProfileLoadStatus::WriteRecoveryFailed; Result.Diagnostic = PreserveError; return Result;
	}
	return RecoverFromBackup(Storage, Backup, Edemo_mapProfileLoadStatus::RecoveredFromBackup, PreservedPath);
}

Fdemo_mapProfileLoadResult Fdemo_mapProfileRepository::LoadOrCreateDefaultProfile(const Fdemo_mapProfileStorageContext& Storage) const
{
	const bool bPrimaryExists = IFileManager::Get().FileExists(*Storage.PrimaryPath());
	const bool bBackupExists = IFileManager::Get().FileExists(*Storage.BackupPath());
	if (bPrimaryExists || bBackupExists) return LoadExistingProfile(Storage);
	Fdemo_mapPersistentProfile Fresh = CreateFreshProfile();
	const Fdemo_mapProfileSaveResult Save = SaveProfile(Fresh, Storage);
	Fdemo_mapProfileLoadResult Result;
	Result.PrimaryPath = Storage.PrimaryPath(); Result.BackupPath = Storage.BackupPath(); Result.TempPath = Storage.TempPath(); Result.bDiskStateChanged = Save.bDiskStateChanged; Result.CommittedGeneration = Save.CommittedGeneration;
	if (!Save.IsSuccess()) { Result.Status = Edemo_mapProfileLoadStatus::WriteRecoveryFailed; Result.Diagnostic = Save.Diagnostic; return Result; }
	Result.Status = Edemo_mapProfileLoadStatus::CreatedFreshAndCommitted;
	Result.Diagnostic = TEXT("Primary and backup were absent; a fresh Profile was atomically committed.");
	Result.Profile = Fresh;
	return Result;
}
