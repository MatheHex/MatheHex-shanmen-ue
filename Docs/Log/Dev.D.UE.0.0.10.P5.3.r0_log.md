# Dev.D.UE.0.0.10.P5.3.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P5.3.r0`；
- 基线提交：`439f3de780eb91a27485e4a139a0061180ee7867`（P5.2）；
- 分支：`agent/0.0.10-p5-3-defense-resource-coordination`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标

把 P5.2 的原子资源批量提交升级为可恢复的跨权威协调：Items 是持久权威，玩家 vitality 是产品运行时权威。任何中断都不能造成资源已消耗而生命未提交、生命已提交而资源永久悬空、或 exact retry 二次扣减。

## Items durable intent

新增 request：

- `FShanmenItemRunResourceIntentRequest`；
- `FShanmenItemRunResourceIntentFinalizeRequest`。

Prepare canonical fingerprint 包含 context、ActiveRun、IntentId、触发前缀、元数据和全部有序 reservation/item identity。Prepare 将每个 reservation 的 PurposeId 原子替换为同一 durable metadata，并写成功 receipt；资源总量保持不变。

Finalize canonical fingerprint 额外包含 PrepareRequestId 和 external success decision。成功决策提交 `[0, TriggeredLineCount)`，取消其余行；失败决策取消全部行。两种决策使用不同稳定 RequestId，已有任一终态后另一决策返回 conflict。

`ValidateState` 从 append-only processed receipts 重建 prepare/finalize 关系，验证 claim/finalize revision 范围、部署物品、资源类型、reserve revision、预约状态与唯一 pending intent。Run finalize 遇到 pending intent 返回 `ResourceIntentConflict`。

## 持久服务

AuthorityService 在既有 mutex 与 `ExecuteCommandLocked` 中执行 prepare/finalize。GameInstance subsystem 只在 Game Thread 且 lifecycle Ready 时转发，并复用 `SynchronizeCommandState`。

故障注入覆盖 prepare 写盘失败、重试、重启重放、finalize 写盘失败、重试、重启重放。失败时内存 repository、authority revision 与 generation 均不提前发布。

## Vitality 恢复

`TryRestoreFromDurableIntent` 只接受满足原命令不变量的数据：有效 identity、非负 revision、有限 vitality/damage、合法 outcome 和伤害守恒。

`RecoverPendingExternalCommit` 先尝试原 `Commit`：

1. 已 processed 的 Impact 返回原 receipt；
2. 非 stale 错误原样失败；
3. fresh ledger + exact before 时重基到当前 revision 后提交；
4. fresh ledger + exact after 时导入 AlreadyCommitted receipt；
5. 其它状态不变并返回 stale。

PlayerHealth 仅在恢复结果为新 `Committed` 且 AppliedDamage 大于零时发布一次伤害事件；导入 already-after 状态不重复广播。

## Product adapter

Vitality metadata 使用版本前缀 `SMV1`，GUID 使用 Digits，float 使用 8 位 bit-pattern hex，revision/outcome 使用严格十进制。解析拒绝字段数量错误、非法 hex、溢出 revision、非法 outcome 与恢复后无效命令。

`BuildIntentRequest` 对照完整 request 与 result：

- result 必须 accepted、conserved 且与 request 的 canonical resolve 完全一致；
- 每个资源输入必须有唯一 LayerId 和 exact prepared equipment SourceInstanceId；
- triggered resource receipts 先写入，未触发资源层随后写入；
- prepare RequestId 由 owner/scope/ActiveRun/Impact/Resolution/行数与全部身份派生。

`CoordinateImpact` 在新 prepare 前先调用 `RecoverPendingIntent`，再按 Items prepare → vitality recover/commit → Items finalize 顺序运行。只有 fresh persisted prepare 后发生明确 vitality 拒绝，才 durable cancel 全部资源；replayed prepare 遇到 neither-before-nor-after 状态保持 pending，禁止猜测。

Run coordinator 在绑定 player vitality identity 后、发布 registry 前执行恢复。资源型 enemy-to-player Impact 进入协调器；普通 Impact 保持原 ledger 路径。

P5.2 的 demo_map `BuildCommitRequest`、`CommitTriggered` 和 subsystem direct commit forwarding 已删除，避免第二产品写路径。

## 测试实现

新增 repository 测试覆盖：

- prepare 不消耗资源；
- pending intent 重启与 exact replay；
- 第二 intent 拒绝；
- Run 提前 finalize 拒绝；
- external success 只提交触发前缀；
- external rejection 取消全部；
- 终态重启 replay 不二次磨损。

新增 service 测试覆盖两个 durable 命令的 pre-commit persistence failure、retry、generation、restart 和 replay。

新增 vitality 测试覆盖 exact before apply、exact after import、重复 recovery 与 ambiguous state fail-closed。

新增 adapter 测试覆盖 trigger/untrigger 排序、确定性 prepare/finalize identity、metadata round-trip、source mismatch 与 malformed metadata。

## 首次失败

首轮 focused adapter 命令退出 `1`。日志：

```text
Saved/Automation/Dev.D.UE.0.0.10.P5.3.r0_adapter_focused.log
SHA256=10080F846A27D5CE4BAA1AD7A043E87DE8ED12DCAC728CC11785D45D4AB6DB70
Assertion failed: Request.IsValid()
demo_mapShanmenDefenseResourceAdapterTests.cpp:127
```

根因是测试夹具漏填 `FShanmenDefenseLayer::LayerTags`，违反既有 CombatCore 契约。补 `Defense.Shield` 与 `Defense.LethalIntercept` 后恢复；没有修改产品 validation。

## Editor 构建

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 完整：`42/42`，Succeeded，退出码 `0`，`124.75s`；
- 夹具修正：`4/4`，Succeeded，退出码 `0`，`5.27s`；
- 最终删除旁路后：`18/18`，Succeeded，退出码 `0`，`66.47s`。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Queue | Fatal | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `Dev.D.UE.0.0.10.P5.3.r0_full.log` | `Shanmen.0_0_10` | 121 | 0 | 1 | 0 | `3CBDF33BE5355A806E3A9C6263FF4439409C802B5E1A714C5F3BE5FB9EF704E6` |
| `Dev.D.UE.0.0.10.P5.3.r0_itemusearmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | 0 | `B4172ECA6ED56C3F208BA8919B748473E63458A98DE1A863CE68C0B095080C3D` |
| `Dev.D.UE.0.0.10.P5.3.r0_hotbar.log` | `demo_map.P4.Hotbar` | 7 | 0 | 1 | 0 | `E3E58071C731AA86CC8127C178ECCB5A1051CEAAB6A807EAABA709F219B8003F` |

三组共 `174 Success / 0 Fail`，原生退出码均为 `0`。

## Changed-file gate

映射要求：

- ShanmenItems → `Shanmen.0_0_10.Items`；
- ShanmenCombatRuntime → `Shanmen.0_0_10.CombatRuntime`；
- defense adapter → Items + CombatCore + ItemUseAndArmor + Hotbar；
- combat coordinator → Product.CombatRunCoordinator；
- PlayerHealth → Product.PlayerVitality + 0.0.10 全量。

最终：

```text
SELF_TEST: PASS 8/8
REGRESSION_COVERAGE: PASS Changed=22 Rules=6 Required=8 Logs=3
```

父组 `Shanmen.0_0_10` 的单一健康日志覆盖全部 Shanmen 子组，两个 legacy demo_map 组使用独立日志。

## Game 构建

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 完整：`61/61`，Succeeded，退出码 `0`，`184.04s`；
- 最终删除旁路后：`17/17`，Succeeded，退出码 `0`，`69.92s`；
- 输出：`Binaries/Win64/demo_map.exe`，未启动。

## 静态与边界

- `git diff --check`：退出码 `0`；
- ShanmenItems forbidden product/Engine actor/damage/RNG：`0`；
- ShanmenCombatRuntime forbidden Items/product/Actor/World/damage/RNG：`0`；
- DefenseResourceAdapter forbidden legacy authorities/damage/RNG：`0`；
- demo_map direct resource commit product symbols：`0`；
- Source/Scripts：`22 files, 2317 insertions, 163 deletions`。

## 最终不变量

1. 一个持久 item authority 同时最多有一个外部资源 intent。
2. Prepare 只冻结，不改变资源总量。
3. Vitality 只接受 canonical result 派生或受验证 durable rehydrate 的命令。
4. exact before 只提交一次，exact after 只导入回执，其它状态失败关闭。
5. Items 终态要么提交触发前缀并取消余项，要么取消全部。
6. Pending intent 阻止 Run finalize。
7. 写盘失败不发布 repository Candidate。
8. 产品不存在绕过 intent 的 direct commit 入口。
9. CombatCore 与 CombatRuntime 保持对 Items 和 demo_map 的单向依赖边界。

## P/F 边界

只执行 P 阶段代码、静态检查、无头 `-NullRHI` Automation、Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-3-defense-resource-coordination/Docs/Report/Dev.D.UE.0.0.10.P5.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-3-defense-resource-coordination/Docs/Log/Dev.D.UE.0.0.10.P5.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-3-defense-resource-coordination>
