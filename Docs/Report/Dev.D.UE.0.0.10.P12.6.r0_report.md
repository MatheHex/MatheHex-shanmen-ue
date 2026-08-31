# Dev.D.UE.0.0.10.P12.6.r0 Report

## 1. 结论

P12.6 已完成并通过 P 阶段门禁。

本阶段在 P12.5 不含数值的 SwordRhythm contribution evidence 之上，建立了 Run/Owner/timeline-scoped、幂等、一次性绑定“下一次 BasicSword activation”的纯值账本。账本记录每一次 BasicSword 观察，即使当时没有 pending contribution，因此迟到事实不能回填旧动作；精准连剑事实也不能绑定到产生它的同一 Activation。

最终结果：

- 新增不可变 binding scope 与 self-validating binding receipt；
- 新增 caller-clocked pending contribution ledger；
- exact pending、bound contribution 和 target observation replay 均幂等；
- cross-scope、过期事实、时间回退、Activation 冲突全部 fail closed；
- focused `4/4`；CombatRuntime `118/118`；0.0.10 全量 `562/562`；
- 7 份 Automation 日志原始合计 `709 Success / 0 Fail`；
- changed-file gate：`Changed=3 / Rules=2 / Required=6 / Logs=7`；
- regression gate self-test：`180/180`；
- `git diff --cached --check`、JSON、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 账本作用域与身份

`FShanmenSwordRhythmContributionBindingScope` 冻结 Run、Owner 与 caller-owned timeline，使用确定性 `ScopeId` 自校验。贡献和 BasicSword observation 必须同时匹配三项作用域；不存在隐式 World、Actor 或第二时钟。

`FShanmenSwordRhythmContributionBindingReceipt` 冻结：

- binding scope；
- 目标 BasicSword observation；
- 按 `ObservedTick + ContributionId` 规范排序的非空 contribution evidence；
- 由 scope、target observation 与全部 contribution identity 派生的 `ReceiptId`。

Receipt 验证贡献时间不得晚于目标、来源 Activation 不得等于目标 Activation、identity 不得重复，并重新派生自身 ID。

### 2.2 真正的“下一次”

账本通过 `TryObserveBasicSword()` 记录所有目标动作，不把“无 pending”视为可忽略事件。贡献只能由记录之后首次满足 `ObservedTick <= target tick` 的新 Activation 绑定；已经观察过的动作重放不会重新触发绑定。

若精准连剑贡献的来源 Activation 就是当前目标，账本保留该贡献，直到后续 BasicSword。贡献先到或同 tick 的来源 observation 先到，两种交付顺序都会绑定到同一个下一动作和同一个 ReceiptId。

### 2.3 幂等与原子性

- pending contribution exact replay 返回成功但不重复插入；
- bound contribution exact replay 返回成功但不重新进入 pending；
- target observation exact replay 返回原 receipt，或保持“已观察、无绑定”；
- 同一 Activation 携带不同 tick/observation identity 时拒绝；
- 每次新记录或观察先在候选账本上完成，再经完整 `IsValid()` 验证后替换，失败不留下部分状态；
- contribution 只能出现在 pending 或某一 binding receipt 中，不能重复绑定。

## 3. 完整性

新增 4 条 focused tests：

1. `NextAction`：证明空动作仍被记录，随后贡献只绑定下一个合格动作并一次性离开 pending；
2. `DeterministicReplay`：反向插入、pending replay、target replay 与 bound replay 都保持稳定 identity 和数量；
3. `SourceWaitsForNext`：证明精准连剑不能强化自身，且两种同 tick 交付顺序得到相同下一动作 receipt；
4. `FailClosedAtomicity`：拒绝 cross-Run、cross-timeline、迟到旧事实、回退时间与 Activation 冲突，并证明 future evidence 不会提前消费。

测试贡献均由真实 P12.0 rhythm chain 和 P12.5 adapter 生成，没有伪造 private receipt。

## 4. 权威与兼容性边界

- ledger 只绑定证据，不计算 strength、倍率、叠加、衰减、伤害或属性；
- 不改变 P12.0–P12.5 rhythm chain、product Host/Session、presentation 或 contribution source；
- 不改变 P10 SpiritEvasion、P11 WeaponPerfectGuard、Impact、Vitality、inventory、schema 或 GAS；
- 没有 World、Actor、component、Tick、Timer、wall clock、RNG、delegate 或异步任务；
- 生产代码边界扫描 `0`，damage/value 扫描 `0`；
- 反射工件字段均为 private `VisibleAnywhere, BlueprintReadOnly`；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- 新增 binding header：140 行；
- 新增 binding implementation：473 行；
- 新增 binding tests：323 行；
- Regression Map 新增 1 条 source-aware rule；
- self-test 新增 1 个正例和 1 个 fail-closed 负例；
- 本轮代码/映射净增 971 行，不修改既有生产文件。

## 6. Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.SwordRhythmContributionBinding` | 4 | 0 | `1EF6D8B9642435EB0203CAA82F689E091AE483C7BC29A2D00E80C01D3D67CDAE` |
| `Shanmen.0_0_10.CombatRuntime.SwordRhythmContribution` | 7 | 0 | `2B74DF9B0E193927D0AA3115B8D05D133D2DD22926F05386AE415574FC0567D9` |
| `Shanmen.0_0_10.CombatRuntime.SwordRhythm` | 13 | 0 | `C7ABCA152F216FD42E1D1E759F2546ED7C7694F0F5DAA6B087BAFDB4DD227779` |
| `Shanmen.0_0_10.CombatRuntime.BasicSword` | 4 | 0 | `9F6392083E8E03DAE012C7F7851DD6608E9F56498ECBD72461D2FC2B5A6C22D9` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `C01CE25D93455EB84C16700661036A56A6B6C93FB68531969E69784EB84A0669` |
| `Shanmen.0_0_10.CombatRuntime` | 118 | 0 | `24FC3EF604CFC955316CE3978929218231900144B0A8D44B8EAF62F85DD58766` |
| `Shanmen.0_0_10` | 562 | 0 | `C2959F6F093C90122F0B93807054BA8290F6DB1CE862304DB3D5C3C940F95984` |

每份最终日志均有唯一 `RunTests` group、terminal queue-empty、原生退出码 `0`、Fail 0；选定测试阶段 Error/Fatal/Unhandled/Assertion/Ensure 为 0。UE 5.8 启动阶段固定噪声均在本轮 `Cmd: Automation RunTests` 之前，未删除或伪装。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=6 Logs=7
SELF_TEST: PASS 180/180
REGRESSION_MAP_JSON: PASS
BOUNDARY_SCAN: PASS ForbiddenHits=0
DAMAGE_SCAN: PASS ProductionDamageHits=0
git diff --cached --check: PASS (native exit 0)
```

Coverage 日志 SHA-256：`3F1E3E1880570E6FF8DDDFA4B68FBA55DA187F6DD802408F5FA0419B3F19CF49`。Self-test 日志 SHA-256：`4DA856B8D633F0D24C828BC0D929C31B6E9C43491635BCE70624742BDED85403`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor final | Succeeded | 5 / 6.70s | 0 | `4EDDA81484385FD051A8272192D21E9A919324F27DC73A83F99639FE5596B51B` |
| Game final | Succeeded | 4 / 13.70s | 0 | `C8ECED069E6C689CBDC706E410CBCC020564DDB6169EFEFB752C96E31B760BCF` |

最终产物：

- `UnrealEditor-ShanmenCombatRuntime.dll`：1,659,904 bytes，SHA-256 `0C52B24EAB46F5FBC29C7255386BBE51F09CDC5C474F1E66E5D602483A4DD9D4`；
- `demo_map.exe`：354,663,936 bytes，SHA-256 `9333B09200D1DAB77BDF3FE045F03D238A34E7F6864DAFF444C6919AEF0D5BA7`。

构建无源码失败，也没有 C3859、C1076、系统代码 1455 或其它内存／页面文件环境错误。

## 9. 流程与异常

- 首轮实现、编译和 focused/full 证据均通过；
- 静态复审发现 bound contribution 重放虽无副作用但返回 false，不符合调用方可安全重试的强幂等语义；修为返回成功且永不回 pending，并补断言；
- 修正后覆盖重建 Editor/Game，覆盖重跑全部 7 份 Automation、coverage gate 和 self-test；本 Report 只引用修正后 SHA；
- 没有 Automation case、产品源码或环境构建失败。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段纯值账本、静态审查、无头 Automation、回归映射与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P12.7 可由 product-owned Session/Host 把既有 SwordRhythm、PerfectGuard 与 SpiritEvasion receipt 路由到该 ledger，并把有效 binding receipt 交给后续 evaluator；仍不应在路由层定义倍率或直接修改 damage。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-6-sword-rhythm-contribution-binding>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-6-sword-rhythm-contribution-binding/Docs/Report/Dev.D.UE.0.0.10.P12.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-6-sword-rhythm-contribution-binding/Docs/Log/Dev.D.UE.0.0.10.P12.6.r0_log.md>
