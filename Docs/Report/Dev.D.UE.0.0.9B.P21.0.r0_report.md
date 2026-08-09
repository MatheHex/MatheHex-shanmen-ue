# Dev.D.UE.0.0.9B.P21.0.r0 Report

## 状态

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

P21 功能开发、静态边界审查与规定的 Editor 编译已完成。未进行游戏运行、自动化、截图、Smoke、回归、Game Build、Cook 或 Package；这些真实验证债务全部保留给 `0.0.9B.F`。

## 文件与职责

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.{h,cpp}`：新增 future-only `CodeB.LootProfile.BasicCorpse.r3` 与 `CodeB.DeterministicWeightedLoot.Crc32.r3`，P11 固定尸体装备位、receipt/JSON/schema r3 provenance、deterministic graph 复核，以及 P12 原子 P11→P6 提取等价校验。
- `Source/demo_map/CodeB/demo_mapCodeBInventory.cpp`：P1 仅对 `CodeB.Body.*` 的尸体 equipment source 允许 `Move` 到 storage；普通玩家 equipment 仍只可 `Unequip`。P12 durable commit 进一步限定这个例外，P1 不成为任意尸体写入入口。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.{h,cpp}`：P12 只读投影三个固定尸体 equipment section；Hidden/Searching 继续掩码；NativeOnDrop 仅允许揭示的尸体装备到空 P6 BaseQuick，拒绝回存、直接装备及其余目的地。
- `PROJECT_INFO_CARD.md`：更新到 P21 和 F 阶段验证债务。

## BasicCorpse r3 与未来范围

- `BasicCorpse.r3` 只由最新 Profile 查找用于尚未 materialize 的 BasicCorpse；已有 r1/r2 receipt 继续按其 own profile id/version/digest 读取、验证和重放。BasicCache r1/r2 未改。
- r3 原样继承 r2 的 `Guaranteed.Main`：IronShard（weight 3，qty 1--2）与 SpiritDust（weight 2，qty 1）；以及 `Optional.SpatialUtility`：NoDrop 9 / Spawn 1，WindTalisman weight 1、BackpackLevel1 weight 3。
- r3 添加 `Optional.EquippedLoadout`：NoDrop 4 / Spawn 1、selection 1、weight 均为 1；DefinitionId 字节序为 `Prototype.Item.Accessory.EvasionCharm`、`Prototype.Item.Armor.ReinforcedVest`、`Prototype.Item.Weapon.HeavyPracticeBlade`，分别绑定 `Body.Accessory0`、`Body.ArmorRobe`、`Body.Weapon`。三项均为现有正式 Code B canonical definition、非 stackable、qty 1、无 child container。
- deterministic identity 覆盖 OwnerId、RunInstanceId、BodyTargetId、source DefinitionId、DeathReceiptId、profile id/version/digest、algorithm 与 equipment candidate-set digest；stable item/container IDs 和 result digest 因此保持同 identity 可重放。

## P11 物质化、持久化与 P12 取得

- r3 P11 graph 固定包括原材料 root 和三个 capacity-1 P1 equipment containers：`CodeB.Body.Weapon`、`CodeB.Body.ArmorRobe`、`CodeB.Body.Accessory0`。无命中时三格全空；命中时仅一个匹配格持有同一真实 item。P11 record schema 升至 4，仍可读取 r1/r2/r3 历史 schema；r3 receipt 单独持久化 candidate-set digest。
- record/receipt validation 复核 exact container identities、slot semantic、capacity、item definition/qty/no-child、visibility coverage、candidate provenance、initial deterministic graph 和 receipt digest。r1/r2 不含 candidate digest，保留其既有 graph/receipt 形状。
- P12 在 UI 和 durable commit 双层拒绝受保护项、非 BaseQuick 目标、occupied BaseQuick、direct equip、merge、swap、return-to-corpse、clone/new identity 与任何 P6→尸体装备位写入。
- 已接受的 r3 取得必须等于同一 P1 composite 中从固定尸体 equipment slot `0` 到 P6 BasicContainerId 空 slot 的一个 `Move`。随后通过同一 Owner sidecar replacement 原子写入 P11/P6；失败在 Save 前返回，不能半提交。

## 静态边界结论

- 新行为位于 `Source/demo_map/CodeB`；没有修改 Code A 战斗、敌人死亡、Actor、地图、Run、Loot、旧库存、SaveGame、结算或 UI 权威路径。
- P12 presentation 仅扩展既有 Code B Host 的 transient containers；没有引入第二份 inventory truth、widget-local inventory、QuickMove 或 P7 直接装备入口。
- 现有 P20 空间 root/child closure 路径与普通 P12 simple-item 路径未被改写；r3 equipment 的额外限制只在其固定 `CodeB.Body.*` source containers 生效。

## 编译

规定目标：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex
```

- 首次执行定位到 `EquipmentCandidateSetDigest` 误归属普通容器 receipt 的编译错误；该错误已做最小移动修正至 body receipt。
- 修正后按相同目标再次编译：exit code `0`，UBT `Result: Succeeded`，4 actions（`demo_mapCodeBOutOfRaidProfile.cpp` 编译、Editor lib/dll 链接、target metadata）。

## 明确保留给 0.0.9B.F

- BasicCorpse r3 的真实死亡、首次 materialize、NoDrop/Spawn、固定 equipment 显示、揭示、P12 Move、重开/中断恢复及 P8 terminal 结果。
- 自动化、真实 Slate/UMG 输入、截图、Smoke、回归、Game Build、Cook、Package 和最终验收。
