# Dev.D.UE.0.0.10.P14.0.r0 Report

## 1. 结论

P14.0 已完成并通过 P 阶段门禁。

本阶段交付 0.0.10 第一条真实战斗异常状态纵切：Boss Charge 只有在既有 vitality authority 成功提交伤害后，才会把同一份 committed receipt 投影为 `Meridian Shock`。状态持续现有 30 Hz canonical fixed timeline 的 90 ticks（3 秒），通过既有 attribute authority 施加单一 exact-handle `MoveSpeed x0.75` modifier，到期自动移除，Combat Run teardown 也会先移除该 modifier 再释放状态。

`90 ticks` 与 `0.75` 是明确标记的原型参数，不宣称为最终平衡值。实现没有建立第二套伤害、生命、属性、时钟或 Actor Tick 真值。

最终证据：focused `5/5`、0.0.10 全量 `699/699`、四组 GameMode legacy 回归合计 `116/116`；六份日志共记录 `820` 个 Success、`0` 个 Fail，均 Queue Empty、原生退出 `0`。changed-file regression gate、边界扫描、JSON、自检、`git diff --check`、Game 与 Editor Development 构建全部通过。

## 2. 功能性

### 2.1 Run-scoped condition authority

新增 `Udemo_mapShanmenCombatConditionComponent`：

- 以 `RunId + TargetEntityId + TimelineId` 绑定一次 Combat Run；
- 只接受同一目标、同一 timeline、单调 tick 的有效 `FShanmenVitalityCommitReceipt`；
- 只接受 `AppliedDamage > 0` 的 committed impact；被闪避、完美格挡、零伤害或无效 receipt 不产生状态；
- 以 `ImpactId + ResolutionId` 派生 immutable application receipt；
- exact replay 返回原 evidence，不重加 modifier、不推进 revision；
- 同 Impact 携带不同 Resolution、foreign target/timeline、stale tick 均 fail closed。

### 2.2 Meridian Shock 投影

首个 condition definition 为：

- `Condition.Injury.MeridianShock.Minor.r1`；
- modifier source `Condition.Injury.MeridianShock.Minor.MoveSpeed.r1`；
- canonical duration `90` ticks；
- movement multiplier `0.75`。

组件使用既有 `Udemo_mapAttributeComponent` 和确定性 `Fdemo_mapModifierHandle`。首次命中安装一个 modifier；新 committed hit 只刷新到 `max(当前到期 tick, 新命中 tick + 90)`，不会堆叠第二个同源 modifier。开始新 Run 前会清理 exact orphan handle，销毁或 recovery reset 也执行 best-effort exact cleanup。

### 2.3 Fixed timeline 生命周期

- GameMode 在 canonical fixed timeline 成功启动后绑定 condition component；
- 每次既有 timeline advance 后，把同一 canonical sample 交给 condition component；
- `ObservedTick == ExpiryTick` 时精确移除 modifier，随后状态保持 inactive；
- 已处理 impact 在过期后重放不会复活状态；
- Combat Run teardown 在 timeline/coordinator 释放前调用 condition `TryEnd`；foreign Run teardown 被拒绝。

### 2.4 Boss Charge 产品接线

`ExecuteM01BossShapeAttack` 只在以下全部事实成立时投影 Meridian Shock：

1. 既有 attack route 已执行；
2. attack family 精确为 `BossCharge`；
3. 既有 vitality commit 成功；
4. committed receipt 的 applied damage 大于零；
5. condition component 与 canonical timeline sample 都可用。

GameMode 只负责接线和审计日志；伤害仍由既有 impact/vitality authority 决定，移动速度仍由既有 attribute modifier authority 决定。

## 3. 完整性

新增五个 Automation contract：

1. `CommittedImpact`：真实 vitality commit 后验证 `MoveSpeed 600 -> 450`，且只有一个 exact modifier；
2. `ReplayAndRefresh`：exact replay 幂等，新 committed impact 在 tick 30 把到期点刷新至 tick 120，modifier 不堆叠；
3. `IdentityFences`：foreign target/timeline、stale sample、零伤害、同 Impact 冲突 Resolution 全部拒绝且不改状态；
4. `Expiry`：tick 89 保持生效，tick 90 精确恢复，过期 impact replay 不复活；
5. `RunTeardown`：foreign Run 不可释放，matching Run 删除 exact modifier 并清空状态，组件可安全绑定下一 Run。

测试使用真实 `CombatRunFixedTimeline + PlayerVitalityAuthority + AttributeComponent`，没有通过手工布尔值或第二套时间/伤害替身制造通过结果。

## 4. 权威与兼容性边界

- impact/防御结算继续属于 `ShanmenCombatCore`；
- applied damage 与生命变化继续属于 `PlayerVitalityAuthority`；
- 30 Hz tick 继续属于 `CombatRunFixedTimeline`；
- movement 结果继续由既有 `AttributeComponent` 聚合；
- condition component 只拥有 Run-scoped 状态、application replay ledger 与其 exact modifier handle；
- Boss Charge 之外的 attack family 不受影响；
- P1-P13 的 inventory、action、guard、evasion、formation、sword-rhythm 与 presentation authority 未复制；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenCombatConditionComponent.h/.cpp`：新增 Run-scoped Meridian Shock condition authority；
- `demo_mapShanmenCombatConditionComponentTests.cpp`：新增五个真实 receipt/timeline/attribute contract；
- `demo_mapGameMode.h/.cpp`：新增组件安装、Run 绑定、Boss Charge committed receipt 投影、timeline advance、expiry 与 teardown 接线；
- `ShanmenRegressionMap.json`：新增 `CombatCondition` changed-file 回归规则，并把 GameMode 接线纳入 focused condition 证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 反例；
- Report/Log 之前 7 个代码、测试、流程文件净变更 `+1378 / -2`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.CombatCondition.MeridianShock` | 5 | 0 | 0 | `9BFBE1FE8199F15218B301F16835ABF4D6A787B4A8CD53F44ED083B85FB4538D` |
| `Shanmen.0_0_10` | 699 | 0 | 0 | `DD5AD5D2C63F17077468B07153EF92A8AC7483937156CD87493C231130AC5DB7` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 0 | `A4D857C4808119CBEBE0A127106A78D7B39B60352B289E9E7D1EFBC9198E0EE7` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `D68CC32D4DB02D9C0CF9378D823E565DCA13F8D51318C745443008BB27D13387` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `C5E39A08AF400189CC1EA960D024BB7721E74B37950251703E13F05CD769BBD1` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | `543EA057213E8C3CDBD0EF9D2E3126B92DC29FE78A55EE866253E0E8423711B2` |

六份最终日志均有 Queue Empty，native exit `0`。全量首末 Success 时间为 `09:35:16.534 -> 10:05:31.453`，约 `30m14.92s`；基线从 P13.2 的 `694` 增至 `699`。selected phase 的 Fatal error、Unhandled Exception、Ensure condition failed、AutomationController Error 与 Result Fail 均为 `0`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=45 Logs=6
SELF_TEST: PASS 236/236
CONDITION_BOUNDARY_SCAN: PASS no UWorld/AActor/ApplyDamage/RNG/timer/Actor-tick dependencies
JSON_PARSE: PASS Rules=138
git diff --check: PASS (native exit 0; only existing LF -> CRLF notices)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Failed, `OtherCompilationError` | 26 planned / 121.85s | 6 |
| Editor retry | Succeeded | 4 / 10.02s | 0 |
| Game final | Succeeded | 25 / 99.57s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.94s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：14,215,680 bytes，SHA-256 `BC50258510EBBAC6C6CBD1BCD8B864CE44C7AC2B999E1D8FFA8E7C31863A613D`；
- `demo_map.exe`：355,655,680 bytes，SHA-256 `83BC49FF5A107A43C471B90DFC30227E0DD1B1DE81CD5C77E8FAEA813FB01298`。

## 9. 异常与修复记录

首次 Editor 构建真实失败：`demo_mapGameMode.cpp` 把运行时三元表达式用于 `UE_LOG` verbosity token，引发 `C2039/C2065/C2275/C2672`，UBT 最终为 `OtherCompilationError`、原生退出码 `6`。这是本轮源码错误，不是 Windows commit memory、页面文件或构建环境故障。

修复为固定 `Log` 审计记录，并在失败结果时另发固定 `Error` 记录。随后 Editor retry、全部 Automation、Game final 与 Editor final 均成功。首次失败日志 SHA-256 为 `A158EEF01CB6306E656BFDC51914EC41C0232E878DC30D0F9A9CAF2D5C23DE5E`；成功 retry 日志 SHA-256 为 `F3CCD47A67522BF533AC7978074EEDF2F1781252159FCC78B181DD15B0EAB905`。raw Automation/build 日志仅本地保留。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 condition authority、Boss Charge 接线、静态审查、NullRHI 无头 Automation、changed-file regression 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、真实表现验收、Cook 或 Package。

下一步可在 P 阶段把 condition 的只读状态投影给表现层，并在出现第二种真实 condition 或策划定稿参数时再抽 authored catalog；当前不为假想扩展建立第二套通用状态框架。Meridian Shock 的实际手感、Boss Charge 命中表现与 3 秒/0.75 参数验收属于明确授权后的 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p14-0-meridian-shock-condition-vertical-slice>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-0-meridian-shock-condition-vertical-slice/Docs/Report/Dev.D.UE.0.0.10.P14.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-0-meridian-shock-condition-vertical-slice/Docs/Log/Dev.D.UE.0.0.10.P14.0.r0_log.md>
