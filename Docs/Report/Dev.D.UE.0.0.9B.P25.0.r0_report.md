# Dev.D.UE.0.0.9B.P25.0.r0 Report

## 1. 结论

- 任务：Dev.D.UE.0.0.9B.P25.0.r0
- Prompt：Docs/Prompt/Dev.D.UE.0.0.9B.P25.0.r0_prompt.md
- Prompt SHA-256：C9E270619EA2F704C8A350FA2355230F26E3CBF727C39BEC659CCDC0DCC8B220
- 实现提交：4c11a61 feat: add exact stack merge intents
- 结果：共享精确数量 Merge 与普通 full-stack partial acceptance 已静态闭合；Editor 与 Game 两个指定目标均以 native exit code 0 完成编译。
- 完成信号：READY_FOR_P26_PLANNING

本任务没有启动产品、PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook、Package 或试玩；F 阶段真实验证全部保留到 0.0.9B.F。

## 2. 静态路径审计

### 2.1 P1/P2 Merge 权威链

审阅结论：

1. FCodeBTransactionRequest 与 FCodeBP2Command 已有 Quantity 字段。Quantity 大于 0 时，P1 ExecuteMerge 将其视为精确接受量；Quantity 为 0 时，P1 使用 min(SourceQuantity, Available) 裁决普通 full-stack Merge 的正式接受量。
2. P1 在 candidate state 中先验证来源位置、普通储物目标、不同 ItemId、相同 DefinitionId、bStackable、MaxStack 和 Available，再同时修改目标与来源 Quantity。
3. 来源余量大于 0 时，P1 保留来源 ItemId、ParentContainerId 与 SlotIndex；来源归零时，P1 只清空其原 Slot 并移除该来源实例。目标 ItemId 始终不变。
4. ExecuteTransaction 只在 candidate 通过 ValidateState 后推进一次 Repository Revision；失败保留旧 State 与旧 Revision。
5. P2 Apply 只把 FCodeBP2Command 转换为唯一 FCodeBTransactionRequest，不实现第二 Merge，不直接写 Item 或 Container。

P25 没有修改 P1 Merge 引擎；它只让共享工作台正确表达 P1 已存在的两种 Quantity 语义。

### 2.2 P4/P23/P24 共享工作台

审阅了共享 Cell、FCodeBP4DragPayload、FCodeBP4DropPreview、modifier router、stable Slot address、SplitDraft、数量输入、取消与 stale 清理、P5/P6 workspace context。

发现并收敛的缺口：

- P24 的 SplitDraft 只能落到空格；已占用目标直接拒绝。
- 普通 full-stack Drag 在目标容量小于来源 Quantity 时由 P4 提前拒绝，没有把 partial acceptance 交给 P1。
- P4 通过独立 item catalog/hard-coded fallback 推测 MaxStack，未直接使用当前 P1 projection 的正式定义元数据。
- 跨 P6↔P9/P11 durable callback 已严格识别 P24 Split 新身份，但没有把数量变化显式证明为一次 P1 Merge。

### 2.3 P5、P6、P9、P11 持久化边界

- P5 内图仍使用 CommitAcceptedSnapshot。
- P6 内图仍使用 CommitAcceptedActiveRunInventorySnapshot。
- P6↔P9 仍使用 CommitAcceptedMatchedRunNormalContainerTransfer。
- P6↔P11 仍使用 CommitAcceptedMatchedRunBodyContainerTransfer。
- P9/P11 页面仍装载 P6 与精确目标的 composite repository；P1 在该 composite 上完成一次 accepted transaction，随后 durable callback 在一个 Owner document candidate 中验证、分区、Hotbar reconcile 和保存。
- Progression Manager、Code A Run/地图/玩家/战斗/搜索权威均未修改。

## 3. 实际修改文件

### 3.1 已修改

1. Source/demo_map/CodeB/demo_mapCodeBP2.h
   - 为只读 Slot projection 增加 bStackable 与 MaxStack。
   - Widget/P4 只读这些字段，不获得 Definition 或库存写权限。

2. Source/demo_map/CodeB/demo_mapCodeBP2.cpp
   - 从 P1 FCodeBItemDefinition 投影正式 bStackable/MaxStack。
   - 把字段纳入 projection equality，确保 accepted refresh 能观察定义变化。

3. Source/demo_map/CodeB/demo_mapCodeBP3.h
   - 更新 SplitDraft 文档语义：同一 transient quantity intent 可落到空格 Split 或既有堆叠的 exact Merge。
   - Draft 本体仍只有身份、revision、来源 stable address 与 RequestedQuantity。

4. Source/demo_map/CodeB/demo_mapCodeBP4.h
   - Payload 保留完整来源 Quantity，并新增只读 RequestedMergeQuantity。
   - 数量 Draft payload 同时携带稳定 GraphIdentity；没有新 ItemId、可写 Item copy、预扣数量或目标数量副本。
   - Preview 增加 ProjectedAcceptedQuantity 与 bPartialAcceptance，仅用于当前 snapshot 的 transient 提示。

5. Source/demo_map/CodeB/demo_mapCodeBP4.cpp
   - 移除独立 catalog/hard-coded MaxStack 推测，改用 P2 从 P1 投影的正式 stack 元数据。
   - SplitDraft 落到兼容未满堆叠时生成 exact Merge preview，只有 Available 大于等于 N 才允许。
   - exact Merge 将 N 原样传入 P3/P2/P1；不足时拒绝，不截断，不先 Split，不新建 ItemId。
   - 普通 full-stack Merge 将 Quantity 0 传给 P1，并显示完整接收或部分接收投影。

6. Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp
   - 数量拖拽显示 RequestedMergeQuantity，而来源完整 Quantity 保持独立。
   - Draft 消费时把稳定 GraphIdentity 放入 payload；stale 图身份、Owner、Run、scope 或 revision 均拒绝。
   - 更新数量输入与 WorldDrop 反馈；Ctrl+左键、Shift+1—9、right-click、double-click 与滚动路由未改。

7. Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp
   - 增加 IsExactP25MergeDelta，只接受结构上等同单次 P1 ExecuteMerge 的跨图数量差。
   - partial Merge 必须只改变一对来源/目标 Quantity，两个 ItemId 与两个 placement 都保持不变。
   - full Merge 只允许来源归零、来源 Slot 清空与来源 ItemId 删除；目标只增加同量且不超过 MaxStack。
   - 禁止新 ItemId、新 Container、Definition 变化、无关 Item/Container 变化、装备格、ChildContainer 或多来源/多目标数量变化。
   - P6↔P9 与 P6↔P11 均在既有单 Owner-document durable commit 中应用该证明。

### 3.2 明确未修改

- Source/demo_map/CodeB/demo_mapCodeBInventory.h/.cpp：P1 Merge 已满足 exact/full-partial 语义，无需重写。
- Source/demo_map/CodeB/demo_mapCodeBP3.cpp：P3 CommitP4Operation 已能把 Quantity 一次传入 P2，并在 durable callback 失败时恢复 BeforeSnapshot。
- Source/demo_map/demo_mapV3ProgressionManager.h/.cpp：既有 P5/P6/P9/P11 callback、Owner/Run/target identity 和打开页面生命周期保持原样。
- Code A 物品、Run、地图、玩家、战斗、敌人、搜索、结算、WorldDrop、P5→P6 bridge、P8 receipt、P15 Use、P17 graph、P20/P21 来源均未修改。

## 4. 完整 call graph

精确数量 Merge：

    Split action / quantity input
      -> UCodeBP3UIHostSubsystem::CreateSplitDraft
      -> stable Source + OwnerId + RunInstanceId + GraphIdentity + ExpectedRevision + N
      -> UCodeBP3UIHostSubsystem::BeginInventoryDrag
      -> FCodeBP4InteractionController::BeginSplitDrag
      -> FCodeBP4DragPayload(RequestedMergeQuantity=N)
      -> UCodeBP3InventoryWidget::PreviewInventoryTransfer
      -> FCodeBP4InteractionController::PreviewDrop
      -> exact target / definition / MaxStack / Available >= N validation
      -> FCodeBP4InteractionController::CommitDrop
      -> FCodeBP3UIController::CommitP4Operation(Merge, Quantity=N)
      -> FCodeBP2ApplicationService::Apply
      -> FCodeBRepository::ExecuteTransaction
      -> FCodeBRepository::ExecuteMerge
      -> candidate ValidateState + one Repository revision
      -> existing ProfileCommit callback
      -> P5/P6 single graph commit or P9/P11 composite commit
      -> durable revision/identity validation
      -> IsExactP25MergeDelta for cross-graph quantity changes
      -> partition + ReconcileHotbarBindings + one SaveRecord
      -> fresh authoritative projection

普通 full-stack Drag：

    BeginDrag(Quantity=authoritative source quantity, RequestedMergeQuantity=0)
      -> PreviewDrop computes display-only Accepted=min(Source, Available)
      -> CommitP4Operation(Merge, Quantity=0)
      -> P2 Apply
      -> P1 ExecuteMerge chooses the formal accepted amount
      -> same accepted callback / rollback / projection chain

没有 UI direct mutation、先 Split 后 Merge、预建 ItemId、自动 target、自动 compact、自动截断 N 或类别专用 Merge handler。

## 5. 精确数量 Intent 与拒绝规则

- 来源资格继续由 ValidateSplitSource 验证：普通储物格、bStackable、MaxStack 大于 1、Quantity 大于 1、无 ChildContainerId、1 <= N < SourceQuantity。
- Payload 的 Quantity 是 drag 开始时权威来源总量；RequestedMergeQuantity 才是一次性 N。两个字段不会互相覆盖。
- exact Merge 目标必须是明确 stable address、非同源、普通储物、已占用、同一正式 DefinitionId、正式 bStackable/MaxStack 一致、未满且 Available >= N。
- P9/P11 Hidden 或 Searching 格在共享 PreviewInventoryTransfer 外层直接拒绝；未 Open/身份 stale/target stale 最终也由 durable callback 拒绝。
- WorldDrop、装备位、Hotbar 引用槽、空间 parent、ChildContainer、外部到外部、UI 外区域均不接受数量 Merge。
- Available < N 时只返回拒绝；不会改成较小 N，不会创建临时 split stack。
- Esc、取消、blur、页面关闭、write gate 变化、另一项 drag、source stale、revision stale 和 graph identity 变化沿用 P24 清理路径，零写入。

## 6. ItemId、数量和稳定布局

### 6.1 exact Merge

- Source ItemId：保持。
- Target ItemId：保持。
- Source Quantity：恰好减少 N。
- Target Quantity：恰好增加 N。
- 新 ItemId：0。
- Container/SlotIndex：全部保持。
- 无关 item/container：结构校验要求完全相等。

### 6.2 full-stack Merge

- P1 接受量为 min(SourceQuantity, Available)。
- 来源有余量：同一来源 ItemId、同一 SlotIndex、同一 ContainerId 保留。
- 来源归零：只清空原 source slot，并按 P1 既有语义删除该来源实例。
- Target ItemId 与 placement 始终保持。
- 不 Sort、不 Compact、不重编号、不重建空间区、不重置无关 scroll offset。

P2 的 MaxStack/bStackable 来源是当前 P1 Definition projection，不再通过显示名称、Widget 文本或硬编码类型猜测。

## 7. 原子提交、Revision 与 Hotbar

### 7.1 P5 / P6 内图

- P1 candidate 接受后只调用一个既有 durable callback。
- callback 失败时 PersistProfileSnapshotAfterAcceptedP1 把 repository 恢复到 BeforeSnapshot，并刷新 projection。
- P5/P6 没有第二保存、先扣来源或 UI-owned quantity。

### 7.2 P6↔P9 / P6↔P11

- composite snapshot 先由 P1 完成一个 Merge revision。
- durable callback 重新核对 OwnerId、RunInstanceId、exact target、Open state、无 active search action、P6 revision 与 target revision。
- IsExactP25MergeDelta 证明数量差只能是一对正式兼容堆叠的一次 Merge。
- 玩家/目标 graph 在同一个 Owner document candidate 中分区；P6 session、target record、PersistentRevision 与时间戳共同保存或全部不保存。

### 7.3 Hotbar

- 来源仍有 Quantity：同一 ItemId 仍存在，P13 binding 保持。
- full Merge 消耗来源：ReconcileHotbarBindings 清理已不存在的来源引用。
- Target ItemId 不继承来源 binding。
- Merge 不自动绑定、不自动使用、不修改其他 binding。

## 8. 输入语义核对

- 普通左键：仍只选择。
- Ctrl + 左键：仍取消 SplitDraft 并执行完整堆 QuickTransfer 候选顺序；不弹数量框、不创建 Draft。兼容未满 target 的 accepted amount 仍由同一 P1 Merge 裁决。
- Shift + 1—9：仍只调用 P13 Bind，不移动、不 Merge、不使用。
- right-click：仍为只读详情/上下文入口。
- double-click：仍只选择，不恢复旧 QuickMove。
- details、scroll、I、Tab、Esc、close：没有新增数量或位置写入；取消路径清理 Draft/preview。
- 明确 Drop：只有 normal full-stack Merge 或已确认 N 的 exact Merge 才能写入。

## 9. WorldDrop 与未实现范围

P14 WorldDrop 继续保持完整图语义；数量 Draft 在 RequestGroundDrop 入口被拒绝。P25 没有接入世界 Actor、地面拾取、随机地面 Loot、装备位、空间 parent、child 操作或自动拾取。

未实现：多格物品、旋转、stack exchange、自动整理、自动 target、Take All、按比例自动分割、右键/双击快速移动、Alt 自动装备、自动绑定、自动使用、额外 1—9 效果、分类筛选或仓库搜索工具。

## 10. 编译

### 10.1 Editor

命令：

    "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

结果：

- native exit code：0
- UnrealBuildTool：Result: Succeeded
- 目标：demo_mapEditor Win64 Development
- 输出：C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe

### 10.2 Game

命令：

    "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

结果：

- native exit code：0
- UnrealBuildTool：Result: Succeeded
- 目标：demo_map Win64 Development
- 输出：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Binaries\Win64\demo_map.exe

## 11. 未执行的 F 阶段验证

以下均未执行：产品启动、PIE、Standalone、实际拖拽/键盘输入、截图巡检、Smoke、Automation、回归、试玩、Cook、Package、存档破坏测试、视觉确认与最终验收。

这些验证统一保留到 0.0.9B.F。

READY_FOR_P26_PLANNING
