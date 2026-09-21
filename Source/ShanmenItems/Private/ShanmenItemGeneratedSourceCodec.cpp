#include "ShanmenItemGeneratedSourceCodec.h"

#include "Dom/JsonObject.h"

namespace
{
	using FObject = TSharedPtr<FJsonObject>;
	using FValue = TSharedPtr<FJsonValue>;
	using FPlan = FShanmenItemGeneratedSourcePlan;
	bool ContainsNull(const FString& Text)
	{
		for (const TCHAR C : Text) { if (C == 0) { return true; } }
		return false;
	}

	// A single field layout drives both directions; adding a field cannot silently
	// update only the writer or only the reader. No reflection/default-value coercion.
	template <typename Archive, typename Value>
	void PlanFields(Archive& A, Value& P)
	{
		A.Field(TEXT("OwnerId"), P.OwnerId);
		A.Field(TEXT("RunId"), P.RunId);
		A.Field(TEXT("SourceRoleId"), P.SourceRoleId);
		A.Field(TEXT("ContentVersion"), P.Content.Version);
		A.Field(TEXT("ContentDigest"), P.Content.Digest);
		A.Field(TEXT("SlotId"), P.SlotId);
		A.Field(TEXT("ProjectionId"), P.ProjectionId);
		A.Field(TEXT("DistributionProfileId"), P.DistributionProfileId);
		A.Field(TEXT("BudgetProfileId"), P.BudgetProfileId);
		A.Field(TEXT("MarkerId"), P.MarkerId);
		A.Field(TEXT("EncounterId"), P.EncounterId);
		A.Field(TEXT("JackpotPolicyId"), P.JackpotPolicyId);
		A.Field(TEXT("RareExtremePolicyId"), P.RareExtremePolicyId);
		A.Field(TEXT("AffixPolicyId"), P.AffixPolicyId);
		A.Field(TEXT("EffectiveSeed"), P.EffectiveSeed);
		A.Field(TEXT("RandomizedBudget"), P.RandomizedBudget);
		A.Field(TEXT("GeneratedTotalValue"), P.GeneratedTotalValue);
		A.Field(TEXT("ResidualValue"), P.ResidualValue);
		A.Field(TEXT("ExpectedSequence"), P.ExpectedSequence);
		A.Field(TEXT("PityStateBefore"), P.PityStateBefore);
		A.Field(TEXT("PityStateAfter"), P.PityStateAfter);
		A.Field(TEXT("PityCommitRequired"), P.bPityCommitRequired);
		A.Field(TEXT("FallbackUsed"), P.bFallbackUsed);
		A.Field(TEXT("LegacyCompatibilityView"), P.bLegacyCompatibilityView);
		A.Array(TEXT("Entries"), P.Entries, FPlan::MaxEntries);
	}

	template <typename Archive, typename Value>
	void EntryFields(Archive& A, Value& E)
	{
		A.Field(TEXT("DefinitionId"), E.Definition.DefinitionId);
		A.Field(TEXT("ItemTags"), E.Definition.ItemTags);
		A.Field(TEXT("MaxStack"), E.Definition.MaxStack);
		A.Field(TEXT("MaxDurability"), E.Definition.MaxDurability);
		A.Field(TEXT("MaxCharges"), E.Definition.MaxCharges);
		A.Field(TEXT("Quantity"), E.Quantity);
		A.Field(TEXT("SectionId"), E.SectionId);
		A.Field(TEXT("SlotIndex"), E.SlotIndex);
		A.Field(TEXT("UnitValue"), E.UnitValue);
		A.Field(TEXT("TotalValue"), E.TotalValue);
		A.Field(TEXT("RewardMetadata"), E.RewardMetadata);
		A.Field(TEXT("ChildContainerType"), E.ChildContainerType);
		A.Field(TEXT("ChildContainerCapacity"), E.ChildContainerCapacity);
	}

	template <typename Archive, typename Value>
	void RewardFields(Archive& A, Value& R)
	{
		A.Enum(TEXT("RewardEventKind"), R.RewardEventKind, 1);
		A.Field(TEXT("RewardEventId"), R.RewardEventId);
		A.Field(TEXT("RewardValueMultiplierBps"), R.RewardValueMultiplierBps);
		A.Field(TEXT("RewardSourceRoleId"), R.RewardSourceRoleId);
		A.Field(TEXT("RareRewardEventId"), R.RareRewardEventId);
		A.Field(TEXT("RareRewardPolicyId"), R.RareRewardPolicyId);
		A.Field(TEXT("RareRewardTierId"), R.RareRewardTierId);
		A.Field(TEXT("RareRewardBonusValue"), R.RareRewardBonusValue);
		A.Field(TEXT("AffixSetEventId"), R.AffixSetEventId);
		A.Field(TEXT("AffixPolicyId"), R.AffixPolicyId);
		A.Enum(TEXT("AffixAcquisition"), R.AffixAcquisition, 2);
		A.Array(TEXT("Affixes"), R.Affixes, 16);
	}

	template <typename Archive, typename Value>
	void AffixFields(Archive& A, Value& F)
	{
		A.Field(TEXT("AffixId"), F.AffixId);
		A.Enum(TEXT("Tier"), F.Tier, 3);
		A.Field(TEXT("ResolvedMagnitudeScaled"), F.ResolvedMagnitudeScaled);
		A.Field(TEXT("ResolvedValue"), F.ResolvedValue);
	}

	struct FWriter
	{
		FObject Object = MakeShared<FJsonObject>();
		void Field(const TCHAR* Key, const FString& V) { Object->SetStringField(Key, V); }
		void Field(const TCHAR* Key, const FName& V) { Field(Key, V.ToString().ToLower()); }
		void Field(const TCHAR* Key, const FGuid& V) { Field(Key, V.ToString(EGuidFormats::Digits)); }
		void Field(const TCHAR* Key, int32 V) { Object->SetNumberField(Key, V); }
		void Field(const TCHAR* Key, bool V) { Object->SetBoolField(Key, V); }
		void Field(const TCHAR* Key, uint64 V) { Field(Key, FString::Printf(TEXT("%llu"), V)); }
		void Field(const TCHAR* Key, int64 V) { Field(Key, static_cast<uint64>(V)); }
		void Field(const TCHAR* Key, const FGameplayTagContainer& V)
		{
			TArray<FString> Names;
			for (const FGameplayTag& Tag : V) { Names.Add(Tag.ToString().ToLower()); }
			Names.Sort();
			TArray<FValue> Values;
			Values.Reserve(Names.Num());
			for (const FString& Name : Names) { Values.Add(MakeShared<FJsonValueString>(Name)); }
			Object->SetArrayField(Key, Values);
		}
		void Field(const TCHAR* Key, const FShanmenItemRewardMetadata& V)
		{
			FWriter Nested;
			RewardFields(Nested, V);
			Object->SetObjectField(Key, Nested.Object);
		}
		template <typename T> void Enum(const TCHAR* Key, T V, int32) { Field(Key, static_cast<int32>(V)); }
		static FObject Entry(const FShanmenItemGeneratedSourceEntry& V)
		{
			FWriter Nested; EntryFields(Nested, V); return Nested.Object;
		}
		static FObject Entry(const FShanmenItemResolvedRewardAffix& V)
		{
			FWriter Nested; AffixFields(Nested, V); return Nested.Object;
		}
		template <typename T> void Array(const TCHAR* Key, const TArray<T>& V, int32)
		{
			TArray<FValue> Values;
			Values.Reserve(V.Num());
			for (const T& Item : V) { Values.Add(MakeShared<FJsonValueObject>(Entry(Item))); }
			Object->SetArrayField(Key, Values);
		}
	};

	struct FReader
	{
		const FObject Object;
		bool bValid = true;
		int32 Fields = 0;
		explicit FReader(const FObject& InObject) : Object(InObject) {}
		FValue Take(const TCHAR* Key, EJson Type)
		{
			++Fields;
			const FValue* V = Object.IsValid() ? Object->Values.Find(Key) : nullptr;
			if (!V || !V->IsValid() || (*V)->Type != Type) { bValid = false; return nullptr; }
			return *V;
		}
		bool Finish() const { return bValid && Object.IsValid() && Object->Values.Num() == Fields; }
		void Field(const TCHAR* Key, FString& V)
		{
			if (const FValue J = Take(Key, EJson::String))
			{
				const FString& Text = J->AsString();
				if (Text.Len() > FPlan::MaxDigestLength) { bValid = false; return; }
				V = Text;
			}
		}
		void Field(const TCHAR* Key, FName& V)
		{
			FString Text; Field(Key, Text);
			// Validate before FName construction (overlong or embedded-NUL input must
			// not assert, truncate, or alias a different persistent identity).
			if (Text.IsEmpty() || Text.Len() >= NAME_SIZE || ContainsNull(Text))
			{
				bValid = false; return;
			}
			V = FName(*Text);
		}
		void Field(const TCHAR* Key, FGuid& V)
		{
			FString Text; Field(Key, Text);
			bValid &= Text.Len() == 32 && FGuid::ParseExact(Text, EGuidFormats::Digits, V);
		}
		void Field(const TCHAR* Key, bool& V)
		{
			if (const FValue J = Take(Key, EJson::Boolean)) { V = J->AsBool(); }
		}
		void Field(const TCHAR* Key, int32& V)
		{
			if (const FValue J = Take(Key, EJson::Number))
			{
				const double N = J->AsNumber();
				if (!FMath::IsFinite(N) || N < MIN_int32 || N > MAX_int32 || FMath::FloorToDouble(N) != N)
				{
					bValid = false; return;
				}
				V = static_cast<int32>(N);
			}
		}
		void Field(const TCHAR* Key, uint64& V)
		{
			const FValue J = Take(Key, EJson::String);
			if (!J) { return; }
			const FString& Text = J->AsString();
			if (Text.IsEmpty() || Text.Len() > 20 || (Text.Len() > 1 && Text[0] == TEXT('0')))
			{
				bValid = false; return;
			}
			uint64 Parsed = 0;
			for (const TCHAR C : Text)
			{
				if (C < TEXT('0') || C > TEXT('9')) { bValid = false; return; }
				const uint64 Digit = C - TEXT('0');
				if (Parsed > (MAX_uint64 - Digit) / 10) { bValid = false; return; }
				Parsed = Parsed * 10 + Digit;
			}
			V = Parsed;
		}
		void Field(const TCHAR* Key, int64& V)
		{
			uint64 Parsed = 0; Field(Key, Parsed);
			if (Parsed > static_cast<uint64>(MAX_int64)) { bValid = false; return; }
			V = static_cast<int64>(Parsed);
		}
		void Field(const TCHAR* Key, FGameplayTagContainer& V)
		{
			const FValue J = Take(Key, EJson::Array);
			if (!J) { return; }
			const auto& Values = J->AsArray();
			if (Values.Num() > FPlan::MaxTagsPerDefinition) { bValid = false; return; }
			for (const FValue& Item : Values)
			{
				if (!Item.IsValid() || Item->Type != EJson::String) { bValid = false; return; }
				const FString& Name = Item->AsString();
				if (Name.IsEmpty() || Name.Len() >= NAME_SIZE || ContainsNull(Name))
				{
					bValid = false; return;
				}
				const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Name), false);
				if (!Tag.IsValid() || V.HasTagExact(Tag)) { bValid = false; return; }
				V.AddTag(Tag);
			}
		}
		void Field(const TCHAR* Key, FShanmenItemRewardMetadata& V)
		{
			const FValue J = Take(Key, EJson::Object);
			if (!J) { return; }
			FReader Nested(J->AsObject());
			RewardFields(Nested, V);
			bValid &= Nested.Finish();
		}
		template <typename T> void Enum(const TCHAR* Key, T& V, int32 Maximum)
		{
			int32 Number = 0; Field(Key, Number);
			if (Number < 0 || Number > Maximum) { bValid = false; return; }
			V = static_cast<T>(Number);
		}
		static bool Entry(const FObject& Object, FShanmenItemGeneratedSourceEntry& V)
		{
			FReader Nested(Object); EntryFields(Nested, V); return Nested.Finish();
		}
		static bool Entry(const FObject& Object, FShanmenItemResolvedRewardAffix& V)
		{
			FReader Nested(Object); AffixFields(Nested, V); return Nested.Finish();
		}
		template <typename T> void Array(const TCHAR* Key, TArray<T>& V, int32 Maximum)
		{
			const FValue J = Take(Key, EJson::Array);
			if (!J) { return; }
			const auto& Values = J->AsArray();
			if (Values.Num() > Maximum) { bValid = false; return; }
			V.Reserve(Values.Num());
			for (const FValue& Item : Values)
			{
				if (!Item.IsValid() || Item->Type != EJson::Object) { bValid = false; return; }
				T Parsed;
				if (!Entry(Item->AsObject(), Parsed)) { bValid = false; return; }
				V.Add(MoveTemp(Parsed));
			}
		}
	};
}

bool FShanmenItemGeneratedSourceCodec::Encode(const FPlan& Plan, FObject& OutObject, FString* OutError)
{
	OutObject.Reset();
	if (OutError) { OutError->Reset(); }
	if (!Plan.IsValid())
	{
		if (OutError) { *OutError = TEXT("Invalid generated source plan."); }
		return false;
	}
	FWriter Writer;
	Writer.Field(TEXT("FormatVersion"), FormatVersion);
	PlanFields(Writer, Plan);
	OutObject = MoveTemp(Writer.Object);
	return true;
}

bool FShanmenItemGeneratedSourceCodec::Decode(const FObject& Object, FPlan& OutPlan, FString* OutError)
{
	OutPlan = FPlan();
	if (OutError) { OutError->Reset(); }
	FReader Reader(Object);
	int32 Version = 0;
	Reader.Field(TEXT("FormatVersion"), Version);
	if (!Reader.bValid || Version != FormatVersion)
	{
		if (OutError) { *OutError = TEXT("Missing or unsupported generated source format version."); }
		return false;
	}
	FPlan Candidate;
	PlanFields(Reader, Candidate);
	if (!Reader.Finish() || !Candidate.IsValid())
	{
		if (OutError) { *OutError = TEXT("Generated source payload has invalid fields, bounds or domain invariants."); }
		return false;
	}
	OutPlan = MoveTemp(Candidate);
	return true;
}
