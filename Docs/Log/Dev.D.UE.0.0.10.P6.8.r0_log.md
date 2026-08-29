# Dev.D.UE.0.0.10.P6.8.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.8.r0`；
- 基线提交：`5934d0de739763105e5365c78b2cc1cbe1242adc`（P6.7）；
- 分支：`agent/0.0.10-p6-8-controlled-weapon-orbit-motion`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.4–P6.7 已建立单剑真实 Actor 控制、多实例 Run Host、生命周期 owner 与输入无关 Command Router，但 `Orbiting` 仍只有逻辑状态；真实 Actor 仅在 `Directed` 后才移动。此时直接进入键位或 presentation 会让“飞剑环绕”缺少确定的世界位姿契约，并迫使输入层承担 cadence 或轨迹数学。

0.0.10 规划将飞剑环绕同时定义为攻击准备、近身威胁、防御准备和发射准备。P6.8 因此只补共同的无伤害准备位姿：显式捕获运动参数、按状态推进、输出审计收据，并由现有 Host / GameMode owner 暴露；近身攻击、防御、编队和输入仍留给后续独立契约。

## 实现记录

### Explicit orbit capture

每个 Product attach 都必须提供中心偏移、世界平面法线、参考轴、半径、带方向的角速度、初相位与最大步长。平面基必须有限、单位且正交；半径必须为正；角速度必须有限且非零；初相位在 Controller 启动时规范到 `[0, 2π)`。

既有 Directed speed 继续由同一 capture 冻结，避免 Orbit 与 Launch 使用两套运动配置 authority。所有既有 Lifecycle / Router 测试夹具也显式填写新字段，没有引入隐式默认值。

### Deterministic non-damaging pose

Controller 的中心取当前 Source Actor 世界位置加冻结偏移；切线轴由 `Cross(PlaneNormal, ReferenceAxis)` 唯一派生。下一相位只由当前规范相位、冻结角速度和本次显式 delta 推导，位姿为平面基上的 sin/cos 参数圆。

入口只允许 active `Orbiting`。它先在 Controller 副本中验证下一相位，再以 non-sweep `SetActorLocation(..., TeleportPhysics)` 放置真实 Actor；收据重算相位与位置公式并核对 actual 等于 requested，最后才提交 Controller 相位。Launch / Redirect / Recall、Directed sweep 与 Contact path 均未改变。

### Stable multi-item host

Host 从既有稳定 GUID 顺序中过滤 Orbiting items。正式移动前先核对所有目标的 `MaximumStepSeconds`；任何超限使整批在变换或相位发生前失败。通过后按稳定顺序 best-effort 放置，并输出 exact item 对应的有序收据。

一把 item 经 canonical Launch 离开 Orbiting 后，后续 batch 不再包含它；同一 Run 的其余 Orbiting items 继续独立推进。

### Owner boundary

GameMode 新增 `AdvanceControlledWeaponOrbit` 薄入口，直接委托其唯一 `ControlledWeaponRunHost`。本轮没有自动调用该入口，因此没有冻结 Tick、timer、substep 或 input cadence。

### Regression routing

ProductController 路径新增 RunHost、RunLifecycle 与 RunCommandRouter 下游映射；RunHost 路径新增 Lifecycle 与 Router 映射。这样以后修改底层 capture / movement 或 Host 时，changed-file gate 不会只跑旧的上游主题组。

## 自动化覆盖

- 单剑 0 → π/4 → π/2 的相位与世界位姿；
- 当前 Source Actor 位置作为动态锚点；
- 超出最大步长时 Actor transform 与 Controller phase 均不变；
- 非正交平面基在启动边界失败关闭；
- Launch 后 Orbit 入口拒绝且 Actor 不动；
- 两把逆序 attach item 按稳定 GUID 顺序推进；
- 不同初相位保持确定间隔；
- host oversized delta 在任何 item 移动前拒绝；
- 单把 Launch 后只推进剩余 Orbiting item；
- P6.6 Lifecycle 与 P6.7 Router 夹具兼容；
- 完整 0.0.10 回归。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p68_controller_first.log` | `Shanmen.0_0_10.Product.ControlledWeaponController` | 4 | 0 | 0 | `E37BF0B063EE87E81638C0E2D1DA2A91A20D57EDCF1AC7C18B8656040459117A` |
| `p68_run_host_first.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 4 | 0 | 0 | `2E11F3333ACA7CDA9A435DF2ECF552C5EDCB1C3F32C0EC05A735292ECC897317` |
| `p68_lifecycle_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunLifecycle` | 3 | 0 | 0 | `44F953AD8B0D176F4E8D3D59C94BAF3C04B61AC0BD4E5023D41C9DCC0C8ABE50` |
| `p68_router_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunCommandRouter` | 4 | 0 | 0 | `A35CD32FDFEE779FE8716E76000E8932D5999CD78D76BEC749C2D641BEDF67E0` |
| `p68_full_final.log` | `Shanmen.0_0_10` | 155 | 0 | 0 | `624CF984D0F242D899DEEDD365C62C7012C553FDF41E70E21C362F009C291E16` |

五份日志均有一个实际 `Cmd: Automation RunTests`、一个 queue-empty、至少一个 Success、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`。

Controller `4/4`、RunHost `4/4`、完整 `155/155`、Lifecycle `3/3` 与 Router `4/4` 均为首轮成功。没有源码、自动化或编译失败。

## Changed-file gate

加入 Report / Log 前的 11 个生产、测试与脚本路径：

```text
REGRESSION_COVERAGE: PASS Changed=11 Rules=5 Required=12 Logs=5
```

Required groups 为完整 `Shanmen.0_0_10`、CombatRuntime、Items、Coordinator、Adapter、Controller、RunHost、Lifecycle、Router、Session、WorldDelivery 与 WorldGameplay。完整套件覆盖全部 12 组；Controller、RunHost、Lifecycle 与 Router 另保留直接 focused 证据。

加入本 Report / Log 后的最终 staged gate：

```text
REGRESSION_COVERAGE: PASS Changed=13 Rules=5 Required=12 Logs=5
```

这次按改动文件推导验证范围，不再机械重复九份已被完整套件等价覆盖的子组日志。

## 构建

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `30/30` actions；
- `Result: Succeeded`；
- native exit `0`；
- `131.04s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `29/29` actions；
- `Result: Succeeded`；
- native exit `0`；
- `120.88s`；
- 仅生成 executable，未启动。

## 静态与兼容性

- regression JSON parse：PASS；
- regression coverage self-test：`14/14 PASS`；
- 新增生产差异中的 spawn / damage / inventory transaction / RNG / input binding：`0`；
- Orbit 函数中的 sweep / hit / resolve / delivery / damage：`0`；
- `git diff --check` 与最终 `git diff --cached --check`：native exit `0`；
- 最新源文件时间 `2026-08-29T00:28:20Z`，早于 Editor DLL `2026-08-29T00:31:15Z` 与 Game executable `2026-08-29T00:36:40Z`；
- Profile schema、存档、item definition、GameplayTags、input mapping 与资产未改；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

P6.9 可在不改变本轮世界位姿公式的前提下，选择一个独立方向继续：冻结 presentation cadence / substep owner；增加仅生成候选、不直接结算的近身威胁 detector；或由正式策划冻结输入与 exact-item 选择策略。三者不应在同一阶段混合。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-8-controlled-weapon-orbit-motion/Docs/Report/Dev.D.UE.0.0.10.P6.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-8-controlled-weapon-orbit-motion/Docs/Log/Dev.D.UE.0.0.10.P6.8.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-8-controlled-weapon-orbit-motion>
