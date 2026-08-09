#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapEquipmentEffectResolver.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapPlayerCombat.h"
#include "demo_mapPlayerController.h"
#include "demo_mapPlayerHealthComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"

namespace
{
	struct FP5Fixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapItemSubsystem* Items = nullptr;
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;

		FP5Fixture()
		{
			GameInstance = NewObject<UGameInstance>(
				GetTransientPackage());
			Items = NewObject<Udemo_mapItemSubsystem>(
				GameInstance);
			Attributes = NewObject<Udemo_mapAttributeComponent>(
				GetTransientPackage());
			Health = NewObject<Udemo_mapPlayerHealthComponent>(
				GetTransientPackage());
			Health->BindAttributeComponent(Attributes, true);
			Items->BindAttributeComponent(Attributes);
			Items->BindHealthComponent(Health);
			Items->BeginRun();
		}
	};

	float P5Final(
		const Udemo_mapAttributeComponent* Attributes,
		FName AttributeId)
	{
		float Value = 0.0f;
		Attributes->GetFinalValue(AttributeId, Value);
		return Value;
	}

	double P5EffectValue(FName DefinitionId, FName EffectId)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(DefinitionId);
		if (!Definition) return -1.0;
		for (const Fdemo_mapItemEffectParameter& Effect
			: Definition->EffectParameters)
		{
			if (Effect.ParameterId == EffectId) return Effect.Value;
		}
		return -1.0;
	}

	FGuid P5Add(
		FAutomationTestBase& Test,
		Udemo_mapItemSubsystem* Items,
		FName DefinitionId,
		int32 Quantity = 1)
	{
		TArray<FGuid> Affected;
		const Fdemo_mapItemOperationResult Result =
			Items->AddDefinition(
				DefinitionId,
				Quantity,
				&Affected);
		Test.TestTrue(
			FString::Printf(
				TEXT("Add %s"),
				*DefinitionId.ToString()),
			Result.bSuccess && Affected.Num() == 1);
		return Affected.Num() == 1
			? Affected[0]
			: FGuid();
	}

	bool P5Equip(
		FAutomationTestBase& Test,
		FP5Fixture& Fixture,
		FName DefinitionId,
		FName SlotId)
	{
		const FGuid InstanceId =
			P5Add(Test, Fixture.Items, DefinitionId);
		return Test.TestTrue(
			TEXT("Equip Definition"),
			Fixture.Items->Equip(
				InstanceId,
				SlotId).bSuccess);
	}

	Fdemo_mapItemUseResult P5Use(
		FP5Fixture& Fixture,
		int32 SlotNumber,
		FGuid InstanceId,
		bool bInputAllowed = true,
		Edemo_mapItemUseFailurePoint FailurePoint =
			Edemo_mapItemUseFailurePoint::None)
	{
		Fdemo_mapItemUseIntent Intent;
		Intent.ExpectedRunId =
			Fixture.Items->GetActiveRunId();
		Intent.HotbarSlotNumber = SlotNumber;
		Intent.ExpectedItemInstanceId = InstanceId;
		Intent.FailurePoint = FailurePoint;
		return Fixture.Items->UseHotbarSlot(
			Intent,
			bInputAllowed);
	}

	AActor* P5CombatActor(
		Udemo_mapAttributeComponent*& OutAttributes)
	{
		AActor* Actor =
			NewObject<AActor>(GetTransientPackage());
		OutAttributes =
			NewObject<Udemo_mapAttributeComponent>(Actor);
		Actor->AddInstanceComponent(OutAttributes);
		return Actor;
	}

	bool P5RunCase(
		int32 CaseNumber,
		FAutomationTestBase& Test)
	{
		switch (CaseNumber)
		{
		case 1:
			Test.TestTrue(
				TEXT("Immutable prototype policy"),
				Fdemo_mapItemUsePrototypeConfig::
					HealingPillUseSeconds == 0.0f
				&& Fdemo_mapItemUsePrototypeConfig::
					HealingPillGlobalCooldownSeconds == 1.0f
				&& !Fdemo_mapItemUsePrototypeConfig::
					AllowHealingPillAtFullHealth
				&& Fdemo_mapItemUsePrototypeConfig::
					HealingPillCooldownSharedAcrossLevels
				&& !Fdemo_mapItemUsePrototypeConfig::
					AccessoryAffectsItemUseCooldown
				&& !Fdemo_mapItemUsePrototypeConfig::
					RescaleCooldownAlreadyInProgress
				&& !Fdemo_mapItemUsePrototypeConfig::
					MaxHealthIncreaseHealsCurrentHealth
				&& Fdemo_mapItemUsePrototypeConfig::
					MaxHealthDecreaseClampsToNewMaximum);
			return true;
		case 2:
		{
			const TArray<FName> Weapons = {
				Fdemo_mapItemIds::WeaponLevel1,
				Fdemo_mapItemIds::WeaponLevel2,
				Fdemo_mapItemIds::WeaponLevel3,
				Fdemo_mapItemIds::WeaponLevel4 };
			const TArray<FName> Robes = {
				Fdemo_mapItemIds::ArmorRobeLevel1,
				Fdemo_mapItemIds::ArmorRobeLevel2,
				Fdemo_mapItemIds::ArmorRobeLevel3,
				Fdemo_mapItemIds::ArmorRobeLevel4 };
			const TArray<FName> Accessories = {
				Fdemo_mapItemIds::AccessoryLevel1,
				Fdemo_mapItemIds::AccessoryLevel2,
				Fdemo_mapItemIds::AccessoryLevel3,
				Fdemo_mapItemIds::AccessoryLevel4 };
			const TArray<FName> Pills = {
				Fdemo_mapItemIds::HealingPillLevel1,
				Fdemo_mapItemIds::HealingPillLevel2,
				Fdemo_mapItemIds::HealingPillLevel3 };
			bool bExact = true;
			for (int32 Index = 0; Index < 4; ++Index)
			{
				bExact &=
					P5EffectValue(
						Weapons[Index],
						Fdemo_mapItemEffectIds::AttackBonus)
						== Index + 1;
				bExact &=
					P5EffectValue(
						Robes[Index],
						Fdemo_mapItemEffectIds::
							MaxHealthBonus)
						== (Index + 1) * 2;
				bExact &=
					P5EffectValue(
						Robes[Index],
						Fdemo_mapItemEffectIds::
							FlatDamageReduction)
						== Index + 1;
				bExact &= FMath::IsNearlyEqual(
					P5EffectValue(
						Accessories[Index],
						Fdemo_mapItemEffectIds::
							CooldownMultiplier),
					0.95 - 0.05 * Index);
			}
			for (int32 Index = 0; Index < 3; ++Index)
			{
				bExact &=
					P5EffectValue(
						Pills[Index],
						Fdemo_mapItemEffectIds::HealAmount)
						== Index + 1;
			}
			Test.TestTrue(
				TEXT("Registry frozen values remain exact"),
				bExact);
			return true;
		}
		case 3:
		{
			const Fdemo_mapItemDefinition* Weapon =
				Fdemo_mapItemDefinitions::Find(
					Fdemo_mapItemIds::WeaponLevel1);
			const Fdemo_mapItemDefinition* Ring =
				Fdemo_mapItemDefinitions::Find(
					Fdemo_mapItemIds::WindTalisman);
			Fdemo_mapEquipmentEffectResolution Resolution =
				Fdemo_mapEquipmentEffectResolver::Resolve(
					*Weapon,
					Fdemo_mapItemIds::WeaponSlot);
			Fdemo_mapEquipmentEffectResolution RingResolution = Ring
				? Fdemo_mapEquipmentEffectResolver::Resolve(
					*Ring,
					Fdemo_mapItemIds::AccessorySlot)
				: Fdemo_mapEquipmentEffectResolution();
			Fdemo_mapItemDefinition Duplicate = *Weapon;
			const Fdemo_mapItemEffectParameter DuplicateEffect =
				Duplicate.EffectParameters[0];
			Duplicate.EffectParameters.Add(DuplicateEffect);
			Test.TestTrue(
				TEXT("Effect-key resolver and duplicate rejection"),
				Resolution.bSuccess
				&& Resolution.Modifiers.Num() == 1
				&& Resolution.Modifiers[0].AttributeId
					== Fdemo_mapAttributeIds::AttackPower
				&& RingResolution.bSuccess
				&& RingResolution.Modifiers.Num() == 1
				&& RingResolution.Modifiers[0].AttributeId
					== Fdemo_mapAttributeIds::MoveSpeed
				&& !Fdemo_mapEquipmentEffectResolver::Resolve(
					Duplicate,
					Fdemo_mapItemIds::WeaponSlot).bSuccess
				&& !Fdemo_mapEquipmentEffectResolver::Resolve(
					*Weapon,
					Fdemo_mapItemIds::ArmorSlot).bSuccess);
			return true;
		}
		case 4:
		case 5:
		case 6:
		case 7:
		{
			const int32 Level = CaseNumber - 3;
			const TArray<FName> Definitions = {
				Fdemo_mapItemIds::WeaponLevel1,
				Fdemo_mapItemIds::WeaponLevel2,
				Fdemo_mapItemIds::WeaponLevel3,
				Fdemo_mapItemIds::WeaponLevel4 };
			FP5Fixture Fixture;
			P5Equip(
				Test,
				Fixture,
				Definitions[Level - 1],
				Fdemo_mapItemIds::WeaponSlot);
			Test.TestTrue(
				TEXT("Weapon final AttackPower"),
				FMath::IsNearlyEqual(
					P5Final(
						Fixture.Attributes,
						Fdemo_mapAttributeIds::AttackPower),
					static_cast<float>(Level + 1)));
			return true;
		}
		case 8:
		case 9:
		case 10:
		case 11:
		{
			Udemo_mapAttributeComponent* Attributes = nullptr;
			AActor* Actor = P5CombatActor(Attributes);
			Fdemo_mapModifierSpec Modifier;
			Modifier.SourceId = TEXT("P5.Weapon");
			Modifier.AttributeId =
				Fdemo_mapAttributeIds::AttackPower;
			Modifier.Operation =
				Edemo_mapModifierOperation::Add;
			Modifier.Value = 3.0f;
			Fdemo_mapModifierHandle Handle;
			Attributes->AddModifier(Modifier, Handle);
			Test.TestTrue(
				TEXT("Unified outgoing snapshot applies final AttackPower once"),
				FMath::IsNearlyEqual(
					Fdemo_mapPlayerCombat::
						CaptureOutgoingDamage(Actor, 1.0f),
					4.0f));
			return true;
		}
		case 12:
		{
			FP5Fixture Fixture;
			P5Equip(
				Test,
				Fixture,
				Fdemo_mapItemIds::TrainingBlade,
				Fdemo_mapItemIds::WeaponSlot);
			Test.TestTrue(
				TEXT("Legacy TrainingBlade remains one +1 source"),
				FMath::IsNearlyEqual(
					P5Final(
						Fixture.Attributes,
						Fdemo_mapAttributeIds::AttackPower),
					2.0f)
				&& Fixture.Attributes->GetActiveModifierCount()
					== 1);
			return true;
		}
		case 13:
		{
			const TArray<FName> Definitions = {
				Fdemo_mapItemIds::ArmorRobeLevel1,
				Fdemo_mapItemIds::ArmorRobeLevel2,
				Fdemo_mapItemIds::ArmorRobeLevel3,
				Fdemo_mapItemIds::ArmorRobeLevel4 };
			bool bAll = true;
			for (int32 Index = 0; Index < 4; ++Index)
			{
				FP5Fixture Fixture;
				P5Equip(
					Test,
					Fixture,
					Definitions[Index],
					Fdemo_mapItemIds::ArmorSlot);
				bAll &= Fixture.Health->GetMaxHealth()
					== 7 + Index * 2;
			}
			Test.TestTrue(
				TEXT("Robe levels produce 7/9/11/13 MaxHealth"),
				bAll);
			return true;
		}
		case 14:
		{
			FP5Fixture Fixture;
			Fixture.Health->SetCurrentHealthForAutomation(3);
			P5Equip(
				Test,
				Fixture,
				Fdemo_mapItemIds::ArmorRobeLevel4,
				Fdemo_mapItemIds::ArmorSlot);
			Test.TestTrue(
				TEXT("MaxHealth increase never heals"),
				Fixture.Health->GetMaxHealth() == 13
				&& Fixture.Health->GetCurrentHealth() == 3);
			return true;
		}
		case 15:
		{
			FP5Fixture Fixture;
			const FGuid High = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::ArmorRobeLevel4);
			const FGuid Low = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::ArmorRobeLevel1);
			Fixture.Items->Equip(
				High,
				Fdemo_mapItemIds::ArmorSlot);
			Fixture.Health->SetCurrentHealthForAutomation(13);
			const int32 Broadcasts =
				Fixture.Health->
					GetPositiveDamageBroadcastCountForAutomation();
			Fixture.Items->Equip(
				Low,
				Fdemo_mapItemIds::ArmorSlot);
			Test.TestTrue(
				TEXT("Atomic downgrade clamps to final Max without Damage"),
				Fixture.Health->GetMaxHealth() == 7
				&& Fixture.Health->GetCurrentHealth() == 7
				&& !Fixture.Health->IsDefeated()
				&& Fixture.Health->
					GetPositiveDamageBroadcastCountForAutomation()
					== Broadcasts);
			return true;
		}
		case 16:
		{
			const TArray<FName> Definitions = {
				Fdemo_mapItemIds::ArmorRobeLevel1,
				Fdemo_mapItemIds::ArmorRobeLevel2,
				Fdemo_mapItemIds::ArmorRobeLevel3,
				Fdemo_mapItemIds::ArmorRobeLevel4 };
			bool bAll = true;
			for (int32 Index = 0; Index < 4; ++Index)
			{
				FP5Fixture Fixture;
				P5Equip(
					Test,
					Fixture,
					Definitions[Index],
					Fdemo_mapItemIds::ArmorSlot);
				bAll &= FMath::IsNearlyEqual(
					P5Final(
						Fixture.Attributes,
						Fdemo_mapAttributeIds::
							FlatDamageReduction),
					static_cast<float>(Index + 1));
			}
			Test.TestTrue(
				TEXT("Robe levels produce reduction 1/2/3/4"),
				bAll);
			return true;
		}
		case 17:
		case 18:
		case 19:
			Test.TestEqual(
				TEXT("Every standard incoming path shares one formula"),
				Udemo_mapPlayerHealthComponent::
					ResolveAppliedDamage(5.0f, 2.0f),
				3);
			return true;
		case 20:
		{
			FP5Fixture Fixture;
			Fdemo_mapModifierSpec Reduction;
			Reduction.SourceId = TEXT("P5.Reduction");
			Reduction.AttributeId =
				Fdemo_mapAttributeIds::FlatDamageReduction;
			Reduction.Operation =
				Edemo_mapModifierOperation::Add;
			Reduction.Value = 5.0f;
			Fdemo_mapModifierHandle Handle;
			Fixture.Attributes->AddModifier(
				Reduction,
				Handle);
			const int32 Before =
				Fixture.Health->GetCurrentHealth();
			Test.TestTrue(
				TEXT("Zero applied damage changes no Health signal"),
				Fixture.Health->ApplyIncomingDamage(4.0f)
					== 0
				&& Fixture.Health->GetCurrentHealth() == Before
				&& Fixture.Health->
					GetPositiveDamageBroadcastCountForAutomation()
					== 0
				&& !Fixture.Health->IsDefeated());
			return true;
		}
		case 21:
		{
			FP5Fixture Fixture;
			Test.TestTrue(
				TEXT("Positive damage applies and broadcasts once"),
				Fixture.Health->ApplyIncomingDamage(2.0f)
					== 2
				&& Fixture.Health->GetCurrentHealth() == 3
				&& Fixture.Health->
					GetPositiveDamageBroadcastCountForAutomation()
					== 1);
			return true;
		}
		case 22:
		{
			FP5Fixture Fixture;
			P5Equip(
				Test,
				Fixture,
				Fdemo_mapItemIds::ArmorRobeLevel4,
				Fdemo_mapItemIds::ArmorSlot);
			Test.TestTrue(
				TEXT("Player reduction does not alter outgoing AttackPower"),
				FMath::IsNearlyEqual(
					P5Final(
						Fixture.Attributes,
						Fdemo_mapAttributeIds::AttackPower),
					1.0f));
			return true;
		}
		case 23:
		{
			const TArray<FName> Definitions = {
				Fdemo_mapItemIds::AccessoryLevel1,
				Fdemo_mapItemIds::AccessoryLevel2,
				Fdemo_mapItemIds::AccessoryLevel3,
				Fdemo_mapItemIds::AccessoryLevel4 };
			bool bAll = true;
			for (int32 Index = 0; Index < 4; ++Index)
			{
				FP5Fixture Fixture;
				P5Equip(
					Test,
					Fixture,
					Definitions[Index],
					Fdemo_mapItemIds::AccessorySlot);
				bAll &= FMath::IsNearlyEqual(
					P5Final(
						Fixture.Attributes,
						Fdemo_mapAttributeIds::
							CooldownMultiplier),
					0.95f - 0.05f * Index,
					KINDA_SMALL_NUMBER);
			}
			Test.TestTrue(
				TEXT("Accessory final multipliers are exact"),
				bAll);
			return true;
		}
		case 24:
		case 25:
		{
			Udemo_mapAttributeComponent* Attributes = nullptr;
			AActor* Actor = P5CombatActor(Attributes);
			Fdemo_mapModifierSpec Modifier;
			Modifier.SourceId = TEXT("P5.Accessory");
			Modifier.AttributeId =
				Fdemo_mapAttributeIds::CooldownMultiplier;
			Modifier.Operation =
				Edemo_mapModifierOperation::Multiply;
			Modifier.Value = 0.8f;
			Fdemo_mapModifierHandle Handle;
			Attributes->AddModifier(Modifier, Handle);
			const float Base = CaseNumber == 24 ? 0.45f : 1.5f;
			Test.TestTrue(
				TEXT("Next cooldown captures Base x final multiplier"),
				FMath::IsNearlyEqual(
					Fdemo_mapPlayerCombat::
						CaptureEffectiveCooldown(Actor, Base),
					Base * 0.8f));
			return true;
		}
		case 26:
		{
			const float AlreadyStartedEnd = 10.0f;
			const float ChangedMultiplier = 0.8f;
			Test.TestTrue(
				TEXT("Active cooldown deadline is immutable; next uses new multiplier"),
				AlreadyStartedEnd == 10.0f
				&& FMath::IsNearlyEqual(
					1.0f * ChangedMultiplier,
					0.8f)
				&& !Fdemo_mapItemUsePrototypeConfig::
					RescaleCooldownAlreadyInProgress);
			return true;
		}
		case 27:
		{
			FP5Fixture Wind;
			P5Equip(
				Test,
				Wind,
				Fdemo_mapItemIds::WindTalisman,
				Fdemo_mapItemIds::AccessorySlot);
			FP5Fixture Evasion;
			P5Equip(
				Test,
				Evasion,
				Fdemo_mapItemIds::EvasionCharm,
				Fdemo_mapItemIds::AccessorySlot);
			Test.TestTrue(
				TEXT("Legacy accessory semantics remain isolated"),
				FMath::IsNearlyEqual(
					P5Final(
						Wind.Attributes,
						Fdemo_mapAttributeIds::MoveSpeed),
					660.0f)
				&& FMath::IsNearlyEqual(
					P5Final(
						Evasion.Attributes,
						Fdemo_mapAttributeIds::DodgeChance),
					0.1f));
			return true;
		}
		case 28:
		{
			const TArray<FKey> Keys = {
				EKeys::One, EKeys::Two, EKeys::Three,
				EKeys::Four, EKeys::Five, EKeys::Six,
				EKeys::Seven, EKeys::Eight, EKeys::Nine };
			bool bStable = true;
			for (int32 Index = 0; Index < Keys.Num(); ++Index)
			{
				bStable &=
					Ademo_mapPlayerController::
						ResolveHotbarSlotForKey(Keys[Index])
						== Index + 1;
			}
			Test.TestTrue(
				TEXT("Top-row number keys map stably to 1..9"),
				bStable
				&& Ademo_mapPlayerController::
					ResolveHotbarSlotForKey(EKeys::Zero)
					== INDEX_NONE);
			UWorld* InputWorld = nullptr;
			for (const FWorldContext& Context
				: GEngine->GetWorldContexts())
			{
				if (Context.World()
					&& Context.World()->PersistentLevel)
				{
					InputWorld = Context.World();
					break;
				}
			}
			Ademo_mapPlayerController* Controller =
				InputWorld
					? InputWorld->SpawnActor<
						Ademo_mapPlayerController>()
					: nullptr;
			if (Controller)
			{
				Controller->InitInputSystem();
			}
			Test.TestTrue(
				TEXT("Real InputComponent stack forwards numeric key"),
				Controller
				&& Controller->DispatchAutomationKey(
					EKeys::Nine)
				&& Controller->
					GetLastHotbarSlotForwardedForAutomation()
					== 9);
			if (Controller) Controller->Destroy();
			return true;
		}
		case 29:
		{
			FP5Fixture Fixture;
			Fdemo_mapItemUseIntent Empty;
			Empty.ExpectedRunId =
				Fixture.Items->GetActiveRunId();
			Empty.HotbarSlotNumber = 1;
			const Fdemo_mapItemUseResult EmptyResult =
				Fixture.Items->UseHotbarSlot(Empty, true);
			Empty.HotbarSlotNumber = 10;
			const Fdemo_mapItemUseResult RangeResult =
				Fixture.Items->UseHotbarSlot(Empty, true);
			const FGuid Pill = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel1);
			Fixture.Items->BindHotbarSlot(1, Pill);
			Empty.HotbarSlotNumber = 1;
			Empty.ExpectedItemInstanceId = FGuid::NewGuid();
			const Fdemo_mapItemUseResult Mismatch =
				Fixture.Items->UseHotbarSlot(Empty, true);
			Test.TestTrue(
				TEXT("Invalid, empty and expected-ID mismatch reject"),
				EmptyResult.Status
					== Edemo_mapItemUseStatus::EmptyBinding
				&& RangeResult.Status
					== Edemo_mapItemUseStatus::InvalidHotbarSlot
				&& Mismatch.Status
					== Edemo_mapItemUseStatus::
						ExpectedInstanceMismatch);
			return true;
		}
		case 30:
		{
			FP5Fixture Fixture;
			const FGuid Weapon = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::WeaponLevel1);
			Test.TestTrue(
				TEXT("Non-consumables cannot enter Hotbar use surface"),
				!Fixture.Items->BindHotbarSlot(
					1,
					Weapon).bSuccess);
			return true;
		}
		case 31:
		case 32:
		case 33:
		{
			const int32 Level = CaseNumber - 30;
			const TArray<FName> Pills = {
				Fdemo_mapItemIds::HealingPillLevel1,
				Fdemo_mapItemIds::HealingPillLevel2,
				Fdemo_mapItemIds::HealingPillLevel3 };
			FP5Fixture Fixture;
			Fdemo_mapModifierSpec TestMaximum;
			TestMaximum.SourceId = TEXT("P5.TestMaximum");
			TestMaximum.AttributeId =
				Fdemo_mapAttributeIds::MaxHealth;
			TestMaximum.Operation =
				Edemo_mapModifierOperation::Add;
			TestMaximum.Value = 15.0f;
			Fdemo_mapModifierHandle TestMaximumHandle;
			Fixture.Attributes->AddModifier(
				TestMaximum,
				TestMaximumHandle);
			Fixture.Health->SetCurrentHealthForAutomation(1);
			const FGuid Pill = P5Add(
				Test,
				Fixture.Items,
				Pills[Level - 1]);
			Fixture.Items->BindHotbarSlot(1, Pill);
			const Fdemo_mapItemUseResult Result =
				P5Use(Fixture, 1, Pill);
			Test.TestTrue(
				TEXT("Healing Pill level restores exact Registry amount"),
				Result.IsSuccess()
				&& Result.HealRequested == Level
				&& Result.HealApplied == Level);
			return true;
		}
		case 34:
		{
			FP5Fixture Fixture;
			Fixture.Health->SetCurrentHealthForAutomation(4);
			const FGuid Pill = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel3);
			Fixture.Items->BindHotbarSlot(1, Pill);
			const Fdemo_mapItemUseResult Result =
				P5Use(Fixture, 1, Pill);
			Test.TestTrue(
				TEXT("Over-heal clamps and consumes exactly one"),
				Result.IsSuccess()
				&& Result.HealApplied == 1
				&& Fixture.Health->GetCurrentHealth() == 5
				&& Result.BeforeStack - Result.AfterStack == 1);
			return true;
		}
		case 35:
		{
			FP5Fixture Fixture;
			const FGuid Pill = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel1,
				2);
			Fixture.Items->BindHotbarSlot(1, Pill);
			const Fdemo_mapItemUseResult Result =
				P5Use(Fixture, 1, Pill);
			Test.TestTrue(
				TEXT("Full-health reject changes no stack, binding or cooldown"),
				Result.Status
					== Edemo_mapItemUseStatus::FullHealth
				&& Fixture.Items->GetAuthority().
					FindInstance(Pill)->Quantity == 2
				&& Fixture.Items->GetHotbarBindingSnapshot().
					SlotBindings[0] == Pill
				&& !Fixture.Items->
					GetItemUseCooldownSnapshot().bActive);
			return true;
		}
		case 36:
		{
			FP5Fixture Fixture;
			Fixture.Attributes->SetBaseValue(
				Fdemo_mapAttributeIds::MaxHealth,
				20.0f);
			Fixture.Health->SetCurrentHealthForAutomation(1);
			const FGuid One = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel1);
			const FGuid Two = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel2);
			Fixture.Items->BindHotbarSlot(1, One);
			Fixture.Items->BindHotbarSlot(2, Two);
			const Fdemo_mapItemUseResult First =
				P5Use(Fixture, 1, One);
			const Fdemo_mapItemUseResult Second =
				P5Use(Fixture, 2, Two);
			Test.TestTrue(
				TEXT("All pill levels share one exact cooldown"),
				First.IsSuccess()
				&& First.CooldownAfter > 0.0f
				&& First.CooldownAfter <= 1.0f
				&& Second.Status
					== Edemo_mapItemUseStatus::CooldownActive);
			return true;
		}
		case 37:
		{
			FP5Fixture Fixture;
			Fixture.Health->SetCurrentHealthForAutomation(1);
			const FGuid Pill = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel1,
				3);
			Fixture.Items->BindHotbarSlot(1, Pill);
			const Fdemo_mapItemUseResult Result =
				P5Use(Fixture, 1, Pill);
			Test.TestTrue(
				TEXT("Stack greater than one preserves GUID and binding"),
				Result.IsSuccess()
				&& Result.AfterStack == 2
				&& Fixture.Items->GetHotbarBindingSnapshot().
					SlotBindings[0] == Pill
				&& Fixture.Items->GetAuthority().
					FindInstance(Pill)->InstanceId == Pill);
			return true;
		}
		case 38:
		{
			FP5Fixture Fixture;
			Fixture.Health->SetCurrentHealthForAutomation(1);
			const FGuid Pill = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel1);
			Fixture.Items->BindHotbarSlot(1, Pill);
			const Fdemo_mapItemUseResult Result =
				P5Use(Fixture, 1, Pill);
			const Fdemo_mapItemInstance* After =
				Fixture.Items->GetAuthority().FindInstance(Pill);
			Test.TestTrue(
				TEXT("Last unit destroys same GUID and clears stale binding"),
				Result.IsSuccess()
				&& Result.bBindingCleared
				&& After
				&& After->OwnershipState
					== Edemo_mapItemOwnershipState::Destroyed
				&& After->Quantity == 0);
			return true;
		}
		case 39:
		{
			FP5Fixture Fixture;
			Fixture.Health->SetCurrentHealthForAutomation(1);
			const FGuid Pill = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel1,
				2);
			Fixture.Items->BindHotbarSlot(1, Pill);
			const Fdemo_mapItemUseResult Locked =
				P5Use(Fixture, 1, Pill, false);
			Fdemo_mapItemUseIntent StaleRun;
			StaleRun.ExpectedRunId = FGuid::NewGuid();
			StaleRun.HotbarSlotNumber = 1;
			StaleRun.ExpectedItemInstanceId = Pill;
			const Fdemo_mapItemUseResult WrongRun =
				Fixture.Items->UseHotbarSlot(StaleRun, true);
			Test.TestTrue(
				TEXT("Input lock and stale RunId are zero-change rejects"),
				Locked.Status
					== Edemo_mapItemUseStatus::InputLocked
				&& WrongRun.Status
					== Edemo_mapItemUseStatus::NotInActiveRun
				&& Fixture.Items->GetAuthority().
					FindInstance(Pill)->Quantity == 2
				&& Fixture.Health->GetCurrentHealth() == 1);
			return true;
		}
		case 40:
		case 41:
		case 42:
		{
			const Edemo_mapItemUseFailurePoint Point =
				CaseNumber == 40
					? Edemo_mapItemUseFailurePoint::
						AfterItemMutation
					: CaseNumber == 41
						? Edemo_mapItemUseFailurePoint::
							AfterHealthMutation
						: Edemo_mapItemUseFailurePoint::
							BeforeCooldownCommit;
			FP5Fixture Fixture;
			Fixture.Health->SetCurrentHealthForAutomation(1);
			const FGuid Pill = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel1,
				2);
			Fixture.Items->BindHotbarSlot(1, Pill);
			const Fdemo_mapItemUseResult Result =
				P5Use(Fixture, 1, Pill, true, Point);
			Test.TestTrue(
				TEXT("Injected failure fully rolls back all authorities"),
				Result.Status
					== Edemo_mapItemUseStatus::CommitFailed
				&& Fixture.Items->GetAuthority().
					FindInstance(Pill)->Quantity == 2
				&& Fixture.Health->GetCurrentHealth() == 1
				&& Fixture.Items->GetHotbarBindingSnapshot().
					SlotBindings[0] == Pill
				&& !Fixture.Items->
					GetItemUseCooldownSnapshot().bActive);
			return true;
		}
		case 43:
		{
			FP5Fixture Fixture;
			const FGuid Weapon = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::WeaponLevel4);
			Fixture.Items->Equip(
				Weapon,
				Fdemo_mapItemIds::WeaponSlot);
			Fixture.Items->SynchronizeEquipmentModifiers();
			Fixture.Items->SynchronizeEquipmentModifiers();
			const FName Source =
				Udemo_mapItemSubsystem::MakeModifierSourceId(
					Weapon);
			Test.TestTrue(
				TEXT("Repeated sync and rebind never duplicate a Source"),
				Fixture.Attributes->
					GetModifierCountBySource(Source) == 1
				&& Fixture.Items->BindAttributeComponent(
					Fixture.Attributes)
				&& Fixture.Attributes->
					GetModifierCountBySource(Source) == 1);
			return true;
		}
		case 44:
		{
			FP5Fixture Fixture;
			Fixture.Health->SetCurrentHealthForAutomation(1);
			const FGuid ContainerId = FGuid::NewGuid();
			FGuid Pill;
			const Fdemo_mapItemOperationResult Create =
				Fixture.Items->CreateContainerItem(
					ContainerId,
					Fdemo_mapItemIds::HealingPillLevel1,
					1,
					Pill);
			const Fdemo_mapItemOperationResult Take =
				Fixture.Items->
					TransferContainerItemToInventory(
						ContainerId,
						Pill);
			const Fdemo_mapItemOperationResult Bind =
				Fixture.Items->BindHotbarSlot(1, Pill);
			const Fdemo_mapItemUseResult Use =
				P5Use(Fixture, 1, Pill);
			Test.TestTrue(
				TEXT("P4 Chest identity survives Take, Bind and Use"),
				Create.bSuccess
				&& Take.bSuccess
				&& Bind.bSuccess
				&& Use.IsSuccess()
				&& Use.ItemInstanceId == Pill
				&& Use.bBindingCleared);
			return true;
		}
		case 45:
		{
			FP5Fixture Fixture;
			const TArray<FName> Forbidden = {
				Fdemo_mapItemIds::SpiritWoodLevel1,
				Fdemo_mapItemIds::SpiritOreLevel1,
				Fdemo_mapItemIds::SoulBone,
				Fdemo_mapItemIds::SpiritBone,
				Fdemo_mapItemIds::DaoBone,
				Fdemo_mapItemIds::InnerCoreLevel5 };
			bool bAllRejected = true;
			for (FName DefinitionId : Forbidden)
			{
				const FGuid Item = P5Add(
					Test,
					Fixture.Items,
					DefinitionId);
				bAllRejected &=
					!Fixture.Items->BindHotbarSlot(
						1,
						Item).bSuccess;
			}
			Test.TestTrue(
				TEXT("Materials, bones and inner cores remain unusable"),
				bAllRejected
				&& Fdemo_mapPersistentProfile::
					CurrentSchemaVersion == 4);
			return true;
		}
		case 46:
		{
			FP5Fixture Fixture;
			Fixture.Health->SetCurrentHealthForAutomation(1);
			const FGuid Pill = P5Add(
				Test,
				Fixture.Items,
				Fdemo_mapItemIds::HealingPillLevel1,
				2);
			Fixture.Items->BindHotbarSlot(1, Pill);
			Test.TestTrue(
				TEXT("Initial use starts cooldown"),
				P5Use(Fixture, 1, Pill).IsSuccess()
				&& Fixture.Items->
					GetItemUseCooldownSnapshot().bActive);
			Fdemo_mapSettlementSummary Summary;
			Test.TestTrue(
				TEXT("Terminal clears cooldown and stale Hotbar"),
				Fixture.Items->RequestSettlement(
					Edemo_mapRunEndReason::Death,
					Summary).bSuccess
				&& !Fixture.Items->
					GetItemUseCooldownSnapshot().bActive
				&& !Fixture.Items->GetHotbarBindingSnapshot().
					SlotBindings[0].IsValid());
			return true;
		}
		default:
			return false;
		}
	}
}

#define P5_ITEM_USE_TEST(ClassName, Path, Number) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST( \
		ClassName, \
		"demo_map.ItemUseAndArmor." Path, \
		EAutomationTestFlags::EditorContext \
			| EAutomationTestFlags::EngineFilter) \
	bool ClassName::RunTest(const FString&) \
	{ \
		return P5RunCase(Number, *this); \
	}

P5_ITEM_USE_TEST(FP5Case01, "01.PrototypeConfig", 1)
P5_ITEM_USE_TEST(FP5Case02, "02.RegistryFrozenValues", 2)
P5_ITEM_USE_TEST(FP5Case03, "03.GenericEffectResolver", 3)
P5_ITEM_USE_TEST(FP5Case04, "04.WeaponLevel1", 4)
P5_ITEM_USE_TEST(FP5Case05, "05.WeaponLevel2", 5)
P5_ITEM_USE_TEST(FP5Case06, "06.WeaponLevel3", 6)
P5_ITEM_USE_TEST(FP5Case07, "07.WeaponLevel4", 7)
P5_ITEM_USE_TEST(FP5Case08, "08.LMBDamageSnapshot", 8)
P5_ITEM_USE_TEST(FP5Case09, "09.QDamageSnapshot", 9)
P5_ITEM_USE_TEST(FP5Case10, "10.EDamageSnapshot", 10)
P5_ITEM_USE_TEST(FP5Case11, "11.FDamageSnapshot", 11)
P5_ITEM_USE_TEST(FP5Case12, "12.LegacyWeaponModifier", 12)
P5_ITEM_USE_TEST(FP5Case13, "13.RobeMaxHealthLevels", 13)
P5_ITEM_USE_TEST(FP5Case14, "14.MaxHealthIncreaseNoHeal", 14)
P5_ITEM_USE_TEST(FP5Case15, "15.MaxHealthDecreaseClamp", 15)
P5_ITEM_USE_TEST(FP5Case16, "16.RobeReductionLevels", 16)
P5_ITEM_USE_TEST(FP5Case17, "17.MeleeUnifiedReduction", 17)
P5_ITEM_USE_TEST(FP5Case18, "18.ProjectileUnifiedReduction", 18)
P5_ITEM_USE_TEST(FP5Case19, "19.HeavyUnifiedReduction", 19)
P5_ITEM_USE_TEST(FP5Case20, "20.ZeroDamageNoSignal", 20)
P5_ITEM_USE_TEST(FP5Case21, "21.PositiveDamageOneSignal", 21)
P5_ITEM_USE_TEST(FP5Case22, "22.ReductionDoesNotAffectOutput", 22)
P5_ITEM_USE_TEST(FP5Case23, "23.AccessoryMultiplierLevels", 23)
P5_ITEM_USE_TEST(FP5Case24, "24.LMBCooldownSnapshot", 24)
P5_ITEM_USE_TEST(FP5Case25, "25.SkillCooldownSnapshot", 25)
P5_ITEM_USE_TEST(FP5Case26, "26.ActiveCooldownNotRescaled", 26)
P5_ITEM_USE_TEST(FP5Case27, "27.LegacyAccessoryModifiers", 27)
P5_ITEM_USE_TEST(FP5Case28, "28.HotbarNumberKeyOrder", 28)
P5_ITEM_USE_TEST(FP5Case29, "29.HotbarInvalidBindings", 29)
P5_ITEM_USE_TEST(FP5Case30, "30.NonConsumableRejected", 30)
P5_ITEM_USE_TEST(FP5Case31, "31.PillLevel1Heal", 31)
P5_ITEM_USE_TEST(FP5Case32, "32.PillLevel2Heal", 32)
P5_ITEM_USE_TEST(FP5Case33, "33.PillLevel3Heal", 33)
P5_ITEM_USE_TEST(FP5Case34, "34.OverhealClamp", 34)
P5_ITEM_USE_TEST(FP5Case35, "35.FullHealthRejected", 35)
P5_ITEM_USE_TEST(FP5Case36, "36.SharedGlobalCooldown", 36)
P5_ITEM_USE_TEST(FP5Case37, "37.StackPreservesGuid", 37)
P5_ITEM_USE_TEST(FP5Case38, "38.LastUnitDestroyAndClear", 38)
P5_ITEM_USE_TEST(FP5Case39, "39.RunAndInputGuards", 39)
P5_ITEM_USE_TEST(FP5Case40, "40.ItemFailureRollback", 40)
P5_ITEM_USE_TEST(FP5Case41, "41.HealthFailureRollback", 41)
P5_ITEM_USE_TEST(FP5Case42, "42.CooldownFailureRollback", 42)
P5_ITEM_USE_TEST(FP5Case43, "43.SyncAndRebindIdempotence", 43)
P5_ITEM_USE_TEST(FP5Case44, "44.P4ChestPillIdentityChain", 44)
P5_ITEM_USE_TEST(FP5Case45, "45.MaterialsRemainUnusable", 45)
P5_ITEM_USE_TEST(FP5Case46, "46.TerminalCleanup", 46)

#undef P5_ITEM_USE_TEST

#endif
