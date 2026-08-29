# Dev.D.UE.0.0.10.P6.22.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.22.r0`；
- 基线提交：`06643f7c2d4ff58f541d57aa89ca5fa4d7cdac37`（P6.21）；
- 分支：`agent/0.0.10-p6-22-threat-sample-router`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 问题与目标

P6.21 把一次调用方提供的多飞剑 contacts 变成稳定顺序、全 Host 原子批次，但调用方仍缺少一个明确的 pulse 协议。如果外部 Tick/timer 重入、重复派发或乱序，Host 会把每次调用都视为新 sample 并推进 ordinal/checkpoint。直接把采样频率和 World overlap query 塞进 Host 又会冻结尚未确定的产品策略。

P6.22 的目标是增加独立 Router：只拥有 pulse identity、sequence、canonical payload、latest replay 与 Host/Router 原子提交；不拥有 cadence 数值或查询行为。

## 设计决策

### Capture 只冻结稳定语义

Intent capture 接受显式 `IntentId`、`RunId`、`SampleSequence` 和 P6.21 requests。每个 overlap 经 Coordinator registry 解析成 `TargetEntityId`，再按稳定字段排序。Intent 临时保留原 contacts 供 Host 投影，但 Router 的持久 latest record 只复制 EntityId/几何指纹和回执，不持有 UObject 引用。

### 最新一条重放，而非无界 ledger

Router sequence 从零连续推进。只缓存 latest accepted sample：

- latest exact replay 返回原 receipt，不重新写 Host；
- latest identity/payload 冲突拒绝；
- 更老 sequence 由 monotonic stale fence 拒绝；
- 更大 sequence 由 gap fence 拒绝；
- latest IntentId 不能标识下一 sequence；
- `MAX_int64` 在递增前拒绝。

因此 replay 状态固定 O(1)，不会随 Run 时长增长。

### 新提交重新验证世界身份

capture 后、实际提交前重新通过 registry 解析每个 contact，并与冻结 TargetEntityId 比较，防止对象解绑/重绑造成 TOCTOU。该检查只针对新提交；latest exact replay 已有 canonical fingerprint 和 receipt，不需要重新依赖 World 对象。

### Host 与 Router 双候选一次提交

Router 先复制 Host，调用 P6.21 batch，再构造自己的候选 latest record。只有 batch、Host candidate、Result 和 Router candidate 全部自检通过，才同时 move 到真实状态。任何失败都不消耗 sequence、IntentId、ordinal、checkpoint 或 authority revision。

### 生命周期由既有 GameMode Run owner 管理

GameMode 暴露一个显式 route seam，但 Tick 不调用它。Run 激活拒绝残留 Router；正常结束、Coordinator 不活跃的孤儿状态和失败回收都 Reset Router，并在 release 日志记录 accepted sample 数。

## 测试增量

新增三个 Router 测试，共覆盖：

- item/contact canonical order；
- low/high 两项稳定 authority revision；
- latest exact replay 零写入；
- latest payload conflict；
- 后项 duplicate evidence 导致整批失败；
- 失败后原 IntentId/sequence 修正重试；
- sequence gap、stale、exhaustion；
- latest IntentId 跨 sequence 冲突；
- cross-Run fence；
- unresolved geometry capture failure；
- Reset 有效空状态。

## 自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p622_router.log` | `Shanmen.0_0_10.Product.ControlledWeaponThreatSampleRouter` | 3 | 0 | 0 | `0.034s` | `B1E51E05763BBB5C6A65B6A2BA433BD0CD9FEA3D55A9C015393A686E8ABEA64B` |
| `p622_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 36 | 0 | 0 | `0.618s` | `1E220C6076F2E1D845000992B3BE5BE992FB46D08E2F459D65C57C7EA9582D03` |
| `p622_full.log` | `Shanmen.0_0_10` | 169 | 0 | 0 | `9.145s` | `B6F8CAA7A3C8B2B5430B6ED0246AC1F7E0E371D628F743FFB0EF72FFBE944E3D` |

日志位于 `Saved/Automation/P622/`。最终日志各有一个 RunTests 命令、一个 queue-empty marker、Fail `0`、Fatal/assert/ensure `0`，原生退出码均为 `0`。

初始实现回归全绿；在代码复核中补充 capture/route identity race 防护与 sequence exhaustion 测试后，重新生成最终三份日志。没有失败轮次。

## Changed-file gate

七个 Source/Script 路径命中 GameMode 与新 Router 两条规则，要求 11 个测试组：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=11 Logs=3
SELF_TEST: PASS 16/16
```

完整日志覆盖 CombatRuntime、Items、CombatRunCoordinator、ControlledWeapon Adapter / Controller / RunHost / Session / ThreatSampleRouter / WorldDelivery 与 WorldGameplay；GameMode 的 broad full-suite 要求也被满足。

## 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 初始 Editor：24/24，Succeeded，native exit `0`，`114.63s`；
- identity hardening Editor：24/24，Succeeded，native exit `0`，`77.05s`；
- 最终 Editor：5/5，Succeeded，native exit `0`，`8.17s`；
- 初始 Game：23/23，Succeeded，native exit `0`，`87.06s`；
- 最终 Game：4/4，Succeeded，native exit `0`，`13.61s`；
- Editor DLL UTC：`2026-08-29T08:13:00.4113431Z`；
- Game executable UTC：`2026-08-29T08:14:31.3638832Z`。

所有构建首次执行成功，后续构建均对应源码增强后的重新验证；Game executable 未启动。

## 静态、兼容性与范围

- `git diff --check`：native exit `0`；
- Source `+1451/-6`，生产 `+802/-6`、测试 `+649/-0`；Scripts `+32/-1`；
- Router 生产代码的 Tick/timer、GetWorld/overlap query、ApplyDamage、SpawnActor、RNG 调用扫描命中 `0`；
- P6.20/P6.21 单项与批次 API、Run command、movement 和 presence authority 保持兼容；
- 未增加 cadence、World query、effect、vitality mutation、impact commit 或 inventory mutation；
- 未修改 schema、存档、tags、item authority、input、资产或 Build.cs；
- 长期未跟踪 0.0.9B 文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

真实 overlap collector 可以在产品策略明确后捕获一次 Intent 并调用 GameMode seam。frequency、shape 与 item participation 仍由外部 cadence owner 决定；伤害/控制效果继续作为独立后续契约。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-22-threat-sample-router/Docs/Report/Dev.D.UE.0.0.10.P6.22.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-22-threat-sample-router/Docs/Log/Dev.D.UE.0.0.10.P6.22.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-22-threat-sample-router>
