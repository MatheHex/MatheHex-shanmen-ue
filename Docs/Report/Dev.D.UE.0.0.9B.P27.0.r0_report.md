# Dev.D.UE.0.0.9B.P27.0.r0 Report

## 1. 任务与结果

- 任务：`Dev.D.UE.0.0.9B.P27.0.r0`
- Prompt：`Docs/Prompt/Dev.D.UE.0.0.9B.P27.0.r0_prompt.md`
- Prompt SHA-256：`0186E5AA0327349223DA070BA0C24C4D5C8F8F3339A50F7AAE70E6F61D74CD67`
- 实现提交：`f732cca`（`feat: add explicit partial world pickup`）
- 结果：P14 simple stack world root 已获得显式 `WorldPickupDraft(N)` → 明确空 P6 玩家格 → 单次 P1 `Split(N)` 的闭环；P26 whole-root `Move`／normal `Merge`、P24 玩家来源 `PlayerSplit`、P13、P8、P19 与 Code A 边界保持。
- 完成信号：`READY_FOR_P28_PLANNING`

## 2. 实际修改文件及职责

1. `Source/demo_map/CodeB/demo_mapCodeBP3.h`
   - 新增 transient `ECodeBP3QuantityDraftKind::{None, PlayerSplit, WorldPickup}`。
   - `FCodeBP3SplitDraft` 增加明确 kind 与 P27 `WorldDropId`；`WorldPickup` 草稿必须具备有效 record identity。
2. `Source/demo_map/CodeB/demo_mapCodeBP4.h`
   - drag payload 增加 quantity draft kind 与 transient `WorldDropId`。
   - payload 有效性要求 `WorldPickup` 同时具备 graph identity、record identity 与 `1 <= N < SourceQuantity`。
3. `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`
   - `BeginSplitDrag` 接受明确 draft kind；仍只在拖拽开始时消费已确认草稿，不创建 ItemId、不改数量。
4. `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`
   - `FCodeBP3WorldDropPresentation` 增加只读 `WorldDropId`，用于 UI scope/stale 校验，不持有可写物品状态。
5. `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
   - 复用 P24 数量控件，地面来源生成独立 `WorldPickup` 草稿。
   - 草稿、payload、preview、显式空 target、Owner/Run/revision、P14 record/root identity 在确认、拖拽与 Drop 前逐层重校验。
   - 地面数量拾回只允许 `Basic6`、当前已打开合法 `QuickSpatial`／`PouchInternal` 的明确空普通储物格；占用格、地面目标、装备、外部目标及非法 child 均拒绝。
   - 保留取消、Esc、失焦、关闭、stale、另一项 drag 的零写入清理。
6. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`
   - 更新唯一 pickup writer 的 P27 契约说明。
7. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
   - `CommitAcceptedMatchedRunWorldDropPickup` 新增 P27 精确 Split 候选分支。
   - 复用 `IsExactP24SplitDelta` 证明候选仅含：一个既有 world source 减 N、一个由已接受 P1 创建的新 ItemId 落入原本为空的明确玩家格、revision +1，且定义／其他 item／container／slot 全部不变。
   - 保留 world root ItemId、world container、record、`WorldDropId` 与 ordinal；只以 accepted source projection 更新余量，并沿原有一次 `SaveRecord(Candidate)` 提交。
8. `Source/demo_map/demo_mapV3ProgressionManager.cpp`
   - Code A 只提供当前 world page 的 Owner/Run/WorldDrop 身份、活动 gate 与只读 revision resolver。
   - durable callback、Store commit 与 Actor refresh 仍是既有边缘适配；Code A 未写 ItemId、Quantity、container、record、ordinal 或 pickup outcome。

未修改：P1 `ExecuteSplit`、P2 ApplicationService、P5→P6 bridge、P8 terminal closure、P13 binding service、P15 Use、P17/P19 图规则、P9/P11 容器、Actor 类、地图／战斗／经济系统及任何测试文件。没有新增 Repository、world inventory、fixture、Widget inventory、存档 schema 或旁路写入。

## 3. 改动前静态路径审计

- P1/P2：`FCodeBP3UIController::CommitP4Operation` 构造一个 `ECodeBOperation::Split` 命令；P2 将其原样转为一个 P1 request。`ExecuteSplit` 在 candidate 内验证 source address、stack definition、`1 <= N < source` 与空 storage target，接受后才生成 `CreatedItemId`，source 原 ItemId 留存，revision 只推进一次。
- P14/P26：world page repository 是当前完整 P6 snapshot 的本地权威候选；durable callback 进入 `CommitAcceptedMatchedRunWorldDropPickup`。P26 whole-root Move 清除 record/Actor，partial Merge 保留 record/root；两者最终均走一个 Owner document candidate 与一次 save。
- P24/P25：数量输入、draft、payload 与 preview 都是 transient。P24 `PlayerSplit` 允许普通玩家来源到空格或兼容堆；P25 exact Merge 不创建 identity。P27 使用新增 kind 隔离 world source，不改变两者资格。
- P4/P23：共享 Cell 提供稳定 `ContainerId + SlotIndex + ItemId`；Drop 才提交。`Basic6` 与当前投影中实际出现的 `QuickSpatial`／`PouchInternal` 是可见目标，动态容量和 scroll context 由既有 workspace 保持。
- P13/P8/P19：Store 保存前仍执行 hotbar reconcile、payload receipt freeze 与完整 session validation；P8 仍处理 player-only closure。带 `ChildContainerId` 的 P19 root 不满足 P27 simple stack 资格。
- 旧旁路审计：未发现或新增按钮直接领取、Actor direct pickup、world auto target、Widget quantity mutation、预建 ItemId、先 Split 后第二 save、直接改 ordinal、`Ctrl+左键` ground pickup 或 P9/P11 target 路径。

## 4. P27 完整 call graph 与单一事务

1. 用户选中当前 world root；普通 left-click 只更新 selection。
2. 用户打开数量输入并确认 N；`CreateSplitDraft` 重新读取当前 projection/source，验证 simple stack、无 child、数量边界、Owner/Run、world container、`WorldDropId`、graph identity 与 revision。
3. Host 只保存 `WorldPickup` transient draft；此时没有 ItemId 创建、source 预扣、target 预占、record/Actor/ordinal 写入。
4. 用户从同一 source 开始 drag；Host 消费草稿并再次验证 live workspace identity/revision/source，生成带 kind 的 one-shot payload。
5. 明确 target Cell 的 preview 要求：玩家普通 storage、当前 projection 可见、正式 capacity 内、空格、非 equipment／Hotbar／P5／P9／P11／world／非法 child。
6. Drop 前 `ValidateTransferContext` 再校验 Owner、Run、source scope/address、P14 `WorldDropId`、world container、graph identity、resolved P6 revision 与 payload expected revision。
7. `CommitP4Operation` 只提交一个 P2 command；P2 只调用一个 P1 `Split(N)`。
8. P1 accepted candidate 中：world source 原 ItemId 数量变为 `SourceQuantity - N`；明确 target 获得唯一 `CreatedItemId`，数量为 N；revision +1。
9. P14 Store 从磁盘重开精确 Owner/Run session，重新验证 record/root/revision、simple stack、空 target、合法 P6/P17 player storage，并用 `IsExactP24SplitDelta` 证明无第三项变化。
10. Store 保留同一 world record、world container、root ItemId、`WorldDropId` 与 ordinal，执行既有 P13 reconcile、P8 receipt/session validation，然后一次 `SaveRecord(Candidate)`。
11. durable callback 成功后本地 controller 接受 projection；Code A 从 Store accepted projection 刷新同一 Actor 的 Quantity。保存失败或任何校验失败时 controller 载回 BeforeSnapshot，Actor/record/target/binding 均保持旧值。

不存在 clone、UI 预建 ItemId、source 预扣、target 预占、second world record、second Actor、new ordinal、reverse Split、second save 或 UI direct mutation。

## 5. Draft 生命周期与 P24 隔离

- `PlayerSplit` 与 `WorldPickup` 为明确不同 kind；P27 payload 必须同时匹配当前 `WorldDropId` 和 world container。
- 数量在确认、drag 消费和最终 Drop/Store 各阶段重新满足 `1 <= N < SourceQuantity`；`N == SourceQuantity` 被拒绝，不降级为 P26 Move。
- Esc、失焦、关闭页面、I 页面关闭、取消、另一项 drag、write gate 变更、Owner/Run/revision/source/record stale 或 Actor EndPlay 均清除 draft/preview，零写入。
- P24 玩家来源继续使用 `PlayerSplit`；其空格 Split 与兼容占用堆 exact Merge 规则未改变。P27 world source 明确拒绝占用 target，因此没有扩大 P24 或实现 ground exact Merge。

## 6. 资格、原子性与边界结论

- Source：必须是当前活动 Owner+Run、当前 P14 record 对应的 slot 0 world root；正式 stackable、MaxStack > 1、Quantity > 1、无 `ChildContainerId`。
- Target：必须是原 snapshot 中为空、当前 projection 明显可见的 `Basic6` 或已装备空间 parent 所拥有的 `QuickSpatial`／`PouchInternal` 普通格。Store 用 P6 layout/equipped child 关系再次验证。
- 排除：P19 完整空间 root、P9、P11、尸体、装备、Hotbar、P5 warehouse、world target、占用 stack、未打开/非法 child、UI 外区域。
- Accepted identity：唯一新 ItemId 是本地 P1 accepted `CreatedItemId`；Store 不重放随机 GUID，也不接受任意 GUID，而是验证 candidate 的完整 exact-P1 Split 结构。world source ItemId 保留。
- World identity：world container、record、`WorldDropId`、ordinal 与 Actor identity 保留；record/Actor Quantity 只来自 accepted source projection。
- 失败：P1、record identity、target、Owner/Run/revision、P13 reconcile、session validation 或 durable save 任一失败，Store 不替换记录；controller 恢复 BeforeSnapshot。
- 稳定布局：未调用 Sort、Compact、AddChild、数组重编号、空间重建或自动整理；未修改其他 SlotIndex、容量、parent placement、child contents 或 scroll policy。

## 7. 输入语义保持

- left-click：只选择。
- normal Drag：继续是 P26 whole-root Move 与 normal Merge 的普通入口；只有带 `WorldPickup` kind 的 confirmed quantity payload 才走 P27。
- `Ctrl + 左键`：world source 仍在 UI 层明确拒绝，不做 quick pickup 或 auto target。
- `Shift + 1—9`：继续只路由 P13 Bind；world source 不满足 BaseQuick binding 条件。
- right-click：只读详情。
- double-click：只选择，不 QuickMove。
- Actor 点击：只打开当前 world page，不领取物品。
- 详情、滚动、无 payload Drop、取消和页面生命周期：均不写 P27。

## 8. 明确未实现范围

未实现 ground exact Merge 到占用 target、Take All、自动目标、Actor direct pickup、自动拾取、右键／双击领取、world source exchange、multiple-root world container、玩家按数量丢地、多个目标拆分、自动装备／绑定／使用／整理、多格旋转、重量、筛选、搜索、网络或多人。未改变 P5、P8、P9、P11、P13、P15、P17、P19、P20、P21 产品语义。

## 9. 静态审查与编译

静态审查：`git diff --check` 通过；实际 diff 仅含上述 8 个文件。审查确认 WorldPickup draft/payload/preview/Actor 均为 transient/read-only 边缘，唯一物品写入为 P1 accepted Split，唯一 durable write 为 P14 Store 的一次 Owner save。

Editor 命令：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 首次 native exit code：`1`。原因仅为本任务新增 UI 文案使用三元格式串不符合 UE 5.8 checked `FString::Printf`，以及 revision resolver lambda 的 `INDEX_NONE` 推导类型冲突。
- 仅修正上述格式/返回签名后重跑 native exit code：`0`。
- 关键结果：`Result: Succeeded`；输出 `UnrealEditor-demo_map.dll`。

Game 命令：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native exit code：`0`。
- 关键结果：`Result: Succeeded`；输出 `Binaries\Win64\demo_map.exe`。

## 10. 未执行的 F 阶段验证

按 P 阶段边界，未启动产品、Editor、PIE 或 Standalone；未执行真实鼠标键盘输入、截图巡检、Smoke、Automation、单元测试、回归、试玩、Cook、Package 或最终验收。上述真实验证全部留到 `0.0.9B.F`。

READY_FOR_P28_PLANNING
