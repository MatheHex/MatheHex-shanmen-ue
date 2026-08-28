# Dev.D.UE.0.0.10.P1.12.r0 开发日志

## 基线

- 日期：2026-08-27（America/New_York）
- 分支：`agent/0.0.10-p1-12-atomic-run-start`
- 基线提交：`f6b020931690fd3de3d039158aef97da2d8106e8`
- 上一阶段：P1.11 canonical reward metadata / authority schema 2
- 阶段目标：消除 prepared commit 与 active Run claim 之间的可持久化中间态，使正常 Run start 的最终权威转换只发生一次。

## 开工审查

现有 `Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun` 名称已经存在，但内部仍执行两次 durable command：

1. `CommitPreparedLoadout` → `CommitBatchDurable`；
2. `ClaimPreparedRunDurable`；
3. Runtime materialization。

Runtime 失败后可以依靠 claim 恢复，因此功能上可重试；但第一次与第二次写盘之间存在“资源已 committed、尚无 ActiveRunId”的持久中间态。若进程在此窗口退出，产品必须额外推断如何继续。P1.12 的核心不是增加另一个 wrapper，而是把这两个权威变化收敛为一个 repository command。

## 设计决策

### Pending intent 与 active 边界

equipment / RunInventory 的 reservation 继续是可观察、可取消的 pending intent。一次性迁移 equipment baseline 也先转换成正常 `DeploymentLock` reservation。它们可以各自持久化，但不会消费 quantity、部署 item 或发布 active Run。

最终命令 `StartPreparedRun` 才同时：

- commit 所有 pending reservations；
- 派生并发布唯一 `ActiveRunId`；
- 记录 processed-request ledger；
- 推进一个 authority revision；
- 由 authority service 写入一个 document generation。

### Runtime 不进入持久事务

Runtime materialization 保持在 durable command 之后。它不能与磁盘 authority 形成跨系统 ACID 事务，因此采用可重建投影：失败时 Runtime rollback，authority receipt 保留；下一次启动从 receipt 重放。这样不会通过“撤销磁盘事实”制造另一个失败窗口。

### 兼容旧文档

没有删除 legacy `CommitBatch` / `ClaimPreparedRun` operation。State validator、active Run 查询与 finalize 同时理解：

- 旧：CommitBatch receipt + Claim receipt；
- 新：一个 StartPreparedRun receipt。

新 operation 追加在 enum 尾部，避免移动已发布 operation 值。当前 lifecycle 不再调用旧双写入口。

## Core 实现

新增 `FShanmenItemRunStartRequest`，结构校验要求：

- operation context 完整；
- reservation list 非空；
- reservation ID 全部有效且唯一；
- 顺序作为 canonical command 输入保留。

`FShanmenItemRepository::StartPreparedRun`：

- 使用 `Shanmen.Items.Command.StartPreparedRun.r1` 指纹命名空间；
- exact replay 在 mutation 前返回；
- request ID 同输入冲突失败关闭；
- 拒绝任何未 finalized legacy Claim 或 atomic Start；
- 校验每条 reservation 为同 owner/scope 的 Reserved；
- 在候选状态中批量 `ApplyCommit`；
- 一次增加 authority revision；
- 生成 active purpose receipt，其中 `RequestId == ItemInstanceId`，后者作为 prepared identity；
- `ActiveRunId = MakeActiveRunId(owner, scope, prepared identity)`；
- 经 `ValidateState` 成功后才发布候选状态。

`ValidateState` 已扩展：

- Start 与 Claim 共同参与 active Run 唯一性；
- legacy Claim 必须引用匹配的 CommitBatch；
- atomic Start 必须自带稳定 prepared identity；
- Finalize 可引用两种 active receipt；
- Start 的每条 reservation 必须处于 Committed 或 Released 且只被一个 aggregate commit 覆盖。

## Durable service 与产品适配

Authority service、GameInstance authority subsystem 各增加 `StartPreparedRunDurable` wrapper，继续使用锁、ready state、Game Thread fence、原子 persistence 与 command-state synchronization。

Preparation adapter 增加：

- `FindActivePreparedLoadout`：从 atomic Start 或 legacy Claim 重建 active prepared loadout；
- `StartPreparedLoadout`：校验/补齐 pending intent，调用一次 atomic command，随后从 durable receipt 重建结果。

Run lifecycle adapter：

- 删除 claim request ID 与第二个 claim command；
- start result 改为持有 atomic `StartCommand`；
- Runtime plan 接受新 Start 或旧 Claim receipt；
- Runtime 失败后用相同 atomic receipt 恢复；
- terminal settlement 通过 `StartPreparedLoadout` 查找 active receipt，兼容新旧文档。

## 测试开发

### Repository.AtomicPreparedRunStart

新增覆盖：

- Quantity 与 DeploymentLock 在一个 revision 中 commit；
- 同一 receipt 发布有效 `ActiveRunId`；
- processed ledger 中有且仅有一个 Start，无 CommitBatch / Claim；
- exact replay 完全相等；
- 相同 request ID、不同 reservation 顺序冲突且不修改 revision；
- snapshot restart 后精确重放；
- Extraction finalize 可释放并恢复资源。

### AuthorityService.AtomicPreparedRunStartDurability

新增真实临时 authority document 测试：

- pending reservations 先正常持久化；
- `WriteTemp` 故障使 Start 完整回滚 snapshot、document 与 bytes；
- 同一请求重试仅增加一个 authority revision 与一个 save generation；
- fresh service restart 后返回相同 receipt 的 `Replayed`，generation 不变。

### RunLifecycle.ClaimRestartFinalize 更新

既有 Runtime mutation failure 场景改为断言：

- failure 后 Runtime inactive 且 item projection 为空；
- durable document 只增加一个 generation；
- ledger 只有一个 Start receipt，无 intermediate CommitBatch / Claim；
- authority restart 后使用同一 active Run ID 恢复；
- 后续 Extraction 与重复 finalize 语义保持不变。

测试总数由 62 增至 64。

## 验证记录

### Editor 构建

统一参数：

`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- 主实现：26/26 actions，成功，112.41 秒，原生退出码 0；
- durability test 补充后：4/4 actions，成功，5.77 秒，原生退出码 0。

### 定向自动化

筛选：

`Shanmen.0_0_10.Items.AtomicPreparedRunStart + Shanmen.0_0_10.Items.AuthorityService.AtomicPreparedRunStartDurability + Shanmen.0_0_10.Items.PreparedRunLifecycleLedger + Shanmen.0_0_10.Items.RunLifecycle`

- 5/5 Success，0 Fail，queue empty，原生退出码 0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.12.r0_targeted_final.log`；
- SHA-256：`ED3C9B0E97E9BEDAA7E8AC6C23849661C61CD40287F1146848FD0895D0E01894`。

### 完整自动化

筛选：`Shanmen.0_0_10`

- 64/64 Success，0 Fail，queue empty，原生退出码 0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.12.r0_automation_final.log`；
- SHA-256：`8968F0CFF85A8393BE0E68D542687F59481D0178DC2F63DDC3527F3E1A201750`。

### Game 构建

- 23/23 actions，成功，87.70 秒，原生退出码 0。

### 静态审查

- `git diff --check`：原生退出码 0；
- ShanmenItems core 的 World / Actor / GameplayStatics / damage / random API：0；
- Run lifecycle 的旧 `CommitPreparedLoadout` / `ClaimPreparedRunDurable` / claim request：0；
- preparation / lifecycle 的旧 Profile / Code B writer / `BeginRun`：0；
- core 对 `demo_map` 的唯一命中是既有注释；
- 工作区无关未跟踪文件保持原状，提交使用显式路径暂存。

## P/F 边界

本阶段仅执行源码开发、静态审查、headless automation 与必要 Editor/Game 编译。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 留待 P1.13

- 将正常 Profile preparation / UI 开始按钮切到 atomic Start 入口；
- 为“authority 已 active、Runtime 尚未物化”的 UI 恢复状态提供明确表现；
- cutover 完成后，把旧 CommitBatch / Claim 产品入口降为仅历史读取兼容。
