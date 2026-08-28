# Dev.D.UE.0.0.10.P1.12.r0 开发报告

## 结论

**PASS。READY_FOR_0_0_10_P1_13。** P1.12 已把准备态资源提交与 `ActiveRunId` 发布合并为 ShanmenItems 单一权威中的一个原子命令。正常 lifecycle 不再先写 `CommitBatch`、再写 `ClaimPreparedRun`；最终转换只推进一次 authority revision、只提交一个 durable document generation，并由同一 `StartPreparedRun` receipt 同时承担 prepared batch identity 与 active Run marker。

最终源代码状态通过：

- P1.12 原子启动定向回归：5/5 Success；
- `Shanmen.0_0_10` 完整回归：64/64 Success；
- Editor Development 构建：成功，原生退出码 0；
- Game Development 构建：成功，原生退出码 0；
- `git diff --check`：原生退出码 0。

## 功能性

### 1. Core 原子 Run-start 契约

ShanmenItems 新增：

- `EShanmenItemTransactionOperation::StartPreparedRun`；
- `FShanmenItemRunStartRequest`；
- `FShanmenItemRepository::StartPreparedRun`；
- `FShanmenItemAuthorityService::StartPreparedRunDurable`。

请求携带一个有序、非空、无重复的 reservation ID 集合。Repository 在候选状态中完成全部校验和 mutation：

1. 验证 content stamp、owner、scope、reservation identity 与 pending state；
2. 拒绝任何尚未 finalized 的 legacy Claim 或 atomic Start；
3. 在候选图中提交每个 Quantity / DeploymentLock；
4. 从 owner、scope 与稳定 request ID 派生唯一 `ActiveRunId`；
5. 仅推进一次 authority revision；
6. 写入一个 `StartPreparedRun` processed-request receipt。

任何一条 reservation 失败时，候选状态不会发布；相同 request ID 与相同有序输入精确重放，相同 request ID 携带不同顺序或内容返回 request conflict，且不修改权威状态。

### 2. 单次 durable document 提交

Authority service 通过既有 `ExecuteCommandLocked` 执行新命令。准备态的 pending reservations 仍是可恢复意图；真正从 prepared 到 active 的边界现在只有一次持久化提交。

故障注入测试在 `WriteTemp` 阶段强制失败，并验证：

- authority snapshot 与调用前逐字段相等；
- authority document generation 不变；
- primary document 字节不变；
- 没有部分 committed reservation；
- 没有可见 `ActiveRunId`；
- 清除故障后，同一请求成功重试并只增加一个 revision / generation。

成功后重启 authority，原命令返回完全相同 receipt 的 `Replayed`，且不再写盘。

### 3. 产品 preparation / lifecycle 入口

`Fdemo_mapShanmenPreparationAdapter::StartPreparedLoadout` 是 P1.12 的受控产品入口：

- 若已有未终结 atomic Start 或 legacy Claim，直接从 durable ledger 重建 prepared loadout；
- 若是一次性迁移产生的 equipment baseline，先转换为普通 pending `DeploymentLock` 意图；
- 校验全部 equipment 与完整堆叠 RunInventory reservation；
- 使用原有稳定 prepared request ID 调用一次 `StartPreparedRunDurable`；
- 从 atomic receipt 重建 Runtime materialization plan。

`Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun` 已切换到该入口，删除正常路径中的第二次 `ClaimPreparedRunDurable`。终局结算也使用同一恢复入口定位 active Run，避免 atomic Start 文档被误当作缺失的旧 `CommitBatch`。

Runtime 仍是瞬态投影，不参与持久事务。若 materialization 中途失败，Runtime 完整回滚为空；durable atomic Start 保留为唯一事实。重启或重试会重建同一个 `ActiveRunId` 和 loadout，不产生第二次 authority 写入。

### 4. 向后兼容

旧 schema-2 文档中的 `CommitBatch + ClaimPreparedRun` 链路继续可读取、重建和 finalized；新文档使用单个 `StartPreparedRun` receipt。Repository state validation、active Run 唯一性检查与 `FinalizePreparedRun` 同时识别两种形态。

旧 `CommitPreparedLoadout`、`ClaimPreparedRun` API 暂时保留用于历史文档与既有测试兼容，但当前 Run lifecycle 源文件对这些旧双写调用的静态命中为 0。新增 operation 位于 enum 尾部，既有 operation 数值不移动；本阶段没有建立第二库存、第二 active Run 标记或旁路 JSON。

## 完整性与兼容性

- commit 与 active marker 在一个 repository candidate、一个 authority revision、一个 durable generation 中共同发布。
- persistence 失败完整回滚内存和磁盘；post-restart replay 不写盘。
- Runtime 失败不撤销 durable truth，而是回滚瞬态投影并允许确定性恢复。
- legacy `CommitBatch + ClaimPreparedRun` 文档仍可终结；新旧格式均受单 active Run 不变量约束。
- preparation reservation 的有序列表进入 command fingerprint；顺序漂移不能伪装成 exact replay。
- `StartPreparedRun` receipt 的 request ID 同时是稳定 prepared-batch identity，`ActiveRunId` 可被 state validator 重新派生并核对。
- ShanmenItems core 不依赖 World、Actor、GameplayStatics、伤害执行或随机 API。
- preparation / lifecycle adapter 不调用旧 Profile、Code B writer 或旧 `BeginRun`。
- 无关未跟踪 Prompt、旧 Report、PDF 与自动化文档未纳入本阶段提交。

## 修改范围

- ShanmenItems 类型、repository、authority service 与 durability tests；
- demo_map authority subsystem、preparation adapter、Run lifecycle adapter 与 integration tests；
- 本 Report 与同名开发 Log。

未修改 Profile schema、Code A / Code B 持久结构、Runtime item authority、UI、地图、输入、战斗或奖励规则。

## 验证

### P1.12 定向回归

命令筛选：

`Shanmen.0_0_10.Items.AtomicPreparedRunStart + Shanmen.0_0_10.Items.AuthorityService.AtomicPreparedRunStartDurability + Shanmen.0_0_10.Items.PreparedRunLifecycleLedger + Shanmen.0_0_10.Items.RunLifecycle`

- 结果：5/5 Success，0 Fail，队列正常清空；
- 覆盖：单 revision、无中间 batch/claim、request conflict、pre-commit disk rollback、单 generation、restart replay、Runtime rollback/recovery、Extraction / Death / Abandon 终结与 legacy ledger 兼容；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.12.r0_targeted_final.log`；
- SHA-256：`ED3C9B0E97E9BEDAA7E8AC6C23849661C61CD40287F1146848FD0895D0E01894`。

### 完整 0.0.10 回归

- 结果：64/64 Success，0 Fail，队列正常清空；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.12.r0_automation_final.log`；
- SHA-256：`8968F0CFF85A8393BE0E68D542687F59481D0178DC2F63DDC3527F3E1A201750`。

### 构建

统一使用：

`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- 主实现 Editor：26/26 actions，成功，112.41 秒，原生退出码 0；
- durability test 补充后 Editor：4/4 actions，成功，5.77 秒，原生退出码 0；
- 最终 Game：23/23 actions，成功，87.70 秒，原生退出码 0。

### 静态边界

- `git diff --check`：原生退出码 0；
- ShanmenItems 中 `UWorld`、`AActor`、`UGameplayStatics`、`ApplyDamage` 与随机执行 API：0；
- Run lifecycle 中 `CommitPreparedLoadout`、`ClaimPreparedRunDurable` 与 legacy claim request：0；
- preparation / lifecycle 中旧 Profile、Code B writer 与旧 `BeginRun`：0；
- core 对 `demo_map` 的唯一文本命中仍是既有边界注释，不是代码依赖。

## P/F 边界声明

本报告属于 P 阶段功能实现、静态审查、headless Unreal automation 与必要 Editor/Game 构建。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 后续建议

P1.13 建议把正常 Profile preparation / UI 的产品启动按钮切到 P1.12 唯一入口，并为已经 active 的 authority receipt 建立 UI 级恢复提示；完成该 cutover 后，再将旧 `CommitPreparedLoadout + ClaimPreparedRun` 降为纯历史读取兼容面。
