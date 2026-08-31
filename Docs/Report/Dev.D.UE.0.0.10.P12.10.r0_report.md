# Dev.D.UE.0.0.10.P12.10.r0 Report

## 1. 结论

P12.10 已完成并通过 P 阶段门禁。

本阶段把 P12.9 的纯 SwordRhythm symbolic evaluator 接入唯一产品 `ProductSession`：产品配置安装 canonical policy，accepted `BasicSword` 在同一候选状态中生成 immutable evaluation receipt，并向既有 presentation read model 只读投影 receipt/policy identity 与 ordered effect token。没有新增第二套 Session、GameMode 权威、数值强度、属性修改或 damage commit。

最终结果：

- canonical product config 升级为 `0.0.10.P12.10` / r2，并拥有一份完整的三来源 symbolic policy；
- config identity 纳入 policy identity，配置与评价规则不能被静默拆换；
- accepted `BasicSword` 原子形成 rhythm receipt、binding receipt、evaluation input、evaluation receipt 与 presentation state；
- Guard + Evasion 的真实首动作产生两个有序 effect token，合法空动作产生零 token，下一次 PreciseSwordLink 产生一个 precise token；
- 同一动作重放保持 evaluation receipt identity 稳定，teardown 清除 evaluation/presentation 状态；
- Session 只读暴露完整 immutable receipt；Blueprint presentation state 只暴露 receipt ID、policy ID 与 ordered effect definition IDs；
- focused route/session/presentation/event/evaluator 全部通过，0.0.10 全量 `566/566`，Fail `0`；
- changed-file gate：`Changed=8 / Rules=2 / Required=16 / Logs=6`；
- regression gate self-test `182/182`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 产品拥有 canonical symbolic policy

`Fdemo_mapShanmenSwordRhythmProductConfig` 现拥有 canonical `FShanmenSwordRhythmEvaluationPolicy`。r2 配置固定：

- policy definition：`Combat.Style.Sword.Taiji01.SymbolicEffects.r1`；
- PreciseSwordLink effect：`Combat.Style.Sword.Taiji01.Effect.PreciseFlow`；
- PerfectWeaponGuard effect：`Combat.Style.Sword.Taiji01.Effect.BorrowedForce`；
- SpiritEvasion effect：`Combat.Style.Sword.Taiji01.Effect.RedirectedMomentum`。

配置构造与 `IsValid()` 都校验 canonical policy；`ConfigId` 派生包含 `PolicyId`。因此内容版本、产品配置和评价词表形成一条可审计身份链。

### 2.2 原子 evaluation route

accepted `BasicSword` 的候选提交顺序为：

1. 由现有 Host 生成 rhythm receipt；
2. 由现有 binding ledger 绑定当前动作已证明的 contribution；
3. 捕获 immutable evaluation input；
4. 使用产品 config 的 canonical policy 运行 P12.9 pure evaluator；
5. 把 rhythm/evaluation receipts 投影为 presentation state；
6. 只有上述步骤全部有效时，候选 Host、ledger、receipt、input、evaluation 与 presentation 一次性提交给 Session。

任一步骤失败都不会留下半提交的 evaluation 或 presentation 状态。新增 precise contribution 仍按既有“本次证明、下次消费”节奏进入 ledger；当前评价不会错误提前消费它。

### 2.3 只读 presentation seam

既有 presentation state 增加：

- `EvaluationReceiptId`；
- `EvaluationPolicyId`；
- ordered `EffectDefinitionIds`。

它不复制完整 evaluator policy，不持有可变 evaluation 权威，也不解释或应用 token。现有 GameMode 已通过 `const ProductSession&` 暴露唯一 Session，因此本阶段没有修改 GameMode，也没有新建 query service。

## 3. 完整性

真实产品 route contract 覆盖：

1. canonical config r2 的三来源 policy 与 effect definition IDs；
2. 首个 accepted BasicSword 把真实 PerfectWeaponGuard + SpiritEvasion evidence 映射为两个 ordered effects；
3. 第二个 accepted action 在没有 contribution 时生成合法空 receipt/state；
4. replay 保持 receipt identity 稳定且不重复消费；
5. 第三个 accepted action 消费上一拍 precise contribution，映射 `PreciseFlow`；
6. teardown 同时清空 evaluation receipt 与 presentation state；
7. presentation projector 拒绝无效 receipt、policy/config 错配与 rhythm/evaluation input 错配；
8. presentation event consumer 继续只消费 self-validating read model。

这些验证使用真实 `ProductSession`、BasicSword host、binding ledger、guard/evasion receipts 与 pure evaluator，不以手填 read model 替代产品链路。

## 4. 权威与兼容性边界

- P12.9 evaluator/policy 仍是纯函数契约，产品 Session 只安装并调用它；
- ProductSession 是唯一运行期聚合权威，没有新增第二套 ledger、policy store 或 presentation service；
- presentation state 只投影 symbolic identity/token，不计算 magnitude、multiplier、strength、damage 或 attribute delta；
- 未引入 `UWorld`、`AActor`、RNG、timer、GameplayEffect 或产品效果应用；
- GameMode、动作权威、资源、Vitality、inventory、GAS 与 damage pipeline 均未改变；
- 所有新增输出保持 private construction 与只读访问；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmProductSession.h/.cpp`：产品 config r2、canonical policy、evaluation receipt 状态与原子接线；
- `demo_mapShanmenSwordRhythmPresentation.h/.cpp`：evaluation identity/effect token 的只读投影与 fail-closed 校验；
- `demo_mapShanmenSwordRhythmProductSessionTests.cpp`：真实三来源 lifecycle、空评价、重放、precise 延迟消费与 teardown；
- `demo_mapShanmenSwordRhythmPresentationTests.cpp`：evaluation projection 与无效 receipt 拒绝；
- `ShanmenRegressionMap.json`：ProductSession/Presentation 改动必须具备 route、pure evaluator、run/host/read-model/event 证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 route 正例与缺证据反例；
- Report/Log 生成前 8 个代码/流程文件净变更 `+458 / -41`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `2AB09F06668A64B8A40E96498E10D95CE2A192E224F0C36ADAE482C7EC649998` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `01019067C7D40C3DDB8E1102B631D505B93C0EAC017D19049C324F922A3F4D06` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `1F3FDC86B474B8F16DD7C4E2DEA963D8F0CB3BEC9137A1CFD03343D7FCECDE0B` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentationEvent` | 2 | 0 | 0 | `BFD52ED9B42A97851EA6538E6F42A456BB173113B34B9121965FC4D924648A32` |
| `Shanmen.0_0_10.CombatRuntime.SwordRhythmEvaluation` | 4 | 0 | 0 | `0C8597C79AB85EB1ADFDC3755777F5808756C6B1EB1A34476D6A48D410D61966` |
| `Shanmen.0_0_10` | 566 | 0 | 0 | `3E7CDBD36C909F3F4A79503A7E26600A28C26AF3397FE97C7BE57E5CE8F2D6B1` |

`SwordRhythmPresentation` 前缀自然包含两个 `PresentationEvent` 子测试，因此显示 4 项；独立 Event 组再次验证精确子边界。六份日志均有唯一选定 group、native terminal success、Fail 0；最后一个 `Cmd: Automation RunTests` 之后的 Fatal/Unhandled/Ensure/Automation failure 为 0。全量唯一用例为 566。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=16 Logs=6
SELF_TEST: PASS 182/182
PRODUCT_BOUNDARY_SCAN: PASS no numeric strength/multiplier/damage/application fields
ENGINE_BOUNDARY_SCAN: PASS no UWorld/AActor/RNG/timer dependency
AUTHORITY_SCAN: PASS GameMode unchanged; one ProductSession authority
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

回归映射要求本轮 ProductSession 与 Presentation 改动同时具备 pure evaluator、三来源 product route、ProductSession、Run/Host、read model 与 event consumer 证据。focused 与 full 日志共同满足全部 16 个要求。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 30 / 134.17s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.92s | 0 |
| Game final | Succeeded | 27 / 112.67s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,083,136 bytes，SHA-256 `0E1930AD90A26B3AB0B9D5A81C65C1FCCA9AE7B1CCB9CB31875B0E80C36151F0`；
- `demo_map.exe`：354,760,704 bytes，SHA-256 `EC77300575A3CED9C4C548A42CC28AA14DCEC066467EF4B043D6BEA0DFA19861`。

构建未出现 C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

- regression self-test、JSON parse、focused Automation、full Automation、source-aware gate 与双目标源码构建均首轮通过；
- 初始 Editor 构建完成 UHT/generated-code 与 30 个单并发 action；最终 Editor 验证为 up to date；
- full Automation 执行期间保持单一进程，没有重复启动回归；
- UE 启动阶段既有 13 条、位于选定 `RunTests` 之前的 automation condition diagnostics 不计入选定测试阶段；最后一个选定命令之后错误计数为 0；
- raw Automation 日志仅作为本地可复核证据，不纳入 Git；Git 中提交本 Report/Log 与精确源码/流程文件。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段产品接线、静态审查、无头 Automation、回归映射与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

建议 P12.11：定义 typed presentation effect cue adapter，把本阶段的 symbolic effect definition IDs 映射为非权威 visual/audio cue command 与幂等 event identity；仍不定义 gameplay magnitude，不修改 attribute，不提交 damage。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-10-sword-rhythm-evaluation-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-10-sword-rhythm-evaluation-product-route/Docs/Report/Dev.D.UE.0.0.10.P12.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-10-sword-rhythm-evaluation-product-route/Docs/Log/Dev.D.UE.0.0.10.P12.10.r0_log.md>
