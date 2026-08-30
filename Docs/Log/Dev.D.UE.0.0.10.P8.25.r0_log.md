# Dev.D.UE.0.0.10.P8.25.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.25.r0`；
- 基线：`2208ec19760cf1f9efcfc8067417ebb9527b729a`（P8.24）；
- 分支：`agent/0.0.10-p8-25-formation-influence-consumer-projection`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

把 P8.24 active lease 中冻结的 evaluation receipt 投影为首个具体产品 consumer contract。要求复用旧 attribute identity 与 modifier handle，不重算 magnitude、不提前转 float、不直接调用 Actor component，并为 Apply／Remove 提供同一可撤销 handle。

## 设计记录

### authored consumer seam

`Fdemo_mapShanmenFormationInfluenceConsumerDefinition` 的首个 factory 固定映射 `Influence.Offense.Power -> AttackPower / Add`。definition 携带 scale、priority 和 content，因此单位解释属于版本化内容，而不是 evaluator 或旧 component 的隐式常量。

### self-contained projection

projection 保存完整 lease snapshot 和 consumer definition；自身 `IsValid()` 会复核 lease、content、channel、非零 magnitude、deterministic handle 与 SourceId。数值保持 signed numerator/positive denominator，不执行除法。

### reversible identity

复用 `Fdemo_mapModifierHandle`，但 Value 由 canonical payload 生成。payload 覆盖 lease/receipt/key、consumer mapping、operation、signed magnitude、scale、priority 与 content。Apply／Remove command ID 纳入 operation，两条命令共享 projection handle。

### fail-closed no-op

失效 lease、无效 definition、跨 content 和 unsupported channel 输出明确错误并保持空 projection。合法 evaluation 的最终和为零时返回 `NoContribution` 成功，不产生 handle。

## 执行序列

1. 审查 P8.24 lease snapshot、evaluation binding、P8.23 evaluator、旧 `Fdemo_mapModifierHandle`／`Fdemo_mapModifierSpec` 与 `AttackPower` identity。
2. 选择复用旧 handle，但禁止调用 `Udemo_mapAttributeComponent::AddModifier/RemoveModifier`，避免建立第二种 handle 或提前进入 Actor mutation。
3. 新增 content-versioned consumer definition、self-validating projection、deterministic handle／SourceId 与 reversible command factory。
4. 新增 deterministic replay、fail-closed contracts、zero/signed/identity fence 三项测试；测试通过真实 evaluator + binding + lease executor 构造 active lease。
5. 增加 changed-file regression mapping，要求 consumer、binding、evaluator、lease、attributes 与 CombatCore 六组证据；self-test 从 `78/78` 增至 `80/80`。
6. 首次 Editor `5 actions / 28.73s / exit 0`；专项首轮 `3/3`，attributes `3/3`，influence `59/59`，全量 `308/308`。
7. 提交前检查 deterministic identity、zero no-op 与 Remove handle 语义；只调整一处边界注释。
8. 最终 Editor `5 actions / 7.17s / exit 0`；最终四组 Automation 全绿。
9. changed-file gate 在产品文件阶段通过 `Changed=5`，Report／Log exact stage 后最终通过 `Changed=7 / Rules=1 / Required=6 / Logs=4`。
10. 最终 Game `4 actions / 23.81s / exit 0`，完成静态边界与 exact staging 检查。

## 首轮证据

- `P8.25-Editor-initial.log`：`5 actions / 28.73s / exit 0`，SHA-256 `7BD95400C9069BFFB9963916B03E42BC55F5CE6CCB39E3F2A9DB237469B372A8`；
- `P8.25-FormationInfluenceConsumerProjection-initial.log`：`3/3`、Fail `0`、queue observed、exit `0`，SHA-256 `071A98A711521351C015AAA0455F72257CB3B76A460D6DE65C7BC73736337711`；
- `P8.25-Attributes-initial.log`：`3/3`、Fail `0`、queue observed、exit `0`，SHA-256 `71D23729D1525F46C05D9B0105AA1D82ED8A544D36E8FFDE7D0A56330B2213D6`；
- `P8.25-FormationInfluence-initial.log`：`59/59`、Fail `0`、queue observed、exit `0`，SHA-256 `53C4EEA19FA429BCB4F2BF27625293C51ABCF50164A5874FA7BC6C1EEF5126BC`；
- `P8.25-Shanmen-full-initial.log`：`308/308`、Fail `0`、queue observed、exit `0`，SHA-256 `F57BBED95ADD5695685687B37162DBCE328008CEB68C2FEE935563D247C9E4E9`。

没有源码编译失败或 Automation case 失败。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P8.25-FormationInfluenceConsumerProjection-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection` | 3 | 0 | 0 | `8E1060ABA1F8C3126C0A823567EDCA4D1FE963CCE234461BE62984828C769558` |
| `P8.25-Attributes-final.log` | `demo_map.V3.Attributes` | 3 | 0 | 0 | `800DE3D47667DE2E571AD8B0342CA1FF2247114CE770F9FDE4B161374DA77B89` |
| `P8.25-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 59 | 0 | 0 | `94082224315A5B762C37A00711181193987D3D0423AB55E9B8689E53DCFD0519` |
| `P8.25-Shanmen-full-final.log` | `Shanmen.0_0_10` | 308 | 0 | 0 | `C5412F67531EBF493D3795AAC732BD9AF1ABA2D1F1F496BFB350DCAB75C7C189` |

四份最终日志均观察到 queue-empty marker；fatal／unhandled／ensure 为 `0`。UnifiedError 启动 self-test 固定 `Condition failed` 各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=60
SELF_TEST: PASS 80/80
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=6 Logs=4
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=6 Logs=4
```

- mapping SHA-256：`2461D4A01279DB86B365916F68F95A3E63051746B3B3731CAAD89715490FBEA9`；
- self-test SHA-256：`443078BC92E7505D2BD2122295CD63BF230C427FD7134C2CE8733FF50B84B3DF`。

## 静态边界

新 consumer production files 扫描：

```text
UWorld/AActor/UObject = 0
GameplayAbility/GameplayEffect/AbilitySystem = 0
Timer/Async/RNG = 0
float/double = 0
Spawn/ApplyDamage/SaveGame/ProfileRepository = 0
Tick/while = 0
AddModifier/RemoveModifier/Fdemo_mapModifierSpec = 0
```

`git diff --cached --check`：PASS。

## 构建证据

- Editor initial：`5 actions / 28.73s / exit 0`，日志 SHA-256 `7BD95400C9069BFFB9963916B03E42BC55F5CE6CCB39E3F2A9DB237469B372A8`；
- Editor final：`5 actions / 7.17s / exit 0`，日志 SHA-256 `FAD108399D180030E104BB51C3EFD679603A1601895772AA5BD238D9238291E6`；
- Game final：`4 actions / 23.81s / exit 0`，日志 SHA-256 `4ABEF6B710228D8B17084588267A2BC4DB61A1B04CA48B0DD58FBD49F8C1510D`；
- `UnrealEditor-demo_map.dll`：`11829248` bytes，SHA-256 `C88E20F793DED94F8AFDD28305CC73893E1B9F4925CBCE0C3BBB539FE62152A3`；
- `demo_map.exe`：`352925696` bytes，SHA-256 `13E28F7B475B9771389979F6125FE9A317BF8AD12ADEDFDE601B32B0AB2F97A0`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.26 可增加纯值 consumer application registry，持有 active projection handle 与 command replay evidence，验证 collision、stale remove、幂等恢复和 teardown drain；该阶段仍不直接写 Actor component。窄 attribute adapter 在 registry contract 稳定后再接入。
