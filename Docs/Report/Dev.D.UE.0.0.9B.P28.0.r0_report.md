# Dev.D.UE.0.0.9B.P28.0.r0 Report

## 1. 结果

- 任务：`Dev.D.UE.0.0.9B.P28.0.r0`
- Prompt：`Docs/Prompt/Dev.D.UE.0.0.9B.P28.0.r0_prompt.md`
- Prompt SHA256：`77DCC32573644A156032FE1025D414F2D9B0E69776B365A224CF454D474C1F5E`
- 实现提交：`f0ec6a6 feat: add exact partial world stack merge`
- 结果：P28 的 `WorldPickupDraft(N) → occupied compatible player stack → Merge(N)` 已静态闭合；P27 空格 `Split(N)`、P26 normal `Merge(0)` 与既有持久化边界保持。

## 2. 改动前静态审计

### P1／P2／P25

- P1 `ExecuteMerge` 已具有两种正式语义：`Quantity > 0` 精确接受 N；`Quantity == 0` 按当前 source 与 available 计算普通接受量。
- P1 已校验 source／target address、occupied target、同 `DefinitionId`、正式 `bStackable`、`MaxStack`、available 与 revision；失败不提交 Candidate。
- P25 的 `IsExactP25MergeDelta` 已证明普通 Merge 只允许一个 source 减量／删除和一个 target 增量，禁止新 ItemId、定义变化、额外物品或容器变化。
- P28 不修改 P1、P2 或 P25 原语义；只在 durable callback 增加更严格的命令绑定证明。

### P14／P26／P27

- P14/P26/P27 共用 `CommitAcceptedMatchedRunWorldDropPickup`，以一个 Owner document Candidate 和一次 `SaveRecord` 提交。
- P26 normal occupied drop 已使用 `Merge(0)`；partial acceptance 时保留 world root、record、world container、`WorldDropId` 与 ordinal。
- P27 的 `WorldPickupDraft`、数量确认、Owner/Run/revision/record identity、取消与 stale 清理均为 transient；空 target 使用一次 P1 `Split(N)`。
- P27 UI 原先明确拒绝 occupied target；这正是 P28 的唯一缺口。

### shared workspace／P13／P8／P19

- P4 已能为 quantity draft 对 compatible occupied target 产生精确 `Merge(N)` preview，并在 `Available < N` 时拒绝而不截断。
- BaseQuick 与当前装备空间 parent 的 child container 继续由正式 P6 projection／stable Cell 提供；装备、Hotbar、parent、外部目标、P9/P11 尸体／容器仍由既有角色与页面 gate 排除。
- P13 binding reconcile、P8 player-only terminal closure、P19 complete spatial root 路径均位于 durable Store 边界，P28 未扩大其产品语义。

## 3. 实际修改文件

1. `Source/demo_map/CodeB/demo_mapCodeBP3.h`
   - `FProfileCommit` 增加只读 `FCodeBP2Command` 参数，使 durable callback 获得已由 P1/P2 接受的精确瞬时命令身份。
   - 命令不持久化，不持有可写 Item 或 Quantity 副本。
2. `Source/demo_map/CodeB/demo_mapCodeBP3.cpp`
   - 在 P1/P2 成功后把同一个 accepted command 传给一次 profile commit。
   - commit 失败仍加载完整 `BeforeSnapshot` 回滚并刷新 projection。
3. `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
   - 保留 WorldPickup 到空格的 P27 `Split(N)` 分支。
   - 仅对明确 occupied、compatible、未满且 `Available >= N` 的玩家普通储物 Cell 接受 P28 `Merge(N)` preview。
   - 数量反馈改为“明确空格或兼容未满玩家堆叠”；无自动目标、无静默截断。
4. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`
   - pickup writer 接收 accepted P2 command 作为 transient proof context。
5. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
   - 新增 `IsExactP28WorldPickupMergeDelta`。
   - 证明 exact command 为正数量 `Merge(N)`、source 是当前 world root、target 是明确 occupied player storage address、`1 <= N < source quantity`、`Available >= N`。
   - 复用 P25 结构证明并进一步要求 item cardinality 不变、source ItemId/placement 保留且恰减 N、target ItemId/placement 保留且恰增 N。
   - accepted P28 Candidate 直接进入同一个 P14/P6 durable Candidate；不 replay 第二 Merge，不使用 `Merge(0)` fallback，不进行第二次保存。
   - world root、world container、record、`WorldDropId` 与 ordinal 保留，record/Actor 后续只从 accepted Store projection 重读。
6. `Source/demo_map/demo_mapV3ProgressionManager.cpp`
   - 五个正式 Profile 页面 callback 适配只读 accepted-command 参数。
   - 只有 world-drop callback 将其交给 P14/P6 Store 做 P28 结构化重验证；Code A 仍只管理页面、Store 调用和 accepted 后 Actor refresh。

## 4. 明确未修改文件／职责

- `demo_mapCodeBInventory.*`：P1 `Merge`、`Split`、`Move` 未修改。
- `demo_mapCodeBP2.*`：P2 Apply 与 projection 未修改。
- `demo_mapCodeBP4.*`：既有 exact quantity preview/commit 已满足 P28，无需修改。
- P5→P6 bridge、StartAttempt、M01、P8 receipt、P13 binding 产品规则、P15 Use、P17 graph、P19 closure、P20/P21 搜索与 Loot Profile未修改。
- world Actor、尸体／容器、装备、Hotbar、地图、敌人、战斗、撤离、商店与经济未修改。

## 5. P28 完整 call graph

1. 用户在 P14 simple stack world root 上开启既有数量入口并确认 `N`。
2. P27 `WorldPickupDraft` 保存 transient source address、Owner、Run、`WorldDropId`、expected revision、graph identity 与 N；零写入。
3. `BeginInventoryDrag` 再验证 source／record／Run／revision，生成一个 `WorldPickup` payload。
4. 明确 occupied target Cell 调用 shared P4 preview。
5. P4 以正式 projection 验证 same Definition、双方 `bStackable`、same `MaxStack`、无 child、target 非 equipment 且 `Available >= N`；生成 `Merge(N)`，不足容量明确拒绝。
6. world workspace gate 再要求目标属于 BaseQuick 或当前合法空间 child，并区分：空 target 仍为 P27 `Split(N)`；occupied target 才为 P28 `Merge(N)`。
7. `CommitP4Operation` 创建一个 P2 command；P2 只调用一次 P1 `Merge(N)`。
8. P1 accepted 后，profile commit 将 CandidateSnapshot 与同一 accepted command 交给 world-drop Store。
9. Store 重验 Owner、Run、world record/root、source/target stable address、revision、正式 stack metadata、N 与 available。
10. `IsExactP28WorldPickupMergeDelta` 证明 source −N、target +N、零 new/delete ItemId、零 placement／container／definition 旁改。
11. 同一 Owner document Candidate 更新 P6 snapshot，执行既有 P13 reconcile、receipt freeze 与 session validation，然后只调用一次 `SaveRecord`。
12. Code A 重新打开 Store projection并刷新同一 world Actor；无第二 record、Actor、world container 或 ordinal。

## 6. 原子性与身份结论

- source ItemId、world container、SlotIndex 0、record、`WorldDropId`、ordinal 保留。
- target ItemId、ContainerId、SlotIndex 保留。
- source Quantity 恰减 N；target Quantity 恰增 N。
- Item 数量不变；无 CreatedItemId、无 delete、无 clone、无预建 ItemId。
- accepted command 的 source/target/quantity/revision 与 Candidate delta 必须同时匹配，否则 Store 拒绝并由 P3 恢复完整 `BeforeSnapshot`。
- P28 不先扣 world Quantity、不先更新 Actor、不进行 Split 后 Merge、`Merge(0)` fallback、two-save 或 UI direct mutation。
- target 原有合法 Hotbar 引用因 ItemId 保持而保持；world source 不绑定，且不复制任何引用。
- P8 终局仍只处理 player graph；地面 remainder 继续留在 P14 record，不回流 P5。

## 7. 分支与输入语义保持

- normal Drag：仍走 P26 whole-root Move 或 `Merge(0)`；P28 不劫持。
- WorldPickupDraft → empty：仍走 P27 `Split(N)` 并使用唯一 P1 CreatedItemId。
- WorldPickupDraft → occupied compatible：仅此路径走 P28 `Merge(N)`。
- `Available < N`：明确拒绝，N 不截断，不退回 normal Merge。
- `Ctrl + 左键`：world source 继续零写入拒绝。
- `Shift + 1—9`：仍只执行 P13 Bind，world item/draft 不绑定。
- right-click 仍只读详情；double-click、Actor 点击、Tab/I/Esc、关闭、取消、失焦、滚动和无 payload Drop 不写 P28。
- success refresh 只影响 world source/Actor 与明确 target；无 Sort、Compact、重编号、空间区重建或 scroll reset。

## 8. 编译

### Editor

命令：

    "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

- native exit code：`0`
- 结果：`Result: Succeeded`
- 输出：`UnrealEditor-demo_map.dll`
- 修复重试：无

### Game

命令：

    "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

- native exit code：`0`
- 结果：`Result: Succeeded`
- 输出：`Binaries\Win64\demo_map.exe`
- 修复重试：无

## 9. 明确未执行／未实现

- 未启动产品、PIE、Standalone、真实输入、截图、Smoke、Automation、回归、试玩、Cook 或 Package。
- 未执行 F 阶段真实验证；统一留到 `0.0.9B.F`。
- 未实现 `N == source quantity`、容量不足自动截断、stack exchange、Take All、自动目标、自动拾取、Actor direct pickup、快捷拾取、从地面拆到多个格或从玩家按数量丢地。
- 未扩大到 P9/P11、尸体、搜索、装备、Hotbar、空间 parent、未打开 child、P5 warehouse 或外部容器。
- 未开始 P29、Fix 或 F。

READY_FOR_P29_PLANNING
