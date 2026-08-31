# Dev.D.UE.0.0.10.P12.0.r0 Report

## 1. 结论

P12.0 已完成并通过 P 阶段门禁。

本阶段建立了太极剑节奏的第一份真实核心：连续 BasicSword 输入现在可以在调用方拥有的确定性时间线上形成 `Started / PreciseLinked / RestartedEarly / RestartedLate` 四类互斥回执。窗口宽度由内容注入，精准区间为半开 `[open, close)`；太早或太晚不会偷偷累计，而是从当前动作重新开始一层。

最终结果：

- focused SwordRhythm：`6/6`；
- broad CombatRuntime：`111/111`；
- 0.0.10 全量：`547/547`；
- 五份 Automation 日志原始合计 `669 Success / 0 Fail`，按 test identity 去重为 `547`；
- changed-file gate：`Changed=3 / Rules=2 / Required=4 / Logs=4`；
- regression gate self-test：`168/168`；
- `git diff --check`、静态边界扫描、Editor/Game Development 构建全部通过。

## 2. 功能性

### 2.1 内容拥有节奏窗口

`FShanmenSwordRhythmDefinition` 冻结：

- canonical style：`Combat.Style.Sword.Taiji01`；
- canonical action：`Combat.Action.Sword.Basic01`；
- 内容规则 ID；
- `LinkOpenOffsetTicks` 与 `LinkCloseOffsetTicks`。

定义要求 `open >= 0` 且 `close > open`，并从全部字段派生 deterministic `DefinitionId`。运行时没有固定平衡数值，也没有自行读取时间。

### 2.2 确定性观察与回执

每次观察冻结完整 action snapshot、caller-owned `TimelineId` 与 `InputTick`。`ObservationId` 覆盖 Run、Owner、Activation、Source、weapon instance、action、content stamp、timeline 和 tick。

首个观察返回 `Started / 1`。后续观察相对当前锚点分类：

- `delta < open`：`RestartedEarly / 1`；
- `open <= delta < close`：`PreciseLinked / previous + 1`；
- `delta >= close`：`RestartedLate / 1`。

早／晚输入都会成为下一次判定的新锚点，因此规则表达的是可恢复节奏，而不是不可审计的输入丢弃。

### 2.3 幂等与原子拒绝

Chain 以 ActivationId 保存 immutable receipt：

- exact observation replay 返回同一 `ReceiptId`，不重复增长；
- 同一 ActivationId 配不同 tick 失败关闭；
- 跨 Run、Owner、Source、weapon、content、source tags 或 timeline 失败关闭；
- tick 回退失败关闭；
- 所有拒绝都清空输出且不改变最后锚点、当前层数或已记录回执。

## 3. 完整性

新增六个 Automation 契约：

1. canonical definition、非法窗口、负 tick 与非 BasicSword action；
2. `[open, close)` 两端精确边界；
3. early restart 成为新的合法锚点；
4. exact replay 幂等与 same-activation conflict 原子拒绝；
5. timeline、weapon、Run、content 与 tick regression 隔离；
6. Definition/Receipt deterministic identity、等价历史重放与 Reset。

Regression 映射同时要求 SwordRhythm、BasicSword、ActionLifecycle 和 broad CombatRuntime 证据，并新增一正一负 self-test；自测由 `166` 增至 `168`。

## 4. 兼容性与权威边界

- BasicSword 仍是唯一被支持的动作定义，没有建立第二套 sword action；
- action snapshot、SourceItemInstanceId 与 content stamp 均来自既有 CombatCore；
- timeline 由未来产品层拥有，本核心不创建 Tick、Timer 或 wall clock；
- 没有修改伤害、Impact、Vitality、inventory、schema 或资源事务；
- 没有把链层数接成 damage multiplier、攻速或其它未定数值；
- 没有接入 GameMode、PlayerController、Enhanced Input、GAS ability 或动画通知；
- 新增源码未引用 `demo_map`、`UWorld`、`AActor`、`GetWorld`、Timer、RNG、`ApplyDamage` 或 `TakeDamage`；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

生产代码、测试与门禁共 5 个文件，新增 `1,052` 行（不含本 Report/Log 与原始证据）：

- `Source/ShanmenCombatRuntime/Public/ShanmenSwordRhythm.h`：203 行；
- `Source/ShanmenCombatRuntime/Private/ShanmenSwordRhythm.cpp`：453 行；
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSwordRhythmTests.cpp`：366 行；
- `Scripts/ShanmenRegressionMap.json`：新增 9 行；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 21 行。

## 6. 测试覆盖

| Group | Success | Fail | Queue | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.SwordRhythm` | 6 | 0 | 1 | `8647AE769722CC2D5F08CDB254748D1CCFAC43D5D1C4E1F46CCA8F66351D4CF3` |
| `Shanmen.0_0_10.CombatRuntime.BasicSword` | 4 | 0 | 1 | `71208E263EEEFC9703388D5638EC8AE7F8A605B5E3EA065232638994204AE3C3` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | 1 | `BFC66642D69933155D78D66634548B443DC10A3DB9FBC9BB0096DEC77623B65F` |
| `Shanmen.0_0_10.CombatRuntime` | 111 | 0 | 1 | `DC70A3FEE9A8280C749481E6A264EC670AFA174FFFBD6991766975AD4E9470DA` |
| `Shanmen.0_0_10` | 547 | 0 | 1 | `1193A09EA18E04DF024A67D0B12F6D08962DDDD405B47775308F845663BED5CF` |

每份最终日志均包含一个 canonical `Automation RunTests <group>` 命令、一个原生 queue-empty / TEST COMPLETE 终止事实、Fail 0，且无 Fatal、Unhandled Exception 或 Ensure。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=4 Logs=4
SELF_TEST: PASS 168/168
STATIC_REVIEW: PASS AddedSourceLines=1022 ForbiddenHits=0
git diff --check: PASS (native exit 0)
```

最终 changed-file gate 日志 SHA-256：`0D1E58D875A0D4FE7AC745BCEB04AC2B45AD2C27E0E0EBA2D3C59CDB11612F2F`。Self-test 日志 SHA-256：`015BBBFC38CA28CACFE378E7684385DD369C9AD2D6B12B599971E9B85F47A68B`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor | Succeeded | 8 / 37.89s | 0 | `6291D4154D0DDC80BA8673D6FD75BB5158723D3C70C46FC964145CDF7B3E75A5` |
| Game | Succeeded | 5 / 33.23s | 0 | `FF31072E84BFE2175294243058C029580E453986D950E0C02785645DF12DBF31` |

最终产物：

- `UnrealEditor-ShanmenCombatRuntime.dll`：1,554,944 bytes，SHA-256 `39A6ECBD65929FA660C02DC1886F119712CD60E0ED8320619F97EB4AE57BF6DF`；
- `UnrealEditor-demo_map.dll`：12,935,168 bytes，SHA-256 `7E1E8F5C3B0B1893FCC98D4E30D12DE38A56897A97A49B405026C1E9D364B848`；
- `demo_map.exe`：354,486,272 bytes，SHA-256 `605B78C8C8E45BC43634CD1E3E833C7B4E1C1E664BDC2D0192D3330DDE025302`。

## 9. 真实异常与修复

产品源码、Automation、Editor 与 Game 构建均为首次成功，未发生源码或环境失败。

证据门禁有两次命令封装错误：首次使用 `pwsh -File` 传递数组导致位置参数绑定错误；第二次 gate 正确拒绝了带 `;Quit` 后缀的非规范 RunTests group。随后删除后缀，以标准 RunTests + queue-empty TestExit 重跑全部五份日志，最终 gate 通过。该异常没有修改产品语义，也没有被描述为测试失败。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、纯确定性实现、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P12.1 应把现有 Run-bound 30 Hz fixed timeline 从 WeaponGuard 专名抽成共享产品时间源，并由真实 BasicSword 成功启动／接受事实生成 observation；仍不应在缺少动画与设计参数前把 chain count 接成数值增伤。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-0-sword-rhythm-core>
