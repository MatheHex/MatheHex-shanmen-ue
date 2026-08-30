# Dev.D.UE.0.0.10.P10.0.r0 Report

## 1. 结论

P10.0 **PASS**。本阶段建立了 0.0.10 第一条主动闪避基础契约：短闪避窗口只能由同一冻结动作的 `Startup -> Active` commit proof 打开，只能在该动作仍为 `Active` 时向 CombatCore 投影 `Evade` 防御层；进入 Recovery 或发生 Interrupted 后不再可投影。动作编排器继续是唯一窗口生命期权威，没有增加第二套 clock、timer 或状态机。

最终验证为 SpiritEvasion `6/6`、ActionLifecycle `1/1`、CombatCore `9/9`、CombatRuntime `82/82`、`Shanmen.0_0_10` 全量 `385/385`。五份证据日志合计 `483` 条 success、`0` fail；changed-file gate、mapping self-test、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

本阶段刻意没有发明产品 SpiritEnergy float、移动距离、持续时长、无敌帧、输入绑定或资源扣除规则。现有产品层没有已冻结的 SpiritEnergy writable authority；这些产品政策必须由后续阶段接入真实 owner，而不是在主动闪避窗口内形成第三套账本。

## 2. 功能性

### 2.1 不可变定义

`FShanmenSpiritEvasionDefinition::TryCapture` 固定：

- canonical action：`Combat.Action.Spell.SpiritEvasion01`；
- 非空 RuleId；
- required/blocked damage、source 与 target tag filters；
- 父子标签范围发生交叠时失败关闭。

成功 capture 后字段仅通过只读 getter 暴露，外部不能在窗口打开后改写覆盖规则。

### 2.2 commit-bound 窗口

`FShanmenSpiritEvasionWindow::TryOpen` 同时验证：

- action snapshot、definition 与 runtime 绑定同一 action definition；
- runtime 当前为非 terminal `Active`；
- receipt 是该 activation 的 `Startup -> Active` 转换；
- receipt 首次跨越并保留 commit point；
- runtime next sequence 与 commit receipt 紧邻；
- runtime 内冻结 action 与传入 action 完整一致。

Startup receipt、外来 action/runtime、过期 phase 或无效 definition 均不能打开窗口。

### 2.3 CombatCore 防御投影

活动窗口投影一个固定防御层：

- operation：`PreventAll`；
- order：`FShanmenDefenseOrder::Avoidance`；
- layer tag：`Shanmen.Defense.Evade`；
- source instance：确定性的 window id；
- `bRequiresCommitOnTrigger=false`；
- 完整继承 definition 的 damage/source/target filters。

在 100 点物理伤害样例中，结果为 `Evaded`、`PreventedDamage=100`、`FinalDamage=0`，且 `RawDamage == PreventedDamage + FinalDamage`。精神伤害或非 Living 目标不触发该层，伤害保持 `Applied=100`。

### 2.4 生命周期关闭

窗口不拥有独立 `Close`、duration 或 timer。每次投影都重新检查 action runtime：

- Active：允许投影；
- Recovery：拒绝；
- Interrupted/terminal：拒绝。

因此不存在动作已经结束而闪避仍悬挂的第二生命期，也不存在 timer 与 action phase 竞态。

## 3. 完整性与兼容性

- 复用 P3 的 `FShanmenActionOrchestrator`，不复制动作状态机；
- 复用 CombatCore 的有序 `FShanmenDefenseLayer` 与纯函数 resolver；
- 不修改 P9 SpiritShield、formation、controlled/thrown weapon 或 0.0.9B 产品权威；
- receipt/window/projection identities 均由规范化 action、commit 与排序 tag 输入确定性派生；
- equivalent frozen input 可重放相同 window、receipt、projection 与 layer ids；
- RuleId 改变时 window id 必然改变；
- 没有 `demo_map`、World、Actor、ApplyDamage、RNG、Timer 或 Tick callback 依赖；
- 未把旧随机 DodgeChance 冒充主动、反应式灵力闪避。

## 4. 关键不变量

1. 只有 canonical SpiritEvasion action 可以建立 definition；
2. required 与 blocked tag 范围不得交叠；
3. 只有 exact `Startup -> Active` commit receipt 可以打开窗口；
4. runtime 与 receipt 必须绑定同一冻结 action；
5. action runtime 是唯一窗口生命期权威；
6. 只有 Active phase 可以投影；
7. Recovery、Interrupted 与 terminal 状态立即停止投影；
8. 投影固定处于 Avoidance 顺序并携带 DefenseEvade tag；
9. active evasion 是无额外 mutable commit 的瞬时防御证明；
10. tag filters 对 damage、source 与 target 全部生效；
11. resolver 结果保持伤害守恒；
12. 等价冻结输入生成等价 identity，不等价规则生成不同 identity。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.CombatRuntime.SpiritEvasion` 六个测试：

- `DefinitionContract`；
- `CommitBoundWindow`；
- `DefenseProjection`；
- `TagFiltering`；
- `ActivePhaseBoundary`；
- `DeterministicIdentity`。

同时执行依赖的 ActionLifecycle、CombatCore、CombatRuntime 宽回归和 0.0.10 全量回归。全量由 P9.6 的 `379` 增至 `385`。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritEvasion.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritEvasion.cpp`。

测试与回归规则：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritEvasionTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

实现为 `5` 个文件、`994` insertions、`0` deletions；加入本 Report 与同名 Development Log 后 exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.0-SpiritEvasion-final.log` | `SpiritEvasion` | 6 | 0 | `164376AAC882F6A7F85D7BFE9B70EEDD93471D631F5EBBDDFDBEC3C5311048AB` |
| `P10.0-ActionLifecycle-final.log` | `ActionLifecycle` | 1 | 0 | `EB5442A065AF40783635CCFC4C2D74509501E2863491684C1DFF13A81CF3CCE3` |
| `P10.0-CombatCore-final.log` | `CombatCore` | 9 | 0 | `50C99CEB62BD0C13F291785CC46ECFD3CB28AAC49509BCB425BAA3C51CBCABCE` |
| `P10.0-CombatRuntime-final.log` | `CombatRuntime` | 82 | 0 | `D3BAFB4591CEDACC1841F117BE815A5ED8AC470EBD5C9A155CF6A743CBA26996` |
| `P10.0-Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 385 | 0 | `8E81A0EE1307C3AE3B71DFEDC5CC4E216D71FB725ACA8D3CF2F12B7FD80CA85B` |

每份日志均有一个目标 RunTests 命令、一个结构化 queue-empty、selected fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=76
SELF_TEST: PASS 114/114
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=4 Logs=5
git diff --check: PASS
BOUNDARY_SCAN: PASS hits=0 (world/actor/damage/RNG/timer/tick-callback)
```

- mapping SHA-256：`7FBCB8F63AAC9B00D73626F2544C162FFF57C49CB72D2F38D7AFB0628872FAFB`；
- self-test SHA-256：`F9623AED7B8CAE97AD52708D1DF08B0F87A29EA7A89B00CED14F2B1A7FC15F21`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 6 / 35.95s | 0 | `DEB6D66DBC69F25E71FD4C1224F3AF4D154443E0E05D1F4E94ECB652870F12DB` |
| Editor final | Succeeded, target up to date | 0 / 1.04s | 0 | `F430314655F2327089EAED4E96F40405105028F113140D8015AB8C20D13D82A1` |
| Game final | Succeeded | 5 / 30.78s | 0 | `275B9A4543DE27FB9AB36151DDD5CF98AE45B3078F1D264982815F17C5AB421A` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1247744` bytes，SHA-256 `33304175024077ED78A7B824F6D01288B78C3B7485E9A6AD112046FD7FB9470C`；
- `demo_map.exe`：`353596416` bytes，SHA-256 `ED52E8FFDB957594C4C3663964E8912C7836A143EAA856CE3A63EF99AB452B82`。

## 9. 真实异常

本阶段没有源码、UHT、Automation、changed-file gate 或构建失败。候选与正式 Automation 启动日志仍包含 UE 5.8 自带 UnifiedError 基线 `Condition failed` 诊断；所有选中测试随后逐项 Success，结构化 queue-empty 存在，selected fail/fatal/unhandled/ensure 均为 `0`，进程原生退出 `0`。

没有发生 C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值契约、代码审查、无头 Automation、静态扫描、changed-file gate、Editor/Game Development 构建与 Git 证据。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P10.1 可在不引入产品数值的前提下冻结“输入意图 -> movement request -> action activation”的单向适配边界；真实 SpiritEnergy owner、移动距离/持续时长、碰撞位移和无敌帧政策仍应由正式产品契约决定，不能在本窗口对象内私自补齐。
