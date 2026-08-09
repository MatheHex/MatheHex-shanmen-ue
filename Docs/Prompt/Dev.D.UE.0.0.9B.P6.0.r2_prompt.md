# Dev.D.UE.0.0.9B.P6.0.r2

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P6——Code B 局外 Profile 到活动 Run 库存会话的 bridge，证据与边界审计返工
- 任务编号：`Dev.D.UE.0.0.9B.P6.0.r2`
- 任务性质：保留 `P6.0.r0` 的候选 bridge 与 `P6.0.r1` 已完成的实现；本任务只补齐 P6 r1 报告未逐项呈现的真实产品生命周期证据、受控退出证据和 Code A 来源审计。不得开始 P7、P8 或 `0.0.9B.F`。
- 执行文件：`Dev.D.UE.0.0.9B.P6.0.r2_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P6.0.r2_report.md`
- 活动工程根：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B`
- 活动工程：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject`
- 工程级开发基线：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9-XFix1`
- 已接收前置：`Dev.D.UE.0.0.9B.P4x.0.r2`；P5 是真实 Profile 实施交接，完整 P1–P6 回归仍留给 `0.0.9B.F`；P6 r0/r1 是待审计候选实现。

---

# 上半部分：只读裁决、现状与边界

## 1. 本次返工的唯一原因

`P6.0.r1` 报告主张七个真实 Sect Teleport Start Run 场景、Code A 激活后 observer、无残留进程和 Editor 编译均通过。这些实现与日志可继续作为候选成果。

但 r1 Report 没有逐项呈现下列 Prompt 明确要求的可审计证据：

1. 七个场景各自的真实 CTA → Code A 请求 → 世界激活成功 → observer 收到稳定身份 → bridge 终态 → Code A Start Run 返回的完整顺序；
2. 每个场景的实际 `OwnerId`、`RunInstanceId`、P5/P6 revision 前后值、receipt 状态、最终 bridge 状态和进程退出状态；
3. CompleteCarry 中基础 6 格、装备栏、空间戒指及其 `ChildContainerId` 内容、空间储物囊及其 `ChildContainerId` 内容、未选仓库物品不变的逐项事实；
4. Prepared recovery 的第二次交付确实重新经由产品 Start Run 成功链，而不是直调 observer；
5. 本轮精确改动文件，以及所有 Code A 改动逐文件不改变 Run/Player/Loot/搜索/结算/Run Save/旧库存权威的理由。

本任务是“补证与审计”，不是对已实现 bridge 的重做、扩展或功能返工。除让既有测试／trace 可审计所需的最小 `WITH_DEV_AUTOMATION_TESTS` 诊断外，默认不改生产逻辑。

## 2. 持续有效的权威边界

1. Code A 继续拥有默认地图、玩家 Actor、战斗、正式 Run 生命周期、旧库存、Loot、搜索、世界物品、结算、Run Save 和旧运行时。
2. Code B P1 Repository 是新物品系统唯一可变真值；P5 是局外 Code B snapshot，P6 是活动 Run Code B snapshot。不存在 Code A / Code B 镜像或双写。
3. P6 只能在 Code A 已成功完成正式 Run 激活后，以只读形式接收稳定 `OwnerId` 与 `RunInstanceId`。若工程字段实际名为 `ProfileId` / `ActiveRunId`，必须在报告中逐次列出并明确一一对应。
4. bridge 无权启动、阻止、重试、回滚、退出或改变 Code A Run。无 P5 sidecar、空布局、receipt 恢复失败、storage 错误和活动 session 冲突不得成为 Start Run 门槛。
5. P4x 规则持续有效：没有备战门槛；普通物品移动、交换、合并、装备和卸下只由真实 Drop 提交；仅装备栏有类别／单槽互斥；空间戒指是装备后的快捷空间，空间储物囊是非快捷独立容器。

---

# 下半部分：授权执行内容

## 3. 单一授权目标

使用 r1 已有的真实 Sect Teleport Start Run CTA 产品生命周期入口（或完全同等、可证明同一生产成功链的入口）重新运行并审计七个 P6 场景。生成一份可由策划单独审读的证据 Report；不要求、也不允许以全量回归或新功能来补偿报告缺口。

允许：

- 为现有产品生命周期 harness 增加只读 trace 字段、断言、日志格式或 `WITH_DEV_AUTOMATION_TESTS` 下的诊断读口；
- 为准确记录最终受控退出状态增加最小测试启动／退出包装；
- 更新 `PROJECT.md`、`PROJECT_INFO_CARD.md` 和本次 Report。

禁止：

- 直接调用 `NotifySuccessfulRun(...)`、`ObserveCodeBRunAfterActivation(...)`、`StartPreparedProfileRun(...)`、P1/P2/P3/P4 Controller、Widget 回调、P5/P6 store 或 Repository 来代替产品 Start Run 入口；
- 改动 P6 抽取语义、P5 迁移、正常局外页面功能、Start Run/地图加载/Player Actor/旧库存/Loot/搜索/世界物品/结算/Run Save；
- 新建 P7 局内背包／I 键、玩家 Actor 应用、Loot、尸体／容器搜索、世界掉落、拾取、丢弃、1—9、消耗品、撤离、死亡、物品返还或关闭 Run session；
- 运行 P1–P6 全量回归、Game 构建、宽范围 UI 截图巡检、最终 SHA-256 总审计，或把工作扩大为 `0.0.9B.F`。

## 4. 强制产品生命周期证据

### 4.1 统一 trace 契约

每个场景必须生成一个独立 trace（可以写入对应日志并在 Report 中逐行摘录），且至少以同一 `Scenario`、`OwnerId`、`RunInstanceId` 和 monotonic sequence 标识如下事件：

1. `ProductStartRunCTA`：真正 Sect Teleport Start Run CTA 已被触发；
2. `CodeAStartRunRequested`：正式 Code A Start Run 生命周期已进入；
3. `CodeAWorldActivated`：`ActivatePreparedProfileWorld()` 或实际等价世界激活已成功；
4. `CodeBObserverDelivered`：成功后首次交给 P6 observer 的稳定身份；
5. `CodeBBridgeFinal`：bridge 最终状态、receipt 状态、P5/P6 revision 前后值；
6. `CodeAStartRunReturned`：Code A Start Run 的最终成功返回，不读取 bridge 结果作为门槛；
7. `ProcessExit`：测试进程最终请求的退出状态与实际退出 code。

禁止用“源码审阅确认在激活后”代替上述逐次运行顺序。Report 中每个场景必须嵌入这些字段的紧凑原文摘录，或给出无歧义、可逐行定位的日志区间和完整命令；不得仅列日志文件名。

### 4.2 七个场景矩阵

用同一个产品生命周期套件覆盖以下所有场景；每行都必须列：实际入口、OwnerId / RunInstanceId、P5 revision 前后、P6 revision 前后、receipt 前后、bridge 终态、Code A Start Run 返回、进程最终退出 code、trace 位置和断言结果。

| 场景 | 额外强制事实 |
| --- | --- |
| CompleteCarry | 从真实 CTA 成功后才创建 P6 session。逐项列出：武器、道袍、各饰品、基础 6 格每个被选 Item、已装备空间戒指及其内部内容、已装备空间储物囊及其内部内容；每项须给 ItemId、源位置、Run 位置、`ChildContainerId`（如适用）。证明每个 ItemId 未复制／重编号，未选仓库 ItemId 与位置不变。 |
| Empty | Code A Start Run 成功；创建持久真实空 P6 session；没有恢复备战或装备门槛。 |
| NotEnrolled | Code A Start Run 成功；bridge 为 `NotEnrolled` 或等价状态；未创建 Profile、fixture、迁移、Repository 或 session。 |
| BridgeFailure | 世界激活成功后、P6 提交前触发可审计故障；Code A 仍成功返回，且无重试、回滚、退出、玩家／旧库存改变。明确列出 P5 source、P6 session、receipt 的最终状态，证明不存在半扣除或不明状态。 |
| PreparedRecovery | 第一次真实 CTA 成功链留下 verified `Prepared` receipt；移除中断钩子后，第二次也必须重新从真实 CTA 经 `CodeAWorldActivated` 和 `CodeBObserverDelivered` 到达恢复。不得直调 observer。列出两次 RunId、receipt 与 revision 演变，并证明无重复 ItemId／半扣除／revision 回退。 |
| ActiveSessionConflict | 新 RunId 遭到 `ActiveSessionConflict` 或等价状态；旧 session 与局外／旧 Run snapshot 不变；对应 Code A Start Run 仍成功且不读取该拒绝作为门槛。 |
| OutOfRaidLock | 活动 session 存在时，从真实“仓库／人物配置”入口获得精确文本 `当前 Run 中，返回后再整理` 或已认可等价文本；明确列出 Repository 创建数、UI Host 创建数与持久化写入数均为 0。 |

### 4.3 受控退出与最小构建

- 只运行 P6 r2 产品生命周期定向测试与必要的单次 `demo_mapEditor Win64 Development` 编译。
- 每个测试命令必须原样写入 Report，包含实际入口、隔离存储根、测试选择、`-TestExit` 或等价受控退出参数。
- 对每个执行，报告必须给出 Automation 结果、`RequestExitWithStatus` 记录、外层进程最终 exit code，以及无残留 `UnrealEditor` / `UnrealEditor-Cmd` 进程的证据。不得把超时、队列完成但进程遗留或人工终止写成成功。
- 可以复用 r1 已有日志，但如果日志缺少本任务 4.1/4.2 的字段，则必须在不改变产品行为的前提下重跑相应定向场景；不得伪造或补写旧日志。

## 5. 来源与边界审计

Report 必须分开列出：

1. r0 保留的实现文件及其职责；
2. r1 相对 r0 的精确新增／修改／删除文件；
3. r2 相对 r1 的精确新增／修改／删除文件；
4. 所有 Code A 文件的逐文件理由、调用方向、读取／写入字段与“为何不改变 Code A Run、Player、Loot、搜索、结算、Run Save、旧库存权威”的结论；若 r2 无 Code A 改动，必须明确写 `无`，并用审计命令／哈希或等价 diff 证明；
5. 所有 Code B 生产逻辑与测试／trace-only 文件的边界。`WITH_DEV_AUTOMATION_TESTS` 诊断不得成为产品可调用写入口。

在 `PROJECT.md`、`PROJECT_INFO_CARD.md` 或等价资料中明确：P6 r0/r1/r2 的关系、P5/P6/P7/P8 的所有权边界，及 P1–P6 全量验证、跨分辨率 UI、Game 构建与最终来源审计仍留待 `0.0.9B.F`。

## 6. Report 与退出条件

生成 `Dev.D.UE.0.0.9B.P6.0.r2_report.md`，存入：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须包含：

1. 任务身份、r1 原 Prompt SHA-256、r0/r1/r2 的明确关系；
2. 第 5 节完整文件和权威审计；
3. 七个产品生命周期 trace 摘录和第 4.2 节完整矩阵；
4. CompleteCarry 的逐 ItemId／ChildContainerId 映射；
5. BridgeFailure 与 PreparedRecovery 的非阻塞、无半状态证据；
6. OutOfRaidLock 的文本及 0 Repository／0 Host／0 持久化写入证据；
7. 编译、每个场景命令、Automation 结果、最终 exit code 与无残留进程证据；
8. 仍明确留给 `0.0.9B.F` 的验证债务。

仅当 P6 r1 的全部七场景、真实产品顺序、受控退出和逐文件边界审计均能被本 Report 审计时，最终状态才可使用：

    READY_FOR_CODE_B_IN_RAID_UI_WITH_F_DEBT

否则只能使用：

    NEEDS_P6_REWORK
    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P7、P8、`0.0.9B.F` 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P6.0.r2","file":"Dev.D.UE.0.0.9B.P6.0.r2_report.md"}

若项目、任务编号、Prompt、Report、活动工程、P5/P6 状态、Code A/Code B 权威边界或目标 Chat 无法对应，停止受影响工作并生成 Error001 Report；不得自行猜测、改号、切换项目或直接开始后续阶段。
