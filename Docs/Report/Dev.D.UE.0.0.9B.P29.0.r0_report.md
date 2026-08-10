# Dev.D.UE.0.0.9B.P29.0.r0 Report

## 结果

- 状态：`READY_FOR_P30_PLANNING`
- 实现提交：`aab8a59 feat: add world drop quick transfer`
- P29 已把当前已打开的单一 P14 simple-stack WorldDropTarget 接入共享 P4/P23 Ctrl+左键 QuickTransfer。
- WorldDrop → Player：当前合法 active P17 child 优先，否则 BaseQuick；目标容器内保持 SlotIndex 升序、merge-first / empty-second。
- Player → WorldDrop：只允许 BaseQuick 或当前 active P17 child 的 simple stack 合并到当前打开、身份匹配且未满的唯一 world root。
- 两方向均沿同一 P3 → P2 → P1 → P14/P6 durable callback；每次手势最多一个 P1 candidate 和一次 Owner `SaveRecord`。
- Editor 与 Game 均以原生 exit code 0 完成。
- 未启动产品，未执行 PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook、Package 或试玩。

## 活动调用链审计

### P4/P23 QuickTransfer

1. `UCodeBP3CellButton::NativeOnMouseButtonDown` 仍是 Ctrl+左键唯一 pointer consumption 点；命中后直接调用 `HandleQuickTransfer` 并返回 `Handled`，不继续普通选择、搜索或 drag-threshold 路径。
2. `HandleQuickTransfer` 继续先取消任何 split draft，再经 `BeginP4Drag` 生成稳定 payload；P29 只为此同步手势附加 transient `QuickTransfer` intent、当前 active P17 child identity 与打开的 WorldDrop graph identity。
3. `ResolveQuickTransferDestination` 对 world source 先解析当前明确 active 的 `QuickSpatial`/`PouchInternal`，无 active child 才回退 BaseQuick；对合法 player source 只解析当前页面的 `WorldDropTarget`。非 active child 在 WorldDrop 页面不再落入其他 QuickTransfer 路由。
4. 候选循环未另建旁路：仍先按真实 SlotIndex 升序尝试兼容 occupied stack，再按 SlotIndex 升序尝试 empty cell；第一次合法 preview 后最多提交一次。
5. `PreviewInventoryTransfer` 与 `CommitInventoryTransfer` 共用同一 preview 门；commit 前再次执行 workspace、Owner/Run/revision 与 WorldDrop exact identity 检查。
6. 旧拒绝门是 `PreviewInventoryTransfer` 对任何 WorldDrop target 的绝对拒绝，以及 `HandleQuickTransfer` 对 WorldDrop source 的直接拒绝。P29 将其替换为只允许 exact opened target + simple stack + transient Ctrl intent 的双向受限门，普通 Drag 语义不变。
7. 取消、失焦、关闭、stale revision、Actor 丢失、Run terminal、record/root mismatch 或无合法候选均在 P1 提交前拒绝；已有 split draft 被 Ctrl 手势清除但不消费数量、不写状态。

### P14/P26/P27/P28 与 P1/P2/P3

- P14 record 仍由 P6 Owner document 持有；WorldDropId、derived world container、root ItemId、map route、Available state 与 `NextWorldDropOrdinal` 都在打开和每次写门解析时重验。
- P26 normal full Move/Merge(0)、P27 empty-target Split(N)、P28 occupied-target Merge(N) 的普通真实 Drag 路径保持不变。P29 不创建或消费 `WorldPickupDraft`、`PlayerSplitDraft`、`RequestedMergeQuantity`，也不使用正数量 N。
- P1 仍唯一裁决 Move、Merge(0)、MaxStack、partial acceptance、source retain/delete、revision 与原子失败；P2 只增加 transient command intent/active-child proof，不获得物品真值。
- P3 仍在 P1 accepted 后才调用 durable callback；callback/save 失败时使用既有 BeforeSnapshot 回滚 transient P1 repository 并刷新 accepted projection，未保存数量不会留在 UI。
- P13 继续在 durable candidate 上调用既有 `ReconcileHotbarBindings`：BaseQuick source 完整删除时清理 binding，partial retain 时原 ItemId/SlotIndex/binding 保持；不会把 binding 复制给 world root。
- P8 terminal、P17 graph/access 与 P19 complete-graph branch 未被改写。P29 的 UI 门和 Store proof 都要求无 ChildContainerId 的 simple stack，P19 图不能进入 QuickTransfer。

## 双向行为与 durable delta

### WorldDrop → Player

- 来源必须是当前打开 WorldDropTarget 的 slot 0 revealed root，并与 OwnerId、RunInstanceId、WorldDropId、world container、root ItemId、Available record、ordinal 与当前 P6 revision 一致。
- active P17 child 仅在其仍为当前 P6 投影里的 `QuickSpatial`/`PouchInternal` 时可用；否则使用 BaseQuick。
- occupied compatible target 提交一次 P1 `Merge(Quantity=0)`；P1 partial acceptance 时保留同一 world root ItemId、container、record、WorldDropId 和 ordinal，仅数量变化。
- empty target 提交一次 P1 `Move`；accepted snapshot 证明 root 离开 world container 后，同一个 Owner candidate 删除空 world container 与对应 P14 record，随后既有 Actor projection refresh 移除 Actor。

### Player → WorldDrop

- 来源只允许 BaseQuick，或 command 中携带且与当前 workspace active identity 相同的合法 P17 child；来源必须是正数量、正式 stackable、MaxStack > 1、无 child 的 simple stack。
- 唯一目标固定为当前打开 WorldDropTarget 的 slot 0 root；不创建新 drop，不调用 GroundDropZone，不选择第二 record。
- 只提交一次 P1 `Merge(Quantity=0)`；不兼容、已满、stale 或 identity mismatch 均零写入。
- partial acceptance 只改变 player source 与 world root 的数量；source 全部接受时只清理该 source ItemId/slot 并交由 P13 reconcile，world root/record/container/WorldDropId/ordinal 保持。

### 结构化 proof、单次保存与 rollback

- 新增 `IsExactP29WorldDropQuickTransferDelta`，直接证明已接受 candidate，而不重放第二次 P1。
- proof 要求 command intent 为 QuickTransfer、Quantity=0、exact expected revision、exact source/target stable address、恰好一侧为当前 world container、合法 Base/current-active P17 player side、正式 simple-stack metadata 与 underfull compatible target。
- proof 从 BeforeSnapshot 构造唯一允许的一次 revision delta：world→player empty Move，或任一方向 Merge(0) 的 P1 容量裁决；Candidate 必须逐 Definition、Item、Container 与 Revision 完全相等。
- world partial retain、world full cleanup 与 player→world root retain 分别输出明确结论；其后才执行空 world container/record cleanup、P13 reconcile、receipt freeze、session validation 与一次 `SaveRecord`。
- `NextWorldDropOrdinal` 未写入、未递增；player→world 不生成新 record、新 Actor、新 item 或第二次保存。

## 修改文件与职责

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：增加 transient QuickTransfer command intent 与 exact active-player-container proof 字段。
- `Source/demo_map/CodeB/demo_mapCodeBP3.h`
- `Source/demo_map/CodeB/demo_mapCodeBP3.cpp`：P3 唯一 UI→P2 bridge 传递 transient P29 proof；原 accepted-candidate/rollback 路径不变。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`
- `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`：P4 payload 携带 Ctrl intent/active child，commit 仍只调用一次 P3 bridge。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`：注入 opened WorldDrop exact identity；受限替换旧拒绝门；实现双向 resolver、simple-stack preview 门与 active-child 选择。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：增加 P29 exact delta proof，并在既有单 Owner candidate/SaveRecord writer 中处理两个方向。
- `Source/demo_map/demo_mapV3ProgressionManager.h`
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：保存当前 opened record/container/root/ordinal identity；write gate 在提交前重新打开 P6 session 并校验 Actor、record、route、revision 与 ordinal。

## 明确未修改的权威与边界

- `Source/demo_map/CodeB/demo_mapCodeBInventory.h/.cpp` 未修改：P1 Move/Merge(0)、MaxStack、atomic transaction 与 revision 语义保持唯一权威。
- `Source/demo_map/demo_mapCodeBWorldDropActor.h/.cpp` 未修改：Code A Actor 仍仅持有投影 identity/visual，不持有可写数量或 P6 graph。
- P5/P6 bridge、M01、P8 terminal receipt、P13 产品规则、P15 Use、P17 容量/graph、P19 closure、P20/P21、P9/P11 搜索/尸体、装备、战斗、地图、经济、制作、网络与多人均未修改。
- 未加入 Actor direct pickup、右键/双击领取、Take All、自动目标、自动装备、自动绑定、自动使用、Sort、Compact、stack exchange、exact-N shortcut 或 UI/Actor quantity cache。

## 稳定布局与既有交互

- QuickTransfer 候选只读取既有 P2 projection 的真实 SlotIndex/Capacity；未进行数组重编号、Sort、Compact 或空位压缩。
- 只改 source/target 的 P1 accepted delta；其他 ItemId、ContainerId、SlotIndex、ChildContainer 与布局由 snapshot 全等 proof 保持。
- P3 页面继续沿既有 scroll offset 保存/恢复和 stable-cell rebuild；P29 未增加 scroll 写入或无关 selection 清除。
- 普通左键选择、右键只读详情、双击无领取语义、Actor interaction 仅打开页面、Shift+1—9 Bind 与 P15 Use 均未更改。

## 编译

1. Editor

   `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

   - 原生 exit code：`0`
   - 关键结果：`Result: Succeeded`
   - UHT 完成；`UnrealEditor-demo_map.dll` 链接成功。

2. Game

   `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

   - 原生 exit code：`0`
   - 关键结果：`Result: Succeeded`
   - `demo_map.exe` 链接成功。

## 留给 0.0.9B.F 的真实验证

- 未执行真实 Ctrl+左键输入与焦点/失焦生命周期。
- 未执行 world→active child/Base 的 merge-first / empty-second 运行验证。
- 未执行 player→world compatible/full/incompatible/stale 运行验证。
- 未执行 partial retain、full cleanup、P13 binding、Actor quantity/cleanup、SaveRecord failure rollback 的运行验证。
- 未执行 UI scroll/selection、右键、双击、Actor interaction、Shift+1—9、P15 或 P19 排除项的运行回归。

`READY_FOR_P30_PLANNING`
