# Dev.D.UE.0.0.10.P11.0.r0 Report

## 1. 结论

P11.0 **PASS**。本阶段建立了 0.0.10 第一条普通武器格挡核心契约：`Combat.Action.Sword.Guard01` 只有在同一冻结动作精确跨过 `Startup -> Active` commit point、且携带有效武器实例身份时，才能打开格挡窗口；该窗口只在动作仍为 `Active` 时向 CombatCore 投影 `ReduceFraction / Guard / DefenseGuard` 防御层。

减伤比例由内容定义传入，不在运行时硬编码平衡值。窗口、开启凭证、投影凭证与 LayerId 全部由规范化冻结输入确定性派生。100 点物理伤害配合测试内容的 `0.25` 格挡比例得到 `PreventedDamage=25`、`FinalDamage=75`，结果为 `Mitigated` 且保持伤害守恒。

正式验证为 WeaponGuard `6/6`、ActionLifecycle `1/1`、CombatCore `9/9`、CombatRuntime `93/93`、`Shanmen.0_0_10` 全量 `461/461`。五份正式日志合计 `570` 条 Success、`0` Fail；changed-file gate、89-rule mapping、138/138 mapping self-test、静态边界扫描、`git diff --check`、Editor Development 与 Game Development 均通过。

本阶段刻意没有实现精准格挡时窗、格挡方向/夹角、体力或灵力数值、武器耐久预留、产品输入、GAS ability 或 Actor/World 适配。未冻结的产品政策没有被塞进纯核心契约。

## 2. 功能性

### 2.1 不可变普通格挡定义

`FShanmenWeaponGuardDefinition::TryCapture` 固定：

- canonical action：`Combat.Action.Sword.Guard01`；
- 非空 `RuleId`；
- 内容提供的 `GuardFraction` 必须有限、`> 0` 且 `<= 1`；
- required/blocked damage、source 与 target tag filters；
- 任一 required/blocked 标签树发生父子交叠时失败关闭。

成功 capture 后，所有字段只通过 getter 读取；动作开始后不能从 Blueprint 或调用方改写格挡比例与覆盖规则。

### 2.2 commit-bound 窗口

`FShanmenWeaponGuardWindow::TryOpen` 同时要求：

1. action snapshot、definition 与 runtime 绑定同一 canonical action；
2. action 冻结了有效 `SourceItemInstanceId`，无武器身份不能格挡；
3. receipt 是同一 activation 的 `Startup -> Active` 转换；
4. receipt 首次跨越并保留 commit point；
5. runtime 当前非 terminal、处于 `Active`，且 next sequence 紧邻 commit receipt；
6. runtime 内冻结 action 与传入 action 的 Run、owner、activation、source entity、source item、content stamp 与 tags 全部一致。

Startup receipt、外来 runtime、无武器 action、错误 definition 或过期 phase 均在产生窗口身份前拒绝。

### 2.3 CombatCore 防御投影

活动窗口投影一个普通格挡层：

- operation：`ReduceFraction`；
- order：`FShanmenDefenseOrder::Guard`；
- layer tag：`Shanmen.Defense.Guard`；
- magnitude：冻结 definition 的 authored fraction；
- source instance：冻结动作的真实武器实例；
- `bRequiresCommitOnTrigger=false`；
- 完整继承 definition 的 damage/source/target filters。

`DefensePerfectGuard` 不会被附加，普通格挡不会伪装成精准格挡。武器实例作为审计来源保留，但由于本阶段没有真实耐久预留，不能把该层伪装成可提交的资源事务；后续产品 adapter 必须先取得真实 reservation，才能建立 resource-backed layer。

### 2.4 生命周期关闭

格挡窗口不拥有独立 timer、duration、Tick 或 Close 状态机。每次投影都重新校验现有 `FShanmenActionOrchestrator`：

- `Active`：允许投影；
- `Recovery`：拒绝；
- `Interrupted` 或其它 terminal：拒绝。

因此动作编排器仍是唯一窗口生命期权威，不存在动作结束后格挡层残留的第二套状态。

## 3. 完整性与兼容性

- 复用 P3 `FShanmenActionOrchestrator`，没有复制动作状态机；
- 复用 CombatCore 已有的 `ReduceFraction`、Guard ordering、`DefenseGuard` tag 与纯函数 resolver；
- 没有修改 CombatCore enum、tag tree、Impact 结构或既有防御顺序；
- 没有修改 P4 BasicSword、P5 durable item authority、P9 SpiritShield、P10 SpiritEvasion 或旧产品入口；
- source item identity 被纳入 window/layer identity，便于后续耐久 adapter 做精确关联；
- fraction 使用 IEEE bits 进入 canonical identity，等价冻结输入可重放相同身份；
- RuleId、fraction、action、weapon 或 tag filters 改变都会产生不同 window identity；
- 没有 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage`、RNG、Timer 或 Tick callback 依赖。

## 4. 关键不变量

1. 只有 `Combat.Action.Sword.Guard01` 能建立普通格挡定义；
2. GuardFraction 必须是 `(0, 1]` 内有限值；
3. required 与 blocked tag 范围不得交叠；
4. 武器格挡必须携带有效 source item identity；
5. 只有 exact `Startup -> Active` commit receipt 可以打开窗口；
6. runtime、receipt 与 action 必须完整绑定同一冻结激活；
7. action runtime 是唯一窗口生命期权威；
8. 只有 Active phase 可以投影；
9. 投影固定为 `ReduceFraction / Guard / DefenseGuard`；
10. 普通格挡绝不携带 `DefensePerfectGuard`；
11. 没有真实资源预留时绝不声明 trigger commit；
12. tag filters 对 damage、source 与 target 全部生效；
13. resolver 结果保持 `RawDamage = PreventedDamage + FinalDamage`；
14. 等价冻结输入生成等价 identity，不等价 fraction 生成不同 identity。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.CombatRuntime.WeaponGuard` 六个测试：

- `DefinitionContract`：canonical action、比例范围、命名规则与标签冲突；
- `CommitBoundWindow`：Startup 拒绝、exact commit 接受、foreign runtime 与 unarmed action 拒绝；
- `DefenseProjection`：Guard layer 结构、武器来源、普通/精准标签隔离及 100→25→75 守恒；
- `TagFiltering`：Mental damage 与 non-Living target 不触发；
- `ActivePhaseBoundary`：Recovery 与 Interrupted 立即停止投影；
- `DeterministicIdentity`：等价重放稳定，fraction 改变时 identity 改变。

同时执行 ActionLifecycle、CombatCore、CombatRuntime 宽回归和 0.0.10 全量回归。全量由 P10.11 的 `455` 增至 `461`。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenWeaponGuard.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenWeaponGuard.cpp`。

测试与回归规则：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenWeaponGuardTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

实现与门禁为 `5 files / +1058 / -0`；加入本 Report 与同名 Development Log 后 exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt、Report、交接文档与用户文件均未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P11.0-WeaponGuard-final.log` | `WeaponGuard` | 6 | 0 | `9EA56D4C547841DD25FAF643F4DA08F9D59D692D7A8715286FF4C34F02E7912B` |
| `P11.0-ActionLifecycle-final.log` | `ActionLifecycle` | 1 | 0 | `FCE16F556D5C35B7938CF0B3322B7D0A71825CA7E09154AC57E7D416E540A07C` |
| `P11.0-CombatCore-final.log` | `CombatCore` | 9 | 0 | `22A995A52E1147EC1127F682698FFD5C738D60F62B49D7261403C9189CBC1742` |
| `P11.0-CombatRuntime-final.log` | `CombatRuntime` | 93 | 0 | `CE70062FA62F6A72AA3997EB7FCF90C53863DFB0426E8257256CDAA9F5C2CD79` |
| `P11.0-Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 461 | 0 | `9EDE3234CB2542365EFB7C14F40D68279436A68DE1FF23A757BF315B59D98DE7` |

每份日志均有且只有一个目标 RunTests 命令、至少一个 terminal queue-empty、selected Fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=89
SELF_TEST: PASS 138/138
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=4 Logs=5
git diff --cached --check: PASS
BOUNDARY_SCAN: PASS hits=0
```

- mapping SHA-256：`AC2CAE819DEE2A48AE8677008422E9FF9E5A5FEA22CFE87A4C4649D850F55612`；
- self-test SHA-256：`1ED3590940664BBE93D3E0B4D76A69CE8852D12F9C221DE9AE9DE6276279BDA2`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development | Succeeded | 8 / 40.63s | 0 | `5FD7AC8C267B9409CF3412D1F495B597DDB9700A7712F6E63783B7C145AFB0E8` |
| Game Development | Succeeded | 5 / 30.78s | 0 | `219F28FD8EFBEF466760A9043144204B0DFA1545E0429E1BD27DCE7FE66CF5F3` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1369088` bytes，SHA-256 `34E7CB337962C9FC0BCA48196A4E12ABB7161FF0B4DDBF1895289DDB1B618835`；
- `demo_map.exe`：`353997312` bytes，SHA-256 `642BCA9323652F9BC624BFAE980D37B1EAFEF03C869907FCA7EA490D0B52E776`；
- 为遵守一次必要成功构建原则，没有在源码未变化后重复 Editor 构建。

## 9. 真实异常

本阶段没有源码、UHT、Automation、changed-file gate、Editor 或 Game 构建失败；没有发生 C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

五份 Automation 日志均含 UE 5.8 自带 `UE::UnifiedErrorTest` 的 13 行 `Condition failed` 初始化自检噪声；这些行不属于所选测试。随后全部选中测试逐项 Success，queue 正常清空，进程原生退出 `0`。本 Report 没有把通用初始化噪声描述为源码失败，也没有只依赖进程码判定测试成功。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值契约、代码审查、无头 Automation、静态/路径门禁、Editor Development 与 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

建议 P11.1 在本契约之上增加“普通格挡 → 精准格挡”的独立 timing policy，但仍复用同一 action lifecycle；若要扣除武器耐久，必须先接入现有 durable item authority 的真实 reservation/commit/cancel 链，不能直接把 P11.0 的无资源投影改写成假提交。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-0-weapon-guard-window/Docs/Report/Dev.D.UE.0.0.10.P11.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-0-weapon-guard-window/Docs/Log/Dev.D.UE.0.0.10.P11.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-0-weapon-guard-window>
