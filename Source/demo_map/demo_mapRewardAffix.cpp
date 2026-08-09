#include "demo_mapRewardAffix.h"

#include "demo_mapItemDefinitions.h"

const FName Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId(
	TEXT("Reward.Affix.Equipment.Default"));
const FName Fdemo_mapRewardAffixPolicyRegistry::PityPolicyId(
	TEXT("Reward.AffixPity.WeaponHighTier.Default"));
const FName Fdemo_mapRewardAffixPolicyRegistry::PityChannelId(
	TEXT("Reward.Pity.Weapon.PowerTier3"));

namespace
{
	uint64 HashUtf8(const FString& Text)
	{
		const FTCHARToUTF8 Utf8(*Text);
		uint64 Hash = 1469598103934665603ull;
		for (int32 Index = 0; Index < Utf8.Length(); ++Index)
		{
			Hash ^= static_cast<uint8>(Utf8.Get()[Index]);
			Hash *= 1099511628211ull;
		}
		return Hash == 0 ? 1 : Hash;
	}

	uint64 NextRandom(uint64& State)
	{
		uint64 X = State == 0 ? 0x9E3779B97F4A7C15ull : State;
		X ^= X >> 12;
		X ^= X << 25;
		X ^= X >> 27;
		State = X;
		return X * 2685821657736338717ull;
	}

	int32 RollBps(const FString& Identity)
	{
		uint64 State = HashUtf8(Identity);
		return static_cast<int32>(NextRandom(State) % 10000ull);
	}

	FGuid StableEventId(const FString& Identity)
	{
		const uint64 A = HashUtf8(Identity + TEXT("|AffixEventId.A"));
		const uint64 B = HashUtf8(Identity + TEXT("|AffixEventId.B"));
		FGuid Result(
			static_cast<uint32>(A >> 32),
			static_cast<uint32>(A),
			static_cast<uint32>(B >> 32),
			static_cast<uint32>(B));
		if (!Result.IsValid())
		{
			Result.D = 1;
		}
		return Result;
	}

	FName CategoryForStack(const Fdemo_mapRewardPlannedStack& Stack)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Stack.DefinitionId);
		return Definition ? Definition->CategoryId : NAME_None;
	}

	bool IsEligibleCategory(FName Category)
	{
		return Category == Fdemo_mapItemIds::WeaponCategory
			|| Category == Fdemo_mapItemIds::ArmorCategory
			|| Category == Fdemo_mapItemIds::SpatialRingCategory;
	}

	int32 MaxAffixes(FName Category)
	{
		return Category == Fdemo_mapItemIds::ArmorCategory ? 2
			: IsEligibleCategory(Category) ? 1 : 0;
	}

	int32 SelectCount(FName Category, int32 Roll)
	{
		if (Category == Fdemo_mapItemIds::ArmorCategory)
		{
			return Roll < 5000 ? 0 : Roll < 9000 ? 1 : 2;
		}
		return Roll < 6000 ? 0 : 1;
	}

	Edemo_mapRewardAffixTier SelectTier(int32 Roll, int32 PityState)
	{
		if (PityState >= 3)
		{
			return Edemo_mapRewardAffixTier::Tier3;
		}
		const int32 Tier3Weight = PityState == 2 ? 2500
			: PityState == 1 ? 1500 : 500;
		const int32 Tier1Weight = PityState == 2 ? 5000
			: PityState == 1 ? 6000 : 7000;
		return Roll < Tier1Weight ? Edemo_mapRewardAffixTier::Tier1
			: Roll < Tier1Weight + 2500
				? Edemo_mapRewardAffixTier::Tier2
				: (Tier3Weight > 0
					? Edemo_mapRewardAffixTier::Tier3
					: Edemo_mapRewardAffixTier::Tier2);
	}

	const Fdemo_mapRewardAffixDescriptor* FindDescriptor(
		FName Category,
		FName Group,
		Edemo_mapRewardAffixTier Tier)
	{
		return Fdemo_mapRewardAffixPolicyRegistry::GetAll().FindByPredicate(
			[Category, Group, Tier](const auto& Descriptor)
			{
				return Descriptor.RequiredCategoryId == Category
					&& Descriptor.CompatibilityGroup == Group
					&& Descriptor.Tier == Tier;
			});
	}

	FString Identity(
		FGuid RunId,
		FName SourceRole,
		FName ProjectionId,
		int32 Ordinal,
		const TCHAR* Domain)
	{
		return FString::Printf(
			TEXT("%s|%s|%s|%d|%s"),
			*RunId.ToString(EGuidFormats::Digits),
			*SourceRole.ToString(),
			*ProjectionId.ToString(),
			Ordinal,
			Domain);
	}

	FString LedgerKey(FGuid RunId, FName ChannelId)
	{
		return FString::Printf(
			TEXT("%s|%s"),
			*RunId.ToString(EGuidFormats::Digits),
			*ChannelId.ToString());
	}
}

bool Fdemo_mapRewardAffixDescriptor::IsValid() const
{
	return !AffixId.IsNone()
		&& !CompatibilityGroup.IsNone()
		&& !RequiredCategoryId.IsNone()
		&& Tier != Edemo_mapRewardAffixTier::None
		&& MagnitudeScaled != 0
		&& ResolvedValue > 0
		&& !DisplayLabel.IsEmpty();
}

const Fdemo_mapRewardAffixPolicy&
Fdemo_mapRewardAffixPolicyRegistry::GetDefault()
{
	static const Fdemo_mapRewardAffixPolicy Policy{ DefaultPolicyId };
	return Policy;
}

const TArray<Fdemo_mapRewardAffixDescriptor>&
Fdemo_mapRewardAffixPolicyRegistry::GetAll()
{
	static const TArray<Fdemo_mapRewardAffixDescriptor> Descriptors = {
		{ TEXT("Reward.Affix.Weapon.Power.T1"), TEXT("Reward.AffixGroup.Weapon.Power"), Fdemo_mapItemIds::WeaponCategory, Edemo_mapRewardAffixTier::Tier1, Edemo_mapRewardAffixEffect::AttackPower, 2, 40, TEXT("POWER I +2 ATK") },
		{ TEXT("Reward.Affix.Weapon.Power.T2"), TEXT("Reward.AffixGroup.Weapon.Power"), Fdemo_mapItemIds::WeaponCategory, Edemo_mapRewardAffixTier::Tier2, Edemo_mapRewardAffixEffect::AttackPower, 5, 120, TEXT("POWER II +5 ATK") },
		{ TEXT("Reward.Affix.Weapon.Power.T3"), TEXT("Reward.AffixGroup.Weapon.Power"), Fdemo_mapItemIds::WeaponCategory, Edemo_mapRewardAffixTier::Tier3, Edemo_mapRewardAffixEffect::AttackPower, 12, 360, TEXT("POWER III +12 ATK") },
		{ TEXT("Reward.Affix.Robe.Vitality.T1"), TEXT("Reward.AffixGroup.Robe.Vitality"), Fdemo_mapItemIds::ArmorCategory, Edemo_mapRewardAffixTier::Tier1, Edemo_mapRewardAffixEffect::MaxHealth, 4, 40, TEXT("VITALITY I +4 HP") },
		{ TEXT("Reward.Affix.Robe.Vitality.T2"), TEXT("Reward.AffixGroup.Robe.Vitality"), Fdemo_mapItemIds::ArmorCategory, Edemo_mapRewardAffixTier::Tier2, Edemo_mapRewardAffixEffect::MaxHealth, 10, 120, TEXT("VITALITY II +10 HP") },
		{ TEXT("Reward.Affix.Robe.Vitality.T3"), TEXT("Reward.AffixGroup.Robe.Vitality"), Fdemo_mapItemIds::ArmorCategory, Edemo_mapRewardAffixTier::Tier3, Edemo_mapRewardAffixEffect::MaxHealth, 25, 360, TEXT("VITALITY III +25 HP") },
		{ TEXT("Reward.Affix.Robe.Guard.T1"), TEXT("Reward.AffixGroup.Robe.Guard"), Fdemo_mapItemIds::ArmorCategory, Edemo_mapRewardAffixTier::Tier1, Edemo_mapRewardAffixEffect::FlatDamageReduction, 1, 50, TEXT("GUARD I +1 DR") },
		{ TEXT("Reward.Affix.Robe.Guard.T2"), TEXT("Reward.AffixGroup.Robe.Guard"), Fdemo_mapItemIds::ArmorCategory, Edemo_mapRewardAffixTier::Tier2, Edemo_mapRewardAffixEffect::FlatDamageReduction, 2, 150, TEXT("GUARD II +2 DR") },
		{ TEXT("Reward.Affix.Robe.Guard.T3"), TEXT("Reward.AffixGroup.Robe.Guard"), Fdemo_mapItemIds::ArmorCategory, Edemo_mapRewardAffixTier::Tier3, Edemo_mapRewardAffixEffect::FlatDamageReduction, 4, 450, TEXT("GUARD III +4 DR") },
		{ TEXT("Reward.Affix.Accessory.Haste.T1"), TEXT("Reward.AffixGroup.Accessory.Haste"), Fdemo_mapItemIds::SpatialRingCategory, Edemo_mapRewardAffixTier::Tier1, Edemo_mapRewardAffixEffect::CooldownMultiplierDeltaBps, -250, 50, TEXT("HASTE I -2.5% CD") },
		{ TEXT("Reward.Affix.Accessory.Haste.T2"), TEXT("Reward.AffixGroup.Accessory.Haste"), Fdemo_mapItemIds::SpatialRingCategory, Edemo_mapRewardAffixTier::Tier2, Edemo_mapRewardAffixEffect::CooldownMultiplierDeltaBps, -500, 150, TEXT("HASTE II -5% CD") },
		{ TEXT("Reward.Affix.Accessory.Haste.T3"), TEXT("Reward.AffixGroup.Accessory.Haste"), Fdemo_mapItemIds::SpatialRingCategory, Edemo_mapRewardAffixTier::Tier3, Edemo_mapRewardAffixEffect::CooldownMultiplierDeltaBps, -1000, 450, TEXT("HASTE III -10% CD") }
	};
	return Descriptors;
}

const Fdemo_mapRewardAffixDescriptor*
Fdemo_mapRewardAffixPolicyRegistry::Find(FName AffixId)
{
	return GetAll().FindByPredicate(
		[AffixId](const auto& Descriptor)
		{
			return Descriptor.AffixId == AffixId;
		});
}

bool Fdemo_mapRewardAffixPolicyRegistry::Validate(FString* OutError)
{
	TSet<FName> Ids;
	TSet<FString> CategoryGroupTier;
	for (const auto& Descriptor : GetAll())
	{
		const FString Key = FString::Printf(
			TEXT("%s|%s|%d"),
			*Descriptor.RequiredCategoryId.ToString(),
			*Descriptor.CompatibilityGroup.ToString(),
			static_cast<int32>(Descriptor.Tier));
		if (!Descriptor.IsValid()
			|| Ids.Contains(Descriptor.AffixId)
			|| CategoryGroupTier.Contains(Key))
		{
			if (OutError)
			{
				*OutError = TEXT("Affix registry contains invalid or duplicate descriptor metadata.");
			}
			return false;
		}
		Ids.Add(Descriptor.AffixId);
		CategoryGroupTier.Add(Key);
	}
	if (GetAll().Num() != 12)
	{
		if (OutError) *OutError = TEXT("P5 requires exactly twelve Affix descriptors.");
		return false;
	}
	return true;
}

bool Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
	FName DefinitionId,
	int32 Quantity,
	const Fdemo_mapRewardAffixSet& Set,
	FString* OutError)
{
	auto Fail = [OutError](const TCHAR* Message)
	{
		if (OutError) *OutError = Message;
		return false;
	};
	if (Set.IsEmpty())
	{
		return true;
	}
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(DefinitionId);
	if (!Definition || Definition->MaxStackSize != 1 || Quantity != 1
		|| !IsEligibleCategory(Definition->CategoryId)
		|| !Set.AffixSetEventId.IsValid()
		|| Set.AffixPolicyId != DefaultPolicyId
		|| Set.Acquisition == Edemo_mapRewardAffixAcquisition::None
		|| Set.Affixes.IsEmpty()
		|| Set.Affixes.Num() > MaxAffixes(Definition->CategoryId))
	{
		return Fail(TEXT("Affix set identity, equipment category, or stack metadata is invalid."));
	}
	TSet<FName> Ids;
	TSet<FName> Groups;
	FString PreviousKey;
	for (const Fdemo_mapResolvedRewardAffix& Affix : Set.Affixes)
	{
		const Fdemo_mapRewardAffixDescriptor* Descriptor = Find(Affix.AffixId);
		if (!Descriptor
			|| Descriptor->RequiredCategoryId != Definition->CategoryId
			|| Descriptor->Tier != Affix.Tier
			|| Descriptor->MagnitudeScaled != Affix.ResolvedMagnitudeScaled
			|| Descriptor->ResolvedValue != Affix.ResolvedValue
			|| Ids.Contains(Affix.AffixId)
			|| Groups.Contains(Descriptor->CompatibilityGroup))
		{
			return Fail(TEXT("Affix set descriptor, category, tier, magnitude, value, or compatibility group is invalid."));
		}
		const FString Key = Descriptor->CompatibilityGroup.ToString()
			+ TEXT("|") + Descriptor->AffixId.ToString();
		if (!PreviousKey.IsEmpty() && Key < PreviousKey)
		{
			return Fail(TEXT("Affix set is not in canonical compatibility-group and AffixId order."));
		}
		PreviousKey = Key;
		Ids.Add(Affix.AffixId);
		Groups.Add(Descriptor->CompatibilityGroup);
	}
	return Set.TotalResolvedValue() >= 0;
}

FString Fdemo_mapRewardAffixPolicyRegistry::BuildDisplayLabel(
	const Fdemo_mapRewardAffixSet& Set)
{
	TArray<FString> Labels;
	for (const auto& Affix : Set.Affixes)
	{
		if (const auto* Descriptor = Find(Affix.AffixId))
		{
			Labels.Add(Descriptor->DisplayLabel);
		}
	}
	return FString::Join(Labels, TEXT(" | "));
}

bool Fdemo_mapRewardAffixPlanner::Apply(
	const Fdemo_mapRewardAffixPolicy& Policy,
	FGuid RunId,
	FName StableSourceRoleId,
	FName ProjectionId,
	int64 AvailableBudget,
	int32 PityStateIn,
	TArray<Fdemo_mapRewardPlannedStack>& InOutStacks,
	Fdemo_mapRewardAffixPlanTrace& OutTrace)
{
	OutTrace = {};
	OutTrace.PolicyId = Policy.PolicyId;
	OutTrace.BudgetBefore = AvailableBudget;
	OutTrace.BudgetAfter = AvailableBudget;
	OutTrace.PityStateIn = PityStateIn;
	OutTrace.PityStateOut = PityStateIn;
	if (!Policy.IsValid() || Policy.PolicyId !=
			Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId
		|| !RunId.IsValid() || StableSourceRoleId.IsNone()
		|| ProjectionId.IsNone() || AvailableBudget < 0
		|| PityStateIn < 0 || PityStateIn > 3)
	{
		OutTrace.Diagnostic = TEXT("invalid_affix_plan_request");
		return false;
	}

	int32 CurrentPity = PityStateIn;
	int32 EquipmentOrdinal = 0;
	for (Fdemo_mapRewardPlannedStack& Stack : InOutStacks)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Stack.DefinitionId);
		const FName Category = CategoryForStack(Stack);
		if (!Definition || Definition->MaxStackSize != 1
			|| Stack.StackCount != 1 || !IsEligibleCategory(Category))
		{
			continue;
		}
		++OutTrace.EligibleEquipmentCount;
		const int32 Ordinal = EquipmentOrdinal++;
		const FString BaseIdentity =
			Identity(RunId, StableSourceRoleId, ProjectionId, Ordinal, TEXT(""));
		int32 Count = SelectCount(
			Category,
			RollBps(BaseIdentity + TEXT("AffixCount")));
		const bool bWeapon = Category == Fdemo_mapItemIds::WeaponCategory;
		Fdemo_mapRewardAffixPityDecision Pity;
		Pity.StateIn = CurrentPity;
		Pity.StateOut = CurrentPity;
		Pity.bEligibleWeaponAttempt = bWeapon;
		if (bWeapon && CurrentPity >= 3)
		{
			Count = 1;
		}
		if (Count <= 0)
		{
			if (bWeapon)
			{
				Pity.Reason = TEXT("no_affix_candidate");
				OutTrace.PityDecisions.Add(Pity);
			}
			continue;
		}

		TArray<FName> Groups;
		if (Category == Fdemo_mapItemIds::WeaponCategory)
		{
			Groups = { TEXT("Reward.AffixGroup.Weapon.Power") };
		}
		else if (Category == Fdemo_mapItemIds::SpatialRingCategory)
		{
			Groups = { TEXT("Reward.AffixGroup.Accessory.Haste") };
		}
		else
		{
			const bool bGuardFirst =
				RollBps(BaseIdentity + TEXT("AffixDescriptor.0.0")) >= 5000;
			Groups = bGuardFirst
				? TArray<FName>{ TEXT("Reward.AffixGroup.Robe.Guard"), TEXT("Reward.AffixGroup.Robe.Vitality") }
				: TArray<FName>{ TEXT("Reward.AffixGroup.Robe.Vitality"), TEXT("Reward.AffixGroup.Robe.Guard") };
		}

		Fdemo_mapRewardAffixSet Set;
		Set.AffixPolicyId = Policy.PolicyId;
		Set.Acquisition = Edemo_mapRewardAffixAcquisition::Natural;
		for (int32 Slot = 0;
			Slot < FMath::Min(Count, Groups.Num());
			++Slot)
		{
			const int32 TierRoll = RollBps(BaseIdentity
				+ FString::Printf(TEXT("AffixTier.%d.%d"), Ordinal, Slot));
			if (bWeapon)
			{
				Pity.TierRoll = TierRoll;
			}
			const Edemo_mapRewardAffixTier Tier =
				bWeapon ? SelectTier(TierRoll, CurrentPity)
					: SelectTier(TierRoll, 0);
			const auto* Descriptor = FindDescriptor(
				Category,
				Groups[Slot],
				Tier);
			if (!Descriptor)
			{
				OutTrace.Diagnostic = TEXT("affix_descriptor_missing");
				return false;
			}
			if (Descriptor->ResolvedValue > OutTrace.BudgetAfter)
			{
				if (bWeapon)
				{
					Pity.Reason = CurrentPity >= 3
						? TEXT("guarantee_budget_insufficient_hold_state3")
						: TEXT("budget_insufficient_no_state_change");
				}
				break;
			}
			Fdemo_mapResolvedRewardAffix Resolved;
			Resolved.AffixId = Descriptor->AffixId;
			Resolved.Tier = Descriptor->Tier;
			Resolved.ResolvedMagnitudeScaled = Descriptor->MagnitudeScaled;
			Resolved.ResolvedValue = Descriptor->ResolvedValue;
			Set.Affixes.Add(Resolved);
			OutTrace.BudgetAfter -= Descriptor->ResolvedValue;
			OutTrace.AffixValueSpent += Descriptor->ResolvedValue;
			if (bWeapon)
			{
				Pity.bQualifyingTier3 =
					Tier == Edemo_mapRewardAffixTier::Tier3;
				Pity.bGuaranteeApplied =
					CurrentPity >= 3 && Pity.bQualifyingTier3;
				Pity.StateOut = Pity.bQualifyingTier3
					? 0 : FMath::Min(3, CurrentPity + 1);
				Pity.bCommitRequired = true;
				Pity.Reason = Pity.bGuaranteeApplied
					? TEXT("tier3_pity_guaranteed_reset")
					: Pity.bQualifyingTier3
						? TEXT("natural_tier3_reset")
						: TEXT("eligible_weapon_failure_increment");
				CurrentPity = Pity.StateOut;
				OutTrace.PityStateOut = CurrentPity;
			}
		}
		if (Set.Affixes.IsEmpty())
		{
			if (bWeapon)
			{
				OutTrace.PityDecisions.Add(Pity);
			}
			continue;
		}
		Set.Affixes.Sort([](const auto& A, const auto& B)
		{
			const auto* DA = Fdemo_mapRewardAffixPolicyRegistry::Find(A.AffixId);
			const auto* DB = Fdemo_mapRewardAffixPolicyRegistry::Find(B.AffixId);
			const FString KA = DA
				? DA->CompatibilityGroup.ToString() + TEXT("|") + DA->AffixId.ToString()
				: A.AffixId.ToString();
			const FString KB = DB
				? DB->CompatibilityGroup.ToString() + TEXT("|") + DB->AffixId.ToString()
				: B.AffixId.ToString();
			return KA < KB;
		});
		Set.AffixSetEventId = StableEventId(
			BaseIdentity + TEXT("AffixEventId"));
		if (bWeapon && Pity.bGuaranteeApplied)
		{
			Set.Acquisition =
				Edemo_mapRewardAffixAcquisition::PityGuaranteed;
		}
		if (!Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
			Stack.DefinitionId,
			Stack.StackCount,
			Set))
		{
			OutTrace.Diagnostic = TEXT("planned_affix_set_invalid");
			return false;
		}
		Stack.AffixSet = MoveTemp(Set);
		++OutTrace.AffixedEquipmentCount;
		if (bWeapon)
		{
			OutTrace.PityDecisions.Add(Pity);
		}
	}
	OutTrace.Diagnostic = TEXT("success");
	return true;
}

int32 Fdemo_mapRewardAffixPityLedger::GetState(
	FGuid RunId,
	FName ChannelId) const
{
	return RunId.IsValid() && !ChannelId.IsNone()
		? States.FindRef(LedgerKey(RunId, ChannelId))
		: 0;
}

bool Fdemo_mapRewardAffixPityLedger::Commit(
	FGuid RunId,
	FName ChannelId,
	FName SourceRoleId,
	int32 StateIn,
	int32 StateOut)
{
	if (!RunId.IsValid() || ChannelId.IsNone() || SourceRoleId.IsNone()
		|| StateIn < 0 || StateIn > 3 || StateOut < 0 || StateOut > 3)
	{
		return false;
	}
	const FString Key = LedgerKey(RunId, ChannelId);
	const FString SourceKey = Key + TEXT("|") + SourceRoleId.ToString();
	if (CommittedSources.Contains(SourceKey))
	{
		return States.FindRef(Key) == StateOut;
	}
	if (States.FindRef(Key) != StateIn)
	{
		return false;
	}
	States.Add(Key, StateOut);
	CommittedSources.Add(SourceKey);
	return true;
}

void Fdemo_mapRewardAffixPityLedger::ClearRun(FGuid RunId)
{
	const FString Prefix = RunId.ToString(EGuidFormats::Digits) + TEXT("|");
	for (auto It = States.CreateIterator(); It; ++It)
	{
		if (It.Key().StartsWith(Prefix)) It.RemoveCurrent();
	}
	for (auto It = CommittedSources.CreateIterator(); It; ++It)
	{
		if (It->StartsWith(Prefix)) It.RemoveCurrent();
	}
}

void Fdemo_mapRewardAffixPityLedger::Reset()
{
	States.Reset();
	CommittedSources.Reset();
}
