# Dev.D.UE.0.0.9B.P43.0.r0 Report

## 1. 结果

P43 已完成：当前已打开、已揭示且 identity-valid 的 P12 BasicCorpse ordinary body-storage 中，一个 canonical non-spatial simple-stack root 可沿既有 normal DragOperation Drop 到既有 GroundDropZone。accepted path 把同一个 ItemId 以一次 P1 whole-root Move 从 exact P11 source 移入新建 P31 record 的 derived WorldDrop container slot 0，并在一个 Owner durable replacement／一次 SaveRecord 中同步替换 P11 与 P6/P31 真值。

实现提交：`520811b feat: add P43 corpse simple-stack ground drop`。

## 2. 文件与职责

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：新增只读、瞬时的 P43 exact body-source proof；不含 WorldDropId、Ordinal、目标或数量草稿。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`：既有 Drag payload 携带 P43 proof；未新增 DragOperation 或输入。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`、`demo_mapCodeBP3UI.cpp`：只在既有 GroundDropZone 请求边缘建立并复验 P43 proof；复用 production Cell、NativeOnDrop 与 GroundDropPresentation。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`、`demo_mapCodeBOutOfRaidProfile.cpp`：新增唯一 P43 P11→P31 writer；在本地 candidate 中派生 record/container identity、执行并重放同一 P1 Move、分区 P11/P6、更新 Registry 并一次保存。
- `Source/demo_map/demo_mapV3ProgressionManager.h`、`demo_mapV3ProgressionManager.cpp`：尸体页挂接既有 GroundDropPresentation；Code A floor adapter 只提供 route/transform，durable accepted 后才刷新 BodyTarget 与 WorldDrop Actor projection。

未修改测试文件、P11/P12 search state machine、P20/P21 materialization/history、P17 construction、P31 schema、P26—P29 pickup writer、P8 terminal/recovery、P5/P6 bridge或 Code A 物品逻辑。

## 3. 唯一输入与调用图

`UCodeBP3CellButton::NativeOnDragDetected` → 既有 `UCodeBP4DragOperation` → 既有 `UCodeBP3GroundDropZone::NativeOnDrop` → `HandleGroundDropZoneDrop` → `UCodeBP3UIHostSubsystem::RequestGroundDrop` → P43 source proof／P2 projection gate → body-page `GroundDropPresentation::RequestDrop` → `RequestCodeBBodyGroundDrop` → existing floor-placement adapter → `DropMatchedRunBodyContainerWorldDropItem` → P1 Move/replay → one P11/P6 Owner replacement／one SaveRecord → accepted-only BodyTarget/Actor projection。

没有新增 Button、hotkey、pointer handler、Widget、Actor click、quantity draft、QuickTransfer、right-click、double-click、Code A inventory writer 或第二 resolver。P43 proof 只在 normal body-source Drop 到 GroundDropZone 时临时建立；普通目标 Cell Drop、Ctrl + 左键、player source、WorldDrop source及其他页面不获得 P43 intent。

## 4. exact P12 source 与 lifecycle gate

- source 必须是当前 `BodyContainerTarget` ordinary root 的 exact ContainerId／SlotIndex／ItemId，`State=Open`、无 active action/search、visibility=`Revealed`，且 root 为 canonical stackable、`MaxStack>1`、Quantity 合法、无 ChildContainer、非装备、非 spatial。
- UI proof 冻结 OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body/open generation、source address/definition/quantity、loot profile/result/materialization digest、workspace pane 与 composite revision；Preview/Commit 前按当前 production projection 重验。
- Store 再从 P11/P6 durable truth 重验相同 receipt/profile/source/revision、BasicCorpse ordinary root、canonical definition、active Run、Prepared/terminal gate与 P31 Registry。
- P21 fixed equipment、P20/P41/P42 spatial parent/child、P17 child、Hidden/Searching、P9/P5/P6 player root、WorldDrop、另一 BodyTarget、split/merge draft、Ctrl intent与任何 stale proof均零写入拒绝，不扫描或回退其他 source/target。

## 5. P1、P31 与一次持久化

- Store 在 validated candidate 内按 `NextWorldDropOrdinal` 派生唯一 WorldDropId 与 WorldContainerId；每次 accepted P43 Drop 新建独立 single-root record，不复用或合并现有 record。
- 新 container 固定为 `WorldDrop` storage、capacity 1、slot 0。唯一 transaction 是 whole-root `Move`，Quantity=0；ItemId、DefinitionId、Quantity 均保持，未创建 ItemId、Split、Merge、Swap、Equip 或中转 P6 格。
- Store 使用同一个 TransactionId/source/target/revision 在第二个瞬时 Repository 中重放该 Move，并要求 replay snapshot 与 accepted composite 完全相等。代码静态计数为一次 candidate Execute、一次 replay Execute、一次 `SaveRecord`。
- accepted composite 按原 P11 container/item ownership 分区：移动 root 进入 P6 world graph；其余 body containers/items、visibility、stable SlotIndex保持。P13 reconcile、payload receipt、P31 Registry validation与 existing-record closure isolation在保存前完成。
- `NextWorldDropOrdinal`、new record、derived container、P11 source removal、P6 snapshot、body revision、session revision与 persistent revision只存在于同一 candidate；SaveRecord failure 不替换 Store truth，因此无 ordinal gap、phantom empty、orphan container、clone或 Actor-first write。
- durability 成功后才重载 production composite、刷新 exact BodyTarget并调用现有 Actor diff；Actor不持有物品权威。

## 6. P26—P29 与既有 record 隔离

P43 record 写入现有 canonical P31 schema：exact OwnerId、RunInstanceId、WorldDropId、Ordinal、WorldContainerId、root ItemId、route/floor transform、Available、RecordRevision=1，以及 provenance `P43.AcceptedGroundDrop.CorpseOrdinarySimpleStack`。

既有 P26/P27/P28/P29 pickup admission按 exact opened record与 `IsP26SimpleStack` 判断，不按 origin 白名单拒绝，因此 P43 simple root自然进入 normal whole pickup、explicit empty-target quantity pickup、compatible-stack quantity pickup与 current-opened-record Ctrl QuickTransfer。P43未改数量规则、target priority、record cleanup、partial retained identity、Actor更新或其他 record。创建前后逐 record closure comparison保证已有 WorldDrop root/container/placement/ordinal/Actor身份不变。

## 7. 静态边界结论

- `git diff --check` 通过；改动仅为 8 个 production C++ 文件；未改测试或 PROJECT 文档。
- 既有 Cell、DragOperation、GroundDropZone、floor adapter、P1 Repository、P11/P6 Owner document、P31 Registry和 Actor diff均被复用。
- P40仍仅由 Ctrl + 左键建立；P43拥有独立 normal GroundDrop proof，不取得 current-child/BaseQuick target或 QuickTransfer语义。
- P20/P21/P38—P42、P17/P19、P12普通尸体→玩家 Drop、P26—P29、P31 existing records、P5/P6/P8/P13/P15与 Code A authority未改产品语义。

## 8. 编译

Editor：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- UnrealBuildTool Result：Succeeded
- Total execution time：26.40 s

Game：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- native exit code：0
- Result：Succeeded
- Total execution time：29.58 s

按 P 阶段约束，未启动产品、PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook或 Package。

## 9. 留给 0.0.9B.F 的真实验证

未执行：future BasicCorpse materialize/open/reveal；ordinary simple stack normal Drag 至 GroundDropZone；multiple existing WorldDrop isolation；new Actor open/close；P26 whole pickup；P27/P28 explicit quantity pickup；P29 Ctrl 双向；wrong/equipment/spatial/child/Hidden/Searching/player/another-body source；BodyTarget/death receipt/profile/Owner/Run/P6 revision stale；floor placement/record/ordinal/container/root mismatch；close/reopen、search/action、Prepared/terminal/Host invalid/SaveRecord failure；P20/P21/P38—P42、P40、P31 other-record isolation、P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook与 Package。

READY_FOR_P44_PLANNING
