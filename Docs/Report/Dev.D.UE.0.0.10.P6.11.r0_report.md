# Dev.D.UE.0.0.10.P6.11.r0 Report

## 1. 结论

P6.11 已完成 detector emission 的确定性候选回执闭环，结论为 **PASS**。

P6.10 的 Orbit 近身威胁候选不再在窗口关闭时被直接丢弃。唯一 `FShanmenDetectorEmissionSession` 现在保留本 emission 已接受的完整候选，并在 End 时返回 `FShanmenDetectorEmissionReceipt`。回执携带冻结 Context 与按稳定 `TargetEntityId` 规范排序的候选数组，可供后续显式 target / effect policy 审计消费，且不依赖不同目标的 UE overlap 回调顺序。

本阶段仍只输出几何证据：不决定采样半径、频率、目标合法性、每目标冷却、威慑/防御/伤害效果，也不读取或写入 Vitality。

## 2. 功能性

- `FShanmenDetectorEmissionReceipt` 提供只读 Context 与完整候选数组；
- `IsValid()` 校验 Context、Activation、Source、Detector、Kind、ordinal，以及严格递增且无重复的 TargetEntityId 顺序；
- `FShanmenDetectorEmissionSession` 由原有 `TSet<FGuid>` 升级为同一权威内的 `TMap<FGuid, FShanmenHitCandidate>`，没有增加第二个候选缓存或去重门；
- Begin 冻结 active Context，Accept 继续按 TargetEntityId 去重，End 生成规范排序回执后原子清空窗口并递增原有 ordinal；
- 空候选 emission 仍可形成合法完成回执，表示一次明确执行但未观察到目标的 sample；
- 旧无参数 `TryEndEmission()` / `TryEndOrbitThreatWindow()` 保留，并通过新回执重载实现，既有调用方无需迁移；
- Orbit receipt 沿 `CombatRuntime -> Product Session -> Product Controller -> Run Host` 逐层透传；
- Run Host 仍按 exact `ItemInstanceId` 路由，回执中的 Action 继续携带同一 source item；
- 规范排序只消除不同目标的回调排列差异，不改变“同一目标首个合法几何样本被接受”的既有语义。

## 3. 完整性

扩展 4 个既有自动化场景：

1. `WorldGameplay.DetectorEmissionSession`：以 B/A 与 A/B 两种目标回调顺序重放，验证两份完成回执获得相同规范顺序；
2. `CombatRuntime.ControlledWeapon.OrbitThreatCandidateOnly`：验证两个 Orbit 候选形成自校验回执、exact item 与共享 ordinal 保持不变，且 ImpactLedger 仍为 0；
3. `Product.ControlledWeaponController.OrbitThreatProjection`：验证 Product End 返回 exact target / item 回执，再进入 Directed damage；
4. `Product.ControlledWeaponRunHost.OrbitThreatRouting`：验证 exact-item Host 路由返回同一 canonical target。

本轮未新增测试名称；全量 `Shanmen.0_0_10` 保持 `159/159` Success。

## 4. 兼容性

- 未新增第二个 detector、candidate、ordinal、Run、Host、entity registry、impact、inventory 或 vitality authority；
- 未修改 `FShanmenHitCandidate`、ImpactId、伤害公式、DefenseResolver 或 Directed delivery；
- 旧 End API 继续可用，现有 BasicSword、ControlledWeapon 与 WorldGameplay 调用保持兼容；
- receipt 字段私有并只读暴露，调用方不能改写已完成证据；
- 未加入默认 target tag 策略、自目标规则、冷却、半径、通道、频率或效果数值；
- 未修改 Profile schema、存档、item definition、GameplayTags、input mapping 或资产；
- 未启动世界查询、自动 overlap 轮询或第二套物理 authority。

## 5. 修改范围

- `Source/ShanmenWorldGameplay/Public/Private/ShanmenDetectorEmissionSession*`；
- `Source/ShanmenCombatRuntime/Public/Private/ShanmenControlledWeaponExecution*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponSession*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponProductController*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost*`；
- 本 Report 与同名 Log。

生产源码 10 个、测试源码 4 个、文档 2 个。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.WorldGameplay.DetectorEmissionSession` | 1 | 0 | 0 | `3AC98284FDACE49E8023D3A915647B529AA6BF25965878258A05485C3B1F95E1` |
| `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 5 | 0 | 0 | `D612405CB70E8A26066AF56F5010DC40EC399D68DFDA6A662272B6B3B10813B9` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 29 | 0 | 0 | `8CAE8866C2E053D917F3DBCDD087CF629215B8D38D00CF9EA5C93C3ACF3406C6` |
| `Shanmen.0_0_10` | 159 | 0 | 0 | `34EE75CE33BC76DB16852619ECE00C5497521888080A07B5B644121890653AA0` |

- 四份日志各有唯一实际 `Cmd: Automation RunTests`、queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`；
- changed-file gate：`PASS Changed=14 Rules=5 Required=11 Logs=1`；
- 加入 Report / Log 后最终 staged gate：`PASS Changed=16 Rules=5 Required=11 Logs=1`；
- regression coverage self-test：`14/14 PASS`；
- 新增 Source 行 `251`；其中生产新增行 `190`；
- 新增行的 `ApplyDamage / TakeDamage / DeliverControlledWeaponImpact / TryResolveCandidate / DamagePacket / ImpactLedger / GetWorld / OverlapMulti / SweepMulti / LineTrace / SpawnActor / RNG / inventory Commit` 扫描真实命中 `0`；
- 最新源码时间早于已测试 Editor DLL 与 Game executable；
- `git diff --check`：native exit `0`。

## 7. 首次验证与契约复审

首次 Editor integration build 为 `52/52` actions、`Result: Succeeded`、native exit `0`、`178.18s`。首次 Game build 为 `47/47`、`Result: Succeeded`、native exit `0`、`154.58s`。

首次 WorldGameplay、Runtime、Product 与 full 自动化分别为 `1/1`、`5/5`、`29/29`、`159/159` Success，没有源码、自动化或编译失败。

实现前复审发现：若 Product Controller 另存 Orbit result，会形成 detector session 之外的第二个候选缓存；若直接按 overlap callback 数组输出，跨平台或不同物理回调排列会影响后续消费顺序。本轮因此把完整候选保留与规范排序放回唯一 detector emission authority，并只向上透传完成回执。

## 8. 编译

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `52/52`，`Result: Succeeded`，native exit `0`，`178.18s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `47/47`，`Result: Succeeded`，native exit `0`，`154.58s`；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-11-deterministic-threat-receipts>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-11-deterministic-threat-receipts/Docs/Report/Dev.D.UE.0.0.10.P6.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-11-deterministic-threat-receipts/Docs/Log/Dev.D.UE.0.0.10.P6.11.r0_log.md>
