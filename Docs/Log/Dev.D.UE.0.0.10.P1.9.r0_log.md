# Dev.D.UE.0.0.10.P1.9.r0 开发日志

## 基线

- 日期：2026-08-27（America/New_York）
- 分支：`agent/0.0.10-p1-9-run-lifecycle`
- 基线提交：`48593d87d17e8d850096d7858ff9f4db18c352fb`
- 上一阶段：P1.8 atomic prepared loadout commit
- 阶段目标：让 P1.8 receipt 在不经过 Profile item-bearing `ActiveRun` 的前提下，完成 claim、Runtime materialization、Extraction settlement、terminal marker 与重启恢复。

## 开工审查

确认现有边界：

- ShanmenItems 已是局内准备物品的唯一事务权威；
- P1.8 `CommitBatch` receipt 能重建装备、完整 Stack 顺序和 Hotbar；
- Runtime subsystem 已具备 plan materialization、同 `ActiveRunId` 恢复和 snapshot；
- 既有 Settlement 仍服务旧 Profile/Code B，不能直接作为 P1.9 writer；
- 产品 `bCanStartRun` 仍关闭，适合先实现并验证内部闭环。

据此选择在 ShanmenItems processed-request ledger 上增加 lifecycle operation，不增加旁路存储或新 schema 字段。

## ShanmenItems 实现

### Operation 与请求类型

增加：

- `ClaimPreparedRun`；
- `FinalizePreparedRun`；
- `FShanmenItemRunClaimRequest`；
- `FShanmenItemRunFinalizeRequest`；
- `FShanmenItemRunSecuredOriginal`；
- terminal outcome / lifecycle purpose 常量。

请求 fingerprint 覆盖 owner、scope、content、prepared batch、active run、terminal outcome 以及按稳定顺序排列的 secured originals。

### Claim 规则

- prepared batch 必须是成功 `CommitBatch` ledger entry；
- batch 中所有 reservation 必须仍处于 committed 状态；
- 同一 authority 同时最多一个 active prepared run；
- `ActiveRunId` 从稳定 canonical parts 确定性派生；
- exact retry 与重启 replay 不增加 authority revision。

### Finalize 规则

- 只接受当前 active claim；
- 只开放 Extraction；
- equipment original 必须 secured 且 amount 精确；
- quantity original 可返回 `0..committed amount`；
- placement 来自 reservation Purpose envelope；
- remaining Stack 返回原容器/格位；
- 全部 reservation 在一个 authority revision 内 release；
- terminal receipt 阻止重复结算；
- durable write 失败恢复命令前 snapshot，retry 后只写一次成功状态。

### Repository / owner 串接

repository、authority service 和 `Udemo_mapShanmenItemAuthoritySubsystem` 均增加 durable claim/finalize API；owner 在每次命令后同步 Ready/RecoveryRequired 状态。

## Runtime / Settlement Adapter

新增：

- `demo_mapShanmenRunLifecycleAdapter.h`
- `demo_mapShanmenRunLifecycleAdapter.cpp`

Start 流程：

1. 重建或提交当前 P1.8 prepared loadout；
2. durable claim；
3. 从 receipt 构建 Runtime plan；
4. Runtime 以 receipt 的 `ActiveRunId` prepare/materialize；
5. materialization 注入失败时仅回滚 Runtime，claim 留在 ledger 供下次恢复；
6. 重启后重建同 prepared receipt、同 claim、同 `ActiveRunId`。

Settlement 流程：

1. 检查 Runtime active summary 与 authority claim 一致；
2. 只接受 Extraction；
3. 对 prepared originals 生成 secured-original summary；
4. 发现新 ItemInstanceId、重复身份或不一致 definition 时 fail closed；
5. durable finalize；
6. terminal replay 返回 NoChange，不再次恢复资源。

## Preparation placement 与向后兼容

P1.9 将 Quantity intent 的逻辑 Purpose 与原始 `ParentContainerId + SlotIndex` 编码在同一 FName 中。prepared receipt 将 placement 解码为显式 line 字段，供 finalize 精确返还。

审查时发现 `FShanmenItemReservationPlacement::Decode` 在失败时会重置输出参数；若直接把原始 Purpose 当作输出，P1.7/P1.8 plain Purpose 会变成 `NAME_None`。修复方式：

- 原始 Purpose 始终保留为 fallback；
- Decode 写入独立 `DecodedLogicalPurpose`；
- 仅在 Decode 成功时替换 logical purpose；
- `HasRunInventoryPurpose` 使用同样规则。

新增 `PlainPurposeReceiptCompatibility`，直接制造旧格式 reservation，验证 atomic commit、receipt 字段、重启重建和无伪造 placement。

## Manifest 验证修复

第一次运行 manifest 定向测试得到 0/2：

- P73.4 reward source policy prototype 未绑定具体 slot，却被 concrete projection 的 `IsValid()` 拒绝；
- 旧测试仍断言四槽 manifest，而当前定义已是五槽，并错误期待 accessory 为 WindTalisman。

修复：

- 增加 `IsPolicyPrototypeValid()`；
- registry/definition 全局检查只验证 policy prototype；
- concrete `IsValid()` 继续要求 `DistributionProfileId + SlotId`；
- 测试更新为 EvasionCharm accessory、WindTalisman spatial ring 与五槽契约。

修复后 manifest 定向测试 2/2 Success，完整回归继续覆盖该结果。

## 自动化测试

### 生命周期最终定向

测试：

- `Shanmen.0_0_10.Items.PreparationAdapter.PlainPurposeReceiptCompatibility`
- `Shanmen.0_0_10.Items.PreparedRunLifecycleLedger`
- `Shanmen.0_0_10.Items.RunLifecycle.ClaimRestartFinalize`

结果：3/3 Success、0 Fail、queue empty、原生退出码 0。

日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.9.r0_lifecycle_final.log`

SHA-256：`7735CF0F34AAEBE39655F74611718EBC62E0B67B4698A6A89ACCFBBE28607752`

### 完整 0.0.10 回归

命令语义：

`UnrealEditor-Cmd.exe <uproject> -unattended -nop4 -nosplash -nullrhi -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"`

结果：60/60 Success、0 Fail、queue empty、原生退出码 0。

日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.9.r0_automation_final.log`

SHA-256：`97338D0714FEE18A822A1798A530E71BACDE98A18439E0DF8D406B7EE3D5D79E`

### Manifest 定向

- 首次：0/2；日志 SHA-256 `FFAA2E3ADC0910E3DF4FFB8A8E2A16E1FD950531B41E21AA5C031BC78407610E`；
- 修复后：2/2 Success；最终日志 SHA-256 `4CA0513EDF36D93516BB062280E2FF80A91A69BD6787E45B1A08018933C4A3B4`。

首次失败属于真实测试/契约问题，未描述为环境故障。

## 构建

统一参数：

`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

本阶段完整依赖构建：

- Editor：137/137 actions，成功，372.34 秒，原生退出码 0；
- Game：144/144 actions，成功，392.75 秒，原生退出码 0。

最终 parser/test 变更后的增量构建：

- Editor：5/5 actions，成功，16.29 秒，原生退出码 0；
- Game：4/4 actions，成功，23.44 秒，原生退出码 0。

## 静态审查

- `git diff --check`：0；
- P1.9 ShanmenItems/adapter 路径中的 World、Actor、伤害、GameplayStatics、随机数、Profile save 与旧 BeginRun 调用：0；
- adapter 的 Profile/Code B 持久化 writer：0；
- 代码保留已有 Runtime POD 类型映射，但不写 Profile；
- unrelated dirty/untracked files 未修改、未删除、未加入提交。

## 有意保持关闭

- 产品 `bCanStartRun`；
- Death/Abandon terminal policy；
- Runtime new-loot canonical import；
- 旧 plain Purpose 的来源格位猜测；
- UI、PIE、Standalone、Cook、Package 与真实输入验证。

## 下一阶段入口

P1.10 先实现 new-loot import/placement 和 Death/Abandon policy，再把已验证的 lifecycle adapter 接入唯一产品 Start 入口。这样产品开启时，所有 terminal path 都已有 ShanmenItems 权威语义，而不是依赖旧 Profile/Code B 旁路。
