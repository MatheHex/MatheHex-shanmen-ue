# Dev.D.UE.0.0.10.P12.3.r0 Report

## 1. 结论

P12.3 已完成并通过 P 阶段门禁。

本阶段把 P12.2 的 immutable SwordRhythm receipt 投影为正式、只读、自校验的表现态，并通过 ProductSession 原子提交给 GameMode 查询。UI、动画或状态机现在可以读取同一 Combat Run 的 band、tick、chain count、配置版本与确定性 identity，但不能反写节奏权威，也没有新增输入、动画命令、计时器或伤害路径。

最终结果：

- 新增 `Fdemo_mapShanmenSwordRhythmPresentationState` 与无状态 projector；
- 新增 GameMode `BlueprintPure` 只读查询，首个已接受动作前 fail closed；
- SwordRhythmPresentation focused：`2/2`；
- SwordRhythmProductSession focused：`2/2`；
- 0.0.10 全量：`553/553`；
- 7 份 Automation 日志原始合计 `673 Success / 0 Fail`，按 test identity 去重为 `669`；
- changed-file gate：`Changed=9 / Rules=3 / Required=44 / Logs=7`；
- regression gate self-test：`174/174`；
- `git diff --check`、JSON、静态边界扫描、Editor/Game Development 单并发构建全部通过。

本阶段没有把 rhythm band、chain count 或 presentation state 接入 damage、attack power、resource multiplier 或其它平衡数值。

## 2. 功能性

### 2.1 Immutable presentation state

新增 `Fdemo_mapShanmenSwordRhythmPresentationState`，只保存一个已接受 receipt 的展示事实：

- 确定性的 `PresentationStateId`；
- Run、config、receipt、activation 与 canonical timeline identity；
- content version/digest、style definition 与 rule id；
- link window、tick rate、前后 input tick；
- observation revision、前后 chain count 与 rhythm band。

所有字段均为 private `VisibleAnywhere, BlueprintReadOnly`；外部只能复制和读取。`IsValid()` 会重算 timeline 与 presentation identity，并验证 Started／PreciseLinked／RestartedEarly／RestartedLate 的窗口和 count 不变量。PreciseLinked 的 `int32` 上界也 fail closed，不允许 chain count 溢出。

### 2.2 Stateless projector 与 identity fence

`Fdemo_mapShanmenSwordRhythmPresentationProjector::Project()` 是纯投影，不持有 World 或 mutable state。它按顺序拒绝：

- invalid product config；
- invalid RunId；
- invalid receipt；
- 非正 observation revision；
- config、receipt、Run 或 canonical timeline identity 不一致；
- 自校验失败的最终候选态。

结果使用 typed status：`Projected / ConfigInvalid / RunInvalid / ReceiptInvalid / RevisionInvalid / IdentityMismatch / ProjectionRejected`。同一 receipt 与 revision 会确定性地产生同一状态 identity。

### 2.3 ProductSession 原子提交

P12.2 Session 现在同时管理 Host、latest receipt 与 latest presentation state：

1. 先在 Host 副本上接受动作；
2. 从候选 receipt 与候选 observation count 投影表现态；
3. 只有 Host、receipt、presentation 三者共同通过 Session 校验后才整体提交；
4. exact replay 保持 receipt、presentation identity、revision 与 observation count 不变；
5. Run teardown 同时清除 config、Host、receipt 与 presentation state。

因此投影失败不会留下“Host 已推进、表现态未推进”的半提交状态。

### 2.4 GameMode 只读出口

GameMode 新增：

```cpp
bool TryGetSwordRhythmPresentationState(
    Fdemo_mapShanmenSwordRhythmPresentationState& OutState) const;
```

该接口为 `BlueprintPure`。Session 未激活、尚无已接受动作或状态无效时返回 `false`，并把输出重置为 canonical empty value；成功时只复制最新 immutable state。成功观察日志增加 `PresentationStateId` 与 `Revision`，便于审计 receipt 与展示态的一一对应。

## 3. 完整性

新增两个 focused tests：

1. `ProjectionAndIdentityFence`：使用真实 Coordinator、30 Hz Run timeline 与 Session 生成首个 BasicSword receipt；验证完整字段冻结、确定性重投影、typed malformed/cross-Run 拒绝，以及动作没有产生 damage；
2. `SessionLifecycleReadModel`：验证首动作前空态、首动作 `Started / revision 1`、exact replay 幂等、tick 8 的第二动作 `PreciseLinked / revision 2`、GameMode 空查询和 Run teardown 清理。

changed-file regression map 新增 Presentation 规则，并把 timeline、Session 与 GameMode 的依赖证据扩展到新的只读 consumer。Self-test 增加正向覆盖和“只有 Presentation/Session focused 证据仍不足”的负向 fail-closed 用例。

## 4. 兼容性与权威边界

- receipt 与 chain 的唯一权威仍是 P12.1/P12.2 Host/Session；表现态只做投影；
- tick 仍来自现有 canonical Run timeline，没有 wall clock、frame counter 或第二套 timer；
- GameMode 不重新计算 band 或 chain count，只读取 Session 已校验结果；
- 没有建立第二套 BasicSword、Impact、Vitality、inventory 或资源事务；
- 没有修改 schema、GAS ability、动画资产或输入绑定；
- presentation 与 Session 4 个生产文件共 791 行，对 `AActor`、`UWorld`、`GetWorld`、Timer、wall clock、RNG、`ApplyDamage`、`TakeDamage`、`UGameplayStatics` 的扫描命中为 `0`；
- GameMode 本轮新增 24 行同一边界扫描命中为 `0`；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

不含本 Report/Log，共 9 个路径，`779 additions / 8 deletions`：

- 新增 presentation state/projector header 与 implementation；
- 新增 2 条 Presentation 自动化测试；
- 修改 SwordRhythm ProductSession 的原子观察、状态校验与 teardown；
- 修改 GameMode 的成功日志和 Blueprint 只读查询；
- 更新 changed-file regression map 与 self-test。

## 6. 测试覆盖

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 2 | 0 | `9BD1B173B6E5FAA702E59D783EAE44BB023F5355ACB4AA8ED8A2CF67EACF5229` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 2 | 0 | `CE23BFFEDE9062FC4467C6CB88E784FFC9DCB7223CCE7F3086528B3B29CD1D10` |
| `Shanmen.0_0_10` | 553 | 0 | `76917A03FE8748AFA44D163841672A595C5FF9DFDF4457444E780D8C13B7953A` |
| `demo_map.V3.Attributes` | 4 | 0 | `77A77EC385ADB6303903DFC5F9479AE1D46E8D9B76F37C360C023AB10FCDF0D3` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `FD9C2CE9D19FFEEBD9B5DA2147CC34CC024BDB8BDB0D23662E2FC1F53CC6E730` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `6F03D211D294679D7BE811820154A113B36A9870B5C5EC6F8A444F4AF046D1B7` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `0578B823DF2E0C8A3C0E173938ECED9D12A7BB25C524A3CFAAE4AEBFBA62AADE` |

Focused 4 条已包含在 full 553 条内；原始日志合计为 673，按 test identity 去重为 669。每份日志均有 terminal completion、原生退出码 `0`、Fail 0，且在选定 `RunTests` 命令之后没有 Error/Fatal/Unhandled/Assertion/Ensure 标记。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=44 Logs=7
SELF_TEST: PASS 174/174
BOUNDARY_SCAN: PASS CorePaths=4 CoreLines=791 CoreForbiddenHits=0
BOUNDARY_SCAN: PASS GameModeAddedLines=24 GameModeAddedForbiddenHits=0
REGRESSION_MAP_JSON: PASS
git diff --check: PASS (native exit 0)
```

changed-file gate 日志 SHA-256：`0838C9A28B363C062078C2727C7EEA4DC4D9A04D5D5592C658ABAB82D908A031`。Self-test 日志 SHA-256：`CACA74D299C199EBED1614BD627F7F68AC3530CEB890B6C117A6F63EA25EB711`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor first | Succeeded | 26 / 120.77s | 0 | `00C94738592CA8FD9A026129B85EEFEAC846CC9CCB7093991DFE27B419CCA39B` |
| Editor final | Succeeded | 5 / 7.39s | 0 | `0AA9AB33FE1C1AF334103E60A7C12700B261E3033BE74B9973525ED5B458378B` |
| Game final | Succeeded | 25 / 101.50s | 0 | `335244AFFBCB924E0C351C39AFF15EC780678EEFB4838441515E4F5D12987BAD` |

最终产物：

- `UnrealEditor-demo_map.dll`：13,015,552 bytes，SHA-256 `039E9B34C370957B03A0E298FDD0D7271F5F807F2CF668A824865376CE768800`；
- `demo_map.exe`：354,553,856 bytes，SHA-256 `8D05361CCB42CCA4FBCEEAB0934EBF1508390707AC6D6945622A5F46355BF691`。

Editor 首次与最终增量构建、Game 构建均成功，没有源码失败，也没有 C3859、C1076、系统代码 1455 或其它内存／页面文件环境错误。

## 9. 流程与异常

第一次尝试用动态 PowerShell 循环连续启动 7 组 Automation 时，命令创建阶段被本机策略拒绝，Unreal 未启动，也没有测试失败。随后改为每组固定参数、顺序执行，7 组均原生退出 `0`。

UE 5.8 在 Engine 初始化前固定输出 UnifiedError 自测信息和 13 行 `LogAutomationTest: Error: Condition failed`；该噪声在每个独立进程中相同，发生于本轮 `Automation RunTests` 命令之前。选定测试阶段错误标记为 `0`，且所有结果、terminal completion 与进程退出码均成功。该启动噪声未被删除或伪装。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、只读表现态、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

建议 P12.4 在该 immutable read model 之上建立一个窄的 animation/state consumer adapter：只把 `Band + ticks + revision` 转为表现事件，继续禁止反写 Session 或转换为伤害。真实 montage marker、输入手感与窗口调优留到正式资产和 F 阶段运行验证。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-3-sword-rhythm-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-3-sword-rhythm-presentation/Docs/Report/Dev.D.UE.0.0.10.P12.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-3-sword-rhythm-presentation/Docs/Log/Dev.D.UE.0.0.10.P12.3.r0_log.md>
