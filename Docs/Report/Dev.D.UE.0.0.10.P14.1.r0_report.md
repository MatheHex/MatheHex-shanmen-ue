# Dev.D.UE.0.0.10.P14.1.r0 Report

## 1. 结论

P14.1 已完成并通过 P 阶段门禁。

本阶段把 P14.0 的 Meridian Shock condition authority 投影为不可变、Blueprint 可读的状态快照，并通过 GameMode 提供只读查询。表现层现在可以读取 Run、目标、固定时间线、状态定义、观测 tick、到期 tick、剩余 tick、condition revision、原型时长与移动倍率，但不能通过该结构改写 condition、attribute 或 timeline 真值。

状态身份由全部规范字段确定性派生；同一 authority state 重复捕获产生相同 `StatusId`，时间推进、刷新、到期或新 Run 会产生不同快照。旧快照在 authority 后续变化或 teardown 后仍保持自校验有效，查询失败则清空输出并 fail closed。

最终证据：status focused `5/5`、parent condition `10/10`、0.0.10 全量 `704/704`、四组 legacy 回归合计 `116/116`；七份日志共记录 `835` 个 Success、`0` 个 Fail，全部原生退出 `0`。changed-file regression gate、238 项自检、只读字段扫描、模块边界扫描、JSON、`git diff --check`、Game 与 Editor Development 构建全部通过。

## 2. 功能性

### 2.1 不可变 Blueprint 状态快照

新增 `Fdemo_mapShanmenCombatConditionStatusSnapshot`：

- `USTRUCT(BlueprintType)`，可供 Blueprint 和表现层读取；
- 13 个字段全部为 private `VisibleAnywhere, BlueprintReadOnly`，没有 `BlueprintReadWrite`、`EditAnywhere`、`EditDefaultsOnly` 或 setter；
- 暴露 `StatusId`、`RunId`、`TargetEntityId`、`TimelineId`、`DefinitionId`、`ObservedTick`、`ExpiryTick`、`RemainingTicks`、`ConditionRevision`、`DurationTicks`、`TimelineTicksPerSecond`、`MoveSpeedMultiplier` 与 `bActive`；
- 构造入口仅对 condition component 开放，外部只能复制和读取。

### 2.2 确定性身份与自校验

`StatusId` 使用 `demo_map.Combat.Condition.StatusSnapshot.r1` 命名空间和全部规范字段确定性派生。`IsValid()` 重算身份并检查：

- Run、target、timeline 与 Meridian Shock definition 身份有效；
- timeline 必须是该 Run 的 canonical fixed timeline；
- 时长固定为 90 ticks，频率固定为 30 Hz，移动倍率固定为 0.75；
- active 状态必须有正 revision、未来 expiry，且 `RemainingTicks == ExpiryTick - ObservedTick`；
- inactive 状态必须使用 `ExpiryTick == INDEX_NONE` 且 remaining 为 0。

`Matches()` 只接受两份自校验有效且 `StatusId` 相同的快照，避免仅凭部分字段误判同一状态。

### 2.3 只读产品查询

`Udemo_mapShanmenCombatConditionComponent::TryCaptureMeridianShockStatus` 只复制组件已有的 Run-scoped 状态，不推进时间、不刷新状态、不安装或移除 modifier。空组件或无效状态返回 false，并把调用方输出恢复为无效空快照。

`Ademo_mapGameMode::TryGetMeridianShockStatus` 是 `BlueprintPure` 查询，只委托既有 condition component 捕获快照。GameMode 不缓存第二份状态，也不成为 condition authority。

### 2.4 生命周期语义

- Run 刚绑定时可读取确定性 inactive snapshot，revision 为 0；
- committed hit 后读取 active snapshot，tick 0 时 remaining 为 90；
- canonical timeline 到 tick 30 后，读取 remaining 60，旧 tick-0 副本不被改写；
- exact impact replay 保持相同 status identity 与 revision；
- 新 committed impact 刷新到期点和 revision，但仍只有一个 exact modifier；
- tick 90 到期后读取 inactive revision 2，active 旧副本仍有效；
- matching Run teardown 后查询失败并清空输出；下一 Run 产生新的确定性 inactive identity。

## 3. 完整性

新增五个 Automation contract：

1. `InitialSnapshot`：验证初始 inactive canonical snapshot、重复捕获身份一致，以及未绑定 GameMode fail closed；
2. `ActiveCountdown`：验证 tick 0 的 90 ticks、tick 30 的 60 ticks与旧副本不可变；
3. `ReplayAndRefresh`：验证 exact replay 身份稳定，刷新后 revision/expiry 更新且 modifier 不堆叠；
4. `ExpiryTransition`：验证 exact expiry 转为 inactive，旧 active evidence 仍自校验有效；
5. `RunTeardown`：验证 teardown 后查询失败并清空输出，后续 Run 获得不同 identity。

测试复用真实 `CombatRunFixedTimeline + PlayerVitalityAuthority + AttributeComponent + CombatConditionComponent`，没有创建第二套 condition、时钟或 attribute 替身。

## 4. 权威与兼容性边界

- condition 生命周期、expiry 与 revision 继续属于 `CombatConditionComponent`；
- 30 Hz tick 继续属于 `CombatRunFixedTimeline`；
- movement modifier 继续属于既有 `AttributeComponent`；
- vitality commit 与 damage 继续属于既有 vitality/impact authority；
- status snapshot 只拥有不可变观察证据，不拥有写入口、timer、Actor Tick 或缓存 authority；
- GameMode 只提供 BlueprintPure 查询，不复制或重算 condition 生命周期；
- P1-P13 与 P14.0 的 inventory、action、defense、formation、sword-rhythm、vitality、condition 行为保持兼容；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenCombatConditionStatus.h/.cpp`：新增 immutable Blueprint-readable snapshot、deterministic identity 与 invariants；
- `demo_mapShanmenCombatConditionComponent.h/.cpp`：新增只读 capture seam；
- `demo_mapGameMode.h/.cpp`：新增 BlueprintPure 产品查询；
- `demo_mapShanmenCombatConditionComponentTests.cpp`：新增五个状态读取生命周期 contract；
- `ShanmenRegressionMap.json`：新增 `CombatConditionStatus` 规则，并强化 parent condition 对 status 证据的要求；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 反例；
- Report/Log 之前 9 个代码、测试、流程文件净变更 `+584 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.CombatCondition.MeridianShock.Status` | 5 | 0 | 0 | `3F4227EDBDA64511E28E55BB03EE0AD693E318FD98B8A10F948630BCDE7DD394` |
| `Shanmen.0_0_10.Product.CombatCondition.MeridianShock` | 10 | 0 | 0 | `B703556E68CD39B6D0A19EB9D7D48C21007AE8BF273673565CC90DB36AE4031F` |
| `Shanmen.0_0_10` | 704 | 0 | 0 | `399C1AE9B355B087EAC7B8A87FB6FCF7823315EEBC476FFC05CD4B3FACA2B394` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 0 | `632F09B7355AEF39279620DF23ADE40A016B8D637C79AAF78C5B37EC222A26E2` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `9BD5EFDC36CF834BC171CD66858C5CC3473F7A55904FFC32CE171ABCCC9115DC` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `C688AD0EDEFA8B85D21204DDA8D5F5580DF8BEF057F8191E0A4ED9C03261936F` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | `F50E09DB6C46AFF8FEEC70CE793C9A6C92BD7204D7A95ED4EC129701EAD1B5A7` |

七份最终日志均有 terminal queue-complete evidence，native exit `0`。日志合计 `835` 个 Success、`0` Fail；group 存在预期重叠。全量首末 Success 时间为 `10:26:18.252 -> 10:56:35.367`，约 `30m17.12s`；基线从 P14.0 的 `699` 增至 `704`。全量日志中的 Fatal error、Unhandled Exception、Ensure condition failed、AutomationController Error 与 Result Fail 均为 `0`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=46 Logs=7
SELF_TEST: PASS 238/238
READ_ONLY_SCAN: PASS BlueprintReadOnly=13 MutableExposures=0
CONDITION_STATUS_BOUNDARY_SCAN: PASS no UWorld/AActor/ApplyDamage/RNG/timer/Actor-tick dependencies
JSON_PARSE: PASS Rules=139
git diff --check: PASS (native exit 0; only LF -> CRLF notices)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 27 / 127.27s | 0 | `EB89F521A5E5A94E2540CB234597528DA6F18D6D02EECF858FC9A1382AF3D339` |
| Game final | Succeeded | 26 / 124.81s | 0 | `81CADA042E5BA1E8060EDD0A9641EE59DADA343202B321B548DD393733ED3D7D` |
| Editor final | Succeeded, up to date | 0 / 1.63s | 0 | `4115FD37FC546508633C22C9CBF9AD7D80732924DD14158C6711729134245AA1` |

最终产物：

- `UnrealEditor-demo_map.dll`：14,242,816 bytes，SHA-256 `BF350EB936F51A983A8188358DE446AC959EE1807C48F4682F7FD200FB6E8191`；
- `demo_map.exe`：355,677,696 bytes，SHA-256 `01CAC69585489630FABEFFE719D0CA9771F29AC344405B1FB503F1B8F6ADB903`。

## 9. 异常与修复记录

本阶段没有源码编译失败、Automation 失败、Windows commit-memory/pagefile 故障或构建重试。Editor initial、全部 Automation、Game final 与 Editor final 均首次成功。raw Automation/build 日志仅本地保留，Report 记录其 SHA-256 作为可核验摘要。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段只读状态模型、产品查询接线、静态审查、NullRHI 无头 Automation、changed-file regression 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、真实表现验收、Cook 或 Package。

下一步可在 P 阶段增加消费该 snapshot 的轻量表现策略或状态提示适配器，但必须继续保持 presentation 单向读取，不能让 UI、Blueprint 或 GameMode 接管 condition/attribute/timeline authority。实际 HUD 可见性、倒计时手感和 3 秒/0.75 参数验收属于明确授权后的 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p14-1-meridian-shock-status-read-model>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-1-meridian-shock-status-read-model/Docs/Report/Dev.D.UE.0.0.10.P14.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-1-meridian-shock-status-read-model/Docs/Log/Dev.D.UE.0.0.10.P14.1.r0_log.md>
