# Dev.D.UE.0.0.10.P10.0.r0 Development Log

## 目标

在 0.0.10 的“手感优先、数值后置”原则下，建立第一条反应式主动闪避纯契约：闪避防御必须绑定动作 commit、只在 Active phase 生效，并复用 CombatCore 的统一 Impact/Defense 结算；不提前发明移动参数、无敌帧、输入层或 SpiritEnergy 产品账本。

## 审计结论

1. CombatCore 已有 `PreventAll`、`FShanmenDefenseOrder::Avoidance` 与 `Shanmen.Defense.Evade`，无需新增第二套结算器；
2. P3 `FShanmenActionOrchestrator` 已把 `Startup -> Active` 定义为唯一 commit point，并能作为短窗口的唯一生命期权威；
3. 旧产品 DodgeChance 是随机属性，不等于玩家主动触发、按动作 phase 生效的灵力闪避；
4. 产品层只有 typed `Shanmen.Resource.SpiritEnergy` channel，没有已冻结的 writable balance owner、revision、恢复或持久化 seam；
5. 在该前提下直接接产品输入/扣费会创建临时 float 或第三套账本；
6. 因此 P10.0 只冻结 action-bound evasion window 与 DefenseLayer projection。

## 设计决策

1. 定义只接受 canonical `Combat.Action.Spell.SpiritEvasion01`；
2. definition capture 固定 RuleId 和六组 tag filters；
3. required/blocked 标签树存在父子交叠时失败关闭；
4. `TryOpen` 要求 exact Startup-to-Active commit receipt 与当前 Active runtime；
5. window/receipt id 由 action、commit sequence、RuleId 和规范排序 tags 确定性派生；
6. window 不拥有 clock、duration、timer 或独立 close；
7. 每次 projection 都重新校验 action runtime 仍为 Active；
8. projection 固定为 PreventAll/Avoidance/DefenseEvade；
9. source instance 使用 window id，保留完整审计关联；
10. evasion trigger 不需要额外 mutable commit；
11. damage/source/target filters 直接交给 CombatCore resolver；
12. 不依赖 demo_map、World、Actor、damage API、RNG、Timer 或 Tick callback。

## 执行序列

1. 审查人工完善的 0.0.10 战斗规划、P3 action lifecycle、P9 SpiritShield 与 CombatCore resolver。
2. 确认 SpiritEnergy 产品 authority 尚未冻结，收紧 P10.0 边界。
3. 新增 immutable definition capture/definition。
4. 新增 window open receipt 与 defense projection receipt。
5. 实现 exact commit/runtime/action 一致性校验。
6. 实现 canonical tag serialization 和确定性 ids。
7. 实现 PreventAll/Avoidance/DefenseEvade 投影。
8. 新增六个 focused tests，覆盖 definition、commit、resolver、filters、phase 与 replay。
9. 新增 changed-file mapping，强制 SpiritEvasion、ActionLifecycle、CombatCore 与 CombatRuntime 证据。
10. 扩展 mapping self-test pass/fail fixtures。
11. JSON mapping `76` rules 与 self-test `114/114` 通过。
12. Editor candidate 6 actions，原生退出 `0`。
13. focused candidate `6/6`，无源码修正轮。
14. 串行执行五组正式 Automation，合计 `483` success、`0` fail，全量 `385/385`。
15. changed-file gate `Changed=5 / Rules=2 / Required=4 / Logs=5` 通过。
16. 精确边界扫描命中 `0`，`git diff --check` 通过。
17. 暂存区限定为 5 个实现/门禁文件。
18. Editor final up-to-date success；Game final 5 actions，原生退出 `0`。
19. 生成同名 Report/Log，执行 exact-stage gate 与暂存范围复核。
20. commit、push，并使用远端 commit SHA 形成不可变链接。

## 状态与数据流

```text
Capture SpiritEvasionDefinition
  -> canonical action + RuleId + frozen tag filters

ActionOrchestrator.TryStart
  Idle -> Startup
  -> cannot open evasion window

ActionOrchestrator.TryAdvance(Startup)
  Startup -> Active + exact commit receipt
  -> TryOpen(action, definition, commit, runtime)
  -> deterministic window/open receipt

TryProjectDefenseLayer(runtime still Active)
  -> PreventAll / Avoidance / DefenseEvade
  -> CombatCore Resolve
  -> matching impact: Evaded, conserved
  -> non-matching tags: Applied unchanged

Action Active -> Recovery | Interrupted
  -> projection rejected
  -> no independent timer or lingering window authority
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `P10.0-SpiritEvasion-final.log` | 6 | 0 | yes | 0 | `164376AAC882F6A7F85D7BFE9B70EEDD93471D631F5EBBDDFDBEC3C5311048AB` |
| `P10.0-ActionLifecycle-final.log` | 1 | 0 | yes | 0 | `EB5442A065AF40783635CCFC4C2D74509501E2863491684C1DFF13A81CF3CCE3` |
| `P10.0-CombatCore-final.log` | 9 | 0 | yes | 0 | `50C99CEB62BD0C13F291785CC46ECFD3CB28AAC49509BCB425BAA3C51CBCABCE` |
| `P10.0-CombatRuntime-final.log` | 82 | 0 | yes | 0 | `D3BAFB4591CEDACC1841F117BE815A5ED8AC470EBD5C9A155CF6A743CBA26996` |
| `P10.0-Shanmen-0_0_10-final.log` | 385 | 0 | yes | 0 | `8E81A0EE1307C3AE3B71DFEDC5CC4E216D71FB725ACA8D3CF2F12B7FD80CA85B` |

命令形态：

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

五份日志均恰有一个 selected queue-empty、`0` selected fail、`0` fatal/unhandled/ensure，进程原生退出码均为 `0`。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=76
SELF_TEST: PASS 114/114
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=2 Required=4 Logs=5
git diff --check: PASS
BOUNDARY_SCAN: PASS hits=0 (world/actor/damage/RNG/timer/tick-callback)
```

- required groups：SpiritEvasion、ActionLifecycle、CombatCore、CombatRuntime；
- mapping SHA-256：`7FBCB8F63AAC9B00D73626F2544C162FFF57C49CB72D2F38D7AFB0628872FAFB`；
- self-test SHA-256：`F9623AED7B8CAE97AD52708D1DF08B0F87A29EA7A89B00CED14F2B1A7FC15F21`；
- 实现：`5 files / +994 / -0`。

## 构建证据

| Build | Actions | Time | Exit | Log SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | 6 | 35.95s | 0 | `DEB6D66DBC69F25E71FD4C1224F3AF4D154443E0E05D1F4E94ECB652870F12DB` |
| Editor final | 0 | 1.04s | 0 | `F430314655F2327089EAED4E96F40405105028F113140D8015AB8C20D13D82A1` |
| Game final | 5 | 30.78s | 0 | `275B9A4543DE27FB9AB36151DDD5CF98AE45B3078F1D264982815F17C5AB421A` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1247744` bytes / `33304175024077ED78A7B824F6D01288B78C3B7485E9A6AD112046FD7FB9470C`；
- `demo_map.exe`：`353596416` bytes / `ED52E8FFDB957594C4C3663964E8912C7836A143EAA856CE3A63EF99AB452B82`。

## 真实异常

没有源码、UHT、Automation、gate 或构建失败。UE 5.8 启动时仍输出 UnifiedError 基线 `Condition failed` 诊断，但它不属于所选测试；随后六个新增测试及全部回归均逐项 Success，queue-empty 与原生退出码正常。

未发生内存、页面文件、外层超时、并发缓存锁或目标平台 Win64 SDK 错误。其他平台 SDK 的 ValidatePlatforms 信息不影响 Win64 目标。

## P/F 边界

仅执行 P 阶段纯值实现、无头 Automation、静态/门禁和 Editor/Game Development 构建。没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P10.1 建议冻结主动闪避的产品命令边界，把输入意图单向转换为 movement request 与 action activation，但继续让动作编排器拥有窗口生命期。只有在真实 SpiritEnergy owner、revision、恢复与持久化语义确定后，才把费用提交接入该链路；不得在输入适配器或窗口对象里增加临时资源 float。
