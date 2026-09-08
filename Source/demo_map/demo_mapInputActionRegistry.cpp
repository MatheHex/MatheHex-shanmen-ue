#include "demo_mapInputActionRegistry.h"

const FName Fdemo_mapInputActionIds::MoveForward(TEXT("MoveForward"));
const FName Fdemo_mapInputActionIds::MoveBackward(TEXT("MoveBackward"));
const FName Fdemo_mapInputActionIds::MoveLeft(TEXT("MoveLeft"));
const FName Fdemo_mapInputActionIds::MoveRight(TEXT("MoveRight"));
const FName Fdemo_mapInputActionIds::PrimaryAttack(TEXT("PrimaryAttack"));
const FName Fdemo_mapInputActionIds::SkillGroundCircle(TEXT("SkillGroundCircle"));
const FName Fdemo_mapInputActionIds::SkillSelfSector(TEXT("SkillSelfSector"));
const FName Fdemo_mapInputActionIds::SkillStraightProjectile(TEXT("SkillStraightProjectile"));
const FName Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall(
	TEXT("ControlledWeaponLaunchRecall"));
const FName Fdemo_mapInputActionIds::ControlledWeaponRedirect(
	TEXT("ControlledWeaponRedirect"));
const FName Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle(
	TEXT("ThrownWeaponTrajectoryToggle"));
const FName Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet(
	TEXT("ThrownWeaponArcTargetSet"));
const FName Fdemo_mapInputActionIds::ThrownWeaponArcApexIncrease(
	TEXT("ThrownWeaponArcApexIncrease"));
const FName Fdemo_mapInputActionIds::ThrownWeaponArcApexDecrease(
	TEXT("ThrownWeaponArcApexDecrease"));
const FName Fdemo_mapInputActionIds::ThrownWeaponArcTargetClear(
	TEXT("ThrownWeaponArcTargetClear"));
const FName Fdemo_mapInputActionIds::SpiritEvasion(TEXT("SpiritEvasion"));
const FName Fdemo_mapInputActionIds::WeaponGuard(TEXT("WeaponGuard"));
const FName Fdemo_mapInputActionIds::Interact(TEXT("Interact"));
const FName Fdemo_mapInputActionIds::ResetRun(TEXT("ResetRun"));
const FName Fdemo_mapInputActionIds::Inventory(TEXT("Inventory"));
const FName Fdemo_mapInputActionIds::Back(TEXT("Back"));
const FName Fdemo_mapInputActionIds::Hotbar1(TEXT("Hotbar1"));
const FName Fdemo_mapInputActionIds::Hotbar2(TEXT("Hotbar2"));
const FName Fdemo_mapInputActionIds::Hotbar3(TEXT("Hotbar3"));
const FName Fdemo_mapInputActionIds::Hotbar4(TEXT("Hotbar4"));
const FName Fdemo_mapInputActionIds::Hotbar5(TEXT("Hotbar5"));
const FName Fdemo_mapInputActionIds::Hotbar6(TEXT("Hotbar6"));
const FName Fdemo_mapInputActionIds::Hotbar7(TEXT("Hotbar7"));
const FName Fdemo_mapInputActionIds::Hotbar8(TEXT("Hotbar8"));
const FName Fdemo_mapInputActionIds::Hotbar9(TEXT("Hotbar9"));

const TArray<Fdemo_mapInputActionDefinition>& Fdemo_mapInputActionRegistry::GetExactDefaultActions()
{
	static const TArray<Fdemo_mapInputActionDefinition> Actions = {
		{ Fdemo_mapInputActionIds::MoveForward, EKeys::W, true, TEXT("向前移动"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::MoveBackward, EKeys::S, true, TEXT("向后移动"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::MoveLeft, EKeys::A, true, TEXT("向左移动"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::MoveRight, EKeys::D, true, TEXT("向右移动"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::PrimaryAttack, EKeys::LeftMouseButton, false, TEXT("普通攻击"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::SkillGroundCircle, EKeys::Q, false, TEXT("范围技能"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::SkillSelfSector, EKeys::E, false, TEXT("扇形技能"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::SkillStraightProjectile, EKeys::F, false, TEXT("直线技能"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall, EKeys::X, false, TEXT("飞剑发射 / 召回"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::ControlledWeaponRedirect, EKeys::C, false, TEXT("飞剑改向"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle, EKeys::T, false, TEXT("切换投掷轨迹"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet, EKeys::MiddleMouseButton, false, TEXT("设定抛投目标方向"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::ThrownWeaponArcApexIncrease, EKeys::RightBracket, false, TEXT("提高抛投弧顶"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::ThrownWeaponArcApexDecrease, EKeys::LeftBracket, false, TEXT("降低抛投弧顶"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::ThrownWeaponArcTargetClear, EKeys::Delete, false, TEXT("清除抛投目标方向"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::SpiritEvasion, EKeys::SpaceBar, false, TEXT("灵息闪避"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::WeaponGuard, EKeys::RightMouseButton, true, TEXT("武器格挡"), TEXT("战斗") },
		{ Fdemo_mapInputActionIds::Interact, EKeys::G, true, TEXT("交互 / 开始搜索"), TEXT("页面与交互") },
		{ Fdemo_mapInputActionIds::ResetRun, EKeys::R, false, TEXT("重置本局"), TEXT("页面与交互") },
		{ Fdemo_mapInputActionIds::Inventory, EKeys::Tab, false, TEXT("人物物品页"), TEXT("页面与交互") },
		{ Fdemo_mapInputActionIds::Back, EKeys::BackSpace, false, TEXT("返回 / 关闭页面"), TEXT("页面与交互") },
		{ Fdemo_mapInputActionIds::Hotbar1, EKeys::One, false, TEXT("快捷使用 1"), TEXT("快捷栏") },
		{ Fdemo_mapInputActionIds::Hotbar2, EKeys::Two, false, TEXT("快捷使用 2"), TEXT("快捷栏") },
		{ Fdemo_mapInputActionIds::Hotbar3, EKeys::Three, false, TEXT("快捷使用 3"), TEXT("快捷栏") },
		{ Fdemo_mapInputActionIds::Hotbar4, EKeys::Four, false, TEXT("快捷使用 4"), TEXT("快捷栏") },
		{ Fdemo_mapInputActionIds::Hotbar5, EKeys::Five, false, TEXT("快捷使用 5"), TEXT("快捷栏") },
		{ Fdemo_mapInputActionIds::Hotbar6, EKeys::Six, false, TEXT("快捷使用 6"), TEXT("快捷栏") },
		{ Fdemo_mapInputActionIds::Hotbar7, EKeys::Seven, false, TEXT("快捷使用 7"), TEXT("快捷栏") },
		{ Fdemo_mapInputActionIds::Hotbar8, EKeys::Eight, false, TEXT("快捷使用 8"), TEXT("快捷栏") },
		{ Fdemo_mapInputActionIds::Hotbar9, EKeys::Nine, false, TEXT("快捷使用 9"), TEXT("快捷栏") }
	};
	return Actions;
}

const Fdemo_mapInputActionDefinition* Fdemo_mapInputActionRegistry::Find(FName ActionId)
{
	return GetExactDefaultActions().FindByPredicate(
		[ActionId](const Fdemo_mapInputActionDefinition& Candidate) { return Candidate.ActionId == ActionId; });
}

FName Fdemo_mapInputActionRegistry::HotbarActionId(int32 ExternalSlotNumber)
{
	const FName Ids[] = {
		Fdemo_mapInputActionIds::Hotbar1, Fdemo_mapInputActionIds::Hotbar2, Fdemo_mapInputActionIds::Hotbar3,
		Fdemo_mapInputActionIds::Hotbar4, Fdemo_mapInputActionIds::Hotbar5, Fdemo_mapInputActionIds::Hotbar6,
		Fdemo_mapInputActionIds::Hotbar7, Fdemo_mapInputActionIds::Hotbar8, Fdemo_mapInputActionIds::Hotbar9
	};
	return ExternalSlotNumber >= 1 && ExternalSlotNumber <= 9 ? Ids[ExternalSlotNumber - 1] : NAME_None;
}

FString Fdemo_mapInputActionRegistry::DisplayLabel(FName ActionId)
{
	if (const Fdemo_mapInputActionDefinition* Action = Find(ActionId))
	{
		return Action->DisplayLabel.IsEmpty()
			? Action->ActionId.ToString()
			: Action->DisplayLabel;
	}
	return ActionId.ToString();
}

bool Fdemo_mapInputActionRegistry::ValidateExactDefaults(FString* OutError)
{
	const TArray<Fdemo_mapInputActionDefinition>& Actions = GetExactDefaultActions();
	if (Actions.Num() != 30)
	{
		if (OutError) *OutError = TEXT("Input registry must contain exactly 30 actions.");
		return false;
	}
	TSet<FName> Ids;
	TSet<FKey> Keys;
	for (const Fdemo_mapInputActionDefinition& Action : Actions)
	{
		if (Action.ActionId.IsNone() || !Action.DefaultKey.IsValid()
			|| Action.DisplayLabel.IsEmpty() || Action.CategoryLabel.IsEmpty()
			|| Ids.Contains(Action.ActionId) || Keys.Contains(Action.DefaultKey))
		{
			if (OutError) *OutError = TEXT("Input registry contains an invalid or duplicate default.");
			return false;
		}
		Ids.Add(Action.ActionId);
		Keys.Add(Action.DefaultKey);
	}
	return true;
}
