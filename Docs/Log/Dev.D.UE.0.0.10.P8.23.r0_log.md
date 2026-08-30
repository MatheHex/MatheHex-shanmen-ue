# Dev.D.UE.0.0.10.P8.23.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.23.r0`；
- 基线：`6a9f49722d1df0a84888584f5c41981ae66795bc`（P8.22）；
- 分支：`agent/0.0.10-p8-23-formation-influence-modifier-evaluator`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

把 P8.11 后置的 magnitude/stack semantics 实现为独立纯值层：冻结 authored modifier、使用 native GameplayTag channel 与 subject filters、确定性求值并生成可自验证 receipt；不改 intent、lease、Host 或 executor authority。

## 设计记录

### immutable authored values

Specification 私有字段由 `TryCreate` 捕获，deterministic ID 覆盖 policy/influence/modifier identity、content、channel、required/blocked tags、signed integer magnitude、stack group/policy 与 priority。零值、矛盾过滤和未知策略 fail-closed。

### canonical tag filtering

Channel exact match；subject required/blocked filters 使用 GameplayTag parent semantics。所有 authored specs 按 SpecificationId 排序，filtered specs 仍保留零贡献 decision。

### explicit stacking

Additive 全贡献。StrongestMagnitude 使用 absolute magnitude → priority → canonical ID；HighestPriority 使用 priority → absolute magnitude → canonical ID。同一 channel/group 的 policy 冲突拒绝整次 evaluation。

### self-validating receipt

Receipt 携带完整 context、完整 specs 与逐条 decisions。`IsValid` 从这些冻结输入重新求值，再核对 decision、winner、contribution、final magnitude 与 ReceiptId。输入顺序、数组地址或运行时 FName index 不参与身份。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceModifierEvaluator.h`（162 行）；
- `Source/demo_map/demo_mapShanmenFormationInfluenceModifierEvaluator.cpp`（538 行）；
- `Source/demo_map/demo_mapShanmenFormationInfluenceModifierEvaluatorTests.cpp`（398 行）；
- `Source/ShanmenCombatCore/Public/ShanmenCombatTags.h`；
- `Source/ShanmenCombatCore/Private/ShanmenCombatTags.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查 P8.11 policy/intent、P8.15 adapter、P8.16 lease executor、CombatTags 与 deterministic ID 惯例。
2. 冻结纯值 specification、context、decision、receipt、status 与 evaluator 契约。
3. 增加五个 generic influence native tags，不引入 GAS module。
4. 实现 canonical tag serialization、duplicate/content/policy conflict gates、三种 stack policy 与 receipt rebuild validation。
5. 新增 filtering/additive、stack/order、fail-closed、receipt integrity 四项测试。
6. 首次 Editor 编译 `43 actions / 131.53s` 成功；focused 首轮 `4/4`。
7. 代码审查明确 secondary tie-break，并补同 definition 异 payload 拒绝；review focused 保持 `4/4`。
8. 回归规则升至 `58`，self-test 升至 `77/77`。
9. 首次 Game Development `38 actions / 114.51s` 成功。
10. 提交前 staged review 将失败 capture 改为 Candidate-then-commit，并补输出清零断言。
11. exact-source Editor `5 actions / 16.92s`、Game `4 actions / 22.54s` 成功。
12. 四组最终 Automation 全绿，全量由 `298` 增至 `302`。
13. changed-file gate 通过：Source／Scripts `Changed=7 / Rules=3 / Required=3 / Logs=4`。
14. 完成 boundary scan 与 `git diff --check`。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.23-FormationInfluenceModifierEvaluator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceModifierEvaluator` | 4 | 0 | 1 | 0 | `5AC4B6AF24A99A57F9F963A81E56E435F1847EB5B979A4990DAB7B42F7C39890` |
| `P8.23-CombatCore-final.log` | `Shanmen.0_0_10.CombatCore` | 9 | 0 | 1 | 0 | `A91FFF0E3B90423E43BBCCBE31F26DA0983DD5B375BAA88C9697842D25BAB6D9` |
| `P8.23-FormationInfluenceIntents-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceIntents` | 4 | 0 | 1 | 0 | `F92C4C3778A5753E0F77EC909A1C32D898B1C5BE52418B53DDF0E6D41EF62BCF` |
| `P8.23-Shanmen-full-final.log` | `Shanmen.0_0_10` | 302 | 0 | 1 | 0 | `FB77DDC8FF2BC89280768DA501E7450C60F869EFF36E5BDC408FC68CD21D4370` |

Fatal／Unhandled／Ensure：四份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## 首轮与审查证据

- 初始 Editor：`43 actions / 131.53s / exit 0`，SHA-256 `6D6BCD40F541C60B12929B357B1532514293B332A413BA8D018031FAF6A98FAF`；
- Evaluator initial：`4/4 / Queue 1 / exit 0`，SHA-256 `59CBCF5ED116E7D5101DF2C89584AD0985E1CBD4776DCED4F040E090052C535C`；
- secondary tie-break review：`4/4 / Queue 1 / exit 0`，SHA-256 `3F41FAA45235D52118C10949D48E391FC4B4CD67C52E028B8D72D0065CB2156F`；
- 最终 Editor exact source：`5 actions / 16.92s / exit 0`，SHA-256 `65AF48F95BBFC1E186719C5E6772646C58D1DBB01AA74EA41AA63B20188B9AEF`；
- 最终 Game exact source：`4 actions / 22.54s / exit 0`，SHA-256 `AB875239568298E28DC520C092EE8E965E262F83CC6F994746D5BE034CC2A888`。

源码无编译失败，Automation case 无失败。43/38 actions 是 CombatCore public tag header 的首次 Editor/Game 依赖传播，不是环境或内存故障；失败输出清零修正后以增量 exact-source Editor/Game 与四组 Automation 全部重验。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=58
SELF_TEST: PASS 77/77
REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=3 Logs=4
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=3 Logs=4
```

- mapping SHA-256：`4ABE2DC6A29386ADFF60CD7B11A069DC531E3818C77A5AB4E7FE0337B3F42043`；
- self-test SHA-256：`B673258DD410E459D6CE66AC9C1207C5D6250F09453A55CFFFEE56BA14C4E788`。

第二条 coverage 是 Report／Log 加入 exact stage 后的最终 gate。

## 静态边界

Evaluator 生产文件扫描：

```text
UWorld/AActor/UObject = 0
GameplayAbility/GameplayEffect/AbilitySystem = 0
Timer/Async/RNG = 0
float/double = 0
Spawn/Damage/Persistence = 0
Tick = 0
while = 0
for = 15 (bounded specification/decision canonicalization only)
```

`git diff --check`：PASS。

## 构建产物

- `UnrealEditor-demo_map.dll`：`11758592` bytes，SHA-256 `31869857F4062D3A4AB33ABEE11663EC23C79FE9CD052BEC1CD9D4ADB7A6588D`；
- `UnrealEditor-ShanmenCombatCore.dll`：`181248` bytes，SHA-256 `0195D61D73E7E6A375CC93E69506A6358230224959E3C71944EF6BF0D565C261`；
- `demo_map.exe`：`352871424` bytes，SHA-256 `3A3A4D0D96BB010043B25F7DAC31C847CD4DCFD1DD1D322BA2EA79C7908990E4`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.24：把 immutable evaluation receipt 接入既有 execution command／invocation 与 active lease。Apply 必须匹配 intent 的 run/subject/policy/content；Remove 读取 lease 冻结 receipt，不重新采样。不得建立第二套 pending/ack authority，也不接 GAS、Actor mutation 或 persistence。
