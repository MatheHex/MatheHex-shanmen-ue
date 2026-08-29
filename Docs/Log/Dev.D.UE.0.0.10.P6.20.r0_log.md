# Dev.D.UE.0.0.10.P6.20.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.20.r0`；
- 基线提交：`ff41a3a599cf83e21e85d6f87ee605065de80b80`（P6.19）；
- 分支：`agent/0.0.10-p6-20-atomic-threat-sampling`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 问题与目标

P6.19 使 stale/in-flight/post-state threat receipt 失败关闭，但 Host 使用者仍需手工编排：打开窗口、逐 contact projection、关闭 emission、world evidence capture、policy、presence 与 authority consumption。任何中间失败都要求调用方自行清理，无法从接口结构上保证 sample ordinal、活动窗口与 authority checkpoint 同步提交。

P6.20 的目标是增加一个 Host-owned 显式事务：调用方仍决定采样频率与 overlap 集合，Host 只原子组织现有链路。阶段不定义 Tick/timer、威慑数值、伤害、控制、持续时间或冷却。

## 设计决策

### 输入只保留一份目标事实

`Fdemo_mapShanmenControlledWeaponOrbitThreatContact` 只包含 `FOverlapResult`、ContactLocation 与 ContactNormal。目标 Actor 直接从 overlap 取得；同一 overlap 先生成 canonical entity candidate，再作为 transient world evidence 输入，避免调用方分别传 geometry target 和 evidence Actor。

### 整个 Host 是候选事务边界

`TrySampleOrbitThreat()` 先验证 Host、Coordinator 与 exact item，然后复制整个 Run Host。Begin、所有 Project、End 和现有 `TryFinalizeOrbitThreatSample()` 都只在候选上运行。最终 result 与候选 Host 同时通过自检后才 move 回 authority Host。

这使 Controller emission ordinal、active context、latest-completed receipt、所有 item Controller 与 Run-scoped presence authority 共享一个提交点。失败时不需要补偿式 End，也不会把失败 ordinal 暴露给下一次调用。

### 复用既有权威层

事务没有新增 geometry、target tags、policy、presence 或 idempotency 算法：

1. Product Controller/World Adapter 负责 overlap -> canonical candidate；
2. Session/Execution 负责 detector emission 与 latest-completed freshness；
3. World Adapter 负责 Actor -> exact target evidence；
4. Execution 负责 policy/presence；
5. Run Host authority 负责 activation/sample checkpoint、retention 与 idempotent consumption。

## 实现链路

1. 新增 caller-owned contact value type；
2. `TrySampleOrbitThreat()` 清空输出并预检 Host/Coordinator/item；
3. 创建完整 Host candidate 并开始 exact-item Orbit threat window；
4. 按调用方顺序投影每个 contact，从同一 overlap 收集 Actor；
5. 任一 projection 或 Actor 失败立即丢弃 candidate；
6. 关闭 candidate emission；
7. 复用 `TryFinalizeOrbitThreatSample()` 完成 evidence、policy、presence 与 consumption；
8. final result 与 candidate Host 均有效后一次提交；
9. contacts 为空时仍执行 Begin/End/Finalize，产生可审计 `NoOp` sample。

## 测试增量

新增 `AtomicThreatSamplingTransaction`：

- 一次调用完成 single-target sample；
- duplicate target 的第二 contact 拒绝并整批回滚；
- 回滚不泄漏 active window，不改变 checkpoint/revision/intents；
- 空 sample 复用回滚 ordinal 并提交 `NoOp`；
- unresolved overlap 与 unknown item 保持失败关闭；
- 合法重试精确推进下一 ordinal；
- 第二 item 首次 sample 仍为 ordinal `0`，证明跨 item 隔离；
- vitality 与 impact ledger 始终不变。

## 自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p620_runhost.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 8 | 0 | 0 | `0.135s` | `4CC9FCB652EE70E9F87536C0E18F42C4A84A20F46C162B95335308C5BDE2B6D8` |
| `p620_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 32 | 0 | 0 | `0.551s` | `F5749E91151C8313C889DD86D50DF592DA372F537BF4F404E33B37164F6ADBDD` |
| `p620_full.log` | `Shanmen.0_0_10` | 165 | 0 | 0 | `8.485s` | `EB1BB8DE707907DEEECE5AEF7856BFD971A8CAA225DDA06FFA88C24BE3E025AD` |

日志位于 `Saved/Automation/P620/`。最终日志各有一个 RunTests 命令、一个 queue-empty marker、Fail `0`、Fatal/unhandled/assert/ensure `0`，原生退出码均为 `0`。

## 失败审计

双 item 测试增强的首轮补丁落错旧夹具，导致新增测试和 `OrbitThreatRouting` 各失败一项；产品源码没有失败。进程因 `TestExit` 仍返回原生退出码 `0`，但日志中的 Fail `2` 使 regression gate 正确失败。

- `p620_runhost-backup-2026.08.29-06.54.47.log`：6/8，SHA `1B5A7BCE8C85592DF139FCA6A12B3BCC003620BA0E034D4CF369BC5178F52C7E`；
- `p620_product-backup-2026.08.29-06.55.08.log`：30/32，SHA `454B65B957E68672546EE3E0F38E7D0F2723072E747D11D6E02D575250353059`；
- `p620_full-backup-2026.08.29-06.55.39.log`：163/165，SHA `17587367F814B2E835658373D63EB5B8C5ACCA3500333C7070C03AB9E5E3FF7C`。

按测试名修正 attachment 所属夹具后，先重跑 RunHost 8/8，再重建产品与完整健康日志。

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

- 初始 Editor：30/30，Succeeded，native exit `0`，`87.07s`；
- 双 item 测试增量 Editor：4/4，Succeeded，native exit `0`，`5.61s`；
- 最终测试夹具修正 Editor：4/4，Succeeded，native exit `0`，`5.40s`；
- 最终 Game：27/27，Succeeded，native exit `0`，`94.32s`。

最终 `UnrealEditor-demo_map.dll` UTC 为 `2026-08-29T06:56:38Z`，`demo_map.exe` UTC 为 `2026-08-29T07:00:01Z`。所有构建首次执行成功，Game executable 未启动。

## 静态、兼容性与范围

- `git diff --check`：native exit `0`；
- Source `+250/-0`；
- 新增生产行的 Tick/Timer、damage/effect、spawn、World/RNG、Cooldown/Duration 扫描命中 `0`；
- 既有分步 threat API 与 finalization result 保持兼容；
- 未增加 cadence、effect、vitality mutation、impact commit、World query 或资源事务；
- 未修改 schema、存档、tags、item authority、input、资产或 Build.cs；
- 长期未跟踪 0.0.9B 文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

Host 现在可安全接收一次显式 overlap sample。下一阶段可在独立 cadence owner 中调用该事务，并继续把采样频率、威慑 gameplay effect、数值与持续时间作为未冻结产品决策；不应把 Tick/timer 或 damage 塞回本事务。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-20-atomic-threat-sampling/Docs/Report/Dev.D.UE.0.0.10.P6.20.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-20-atomic-threat-sampling/Docs/Log/Dev.D.UE.0.0.10.P6.20.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-20-atomic-threat-sampling>
