# Dev.D.UE.0.0.10.P14.2.r0 Report

## 1. 结论

P14.2 已完成并通过 P 阶段门禁。

本阶段把 P14.1 的 immutable Meridian Shock status snapshot 投影为无状态、确定性的表现事件。表现消费者现在可以用自己持有的前一份与当前状态快照，识别 `Activated`、`Refreshed`、`Expired` 三种离散 cue；普通倒计时、完全重复轮询与 inactive 时间推进明确不产生事件。

适配器不保存 cursor、不分发事件、不创建 timer、widget、condition、attribute 或 timeline 真值。每个消费者独立持有 previous snapshot，因此不会引入共享表现 authority。事件携带前后完整状态证据，并由两个 `StatusId` 与 cue 确定性派生 `EventId`；重复适配同一转换得到相同事件身份，跨 Run、倒序观察及不可能转换全部 fail closed。

最终证据：presentation focused `4/4`、parent condition `14/14`、status `5/5`、0.0.10 全量 `708/708`、四组 legacy 回归合计 `116/116`；八份日志共记录 `847` 个 Success、`0` 个 Fail，全部原生退出 `0`。changed-file regression gate、240 项自检、只读字段扫描、模块边界扫描、JSON、`git diff --check`、Game 与 Editor Development 构建全部通过。

## 2. 功能性

### 2.1 离散表现 cue

新增 `Edemo_mapShanmenCombatConditionPresentationCue`：

- `Activated`：同一 authority identity 从 inactive 转为 active，且 condition revision 前进；
- `Refreshed`：同一 active condition 的 revision 前进，expiry 不得倒退；
- `Expired`：active 转为 inactive，revision 前进，且 current observed tick 已达到 previous expiry；
- `Invalid`：仅作默认无效值，不是可发出的 cue。

完全相同的快照、active 状态中 revision/expiry 不变而 remaining ticks 正常下降，以及 inactive 状态中 revision 不变的时间推进，都返回 typed `NoTransition`，不会制造每帧或每 tick 的表现事件。

### 2.2 不可变事件证据

新增 `Fdemo_mapShanmenCombatConditionPresentationEvent`：

- `USTRUCT(BlueprintType)`，可供 Blueprint 与表现层读取；
- 四个字段全部为 private `VisibleAnywhere, BlueprintReadOnly`，没有 `BlueprintReadWrite`、`EditAnywhere`、`EditDefaultsOnly` 或 setter；
- 保存 `EventId`、`PreviousStatus`、`CurrentStatus` 与 `Cue`；
- getter 可读取 source status identities、Run、target、timeline、definition、observed tick、前后 revision 与 remaining ticks；
- 事件复制后不依赖 condition component 生命周期，authority teardown 后仍可自校验。

### 2.3 确定性身份与自校验

`EventId` 使用 `demo_map.Combat.Condition.PresentationEvent.r1` 命名空间，由 previous `StatusId`、current `StatusId` 与 cue 的规范值确定性派生。`IsValid()` 会重新执行转换分类并重算 identity，只有 source snapshots 自身有效、转换合法、分类 cue 一致且 identity 精确匹配时才接受事件。

同一转换重复适配得到同一 `EventId`。事件的 `Matches()` 只接受两份都能自校验且 identity 相同的事件，避免仅凭部分字段误判。

### 2.4 Typed fail-closed 适配

`Fdemo_mapShanmenCombatConditionPresentationEventAdapter::Adapt` 返回 typed 状态：

- `Adapted`；
- `NoTransition`；
- `PreviousStatusInvalid` / `CurrentStatusInvalid`；
- `IdentityMismatch`；
- `StaleObservation`；
- `TransitionRejected`；
- `EventRejected`。

适配器先校验 source snapshots，再要求 Run、target、timeline 与 definition identity 全部一致；current observed tick 与 revision 不得小于 previous。非法输入不降级为普通事件，也不修改任何 authority。

### 2.5 Blueprint-pure facade

`Udemo_mapShanmenCombatConditionPresentationLibrary::TryAdaptMeridianShockTransition` 是 `BlueprintPure` facade。调用者显式传入自己持有的 previous/current snapshots；成功时输出 immutable event，`NoTransition` 或任意失败时返回 false 并清空 `OutEvent`。

该 facade 不需要修改 GameMode，也不缓存全局 previous snapshot。不同 HUD、音效、VFX 或调试消费者可按各自刷新节奏维护独立 cursor，同时共享完全一致的转换规则和确定性 event identity。

## 3. 完整性

新增四个 Automation contract：

1. `Activation`：验证真实 dormant -> committed active 转换产生 deterministic `Activated` event、Blueprint facade 返回精确同一事件，以及无效 previous source typed fail closed；
2. `CountdownIsNotAnEvent`：验证 tick 0 -> tick 30 的 remaining `90 -> 60` 明确返回 `NoTransition`，Blueprint 输出被清空，倒序轮询被识别为 stale；
3. `Refresh`：验证第二个 committed impact 在 tick 30 产生 `Refreshed`、revision `1 -> 2`、remaining 恢复 90、modifier 仍只有一个，exact replay 不重复发事件；
4. `ExpiryAndIdentityFences`：验证 exact tick 90 产生 `Expired`、revision `1 -> 2`、remaining 为 0，事件在 teardown 后仍有效，跨 Run snapshots 被 `IdentityMismatch` 拒绝。

测试复用真实 `CombatRunFixedTimeline + PlayerVitalityAuthority + AttributeComponent + CombatConditionComponent` fixture，没有创建第二套 condition、时钟、attribute 或 event-dispatch authority。

## 4. 权威与兼容性边界

- condition 生命周期、expiry 与 revision 继续属于 `CombatConditionComponent`；
- 30 Hz tick 继续属于 `CombatRunFixedTimeline`；
- movement modifier 继续属于既有 `AttributeComponent`；
- vitality commit 与 damage 继续属于既有 vitality/impact authority；
- P14.1 status snapshot 继续只承载 immutable observation evidence；
- P14.2 adapter 只是两份证据之间的纯转换，不持有 previous cursor；
- 每个表现消费者拥有自己的 polling cursor 和去重选择，不产生共享 UI 真值；
- 没有新增 Actor、Actor Tick、timer、widget、damage、RNG、world query 或 dispatch bus；
- P1-P13 与 P14.0/P14.1 的 inventory、action、defense、formation、sword-rhythm、vitality、condition 与状态读取行为保持兼容；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenCombatConditionPresentationEvent.h/.cpp`：新增 cue、immutable event、typed adapt result、无状态 adapter 与 BlueprintPure facade；
- `demo_mapShanmenCombatConditionComponentTests.cpp`：新增四个转换、倒计时、刷新、到期与 identity fence contract；
- `ShanmenRegressionMap.json`：新增 `CombatConditionPresentationEvent` 路径规则，并强化 parent condition/status 的表现事件证据要求；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 反例；
- Report/Log 之前 5 个代码、测试、流程文件净变更 `+733 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.CombatCondition.MeridianShock.PresentationEvent` | 4 | 0 | 0 | `BBBA05F6015E550AF05330B9C68FC8FF9BBB079E3FCB2C26C5501EFB19D7A0FD` |
| `Shanmen.0_0_10.Product.CombatCondition.MeridianShock` | 14 | 0 | 0 | `8EF60C49CCB3D17B1405AE844A3A83540BC9DB0F1A3C70BDE8AC5BBC8F7AF8CC` |
| `Shanmen.0_0_10.Product.CombatCondition.MeridianShock.Status` | 5 | 0 | 0 | `75B390E7029E9D9370043A8F36563EF47BB18E7529365F9FC2E9EE0A4FD093CC` |
| `Shanmen.0_0_10` | 708 | 0 | 0 | `83B04F073C2799D7E885E642E426FCEFDE35F00EB8C8A8AF8C129EF7D2B96293` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 0 | `6F6F36270F6D2ECF4EC0E742C22DD2B291F4072687C4684B93EFBCBF715E0522` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `25C8ED12911764EBF3EE0E54B5F977BB1114A94F58EDC8BC51CDF976460D5242` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `83019182C75E657FA3AB3AF4C6C3D314127F011EE8C1486E298E6D1398F01D19` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | `97E039004A9B2E3D617A546133363D04F7EE07A45F4956915367CF024BE90B8E` |

八份最终日志均有 terminal completion evidence，native exit `0`。日志合计 `847` 个 Success、`0` Fail；group 存在预期重叠。全量首末 Success 时间为 `11:46:33.690 -> 12:17:04.956`，约 `30m31.27s`；基线从 P14.1 的 `704` 增至 `708`。全量日志中的 Fatal error、Unhandled Exception、Ensure condition failed、AutomationController Error 与 Result Fail 均为 `0`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=7 Logs=8
SELF_TEST: PASS 240/240
READ_ONLY_SCAN: PASS BlueprintReadOnly=4 MutableExposures=0
PRESENTATION_BOUNDARY_SCAN: PASS no UWorld/AActor/ApplyDamage/RNG/timer/TickComponent dependencies
JSON_PARSE: PASS Rules=140
git diff --check: PASS (native exit 0)
```

changed-file gate 要求并实际找到 `demo_map.V3.Attributes`、0.0.10 全量、Meridian Shock parent、PresentationEvent、Status、CombatRunFixedTimeline 与 PlayerVitality 七组证据。

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded, UHT 3 generated files | 6 / 62.01s | 0 | `71B4DF3B00A285334939D25262951AFE3099CC67DFF23225ECC7512FFD0712B7` |
| Game final | Succeeded | 5 / 48.18s | 0 | `1ABF55870195770DD6B7AF297498C226F64EAADEE1E2FA88B466835130147660` |
| Editor final | Succeeded, up to date | 0 / 0.96s | 0 | `04CF4FEB69A27E56FB98BA97A66010EFAFDA0236F36E616309DE44B67D02FD23` |

最终产物：

- `UnrealEditor-demo_map.dll`：14,275,072 bytes，SHA-256 `088C92A95C53C0020E5EFFD83972CE23AACB653FD4445BF0C21C7D603A82AC03`；
- `demo_map.exe`：355,701,760 bytes，SHA-256 `DC42690A05A1699586E6921A070C940AB1ADC0A931304080A6B955EAE608E660`。

## 9. 异常与修复记录

本阶段没有源码编译失败、Automation 失败、Windows commit-memory/pagefile 故障或构建重试。Editor initial、全部 Automation、Game final 与 Editor final 均首次成功。

全量 Automation 期间 UE 后台 connectivity probe 对 `https://www.google.com/generate_204` 记录了超时 warning；该探测不属于产品测试路径，测试队列继续执行并以 `708/708`、terminal success 与 native exit `0` 完成。raw Automation/build 日志仅本地保留，Report 记录其 SHA-256 作为可核验摘要。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 immutable presentation event、纯转换规则、BlueprintPure 适配入口、静态审查、NullRHI 无头 Automation、changed-file regression 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、真实 HUD/VFX/音效验收、Cook 或 Package。

下一步可在 P 阶段增加 consumer-owned view-state reducer：由每个消费者保存自己的 previous snapshot，消费本阶段 event 并生成只读显示模型；仍不得让 HUD、Blueprint、GameMode 或共享 event bus 接管 condition/attribute/timeline authority。实际 cue 可见性、倒计时手感、HUD/VFX/音效表现与 3 秒/0.75 参数体验验收属于明确授权后的 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p14-2-meridian-shock-presentation-events>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-2-meridian-shock-presentation-events/Docs/Report/Dev.D.UE.0.0.10.P14.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-2-meridian-shock-presentation-events/Docs/Log/Dev.D.UE.0.0.10.P14.2.r0_log.md>
