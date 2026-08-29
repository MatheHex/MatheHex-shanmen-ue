# Dev.D.UE.0.0.10.P6.10.r0 Report

## 1. 结论

P6.10 已完成受控武器 Orbit 近身威胁的候选投影闭环，结论为 **PASS**。

外层现在可以为一把明确的 Orbiting 飞剑提供一次显式 overlap 样本；系统把该样本转换为统一 `FShanmenHitCandidate`，并返回包含冻结 Action、exact item、Detector 与 HitOrdinal 的自校验回执。该路径只记录几何候选，不捕获目标生命值、不构造伤害包、不调用 DefenseResolver、不写 ImpactLedger，也不提交 Vitality。

Orbit 威胁与 Directed 命中复用同一个 detector emission ordinal authority，因此不会产生两个互相冲突的命中身份序列。Orbit 窗口关闭后，第一个 Directed 窗口严格使用下一个 ordinal。

## 2. 功能性

- `FShanmenControlledWeaponExecution` 新增 Orbit-only candidate emission：Begin、Accept、End；
- Orbit candidate acceptance 只调用既有 `FShanmenDetectorEmissionSession` 的身份与每目标去重门，不进入 Damage/Impact 路径；
- Orbit 与 Directed 共用既有 `EmissionSession`，同一 Action + Detector 只有一条单调 ordinal 流；
- Orbit 窗口活动时禁止 Launch，避免状态在同一 emission 内从 candidate-only 切换到 damage-capable Directed；
- Product Controller 同时区分 `HasActiveOrbitThreatWindow` 与 `HasActiveDirectedContactWindow`；
- Orbit 窗口活动时禁止 Orbit 位姿继续推进，使一个显式 overlap sample 对应稳定的物理姿态；
- Directed sweep / overlap delivery 现在显式要求 Directed contact window，不能误用 Orbit context；
- `ProjectOrbitThreatOverlap` 只消费调用方提供的 `FOverlapResult + location + normal`；不查询 `UWorld`，不选择碰撞通道、半径、频率或触发策略；
- World adapter 继续使用唯一 `FShanmenWorldEntityRegistry` 把 Actor / Component / BodyIndex 映射为稳定 TargetEntityId；
- projection result 同时携带 `FShanmenWorldHitContext` 与 `FShanmenHitCandidate`，并自校验 Activation、Source、Detector、Kind 与 ordinal 一致；
- Run Host 以 exact `ItemInstanceId` 路由 Begin / Project / End，未知 item 不能借用另一把飞剑的窗口。

## 3. 完整性

新增 3 个自动化：

1. `ControlledWeapon.OrbitThreatCandidateOnly`：验证 Orbit candidate 去重、零 Impact、开放窗口期间 Launch 栅栏，以及 Orbit ordinal 0 后 Directed ordinal 1；
2. `ControlledWeaponController.OrbitThreatProjection`：验证显式 overlap 转换、exact item / entity identity、生命值与 ImpactLedger 不变、重复拒绝、移动/Launch/Directed adapter 栅栏，以及后续 Directed delivery 仍可造成唯一一次伤害；
3. `ControlledWeaponRunHost.OrbitThreatRouting`：验证 exact-item Host 路由、未知 item 拒绝、零生命值写入与共享 ordinal。

全量 `Shanmen.0_0_10` 从 P6.9 的 `156` 个增加至 `159` 个，最终 `159/159` Success。

## 4. 兼容性

- 未建立第二个 detector、candidate、ordinal、Run、Host、entity registry、impact、inventory 或 vitality authority；
- 未修改既有 `FShanmenHitCandidate`、ImpactId 或 Directed damage formula；
- 既有 Directed contact window、sweep / overlap delivery、Launch / Redirect / Recall 与 lifecycle 测试全部保持通过；
- 候选层不禁止 self target；自目标是否合法仍属于后续 target policy，不混入几何转换；
- 未新增默认 overlap 半径、碰撞通道、采样频率、每目标冷却、伤害、威慑、格挡或防御数值；
- 未修改 Profile schema、存档、item definition、GameplayTags、input mapping 或资产；
- 未启动第二套世界查询或自动碰撞轮询。

## 5. 修改范围

- `Source/ShanmenCombatRuntime/Public/Private/ShanmenControlledWeaponExecution*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponSession*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponWorldAdapter*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponProductController*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost*`；
- 本 Report 与同名 Log。

生产源码 10 个、测试源码 3 个、文档 2 个。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 5 | 0 | 0 | `D4A621C60BCEF7C0C8D56E1765ECD83AF6F6FE8CF61352BCE99A3D38B90CA937` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 29 | 0 | 0 | `01B92AF1581A41E8E03AFC4543E873AC3C893A86A5577BA482FF05DF7839498D` |
| `Shanmen.0_0_10` | 159 | 0 | 0 | `27489DE0C5E2A2939B70365172C9F38EEAE23988D9C0BD668FC0807B7FAAD95F` |

- 三份日志均有唯一实际 `Cmd: Automation RunTests`、queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`；
- changed-file gate：`PASS Changed=13 Rules=5 Required=11 Logs=3`；
- 加入 Report / Log 后最终 staged gate：`PASS Changed=15 Rules=5 Required=11 Logs=3`；
- regression coverage self-test：`14/14 PASS`；
- 新增生产行的 `ApplyDamage / TakeDamage / DeliverControlledWeaponImpact / TryResolveCandidate / DamagePacket / ImpactLedger / GetWorld / OverlapMulti / SweepMulti / LineTrace / SpawnActor / RNG / input / Reserve / Commit` 扫描命中 `0`；
- 最新源文件时间早于已测试 Editor DLL 与 Game executable；
- `git diff --check`：native exit `0`。

## 7. 首次验证与契约复审

首次 Editor integration build 为 `42/42` actions、`Result: Succeeded`、native exit `0`、`160.14s`。首次 Game build 为 `39/39`、`Result: Succeeded`、native exit `0`、`137.05s`。

首次 focused Runtime、focused Product 与 full 自动化分别为 `5/5`、`29/29`、`159/159` Success，没有源码、自动化或编译失败。

实现前复审发现：若 Orbit 与 Directed 各自维护 ordinal，会产生同 Action / Detector / Target / Ordinal 的身份碰撞；若在 Orbit emission 活动时允许 Launch，候选窗口可能在中途变成可结算窗口。本轮因此复用唯一 `EmissionSession`，并在 Launch 上加入活动 emission 栅栏。

## 8. 编译

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `42/42`，`Result: Succeeded`，native exit `0`，`160.14s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `39/39`，`Result: Succeeded`，native exit `0`，`137.05s`；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-10-controlled-weapon-orbit-threat-candidates>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-10-controlled-weapon-orbit-threat-candidates/Docs/Report/Dev.D.UE.0.0.10.P6.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-10-controlled-weapon-orbit-threat-candidates/Docs/Log/Dev.D.UE.0.0.10.P6.10.r0_log.md>
