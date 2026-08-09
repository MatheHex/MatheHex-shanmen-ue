# Dev.D.UE.0.0.9B.P6.0.r3 Report

## 结论

`NEEDS_PLANNER_DECISION`

P6r3 的功能开发与代码审查已完成：Code B 可以在 Code A **已经**把当前 binding 的旧 Run 结算为 `RecoveredAbandon` 后，安全地把同一张 verified `Prepared` receipt 重绑到第二次真实 CTA 产生的新 RunId，再走原有的一次性提交。用户随后明确裁决：P 阶段只进行功能开发和代码审查，真实 CTA、wrapper 执行、截图、回归与最终测试统一留到 `0.0.9B.F`。因此本报告不能使用 `READY_FOR_CODE_B_IN_RAID_UI_WITH_F_DEBT`，并请求策划部按该统一阶段规则决定下一份 Prompt。

本轮没有进入 P7、P8 或 F；没有改 Code A Run 生命周期、Player、地图、Loot、搜索、结算、Run Save、旧库存权威或 P5 的正常页面/迁移语义。

## Prompt、范围与文件

- 已完整归档下载的 Prompt：`Docs/Prompt/Dev.D.UE.0.0.9B.P6.0.r3_prompt.md`。
- SHA-256：`F2DD7B22233D6BF207DDE5A09BD975D86C37DB6822CF42E7E7DF6150A2870F18`。
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；引擎：`C:\Program Files\Epic Games\UE_5.8`。
- r0：建立 Owner sidecar 内的 P6 `Prepared → Committed` receipt/session；r1 首次接入正式 CTA，但遗留直接 observer recovery；r2 通过真实 trace 证明“重启后新 RunId + 旧 session 拒绝”的冲突；r3 仅改该冲突的 Code B receipt 恢复语义与最小只读 Code A context。

本轮新增/修改：

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp`
- `Source/demo_map/demo_mapProfilePreparationFlow.h/.cpp`
- `Source/demo_map/demo_mapV3ProgressionManager.h/.cpp`
- `Scripts/Run-P6r3ProductScenario.ps1`
- `Docs/Prompt/Dev.D.UE.0.0.9B.P6.0.r3_prompt.md`
- `PROJECT.md`、`PROJECT_INFO_CARD.md`、本 Report。

## Code B receipt 与 rebind 审查

`FCodeBRunInventoryBridgeReceipt` 增加并持久化：

- 稳定 `ReceiptId`；
- 永不变更的 `OriginRunId`；
- 当前 binding `RunInstanceId`；
- 原 `SourceOutOfRaidRevision`、`MovedItemIds`、完整 snapshot/layout；
- 由 source revision 与每个 ItemId/Definition/Quantity/ParentContainerId/SlotIndex/ChildContainerId 构成的 `PayloadDigest`；
- 连续的 `RecoveryRebindHistory`：sequence、OldRunId、NewRunId、`RecoveredAbandon`、UTC 时间。

旧 r2 schema `1` receipt 在读取时仅作内存兼容提升：旧 RunId 同时成为 `ReceiptId` 与 `OriginRunId`，再从既有 immutable graph 计算 digest；第一次 r3 写入才序列化为 schema `2`。这不会从当前 P5 snapshot 重新挑选物品，也不会创建新的 P6 session。

`RebindPreparedRunInventoryReceipt` 只在以下条件成立时执行：Owner 匹配、唯一 active session 为 verified `Prepared`、session/receipt/source revision/item graph/digest 均通过校验、Code A context 的 `RecoveredAbandonRunId` 精确等于当前旧 binding、且 observer 提供不同的新 RunId。它先原子保存新 binding/history/revision，随后复用原本的 `FinalizePreparedRunInventoryReceipt` 单次抽取/提交；因此 warehouse 中未选中项不参与 rebind。任一缺失/损坏/不匹配 context，或已 `Committed` session/普通 active conflict，都返回拒绝。相同新 RunId 的重复 observer 落入现有 `AlreadyCommitted` 分支，保持幂等。

如果 rebind 写入后、提交前发生进程中断，当前 binding 已变为新 RunId；下一次 Code A 对该 binding 作 `RecoveredAbandon` 后可继续追加连续 history。不会回写或复活任何 Code A 旧 RunId。

## Code A / Code B 边界审查

- `demo_mapProfileSessionCoordinator` 仍独占 `RecoveredAbandon` 生命周期决定；r3 未修改 coordinator。
- `Fdemo_mapProfilePreparationFlow` 只在 `InitializeSession` 已返回 `RecoveredAbandonCommitted` 或 `RecoveredAbandonAlreadyCommitted` 后，保留该 snapshot 的旧 RunId 为 transient、只读 context；`Unbind` 清除它。
- `ObserveCodeBRunAfterActivation` 仍在 `StartPreparedProfileRun → ActivatePreparedProfileWorld` 成功后调用唯一 `NotifySuccessfulRun` observer。它仅随调用传递该只读 context，绝不据 bridge 结果回写 Code A。
- 重启恢复阶段不再以包含旧 terminal ActiveRunId 的 Code A snapshot 调用 P5 `OpenOrMigrate`；r3 只读读取已经存在的 Owner P6 sidecar，避免错误触发“结束当前 Run 后再整理”，随后仍由第二次真实 CTA 正常产生新 RunId。
- r1 的直接 `ObserveCodeBRunAfterActivation(Active)` compatibility 分支保留在非 r3 路径；r3 的 Initial/Recovery 分支均在该 legacy replay 之前返回，r3 的设计链路只允许真实 CTA 的正常 observer。

## 外层结果契约（实现，未作为 P 阶段验收运行）

`Scripts/Run-P6r3ProductScenario.ps1` 以独立 P6r3 automation root 启动 `UnrealEditor-Cmd.exe`，并仅在以下同时成立时返回 `0`：editor exit `0`、同 scenario/phase 的 machine-readable `PASS` result、非空 trace sequence、且无该子进程残留。editor 非零、超时、残留、缺失/无效/FAIL result 都返回非零。`-NegativeControl` 仅写 `FAIL` result，不启动 Unreal 且返回 `2`。

该包装器和 r3 product trace 是 F 的定向测试资产；P 阶段不再执行。

## P 阶段验证政策与 F 债务

用户最新明确规则：**P 阶段进行功能开发与代码审查，不进行实际测试；F 执行真正测试。** 本轮之后未再启动 Unreal、真实 CTA、截图、回归或 Game build。最终一次源修改（restart sidecar read path）按此规则未做后续 Editor 编译；只完成静态调用、字段、迁移和边界审查。

在该规则下，F 必须重新从干净隔离根执行，而不是引用本轮指令前的探索性运行：

1. wrapper negative-control 非零；
2. CompleteCarry、Empty、NotEnrolled、BridgeFailure、PreparedRecovery（Initial + Recovery）、ActiveSessionConflict（Initial + Recovery）、OutOfRaidLock 的七条真实 CTA；
3. 每条记录 `ProductStartRunCTA`、`CodeAStartRunRequested`、`CodeAWorldActivated`、`CodeBObserverDelivered`、需要时的 `CodeARecoveredAbandonContext`/`P6PreparedRebind`、`CodeBBridgeFinal`、`CodeAStartRunReturned`、`ProcessExit`；
4. PreparedRecovery 的 stable ReceiptId、OriginRunId、old/new binding、history、P5/P6 revisions、digest 和完整 item/container/child graph；
5. 重复同 NewRunId observer 的幂等结果；以及 F 的全量回归、Game build、可见/截图和最终来源审计。

先前在本轮指令变更前启动的包装器/CTA 探索运行不构成 P6r3 验收证据：其中存在旧地图参数、旧自动化根拒绝与被停止的 recovery 子进程；即使个别 wrapper 曾返回 `0`，也不得计入本报告的场景通过矩阵。它们不应替代 F 的干净执行。

## 静态审查命令

```powershell
Get-FileHash Docs\Prompt\Dev.D.UE.0.0.9B.P6.0.r3_prompt.md -Algorithm SHA256
rg -n -C 2 "ObserveCodeBRunAfterActivation\(Active\)|NotifySuccessfulRun\(|StartPreparedRunDirect\(|RecoveredAbandon|RebindPreparedRunInventoryReceipt|RunInventoryPayloadDigest|P6r3RestartRecovery|P6ProductStartBridgeR3Trace" Source\demo_map\CodeB\demo_mapCodeBOutOfRaidProfile.cpp Source\demo_map\demo_mapV3ProgressionManager.cpp Source\demo_map\demo_mapProfilePreparationFlow.cpp Source\demo_map\demo_mapProfilePreparationFlow.h
rg -n "P6ProductStartBridgeR3Trace|UnrealEditor-Cmd|NegativeControl|wrapper_exit" Scripts\Run-P6r3ProductScenario.ps1
```

这些命令只读取源码/脚本与 Prompt；不启动测试。
