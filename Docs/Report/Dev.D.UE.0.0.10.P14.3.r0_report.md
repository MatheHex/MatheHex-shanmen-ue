# Dev.D.UE.0.0.10.P14.3.r0 Report

## 1. 结论

P14.3 已完成并通过 P 阶段门禁。

本阶段把 P14.2 的 immutable Meridian Shock presentation event 收敛为 consumer-owned、immutable、self-validating 的显示模型。HUD、音频、VFX 或调试消费者现在可以各自保存上一份 `PresentationViewState`，把下一份合法事件交给无状态 reducer，并获得 `Visible` 或 `Hidden` 的只读模型；代码库没有新增共享 cursor、事件总线、Widget、timer、condition、attribute 或 timeline 权威。

最终证据：ViewState focused `4/4`、0.0.10 全量 `712/712`、V3 Attributes `4/4`；三份最终日志共记录 `720` 个 Success、`0` Fail，全部 native exit `0`。changed-file regression gate、248 项流程自检、144 条 JSON 规则、13 个只读反射字段扫描、模块边界扫描、`git diff --check`、Game 与 Editor Development 构建全部通过。

## 2. 功能性

### 2.1 Consumer-owned reducer

新增 `Fdemo_mapShanmenCombatConditionPresentationViewReducer::Reduce`。调用方显式传入自己的 previous view state 与 P14.2 event；reducer 本身不缓存任何状态。

- 空 cursor 只接受 `Activated`，不能从 `Refreshed` 或 `Expired` 中途伪造消费历史；
- 有效 cursor 要求 Run、target、timeline、definition identity 完全一致；
- event 的 previous status 必须精确等于 consumer cursor 的 current status；
- observed tick 与 condition revision 必须前进；
- exact event replay 返回 typed `DuplicateEvent`，保留上一状态但不生成新状态；
- foreign、stale、skipped、invalid 或损坏输入全部 fail closed。

### 2.2 Immutable display model

新增 `Fdemo_mapShanmenCombatConditionPresentationViewState`：

- `USTRUCT(BlueprintType)`；
- 13 个反射字段全部为 private `VisibleAnywhere, BlueprintReadOnly`；
- 没有 `BlueprintReadWrite`、`EditAnywhere`、`EditDefaultsOnly` 或 setter；
- 保存 source event、authority identity、display mode、observed/remaining/duration ticks、timeline rate、condition revision 与 movement multiplier；
- `Activated` / `Refreshed` 投影为 `Visible`，`Expired` 投影为 `Hidden`；
- `ViewStateId` 由 source event、current status、mode、tick 与 revision 确定性派生；
- `IsValid()` 重新校验 source event、字段投影、mode 与 identity；复制后不依赖组件生命周期。

### 2.3 Blueprint-pure facade

`Udemo_mapShanmenCombatConditionPresentationViewLibrary::TryReduceMeridianShockPresentationView` 提供 `BlueprintPure` 入口。它只在 reducer 产生新状态时返回 true；duplicate 或任意失败返回 false，并清空 `OutState`，避免调用方误用旧输出。

该 facade 不接入 GameMode，不持有全局 previous state。不同消费者可以用不同刷新节奏独立前进，互不覆盖 cursor。

## 3. 完整性

新增四个 Automation contract：

1. `Activation`：空 consumer cursor 从真实 dormant -> active event 生成 deterministic visible state，Blueprint facade 返回同一模型；
2. `RefreshAndDuplicate`：tick 30 refresh 精确前进 revision 与 remaining ticks，exact event replay 为 explicit no-change，Blueprint failure 清空输出；
3. `ExpiryAndReactivation`：tick 90 expiry 投影为 hidden，随后同一 authority 的合法 reactivation 再投影为 visible；
4. `ConsumerFences`：invalid event、缺失 activation cursor、stale event、跳过 refresh 的 sequence mismatch 与 foreign Run event 全部返回精确 typed failure。

测试复用真实 `CombatRunFixedTimeline + PlayerVitalityAuthority + AttributeComponent + CombatConditionComponent + Status + PresentationEvent` 链，没有创建第二套 condition、时钟、attribute 或 dispatch authority。

## 4. 权威与兼容性边界

- condition 生命周期、expiry 与 revision 仍属于 `CombatConditionComponent`；
- 30 Hz tick 仍属于 `CombatRunFixedTimeline`；
- movement modifier 仍属于既有 `AttributeComponent`；
- vitality commit 与 damage 仍属于既有 vitality/impact authority；
- P14.1 status snapshot 仍是 immutable observation evidence；
- P14.2 event 仍是两份 status 之间的 immutable transition evidence；
- P14.3 view state 只属于持有它的单个表现消费者；
- reducer 不分发事件、不轮询世界、不创建 timer、Actor Tick 或 Widget；
- 没有修改 GameMode、HUD、VFX、音频、输入、damage、inventory、schema 或 GAS；
- P1-P13、P14.0-P14.2 与 P15.0-P15.1 行为保持兼容；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenCombatConditionPresentationViewState.h/.cpp`：新增 read-only ViewState、typed reducer result、无状态 reducer 与 BlueprintPure facade；
- `demo_mapShanmenCombatConditionComponentTests.cpp`：fixture 支持显式 Run/target，并新增四个 reducer contract；
- `ShanmenRegressionMap.json`：新增 ViewState 精确路径规则，并让 condition/status/event 上游修改都要求 ViewState 证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 反例；
- Report/Log 之前 5 个代码、测试、流程文件净变更 `+889 / -4`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.CombatCondition.MeridianShock.PresentationViewState` | 4 | 0 | 0 | `35C79C2DD378B1C814D0308D6FC6FC1F4FEA2BD4B60B80699E1B7991DA35A2AB` |
| `Shanmen.0_0_10` | 712 | 0 | 0 | `C668E7D5EC5993CDBE8038F817DF9A495C50E68118DD5391092DADFC30A54169` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | `07D37124E552DDCD789A457046A07CE158603BF9A78792D3F80BF61EBF1C9D90` |

三份最终日志均有 terminal completion evidence，native exit `0`，合计 `720` 个 Success、`0` Fail；group 存在预期重叠。全量首末 Success 时间为 `14:43:52.272 -> 15:11:34.327`，约 `27m42.06s`。全量日志中的 Fatal error、Unhandled Exception、Ensure condition failed 与 Result Fail 均为 `0`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=8 Logs=3
SELF_TEST: PASS 248/248
READ_ONLY_SCAN: PASS BlueprintReadOnly=13 MutableExposures=0
PRESENTATION_VIEW_BOUNDARY_SCAN: PASS no UWorld/AActor/ApplyDamage/RNG/timer/TickComponent/GetWorld dependencies
JSON_PARSE: PASS Rules=144
git diff --check: PASS (native exit 0)
```

全量 `Shanmen.0_0_10` 覆盖 condition parent、status、event、view、timeline 与 vitality；独立 ViewState focused 提供精确新切片证据，V3 Attributes 提供 legacy attribute 兼容证据。

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded, UHT source discovery | 6 / 47.89s | 0 | `667DB7A36E7F3E8F8ED24AB33154876254197F8948A28E1726C0DFCCFF95F5CC` |
| Editor post-review | Succeeded | 4 / 5.43s | 0 | `EE2DA707507732A6D0F59B7E1A34B9C5EDB8B3B9E2BE2F52F9DE0ABC50B97043` |
| Game final | Succeeded | 5 / 32.61s | 0 | `FE99D041938AC344ADB4F8A9F7683742D633AD049533E67524F33EA06D384CB8` |
| Editor final | Succeeded, up to date | 0 / 0.94s | 0 | `019B490E2DF87E15469D94D5B4AE86CEE3B388FDDD7A89BD6EA56210590C9B80` |

最终产物：

- `UnrealEditor-demo_map.dll`：14,319,616 bytes，SHA-256 `14CE6EB796673ACFF829437E45CD5A4658B8AE0D2821FDB3B11E381A45C82B1D`；
- `demo_map.exe`：355,735,552 bytes，SHA-256 `61DD62F816CA99B80CB3DE3154CEEB4059A5ADC9A5F7C37F873F651B33A2542D`。

## 9. 异常与修复记录

首次完整实现、聚焦组、全量与 Editor build 均通过。提交前源码复审发现：`IsEmpty()` 若只检查 `SourceEvent.IsValid()==false`，理论上可能把“非空但损坏的 source event”误判为默认空 cursor。最终实现改为逐项要求 event id、previous/current status id 均无效且 cue 为 `Invalid`；随后重新编译并重跑最终全量证据，没有复用修改前结果。

最终 V3 Attributes 的第一次组合执行虽返回 native exit `0`，日志只完成 `3/4` 且没有 terminal marker。changed-file gate 正确拒绝该证据；单独有界重跑得到 `4/4` 与 terminal success。该异常没有被描述为产品失败，也没有通过放宽门禁处理。

全量 Automation 期间存在 UE 后台 connectivity probe warning；它不属于产品测试路径，队列以 `712/712`、terminal success 与 native exit `0` 完成。raw Automation/build logs 仅本地保存，Report 记录 SHA-256 摘要。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 consumer-owned immutable ViewState、纯 reducer、BlueprintPure facade、静态审查、NullRHI 无头 Automation、changed-file regression 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、真实 HUD/VFX/音效验收、Cook 或 Package。

P14.3 已完成 Meridian Shock 从 authority condition 到 status、event、consumer view 的只读链。下一步不应继续增加抽象 presentation wrapper；只有出现明确 HUD/VFX/音效内容或进入授权 F 阶段时，才把 consumer-owned state 接到真实表现并验证可见性、倒计时手感与 3 秒/0.75 参数体验。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p14-3-meridian-shock-consumer-view-state-reducer>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-3-meridian-shock-consumer-view-state-reducer/Docs/Report/Dev.D.UE.0.0.10.P14.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-3-meridian-shock-consumer-view-state-reducer/Docs/Log/Dev.D.UE.0.0.10.P14.3.r0_log.md>
