# Dev.D.UE.0.0.10.P15.5.r0 Report

## 1. 结论

P15.5 完成 legacy `demo_map.InputRestore` 三项陈旧断言的有界收敛：保留热栏与普攻的真实 gameplay gate，删除对不存在的 0.0.5 `Saved` 生成脚本的机器本地依赖，并把终端标记契约改为直接执行当前统一 emitter。

`demo_map.InputRestore` 从 `98 Success / 3 Fail / 101 Total` 收敛为 `101 Success / 0 Fail / 101 Total`。同命令 legacy `demo_map` 父组从 P15.4 的 `1318 Success / 12 Fail / 1330 Total` 变为 `1321 Success / 9 Fail / 1330 Total`，精确消除这三项失败，没有新增或隐藏其它失败。

本阶段 production C++ 零修改；没有放宽输入门禁、终端记录、幂等或失败关闭约束。

## 2. 根因

### 2.1 HotbarAndCombatGatePreserved

旧测试只在整个 `demo_mapPlayerController.cpp` 中寻找：

```text
UseHotbarSlot(Intent, IsGameplayInputAllowed())
```

当前实现已经把 gate 内聚到产品入口：

- `UseHotbarSlot` 在投递飞行暗器或普通快捷栏前检查 `!IsGameplayInputAllowed()`；
- `StartBasicAttack` 在确认地面技能或调用普攻前检查同一 gate；
- `TryBasicAttack` 自身再次在写入冷却前独立检查 gate。

产品约束比旧调用点传布尔值的形式更强；失败来自精确字符串已过时，而非输入门禁丢失。

### 2.2 两个 terminal plan 断言

case 100/101 都无条件读取：

```text
Saved/Automation/Dev.D.UE.0.0.5.P8.18.r0/Tools/GenerateP818Plans.ps1
```

该文件不在工作区、不在 git 历史中，也不是 0.0.10 的版本化输入。测试随后检查旧任务号、旧 slot 数量和 PowerShell 文本。结果依赖某台机器遗留的 `Saved` 工件，无法在干净 checkout 中确定性重放。

当前真实契约已经由 `Fdemo_mapInputRestoreTerminalStateEmitter` 统一拥有，应直接验证其输出，而不是验证历史生成器源码。

## 3. 实现

### 3.1 门禁顺序契约

新增测试侧函数块读取器，将检查范围限制在明确的产品函数体内。case 26 现在验证：

- hotbar gate 先于 thrown-weapon route 与 quick-slot route；
- bound attack gate 先于 ground-circle confirm 与 `TryBasicAttack`；
- direct basic attack gate 先于 `BasicAttackReadyTime` 状态写入。

这仍是静态边界证据，但不再依赖已删除的调用签名，也不会被文件其它位置的同名文本误满足。

### 3.2 可执行 terminal 契约

- case 100 使用两个独立 emitter 实例和同一组真实 transient World/Controller/Character 输入；两端都必须成功产生完全相同的 canonical line，并保留专用 marker、phase 与 boundary；
- case 101 直接产生一条 terminal line，要求以 `INPUT_RESTORE_TERMINAL_STATE ` 开头，且不依赖或夹带通用 `INPUT_CONTEXT_TRANSITION` marker；
- 两项都不再读取 `Saved`、旧任务号、旧 slot count 或机器本地脚本。

## 4. 回归覆盖门禁

`demo_mapInputRestoreTests.cpp` 从 `UnifiedInputRegistryAndBindings` 的多系统映射中拆出 exact mapping：

```text
Source/demo_map/demo_mapInputRestoreTests.cpp
  -> demo_map.InputRestore
```

输入注册表、绑定设置和其它物理输入 fixture 仍保留原来的 broad mapping。新映射只避免“单独修改 InputRestore legacy fixture，却被要求提供无关新系统 suite”的过度覆盖。

流程自检新增一正一反：

- `demo_map.InputRestore` 父组日志可覆盖该 fixture；
- 无关 `Shanmen.0_0_10` 全量日志不能替代 legacy InputRestore suite。

最终流程证据：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 256/256
JSON_PARSE: PASS Rules=153
git diff --check: PASS
```

## 5. Automation 证据

| Evidence | Success | Fail | Total | Status | SHA-256 |
|---|---:|---:|---:|---:|---|
| InputRestore baseline | 98 | 3 | 101 | 0 | `2083E1DD36B42295C6F03531D5FECD3A628488C76DA72D179D479EBA0C161BEC` |
| InputRestore after | 101 | 0 | 101 | 0 | `416FC5A5F640318E626AE093BE5226EB4676B11DD1337552A15F5F4F0A543D11` |
| legacy `demo_map` after | 1321 | 9 | 1330 | 0 | `C67CCCA31E0EC62C582CB6B7F29DDCA9615A35C679C05B386DF0D83D805B5453` |
| `Shanmen.0_0_10` full | 712 | 0 | 712 | 0 | `F3B4C392B57A03C2120090E8DB04636545339DBC6ED0D21AF77BC6FCD50F9FFA` |

四份日志都有 queue-empty terminal evidence，且 Fatal error、Unhandled Exception、Ensure condition failed 为 0。legacy 父组 status `0` 不被当作全绿；仍以逐项结果判定其 9 项失败。

0.0.10 全量首末 Success 时间为 `17:58:15.387 -> 18:28:41.265`，约 `30m25.88s`，随后出现 `712 tests performed` queue-empty terminal evidence。

legacy 剩余失败保持可见：AutomationRootBoundary 1、EnemyRouteLoot 1、P1 SectNavigation 1、P5RuntimeInterface 1、P6 4、V3 Lifecycle 1。

## 6. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Status | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 4 / 24.51s | 0 | `1C36FC973FC442497ED3DC600340E96D3C1A6F2634D78C74AA085DFCB83ABDA0` |
| Game final | Succeeded | 3 / 23.72s | 0 | `1A4910E6548417E188652E2E71DBE94E615AA9CC931521FFF27F71F133B5C01C` |
| Editor final | Succeeded, up to date | 0 / 0.94s | 0 | `019B490E2DF87E15469D94D5B4AE86CEE3B388FDDD7A89BD6EA56210590C9B80` |

最终产物：

- `demo_map.exe`：355,744,768 bytes，SHA-256 `4B6B5232689DFEEC910DDBB01D49304EF24DF93153782770E9D6CB7DA3A9B1D5`；
- `UnrealEditor-demo_map.dll`：14,330,368 bytes，SHA-256 `5565C90671A222D7750783BFB0719FC8085A5987966CC44F772ED1102C830230`。

## 7. 修改范围

- `demo_mapInputRestoreTests.cpp`：替换 3 个陈旧断言，新增函数块局部顺序检查；
- `ShanmenRegressionMap.json`：拆出 exact InputRestore fixture mapping；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 exact mapping 正反自检；
- Report/Log；
- production C++ 零修改，raw logs 仅本地保存；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 8. P/F 边界

本 Report 只包含 P 阶段 legacy test contract 修复、NullRHI 无头 Automation、静态审查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

本轮没有把缺失的旧 `Saved` 脚本伪造回工作区，也没有为了通过旧字符串断言改写产品 gate。

## 9. 下一步

P15.6 可优先审查剩余单项 `demo_map.AutomationRootBoundary.26.RedirectedRuntimeProductionContextIgnoredAsProtectedFact`，区分当前 workspace/root 安全契约与历史路径文本，再用同样的“生产约束不降级、fixture 证据可重放”原则收敛。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-5-input-restore-contract-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-5-input-restore-contract-regression/Docs/Report/Dev.D.UE.0.0.10.P15.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-5-input-restore-contract-regression/Docs/Log/Dev.D.UE.0.0.10.P15.5.r0_log.md>
