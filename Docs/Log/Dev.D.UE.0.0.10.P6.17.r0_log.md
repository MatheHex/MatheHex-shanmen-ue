# Dev.D.UE.0.0.10.P6.17.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.17.r0`；
- 基线提交：`6a00cc5f3742bd46d56c218fdfcf776db1217d07`（P6.16）；
- 分支：`agent/0.0.10-p6-17-threat-sample-watermarks`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标推导

P6.16 已把一次显式完成的 Orbit threat sample 原子收口，但 authority 会永久保存全部已消费 intent。调用方持续提供 sample 时，回执数量随 Run 长度增长；同时空 sample 不留 tombstone，旧非空 sample 仍可精确重放。

P6.17 因此冻结最小刷新语义：调用方继续拥有 cadence，Run authority 只记录每件 item 最新完成 sample 的单调水位线，并将 replay retention 限制为该最新 sample。没有引入时间、频率或 effect 数值。

## 设计决策

### 逐 item checkpoint

`LatestSamples` 以 `SourceItemInstanceId` 为键。checkpoint 保存确定性 sample ID、activation、detector、ordinal 与该 sample 的 exact intent IDs。sample ID 使用命名空间 `Shanmen.ControlledWeapon.ThreatSample.r1`，并纳入 Run/source/item 身份，避免不同 authority 或不同物品碰撞。

### 累计审计与有界重放分离

`AuthorityRevision` 继续累计已接受 intent，保留 `NumConsumedIntents()` 的既有语义；`ProcessedIntents` 只保留最新 sample 的 replay receipts。新增 `SampleCheckpointRevision` 表示 sample 级进展，使空 sample 也有可观察的单调提交，而不伪造 intent revision。

### 原子替换

新 sample 在 authority 副本中裁剪旧 item receipts、增加 sample revision、写入 checkpoint，再执行全结构验证。仅候选完全有效时提交。精确重放和所有拒绝路径均不推进任何 revision。

## 实现链路

1. 增加 `SampleExpired` 与 `SampleConflict` 拒绝原因；
2. 从 presence receipt 派生并验证 sample checkpoint；
3. 比较 item 最新 ordinal：旧值过期，同值要求 exact sample/payload，新值进入替换；
4. 非空新 sample 消费 intent 后安装 checkpoint，空新 sample 直接安装 checkpoint；
5. 替换前删除同 item 上一 checkpoint 的 retained receipts；
6. Authority 自校验 checkpoint 与 retained receipts 的双向归属；
7. 暴露 cumulative、retained、tracked、sample revision 与 latest ordinal 观察面；
8. Run Host 保持既有原子 finalization，最后 item 移除时继续 reset 全部 Run authority。

## 测试增量

Runtime `ThreatPresenceAuthority` 增加：

- checkpoint 初值与首次提交 revision；
- exact replay 不推进 sample revision；
- 同 ordinal、不同 payload 返回 `SampleConflict`；
- 后续 sample 只保留最新 receipts，累计 intent revision 不回退；
- superseded sample 返回 `SampleExpired`；
- 空 sample 推进 checkpoint、裁剪旧 receipts；
- 空 sample exact replay 幂等；
- 空 watermark 过期上一非空 sample。

产品 `OrbitThreatRouting` 增加 retained/checkpoint/ordinal 断言，并验证新空 sample 过期旧原子收尾。新增 `PerItemThreatSampleWatermarks`，证明两个 item 水位线独立、另一 item 的最新 sample 仍可重放、stale low-item replay 不改变状态、Run teardown 清空全部 checkpoint。

## 自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 测试完成跨度 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p617_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 8 | 0 | 0 | `0.133s` | `06EF0158B41D5D62F0856C86960D8FE3BC1BBAFAF0013B93CE259B9B17628E22` |
| `p617_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 31 | 0 | 0 | `0.516s` | `0B6F2FAC7D564AF88EC822DD642C9D8F32A43A1BBC1E1F45B8AD6E59465DC37C` |
| `p617_full.log` | `Shanmen.0_0_10` | 164 | 0 | 0 | `8.326s` | `EFE47EE3D8BAB7401353F71A3C2896079365CCB52EBAC5DCC0F611A3F62E8442` |

日志位于 `Saved/Automation/P617/`。三份日志各有一个实际 RunTests 命令、一个 queue-empty、Fail `0`、Fatal/unhandled/assert/ensure `0`，进程原生退出码均为 `0`。

## Changed-file gate

五个 Source 改动文件命中 CombatRuntime 与 ControlledWeaponRunHost 两条规则：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=11 Logs=3
SELF_TEST: PASS 14/14
```

完整 `Shanmen.0_0_10` 日志覆盖 CombatRuntime、Items、CombatRunCoordinator、ControlledWeapon Adapter / Controller / RunCommandRouter / RunHost / RunLifecycle / Session / WorldDelivery 与 WorldGameplay 共 11 个必跑组。

## 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor：getter 误置于 consume receipt，`C2065 SampleCheckpointRevision undeclared identifier`；在 action `16/32` 时终止无意义后续编译，native exit `1`；
- 修正后 Editor：`32/32` actions，`Result: Succeeded`，native exit `0`，`88.50s`；
- 生产代码固定后的 Game：`29/29` actions，`Result: Succeeded`，native exit `0`，`93.72s`；
- 静态审查补充同 ordinal 冲突测试后，最终 Editor：`4/4` actions，`Result: Succeeded`，native exit `0`，`9.53s`；
- 同一最终状态 Game：`3/3` actions，`Result: Succeeded`，native exit `0`，`10.79s`；
- `UnrealEditor-ShanmenCombatRuntime.dll` UTC：`2026-08-29T05:35:47Z`；
- `UnrealEditor-demo_map.dll` UTC：`2026-08-29T05:30:17Z`；
- `demo_map.exe` UTC：`2026-08-29T05:37:23Z`。

首次失败为已修复的源码错误；没有环境、内存、SDK、外层超时或链接错误。Game executable 未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- Source 新增 `535` 行、删除 `11` 行；
- 新增生产代码的 `Tick(`、`DeltaSeconds`、`SetTimer`、`ApplyDamage`、impact/vitality commit、spawn、Actor/World、RNG、Cooldown 与 Duration 扫描命中 `0`；
- 未增加 cadence owner、timer、damage/control/effect、Actor/World 状态、inventory 或存档状态；
- `NumConsumedIntents()` 保留累计语义，Run Host 返回类型扩宽为 `int64`；
- Registry、GameplayTags、Profile schema、存档、item authority、input、资产和 Build.cs 均未修改；
- 长期未跟踪历史文件未纳入本阶段 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

下一阶段应先由产品冻结 cadence owner 与玩家可感知的威慑效果，再决定 sample 何时刷新。若 cadence 未冻结，继续保持当前显式完成 API，不把 overlap、Tick 或 timer 隐式升级为伤害/控制规则。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-17-threat-sample-watermarks/Docs/Report/Dev.D.UE.0.0.10.P6.17.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-17-threat-sample-watermarks/Docs/Log/Dev.D.UE.0.0.10.P6.17.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-17-threat-sample-watermarks>
