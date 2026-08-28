# Dev.D.UE.0.0.10.P1.13.r0 开发日志

## 基线

- 日期：2026-08-28（America/New_York）
- 分支：`agent/0.0.10-p1-13-product-start-cutover`
- 基线提交：`b266ff3eefd5e2cb2d661614210917b2ee695729`
- 上一阶段：P1.12 atomic prepared Run-start
- 阶段目标：让正常 Profile preparation / UI 产品入口使用唯一原子 authority lifecycle，并为 durable active / Runtime absent 提供明确恢复，同时避免把技术性世界失败写成玩家 Abandon。

## 开工审查

P1.12 已提供正确的原子 core 与 lifecycle adapter，但正常产品入口仍有三处未完成：

1. `Fdemo_mapProfilePreparationFlow::StartPreparedRunDirect` 仍调用旧 `ProfileSession.StartRunWithoutPreparation`；
2. authority 已 active、Runtime 未物化时，UI 没有可恢复身份或 Start 能力；
3. 世界激活失败沿用旧 Profile technical settlement 语义，coordinator 会把 RunId 标为 released，无法表达“durable Run 仍在、只清了 Runtime”。

产品 host 同时仍以 Code B loadout selection 作为 Start 前置读取。它可以保留审计价值，但不能继续承担资源真值或 active identity。

## 设计决策

### 动态、精确的 authority 选择

Flow 在初始化时保留弱 authority 引用，但每次使用前必须满足：

- lifecycle state 为 Ready；
- bound OwnerId 等于当前 ProfileId；
- bound storage root 与当前 root 精确匹配。

只有同时满足时才切换到 Shanmen lifecycle。这样产品初始化后完成的 cutover 会立即生效，旧 fixture 未建立 authority 时则继续使用原 Profile 流程。

### durable truth 与 presentation 分离

旧 Profile 不再写 active Run。`GetPresentationSnapshot` 只在 Runtime 已物化时，把 Flow 的 transient identity 投影为 UI 可读的 RunActive snapshot。原始 Profile snapshot 与 primary document 保持不变。

恢复状态不伪装成 RunActive：Runtime absent 时 Profile presentation 保持 ReadyForPreparation，而 preparation projection 通过只读 authority probe 显示 recoverable RunId 并允许 Start。

### technical rollback 不是 terminal

世界激活失败只执行 Runtime `ActivationFailure` summary 与 `PrepareForPersistentRun` 清理，不调用 Shanmen finalize。新增 `RuntimeRollbackReady` 状态明确表示：返回宗门已就绪，但 durable active receipt 仍等待同身份恢复。该状态故意不被 `IsDurablySettled()` 接受。

### terminal 只走 authority

Runtime 产生的合法 Extraction / Death / Abandon evidence 交给 Shanmen lifecycle finalize。成功后清 Runtime；retryable authority failure 保留 summary；旧 Profile settlement 只服务 compatibility 路径。

## 实现记录

### Product host

- 在 host 初始化时执行 existing-first authority cutover；首次启动在旧稳定源打开后补做一次 cutover。
- 宗门入口显示 ShanmenItems 产品就绪或精确 recoverable RunId。
- Start 前的 Code B selection 降为 read-only audit correlation；active recovery 时即使旧仓库不能重新打开，也可复用此前读取的审计选择。
- terminal callback 文案改为 Shanmen authority 已持久化。

### Profile flow / UI

- `StartPreparedRunThroughWidget` 在 authority-backed 模式直接进入统一 direct Start。
- `StartPreparedRunDirect` 调用 Shanmen lifecycle；成功时生成 presentation RunActive，不写 Profile。
- Runtime materialization failure 后探测 durable active receipt，保留 same RunId 并回到可重试 Preparation。
- 新增 `GetPresentationSnapshot`、`UsesShanmenItemLifecycle` 与 `GetRecoverableShanmenRunId`。
- authority-backed preparation projection 在存在 selection 或 recoverable Run 时启用 Start；空选择保持禁用。
- terminal settlement 与 retry 转给 Shanmen finalize。

### Recovery / coordinator

- lifecycle adapter 新增只读 `TryFindRecoverableActiveRun`，兼容 atomic Start 和 legacy Claim，并排除已 finalized receipt。
- world activation rollback 返回 `RuntimeRollbackReady`，只清 Runtime。
- V3 progression 接受该状态返回 AtSect，但不把它当 durable terminal。
- coordinator 在有 recoverable identity 时保留 RunId，不写 `ReleasedRunId`，并给出“恢复同一远征”的玩家反馈。

## 自动化开发

新增：

- `Shanmen.0_0_10.Items.ProductFlow.AtomicStartAndTerminal`
- `Shanmen.0_0_10.Items.ProductFlow.RuntimeFailureRecovery`

关键断言：

- 产品开始只产生一个 `StartPreparedRun`，`CommitBatch == 0`、`ClaimPreparedRun == 0`；
- authority generation 只增加一次；
- raw Profile 不进入 RunActive，primary Profile bytes 全周期不变；
- Runtime failure 后暴露可恢复 RunId；
- retry 使用相同 RunId 且 authority document 不变；
- technical rollback 不 durable、不 finalize、不 Abandon；
- recovery 后仍可正常 Extraction finalize；
- 清空全部 selection 后 Start 禁用并显示选择提示。

测试总数由 64 增至 66。

## 中间修正记录

### 首次 Editor 编译

首次 Editor 构建原生退出码 1。产品实现已完成编译，错误位于新测试辅助代码：当前 Unreal 容器不提供所写的 `CountByPredicate` 用法，且测试误引用未声明的 `ReadBytes`。随后改为显式遍历计数并新增 `ReadFlowBytes`。这是测试源码错误，不是 Windows commit memory、页面文件或 UE 环境错误。

### 首次完整回归

首次 `Shanmen.0_0_10` 运行得到 65/66。唯一失败为既有 `PreparationAdapter.CapabilityAndProjection` 仍断言 `bCanStartRun == false`；P1.13 正是将有有效 selection 的产品 Start 改为原子入口，因此更新为启用断言，并额外在清空 selection 场景断言 Start 仍禁用。修正后最终 66/66。

### 技术回滚诊断审查

在第一次全量通过后继续检查 coordinator，发现其旧行为会把 technical rollback 的 RunId 写入 `ReleasedRunId`，与 durable receipt 实际仍 active 冲突。最终修正为先读取 recoverable RunId：存在时保留原身份并显示恢复提示；不存在时才走旧释放诊断。该修正后重新完成 Editor、全量自动化与 Game 构建。

## 最终验证记录

### 定向自动化

- 筛选：`Shanmen.0_0_10.Items.ProductFlow`
- 2/2 Success，0 Fail，queue empty，原生退出码 0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.13.r0_targeted_final.log`；
- SHA-256：`EDE016A15AD32439A94F447172B02E0F12FF3C2592DACCFDF20A61FDF3698932`。

### 完整自动化

- 初次：65/66；日志 `Saved/Logs/Dev.D.UE.0.0.10.P1.13.r0_automation_initial.log`；SHA-256 `72ADBBA17F0952A22DC201CABDED6673FFB22EC1A8457EC4417239767C5B8983`。
- 最终：66/66 Success，0 Fail，queue empty，原生退出码 0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.13.r0_automation_final.log`；
- SHA-256：`BCD5067846C35913F81C3A90E1FB4D07A941F4F87A75605F5912462EB5396230`。

### 构建

统一参数：`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- 首次 Editor：测试辅助源码编译失败，原生退出码 1；修正后继续。
- 最终 Editor：23/23 actions，成功，104.53 秒，原生退出码 0；日志 SHA-256 `11BCD9FB95F56A708A430160E8D5EB638945893259508BBF34BFB49B3C09D4A7`。
- 最终 Game：22/22 actions，成功，99.18 秒，原生退出码 0；日志 SHA-256 `8890C89B7E74C40690D169753701D9A24C5F73FBA02362D8B593511D9974E46C`。

### 静态审查

- `git diff --check`：原生退出码 0；
- Shanmen core 的 World / Actor / GameplayStatics / damage / random / demo_map 命中：0；
- product lifecycle 的旧 Commit + Claim 两阶段调用：0；
- Run lifecycle 中旧 Profile / Code B writer 与 `StartRunWithoutPreparation`：0；
- 工作区无关未跟踪文件保持原状，提交使用显式路径暂存。

## P/F 边界

本阶段只执行源码开发、静态审查、headless Unreal automation 与必要 Editor/Game 编译。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 留待 P1.14

- 将 world-confirmed audit selection 改为 Shanmen authority 原生、不可变 correlation；
- 让 active Run 恢复不再需要重新打开旧 Code B warehouse 获取审计摘要；
- 继续保留旧文档只读兼容，不重新引入第二权威。
