# Dev.D.UE.0.0.10.P6.18.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.18.r0`；
- 基线提交：`bdf2b4c975e367540ae11e905d91620653886d7d`（P6.17）；
- 分支：`agent/0.0.10-p6-18-threat-activation-lifecycle`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 根因与目标推导

P6.17 将 replay retention 限制为每件物品最新 sample，但 checkpoint 的生命周期仍等同于整个 Run。若 item A 已 terminal 并被移除，而 item B 仍存活，Host 不会 reset authority，因此 A 的 checkpoint 与 retained receipts 继续存在。

这造成两个结构性问题：退役 item churn 会扩大 authority；同一物理 item 在同 Run 中以新 activation 重新 attach 时，其 detector ordinal 会从 `0` 重启，却会与旧 activation 的 item checkpoint 冲突。P6.18 因此把消费准入与 Controller 生命周期对齐，并要求退役只清理 exact item/activation 的状态。

## 设计决策

### Exact activation admission

Authority 新增 `RegisteredItemActivations`，以 item 为键保存当前唯一 activation。登记相同 pair 幂等；同 item/different activation 与 different item/same activation 均拒绝。Presence 在 sample 比较之前必须先通过 exact registration fence。

### Retirement is state-visible, not a new intent

Retirement 删除目标 item 的 checkpoint 与 retained replay receipts。若 checkpoint 确实存在，`SampleCheckpointRevision` 增加一次；`AuthorityRevision` 不变，因为没有接受新 intent。无 checkpoint 的 registration 退役不会制造 revision。

### Host atomicity

Attach 在 Host 候选副本中先加入 Controller，再登记其 action activation；terminal removal 在候选副本中先退役 exact activation，再删除 Controller。候选经完整 Host/authority `IsValid()` 后才替换 live 状态。

## 实现链路

1. 增加 `ItemNotRegistered` 与 `ActivationMismatch` consume error；
2. 增加 exact item activation registration、retirement 与查询接口；
3. Authority `IsValid()` 验证 registration ID、active activation 唯一性与 checkpoint 所属 activation；
4. Presence consume 在 Run/source fence 后验证 item 已登记且 activation 完全匹配；
5. Retirement 复用 checkpoint pruning，删除该 item 的 retained receipts 与 checkpoint；
6. Host attach 将 Controller 与 registration 放入同一候选事务；
7. Host terminal removal 将 exact retirement 与 Controller removal 放入同一候选事务；
8. Host `IsValid()` 要求 Controller 数与 registration 数一致，并逐项匹配 action activation；
9. 保留最后一个 item 退出时的既有整体 reset。

## 测试增量

Runtime `ThreatPresenceAuthority` 扩展覆盖：

- 未登记 item 返回 `ItemNotRegistered`；
- exact pair 重复登记幂等；
- active item 与 activation 双向唯一；
- 非 exact activation 无法退役；
- exact retirement 裁剪 checkpoint 与 retained receipts；
- retirement 不回退 lifetime intent revision；
- 同 item 可登记 replacement activation；
- 旧 activation presence 返回 `ActivationMismatch`；
- replacement activation 可消费 fresh sample；
- 无 checkpoint 的 replacement registration 退役不推进 sample revision。

产品 `PerItemThreatSampleWatermarks` 扩展覆盖：两个 active registrations、低序 item 独立退役、高序 checkpoint/replay 保留、同物理 item 新 activation 重挂、fresh ordinal `0` 接受、旧 activation 拒绝、replacement 再退役与最后 item teardown。

## 自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 测试完成跨度 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p618_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 8 | 0 | 0 | `0.124s` | `B158B14179F19C8DDACD18D2E2931116073C24FA36C6843C112E0CDF1A9FD08D` |
| `p618_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 31 | 0 | 0 | `0.545s` | `7A69D2C958D0295DA3C2AB9F5479C3E43CF32535EA8F71837AEDA6F17E00E984` |
| `p618_full.log` | `Shanmen.0_0_10` | 164 | 0 | 0 | `8.213s` | `75C9CC3E361493DA50A27AC46459478180435F7DA91581726EB790CA9A215353` |

日志位于 `Saved/Automation/P618/`。三份日志各有一个实际 RunTests 命令、一个 queue-empty 终止、Fail `0`、Fatal/unhandled/assert/ensure `0`，进程原生退出码均为 `0`。

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

- Editor：`32/32` actions，`Result: Succeeded`，native exit `0`，`95.88s`；
- Game：`29/29` actions，`Result: Succeeded`，native exit `0`，`95.64s`；
- `UnrealEditor-ShanmenCombatRuntime.dll` UTC：`2026-08-29T05:48:12.7315351Z`；
- `UnrealEditor-demo_map.dll` UTC：`2026-08-29T05:49:38.9721216Z`；
- `demo_map.exe` UTC：`2026-08-29T05:53:11.6766496Z`。

Editor 与 Game 均为首次执行成功；没有源码、环境、内存、SDK、外层超时或链接失败。Game executable 未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- Source 新增 `310` 行、删除 `7` 行；
- 新增生产可执行代码的 cadence/timer、damage/control/effect commit、spawn、Actor/World、RNG、Cooldown/Duration 与 inventory transaction 扫描命中 `0`；
- 未引入 Tick/timer cadence owner、damage/control/effect、World 状态或资源事务；
- P6.17 的 cumulative intent revision、bounded replay retention、逐 item sample watermark 与 exact replay 接口保持兼容；
- Registry、GameplayTags、Profile schema、存档、item authority、input、资产和 Build.cs 均未修改；
- 长期未跟踪历史文件未纳入本阶段 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

下一阶段若仍在结构层推进，应先冻结 cadence owner、sample 触发时机与玩家可感知的 threat effect，再决定由哪个产品层调用现有显式完成 API。未冻结前继续保持当前 authority 无时间、无伤害、无控制、无库存副作用。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-18-threat-activation-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P6.18.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-18-threat-activation-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P6.18.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-18-threat-activation-lifecycle>
