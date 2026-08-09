#include "demo_mapRewardGenerator.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardGenerationRegistry.h"

namespace
{
	struct FRewardRandom
	{
		explicit FRewardRandom(uint64 InSeed)
			: State(InSeed == 0
				? 0x9E3779B97F4A7C15ull
				: InSeed)
		{
		}

		uint64 Next()
		{
			uint64 X = State;
			X ^= X >> 12;
			X ^= X << 25;
			X ^= X >> 27;
			State = X;
			return X * 2685821657736338717ull;
		}

		uint64 RangeInclusive(uint64 Min, uint64 Max)
		{
			return Min >= Max
				? Min
				: Min + (Next() % (Max - Min + 1));
		}

		uint64 State;
	};

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

	bool CheckedPositiveMultiply(int64 A, int64 B, int64& Out)
	{
		if (A <= 0 || B <= 0 || A > MAX_int64 / B)
		{
			Out = 0;
			return false;
		}
		Out = A * B;
		return true;
	}

	bool CheckedPositiveAdd(int64 A, int64 B, int64& Out)
	{
		if (A < 0 || B < 0 || A > MAX_int64 - B)
		{
			Out = 0;
			return false;
		}
		Out = A + B;
		return true;
	}

	bool HasAllTags(
		const TSet<FName>& SourceTags,
		const TArray<FName>& Required)
	{
		for (const FName Tag : Required)
		{
			if (!SourceTags.Contains(Tag))
			{
				return false;
			}
		}
		return true;
	}

	bool HasExcludedTag(
		const TSet<FName>& SourceTags,
		const TArray<FName>& Excluded)
	{
		for (const FName Tag : Excluded)
		{
			if (SourceTags.Contains(Tag))
			{
				return true;
			}
		}
		return false;
	}

	Fdemo_mapRewardGenerationResult Failure(
		const Fdemo_mapRewardGenerationRequest& Request,
		Edemo_mapRewardGenerationStatus Status,
		const FString& Diagnostic)
	{
		Fdemo_mapRewardGenerationResult Result;
		Result.Status = Status;
		Result.Trace.RequestId = Request.RequestId;
		Result.Trace.RunId = Request.RunId;
		Result.Trace.LootSourceId = Request.LootSourceId;
		Result.Trace.BudgetProfileId = Request.BudgetProfileId;
		Result.Trace.EffectiveSeed = Request.StableSeed;
		Result.Trace.Diagnostic = Diagnostic;
		return Result;
	}

	struct FEligibleRewardEntry
	{
		const Fdemo_mapRewardPoolEntry* Pool = nullptr;
		const Fdemo_mapItemDefinition* Definition = nullptr;
	};
}

uint64 Fdemo_mapRewardGenerator::ComputeStableSeed(
	FGuid RunId,
	FName LootSourceId,
	FName BudgetProfileId)
{
	return HashUtf8(FString::Printf(
		TEXT("%s|%s|%s"),
		*RunId.ToString(EGuidFormats::Digits),
		*LootSourceId.ToString(),
		*BudgetProfileId.ToString()));
}

Fdemo_mapRewardGenerationResult Fdemo_mapRewardGenerator::Generate(
	const Fdemo_mapRewardGenerationRequest& Request)
{
	const Fdemo_mapRewardBudgetProfile* Profile =
		Fdemo_mapRewardGenerationRegistry::FindBudgetProfile(
			Request.BudgetProfileId);
	if (!Profile)
	{
		return Failure(
			Request,
			Edemo_mapRewardGenerationStatus::InvalidBudgetProfile,
			TEXT("Reward Budget Profile does not exist."));
	}
	return GenerateWithData(
		Request,
		*Profile,
		Fdemo_mapRewardGenerationRegistry::GetHighValueContainerPool());
}

Fdemo_mapRewardGenerationResult Fdemo_mapRewardGenerator::GenerateWithData(
	const Fdemo_mapRewardGenerationRequest& Request,
	const Fdemo_mapRewardBudgetProfile& Profile,
	const TArray<Fdemo_mapRewardPoolEntry>& PoolEntries)
{
	if (Request.RequestId.IsNone()
		|| !Request.RunId.IsValid()
		|| Request.LootSourceId.IsNone()
		|| Request.BudgetProfileId.IsNone()
		|| Request.BudgetProfileId != Profile.ProfileId
		|| Request.SourceTags.IsEmpty()
		|| Request.SourceTags.Contains(NAME_None)
		|| Request.StableSeed == 0
		|| Request.TargetCapacity <= 0)
	{
		return Failure(
			Request,
			Edemo_mapRewardGenerationStatus::InvalidRequest,
			TEXT("Reward request identity, tags, seed, or capacity is invalid."));
	}
	if (!Profile.IsValid())
	{
		return Failure(
			Request,
			Edemo_mapRewardGenerationStatus::InvalidBudgetProfile,
			TEXT("Reward Budget Profile is invalid."));
	}
	if (PoolEntries.IsEmpty())
	{
		return Failure(
			Request,
			Edemo_mapRewardGenerationStatus::InvalidPool,
			TEXT("Reward Pool is empty."));
	}

	const TSet<FName> SourceTags(Request.SourceTags);
	TArray<FEligibleRewardEntry> Eligible;
	TSet<FName> EligibleDefinitions;
	for (const Fdemo_mapRewardPoolEntry& Entry : PoolEntries)
	{
		if (!Entry.IsValid())
		{
			return Failure(
				Request,
				Edemo_mapRewardGenerationStatus::InvalidPool,
				FString::Printf(
					TEXT("Reward Pool entry is invalid: %s"),
					*Entry.EntryId.ToString()));
		}
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Entry.DefinitionId);
		if (!Definition)
		{
			return Failure(
				Request,
				Edemo_mapRewardGenerationStatus::MissingDefinition,
				FString::Printf(
					TEXT("Reward Pool Definition is missing: %s"),
					*Entry.DefinitionId.ToString()));
		}
		if (Definition->SellPrice <= 0)
		{
			return Failure(
				Request,
				Edemo_mapRewardGenerationStatus::InvalidUnitValue,
				FString::Printf(
					TEXT("Reward Pool Definition has no positive SellPrice: %s"),
					*Entry.DefinitionId.ToString()));
		}
		if (!HasAllTags(SourceTags, Entry.RequiredSourceTags)
			|| HasExcludedTag(SourceTags, Entry.ExcludedSourceTags)
			|| Definition->Level < Entry.MinItemLevel
			|| Definition->Level > Entry.MaxItemLevel
			|| Definition->SellPrice < Entry.MinUnitValue
			|| Definition->SellPrice > Entry.MaxUnitValue)
		{
			continue;
		}
		FEligibleRewardEntry Candidate;
		Candidate.Pool = &Entry;
		Candidate.Definition = Definition;
		Eligible.Add(Candidate);
		EligibleDefinitions.Add(Definition->DefinitionId);
	}
	Eligible.Sort(
		[](const FEligibleRewardEntry& A, const FEligibleRewardEntry& B)
		{
			return A.Pool->EntryId.LexicalLess(B.Pool->EntryId);
		});
	if (Eligible.IsEmpty())
	{
		return Failure(
			Request,
			Edemo_mapRewardGenerationStatus::NoEligibleItem,
			TEXT("Reward source tags and item eligibility produced no candidates."));
	}

	int64 ScaledBudget = 0;
	if (!CheckedPositiveMultiply(
			Profile.BaseValue,
			Profile.MaxMultiplierBps,
			ScaledBudget))
	{
		return Failure(
			Request,
			Edemo_mapRewardGenerationStatus::ArithmeticOverflow,
			TEXT("Reward Budget Profile exceeds checked int64 multiplication."));
	}
	(void)ScaledBudget;

	FRewardRandom Random(Request.StableSeed);
	const int32 MultiplierBps = static_cast<int32>(
		Random.RangeInclusive(
			static_cast<uint64>(Profile.MinMultiplierBps),
			static_cast<uint64>(Profile.MaxMultiplierBps)));
	int64 Product = 0;
	if (!CheckedPositiveMultiply(
			Profile.BaseValue,
			MultiplierBps,
			Product)
		|| Product > MAX_int64 - 5000)
	{
		return Failure(
			Request,
			Edemo_mapRewardGenerationStatus::ArithmeticOverflow,
			TEXT("Reward randomized budget overflowed checked int64 arithmetic."));
	}
	const int64 RandomizedBudget = FMath::Max<int64>(
		1,
		(Product + 5000) / 10000);

	Fdemo_mapRewardGenerationResult Result;
	Result.Status = Edemo_mapRewardGenerationStatus::Success;
	Result.Trace.RequestId = Request.RequestId;
	Result.Trace.RunId = Request.RunId;
	Result.Trace.LootSourceId = Request.LootSourceId;
	Result.Trace.BudgetProfileId = Request.BudgetProfileId;
	Result.Trace.BaseValue = Profile.BaseValue;
	Result.Trace.MultiplierBps = MultiplierBps;
	Result.Trace.RandomizedBudget = RandomizedBudget;
	Result.Trace.EligibleDefinitionCount = EligibleDefinitions.Num();
	Result.Trace.EffectiveSeed = Request.StableSeed;
	for (const FEligibleRewardEntry& Candidate : Eligible)
	{
		Result.Trace.OrderedEligibleEntryIds.Add(
			Candidate.Pool->EntryId);
	}

	int64 Remaining = RandomizedBudget;
	while (Result.PlannedStacks.Num() < Request.TargetCapacity)
	{
		TArray<const FEligibleRewardEntry*> Affordable;
		int64 TotalWeight = 0;
		for (const FEligibleRewardEntry& Candidate : Eligible)
		{
			const int32 MaxStack = FMath::Min(
				Candidate.Pool->MaxStack,
				Candidate.Definition->MaxStackSize);
			int64 MinimumCost = 0;
			int64 NewWeight = 0;
			if (MaxStack < Candidate.Pool->MinStack
				|| !CheckedPositiveMultiply(
					Candidate.Definition->SellPrice,
					Candidate.Pool->MinStack,
					MinimumCost)
				|| MinimumCost > Remaining)
			{
				continue;
			}
			if (!CheckedPositiveAdd(
				TotalWeight,
				Candidate.Pool->Weight,
				NewWeight))
			{
				return Failure(
					Request,
					Edemo_mapRewardGenerationStatus::ArithmeticOverflow,
					TEXT("Reward candidate weight sum overflowed int64."));
			}
			TotalWeight = NewWeight;
			Affordable.Add(&Candidate);
		}
		if (Affordable.IsEmpty())
		{
			break;
		}

		const int64 Roll = static_cast<int64>(
			Random.RangeInclusive(
				0,
				static_cast<uint64>(TotalWeight - 1)));
		int64 Cursor = 0;
		const FEligibleRewardEntry* Selected = nullptr;
		for (const FEligibleRewardEntry* Candidate : Affordable)
		{
			Cursor += Candidate->Pool->Weight;
			if (Roll < Cursor)
			{
				Selected = Candidate;
				break;
			}
		}
		if (!Selected)
		{
			return Failure(
				Request,
				Edemo_mapRewardGenerationStatus::InvalidPool,
				TEXT("Reward weighted selection failed to resolve a candidate."));
		}

		const int64 UnitValue = Selected->Definition->SellPrice;
		const int32 MaximumStack = static_cast<int32>(FMath::Min<int64>(
			FMath::Min(
				Selected->Pool->MaxStack,
				Selected->Definition->MaxStackSize),
			Remaining / UnitValue));
		const int32 StackCount = static_cast<int32>(
			Random.RangeInclusive(
				static_cast<uint64>(Selected->Pool->MinStack),
				static_cast<uint64>(MaximumStack)));
		int64 TotalValue = 0;
		int64 NewTotal = 0;
		if (!CheckedPositiveMultiply(
			UnitValue,
			StackCount,
			TotalValue)
			|| !CheckedPositiveAdd(
				Result.Trace.GeneratedTotalValue,
				TotalValue,
				NewTotal)
			|| NewTotal > RandomizedBudget)
		{
			return Failure(
				Request,
				Edemo_mapRewardGenerationStatus::ArithmeticOverflow,
				TEXT("Reward planned stack violated checked budget arithmetic."));
		}

		Fdemo_mapRewardPlannedStack Planned;
		Planned.DefinitionId = Selected->Definition->DefinitionId;
		Planned.StackCount = StackCount;
		Planned.UnitValue = UnitValue;
		Planned.TotalValue = TotalValue;
		Planned.Section = Request.TargetSection;
		Planned.SlotIndex = Result.PlannedStacks.Num();
		Result.PlannedStacks.Add(Planned);
		Result.Trace.GeneratedTotalValue = NewTotal;
		Remaining -= TotalValue;
	}

	Result.Trace.ResidualValue = Remaining;
	if (Result.PlannedStacks.IsEmpty())
	{
		Result.Status = Edemo_mapRewardGenerationStatus::BudgetTooSmall;
		Result.Trace.Diagnostic =
			TEXT("Randomized budget cannot buy one legal minimum unit.");
		return Result;
	}

	bool bAffordableRemainder = false;
	for (const FEligibleRewardEntry& Candidate : Eligible)
	{
		int64 MinimumCost = 0;
		if (CheckedPositiveMultiply(
			Candidate.Definition->SellPrice,
			Candidate.Pool->MinStack,
			MinimumCost)
			&& MinimumCost <= Remaining)
		{
			bAffordableRemainder = true;
			break;
		}
	}
	Result.Trace.bCapacityLimited =
		Result.PlannedStacks.Num() >= Request.TargetCapacity
		&& bAffordableRemainder;
	Result.Status = Result.Trace.bCapacityLimited
		? Edemo_mapRewardGenerationStatus::CapacityLimited
		: Edemo_mapRewardGenerationStatus::Success;
	Result.Trace.Diagnostic = Result.Trace.bCapacityLimited
		? TEXT("capacity_limited")
		: TEXT("success");
	return Result;
}

TArray<Fdemo_mapRuntimeContainerSeedEntry>
Fdemo_mapRewardGenerator::BuildContainerSeed(
	const Fdemo_mapRewardGenerationResult& Result)
{
	TArray<Fdemo_mapRuntimeContainerSeedEntry> Seed;
	if (!Result.IsSuccess())
	{
		return Seed;
	}
	for (const Fdemo_mapRewardPlannedStack& Planned : Result.PlannedStacks)
	{
		Fdemo_mapRuntimeContainerSeedEntry Entry;
		Entry.Section = Planned.Section;
		Entry.SlotIndex = Planned.SlotIndex;
		Entry.DefinitionId = Planned.DefinitionId;
		Entry.StackCount = Planned.StackCount;
		Seed.Add(Entry);
	}
	return Seed;
}

FString Fdemo_mapRewardGenerationSession::MakeKey(
	FGuid RunId,
	FName LootSourceId)
{
	return FString::Printf(
		TEXT("%s|%s"),
		*RunId.ToString(EGuidFormats::Digits),
		*LootSourceId.ToString());
}

bool Fdemo_mapRewardGenerationSession::IsProcessed(
	FGuid RunId,
	FName LootSourceId) const
{
	return RunId.IsValid()
		&& !LootSourceId.IsNone()
		&& ProcessedSourceKeys.Contains(MakeKey(RunId, LootSourceId));
}

bool Fdemo_mapRewardGenerationSession::Commit(
	FGuid RunId,
	FName LootSourceId)
{
	if (!RunId.IsValid()
		|| LootSourceId.IsNone()
		|| IsProcessed(RunId, LootSourceId))
	{
		return false;
	}
	ProcessedSourceKeys.Add(MakeKey(RunId, LootSourceId));
	return true;
}

void Fdemo_mapRewardGenerationSession::Reset()
{
	ProcessedSourceKeys.Reset();
}
