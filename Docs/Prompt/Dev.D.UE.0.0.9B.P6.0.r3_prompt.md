# Dev.D.UE.0.0.9B.P6.0.r3

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P6——Code B 局外 Profile 到活动 Run 库存会话的 bridge，Prepared receipt 恢复语义修订与产品证据返工
- 任务编号：`Dev.D.UE.0.0.9B.P6.0.r3`
- 任务性质：`P6.0.r2` 已如实证明现有契约无法通过第二次真实 CTA 恢复同一旧 RunId。本任务依据策划裁决，最小改造 Code B 的 pending-receipt 恢复语义，并补齐真实产品验证与外层退出证据；不得开始 P7、P8 或 `0.0.9B.F`。
- 执行文件：`Dev.D.UE.0.0.9B.P6.0.r3_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P6.0.r3_report.md`
- 活动工程根：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B`
- 活动工程：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject`
- 工程级开发基线：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9-XFix1`
- 已接收前置：`Dev.D.UE.0.0.9B.P4x.0.r2`；P5 为真实 Profile 实施交接，完整 P1–P6 验证仍留给 `0.0.9B.F`。
- 候选输入：P6 r0/r1 实现、P6 r2 真实产品 trace 与审计结论。

---

# 上半部分：只读裁决、现状与边界

## 1. P6 r2 的正式审阅结论

`P6.0.r2` 不能通过。其六个可独立完成的产品场景、CTA 入口审计和增量 Editor 编译可作为候选证据保留；它没有伪造失败，也没有越界启动 P7/P8/F。

阻塞点是现有契约的真实冲突：重启时 Code A 按自己的正式生命周期把旧 `ActiveRun` 结算为 `RecoveredAbandon`；下一次真实 CTA 因而产生新的 RunId；未关闭的 P6 session 对不同 RunId 返回 `ActiveSessionConflict`。因此，在禁止直接调用 observer 的条件下，不可能经第二次真实 CTA 回到原 RunId。`RequestExitWithStatus(1)` 与 `UnrealEditor-Cmd.exe` 外层退出码仍为 `0` 也是未完成的受控退出证据，不得被改写成通过。

## 2. 本次策划裁决：新 RunId 的安全重绑恢复

不恢复、重开或复活已被 Code A 正式判定为 `RecoveredAbandon` 的旧 RunId。Code A 继续拥有正式 Run 生命周期，P6 不得反向改变这个结果。

当且仅当同一 Owner 的一个 P6 receipt 仍为 verified `Prepared`、其当前绑定的旧 Run 已由 Code A 明确记录为 `RecoveredAbandon`，且第二次真实 CTA 已成功激活一个新的 Code A Run 后，P6 可以把**同一 receipt 的未完成 Code B 事务**安全重绑到这个新 RunId，再继续既有的单次提交。

这不是重新从 P5 选择物品，也不是重新生成 session：

1. receipt 的稳定 `ReceiptId`、原始随身布局、每个 ItemId、ContainerId、ChildContainerId、来源 P5 revision 和原始 RunId 都必须保留；
2. 新 RunId 作为该 receipt 的当前绑定 RunId，并以追加式审计历史记录 `OldRunId → NewRunId`、Code A 原因 `RecoveredAbandon`、时间和恢复次数；原始 RunId 不得被抹除或伪装成新 RunId；
3. recovery 只能使用 receipt 内已经验证的 carry payload 和同一事务身份。不得从当前 P5 snapshot 重新计算或再次抽取布局；
4. 最终只能出现一次完整提交：携带物品恰好一次位于新的 P6 Run snapshot，局外 P5 恰好一次不再拥有这些携带物品；未选仓库物品保持原 ItemId、父位置和数量；
5. 若 recovery 的任何前置不满足，P6 只记录可审计拒绝，不写 P5/P6，Code A 的新 Run 仍按成功路径继续。

## 3. 持续有效的权威边界

1. Code A 继续拥有默认地图、玩家 Actor、战斗、正式 Run 生命周期、旧库存、Loot、搜索、世界物品、结算、Run Save 和旧运行时。
2. Code B P1 Repository 是新物品系统唯一可变真值；P5 是局外 Code B snapshot，P6 是活动 Run Code B snapshot。不存在 Code A / Code B 镜像或双写。
3. P6 只能在 Code A 已成功完成正式 Run 激活后，从正常 observer 路径只读接收稳定 `OwnerId` 和 `RunInstanceId`。测试不得直接调用 observer；生产 observer 在该成功路径中调用 P6 的恢复逻辑是本任务允许的正常链路。
4. bridge 无权启动、阻止、重试、回滚、退出或改变 Code A Run。无 P5 sidecar、空布局、receipt 恢复拒绝、storage 错误和活动 session 冲突都不得成为 Start Run 门槛。
5. P4x 规则持续有效：没有备战门槛；普通物品移动、交换、合并、装备和卸下只由真实 Drop 提交；仅装备栏有类别／单槽互斥；空间戒指是装备后的快捷空间，空间储物囊是非快捷独立容器。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

在不改变 Code A `RecoveredAbandon` 生命周期结果的前提下，实施并验证一个窄范围的 `Prepared` receipt 新-RunId恢复分支。它只解决“第一次真实 CTA 在 verified Prepared 后中断，重启后第二次真实 CTA 已取得新 Code A RunId”的 P6 bridge 恢复。

允许：

- 修改隔离的 Code B P6 receipt、session、bridge、测试和 trace 文件；
- 在现有 Code A lifecycle adapter / coordinator 上增加最小、只读的恢复上下文传递，例如“此前 RunId 已被 Code A 结算为 `RecoveredAbandon`”。该上下文只能由 Code A 在其既有恢复决定后提供，P6 无权写入、改变或请求该决定；
- 增加一个项目内、仅用于定向验证的外层启动／退出包装器，以及结果文件或等价 machine-readable outcome；
- 更新 `PROJECT.md`、`PROJECT_INFO_CARD.md` 和本次 Report。

禁止：

- 让 P6 恢复、重新打开、复活或修改旧 Code A RunId，或改变 Code A 对 `RecoveredAbandon` 的判断、时机和正式结果；
- 改动 P5 正常页面、P5 迁移、P4x UMG/拖拽语义、Start Run 成功条件、地图加载、Player Actor、旧库存、Loot、搜索、世界物品、结算或 Run Save；
- 新建 P7 局内背包／I 键、玩家 Actor 应用、Loot、尸体／容器搜索、世界掉落、拾取、丢弃、1—9、消耗品、撤离、死亡、物品返还或关闭 Run session；
- 用直接调用 `NotifySuccessfulRun(...)`、`ObserveCodeBRunAfterActivation(...)`、`StartPreparedProfileRun(...)`、P1/P2/P3/P4 Controller、Widget 回调、P5/P6 Store 或 Repository 替代真实 Sect Teleport Start Run CTA；
- 运行 P1–P6 全量回归、Game 构建、宽范围截图巡检、最终 SHA-256 总审计，或把工作扩大为 `0.0.9B.F`。

## 5. 恢复状态机与不变量

实现名称可以按工程现有风格调整，但外部可审计语义必须如下。

### 5.1 receipt 与 session 的审计字段

每个 pending receipt 至少可追溯地呈现：

- `ReceiptId`；
- `OwnerId`；
- 不可变 `OriginRunId`；
- 当前绑定 `RunInstanceId`；
- `RecoveryRebindHistory`（每个旧／新 RunId、Code A terminal cause、序号与时间）；
- 来源 P5 revision、准备 payload digest、P6 provisional/session revision、receipt 状态和 bridge 状态；
- carry 集合中每个 ItemId 的位置与必要的 ChildContainerId。

已有持久化格式可以迁移，但不得丢失既有 receipt 或把旧 RunId 覆盖成新 RunId。若需要 schema 迁移，迁移必须是一次性、幂等、可审计的本地 P6 记录迁移，且不触及 Code A save / Run Save。

### 5.2 唯一允许的 Prepared 重绑条件

正常 lifecycle observer 收到一个新 `OwnerId + NewRunId` 后，只有以下条件同时成立才进入 recovery 分支：

1. 新 Run 的 Code A world activation 已成功，且 observer 从真实 CTA 链得到稳定身份；
2. 同一 Owner 仅有一个可恢复的 verified `Prepared` receipt；
3. receipt 尚未 `Committed`，其 payload digest、来源 revision 与内部 ItemId 图完整可验证；
4. Code A 只读恢复上下文明确表明 receipt 当前绑定的旧 RunId 已被 Code A 判为 `RecoveredAbandon`；
5. 新 RunId 与旧 RunId 不同，且新 RunId 尚无已提交或无关 P6 session；
6. 该 Owner 没有一个应继续保持的 `Committed`／正常活动 P6 session。

满足后，以一个可恢复、幂等的 P6 事务：追加审计 history，更新当前绑定 RunId，保留 `OriginRunId` 与原 payload，再执行既有的 `Prepared → Committed` 单次提交。重复 observer、进程中断或重复启动都必须落到同一 receipt，不得产生第二个携带集合、第二个活跃 ItemId 或 revision 回退。

如果首次重绑后再次在提交前中断，可以继续以同一 `ReceiptId` 追加下一条完整 history；每次只接受 Code A 已正式标记为 `RecoveredAbandon` 的当前绑定旧 Run。实现不得把不同 RunId 普遍视为可恢复。

### 5.3 仍须拒绝的不同 RunId

下列情形继续使用 `ActiveSessionConflict` 或等价拒绝，并保持 P5、旧 P6 snapshot、receipt 和 revision 不变：

- 已有 `Committed` 或其他正常活动 P6 session；
- Owner 不同；
- receipt 不为 verified `Prepared`；
- 缺少或不匹配 Code A `RecoveredAbandon` 上下文；
- payload digest、来源 revision、ItemId 图或事务记录不可验证；
- 新 RunId 已有无关 P6 session。

每种拒绝均不得影响相应 Code A Start Run 的成功返回。

## 6. 真实产品生命周期验证

只使用与正常产品相同的 Sect Teleport Start Run CTA（或可证明完全同一成功链的入口）。每个运行均应有独立隔离存储根；不得由 harness 直接调用 observer 或 P6 写接口。

每个场景在 Report 中必须嵌入可逐行定位的、按单调 sequence 排列的 trace 摘录，不得只列日志文件名。每条 trace 至少含：

1. `ProductStartRunCTA`；
2. `CodeAStartRunRequested`；
3. `CodeAWorldActivated`；
4. `CodeBObserverDelivered`；
5. 若适用的 `CodeARecoveredAbandonContext`、`P6PreparedRebind` 与其旧／新 RunId；
6. `CodeBBridgeFinal`（receipt、P5/P6 revision、bridge 状态）；
7. `CodeAStartRunReturned`；
8. `ProcessExit`（engine 退出码、wrapper 判定和最终 wrapper 退出码）。

### 6.1 七个当前 r3 场景

运行并报告以下七行当前 r3 evidence。它们是 P6 定向场景，不构成 F 的全量回归。

| 场景 | r3 必须证明的事实 |
| --- | --- |
| CompleteCarry | 正常 CTA 后才创建 P6 session；兵器、道袍、所有实际存在的饰品、基础 6 格每个选中物、已装备空间戒指及内部内容、已装备空间储物囊及内部内容均以同 ItemId、位置、ChildContainerId 进入 P6；未选仓库 ItemId 不变。 |
| Empty | 空 P5 layout 不会恢复备战或装备门槛；Code A Start Run 成功，真实空 P6 session 正确提交。 |
| NotEnrolled | Code A Start Run 成功；P6 为 `NotEnrolled`／等价状态，0 Profile、0 fixture、0 migration、0 Repository、0 session。 |
| BridgeFailure | 在 world activation 后、P6 commit 前触发可审计故障；Code A 仍成功返回；P5/P6 保持定义明确的可恢复 Prepared 状态或完整旧状态，无半扣除、重复 ItemId、revision 回退或暗中重试。 |
| PreparedRecovery | 第一次真实 CTA 留下 verified Prepared receipt 后结束进程。重启时 Code A 自己记录旧 Run `RecoveredAbandon`；第二次真实 CTA 生成不同的新 RunId，经 `CodeAWorldActivated` 和正常 observer 触发 P6 rebind。列出 ReceiptId、OriginRunId、旧／新 CurrentRunId、history、两次 P5/P6 revision、payload digest、全量携带 ItemId 图。证明只提交一次、无重复／丢失、未选仓库不变；对第二次新 RunId 的重复 observer 仍幂等。 |
| ActiveSessionConflict | 已提交的旧 P6 active session 面对另一个不同 RunId 时仍拒绝，不可误入 Prepared recovery；Code A Start Run 仍成功，P5／旧 P6／revision 不变。 |
| OutOfRaidLock | 活动 session 存在时，正常仓库／人物配置入口显示精确文本 `当前 Run 中，返回后再整理` 或已认可等价文本，且 Repository 创建数、UI Host 创建数、持久化写入数均为 0。 |

## 7. 外层退出包装器

`UnrealEditor-Cmd.exe` 的内部 `RequestExitWithStatus` 不能单独作为真实进程退出证据。新增或调整一个项目内、仅定向测试使用的启动包装器；它只负责启动既有产品 CTA harness、等待其退出、读取同次运行写出的 machine-readable result，并将最终状态返回给外层 shell。

包装器规则：

1. Editor-Cmd 非零退出时，包装器保留非零失败；
2. Editor-Cmd 为 0 且 result 明确为 `PASS` 时，包装器退出 0；
3. Editor-Cmd 为 0 但 result 缺失、格式错误、scenario 未完成、trace 断言失败或明确 `FAIL` 时，包装器退出非零；
4. 超时、残留 `UnrealEditor` / `UnrealEditor-Cmd` 进程、人工终止或无 result 都只能是非零失败；
5. 运行一个隔离的 wrapper negative-control：它只生成明确 `FAIL` result，不改任何产品／P5／P6 数据；Report 必须证明 wrapper 的真实外层 exit code 非零。它不计入七个产品场景。

常规七场景均由 wrapper 成功返回 0。Report 必须原样列出每条命令、隔离根、Engine code、result outcome、wrapper code 与无残留进程检查；不得再用日志中的 request code 替代 wrapper 实际退出码。

## 8. 来源、资料与 Report

Report 必须分开列出：

1. r0、r1、r2 保留内容与 r3 的精确新增／修改／删除文件；
2. 每个 Code A 文件的逐文件理由、调用方向、读取／写入字段，特别说明恢复上下文仅在 Code A 已作出 `RecoveredAbandon` 后只读传递，且没有改变 Code A Run、Player、Loot、搜索、结算、Run Save 或旧库存权威；
3. Code B receipt/session/bridge 的新 RunId rebind 事务、迁移（如有）、幂等与拒绝规则；
4. 全部七个 r3 场景的完整 trace 摘录、实际命令、矩阵、ItemId／ChildContainerId 映射和 wrapper 退出事实；
5. `PROJECT.md`、`PROJECT_INFO_CARD.md` 中 P6 r0/r1/r2/r3 的关系，以及 P5/P6/P7/P8/F 的所有权和剩余验证债务。

生成 `Dev.D.UE.0.0.9B.P6.0.r3_report.md`，存入：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

只有以下全部完成时，Report 才可使用：

    READY_FOR_CODE_B_IN_RAID_UI_WITH_F_DEBT

- 新-RunId Prepared rebind 符合第 5 节的全部前置、不变量和拒绝边界；
- 七个真实 CTA 场景均有当前 r3 证据并通过；
- PreparedRecovery 未直调 observer，且展示 Code A 旧 Run `RecoveredAbandon`、新 RunId 与单 receipt 单提交；
- 外层 wrapper 对七场景为 0、negative-control 为非零，且无残留进程；
- Code A / Code B 边界审计完整，没有进入 P7/P8/F。

否则只能使用：

    NEEDS_P6_REWORK
    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P7、P8、`0.0.9B.F` 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P6.0.r3","file":"Dev.D.UE.0.0.9B.P6.0.r3_report.md"}

若项目、任务编号、Prompt、Report、活动工程、P5/P6 状态、Code A/Code B 权威边界或目标 Chat 无法对应，停止受影响工作并生成 Error001 Report；不得自行猜测、改号、切换项目或直接开始后续阶段。
