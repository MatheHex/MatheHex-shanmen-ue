# Dev.D.UE.0.0.10.P12.20.r0 Report

## 1. 结论

P12.20 已完成并通过 P 阶段门禁。

本阶段把 P12.10 `SwordRhythmProductSession` 的当前不可变展示状态接到 P12.19 command Host，但没有把 ProductSession、event batch、transport、executor 或 retry 合并成第二套权威。新 seam 分为三步：

1. 从有效 ProductSession 的当前 `PresentationState + canonical EffectCuePolicy` 捕获冻结 product projection；
2. 由调用方显式提供有序 projections、HostId、sequence `0`、CommandId 与双 consumer identity，捕获唯一 Create request；
3. stateless product route 只调用一次现有 `Host.TryRoute(CreateEnvelope)`，ProcessNext、End、attempt 与 executor 继续由调用方通过 P12.19 Host 管理。

最终结果：

- projection 同时冻结 `RunId + ConfigId + CuePolicyId + EffectCueEvent`，并在 `IsValid` 中验证 event state/policy 的交叉身份；
- `CaptureCurrent` 只读取 ProductSession 的现有权威值，并通过 P12.11 Adapter 产生 event，不复制 evaluation、presentation 或 cue 映射逻辑；
- 已捕获 projection 在源 ProductSession 继续推进后仍保持有效，但不再伪装为 current state；
- Create request 要求非空、同 Run/config/policy 且 observation revision 严格递增的 projection batch；
- request 只接受显式 sequence `0`，重建 P12.18 Create command 与 P12.19 envelope 后 deep-match，自校验完整 payload；
- exact request 在 ProductSession 推进后、以及 Host 完成 Process/End 后仍由 P12.19 返回历史 receipt，不重新创建 batch 或调用 executor；
- production route 中恰有一次 `Host.TryRoute`，没有 ProcessNext/End、executor、World、资产、timer、线程或后台 retry ownership；
- focused ProductRoute `5/5`，EffectCue 父前缀 `47/47`，0.0.10 全量 `613/613`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=21 / Logs=14`；
- regression gate self-test `202/202`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Product provenance projection

`Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::CaptureCurrent` 要求 ProductSession 已开始且至少接受一个观察。它读取：

- Session 的 RunId；
- canonical ProductConfig 的 ConfigId；
- canonical EffectCuePolicy 的 PolicyId；
- 当前 immutable PresentationState。

随后只调用现有 `EffectCueAdapter::Adapt`。projection 不保存 ProductSession 指针；`MatchesCurrentSession` 会重新核对当前 state 与 canonical policy，因此既能证明 capture-time provenance，也能在 Session 推进后明确返回 false，而冻结 event 本身仍可 replay。

### 2.2 Caller-owned ordered batch

`ProductCreateFactory::Capture` 不发现、不轮询、不收集事件。调用方传入自己保留的 projection 数组；factory 验证：

- 每项 projection 自验证成功；
- RunId、ConfigId、CuePolicyId 全部相同；
- observation revision 严格递增，拒绝倒序与重复；
- HostId、CommandId、Visual/Audio consumer 有效且双 consumer 不别名；
- sequence 必须由调用方明确传入并严格等于 `0`。

factory 从 projections 复制 event 数组，调用 P12.18 `TryCaptureCreate`，再调用 P12.19 envelope capture。request 的 `IsValid` 会从 private projections 重建 command/envelope 并 deep-match，不能以表面 GUID 掩盖 payload 差异。

### 2.3 Stateless Create route 与 replay

`ProductRoute::TryCreate` 只接收 frozen request 和 caller-owned Host。它不保存两者，不推导 sequence，不创建 executor，也不提供 ProcessNext/End 便捷包装。

首次 Create 后验证 Host 的 sequence-0 record、Host/Run identity 与 request envelope 一致；exact replay 则允许 Host 已继续推进或已 terminal，只要求历史 sequence-0 record 仍与 request deep-match。因此 replay 不依赖 ProductSession 当前状态，也不会因后续 durable records 被误判为新 Create。

## 3. 完整性

新增五个 focused Automation contract：

1. `Projection`：无 presentation 时 fail closed；真实 action 后 capture；重复 capture 确定；Session 推进后旧 projection 有效但不再 current；
2. `CreateReplay`：真实三 projection ordered batch；request 确定性；Create 后源 Session 继续推进；exact request replay 保持一条 Host record；
3. `CaptureFence`：empty、倒序、重复 revision、foreign Run 与非零 Create sequence/consumer alias 全部拒绝；
4. `HostFence`：sequence 0 建立 Host；foreign/different historical envelope 均由既有 historical sequence fence 拒绝；exact request 可重放；
5. `FullCompatibility`：真实 effect projections 经 Product route Create，再由调用方直接使用现有 ProcessNext + injected executors + End；terminal 后 product Create 历史 replay 不增加 record。

测试使用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + delivery/attempt/executor-driver/host/session/router/command-host` 链。fake executor 仅替代既有显式注入边界，不伪造 projection、event、command、envelope、session 或 Host receipt。

## 4. 权威与兼容性边界

- P12.1—P12.19 的 ProductSession、presentation、EffectCue、delivery、attempt、executor、Host、session、router 与 command Host 均未修改；
- GameMode、CombatRunCoordinator、fixed timeline 与 runtime action authority 均未修改；
- projection/request/route 均为 caller-owned 普通 C++ value 或 stateless class，不是 UObject、Subsystem、World service、registry 或 worker；
- product route 不持有 ProductSession、Host、projection buffer、executor、attempt、sequence、cursor 或 retry state；
- production 文件无 reflection、World/Actor、asset loading/playback、spawn、timer、async/thread、RNG、damage、attribute 或 gameplay-effect application；
- 多事件 batch 的收集和生命周期时机仍由上层明确拥有；本阶段不引入隐藏 append 或 streaming authority；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRoute.h/.cpp`：product projection、ordered Create request factory、stateless Create route 与 evidence result；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRouteTests.cpp`：五个真实 source-to-Host contract；
- `ShanmenRegressionMap.json`：新 ProductRoute 映射到 21 组完整 source-to-Host evidence；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 ProductRoute 正例与 focused-only 反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+1380 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `0D7E65CAEB168B728C97A49B16FFD38FE0DBAB5B94719F7AA1FF9C06815DBA3D` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `921D2879EEF04BD8688B204E3F907F36E83B65E0519853CD23133CE6EF7E5867` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `8B2C69A4D8E9EDD33A541B2A87D06C9C61D007FE6E2DFA6D237A440431FFBF49` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `38C5F7E6C10B26DF3D115C72C5B9AA03BAD7EA5B65138BD202A3C9E363B68943` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `8DB08DA58918240A7F147D8422369415CAD71ABD97BDEDB0B5AAA2809B034641` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `F4B09A9290D0A3081D363FA7508F675942226E0247916BB4ABB38FA6FE4389E5` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `21CAD0E13ACEF55DB4DF2CF500618F7664C25933944A9327B300F7F75E8780FF` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `704F0FB971504544B247E50B26F466AFF34F2435426FFFDD42B093744A4DE9E6` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `CF1538F4EE21B0FBC1CD43EB06866E98F41D01EFBC06AEF012F0A669BE7F27D4` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 47 | 0 | 0 | `C71A08567E7CFE03508981C45EB6AA2899FA9F254BED7DC6A348D40B6138A68F` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `14955B567A110208DD13784AD9DD2C0301D7DBA076A7C861BDDC01F375941771` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `2B3C9E2928FB3CE58485CD1ED4F6296F03306CF410D05093D9C4392230884CC7` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `A40204358940177D67CA27B0DD576AFF65BB229B4A633AC78C7480BE632D5166` |
| `Shanmen.0_0_10` | 613 | 0 | 0 | `91138941141986D175E0AA54933B5453D1868CBD9F27557C83E90C60B4751E19` |

EffectCue 父前缀由 `42` 增加至 `47`；全量由 `608` 增加至 `613`。十四份最终日志均有唯一选定 `RunTests` group、native terminal marker、Fail `0`；最后一个选定命令之后 automation error/Fatal/Unhandled/Ensure 为 0。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=21 Logs=14
SELF_TEST: PASS 202/202
PRODUCT_ROUTE_CALL_SCAN: PASS exactly one Host.TryRoute call
PROCESS_END_OWNERSHIP_SCAN: PASS no ProcessNext/End call in production route
RUNTIME_BOUNDARY_SCAN: PASS World/Actor/assets/timer/async/thread/RNG absent
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 24.67s | 0 |
| Editor after test corrections | Succeeded | 4 / 5.78s and 4 / 6.57s | 0 |
| Game final | Succeeded | 4 / 24.09s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.93s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,600,256 bytes，SHA-256 `16867E530BB15DE42B212D537CF9BFEDE511BF7DD65BFF6C67CD6A0C93BF8385`；
- `demo_map.exe`：355,168,768 bytes，SHA-256 `FBB4D4142A5B0E07D0F5427C635BA7C9ABA59702C1E55748C2B380D56475D47F`。

构建未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 focused Automation 原生退出码为 `0`，但 selected-result 扫描发现 `4 Success / 1 Fail`，日志 SHA-256 为 `3765CDB86D1AD4F137AFD571AB29E8A6F2583741DD6CB61F6981DD352208308D`。失败来自测试把“已占用 historical sequence 0 + foreign HostId”预期为 `HostIdentityConflict`；P12.19 的冻结顺序会先把所有 historical sequence 不同 envelope 判为 `SequenceConflict`。生产代码无需修改，修正测试预期后 focused 达到 `5/5`。

封版审查随后补强 source Session advance 与 terminal Host historical Create replay 两条断言；重新构建并重跑 focused、EffectCue 父组和全量，全部成功。首次失败日志保留在本地，没有把 native exit `0` 误报为测试通过。

UE 启动阶段仍有选定 `RunTests` 之前的既有 13 条 automation condition diagnostics；选定阶段 error/Fatal/Unhandled/Ensure 为 0。raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 product projection/Create seam、注入 fake executor、静态审查、无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.21 解决实时调用粒度，而不继续叠 ledger wrapper：定义一个 caller-driven、单 projection 的完整 execution transaction，由上层显式提供 Host/command/attempt identities 与 Visual/Audio executors，在一次同步调用中走 Create → Process → End 并返回全证据；它仍不保存 ProductSession、Host 或 executor。这样下一步可接 GameMode 当前 action 回调，同时避免为了实时 cue 给 immutable batch 引入隐藏 append 权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-20-sword-rhythm-cue-execution-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-20-sword-rhythm-cue-execution-product-route/Docs/Report/Dev.D.UE.0.0.10.P12.20.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-20-sword-rhythm-cue-execution-product-route/Docs/Log/Dev.D.UE.0.0.10.P12.20.r0_log.md>
