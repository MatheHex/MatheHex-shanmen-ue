# Dev.D.UE.0.0.9B.P24.0.r0 Report

## 1. 任务与结论

- Task：`Dev.D.UE.0.0.9B.P24.0.r0`
- Prompt：`Docs/Prompt/Dev.D.UE.0.0.9B.P24.0.r0_prompt.md`
- Prompt bytes：`15638`
- Prompt SHA-256：`111FFB4D3C6B78894AA7B9C58CE371B31A16AE709B690974505CA0AB9B99C6C2`
- 实现 commit：`ed42391 feat: add shared atomic stack splitting`
- P 阶段结论：共享 Split 功能已完成静态闭合；最终 Editor 与 Game 编译均为 native exit code `0`。
- 完成状态：`READY_FOR_P25_PLANNING`

## 2. 改动文件与职责

### 实际修改

- `Source/demo_map/CodeB/demo_mapCodeBP3.h`
  - 新增最多一个活动项的 transient `FCodeBP3SplitDraft`；只保存 workspace/source scope、Owner、可选 Run、graph identity、来源 stable address/ItemId、expected revision 与请求数量。
  - 新增权威只读的 Split 来源资格检查声明。
- `Source/demo_map/CodeB/demo_mapCodeBP3.cpp`
  - 从当前真实 P1 Repository 校验来源 placement、普通储物容器、stackable、MaxStack、Quantity 与无 ChildContainer；不执行写入。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`
  - 为共享 drag payload 增加 `bSplitIntent`，为 preview 增加 `Split` 类型，并声明一次性 split drag 入口。
- `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`
  - 将已确认 Draft 转换为不含新 ItemId 的 split payload。
  - Split preview 仅接受明确、空、非装备普通储物格；最终 commit 继续进入 P3/P2/P1。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`
  - 增加共享数量输入、Draft 创建／消费／取消、Esc／失焦清理接口；没有新增物品图或库存镜像。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
  - 详情区仅对可拆分 selection 显示 `拆分` 动作；动作先打开 transient 数量输入，确认后才创建 Draft。
  - 统一显示“拆分拖拽 N”；普通 drag、Ctrl QuickTransfer、Shift bind、右键详情、双击与滚动仍走原路径。
  - Esc、页面关闭、窗口失焦、gate/read-only、stale、来源变化、另一项拖拽与取消会清理 Draft/输入；P14 ground drop 明确拒绝 split payload。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
  - 为 P6↔P9 与 P6↔P11 的既有单 Owner 文档提交边界增加窄 P24 adapter：只有候选图与 P1 `ExecuteSplit` 完全同构，才允许恰好一个新身份。
  - 普通容器新进入的 split item 标记为 `Revealed`；尸体容器继续使用既有 visibility 重建并将玩家来源新项标记为 `Revealed`。
  - 两条提交均继续在保存前执行 Hotbar reconcile，并以单次 Owner record replacement 原子持久化。
- `Docs/Report/Dev.D.UE.0.0.9B.P24.0.r0_report.md`
  - 本报告。

### 审阅但未修改

- `Source/demo_map/CodeB/demo_mapCodeBInventory.h/.cpp`：P1 Split 已满足最终接受时创建 ItemId、数量验证、candidate-state 原子提交、失败零写入和 Revision `+1`，无需修改。
- `Source/demo_map/CodeB/demo_mapCodeBP2.h/.cpp`：P2 已完整转发 Split request/result，未复制规则。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：P5、P6、P10、P12 已通过共享 Host 与既有 durable callback 进入同一边界；无需新增类型专用 handler。
- `Source/demo_map/CodeB/*Tests*`、`demo_map.Build.cs`、Code A 战斗／Run／地图／Player／搜索来源文件：未修改。

## 3. 静态路径审计

### P1/P2 Split 现状

1. `FCodeBP2ApplicationService::Apply` 将 `FCodeBP2Command` 转成 `FCodeBTransactionRequest`。
2. `FCodeBRepository::ExecuteTransaction` 在 candidate snapshot 上调用 `ExecuteSplit`。
3. `ExecuteSplit` 校验来源 identity/placement、非装备容器、Definition `bStackable`、`1 <= Quantity < source Quantity`、明确空 target storage 与 Revision。
4. 所有校验通过后才调用 `FGuid::NewGuid()` 创建目标实例；来源保留 ItemId 并扣减数量。
5. candidate invariants 通过后才替换 repository state，并将 Revision 增加一次；失败保持原图、原数量和原 Revision。
6. `FCodeBTransactionResult::CreatedItemId` 只表达已接受结果；P24 UI/Draft/payload 均不预创建或保存该值。

### 共享 UI 到唯一提交路径

```text
详情 Split 动作
  -> transient quantity input
  -> UCodeBP3UIHostSubsystem::CreateSplitDraft
  -> FCodeBP3UIController::ValidateSplitSource（只读 P1）
  -> FCodeBP3SplitDraft（无新 ItemId、无可写 item copy）
  -> UCodeBP3InventoryWidget::BeginP4Drag
  -> UCodeBP3UIHostSubsystem::BeginInventoryDrag（一次性消费 Draft）
  -> FCodeBP4InteractionController::BeginSplitDrag
  -> PreviewDrop（只接受明确空普通储物格）
  -> CommitDrop
  -> FCodeBP3UIController::CommitP4Operation(ECodeBOperation::Split)
  -> FCodeBP2ApplicationService::Apply
  -> FCodeBRepository::ExecuteTransaction / ExecuteSplit
  -> durable callback；失败则 P3 恢复 BeforeSnapshot
```

没有 Widget direct mutation、按物品类型分叉、预扣来源、预创建 placement、自动 target 或第二份数量真值。

## 4. SplitDraft、数量与取消证据

- Draft 仅为 Host/Workspace 生命周期内的 `TOptional<FCodeBP3SplitDraft>`；同一页面最多一个，新建时替换旧 Draft。
- 数量严格由权威来源检查为 `1 … Quantity - 1`；入口还要求 visible、writable、stackable、MaxStack > 1、普通 storage、无 ChildContainer。
- 数量输入打开和 Draft 确认均不调用 P2/P1；只有 split payload 的合法 Drop 才提交。
- Draft 不含新 ItemId、修改后的 Quantity、item instance copy 或 target placement。
- Esc、取消、窗口 blur、关闭、read-only/gate 拒绝、revision stale、来源消失、身份变化与另一个 drag 会清理 transient 状态并零写入。
- Drop 失败或 durable callback 保存失败时，P3 使用操作前 snapshot 回滚；跨图 Store 在 `SaveRecord` 前只操作 Owner-record candidate。

## 5. 原子提交边界与 Revision

### P5 内图

- P5 仍使用一个真实 OutOfRaid Repository。
- accepted Split 经 P4→P3→P2→P1 后，由既有 `CommitAcceptedSnapshot` 单次保存。
- P1 Revision 只在 accepted Split 增加一次；保存失败恢复 P1 BeforeSnapshot。

### P6 内图

- P6 BaseQuick／合法 child 的 Split 仍由 active Run Repository 与 `CommitAcceptedActiveRunInventorySnapshot` 单次保存。
- 来源 ItemId 不变，新 ItemId 不自动绑定；既有 repository-level Hotbar reconcile 保留合法来源 binding 或清理失效引用。

### P6↔P9 与 P6↔P11

- P10/P12 页面继续使用 P6 + exact target 的一个 P1-valid composite Repository。
- P1 accepted Split 生成 composite Revision `old composite + 1`。
- durable callback 重新加载 exact Owner/Run/target/definition/source revisions；任何 stale、非 Open、action pending、capacity、identity、graph 或保存失败均拒绝。
- 新 adapter 只接受一个新增 identity，且逐字段证明：唯一来源 stack 只减少 N；新项除 ItemId、Quantity、ParentContainerId、SlotIndex 外与来源相同；唯一 target cell 原为空；所有其他 item、container、definition 与 stable slot 不变；无 ChildContainer。
- 验证后才 partition 为 P6 与 P9/P11 snapshot，并在一个 Owner document candidate 中同时更新 P6 session、target record、visibility/reveal、Hotbar、session/target/persistent revisions，再执行一次原子 `SaveRecord`。
- P9 新进入项和 P11 新进入项均为显式已知 Drop，状态为 `Revealed`；Hidden/Searching 项仍不得离开目标。

## 6. 资格、空间图、Hotbar 与稳定布局

- 允许：Definition 明确 stackable、Quantity > 1、请求数量合法、来源和目标均为当前可写普通 storage、目标明确为空。
- 拒绝：装备格／装备类、非 stackable、Quantity 1、带 ChildContainer 的空间 parent、同源格、占用格、Hidden/Searching、stale、invalid child、WorldDrop 与 UI 外 Drop。
- `WindTalisman`、`BackpackLevel1` 和所有带 ChildContainer 的 parent graph 不可 Split；P17/P18/P20/P21 完整图规则未改变。
- 来源仍在合法 BaseQuick 时保留同一 ItemId，原 Hotbar binding 保持；新 ItemId 不自动绑定。若来源不再合法，沿现有 reconcile 清理。
- 没有 Sort/Compact/Take All/自动 Merge/自动 Equip/自动绑定。成功后只改变来源 Quantity 与明确 target stable SlotIndex。
- 双栏 `PlayerScrollOffset`/`TargetScrollOffset` 仍由 P23 workspace context 保留；数量输入和 Draft 不重建容器或重排 slot。

## 7. 输入语义审计

- 普通 left-click：仍只选择；不会拆分。
- 普通 full-stack Drag：仍为既有 Move/Swap/Merge/Equip/Unequip，不受 SplitDraft 默认化。
- `Ctrl + 左键`：显式清理 Draft 并保持完整堆 QuickTransfer；不打开数量输入、不拆分。
- `Shift + 1—9`：仍只走 P13 bind/unbind；Split 不自动绑定。
- right-click：仍为只读详情菜单。
- double-click：没有新增 Split 或位置写入。
- Esc／关闭／blur：只清理输入、Draft、preview 或关闭页面，不提交 Split。
- 滚动：只更新两个 pane 的 transient offset。

## 8. 明确未实现范围

- 未把 Split 接入 P14 WorldDrop、世界 Actor 或地面 root；ground-drop callback 对 split payload 明确拒绝。
- 未实现 partial merge、自动 target、自动分配、自动 compact、Take All、自动装备、自动绑定、自动使用、空间 graph 拆分、多人／网络或 Code A 库存镜像。
- 未修改 P5→P6 bridge、P8 terminal receipt、P13/P15/P17/P19/P20/P21、StartAttempt、M01、战斗、死亡、撤离或搜索来源语义。

## 9. 编译结果

### Editor

命令：

```powershell
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 首次 native exit code：`1`。唯一错误为本任务新增 drag label 使用条件表达式作为 UE 5.8 checked format string；未涉及旧代码或范围外问题。
- 修复：改为两个编译期常量 `FString::Printf` 分支，没有扩大任务范围。
- 最终 native exit code：`0`。
- 关键结果：`Result: Succeeded`；生成 `UnrealEditor-demo_map.dll` 与 `demo_mapEditor.target`。

### Game

命令：

```powershell
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native exit code：`0`。
- 关键结果：`Result: Succeeded`；生成 `Binaries/Win64/demo_map.exe` 与 `demo_map.target`。

## 10. 未执行的 F 阶段验证

按 P24 停止条件，未启动产品、Editor、PIE、Standalone、真实输入、截图、Smoke、Automation、回归、试玩、Cook 或 Package。Split 的真实交互、重启持久化、跨目标场景矩阵和最终产品验收统一留给 `0.0.9B.F`。

## 11. 最终状态

`READY_FOR_P25_PLANNING`
