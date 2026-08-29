# Dev.D.UE.0.0.10.P6.21.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.21.r0`；
- 基线提交：`debf6624db80e520c104a419fa95387aba0625f8`（P6.20）；
- 分支：`agent/0.0.10-p6-21-atomic-threat-batch`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 问题与目标

P6.20 的 `TrySampleOrbitThreat()` 对一个 exact item 已是 Host-wide 原子事务，但调用方若要同一采样时机处理多件飞剑，只能连续调用。前一 item 成功、后一 item 失败时，前者已经推进 detector ordinal、sample checkpoint 与 presence authority；调用方无法把“一次多飞剑采样”视为一个事务。此外，若调用顺序来自无序容器，authority revision 分配会随迭代顺序漂移。

P6.21 的目标是提供显式多 item 批次：稳定 GUID 顺序、完整 Host 候选、全部成功才提交。阶段不拥有 cadence、World query、效果或数值。

## 设计决策

### 显式子集，不扫描 Host

调用方提交 `Fdemo_mapShanmenControlledWeaponThreatSampleRequest` 数组，每项携带 exact item 与其 contacts。Host 不自动枚举所有绑定或 Orbiting item，因此调用方仍拥有采样时机、空间查询和本次参与对象。

### 排序前复制，排序后统一预检

请求复制到局部数组并按既有 `GuidLess` canonical order 排序。排序后一次检查无效 ID、未知 item 与相邻重复 ID；所有结构性错误都在任何 sample ordinal 产生前返回。

### 整个批次共享一个 Host 候选

完整 Host 只在预检通过后复制。每项在该候选上调用 P6.20 单 item 事务；P6.20 内部虽有自己的候选提交，但提交目标只是批次候选。任一后项失败时，外层候选整体丢弃，所以之前成功项的 Controller 与 Run-scoped authority 变化都不会进入真实 Host。

### 回执自身强制 canonical 语义

`Fdemo_mapShanmenControlledWeaponThreatSampleBatch::IsFullyFinalized()` 检查：

1. RunId 有效；
2. attempted/finalized/entries 数量严格相等且大于零；
3. 每项 exact item 与 finalization item 一致；
4. 每项 action RunId 与批次 RunId 一致；
5. entries 按 GUID 严格递增，从而同时拒绝乱序与重复。

## 实现链路

1. 清空输出并验证 Host、Coordinator、非空 requests；
2. 复制并稳定排序 requests；
3. 对每项执行 invalid/unknown/duplicate 预检；
4. 复制整个 Run Host；
5. 按 canonical item order 对候选调用 `TrySampleOrbitThreat()`；
6. 每项成功后写入 exact finalization entry；
7. 任一失败立即丢弃局部 Batch 与 Host candidate；
8. 全部完成后验证 batch 与 candidate；
9. 一次 move 提交 Host 和输出回执。

## 测试增量

新增 `AtomicThreatSampleBatch`：

- 反序输入仍生成 low/high 稳定顺序；
- 首轮两项 ordinal 独立为 `0`；
- authority receipt revision 按稳定顺序为 `1`、`2`；
- 交换成功回执 entries 或替换 RunId 后自检失败；
- low 先成功、high duplicate contact 后失败时整批回滚；
- 回滚后两项 ordinal、checkpoint、authority 与 consumed intents 均不变；
- 两项空 sample 重试复用 ordinal `1` 并提交两个 `NoOp`；
- duplicate request、unknown item、empty batch 失败关闭；
- 无 active window、vitality 或 impact 变更。

## 自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p621_runhost.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 9 | 0 | 0 | `0.148s` | `7F6657A8F23FC2AF7AE723C1F2C363EA9E7E058CD45194AB120502354AB4AD9F` |
| `p621_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 33 | 0 | 0 | `0.592s` | `CCB4DB43F241F7B77B0F048CF6FDE5B41EEDDE55D20F9CC4458D4D9610965D41` |
| `p621_full.log` | `Shanmen.0_0_10` | 166 | 0 | 0 | `8.245s` | `CF5FA9DCF9D315CA7F51EA81A61EB645A41B7BB8D0EA028EAE7FFF811AD9CCE0` |

日志位于 `Saved/Automation/P621/`。最终日志各有一个 RunTests 命令、一个 queue-empty marker、Fail `0`、Fatal/unhandled/assert/ensure `0`，原生退出码均为 `0`。

首轮实现和最终回执断言增强轮都通过；没有失败自动化需要保留或解释。

## Changed-file gate

三个 Source 文件命中 ControlledWeaponRunHost 规则，要求 11 个测试组：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=11 Logs=3
SELF_TEST: PASS 14/14
```

完整日志覆盖 CombatRuntime、Items、CombatRunCoordinator、ControlledWeapon Adapter / Controller / RunCommandRouter / RunHost / RunLifecycle / Session / WorldDelivery 与 WorldGameplay。

## 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 初始 Editor：28/28，Succeeded，native exit `0`，`90.14s`；
- 回执断言后的最终 Editor：4/4，Succeeded，native exit `0`，`5.53s`；
- 初始 Game：27/27，Succeeded，native exit `0`，`87.43s`；
- 最终 Game：3/3，Succeeded，native exit `0`，`11.37s`。

最终 `UnrealEditor-demo_map.dll` UTC 为 `2026-08-29T07:28:28.7348060Z`，`demo_map.exe` UTC 为 `2026-08-29T07:30:16.7266437Z`。所有构建首次执行成功，Game executable 未启动。

## 静态、兼容性与范围

- `git diff --check`：native exit `0`；
- Source `+315/-0`，生产 `+148`，测试 `+167`；
- 新增生产行的 Tick/Timer、ApplyDamage/GameplayEffect、SpawnActor、World/RNG、Cooldown/Duration 扫描命中 `0`；
- 既有分步 threat API、P6.20 单 item API 与 finalization result 保持兼容；
- 未增加 cadence、World query、effect、vitality mutation、impact commit 或资源事务；
- 未修改 schema、存档、tags、item authority、input、资产或 Build.cs；
- 长期未跟踪 0.0.9B 文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

下一阶段把外部 cadence owner 的一次采样时机和 caller-collected contacts 适配到本批次。不要把 Tick/timer、overlap query、damage 或 gameplay effect 放进 Run Host；这些仍是独立产品策略。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-21-atomic-threat-batch/Docs/Report/Dev.D.UE.0.0.10.P6.21.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-21-atomic-threat-batch/Docs/Log/Dev.D.UE.0.0.10.P6.21.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-21-atomic-threat-batch>
