# Dev.D.UE.0.0.10.P6.23.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.23.r0`；
- 基线提交：`aedf6d8e228a642522a2b11ae9bb573eaa8f100e`（P6.22）；
- 分支：`agent/0.0.10-p6-23-orbit-defense-readiness`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 问题与目标

人工战斗规划把“飞剑环绕”定义为攻击准备、近身威胁、防御准备与后续发射的共同姿态。P6.8–P6.22 已实现轨道位姿、近身威胁证据与显式 sample Router，但不存在一种证据能回答：某个 exact item 在一个 Impact 前是否仍处于真实、可复核的环绕准备状态。

直接创建 `FShanmenDefenseLayer` 会提前冻结覆盖角度、触发窗口、减伤数值、参与 item 与耐久成本。P6.23 因此只建立零效果 readiness snapshot，为未来产品防御策略提供输入，不碰既有 DefenseResolver 真值。

## 设计决策

### Runtime receipt 只证明状态

`FShanmenControlledWeaponDefenseReadinessReceipt` 冻结完整 Action identity、Content stamp、exact item、`Orbiting` 与当前 command sequence checkpoint。ID 使用独立 `Shanmen.ControlledWeapon.DefenseReadiness.r1` namespace 确定性派生。

捕获要求 Action Active 且 Execution Orbiting。current 校验重新比较 Action 与命令检查点，因此 Launch、Recall、terminal 或不同 Run/activation 均不能复用旧证据。Orbit threat sample 只推进 detector ordinal，不改变命令/姿态，所以不会误让 readiness 失效。

### Product receipt 证明物理准备姿态

Controller receipt 冻结：Source 当前位置加 authored center offset、轨道法线、参考轴、半径、当前相位与武器 Actor 实际位置。`IsValid()` 重算轨道方程；逻辑状态正确但 Actor 尚未放置、Source 已移动而 Actor 尚未跟随时均拒绝。

SnapshotId 使用所有几何分量的位级表示，并把正负零规范为同一 canonical 值。轨道移动会产生新 SnapshotId；没有控制命令时，底层 Runtime readiness identity 保持不变。

### Host 不拥有参与策略

Host 接受调用方明确 item 列表，验证合法/唯一后按 GUID 排序。任何未知、重复、非 Orbiting 或物理未就绪 item 都使整个只读捕获失败并清空输出。额外绑定 item 不会自动加入，也不会使已明确的合法子集失效。

current 校验逐项穿透到 Controller/Session/Execution，重新验证 Host Run/Source、Action、命令状态、Source anchor 与物理 pose。

## 自动化增量

Runtime 新增 `OrbitDefenseReadiness`：

- 默认 receipt 无效；
- exact Active/Orbiting capture；
- 等价执行重放同一 readiness ID；
- candidate-only threat window 前后仍 current；
- Launch 后旧 checkpoint 失效；
- Directed 无法重新捕获；
- terminal Action 使原 Orbiting evidence 失效。

RunHost 新增 `OrbitDefenseReadiness`：

- 首次物理放置前拒绝；
- reverse input 稳定输出 low/high GUID；
- duplicate、unknown、empty subset 拒绝且无部分输出；
- orbit movement 使旧 pose stale、fresh pose 保留 Runtime ID；
- 一把 Launch 后 mixed batch 拒绝，高 item 显式子集仍成立；
- Source anchor 移动使证据 stale，下一 orbit step 后恢复。

## 最终自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p623_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 9 | 0 | 0 | `0.141s` | `0EF112314FFBD7D24110056C31A4C62966338C8A52BA9906472B7058AB590968` |
| `p623_host.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 10 | 0 | 0 | `0.176s` | `D9AB96D5418F47723CEB35545DE5CD0164471CC01C200F9298FF3CFA04B5282C` |
| `p623_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 37 | 0 | 0 | `0.641s` | `A98F15645DDF7E29DC71114097CBFC926077304361CDA3139D3440BA88347DC6` |
| `p623_full.log` | `Shanmen.0_0_10` | 171 | 0 | 0 | `8.414s` | `5D9A0E11946EFC68A43D5000319F83CEFC7509FBC83A0E6493E382BD49540E83` |

四份最终日志各有一个实际 RunTests 命令、queue-empty、Fail `0`、Fatal/assert/ensure `0`，进程原生退出码均为 `0`。日志位于 `Saved/Automation/P623/`，不进入 Git 工件。

## Changed-file gate

十个 Source 文件命中 CombatRuntime、Product Session、Controller 与 RunHost 四条规则，要求十二个测试组：

```text
REGRESSION_COVERAGE: PASS Changed=10 Rules=4 Required=12 Logs=4
SELF_TEST: PASS 16/16
```

`p623_full.log` 覆盖全部十二组；Runtime、RunHost 与 ControlledWeapon parent 日志提供额外聚焦证据。

## 构建时间线

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

1. Editor integration：`45/45`，Succeeded，native exit `0`，`133.26s`；
2. 把 Content stamp 纳入 readiness ID 后首次增量 Editor：native exit `1`，`OtherCompilationError`，`C2664`；`Content.Version` 是 `FName`，不能直接进入 `TArray<FString>`；
3. 改为 `Content.Version.ToString()` 后最终 Editor：`5/5`，Succeeded，native exit `0`，`5.39s`；
4. 最终 Game：`42/42`，Succeeded，native exit `0`，`134.91s`。

首次失败属于新增源码类型错误，未描述为环境或内存故障。修正后所有构建与自动化通过。Game executable 未启动。

## 静态、范围与兼容性

- `git diff --check`：native exit `0`；
- Source：生产 `+488/-0`，测试 `+200/-0`，合计 `+688/-0`；
- 新增行的 DefenseLayer/DefenseSnapshot/Resolver、damage/vitality commit、item transaction、Tick/World query、Spawn/RNG 扫描命中 `0`；
- 没有新增 cadence、query shape、defense angle/window/value、resource cost、Gameplay Effect 或 inventory mutation；
- 没有修改 Build.cs、tags、schema、存档、item definitions、input mappings 或资产；
- 长期未跟踪 0.0.9B 文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-23-orbit-defense-readiness/Docs/Report/Dev.D.UE.0.0.10.P6.23.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-23-orbit-defense-readiness/Docs/Log/Dev.D.UE.0.0.10.P6.23.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-23-orbit-defense-readiness>
