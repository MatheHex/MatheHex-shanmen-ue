# Dev.D.UE.0.0.10.P6.15.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.15.r0`；
- 基线提交：`8aee5ab2c96ad4417d87246d07661f3a465a1988`（P6.14）；
- 分支：`agent/0.0.10-p6-15-threat-presence-authority`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标推导

P6.14 只生成确定性的零效果 `ThreatPresenceIntent`，尚无谁负责接收、如何去重、是否允许部分提交以及 Run 结束后如何回收的权威。若直接进入 cadence 或效果层，重放同一 overlap receipt 可能重复施加未来效果。因此本阶段先冻结消费 authority 和幂等 ledger，不定义任何 gameplay 数值。

## 架构判断

### Authority 属于 Run Host

presence intent 的 ID 含 source item，但一个 Run 中可存在多把飞剑。将 ledger 放在单个 Controller 会让跨物品 revision 无统一顺序；放在 World 或全局 subsystem 又会扩大生命周期。因此产品 `RunHost` 持有一个绑定 `RunId + SourceEntityId` 的 authority，所有 item 通过 exact route 单向提交。

### 生产与消费分离

`ControlledWeaponExecution` 继续只生产 intent。消费状态、revision、幂等和冲突逻辑位于独立 `ShanmenControlledWeaponThreatPresenceAuthority` 单元，避免执行类同时成为 ledger owner。

### 原子而非逐条副作用

消费先完整校验 receipt、authority 绑定和 replay 状态，再在 authority 副本上生成所有 receipt。只有 candidate authority 与结果都有效才整体提交；无任何路径在失败时留下部分 revision。

## 契约

- `Consumed`：首次完整批次，revision 逐 intent 连续递增；
- `AlreadyConsumed`：所有 intent 均已有完全匹配 receipt，返回原 revision，authority 不变；
- `NoOp`：有效完成 sample 没有 accepted intent，revision 不变；
- `Rejected`：输入或 ownership 冲突，返回精确错误且 authority 不变；
- default-constructed result/authority 均无效，必须显式创建；
- Run Host 在消费前核对 exact item 与 live action 的所有稳定身份字段；
- 最后一个 terminal item 移除时，Host reset 同时回收 ledger；
- authority 不读取或写入 vitality、impact、World、Actor、item/inventory 或 frame timing。

## 实现链路

1. Runtime 新增 consume status/error、immutable consume receipt、atomic result 与 Run-scoped authority；
2. authority 对 deterministic intent ID 建 ledger，并保留首次 canonical intent 与 authority revision；
3. Run Host 首次 attach 时与 Run/source 一起创建 authority；
4. Host validity 把 authority binding 纳入不变量；
5. `TryConsumeOrbitThreatPresence` 只接受 exact controller action 的 receipt；
6. Host 最后 item 退休时复用既有 reset 生命周期清空 authority；
7. Runtime 与产品测试覆盖首次、重放、后续 sample、空 sample、foreign ownership、未知 item、零 effect 和生命周期。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 运行段 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p615_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 8 | 0 | 0 | `5.78s` | `BEEB8556EEF5CBACCBC59FDCDD830522D34553F5F3A315E4909174DD2FF68D46` |
| `p615_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 30 | 0 | 0 | `6.11s` | `7F9BB55E96A03AC2A0EA520B379670D7C06EBBBE411AC7B97FF74FC6556D51DD` |
| `p615_full.log` | `Shanmen.0_0_10` | 163 | 0 | 0 | `14.36s` | `05890B2B87FE9CB7CB824A04CDD44B723832667D5B5031F2F213FAFB63309D40` |

三份日志位于 `Saved/Automation/P615/`。每份有一个实际 RunTests 命令、一个 queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与 native exit `0`。测试发现前各有 13 条既存 `LogAutomationTest: Error: Condition failed` 固定诊断噪声，目标测试全部 Success。

## Changed-file gate

最终 Source 状态：

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=11 Logs=3
```

完整 `Shanmen.0_0_10` 覆盖 CombatRuntime、Items、CombatRunCoordinator、ControlledWeapon Adapter / Controller / RunCommandRouter / RunHost / RunLifecycle / Session / WorldDelivery 与 WorldGameplay 共 11 个必跑组；Runtime 与 Product focused 日志提供直接证据。Regression coverage self-test：`14/14 PASS`。

加入 Report / Log 后 Docs 属于 mapping ignored path，最终 staged gate 仍为 `PASS Changed=8 Rules=2 Required=11 Logs=3`。

## 构建与复审

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首版 authority 已通过 Editor 与全部自动化；没有失败；
- 收口时主动将约束/ledger 从 `ControlledWeaponExecution.cpp` 拆为独立 Runtime 单元；
- 拆分后 Editor：`43/43` actions，`Result: Succeeded`，native exit `0`，`147.88s`；
- 拆分后重新执行三组自动化、changed-file gate 与 self-test，全部通过；
- Game：`40/40` actions，`Result: Succeeded`，native exit `0`，`135.21s`；
- 没有源码构建失败、环境错误、内存错误或外层超时；Game executable 未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- Source 6 个文件，新增 `723` 行、删除 `4` 行；生产新增 `514` 行、删除 `2` 行；
- authority 生产单元的 damage/impact delivery、UWorld、AActor、Spawn、RNG、item/inventory mutation 命中 `0`；
- `Reserve` 唯一命中是 `TArray::Reserve` 容量预留；
- Registry、GameplayTags、Profile schema、存档、Item authority、input、资产和 Build.cs 均未修改；
- 最新测试源码 UTC `2026-08-29T04:20:43.4219219Z`；
- Runtime Editor DLL UTC `2026-08-29T04:21:24.4682019Z`；
- demo_map Editor DLL UTC `2026-08-29T04:23:35.9973174Z`；
- Game executable UTC `2026-08-29T04:27:50.4382580Z`；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

下一阶段应先冻结 presence 的显式 sample/cadence authority 与 ledger 生命周期上限，再定义每目标 cooldown 或可观察效果。不能把每帧 overlap 默认解释成伤害、硬控或无限期状态，也不应在未定义回收策略前长期高频累积 intent。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-15-threat-presence-authority/Docs/Report/Dev.D.UE.0.0.10.P6.15.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-15-threat-presence-authority/Docs/Log/Dev.D.UE.0.0.10.P6.15.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-15-threat-presence-authority>
