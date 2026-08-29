# Dev.D.UE.0.0.10.P6.19.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.19.r0`；
- 基线提交：`e908dd833778b7acc69c50262852ee3ba809c065`（P6.18）；
- 分支：`agent/0.0.10-p6-19-threat-sample-freshness`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 根因与目标推导

P6.17/P6.18 的 authority checkpoint 只在消费时推进。若 sample N 已生成，随后 sample N+1 打开或关闭但尚未消费，authority 仍把 N 视作 latest consumed sample，因此直接 `TryConsumeOrbitThreatPresence` 可以把旧 receipt 当作 exact replay。类似地，Controller 离开 Orbiting 后仍暂时保留在 Host，既有 action identity 与 registration 仍匹配，最新 receipt 也可被 authority 重放。

Execution 自身已经拥有单调 `NextEmissionOrdinal` 和当前状态，因此 freshness 应先在生成 owner 处判定，再进入消费 authority。本阶段不定义 cadence、威慑伤害、控制或持续时间。

## 设计决策

### Latest completed means exact next ordinal predecessor

合法 receipt 必须满足：Execution 有效、状态为 Orbiting、没有活动 emission、receipt/action/detector 完整匹配，并且 `receipt.HitOrdinal + 1 == NextEmissionOrdinal`。开始 N+1 后因 emission active 拒绝 N；结束 N+1 后因 ordinal 不匹配拒绝 N。

### Generation fence precedes consumption fence

Execution freshness 负责“是否仍是当前完成 sample”；authority checkpoint 继续负责“是否已经消费、是否 exact replay、是否 conflict”。两个职责不合并，也不提前写入空 checkpoint。

### Product state remains authoritative

Host 只允许 Active Orbiting、无活动 contact window 的 Controller 消费 presence，并调用 Execution 的 canonical freshness helper。Host 不复制 ordinal 算法。每件 item 独立拥有 Controller/EmissionSession，因此跨 item replay 不受影响。

## 实现链路

1. 在 ControlledWeapon Execution 暴露只读 `IsLatestCompletedOrbitThreatEmission()`；
2. helper 验证 Execution、Orbiting、inactive emission、receipt、detector、action 与 ordinal；
3. policy evaluation 要求 ActionRuntime 仍可发射候选，并复用 helper；
4. presence intent 构造复用同一 helper；
5. 删除上述两条路径中重复的逐字段 action/detector 比较；
6. RunHost `PresenceMatchesController()` 增加 Active Orbiting、无窗口与 canonical freshness fence；
7. authority consumption、registration、checkpoint 与 revision 实现保持不变。

## 测试增量

Runtime `OrbitThreatTargetPolicy` 增加 open-newer 与 completed-newer 对旧 emission 的拒绝；`OrbitThreatPresenceIntents` 增加 completed-newer 对旧 policy 的拒绝，并验证 newest policy 正常构造。

产品 `PerItemThreatSampleWatermarks` 在 low item 的 N+1 开启与完成两个时点直接 replay N，均要求拒绝且 authority 不变；随后 N+1 finalization 正常推进，并继续验证 high item 独立 replay。`OrbitThreatRouting` 在 Launch 后重放最新 empty presence，验证 Directed 状态立即拒绝。

## 自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 测试完成跨度 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p619_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 8 | 0 | 0 | `0.117s` | `EE2BDEB6AF4A3D383E9E1F476A1A6BFDE981D99327E7EB1C08C6EAC3A83BB4DB` |
| `p619_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 31 | 0 | 0 | `0.517s` | `A71468432A2E3125C73FC3903B6F43F6BBC87A74F7592DFA64FE12F0D43BDCD4` |
| `p619_full.log` | `Shanmen.0_0_10` | 164 | 0 | 0 | `8.256s` | `0835EC1D79DB4DD8E263C49AA1688BE1265E6D55CA99685613DEA1DDE715AA0B` |

日志位于 `Saved/Automation/P619/`。三份日志各有一个实际 RunTests 命令、一个 queue-empty 终止、Fail `0`、Fatal/unhandled/assert/ensure `0`，进程原生退出码均为 `0`。

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

- 首次 Editor：`43/43` actions，`Result: Succeeded`，native exit `0`，`124.15s`；
- 首次 Game：`40/40` actions，`Result: Succeeded`，native exit `0`，`116.14s`；
- 最终测试增量后的 Editor：`4/4` actions，`Result: Succeeded`，native exit `0`，`5.42s`；
- 同一最终状态 Game：`3/3` actions，`Result: Succeeded`，native exit `0`，`11.32s`；
- `UnrealEditor-ShanmenCombatRuntime.dll` UTC：`2026-08-29T06:18:30.0176487Z`；
- `UnrealEditor-demo_map.dll` UTC：`2026-08-29T06:24:16.5766844Z`；
- `demo_map.exe` UTC：`2026-08-29T06:25:38.5233279Z`。

四次构建都在首次执行成功；没有源码、环境、内存、SDK、外层超时或链接失败。Game executable 未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- Source 新增 `75` 行、删除 `42` 行；
- 新增生产可执行代码的 cadence/timer、damage/control/effect commit、spawn、Actor/World、RNG、Cooldown/Duration 扫描命中 `0`；
- 未增加 Tick/timer、威慑 effect、vitality mutation、World 状态或资源事务；
- current sample exact replay、authority revision、bounded retention、activation retirement 与多 item 独立水位线保持兼容；
- Registry、GameplayTags、Profile schema、存档、item authority、input、资产和 Build.cs 均未修改；
- 长期未跟踪历史文件未纳入本阶段 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

freshness 与 activation 生命周期已闭合。下一阶段若继续推进结构层，可增加 Host-owned 的显式 sample transaction，原子组织 Begin/Project/End/Finalize；cadence 触发频率与 threat gameplay effect 仍应保持独立冻结。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-19-threat-sample-freshness/Docs/Report/Dev.D.UE.0.0.10.P6.19.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-19-threat-sample-freshness/Docs/Log/Dev.D.UE.0.0.10.P6.19.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-19-threat-sample-freshness>
