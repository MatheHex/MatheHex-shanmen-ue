# Dev.D.UE.0.0.10.P8.24.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.24.r0`；
- 基线：`36fa407d87e90beeb7b6adc993ed9d28484f5b98`（P8.23）；
- 分支：`agent/0.0.10-p8-24-formation-influence-evaluation-binding`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

把 P8.23 已求值且自验证的 formation influence receipt 接入现有 P8.15–P8.22 execution chain。Apply 必须冻结匹配 Host intent 的回执；Remove 不得重新采样，必须复用 active lease 的冻结证据。保持 ProductHost／ledger 单一 authority，不建立第二套队列或 executor。

## 设计记录

### explicit operation shape

`Fdemo_mapShanmenFormationInfluenceEvaluationBinding` 只有 Apply receipt 和 Remove empty 两种结构。Apply capture 拒绝空 decisions 与篡改回执；Remove 的默认对象就是 canonical empty binding。

### host-owned identity fence

Router 不根据 caller 自述判断匹配，而是读取 Host pending intent 或 ledger historical intent，再核对 run、subject、policy、influence 与 content。request、command、invocation 三层都携带并验证同一 binding。

### frozen lease evidence

Apply 把完整 receipt 存入 active lease 和 completion history。Remove invocation 不携带 receipt，LeaseExecutor 从 active lease 复制冻结回执，在删除 lease 前保存到 Remove completion。completed replay 读取历史回执，不重复 mutation。

### deterministic schema revision

AttemptId、LeaseId 和 ExecutorReceiptId 都增加 receipt identity，因此 canonical namespaces 从 `r1` 升至 `r2`。Remove attempt 使用固定 canonical marker，避免空 GUID 与遗漏字段混淆。

## 执行序列

1. 审查 P8.23 evaluator、P8.15 adapter、P8.16 lease、P8.19 router、P8.22 lifecycle command host 与 replay 语义。
2. 新增 evaluation binding 纯值边界及 Host intent identity matching。
3. 把 binding 接入 request、command、invocation、route record 与 adapter replay。
4. 扩展 lease snapshot／completion history，Apply 冻结 receipt，Remove 复用并保留历史证据。
5. 调整 lifecycle terminal empty-step 校验，使 canonical empty binding 成为终止命令唯一合法形态。
6. 更新既有测试 helper 和手工 replay command，保持所有既有路径显式构造 binding。
7. 核心接线首次 Editor `15 actions / 51.43s / exit 0`；既有 influence 父组 `53/53`。
8. 新增 ApplyRemoveFreeze、RequestIdentityFence、TamperAndOperationFence 三项；增量 Editor `4 actions / 7.37s / exit 0`，focused 首轮 `3/3`。
9. 审查 deterministic ID schema，把三个 namespace 升至 `r2`。
10. 新增 regression mapping rule 与 fail-closed fixture；JSON 解析通过，自检升至 `78/78`。
11. 最终 Editor `5 actions / 7.30s / exit 0`；四组 Automation 全绿，全量从 `302` 增至 `305`。
12. changed-file gate 通过：`Changed=12 / Rules=6 / Required=26 / Logs=4`。
13. 最终 Game `12 actions / 42.28s / exit 0`，完成 boundary scan 与 `git diff --check`。

## 首轮证据

- `P8.24-FormationInfluence-initial.log`：`53/53`、Fail `0`、Queue marker observed、exit `0`、SHA-256 `28AB6CD8B780E957655CDD07115B5FB09652A180DB16489AB0C6721B2F2A6498`；
- `P8.24-FormationInfluenceEvaluationBinding-initial.log`：`3/3`、Fail `0`、Queue marker observed、exit `0`、SHA-256 `3785B485DFE81DDF8B988F9F56E7E26E25242E70940C53EE8674D83F5F941313`。

没有源码编译失败或 Automation case 失败。审查修正是 deterministic namespace 版本升级，不是运行失败补丁。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P8.24-FormationInfluenceEvaluationBinding-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceEvaluationBinding` | 3 | 0 | 0 | `06089066B43A3732C9556EDC19FB6C562BFF22DE64337FFA789149CA481B03FB` |
| `P8.24-FormationInfluenceModifierEvaluator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceModifierEvaluator` | 4 | 0 | 0 | `CFC3ED2DA4B92234760A7F8A9499EEA90F69ADC6CD0336688A95C17A3CD62F39` |
| `P8.24-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 56 | 0 | 0 | `4D33AB2F1B9FB317EFE80ABDC2E84604ABEB3A592CD821A33CB0E299D1987EEA` |
| `P8.24-Shanmen-full-final.log` | `Shanmen.0_0_10` | 305 | 0 | 0 | `3127FAE643E53ADA32476377AF8EB91B8EBA651444FA9838E301386441640DB7` |

四份日志均观察到 queue-empty marker；fatal／unhandled／ensure 为 `0`。UnifiedError 启动 self-test 固定 `Condition failed` 各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=59
SELF_TEST: PASS 78/78
REGRESSION_COVERAGE: PASS Changed=12 Rules=6 Required=26 Logs=4
REGRESSION_COVERAGE: PASS Changed=14 Rules=6 Required=26 Logs=4
```

- mapping SHA-256：`EB0BA58BBD7EE49AD47EC895ACCD56B30CD55C04F4367E2D71EC84EED3C09862`；
- self-test SHA-256：`FFE5C762D96EEDFE690B7905658736176FDA8E55303529802ACADF0BB1C57871`。

## 静态边界

新 binding 生产文件扫描：

```text
UWorld/AActor/UObject = 0
GameplayAbility/GameplayEffect/AbilitySystem = 0
Timer/Async/RNG = 0
float/double = 0
Spawn/ApplyDamage/SaveGame/ProfileRepository = 0
Tick = 0
while = 0
```

`git diff --check`：PASS。

## 构建证据

- Editor final：`5 actions / 7.30s / exit 0`，日志 SHA-256 `174387E46D79B62E45E89AF4B2FC3DE3E62699F7ECAE8FF0980A89D24874BF6C`；
- Game final：`12 actions / 42.28s / exit 0`，日志 SHA-256 `B23C1558C224D493FBC3FF0C03766FA27B4C4A5915469A3DCD0F099231A9B3CB`；
- `UnrealEditor-demo_map.dll`：`11791360` bytes，SHA-256 `452456AD45557274128BC15092029EFAECCB5759C350B5E3B0731F61B6A1BDF0`；
- `demo_map.exe`：`352899072` bytes，SHA-256 `89549F04F6E6934227E531B7D07855A12B4F89EE0692C5A285A8A9C07B611FA5`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.25 可在现有 frozen receipt／lease 上增加第一个纯值 consumer projection 与可撤销 handle，但不得绕过 ProductHost/ledger authority，也不得让 Remove 重新求值。GAS、Actor mutation、persistence 与正式数值资产继续后置。
