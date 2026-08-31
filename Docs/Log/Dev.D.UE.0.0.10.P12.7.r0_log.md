# Dev.D.UE.0.0.10.P12.7.r0 Development Log

## 1. 目标

让既有 SwordRhythm ProductSession 成为 P12.5 contribution evidence 与 P12.6 next-action binding ledger 的唯一产品所有者：接收 typed Guard/Evasion receipt，自动记录 precise-link evidence，并在后续 BasicSword 上返回 immutable binding receipt；仍不定义数值或修改伤害。

## 2. 实现

- Session 内置一个 Run/Owner/canonical-timeline binding ledger；
- scope 由首个合法 source 或 target 延迟冻结，保留既有 `TryBegin(RunId)` 兼容性；
- 新增 PerfectGuard 与 SpiritEvasion typed contribution routes；
- 新增返回 optional binding receipt 的 BasicSword overload；
- 旧 overload 委托新路径，现有 GameMode/表现调用无需改写；
- BasicSword 先消费既有 pending，再登记自身 precise-link 给下一动作；
- source/target replay 幂等，ordinary guard、foreign Run 与 scope conflict 原子失败；
- Session 完整性校验覆盖 ledger scope、observation count 与 last observation；
- teardown 同时清理 product Host、presentation 与 contribution ledger。

## 3. Automation

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `SwordRhythmProductSession` | 2 | 0 | `C2338F01F246AE55994D220CD8DFDD8D65DF98750C08B61621F8920C0F590232` |
| `SwordRhythmContributionBinding` | 4 | 0 | `5D79DA09B22E3886E3714A02AD19E6764F6272EA4954C486CBF04213B7DFF04E` |
| `SwordRhythmContribution` | 7 | 0 | `AD05ABC32980519EF1EE61CCF4A3EF76CC4A9AF4A24EA187B7C0ED5A77567944` |
| `SwordRhythm` | 13 | 0 | `916C184C54A7A71725DE7869F4DCE8AB2867157ED73B512E18A3913396F2EC7C` |
| `WeaponPerfectGuard` | 6 | 0 | `E646A8DF0B706AC720978D899B9FEA1EEFE49C7C631A614061D7DDBCE448CAA5` |
| `SpiritEvasion` | 11 | 0 | `FED73B68FC4A9CB82CCA5670CB4C56342272BB3BE6E4E29F86D9AB4DD88D6A72` |
| `BasicSword` | 4 | 0 | `D57DE71717356D5C3F3891CC75E3E2460CA906D770CBEEAF78DDC4FDDE9500B1` |
| `ActionLifecycle` | 1 | 0 | `2336816169535E895A84961550F88FCD6B32034D283293100BB70AE810885CE0` |
| `Shanmen.0_0_10` | 562 | 0 | `A414AE9B32718C447FA6CC77E89274C8BBD0BB8C428F146D11C300AD4AB56269` |

正式日志原始合计 `610 Success / 0 Fail`；全量唯一用例 `562`。全部最终进程原生退出 `0`，选中测试阶段错误标记 `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=13 Logs=9
SELF_TEST: PASS 180/180
BOUNDARY_SCAN: PASS no World/damage/RNG/timer dependencies
git diff --check: PASS (native exit 0)
```

Coverage 首次外层调用发生 PowerShell 数组位置参数错误；在 pwsh 内构造数组后通过。Automation 证据未受影响。

## 5. 构建

- Editor compile：29 actions / 110.18s / exit `0`；
- Editor final：4 actions / 5.36s / exit `0`；
- Game final：26 actions / 103.39s / exit `0`；
- Editor product DLL：13,060,608 bytes / SHA `2A491595608ADF81D6936D105659B825F53F904AB74088E952CA8E1C2421B520`；
- Game EXE：354,681,344 bytes / SHA `7BE190178C20D4FCCD06C5843DCDB3F577E024916401F03970D9442C0CE7046D`；
- 无源码或环境构建失败。

## 6. 修改与兼容性

Report/Log 前 5 个代码/流程文件净变更 `+563 / -8`。不修改 GameMode、Impact、Vitality、inventory、schema、GAS、输入、动画或 damage；旧 BasicSword Session API 保留。Guard/Evasion live caller 接线留给下一阶段。

## 7. 流程与边界

实现与所有产品验证首次通过；只有 coverage wrapper 参数形式做了纠正。长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-7-sword-rhythm-product-contribution-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-7-sword-rhythm-product-contribution-route/Docs/Report/Dev.D.UE.0.0.10.P12.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-7-sword-rhythm-product-contribution-route/Docs/Log/Dev.D.UE.0.0.10.P12.7.r0_log.md>
