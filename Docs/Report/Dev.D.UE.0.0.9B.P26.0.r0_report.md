# Dev.D.UE.0.0.9B.P26.0.r0 Report

## 1. 任务结论

- 任务：`Dev.D.UE.0.0.9B.P26.0.r0`
- Prompt 归档提交：`b0f0c0624bd2b3d6374356a383121ede5995357a`
- 实现提交：`54db268150de0fd3b445ccc7f819f8e742e25688`
- 结果：P26 的 simple stack 四条权威路径已静态闭合：整堆落地 `Move`、部分落地 `Split`、空格拾回 `Move`、兼容堆叠拾回 `Merge(Quantity=0)`。
- 权威边界：P1 Repository 与 P6/P14 Owner document 仍是唯一可写真值；Widget、payload、preview、world Actor 与 Code A floor adapter 只携带瞬时意图或只读投影。
- P19 空间 parent 完整图路径保持原状；P26 没有扩大其闭包资格、来源、目标、装备语义或 P8 处理。

## 2. 实际修改文件与职责

1. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`
   - 为只读 `FCodeBWorldDropProjection` 增加正式 `DefinitionId` 与 `Quantity` 投影。
   - 将 P14/P19 ground-drop writer 扩展为接收稳定 source slot、expected P6 revision 与已确认 split quantity。
   - pickup writer 的职责注释扩展为 P14/P19/P26 精确 P1 candidate 边界。
2. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
   - 增加 P26 玩家合法储物目标判定：`BaseQuick` 或当前装备空间 parent 所拥有的 child container。
   - 增加 simple stack 正式资格判定：正式 definition、`bStackable`、`MaxStack > 1`、正数量、无 `ChildContainerId`。
   - 扩展 P14 drop store：normal drag 走原 `Move/Unequip`；confirmed quantity 走一次 P1 `Split`，从 accepted world slot/`CreatedItemId` 建 record。
   - 扩展 pickup durable callback：重建并比对精确 P1 `Move/Equip/Merge(0)`；partial Merge 保留 world root/record，full acceptance 才清理。
   - record、ordinal、P6 snapshot、P13 reconcile、receipt freeze、session revision 与 Owner save 保持一个 candidate、一次保存。
3. `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
   - GroundDropZone 接受合法 P24 split payload，并在提交前复用 Owner/Run/scope/revision/graph identity 校验。
   - world simple stack 只允许拖到明确的 `Basic6 / QuickSpatial / PouchInternal` 空格或兼容未满堆叠。
   - P19 spatial root 继续只允许合法空 BaseQuick／匹配空装备位；旧 P14 非堆叠 simple root 继续只允许空 BaseQuick。
   - `Ctrl + 左键` 对 world source 显式零写入拒绝，避免形成快捷拾取或自动目标。
   - partial drop 保留来源稳定选择；whole drop 才清除已离开玩家图的瞬时选择。
4. `Source/demo_map/demo_mapV3ProgressionManager.cpp`
   - Code A 继续先解析 floor placement，再把 exact Owner/Run、stable source address、expected revision、drop mode 与 placement 交给 Store。
   - 增加 payload Owner/Run/quantity 与当前 P6 source 的静态一致性门禁。
   - 未改变 floor trace 的地图落点职责，未赋予 Code A ItemId/Quantity/Container/record/ordinal 写权。
5. `Source/demo_map/demo_mapCodeBWorldDropActor.cpp`
   - Actor label 从 Store-owned projection 显示 `DefinitionId x Quantity`。
   - Actor 仍只缓存 Owner/Run/WorldDrop identity；没有库存、可写数量、ItemId 或 pickup 结果副本。

明确未修改：

- P1 `demo_mapCodeBInventory.*` 的 `Move / Split / Merge` 实现与 candidate invariant；P26 完整复用现有事务。
- P2/P4 的正式 `bStackable / MaxStack` projection、P24 SplitDraft 与 P25 full-stack partial acceptance 语义。
- P13 binding 数据结构与 reconcile 规则；P8 terminal player-only closure；P19 closure validator/discard 规则。
- P9/P11、搜索、尸体装备、战斗、终局、经济、地图与敌人系统。

## 3. P14/P19/P24/P25 → P26 call graph

### 3.1 整堆落地：P1 Move

`P3 normal drag payload`
→ `UCodeBP3GroundDropZone::NativeOnDrop`
→ `UCodeBP3UIHostSubsystem::RequestGroundDrop`（workspace identity/revision 校验）
→ `Ademo_mapV3ProgressionManager::ResolveCodeBWorldDropPlacement`（只读 floor input）
→ `FCodeBOutOfRaidProfileStore::DropMatchedActiveRunWorldDropItem`
→ exact source address/revision + P14/P19/P26 eligibility
→ candidate Repository 创建 deterministic single-slot world container
→ P1 `Move`（P19 equipment source 保留既有 `Unequip`）
→ accepted ItemId 原样成为 world root
→ P14 record + ordinal + P13 reconcile + P6 candidate
→ 一次 Owner save
→ 重读 projection / Actor refresh。

### 3.2 部分落地：P1 Split

`P24 confirmed SplitDraft(N)`
→ `BeginSplitDrag`（payload 只保存 N、stable address、Owner/Run/graph/revision）
→ GroundDropZone/Host 同一门禁
→ Store 验证 `1 <= N < SourceQuantity`、simple stack、BaseQuick/当前装备 child
→ candidate Repository 创建 single-slot world container
→ 一次 P1 `Split(N)`
→ 从 accepted transaction 的 `CreatedItemId` 与 world slot 读取新 root
→ 来源 ItemId 保留且 Quantity 恰减 N
→ 新 world ItemId/Quantity N 建立唯一 P14 record
→ ordinal、binding reconcile、P6 revision 与一次 Owner save 同候选提交。

Draft、payload、preview、Actor 均不预建 ItemId、不预扣数量、不预占持久 ordinal、不预写 record。

### 3.3 空格拾回：P1 Move

`P14 world Actor interaction` 只打开共享页面，不领取
→ world root normal drag
→ P4/P3 明确目标 Cell preview/commit
→ P2 Apply → P1 `Move`
→ P3 durable callback 将 candidate 交给 `CommitAcceptedMatchedRunWorldDropPickup`
→ Store 从磁盘重读 exact Owner/Run/drop/revision
→ 重建同一个 P1 Move 并逐字段比对 candidate
→ 移除空 world container + P14 record
→ P13 reconcile + 一次 Owner save
→ Actor projection 销毁/页面生命周期清理。

simple stack 可进入空 `BaseQuick` 或当前装备 parent 的合法 child；旧 P14 非堆叠 simple root 仍只进入空 BaseQuick；P19 空间 parent 仍走原 BaseQuick/匹配装备空位路径。

### 3.4 兼容堆叠拾回：P1 Merge(Quantity=0)

`world root normal drag → occupied explicit Cell`
→ P4 正式 Definition/Stackable/MaxStack preview
→ P2 Apply → P1 `Merge`，Quantity 为 0
→ P1 独立裁决 accepted amount
→ Store 先以 `IsExactP25MergeDelta` 证明仅有一个 source 减少/删除、一个 target 增加且无新 ItemId
→ 再重建 `Merge(0)` 并与 candidate 全图相等比较
→ partial：同一 world root ItemId、world container、WorldDropId、record 与 Actor 保留，只从 accepted projection 更新 Quantity
→ full：source 归零后才移除 world container、record 与 Actor
→ 一次 Owner save。

## 4. 资格、排除与稳定地址证据

- simple source：正 Quantity、正式 `bStackable`、`MaxStack > 1`、无 `ChildContainerId`、普通 storage slot。
- 玩家 source/target：仅 `Layout.BasicContainerId` 或当前装备空间 parent 的 exact `ChildContainerId`；equipment container、未装备/失效 child、任意外部容器均不通过 Store 判定。
- P19 complex graph：继续由 `ValidateP19WorldDropClosure` 仅识别 `WindTalisman` 与 `BackpackLevel1` 正式完整闭包；P26 simple branch 要求 `!bIsSpatialClosure` 与无 child。
- P9/P11：不属于 P6 player storage helper，且 world page 的外部/尸体目标仍受既有 P3 protected/external gates 约束。
- 装备/Hotbar：simple stack target 不接受装备 role；world source 的 Ctrl 快捷移动被显式拒绝；Shift+1—9 仍只进入 P13 Bind 并由既有 eligibility 拒绝 ground item。
- stale/非法条件：Owner、Run、source ItemId、stable ContainerId/SlotIndex、payload Quantity、P6 revision、world record/root identity、floor transform、target placement 与 P1 definition 均在写入前重新校验。
- SlotIndex/容量/布局：事务只改 source/target/world root 的明确 slot；没有 Sort、Compact、occupied-only AddChild、数组重编号、空间区重建或滚动归零。

## 5. 原子性、ItemId、record 与 ordinal

- Drop：Store 先在内存 candidate Repository 创建 deterministic world container 并执行 P1；只有 accepted result 满足唯一 world slot/root identity 后，才把 snapshot、record 与 `NextWorldDropOrdinal + 1` 放入 Owner candidate；`SaveRecord` 成功后才替换 Store record。
- Partial drop：唯一新 ItemId 来自 P1 `ExecuteSplit` 的 `CreatedItemId`；来源 ItemId/位置保留，world root 数量来自 accepted snapshot。没有 UI/Code A/record 层生成 ItemId。
- Pickup：P3 保存 `BeforeSnapshot`；durable callback 拒绝或保存失败时，P3 恢复 BeforeSnapshot。Store 自身也只在一次 `SaveRecord` 成功后替换 record。
- Partial Merge：不创建 ItemId、不创建第二 record、不递增 ordinal、不生成第二 Actor；accepted amount 为 target quantity delta，world remainder 来自 P1 candidate。
- Full Move/Merge：world root 不再存在时，才在同一 durable candidate 中删除 world container 与 record。

## 6. P13、P8 与 Code A 边界

- P13：whole player-source drop 删除来源时，既有 `ReconcileHotbarBindings` 清理引用；partial drop 保留来源 ItemId，因此原 binding 保持；world root 从不自动绑定，从 world Move 回玩家也不自动绑定。
- P8：`BuildP14PlayerOnlySession` 与 `DiscardP19WorldDropClosure` 未修改；任何剩余 simple/partial world root 均随全部 P14 world records 从 terminal player-only closure 排除，不回流 P5。
- Code A：只提供 map route/floor transform、刷新/销毁 Actor projection 与页面入口。P6 Store 独立重验 Owner/Run/revision/graph；Actor 失败不产生第二库存，也不改变已保存事务。

## 7. 输入语义保持

- 普通左键：仍只选择。
- 普通 Drag：是唯一 P26 drop/pickup 写入入口。
- `Ctrl + 左键`：仍是既有 QuickTransfer；world source 显式拒绝，不形成快捷拾取。
- `Shift + 1—9`：仍只进行 P13 Bind。
- right-click：仍只读详情。
- double-click、详情按钮、Tab、I、Esc、关闭、取消、失焦、滚动、无 payload Drop：不写 P26。
- world source 不进入 SplitDraft；P26 未实现从地面精确拿 N、Take All、自动分配或 Actor direct pickup。

## 8. 编译结果

### Editor

命令：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- 目标：`demo_mapEditor Win64 Development`
- native exit code：`0`
- 关键结果：`Result: Succeeded`
- 总执行时间：约 `20.38s`

### Game

命令：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- 目标：`demo_map Win64 Development`
- native exit code：`0`
- 关键结果：`Result: Succeeded`
- 总执行时间：约 `25.05s`

两个目标均仅执行一次。

## 9. 明确未执行的 F 阶段验证

本 P 阶段未启动产品、Editor UI、PIE、Standalone；未执行真实鼠标键盘输入、截图巡检、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。四种运行时行为、partial acceptance 的真实显示、Actor 交互与持久化恢复测试统一留待 `0.0.9B.F`。

## 10. 完成信号

READY_FOR_P27_PLANNING
