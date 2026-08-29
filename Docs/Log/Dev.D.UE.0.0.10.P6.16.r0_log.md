# Dev.D.UE.0.0.10.P6.16.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.16.r0`；
- 基线提交：`f491ef6c778b020ef5f0ebae22273551b33c9edf`（P6.15）；
- 分支：`agent/0.0.10-p6-16-orbit-threat-finalization`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标推导

P6.15 已冻结 Run-scoped presence consumption authority，但产品调用者仍必须分别调用 World evidence capture、policy、presence build 和 consume。四段式手工编排允许调用方在中途退出、错配 item，或把已构造 presence 遗留为未消费状态。

规划只确认环绕飞剑可以提供近身威慑，没有冻结每秒频率、持续时间、伤害或控制。因此 P6.16 只收口“一次由外部明确完成的 sample”，不把 threat 绑定到 Tick，也不提前发明 cadence/effect。

## 架构判断

### 显式完成，而非自动时间驱动

Run Host 接收已经关闭的 `FShanmenDetectorEmissionReceipt`。开始、投影、结束 sample 的时机仍由调用方拥有；Host 不创建 timer、不读取 DeltaSeconds，也不重复投影 World。

### 候选 authority 上执行

收尾先完成既有的 exact-item evidence/policy/presence 链路，再复制 Run authority 并在副本上消费。只有完整 result 自校验和 candidate authority 验证都成功，才将副本一次性提交到 Host。

### 完整结果作为审计边界

新 `Fdemo_mapShanmenControlledWeaponThreatFinalizationResult` 同时持有 item identity、无 Actor 引用的 captured evidence、presence receipt 和 consumption result。`IsFinalized()` 交叉验证各层 identity、target/tag 对应关系、intent/receipt 对应关系与 NoOp 结构。

## 实现链路

1. 新增 self-validating finalization result；
2. 新增 `TryFinalizeOrbitThreatSample` 原子产品入口；
3. 抽取 `PresenceMatchesController`，让手工 consume 与原子收尾共享 exact-action fence；
4. evidence/policy/presence 任一失败时立即返回，输出保持 invalid；
5. consume 在 authority 副本上运行，拒绝或结果校验失败不触碰 live ledger；
6. 成功后整体提交 authority 与 finalization result；
7. 保留全部低层入口，未改变现有调用方行为。

## 测试增量

在既有 `OrbitThreatRouting` 产品测试中加入：

- 首次 `Consumed`：一个 target、一个 intent、一个 receipt、revision `1`；
- 精确重放 `AlreadyConsumed`：返回首次 revision，ledger 数量保持 `1`；
- unknown item 不能借用另一 item 的 emission；
- emission 有 candidate 但 Actor 数组为空时失败，live authority 不变；
- 第二个显式空 sample 得到 `NoOp`，revision 保持 `1`；
- 空 sample 仍占用一个 detector ordinal，后续 directed contact ordinal 连续；
- presence finalization 不修改 target vitality 或 accepted impact 数；
- 最后 item 退休后 authority ledger 清零。

## 自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 测试完成跨度 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p616_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 30 | 0 | 0 | `0.533s` | `0FF67AD877E7CDBBDADEABECACB560A21141D851AF5427F777DA3F0FB62F6092` |
| `p616_full.log` | `Shanmen.0_0_10` | 163 | 0 | 0 | `8.387s` | `2CA5399C4956EA8C17AAFDBDD53F88131352C1F1FEC90EB1DD3145FDC74CDC90` |

两份日志位于 `Saved/Automation/P616/`；每份只有一个实际 RunTests 命令、Fail `0`、Fatal/unhandled/ensure `0`、queue-empty 和 native exit `0`。

## Changed-file gate

Source 三个改动文件全部命中 `ControlledWeaponRunHost` 映射规则：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=11 Logs=2
```

完整 `Shanmen.0_0_10` 覆盖 CombatRuntime、Items、CombatRunCoordinator、ControlledWeapon Adapter / Controller / RunCommandRouter / RunHost / RunLifecycle / Session / WorldDelivery 与 WorldGameplay 共 11 个必跑组。Regression coverage self-test：`14/14 PASS`。

## 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- Editor：`28/28` actions，`Result: Succeeded`，native exit `0`，`95.28s`；
- Game：`27/27` actions，`Result: Succeeded`，native exit `0`，`92.97s`；
- demo_map Editor DLL UTC：`2026-08-29T04:45:42Z`；
- Game executable UTC：`2026-08-29T04:49:39Z`；
- 没有首次失败、源码失败、环境错误、内存错误或外层超时；Game executable 未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- Source 新增 `264` 行、删除 `48` 行；
- 新增生产代码的 `Tick(`、`DeltaSeconds`、`SetTimer`、`ApplyDamage`、impact/vitality commit、spawn 与 RNG 扫描命中 `0`；
- finalization result 不保存 Actor 引用；Actor 数组只用于瞬时 World evidence capture；
- Registry、GameplayTags、Profile schema、存档、item authority、input、资产和 Build.cs 均未修改；
- 长期未跟踪历史文件未纳入本阶段 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

下一阶段应由产品语义先冻结 sample cadence owner、刷新/过期规则和可观察效果，再决定是否增加 per-target cooldown 或持续 presence。不能把每帧 overlap 默认解释成伤害、硬控或永久状态。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-16-orbit-threat-finalization/Docs/Report/Dev.D.UE.0.0.10.P6.16.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-16-orbit-threat-finalization/Docs/Log/Dev.D.UE.0.0.10.P6.16.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-16-orbit-threat-finalization>
