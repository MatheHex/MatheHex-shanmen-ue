# Dev.D.UE.0.0.10.P5.0.r0 Development Log

## 目标

在 P4 canonical combat 与 P4.14 changed-file regression gate 之后，进入 P5 的局内资源权威：先把准备阶段带入 ActiveRun 的快捷栏消耗品建立 durable-first 使用链，避免局内 Runtime 先改 stack、持久 authority 后补账造成双写或崩溃复活。

## 基线

- 基线分支：`agent/0.0.10-p4-14-regression-coverage-gate`；
- 基线提交：`3b2a4d0d75422b8ba7223728a7f08e42c5f4d1fe`；
- 当前分支：`agent/0.0.10-p5-0-authority-quick-use`；
- 产品事实：准备原件在 `ShanmenItems` 中是 committed reservation/tombstone，局内物品与快捷栏是 Runtime projection；
- 纪律：authoritative consumption 必须先于局内 effect，旧 Code B 只保留无 Shanmen lifecycle 时的 compatibility fallback。

## 契约决策

### ActiveRun quantity

不在 persistent tombstone 上恢复一个可变 `Quantity` 字段。运行中剩余量由冻结 reservation amount 减去 append-only success consumption receipts 重建，避免出现 repository quantity、Runtime stack 与另一本账三份真值。

### CAS 与幂等

请求携带 `ExpectedQuantityBefore`。Repository 先从 receipt chain 重建 durable balance，再精确比较；不一致返回 `RunItemQuantityConflict`。Request fingerprint 覆盖 scope、owner、content、ActiveRun、item、amount、expected-before 与 purpose。

Runtime request id 由 owner、scope、ActiveRun、item、hotbar slot、before-stack 与固定 purpose 派生。Runtime 失败回滚后同一意图拥有同一 before-stack，因而重放既有 receipt；成功后 stack 改变，下一次合法使用自然得到新 RequestId。

### failure ordering

权威提交成功、Runtime 提交失败时，不尝试逆向删除 durable receipt。局内 item/health/cooldown 自身原子回滚，调用方可重试原意图。进程重启后 Runtime 以 durable balance rematerialize。

## 实现步骤

1. 扩展 `EShanmenItemTransactionOperation`、错误枚举、consume request 与 success receipt validity。
2. Repository 新增 request fingerprint、replay/conflict、Run/reservation/scope/CAS 校验与 append-only receipt。
3. `ValidateState` 新增 start-consume-finalize 顺序、单 reservation identity 与资源连续性验证。
4. Finalize extraction 以 durable consumed quantity 为恢复上限。
5. AuthorityService 与产品 GameInstance subsystem 接入 durable command，复用现有持久化、失败注入与 snapshot 同步。
6. ItemSubsystem 抽出只读 hotbar preflight；原写入 API 复用 preflight。
7. 修正准备原件 Runtime ownership：persistent original 使用 `DeployedItemIds`，run-acquired item 使用 `OriginRunId`。
8. Lifecycle adapter 实现 authority-first use；rematerialization 从 receipt history 扣除使用量并清理耗尽 binding。
9. ProfilePreparationFlow、V3ProgressionManager 与 PlayerController 改走通用 quick-use 入口；Shanmen 路径失败时不回退 Code B。
10. 新增 repository consumption 与 durable hotbar use/retry/restart 自动化。
11. 按 P4.14 映射执行全组回归，修正被真实覆盖暴露的历史测试债。
12. 补齐 preparation adapter 的映射规则并运行门禁。

## 关键自动化

### Repository

`Shanmen.0_0_10.Items.PreparedRunConsumption` 验证：

- `10 -> 9` receipt；
- exact replay；
- RequestId payload conflict；
- stale CAS 与 foreign ActiveRun 拒绝；
- snapshot reload 后 replay；
- extraction overclaim 拒绝；
- exact remaining finalization；
- terminal reload 不复活已消费单位。

### 产品桥

`Shanmen.0_0_10.Items.RunLifecycle.DurableHotbarUse` 验证：

- 准备药品 stack `3` 与 slot `4` materialization；
- `AfterItemMutation` 注入失败后 authority success、Runtime stack/health 回滚；
- exact retry 返回 durable `Replayed`，Runtime 仅提交一次；
- authority snapshot 只有一条 successful consumption；
- restart rematerialization 为 stack `2` 且 binding 保持。

## 首次失败与恢复

1. 最早一次 Automation 调用漏写 `-NoCompile`：进程退出码 `1`，只执行 SDK/build validation，没有形成测试证据。修正启动参数后再运行。
2. `items_initial`：`59` 个测试中 `58 Success / 1 Fail`。失败点是准备原件没有 `OriginRunId`，旧 ownership 条件把它当作 foreign item。生产语义确认准备原件不能伪造 run-origin，改用 materialization 已维护的 `DeployedItemIds`。
3. `durable_use_diagnostic` 验证修正后注入失败状态与 retry 状态；`durable_use_final` 通过。
4. 第一轮完整 changed-file 回归：Shanmen `114/114`；Profile `198/211`；Code B `57/60`；V2 `22/22`。失败断言全部定位为现行 schema/slot/capacity/whole-graph contract 已变而 fixture 未更新。
5. 修正 Profile 与 Code B 测试后，ItemEconomySchema 独立组首次运行 `19/23`，暴露同源 SpatialRing/Backpack 旧断言；修正后 `23/23`。
6. 第一次真实门禁正确报告 `demo_mapShanmenPreparationAdapterTests.cpp` 未映射。扩展 ItemProductAdapters regex 后通过。

失败日志 SHA-256：

- `items_initial`：`B0848C519DA3E5B82173640F35FB8B4C60C5D8BA5BC7583DB3ECC0619F826199`；
- `profile_final`：`A30C91C80A37B0045AB60A1C8E1383D2BA471FBAD6AEE12590792A42BE344B28`；
- `codeb_final`：`79EAA6F691C0B677A0BCBF3EEAA740A56BA7759602E2A24658B54DEDCBCE44B2`；
- `itemeconomy_final`：`B513469E4824E6D5AD4AFDC29465224011F6DAF5592DDAA1A83A3123967ABAAD`。

UE process exit code alone cannot represent test health：上述 assertion failure runs 仍返回 `0`。最终 acceptance 以日志内 `Result={Fail}`、queue completion 与门禁结果为准。

## 最终 Automation

统一命令形态：

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty"
```

结果：

- `Saved/Logs/Dev.D.UE.0.0.10.P5.0.r0_shanmen_final2.log`：`114 Success / 0 Fail`，SHA `52676BCAB8E76A1877C5791F9A67A1EC7BB5A7867E0407E6D46A56440B85088B`；
- `Saved/Logs/Dev.D.UE.0.0.10.P5.0.r0_profile_final2.log`：`211 Success / 0 Fail`，SHA `FF72C99EF55D246C610B3BE1DD8C2B35C14291694F5F4FA82F2FE2E2492B7ABB`；
- `Saved/Logs/Dev.D.UE.0.0.10.P5.0.r0_itemeconomy_final2.log`：`23 Success / 0 Fail`，SHA `7B9D76C9974F78D7F73F386B39E33FC93906E5E3DAC1AC7C27A30B1665FAA427`；
- `Saved/Logs/Dev.D.UE.0.0.10.P5.0.r0_codeb_final2.log`：`60 Success / 0 Fail`，SHA `54FA96E2A1A9337B25E07F3B78EFBFA639DB4462F83ABA997AC0CC63816B5158`；
- `Saved/Logs/Dev.D.UE.0.0.10.P5.0.r0_v2ranged_final2.log`：`22 Success / 0 Fail`，SHA `35AC19A15846877F197BCDF2AB794BD12AE1069D4F59BC91D8441B1AEAB5461D`。

五组均 queue empty、fatal/unhandled/ensure `0`、进程退出码 `0`。

## Changed-file regression gate

Self-test：`SELF_TEST: PASS 7/7`。

最终真实检查：

```text
REGRESSION_COVERAGE: PASS Changed=30 Rules=6 Required=6 Logs=5
```

映射证据：

- `Shanmen.0_0_10` 与 `Shanmen.0_0_10.Items` → `shanmen_final2`；
- `demo_map.Profile` → `profile_final2`；
- `demo_map.ItemEconomySchema` → `itemeconomy_final2`；
- `demo_map.CodeB` → `codeb_final2`；
- `demo_map.V2RangedCompatibility` → `v2ranged_final2`。

## 构建

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 最终源码：`57/57`，Succeeded，退出码 `0`，`179.49s`；
- test-only ItemEconomy correction 后增量：`4/4`，Succeeded，退出码 `0`，`6.39s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `86/86`，Succeeded，退出码 `0`，`259.60s`；
- `Binaries/Win64/demo_map.exe` 生成但未启动。

没有 commit-memory、pagefile、编译器源码错误或构建重试。

## 静态检查

- `git diff --check`：退出码 `0`；
- `ShanmenItems` 精确边界扫描：`demo_map` include、`UWorld`、`AActor`、`ApplyDamage`、`FMath::Rand`、`FRandomStream`、`RandRange` 均 `0`；
- changed-file gate：退出码 `0`；
- Source/Scripts changed files：`30`；
- 未暂存长期未跟踪文件。

## 最终不变量

1. 准备 Quantity 的局内余额只有 reservation amount 与 ordered durable receipts 两个输入。
2. Runtime stack 只是 projection，不是持久权威。
3. authority receipt 必须先于 item/heal/cooldown effect。
4. Runtime rollback 不删除 durable receipt；exact intent replay 不重复持久扣减。
5. restart materialization 必须扣除全部 successful consumption。
6. extraction 不能恢复超过 durable maximum 的数量。
7. Shanmen lifecycle 存在时产品 quick use 不回退 Code B。
8. 准备原件与 run-acquired item 使用不同、明确的 ownership evidence。
9. 每个 changed path 的映射组必须有健康日志，不能靠主题选测。

## P/F 边界

本轮只执行 P 阶段源码开发、静态审查、无头 `-NullRHI` Automation、Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-0-authority-quick-use/Docs/Report/Dev.D.UE.0.0.10.P5.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-0-authority-quick-use/Docs/Log/Dev.D.UE.0.0.10.P5.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-0-authority-quick-use>
