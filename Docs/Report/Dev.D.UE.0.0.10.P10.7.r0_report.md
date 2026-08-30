# Dev.D.UE.0.0.10.P10.7.r0 Report

## 1. 结论

P10.7 **PASS**。本阶段把 P10.6 Spirit Evasion 组件接入现有玩家运行时：GameMode 初始化时幂等确保玩家 Character 上恰好一个组件；新增无状态 typed command router，接受外部已经冻结的 start payload，或 payload-free 的 cancel、interrupt、finish-recovery 信号。Router 不生成 GUID、不选择 content/policy、不维护资源或 replay ledger，也不绑定输入。

Start 只有在 action 的 RunId、SourceEntityId 和真实 owner 均与当前 `Fdemo_mapCombatRunCoordinator` 注册结果一致时才进入组件。GameMode 释放 combat Run 前会先中断尚未终止的 Spirit Evasion；新 Run 激活会拒绝遗留的非终态 Host，避免运动跨 Run 泄漏。

最终验证为 focused `7/7`、0.0.10 全量 `430/430`、EnemySkillFramework `44/44`、V2RangedCompatibility `22/22`，四份正式日志合计 `503` 条 Success、`0` Fail。changed-file gate、128/128 mapping self-test、静态边界扫描、`git diff --check`、Editor Development 和 Game Development 均通过。

## 2. 功能性

### 2.1 唯一安装

`Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled` 只接受有效 `ACharacter`：

- 0 个组件：创建 `RuntimeShanmenSpiritEvasion`、加入 owner instance components 并注册；
- 1 个组件：复用，必要时注册；
- 多于 1 个组件：不选择任意实例，fail closed；
- 创建或注册失败：返回 typed installation status，不发布半安装结果。

GameMode 的常规 mission 初始化把该结果纳入 readiness 条件，因此正常玩家运行时不会静默缺少组件。

### 2.2 冻结命令与 authority fence

Start command 只保存外部传入的：

- `FShanmenCombatActionSnapshot`；
- `FShanmenSpiritEvasionDefinition`；
- movement policy snapshot；
- trajectory snapshot；
- 归一化 planar candidate direction。

捕获时要求 action/definition 为 canonical Spirit Evasion，所有 snapshot 有效且方向有限非零。路由时进一步要求 active Coordinator、相同 RunId、相同 player entity，以及 owner object 在 World Entity Registry 中解析为该 SourceEntityId。Router 不接受调用方注入另一个实体，也不把输入事件变成身份。

Cancel、Interrupt、FinishRecovery 不携带 start payload；它们在 Coordinator 已关闭后仍可用于安全收口当前 Host。

### 2.3 Run 生命周期接线

`Ademo_mapGameMode` 新增 `RouteSpiritEvasionCommand`，只寻找/确保玩家组件并把 typed command、Coordinator 与真实 Character 交给 Router。Run 释放前：

1. 检查组件重复；
2. 未启动或已终态则直接通过；
3. Active/Recovery Host 通过 typed Interrupt 终止；
4. 中断失败则阻止 Run release，不留下跨 Run 活动状态。

新 Run 激活也会拒绝任一已有但非终态 Spirit Evasion Host。

## 3. 完整性与兼容性

- 复用 P10.6 Component 及其唯一 P10.5 ProductHost；
- 复用 CombatRunCoordinator 和 World Entity Registry，不建立第二个 Run/实体映射；
- start 的 action/content/policy/trajectory 仍由未来上游 authority 冻结；
- 不修改 P10.0—P10.6 数学、运动 scheduler、碰撞 preflight 或 swept mutation；
- 不修改 PlayerController，不占用按键，不改变现有输入消费顺序；
- 不读取/写入 SpiritEnergy，不修改 item/profile/schema/CodeB；
- 不引入 command replay map、active bit、action phase 或 motion state；
- 旧 enemy/V2 displacement 与 0.0.10 全量回归通过。

本阶段形成了可调用的产品命令边界，但没有提供默认 Spirit Evasion content、Run-owned activation sequence 或真实按键入口；因此不能把“已接线”误报为“玩家已可通过真实输入施放”。

## 4. 关键不变量

1. 一个玩家 Character 最多一个 Spirit Evasion Component；
2. 重复组件必须 fail closed；
3. Router 不生成 action/content/command 身份；
4. Start 必须引用当前 active Run；
5. Start source 必须是 Coordinator 注册的玩家实体；
6. routed component 必须属于传入 owner；
7. control signal 不复制 start payload；
8. busy Component 拒绝第二个 Start，且不执行第二次 preflight；
9. Router 不拥有 ledger 或生命周期状态；
10. Run release 前必须终止非终态 Host；
11. 新 Run 不得接纳遗留的非终态 Host；
12. 所有运动 mutation 继续只存在于 P10.3 shared swept authority。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionCommandRouter` 七个测试：

- `FrozenCommandCapture`：冻结 payload、方向归一化、identity 保留、空方向拒绝、control 无 payload；
- `UniqueInstallation`：null owner、首次安装、重复 ensure 复用、双组件拒绝；
- `RunAndSourceFences`：Coordinator 未就绪、foreign Run、foreign source 全部在 preflight 前拒绝；
- `StartAndBusyFence`：成功启动、busy replay 不重复 preflight、不覆盖 HostId；
- `CancelAfterRunClose`：Coordinator reset 后仍可安全 cancel；
- `InterruptAndOwnerFence`：interrupt 委托与 foreign owner 拒绝；
- `FinishRecovery`：分段完成进入 Recovery，再通过 typed command 完成 action。

0.0.10 全量由 P10.6 的 `423` 增至 `430`。

## 6. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenSpiritEvasionCommandRouter.h`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionCommandRouter.cpp`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionCommandRouterTests.cpp`。

更新：

- `Source/demo_map/demo_mapGameMode.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverage.ps1`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

产品、测试和流程本体共 `8` 个文件、`1246` insertions、`6` deletions；加入本 Report 与同名 Log 后 exact stage 为 `10` 个文件。长期未跟踪的 0.0.9B Prompt/Report 和用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.7-SpiritEvasionCommandRouter-final.log` | focused router/install | 7 | 0 | `2FDDA039E9957E9241606CA9FD0D9B7C5219F3EB28E0BF58D358F7E9B7AC0E12` |
| `P10.7-Shanmen-0_0_10-final.log` | 0.0.10 full | 430 | 0 | `465E8030F8EF94F63FFE9AF1A0FACE0CCBA1C691AD9187FBADC3E3452017A528` |
| `P10.7-EnemySkillFramework-final.log` | enemy skills | 44 | 0 | `A061DF8A7283AF74BCA97916E7A3096F1F359E0C4C0E7B8E0C999DECC2F960C8` |
| `P10.7-V2RangedCompatibility-final.log` | V2 displacement | 22 | 0 | `054B7CC55545A44D8CC49CDCE111C4D61E3D68A530EDFDED41DD416E27EF4996` |

所有正式进程原生退出码均为 `0`，每份日志都有 UE 5.8 native `TEST COMPLETE / EXIT CODE 0`，且没有 selected fail、fatal、unhandled 或 ensure。父组 `Shanmen.0_0_10` 直接覆盖 21 个同前缀 required groups，避免为同一 430-test 队列重复启动 21 次 Editor；focused 与两个 legacy 根组仍单独保留。

```text
REGRESSION_MAP_JSON: PASS Rules=84
SELF_TEST: PASS 128/128
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=24 Logs=4
git diff --check: PASS
DIRECT_MUTATION_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
INPUT_BINDING_HITS=0
ROUTER_STATE_FIELDS=0
GAME_MODE_INSTALL_CALLS=3
RUN_RELEASE_ROUTES=3
```

mapping SHA-256：`B113565B9C5DF6AFC5E8CDCB15B3FC0C9E494EF5870EB6155485F04B0F1BF8E9`；self-test SHA-256：`124357A32F145D04B4497C7318C87B46863CF6A67321585EDB26F7C121C4C8DF`。

UE 5.8 将旧 `Automation Test Queue Empty N tests performed` 改为 `**** TEST COMPLETE. EXIT CODE: 0 ****`。验证脚本现在只把这两个 native terminal-success 标记视为等价，并新增专门自测；fail/fatal/ensure 与成功计数检查未放宽。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate (initial route) | Succeeded | 24 / 122.96s | 0 | `63395EDE5FB70EB9BBE914AA16EEF8F549973EA868B5C2B2B1E06EE41EE15AEF` |
| Editor candidate (Run-release addition) | Succeeded | 22 / 76.68s | 0 | `E23569A67DC042B8814B88283338EE47FE44A27798D0E6CE219E0F125268E337` |
| Editor final | Succeeded, up to date | 0 / 0.89s | 0 | `37405834F1C95BDAA7E1C433C11FB4A318B0CB059F41EFC2E341BC421A4064BF` |
| Game final | Succeeded | 23 / 106.48s | 0 | `E39E8FFEEBAC28AE09B7B3A594C6CDD22C1593A6A082A01544D5D72800FB8BC8` |

最终 `UnrealEditor-demo_map.dll`：`12392960` bytes / SHA-256 `62FB6D404C1368BBAFF5FEA140EDE7251A627A8570AE3594093F67EA3F4E33B0`；`demo_map.exe`：`353851392` bytes / SHA-256 `D5D1352CFE4400863C69F7628EC96C878D2AD399D7161BF374B76971734D675C`。

## 9. 真实异常

没有源码编译失败、Automation failure、changed-file gate failure、C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。初始实现通过后，代码审查发现 combat Run release 也必须收口活动 Host，因此增加该生命周期接线并重新执行必要的增量 Editor 构建；这不是失败重试。

候选 Automation 首次命令使用了历史 `;Quit` 后缀，7 个测试和原生退出均成功，但该后缀被旧 group parser 当作组名的一部分，故没有用作正式门禁证据。正式四组去掉后缀，完全由 `-TestExit` 在 native TEST COMPLETE 后退出；没有修改产品代码来掩盖此流程兼容问题。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Automation 使用测试 World 与 fake preflight/execution ports 验证唯一安装、Run/owner fence 和 command-to-component 委托；真实地图 GameMode 初始化、真实输入与连续 swept 画面只完成编译，尚未执行。

下一阶段建议 P10.8 建立 Run-owned Spirit Evasion action reservation 与 canonical product config authority：由 Coordinator 分配 activation sequence，content owner 冻结 definition/policy/trajectory，再交给本阶段 Router。等该身份链完成后才接真实 PlayerController input；SpiritEnergy 仍等待唯一资源 authority，不能在输入层临时扣减。
