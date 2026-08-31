# Dev.D.UE.0.0.10.P12.0.r0 Development Log

## 1. 目标

开始 0.0.10 太极剑节奏纵切的第一份真实核心：在不接入输入、动画、GameMode 或数值增益的前提下，把连续 `Combat.Action.Sword.Basic01` 的时间关系固化为确定性、可重放、可审计的回执。

## 2. 产品边界

本阶段只接受调用方已经冻结的 BasicSword action snapshot、调用方拥有的 timeline identity 和单调 tick。运行时不读取帧计数、wall clock、World、Actor、Timer 或 RNG，也不推测动画通知时间。

窗口宽度完全由内容定义注入；本阶段没有烧入攻速、伤害倍率、连击上限或动作时长。太早和太晚的输入不会被伪装成失败动作，而是形成可审计的 `RestartedEarly` / `RestartedLate` 新锚点；只有半开区间 `[open, close)` 形成 `PreciseLinked`。

## 3. 实现

新增：

- `FShanmenSwordRhythmDefinition`：冻结 Tai Chi style、BasicSword action、规则 ID 与半开时间窗口，并派生 deterministic `DefinitionId`；
- `FShanmenSwordRhythmObservation`：冻结 action、timeline 与 input tick，并派生 deterministic `ObservationId`；
- `FShanmenSwordRhythmReceipt`：记录 previous/current observation、旧／新链计数和 timing band，并自验证 `ReceiptId`；
- `FShanmenSwordRhythmChain`：接受首次启动、精准衔接、早／晚重启，保留 activation ledger，并对 exact replay 返回原回执。

同一 ActivationId 配不同 tick、跨 Run、跨 weapon instance、跨 content stamp、跨 timeline 或 tick 回退均原子拒绝，不改变现有锚点和计数。

## 4. 测试

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.SwordRhythm` | 6 | 0 | `8647AE769722CC2D5F08CDB254748D1CCFAC43D5D1C4E1F46CCA8F66351D4CF3` |
| `Shanmen.0_0_10.CombatRuntime.BasicSword` | 4 | 0 | `71208E263EEEFC9703388D5638EC8AE7F8A605B5E3EA065232638994204AE3C3` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `BFC66642D69933155D78D66634548B443DC10A3DB9FBC9BB0096DEC77623B65F` |
| `Shanmen.0_0_10.CombatRuntime` | 111 | 0 | `DC70A3FEE9A8280C749481E6A264EC670AFA174FFFBD6991766975AD4E9470DA` |
| `Shanmen.0_0_10` | 547 | 0 | `1193A09EA18E04DF024A67D0B12F6D08962DDDD405B47775308F845663BED5CF` |

五份最终 Automation 日志原始合计 `669 Success / 0 Fail`；focused 与 broad 均包含于 full，按 test identity 去重为 `547`。

## 5. 路径门禁

新增 `SwordRhythm` 映射，三个新源码路径必须同时具备：

- `Shanmen.0_0_10.CombatRuntime.SwordRhythm`
- `Shanmen.0_0_10.CombatRuntime.BasicSword`
- `Shanmen.0_0_10.CombatRuntime.ActionLifecycle`
- `Shanmen.0_0_10.CombatRuntime`

最终结果：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=4 Logs=4
SELF_TEST: PASS 168/168
STATIC_REVIEW: PASS AddedSourceLines=1022 ForbiddenHits=0
git diff --check: PASS (native exit 0)
```

coverage 日志 SHA-256：`0D1E58D875A0D4FE7AC745BCEB04AC2B45AD2C27E0E0EBA2D3C59CDB11612F2F`；self-test 日志 SHA-256：`015BBBFC38CA28CACFE378E7684385DD369C9AD2D6B12B599971E9B85F47A68B`。

## 6. 构建

- Editor：`8/8`，原生退出 `0`，37.89 秒，日志 SHA-256 `6291D4154D0DDC80BA8673D6FD75BB5158723D3C70C46FC964145CDF7B3E75A5`；
- Game：`5/5`，原生退出 `0`，33.23 秒，日志 SHA-256 `FF31072E84BFE2175294243058C029580E453986D950E0C02785645DF12DBF31`。

Win64 SDK `10.0.22621.0` 为 VALID；没有 C3859、C1076、系统代码 1455 或其它源码／环境构建失败。

## 7. 真实异常与修复

产品源码、Automation 与构建均未发生失败。回归 gate 的前两次调用发生流程错误：

1. 首次以 `pwsh -File` 传递数组，PowerShell 将后续日志路径误绑定为位置参数；
2. 修正参数绑定后，gate 正确拒绝了带 `;Quit` 后缀的非规范 RunTests group，报告四组 required group 缺失。

随后使用标准 `Automation RunTests <group>` 与 `TestExit=Automation Test Queue Empty` 重跑五组日志；最终 gate、日志 SHA 与测试计数均基于重跑后的规范证据。两次错误属于证据命令封装，不是测试失败或产品源码失败。

## 8. 边界

未修改 BasicSword 执行、ActionOrchestrator、CombatCore、Impact、damage、vitality、inventory、schema、输入或产品 Host。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
