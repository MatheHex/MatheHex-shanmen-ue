# Dev.D.UE.0.0.10.P6.9.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.9.r0`；
- 基线提交：`9d6f7104e9077927a6df4b4526f59e6affca6f88`（P6.8）；
- 分支：`agent/0.0.10-p6-9-controlled-weapon-orbit-frame-owner`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.8 已实现显式、确定性且无伤害的 Orbit 世界位姿，但只暴露手工调用；真实 `GameMode::Tick` 没有消费它。若在此基础上先实现近身威胁或输入，产品仍可能只有逻辑 Orbiting、没有持续物理位姿。

GameMode 本来已经是 M01 extraction 的帧 owner，且 `PrimaryActorTick` 已启用。P6.9 因此不增加第二个 ticker，而是在同一个 Tick 中把引擎 delta 交给唯一 Run Host，并建立可审计的成功/no-op/拒绝结果。它不决定固定帧频或补帧策略。

## 实现记录

### Frame result contract

`Fdemo_mapShanmenControlledWeaponOrbitFrameResult` 捕获 status、exact RunId、DeltaSeconds、BoundCount、OrbitingCount 与 P6.8 有序 batch。

状态契约：

- `NoOrbitingItems`：空 Host，或有效 Run Host 中没有 active Orbiting item；
- `Advanced`：所有 Orbiting exact items 按稳定顺序成功推进；
- `DeltaInvalid`：owner delta 非有限或不为正；
- `HostInvalid`：Host 非空但其 Run / Actor / component / controller 契约损坏；
- `MovementRejected`：存在 Orbiting item，但 P6.8 预检或 Actor placement 拒绝。

`IsValid` 按状态核对 Run、绑定数、Orbiting 数、batch 数与收据。空 Host no-op 的 RunId 无效且 BoundCount 为 0；有效 Run no-op 保留 exact RunId 与正绑定数；成功/移动拒绝必须满足 `BoundCount >= OrbitingCount > 0`。

### Host frame pump

`AdvanceOrbitingFrame` 先捕获原始 Host identity/count，再依次判断 delta、empty、Host validity 和 Orbiting count。只有存在可推进 item 时才调用 P6.8 `TryAdvanceOrbitingInOrder`。

不新增 accumulator、fixed step 或 substep。引擎 delta 超过任一 item 的显式 `MaximumStepSeconds` 时直接得到带 Run evidence 的 `MovementRejected`，并保持 P6.8 全批预检的不变性。

### GameMode ownership

`GameMode::Tick` 在 `Super::Tick` 后、原 M01 extraction early-return 前调用 `AdvanceControlledWeaponOrbitFrame`。这样 extraction inactive 的地图也可推进已附着 Orbiting items，同时原 extraction 分支内部没有修改。

GameMode 只在损坏或拒绝时写诊断。Host 损坏/结构失真为 Error；非法 delta/移动拒绝为 Warning；消息包含 RunId、status、delta、bound、orbiting、attempted 和 advanced。成功与 no-op 不逐帧写日志。

## 自动化覆盖

- 非法 delta 与空 Host no-op；
- empty no-op 的无效 RunId / BoundCount 0；
- 两把逆序 attach item 的 stable batch 与 exact RunId；
- 成功 frame 的 bound/orbiting/batch 守恒；
- 超步长 frame 的 MovementRejected 与零 transform mutation；
- Launch 后 NumOrbiting 降为 0；
- 有效 directed-only Host no-op 保留 RunId / BoundCount；
- 非空 Host Actor root 契约损坏返回 HostInvalid；
- 完整 0.0.10 既有回归。

## 首次验证与复审

首次 Editor integration build：`28/28`、`Result: Succeeded`、native exit `0`、`126.30s`。首次 Game build：`27/27`、native exit `0`、`105.47s`。

首次 `p69_run_host_final.log` 为 `5/5`、SHA-256 `4BF611CA7EF865605DFF8F02D77D6000EE166D149168164453AB9E260712845E`；首次 `p69_full_final.log` 为 `156/156`、SHA-256 `3AD291EC62053D2FFF6889A8F24CC3FC99B988E774C354593441D3A8C793BE73`。

复审补入 exact RunId 与 BoundCount，收紧各状态自校验并扩充断言。最终 Editor / Game、focused / full 均再次成功；没有源码、测试或编译失败。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p69_run_host_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 5 | 0 | 0 | `8C457CAABADC3576A70802939E27B35BC2034FEEB62609F7ACDEAA0FFF1BC815` |
| `p69_full_final.log` | `Shanmen.0_0_10` | 156 | 0 | 0 | `645424E8F05812CA30823D60A3436D0D8AB11A2E2EBD1B9F5F0E69513CC61087` |

两份最终日志均有一个实际 `Cmd: Automation RunTests`、一个 queue-empty、Success、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`。

## Changed-file gate

加入 Report / Log 前：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=12 Logs=2
```

完整 `Shanmen.0_0_10` 覆盖 GameMode / Host 路径映射的 12 个必跑组；RunHost `5/5` 是直接 focused 证据。没有重复生成其余已被完整套件覆盖的子组日志。

加入 Report / Log 后最终 staged gate：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=12 Logs=2
```

## 构建

Editor：

- 首次 `28/28`，`Result: Succeeded`，native exit `0`，`126.30s`；
- 最终 `28/28`，`Result: Succeeded`，native exit `0`，`95.54s`。

Game：

- 首次 `27/27`，`Result: Succeeded`，native exit `0`，`105.47s`；
- 最终 `27/27`，`Result: Succeeded`，native exit `0`，`94.42s`；
- 仅生成 executable，未启动。

## 静态与兼容性

- regression coverage self-test：`14/14 PASS`；
- 新增生产差异中的 spawn / damage / inventory transaction / RNG / input binding / sweep / hit / resolve / delivery：`0`；
- `git diff --check` 与最终 `git diff --cached --check`：native exit `0`；
- 最新源文件时间 `2026-08-29T01:02:25Z`，早于最终 Editor DLL `2026-08-29T01:04:09Z` 与 Game executable `2026-08-29T01:06:55Z`；
- Profile schema、存档、item definition、GameplayTags、input mapping 与资产未改；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

P6.10 可在自动 Orbit 位姿已经真实运行的前提下，增加只生成候选、不直接结算的近身威胁 detector；伤害、触发间隔、每目标 hit policy 与防御效果仍应留在独立契约。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-9-controlled-weapon-orbit-frame-owner/Docs/Report/Dev.D.UE.0.0.10.P6.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-9-controlled-weapon-orbit-frame-owner/Docs/Log/Dev.D.UE.0.0.10.P6.9.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-9-controlled-weapon-orbit-frame-owner>
