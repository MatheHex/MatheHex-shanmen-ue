# Dev.D.UE.0.0.10.P12.23.r0 Report

## 1. 结论

P12.23 已完成并通过 P 阶段门禁。

本阶段在 P12.22 current Product dispatch 之前增加 caller-owned deterministic dispatch plan：调用方只提供一个 dispatch seed 与 Visual/Audio 两个逻辑 consumer scope，factory 将它们连同 frozen Product projection identity 规范化后，确定性派生 P12.21 transaction 所需的 8 个角色隔离 GUID。

最终结果：

- Host、Create/ProcessNext/End command、Visual/Audio consumer、Visual/Audio attempt 共 8 个 identity 均由单一 canonical hash seam 派生；
- 每个 identity 都绑定 dispatch seed、双 consumer scope、RunId、ConfigId、CuePolicyId、EventId 与 observation revision；
- plan 自验证会重算全部 identity 并逐字段核对，不能以任意 GUID 冒充 deterministic plan；
- 同输入严格重放同一 plan；任一 seed、scope 或 projection revision 变化都会分离全部 8 个角色；
- plan 可直接提供给 P12.22，首次 dispatch 完成，第二次 exact replay 不重新进入 executor；
- foreign plan 无法别名已绑定 Host，在 executor 前由既有 Create/Host authority 拒绝；
- factory 不调用 `FGuid::NewGuid`，不保存 allocator、registry、Host、executor、sequence 或 retry 状态；
- focused plan `5/5`，ProductDispatch 父前缀 `10/10`，EffectCue 父前缀 `62/62`，0.0.10 全量 `628/628`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=24 / Logs=17`；
- regression gate self-test `208/208`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 最小 caller-owned 输入

`DispatchPlanSeed` 只包含：

- `DispatchSeed`：本次 caller-owned dispatch 根 identity；
- `VisualConsumerScopeId`：Visual 逻辑 consumer scope；
- `AudioConsumerScopeId`：Audio 逻辑 consumer scope。

三者必须有效，且双 consumer scope 必须不同。factory 不生成根 seed，也不持久化调用方 scope。

### 2.2 角色隔离的 canonical derivation

所有派生都使用同一 namespace：

`demo_map.Combat.SwordRhythm.EffectCueExecutionProductDispatchPlan.r1`

规范 parts 固定为：角色名、dispatch seed、Visual scope、Audio scope、RunId、ConfigId、CuePolicyId、EventId、observation revision。角色名分别为：

- `Host`；
- `Command.Create`；
- `Command.ProcessNext`；
- `Command.End`；
- `Consumer.Visual`；
- `Consumer.Audio`；
- `Attempt.Visual`；
- `Attempt.Audio`。

派生后要求 8 个 identity 全部有效且两两不同，并复用 P12.21 的固定 sequence `0/1/2`。底层 `FShanmenDeterministicId` 继续使用长度前缀 canonical parts，避免简单字符串拼接歧义。

### 2.3 重建证明与 P12.22 接入

plan 保存 frozen projection、seed 和派生 transaction identity。`IsValid()` 会从 projection/seed 重新派生 expected identity，再核对 sequence 与所有 GUID；`Matches()`、`MatchesProjection()` 和 `MatchesCurrentSession()` 提供 caller 可验证的 replay/current-state fence。

测试确认 plan identity 能被 P12.21 transaction factory 接受，并直接作为 P12.22 `TryDispatchCurrent` 输入。相同 plan 在 terminal Host 上走 exact replay；不同 seed 形成不同 Host/command/consumer/attempt identity，不能接管旧 Host。

## 3. 完整性

新增五个 focused Automation contract：

1. `DeterministicCapture`：同 projection/seed 两次 capture deep-match，8 个角色两两不同，P12.21 frozen request 也一致；
2. `SeedAndScopeDomainSeparation`：只改变 dispatch seed 或任一 consumer scope，8 个派生 identity 全部变化；
3. `ProjectionRevisionSeparation`：source Session 推进后，同 seed 对新 revision 生成全新 plan；旧 plan 不再匹配 current Session；
4. `FeedsCurrentDispatchReplay`：deterministic identity 首次通过 P12.22 完成，第二次 terminal replay 不增加 executor invocation；
5. `InvalidAndForeignPlanFence`：无效 projection、scope alias fail closed；foreign valid plan 对已绑定 Host 被拒绝且不重新进入 executor。

测试 projection 来自真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue` 链。fake executor 只返回既有 opaque receipt，不伪造 plan、projection、transaction、Host record 或 replay evidence。

## 4. 权威与兼容性边界

- P12.1—P12.22 的 ProductSession、ProductHost、ProductDispatch、transaction、GameMode 与 CombatRunCoordinator 均未修改；
- plan/factory 是普通 C++ value/stateless class，不是 UObject、Subsystem、World service、registry、allocator、worker 或 scheduler；
- caller 继续拥有 root seed、consumer scope、Host 与 executor；本层只做纯 deterministic projection；
- production 文件无 reflection、World/Actor、资产加载/播放、spawn、timer、async/thread、RNG、damage、attribute 或 gameplay-effect application；
- 本阶段不把 plan 自动安装到任何全局对象，也不尝试从 Host 反推 seed；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan.h/.cpp`：seed、immutable plan、capture result、canonical role derivation 与重建证明；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanTests.cpp`：五个真实 source-to-P12.22 contract；
- `ShanmenRegressionMap.json`：新 plan 路径映射到 24 组 deterministic、dispatch、Host 与 runtime authority 证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整日志正例与 focused-only 缺证据反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+846 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatchPlan` | 5 | 0 | 0 | `3C1F27E029B3E3B102761FBACD0F84BA4598ABB964725DAD22275C18431DFC05` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch` | 10 | 0 | 0 | `85C49BBFAEE559F9E0411D68A73FB3972AF7316AAEE98542F6E7E029FCFAEBA3` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction` | 5 | 0 | 0 | `B7A64F599F561A075C20E109DE76CCA1BBF76763F427B2520D55470FE831D26E` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `624FF6AF122609CF3C1C666AFAC5243072529B822FE745E6F203AE6D0C3494DF` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `2783C6F83EAF17BF2C50BA2280896B571743CF810123EAC75FA7E0E8CC3779CF` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `13AF2A57A14FD797ECC15AE5D1F0B9F7DB65D1FC07E623DB945F1B26B9F002D1` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `9407CC5D0CF4204B1B1C9EE738057E8703593FAC51ADDCA41460FA1EE18A13BB` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `DE309CE4B8B33290DACF93A213D6EFAB1148FE6D22429E103CE69F9C0BEABECC` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `243982E1D439BE090488A8DCE655B2599370922F9B882549BDB8DD5DBAD86A75` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `13D1119CF5F38590730F9B005124C1C754FA5968BBAAD4658990BABD27599172` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `85A01BDC3029F3B4DCFF3948FF24E7354D6404B2156206DC62BE0D755679130D` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `0C670DBA909CC07D8A04FEB19EFEEA58BEA3DB59D069B37F3FFA186A277114C7` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 62 | 0 | 0 | `D63BE16DFD91581AE485ECDCD261EB9C8FE8E972A3DFEF5E246C4DAA7B5995E5` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `2F5182BFCFD964DDE1C5A69DE2B25B3556605485A67B7BADC28F4EF760F00B59` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `F5BD91337D459742B1A787178E3AA621F67CA7D338D57268F12A416BFAB11092` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `0CBB798585CCBEF8990D8E26872221A0349B091F8AB3B3668824EB573883ACFD` |
| `Shanmen.0_0_10` | 628 | 0 | 0 | `94CC1D1E6A7260014323011F28B8719E3D9DD5CB4E408AD5ED85653BF4BC8612` |

ProductDispatch 父前缀包含自身 5 个 contract 与新 plan 的 5 个 child contract，因此为 `10/10`；EffectCue 父前缀由 `57` 增加至 `62`；全量由 `623` 增加至 `628`。十七份最终日志均有唯一 selected `RunTests` group、native terminal marker、Fail `0`；selected phase error/Fatal/Unhandled/Ensure 为 0。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=24 Logs=17
SELF_TEST: PASS 208/208
DETERMINISTIC_ID_SCAN: PASS NewGuid=0 CanonicalHashCalls=1 RoleDerivations=8
RUNTIME_BOUNDARY_SCAN: PASS World/Actor/assets/timer/async/thread/RNG absent
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 24.43s | 0 |
| Game final | Succeeded | 4 / 23.90s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.96s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,689,344 bytes，SHA-256 `2A23014CB1D30914E72BA8FAC1ED71239FE96A8720B6FF68BD3C5DF9412F5439`；
- `demo_map.exe`：355,239,424 bytes，SHA-256 `F913EE93B6A488012A659F787B65388962DF27B8F32817F154507ACB7959F817`。

三次有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 Editor 构建即成功；首次 focused Automation 即为 `5 Success / 0 Fail`、原生退出码 `0`，没有产品代码或测试修正轮。最终证据采用源码与回归映射封定后的 `P12.23-*-final.log`。

UE 启动阶段仍有 selected `RunTests` 之前的既有 13 条 automation condition diagnostics；selected phase error/Fatal/Unhandled/Ensure 为 0。跨平台 SDK 探测仍提示 LinuxArm64/VisionOS 缺少 `MainVersion`，但 Win64 SDK 明确为 VALID，且 17 个 Windows Automation 进程均原生退出 `0`。raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 deterministic value plan、注入 fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.24 增加 current planned-dispatch preparation seam：一次捕获当前 projection 与本阶段 plan，并为 P12.22 提供“已冻结 projection”的复用入口，消除 live caller 的重复 projection 读取；保留原 `TryDispatchCurrent` 兼容入口，不在新层保存 Session、Host、executor 或 retry 状态。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-23-sword-rhythm-cue-execution-dispatch-plan>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-23-sword-rhythm-cue-execution-dispatch-plan/Docs/Report/Dev.D.UE.0.0.10.P12.23.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-23-sword-rhythm-cue-execution-dispatch-plan/Docs/Log/Dev.D.UE.0.0.10.P12.23.r0_log.md>
