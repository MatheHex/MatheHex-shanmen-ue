# Dev.D.UE.0.0.10.P12.7.r0 Report

## 1. 结论

P12.7 已完成并通过 P 阶段门禁。

本阶段把 P12.5 的三类 contribution evidence 与 P12.6 的 next-BasicSword ledger 收进既有 `Fdemo_mapShanmenSwordRhythmProductSession`，没有建立第二个产品状态系统。Session 现在能够接收真实类型的 PerfectGuard / SpiritEvasion receipt、自动捕获 PreciseLinked receipt，并在观察 BasicSword 时返回可选的 immutable binding receipt。

最终结果：

- 既有 SwordRhythm ProductSession 成为唯一产品级 contribution ledger 所有者；
- ledger scope 从首个合法事实冻结 Run / Owner / canonical timeline，支持 source-first 与 action-first；
- 当前 BasicSword 先消费此前 pending，再把本次精准连段登记给下一动作，禁止自绑定；
- 保留旧 BasicSword API，现有 GameMode 与表现层无需同步改写；
- typed overload 可把有效 binding receipt 交给后续 evaluator；
- focused `2/2`，0.0.10 全量 `562/562`；
- 9 份 Automation 日志原始合计 `610 Success / 0 Fail`；
- changed-file gate：`Changed=5 / Rules=1 / Required=13 / Logs=9`；
- regression gate self-test：`180/180`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 唯一产品所有者

`Fdemo_mapShanmenSwordRhythmProductSession` 继续拥有 canonical rhythm Host、latest receipt 与 presentation state，同时新增唯一 `FShanmenSwordRhythmContributionBindingLedger`。没有新建平行 Session、GameMode map、Actor component 或第二时钟。

Binding scope 延迟到首个合法 contribution 或 BasicSword observation 时创建，因为现有 `TryBegin(RunId)` 不携带 Owner。首个事实冻结：

- Session 当前 Run；
- action snapshot 的 Owner；
- 由 Run 确定的 canonical fixed timeline。

后续事实若跨 Run、Owner 或 timeline，原子拒绝。该策略既兼容现有 API，也允许 PerfectGuard / SpiritEvasion 在第一剑之前先进入 pending。

### 2.2 三类来源路由

- `TryRecordPerfectWeaponGuardContribution()` 只接受 `Perfect` timing projection；ordinary guard 失败并清空输出；
- `TryRecordSpiritEvasionContribution()` 接受有效 evasion projection 与 caller-owned immutable timeline sample；
- `TryObserveExecutedBasicSword()` 自动把 `PreciseLinked` rhythm receipt 捕获为 precise-link contribution；
- source receipt 的 exact replay 由 P12.6 ledger 幂等吸收，不重复 pending 或 bound 计数。

这三条入口只把已经成立的 receipt 转换为 evidence，不重新判断 Guard / Evasion / rhythm，也不读取 World 或瞬时输入。

### 2.3 目标绑定顺序

增强 overload 在同一候选 Session 中按以下顺序执行：

1. 既有 Host 接受并冻结 BasicSword observation；
2. binding ledger 观察该目标并消费此前合格 pending；
3. 若该目标本身形成 `PreciseLinked`，再把它登记为下一动作的 pending；
4. presentation projector 生成原有只读状态；
5. 完整 `IsValid()` 通过后一次性提交候选 Session。

因此精准连段绝不强化产生它的自身 Activation。无 pending 时输出 binding receipt 保持无效；有 pending 时返回稳定 receipt。旧 overload 委托给增强 overload 并丢弃返回值，但仍维护完整 ledger，保证现有调用方行为兼容。

### 2.4 原子性与重放

- source receipt exact replay 保持同一 ContributionId；
- target exact replay 返回同一 BindingReceiptId；
- ordinary guard、foreign Run 与 scope 冲突不改变 pending；
- Session `IsValid()` 交叉核验 Host observation 数、last observation、scope Run 与 canonical timeline；
- teardown/reset 同时清除 Host、presentation、receipt 与 contribution ledger；
- 所有 mutation 均先复制候选，再验证并替换。

## 3. 完整性

扩展现有真实产品生命周期用例，覆盖：

1. 非执行 BasicSword 不创建 scope；
2. 有效 PerfectGuard 与 SpiritEvasion receipt 在第一剑前进入同一 ledger；
3. source exact replay 幂等；
4. ordinary guard 与 foreign Run evidence 原子拒绝；
5. 第一剑一次绑定两条先前防御事实；
6. 第二剑在 precise boundary 只产生下一动作 pending，不自绑定；
7. 第三剑绑定第二剑的 precise-link evidence；
8. 第三剑 exact replay 返回同一 binding receipt；
9. Run teardown 清空完整产品 Session。

BasicSword 由真实 `Fdemo_mapCombatRunCoordinator` 与 fixed timeline 产生；Guard / Evasion receipt 由对应 Runtime action/window/timing authority 产生，没有伪造 private receipt 字段。

## 4. 权威与兼容性边界

- 不定义 contribution strength、倍率、叠加、衰减、伤害或属性；
- 不修改 Impact、Vitality、inventory、schema、GAS、输入、动画或表现内容；
- 不修改 GameMode；其旧 overload 保持源码与行为兼容；
- PerfectGuard / SpiritEvasion 的 live product call site 尚未接线，本阶段只完成 typed Session seam；
- 生产文件无 `UWorld`、`AActor`、`ApplyDamage`、timer 或 RNG 依赖；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmProductSession.h/.cpp`：新增 typed source routes、binding-return overload、scope freeze 与一致性校验；
- `demo_mapShanmenSwordRhythmProductSessionTests.cpp`：扩展真实 receipt / BasicSword lifecycle 覆盖；
- `ShanmenRegressionMap.json`：Session rule 新增 binding、source 与 active-defense 依赖组；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：同步正例证据；
- Report/Log 生成前 5 个代码/流程文件净变更 `+563 / -8`。

## 6. Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 2 | 0 | `C2338F01F246AE55994D220CD8DFDD8D65DF98750C08B61621F8920C0F590232` |
| `Shanmen.0_0_10.CombatRuntime.SwordRhythmContributionBinding` | 4 | 0 | `5D79DA09B22E3886E3714A02AD19E6764F6272EA4954C486CBF04213B7DFF04E` |
| `Shanmen.0_0_10.CombatRuntime.SwordRhythmContribution` | 7 | 0 | `AD05ABC32980519EF1EE61CCF4A3EF76CC4A9AF4A24EA187B7C0ED5A77567944` |
| `Shanmen.0_0_10.CombatRuntime.SwordRhythm` | 13 | 0 | `916C184C54A7A71725DE7869F4DCE8AB2867157ED73B512E18A3913396F2EC7C` |
| `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard` | 6 | 0 | `E646A8DF0B706AC720978D899B9FEA1EEFE49C7C631A614061D7DDBCE448CAA5` |
| `Shanmen.0_0_10.CombatRuntime.SpiritEvasion` | 11 | 0 | `FED73B68FC4A9CB82CCA5670CB4C56342272BB3BE6E4E29F86D9AB4DD88D6A72` |
| `Shanmen.0_0_10.CombatRuntime.BasicSword` | 4 | 0 | `D57DE71717356D5C3F3891CC75E3E2460CA906D770CBEEAF78DDC4FDDE9500B1` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `2336816169535E895A84961550F88FCD6B32034D283293100BB70AE810885CE0` |
| `Shanmen.0_0_10` | 562 | 0 | `A414AE9B32718C447FA6CC77E89274C8BBD0BB8C428F146D11C300AD4AB56269` |

每份日志均有唯一 `RunTests` group、queue-empty/terminal success、进程原生退出码 `0`、Fail 0；选定测试阶段 Fatal/Unhandled/Ensure 为 0。UE 5.8 启动期固定噪声位于本轮 `Cmd: Automation RunTests` 之前，未计入选中阶段。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=13 Logs=9
SELF_TEST: PASS 180/180
BOUNDARY_SCAN: PASS no World/damage/RNG/timer dependencies
git diff --check: PASS (native exit 0)
```

Coverage 首次外层调用把 PowerShell 数组错误展开为位置参数，脚本在读取证据前失败；改为 pwsh 内部构造数组后通过。该问题属于验证命令包装，不是源码、Automation 或构建失败。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor compile | Succeeded | 29 / 110.18s | 0 |
| Editor final | Succeeded | 4 / 5.36s | 0 |
| Game final | Succeeded | 26 / 103.39s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,060,608 bytes，SHA-256 `2A491595608ADF81D6936D105659B825F53F904AB74088E952CA8E1C2421B520`；
- `demo_map.exe`：354,681,344 bytes，SHA-256 `7BE190178C20D4FCCD06C5843DCDB3F577E024916401F03970D9442C0CE7046D`。

构建无源码失败，也没有 C3859、C1076、系统代码 1455 或其它内存／页面文件环境错误。

## 9. 流程与异常

- 实现、首次 Editor 编译、focused、依赖组、全量 Automation 与最终构建均首次通过；
- 唯一修正是 coverage gate 外层 PowerShell 数组传参方式，未修改产品行为；
- 没有 Automation case、产品源码或环境构建失败；
- 只暂存并提交本阶段明确文件，不使用 `git add .`。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段产品 Session 路由、静态审查、无头 Automation、回归映射与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P12.8 应把现有 WeaponGuard defense result 与 SpiritEvasion active projection 的 live product call site 接入这两个 typed seam，并把有效 binding receipt 交给独立 evaluator 输入；仍不在路由层定义倍率或直接修改 damage。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-7-sword-rhythm-product-contribution-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-7-sword-rhythm-product-contribution-route/Docs/Report/Dev.D.UE.0.0.10.P12.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-7-sword-rhythm-product-contribution-route/Docs/Log/Dev.D.UE.0.0.10.P12.7.r0_log.md>
