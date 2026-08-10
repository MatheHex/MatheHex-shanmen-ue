# Dev.D.UE.0.0.9B.P38.0.r0 Report

## 状态与范围

P38 已静态闭合：当前 Open/Revealed/identity-valid P21 尸体装备 root 可经既有 production Cell 的普通 Drag，直接落到用户明确命中的空 BaseQuick、当前合法 P17 child 空 ordinary cell 或 compatible empty formal Weapon/Armor/Accessory cell。三条路径均为同一个 ItemId 的单次 P1 `Move(1)`，没有 target resolver、fallback、Swap、Replacement、auto-equip 或临时 BaseQuick staging。

本轮仅实现、静态审查和编译；未启动产品、PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook 或 Package。

## 文件与职责

新增文件：仅本 Report。

修改文件：

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：增加不持久化的 `FCodeBP38BodyEquipmentTransferProof`，冻结 Owner/Run、BodyTarget/death receipt、body revision、page generation、r3 profile/digest、slot semantic、pane route 及当前 child/parent/generation。
- `Source/demo_map/CodeB/demo_mapCodeBP4.{h,cpp}`：识别三个 exact `Body.*` source role；formal empty equipment 仍规划为 `Move(1)`，occupied target 拒绝；普通空格也是 `Move(1)`。
- `Source/demo_map/CodeB/demo_mapCodeBP3.{h,cpp}`：把同一瞬时 proof 随既有 accepted P2 command 送入 durable callback；BeforeSnapshot/P1/P2/selection 恢复链未改变。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.{h,cpp}`：复用既有 Cell/DragOperation/NativeOnDrop；生成并重验 source proof、body page/focus generation、三类 explicit target、current-child parent topology；P21 source 的 Ctrl QuickTransfer 明确拒绝。
- `Source/demo_map/CodeB/demo_mapCodeBInventory.cpp`：P1 对 `CodeB.Body.*` source 的一次 `Move(1)`增加 compatible empty formal Weapon/Armor/Accessory target；普通 equipment 仍要求 Equip/Unequip，P38 不允许 replacement。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.{h,cpp}`：P11/P6 durable callback 接收 accepted command；重建 exact r3 source、exact target category 与唯一 P1 candidate，再进行原有单 Owner replacement/save。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：仅把既有 P3 commit callback 已收到的只读 `AcceptedCommand` 转发给 Code B Store；未增加 Code A item/container/quantity/loot 权威或写入逻辑。

未修改：P21 roll/profile/schema history，P12 search state machine，P17 graph construction，P5 warehouse/P6 bridge，P8 terminal policy，P13 binding/P15 use，P14/P31 WorldDrop schema/actor，Code A 战斗、死亡、尸体 Actor、地图、Run 与旧库存逻辑，PROJECT 文档及所有测试文件。

实现提交：`00a9020 feat: add P38 corpse equipment explicit targets`。

## P21 r3 实际来源与历史边界

- Profile：`CodeB.LootProfile.BasicCorpse.r3`，version `3`；algorithm 保持 `CodeB.DeterministicWeightedLoot.Crc32.r3`。proof/Store 对 receipt 中 exact `LootProfileDigest` 与 `EquipmentCandidateSetDigest` 做相等重验，不生成或改写 digest。
- `Body.Weapon` → `Prototype.Item.Weapon.HeavyPracticeBlade` → equip `Weapon` → P1 container type `CodeB.Body.Weapon`。
- `Body.ArmorRobe` → `Prototype.Item.Armor.ReinforcedVest` → equip `Armor` → P1 container type `CodeB.Body.ArmorRobe`。
- `Body.Accessory0` → `Prototype.Item.Accessory.EvasionCharm` → equip `Accessory` → P1 container type `CodeB.Body.Accessory0`。
- 三项仍是 P21 candidate-set 中最多一个真实、non-stackable、Quantity=1、MaxStack=1、无 ChildContainerId、无 spatial semantic 的 root。eligibility 来自 P11 receipt/profile/catalog/snapshot，不来自显示文本、图标、Actor 或位置顺序。
- r1/r2、已 materialized record、r3 deterministic item/container identity、Optional.EquippedLoadout 权重与 candidate-set 均未修改；历史 record 不能携带 P38 proof。

## 唯一输入与调用图

`UCodeBP3CellButton::NativeOnMouseButtonDown(DetectDrag)` → `NativeOnDragDetected` → `UCodeBP3InventoryWidget::BeginP4Drag` → `UCodeBP3UIHostSubsystem::PopulateTransferContext` → 既有 `UCodeBP4DragOperation` → target Cell `NativeOnDrop` → `HandleP4Drop/PreviewInventoryTransfer` → `ValidateTransferContext + ValidateP38BodyEquipmentTransferContext` → `FCodeBP4InteractionController::PreviewDrop/CommitDrop` → `FCodeBP3UIController::CommitP4Operation` → `FCodeBP2ApplicationService::Apply` → `FCodeBRepository::ExecuteTransaction/ExecuteMove` → 既有 P3 profile callback → `CommitAcceptedMatchedRunBodyContainerTransfer` → one P11/P6 replacement + `SaveRecord` → projection/selection restore。

没有新增 button、hotkey、pointer handler、Widget inventory、Actor pickup、right-click Take、double-click、QuickTransfer resolver、第二 P1 transaction、第二 save、预建 ItemId 或 Code A inventory writer。普通 click/right-click/快捷键/close/cancel/focus loss 保持只读或取消；focus loss/close 递增或销毁 page generation，使旧 payload 失效。

## Source、session 与 lifecycle gate

- input/preview/commit 共同要求：active InRun P12 Host、exact Open BodyTarget、无 active action/search、exact OwnerId/RunInstanceId/BodyTargetId/DeathReceiptId/body revision/P6 composite revision、`InRun.External` pane route、非零 page/focus generation。
- source 必须是上表 exact semantic/container 的 slot 0、Projection 与 P1 中同一 ItemId/Definition、Revealed、Quantity=1、MaxStack=1、non-stack/no-child；Hidden、Searching、empty、ordinary corpse item、player/P9/P14/P31/another BodyTarget source 均不能建立 P38 intent。
- payload 只携带只读身份与 revision；命令固定 `Intent=Standard`、`Operation=Move`、`Quantity=1`、legacy quick mode。Ctrl、Split/Merge、Quantity=0、WorldPickupDraft/PlayerSplitDraft 均拒绝。
- Store 在写盘前重新打开 exact committed active P6 session，重验 Prepared/terminal gate、P6/body revisions、Open/Reveal、receipt/profile/candidate digest、source address、command address及 candidate；Host/page/route/focus/child generation stale 会在 P1 前拒绝。

## 三类 exact target

1. BaseQuick：target ContainerId 必须等于 active P6 `Layout.BasicContainerId`，role `Basic6`，实际 Drop SlotIndex 存在、ordinary、empty；不扫描首空格。
2. Current P17 child：target 必须等于 pointer-down proof 与当前 Workspace 的 exact child ContainerId/open generation；唯一 parent ItemId 必须匹配，parent 位于 formal `SpatialRing` 或 `Backpack`，Store 以 canonical P17 definition、one-layer/no-cycle、dynamic capacity 和 ordinary contents 重建 topology；close/switch/focus/generation/parent/capacity/slot stale 均拒绝且不回退。
3. Formal equipment：target 必须是 active P6 layout 中 exact Weapon、Armor 或 Accessory container，slot 0 empty；P4 projection与 P1 Catalog equip slot 必须 compatible，Store 以 `IsP32ActiveStandardEquipmentContainer` 再验 layout identity。SpatialRing、Backpack、Hotbar、warehouse、ordinary/body/world container 与 occupied/incompatible cell 均拒绝。

所有 target 都来自用户实际 Drop 命中的 production Cell stable ContainerId/SlotIndex；没有 first/nearest/selected scan、auto-open、auto-target、fallback、Swap、Replacement、auto-equip 或先落 BaseQuick 再移动。

## 原子性、回滚与非回归

- accepted command 只执行一次 P1 whole-root `Move(1)`；Store 从原始 P6+P11 composite 用同一 transaction/source/target 重放，并要求 resulting snapshot 与 accepted candidate 全等。ItemId、DefinitionId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、candidate provenance、无 child 均保持，唯一变化是 parent/slot。
- partition 后 P11 source 不再含该 root、P6 exact target 含同一 root，随后 P13 仅做既有 reconcile；P11 record 与 P6 session 在同一 Owner candidate 中各升一次 revision并由一个 `SaveRecord` replacement 持久化。P8 仍只结算 P6 player graph，P11 residual/terminal规则未变。
- P3 保存失败使用既有 BeforeSnapshot 恢复；任何 source/target/P1/candidate/reconcile/save 失败都在 durable replacement 前返回，不能留下 phantom empty/equipment/copy。没有 WorldDrop、ordinal、Actor、child、binding 或第二 receipt 写入。
- P12 ordinary corpse Move/Merge/Swap、P17/P19/P26—P37/P31、P21 P6→corpse 拒绝、P5/P6/P8/P13/P15 与 Code A authority 均未扩展。成功后的既有 P3 refresh/selection restore 只更新 accepted graph；未增加 Sort/Compact，stable SlotIndex 与 scroll policy 未改。

## 静态审查与编译

- `git diff --check`：exit code `0`。
- Editor：`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`；最终 native exit code `0`，`Result: Succeeded`。首次编译只发现 manager callback 参数仍匿名；局部命名修正后成功。最终 Accessory0 静态修正后的增量编译亦为 exit `0`。
- Game：`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`；最终 native exit code `0`，`Result: Succeeded`；最终增量编译 exit `0`。

## 留给 0.0.9B.F 的真实验证

- 真实 P21 r3 future corpse materialize/reveal；Weapon/ArmorRobe/Accessory 各自真实拖至空 BaseQuick、valid current child empty ordinary cell、compatible empty formal equipment slot。
- wrong/incompatible/occupied target；Hidden/Searching/closed/reopened body；child close/switch/focus/generation/parent/capacity stale；BodyTarget/death receipt/Owner/Run/P6 revision stale；Prepared/terminal/Host invalid/SaveRecord failure。
- P21 ordinary corpse rule、P6→corpse 拒绝、P36/P37 Ctrl branches、P12 normal Move/Merge/Swap、P14/P31 other-record isolation、P8 terminal/recovery、P13 reconcile。
- 真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook、Package 与最终验收。

READY_FOR_P39_PLANNING
