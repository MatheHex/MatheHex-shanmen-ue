# Dev.D.UE.0.0.10.P12.4.r0 Report

## 1. 结论

P12.4 已完成并通过 P 阶段门禁。

本阶段把 P12.3 的 immutable SwordRhythm presentation state 适配为窄、确定性、可重复轮询的表现事件。UI、动画蓝图或状态机可以通过 GameMode 读取 `Cue + tick window + revision + identity`，并用 `EventId` 在各自消费者内部去重；事件层不持有全局 cursor，不派发 delegate，不反写 Session，也不参与伤害或平衡数值计算。

最终结果：

- 新增 `Fdemo_mapShanmenSwordRhythmPresentationEvent` 与无状态 typed adapter；
- 新增 Started／PreciseLink／RestartedEarly／RestartedLate 四种表现 cue；
- 新增 GameMode `BlueprintPure` 只读事件查询；
- Event focused：`2/2`；Presentation 前缀 focused：`4/4`；
- 0.0.10 全量：`555/555`；四组旧版兼容回归：`116/116`；
- 7 份 Automation 日志原始合计 `677 Success / 0 Fail`，按 test identity 去重为 `671`；
- changed-file gate：`Changed=7 / Rules=2 / Required=45 / Logs=7`；
- regression gate self-test：`176/176`；
- `git diff --check`、JSON、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Immutable presentation event

新增 `Fdemo_mapShanmenSwordRhythmPresentationEvent`，冻结一个 presentation state 对应的展示事实：

- 确定性的 `EventId` 与源 `PresentationStateId`；
- Run、receipt、activation 与 canonical timeline identity；
- cue、observation revision、前后 input tick；
- transition offset、link window 与 timeline tick rate。

所有反射字段均为 private `VisibleAnywhere, BlueprintReadOnly`。`IsValid()` 会重算 `EventId`，验证 identity、revision、tick rate、窗口边界以及 cue 与 offset 的一致性；外部不能伪造一个 identity 与内容不一致的有效事件。

### 2.2 Stateless typed adapter

`Fdemo_mapShanmenSwordRhythmPresentationEventAdapter::Adapt()` 只读取 P12.3 state，并返回：

- `Adapted`；
- `StateInvalid`；
- `CueUnsupported`；
- `EventRejected`。

映射规则为：

- `Started` → `SequenceStarted`；
- `PreciseLinked` → `PreciseLink`；
- `RestartedEarly` → `SequenceRestartedEarly`；
- `RestartedLate` → `SequenceRestartedLate`。

同一 state 会派生同一 `EventId`。因此每个消费者可以独立记录自己的 last-seen identity，不需要共享可变 cursor，也不会让一个 UI 消费者吞掉另一个动画消费者的事件。

### 2.3 GameMode 只读出口

GameMode 新增：

```cpp
bool TryGetSwordRhythmPresentationEvent(
    Fdemo_mapShanmenSwordRhythmPresentationEvent& OutEvent) const;
```

该接口为 `BlueprintPure`。它复用 P12.3 的 state 查询，再执行无状态适配；Session 未激活、首个动作尚未接受或 state/event 无效时返回 `false`，并把输出重置为 canonical empty value。GameMode 不缓存、不派发、不确认消费，也不改写节奏权威。

## 3. 完整性

新增两条 focused tests：

1. `IdentityAndPolling`：使用真实 Run coordinator、30 Hz fixed timeline 与 ProductSession 生成首个 BasicSword receipt；验证事件完整字段、重复轮询 identity 稳定、typed invalid-state 拒绝、GameMode 空查询 fail closed，且动作不产生 damage；
2. `AllTransitionCues`：在 tick `0 / 1 / 9 / 22` 生成 Started、Early、Precise、Late 四种事件；验证 offset `none / 1 / 8 / 13`、revision `1..4`、事件 identity 唯一、无伤害，以及 Session teardown 后已复制事件仍保持 immutable valid。

changed-file regression map 新增 PresentationEvent 规则，并把 fixed timeline 与 GameMode 的依赖证据扩展到该只读消费者。Self-test 同时加入正向覆盖和“只有 Event/Presentation focused 仍不足”的负向 fail-closed 用例。

## 4. 兼容性与权威边界

- rhythm receipt、chain 与 state 的唯一权威仍是 P12.1–P12.3 Host/Session；事件只做投影；
- tick 仍来自 canonical Combat Run fixed timeline，没有 wall clock、frame counter 或第二套 timer；
- 没有 dispatcher、delegate、共享 cursor 或确认回写；
- 没有修改 BasicSword、Impact、Vitality、inventory、schema、GAS、输入或动画资产；
- event header/implementation 共 342 行，对 `AActor`、`UWorld`、`GetWorld`、Timer、wall clock、RNG、delegate、`ApplyDamage`、`TakeDamage`、`UGameplayStatics` 的扫描命中为 `0`；
- GameMode 本轮新增 24 行同一边界扫描命中为 `0`；
- event 生产代码对 damage 字段/API 的扫描命中为 `0`；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

不含本 Report/Log，共 7 个路径，`683 additions / 2 deletions`：

- 新增 presentation event header 与 implementation；
- 新增 2 条 PresentationEvent 自动化测试；
- 修改 GameMode 的 Blueprint 只读事件查询；
- 更新 changed-file regression map 与 self-test。

## 6. 测试覆盖

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmPresentationEvent` | 2 | 0 | `3952FAE31551E0BDCF3BCA1D0BC1D59DEC2018EEDD47C6D2AF9B371DB2D57AC5` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | `D971DC1086528382CCF44C7D8EE8E4B8D7D037602F00AD89702A6D8A2ED96F1A` |
| `Shanmen.0_0_10` | 555 | 0 | `32388AF2DE3A746F30EE12DF02961E7646E3B9B25ED7EBB0CEAD4BA9DFB3AD49` |
| `demo_map.V3.Attributes` | 4 | 0 | `2990711E253A7398313E39945C5842C362108B6DBE16002BC3EE4C8B83FC2ADB` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `5EF6BB9E6B544DF95E97F6D41F333BF4958408F09ED75E7F31711178AC8A32A4` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `781E8577F6CE1FEBF1AC20AD13EFA8683FF7C8C253242FE5F4E804F364D6BA15` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `98D7124CC8AB6E57393493E09587A94EDB8CB098A662BC20D4352B793765F5F6` |

Presentation 的 4 条包含其 Event 子组 2 条；两组 focused 均已包含在 full 555 内。原始日志合计 677，按 test identity 去重为 671。每份最终日志均有唯一 `RunTests` group、terminal queue-empty、原生退出码 `0`、Fail 0，且选定测试阶段 Error/Fatal/Unhandled/Assertion/Ensure 为 0。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=45 Logs=7
SELF_TEST: PASS 176/176
BOUNDARY_SCAN: PASS CorePaths=2 CoreLines=342 CoreForbiddenHits=0
BOUNDARY_SCAN: PASS GameModeAddedLines=24 GameModeAddedForbiddenHits=0
DAMAGE_SCAN: PASS CoreDamageHits=0
REGRESSION_MAP_JSON: PASS
git diff --check: PASS (native exit 0)
```

changed-file gate 日志 SHA-256：`2F066C2D39D11E44E5108BA5D0B54D55909EE8AB559BF28CD94C31039D481198`。Self-test 日志 SHA-256：`DFF268C304A19751D44840CCC89907DD5C564A632931254707A69EABDF5D4B6A`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor first | Succeeded | 25 / 121.13s | 0 | `EC25678129BD8852B45C165D888A16A39A02CEFF53224267315D4464609C001B` |
| Editor final | Succeeded | 0 / 0.91s | 0 | `BD448C22AD7ECB2C4AB744CEE78B7E9B8F28B7259D799B80A04BCA257E44A901` |
| Game final | Succeeded | 24 / 106.85s | 0 | `C715188D8D104C2F18067F36E91B5509F1A73D5387ABB1654EFE2F1C2132ED4B` |

最终产物：

- `UnrealEditor-demo_map.dll`：13,044,736 bytes，SHA-256 `9D2C983A017CE30CD323F6AA2EC3529F659316879DB1170941F6D12DE01CF55E`；
- `demo_map.exe`：354,576,384 bytes，SHA-256 `07F01424C4082B710EAE0259A14365436A61214B25036C294FFEC72E941A0682`。

构建没有源码失败，也没有 C3859、C1076、系统代码 1455 或其它内存／页面文件环境错误。

## 9. 流程与异常

- 首次 self-test 命令误调用 Windows PowerShell 5.1，既有脚本的 PowerShell 7 行首管道语法在解析阶段被拒绝；改用项目当前 PowerShell 7.6.4 后 `176/176`。该错误发生在产品测试之前，不是门禁用例失败；
- 首次 Event 测试启动使用 `-Log` 而非绝对日志参数，进程返回 0 但没有生成证据文件，因此没有计入任何测试结论；
- 随后的 7 组测试虽全部成功，但 `;Quit` 被拼进 `RunTests` group，changed-file gate 按 fail-closed 原则拒绝这些组名；改为由 `TestExit` 原生退出并覆盖重跑后，最终 7 份日志全部通过；
- UE 5.8 固定启动自测的 13 行 `LogAutomationTest: Error: Condition failed` 均发生在本轮 `Cmd: Automation RunTests` 之前。最终选定测试阶段错误标记为 0，该启动噪声未被删除或伪装。

本阶段没有产品源码失败、Automation case 失败或构建失败。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、只读事件适配、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

SwordRhythm 的 P 阶段展示交接面至此闭合：receipt → immutable state → deterministic event → BlueprintPure query。真实 montage、notify、音效、镜头、输入手感与窗口调优应在正式资产和 F 阶段验证；不建议再加入全局事件 cursor 或把 cue 转换为伤害。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-4-sword-rhythm-presentation-event>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-4-sword-rhythm-presentation-event/Docs/Report/Dev.D.UE.0.0.10.P12.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-4-sword-rhythm-presentation-event/Docs/Log/Dev.D.UE.0.0.10.P12.4.r0_log.md>
