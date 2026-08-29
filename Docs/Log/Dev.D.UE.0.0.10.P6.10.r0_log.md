# Dev.D.UE.0.0.10.P6.10.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.10.r0`；
- 基线提交：`1133286bcfa6796999c4d1b81a8524958228d09c`（P6.9）；
- 分支：`agent/0.0.10-p6-10-controlled-weapon-orbit-threat-candidates`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.9 已让 Orbit 世界位姿由唯一 GameMode frame owner 持续推进，但近身关系仍不可观察。直接复用 Directed contact delivery 会把 overlap 同时变成伤害，并迫使本阶段决定碰撞半径、频率、target policy、冷却与数值。

P6.10 因此只建立 candidate projection：调用方必须显式提供一次 overlap 证据，系统只把它转成统一候选并去重。世界查询策略和候选如何转化为威慑、防御或伤害继续留给后续显式契约。

## 契约设计

### 唯一身份序列

Orbit 与 Directed 共用 `FShanmenControlledWeaponExecution` 内既有 `FShanmenDetectorEmissionSession`。一个 emission 中所有目标共享 ordinal，TargetEntityId 保证不同目标身份唯一；重复 target 只接受一次。Orbit 窗口 End 后递增同一个 `NextEmissionOrdinal`，后续 Directed 窗口自然使用下一个值。

没有新增 OrbitDetectorId 或第二个计数器，避免同 Action + Detector 下的 ImpactId / candidate identity 冲突。

### 零伤害边界

`TryAcceptOrbitThreatCandidate` 只调用 emission duplicate gate。它不调用 `IsTargetAllowed`、`TryBuildDamagePacket`、`FShanmenImpactLedger` 或 `FShanmenDefenseResolver`。

`TryResolveCandidate` 保持 `State == Directed` 的原门槛。Orbit window 中即使取得合法候选，也不能进入 damage path；测试同时核对 vitality 与 accepted impact count 均不变。

### 状态栅栏

Orbit threat emission 活动时：

- Launch 拒绝，防止 candidate-only emission 中途转为 Directed；
- Orbit movement 拒绝，保证显式 overlap sample 对应稳定姿态；
- Directed sweep / overlap delivery 拒绝；
- Interrupt 仍可通过既有 termination 路径原子关闭 emission；
- 正常 End 后可 Launch，Directed contact 获得下一个共享 ordinal。

### 世界与产品路由

`ProjectOrbitThreatOverlap` 接受调用方提供的 `FOverlapResult`、contact location 与 normal，并复用 `FShanmenWorldHitAdapter`、`FShanmenWorldEntityRegistry` 和 `FShanmenHitCandidate`。

World adapter 返回 Context + Candidate 自校验回执，不读取 vitality。Product Controller 拥有一个窗口，Run Host 按 exact ItemInstanceId 路由；未知 item 无法借用现有窗口。

## 自动化覆盖

### Runtime

- Orbiting 状态可打开 candidate-only emission；
- 同 target 重复候选拒绝；
- Orbit candidate 不能 resolve damage；
- ImpactLedger 数量保持 0；
- open emission 阻止 Launch；
- End 后 Launch 成功；
- Directed context ordinal 紧随 Orbit ordinal；
- Directed candidate 仍可正常 resolve。

### Product Controller

- 显式 overlap 映射到 exact canonical enemy entity；
- result 保留 exact source item；
- projection 前后 vitality 相等，ImpactLedger 为 0；
- duplicate、open-window movement、Launch 与 Directed adapter 均拒绝；
- End + Launch 后 Directed overlap 只提交一次真实伤害。

### Run Host

- exact item Begin / Project / End；
- 未绑定 item 拒绝；
- projection 零 vitality mutation；
- open-window Launch 栅栏；
- 后续 Directed window 共享 ordinal。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p610_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 5 | 0 | 0 | `D4A621C60BCEF7C0C8D56E1765ECD83AF6F6FE8CF61352BCE99A3D38B90CA937` |
| `p610_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 29 | 0 | 0 | `01B92AF1581A41E8E03AFC4543E873AC3C893A86A5577BA482FF05DF7839498D` |
| `p610_full.log` | `Shanmen.0_0_10` | 159 | 0 | 0 | `27489DE0C5E2A2939B70365172C9F38EEAE23988D9C0BD668FC0807B7FAAD95F` |

日志均位于 `Saved/Automation/P610/`，每份各有一个实际 RunTests 命令、queue-empty、Fail 0、Fatal / unhandled / ensure 0 与原生退出 0。

## Changed-file gate

生产与测试源码完成后：

```text
REGRESSION_COVERAGE: PASS Changed=13 Rules=5 Required=11 Logs=3
```

完整 `Shanmen.0_0_10` 覆盖 11 个由改动路径映射出的必跑组；Runtime `5/5` 与 Product ControlledWeapon `29/29` 提供直接 focused 证据。

加入 Report / Log 后最终 staged gate：

```text
REGRESSION_COVERAGE: PASS Changed=15 Rules=5 Required=11 Logs=3
```

regression coverage self-test：`14/14 PASS`。

## 构建

Editor：

- `42/42` actions；
- `Result: Succeeded`；
- native exit `0`；
- `160.14s`。

Game：

- `39/39` actions；
- `Result: Succeeded`；
- native exit `0`；
- `137.05s`；
- 只生成 executable，未启动。

没有首次失败、内存错误、外层超时或源码重试。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- 新增生产行共 365 行；世界查询、Spawn、Damage/Impact 结算、库存事务、RNG 与输入调用扫描命中 `0`；
- 最新源码 UTC `2026-08-29T01:31:49.3490734Z`；
- Editor DLL UTC `2026-08-29T01:35:00.3402190Z`；
- Game executable UTC `2026-08-29T01:39:56.8768481Z`；
- source 早于两个最终构建产物；
- 未修改 Profile schema、存档、item definition、GameplayTags、input mapping 或资产；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

下一阶段如继续消费近身候选，应由显式 content / product policy 决定采样来源、半径、频率、目标合法性、每目标冷却，以及候选最终产生威慑、防御还是伤害。本阶段不预设这些产品规则。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-10-controlled-weapon-orbit-threat-candidates/Docs/Report/Dev.D.UE.0.0.10.P6.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-10-controlled-weapon-orbit-threat-candidates/Docs/Log/Dev.D.UE.0.0.10.P6.10.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-10-controlled-weapon-orbit-threat-candidates>
