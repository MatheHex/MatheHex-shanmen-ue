# Dev.D.UE.0.0.10.P12.9.r0 Report

## 1. 结论

P12.9 已完成并通过 P 阶段门禁。

本阶段在 P12.8 immutable evaluator input 之后建立了独立、版本化、纯函数的 SwordRhythm evaluator policy 与 self-validating receipt。根据 0.0.10 已冻结的“手感优先、数值后置”边界，policy 只把三类已证明 contribution 映射为具名 authored effect token，不提前发明倍率、强度、伤害或属性公式。

最终结果：

- 完整 policy 必须精确覆盖 `PreciseSwordLink`、`PerfectWeaponGuard`、`SpiritEvasion` 三类来源；
- policy 与每条 specification 都绑定 `FShanmenContentStamp`，身份可版本化、可重放；
- authored specification 输入顺序不影响 canonical policy identity；
- evaluator 按 binding 的因果顺序生成 effect，每条 effect 保留 input、specification 与原 contribution；
- 无 contribution 的合法 input 生成合法空 receipt，而不是伪造效果或失败；
- receipt 会重建期望 effect 并核对所有 deterministic ID，篡改或错配 fail closed；
- 没有接入 GameMode、Damage、Vitality、Attribute、inventory、GAS 或表现层；
- focused `4/4`，0.0.10 全量 `566/566`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=2 / Required=8 / Logs=2`；
- regression gate self-test 最终 `182/182`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 版本化 symbolic policy

新增 mutable capture 与 immutable policy/specification 两层结构。每条 specification 只定义：

- contribution kind；
- authored `EffectDefinitionId`；
- 与 policy 相同的 content version/digest；
- 由上述内容派生的 deterministic `SpecificationId`。

policy 必须具备唯一 `PolicyDefinitionId`、有效 content stamp、三类来源各一条且 effect definition 不重复。capture 后按 contribution kind canonical sort，再派生 `PolicyId`，因此 authored 数组顺序不会改变同一 policy 的身份。

### 2.2 纯函数 evaluator

`FShanmenSwordRhythmEvaluator::Evaluate()` 只消费 P12.8 的 immutable input 与本阶段 policy：

1. 分别验证 input 与 policy；
2. 沿 contribution binding 的既有因果顺序查找 specification；
3. 为每条 contribution 生成 immutable symbolic effect；
4. 生成并自校验 evaluation receipt；
5. 以明确 status/diagnostic 返回，不修改任何产品状态。

effect identity 同时包含 evaluator input、specification 与 contribution identity。独立 `TryCreate()` 还要求 contribution 确实属于该 input 的 binding，不能把任意合法 contribution 注入另一份 input。

### 2.3 immutable receipt

receipt 冻结完整 policy、input 与 ordered effects。`IsValid()` 会从 policy/input 重新构造 canonical effects，逐项核对 effect identity、input identity、specification identity、contribution identity与最终 `ReceiptId`。

没有 contribution binding 时，receipt 仍有效且 `NumEffects()==0`。这明确区分“没有可映射事实”和“输入或 policy 无效”。

## 3. 完整性

新增四个 focused Automation contract：

1. `PolicyContract`：三类来源完整覆盖、顺序无关 identity、missing/duplicate kind/duplicate effect fail closed；
2. `SymbolicMapping`：使用真实 PreciseSwordLink、PerfectWeaponGuard、SpiritEvasion receipt，验证三条映射与因果顺序；
3. `EmptyAndReplay`：空 binding 合法、相同 evidence 与 policy 精确重放；
4. `FailClosed`：无效 input、无效 policy、错误 contribution/specification kind 组合均不产生有效 receipt/effect。

测试 fixture 通过现有 BasicSword rhythm、WeaponGuard timing evaluator 与 SpiritEvasion projection 构造真实来源 receipt，不以手填假的 contribution 代替上游契约。

## 4. 权威与兼容性边界

- P12.8 `FShanmenSwordRhythmEvaluationInput` 的既有字段和语义保持不变；
- evaluator 是纯 translator，不持有 ledger、clock、World、Actor、Component 或 GameMode 状态；
- policy 只描述 symbolic effect vocabulary，不内置产品默认 policy，也不应用 effect；
- 未新增 `float`/`double`、magnitude、multiplier、strength、damage 或属性修改字段；
- 未改变 SwordRhythm ProductSession、contribution binding、动作、资源或伤害权威；
- 所有 immutable 输出字段保持 private + `BlueprintReadOnly`；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `ShanmenSwordRhythmEvaluation.h/.cpp`：新增 effect specification、versioned policy、evaluated effect、receipt、result status 与纯 evaluator；
- `ShanmenSwordRhythmEvaluationTests.cpp`：新增四个 focused contract；
- `ShanmenRegressionMap.json`：把 evaluator source/test 映射到 focused group 及既有七组上游/产品契约；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：扩展正例与缺证据反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+1177 / -6`，其中新测试文件 477 行。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.SwordRhythmEvaluation` | 4 | 0 | 0 | `E08F2C8AB62E36732558291A59588BD33F67786EB97AA423FD73CE6E2643D887` |
| `Shanmen.0_0_10` | 566 | 0 | 0 | `496CFC4B6C3B6F20FD0F3E2593DEAA27CD2F669059792863321FEF0CC34B2642` |

两份日志都有唯一 `RunTests` group、native terminal success、Fail 0；最后一个 `Cmd: Automation RunTests` 之后的 Fatal/Unhandled/Ensure/Automation failure 为 0。focused 与 full 原始合计 `570 Success / 0 Fail`，全量唯一用例为 566。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=8 Logs=2
SELF_TEST: PASS 182/182
POLICY_BOUNDARY_SCAN: PASS no numeric strength/multiplier/damage fields
MODULE_BOUNDARY_SCAN: PASS no demo_map/UWorld/AActor/application/RNG/timer dependency
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

回归映射要求 evaluator 改动具备 focused evaluation、ProductSession、binding、contribution、rhythm、BasicSword、ActionLifecycle 与 broad CombatRuntime 证据。focused 与 full 日志共同满足全部 8 组要求。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 32 / 141.28s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.94s | 0 |
| Game final | Succeeded | 29 / 121.34s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,068,288 bytes，SHA-256 `3E2A2912D5DE153A3FE1549E9A141AE6679C3FFF62538177513D070D2F0B21B7`；
- `demo_map.exe`：354,750,976 bytes，SHA-256 `8A86A490F006A9F863E0AE5F37754BEE8A2FFDDF3AACFC713650CEF3D8E64461`。

构建未出现 C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

- regression self-test 首轮新增了一条错误反例：它假设 broad `Shanmen.0_0_10.CombatRuntime` 不能覆盖 focused child group；但门禁的既有语义明确允许父组覆盖子组，因此该轮退出 `1`；
- 删除错误反例后，保留有效的“focused + ProductSession + binding 不能替代 source/rhythm/action/broad”反例，最终 `182/182`；
- 一次验证 wrapper 指向本机不存在的固定 `C:\Program Files\PowerShell\7\pwsh.exe`，项目脚本尚未启动；确认当前 shell 已是 pwsh `7.6.4` 后直接执行，结果通过；
- 产品源码、UHT、focused/full Automation 与双目标构建没有失败或重跑；
- raw Automation 日志仅作为本地可复核证据，不纳入 Git；Git 中提交本 Report/Log 与精确源码/流程文件。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段纯函数契约、静态审查、无头 Automation、回归映射与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

建议 P12.10：让产品拥有的配置/Session 安装一个 canonical symbolic policy，并只读评估 P12.8 的 `LastEvaluationInput`，向表现或后续消费者暴露 immutable receipt；在数值设计正式冻结前，仍不定义 multiplier、不修改 attribute、不提交 damage。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-9-sword-rhythm-evaluator-policy>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-9-sword-rhythm-evaluator-policy/Docs/Report/Dev.D.UE.0.0.10.P12.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-9-sword-rhythm-evaluator-policy/Docs/Log/Dev.D.UE.0.0.10.P12.9.r0_log.md>
