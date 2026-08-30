# Dev.D.UE.0.0.10.P11.0.r0 Development Log

## 目标

在 0.0.10 “手感优先、数值后置”的边界内，建立第一条普通武器格挡纯契约：格挡必须绑定真实武器动作 commit，只在 Active phase 生效，并复用 CombatCore 的统一 DefenseLayer 结算；不提前实现精准格挡、产品输入、方向判定或未冻结的体力/灵力/耐久数值。

## 审计结论

1. CombatCore 已有 `ReduceFraction`、`FShanmenDefenseOrder::Guard` 与 `Shanmen.Defense.Guard`，无需新增第二套结算器；
2. CombatCore 已把 `PerfectGuard` 放在普通 `Guard` 之前，两种语义可以保持独立；
3. P3 `FShanmenActionOrchestrator` 已把 `Startup -> Active` 定义为唯一 commit point，可继续作为格挡窗口唯一生命期权威；
4. P4 BasicSword 测试使用过合成 Guard layer，但项目尚无主动武器格挡 runtime；
5. P6 orbiting-sword readiness 明确不是实际格挡，不应被复用成普通手持武器防御；
6. P9/P10 的 SpiritShield 与 SpiritEvasion 已证明 action-bound 防御投影结构可行；
7. 现有产品 item authority 能做 durable reservation，但本阶段没有武器格挡耐久政策与预留输入；
8. 直接声明 `bRequiresCommitOnTrigger=true` 会制造无法兑现的资源提交；
9. 因此 P11.0 只冻结普通格挡定义、commit-bound window 与无资源 DefenseLayer projection；
10. 精准格挡 timing、武器耐久 reservation 与输入链分离到后续阶段。

## 设计决策

1. canonical action 为 `Combat.Action.Sword.Guard01`；
2. definition capture 包含 RuleId、内容提供的 fraction 与六组 tag filters；
3. fraction 只校验 `(0,1]`，运行时不选择具体平衡值；
4. required/blocked 标签树父子交叠时失败关闭；
5. action 必须冻结有效武器 `SourceItemInstanceId`；
6. `TryOpen` 要求 exact Startup-to-Active commit receipt 与当前 Active runtime；
7. window/receipt id 纳入 action、weapon、content、RuleId、fraction IEEE bits、commit sequence 与规范排序 tags；
8. window 不拥有 clock、timer、duration 或独立 close；
9. projection 固定为 ReduceFraction/Guard/DefenseGuard；
10. source instance 使用真实武器实例，保留后续 durable adapter 的关联点；
11. 没有真实 reservation 时 `bRequiresCommitOnTrigger=false`；
12. 普通格挡不附加 DefensePerfectGuard；
13. tag filters 直接交给 CombatCore resolver；
14. 新模块不依赖 demo_map、World、Actor、ApplyDamage、RNG、Timer 或 Tick callback。

## 执行序列

1. 审查人工完善的 0.0.10 战斗规划、最近 P10 阶段与现有资源权威。
2. 审查 CombatCore defense order/tag/resolver、P3 action lifecycle、P4 BasicSword、P6 controlled-weapon readiness、P9 SpiritShield 与 P10 SpiritEvasion。
3. 确认 SpiritEnergy 数值与武器格挡耐久政策均未冻结，收紧 P11.0 边界。
4. 创建分支 `agent/0.0.10-p11-0-weapon-guard-window`。
5. 新增 mutable capture 与 immutable weapon-guard definition。
6. 新增 commit-bound window/open receipt 与 defense projection receipt。
7. 实现 exact action/runtime/commit/weapon 一致性校验。
8. 实现 fraction bits、排序 tags 与 action evidence 的 canonical identity。
9. 实现 ReduceFraction/Guard/DefenseGuard 投影。
10. 新增六个 focused tests，覆盖定义、commit、投影、filters、phase 与 replay。
11. 新增 WeaponGuard changed-file mapping 与正/反 self-test fixtures。
12. mapping JSON `89` rules、self-test `138/138`、`git diff --check` 通过。
13. Editor Development：8 actions、40.63s、原生退出 `0`。
14. focused WeaponGuard：6/6，首轮通过。
15. 串行运行 ActionLifecycle、CombatCore、CombatRuntime 与 0.0.10 全量。
16. 五组正式 Automation 合计 570 Success、0 Fail，进程退出均为 0。
17. changed-file gate：Changed=5、Rules=2、Required=4、Logs=5，PASS。
18. 精确边界扫描命中 0，`git diff --cached --check` 通过。
19. Game Development：5 actions、30.78s、原生退出 `0`。
20. 暂存范围限定为 5 个实现/门禁文件；生成同名 Report/Log 后扩展为 exact 7-file stage。
21. 复核长期未跟踪文件未进入暂存区，commit 并 push。

## 状态与数据流

```text
Capture WeaponGuardDefinition
  -> Combat.Action.Sword.Guard01
  -> RuleId + authored fraction + frozen tag filters

ActionOrchestrator.TryStart
  Idle -> Startup
  -> cannot open guard window

ActionOrchestrator.TryAdvance(Startup)
  Startup -> Active + exact commit receipt
  -> TryOpen(action with weapon id, definition, commit, runtime)
  -> deterministic window/open receipt

TryProjectDefenseLayer(runtime still Active)
  -> ReduceFraction / Guard / DefenseGuard
  -> SourceInstanceId = frozen weapon item
  -> no resource commit without reservation
  -> CombatCore Resolve
      -> matching 100 damage at authored 0.25 = 25 prevented / 75 final
      -> non-matching damage or target = 100 unchanged

Action Active -> Recovery | Interrupted
  -> projection rejected
  -> no independent timer or lingering guard authority
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `P11.0-WeaponGuard-final.log` | 6 | 0 | yes | 0 | `9EA56D4C547841DD25FAF643F4DA08F9D59D692D7A8715286FF4C34F02E7912B` |
| `P11.0-ActionLifecycle-final.log` | 1 | 0 | yes | 0 | `FCE16F556D5C35B7938CF0B3322B7D0A71825CA7E09154AC57E7D416E540A07C` |
| `P11.0-CombatCore-final.log` | 9 | 0 | yes | 0 | `22A995A52E1147EC1127F682698FFD5C738D60F62B49D7261403C9189CBC1742` |
| `P11.0-CombatRuntime-final.log` | 93 | 0 | yes | 0 | `CE70062FA62F6A72AA3997EB7FCF90C53863DFB0426E8257256CDAA9F5C2CD79` |
| `P11.0-Shanmen-0_0_10-final.log` | 461 | 0 | yes | 0 | `9EDE3234CB2542365EFB7C14F40D68279436A68DE1FF23A757BF315B59D98DE7` |

命令形态：

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

五份日志均恰有一个目标 RunTests 命令、一个结构化 queue-empty、selected fail 0、fatal/unhandled/ensure 0，原生退出码均为 0。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=89
SELF_TEST: PASS 138/138
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=4 Logs=5
git diff --cached --check: PASS
BOUNDARY_SCAN: PASS hits=0
```

- required groups：WeaponGuard、ActionLifecycle、CombatCore、CombatRuntime；
- mapping SHA-256：`AC2CAE819DEE2A48AE8677008422E9FF9E5A5FEA22CFE87A4C4649D850F55612`；
- self-test SHA-256：`1ED3590940664BBE93D3E0B4D76A69CE8852D12F9C221DE9AE9DE6276279BDA2`；
- implementation/mapping：`5 files / +1058 / -0`。

## 构建证据

| Build | Actions | Time | Exit | Log SHA-256 |
|---|---:|---:|---:|---|
| Editor Development | 8 | 40.63s | 0 | `5FD7AC8C267B9409CF3412D1F495B597DDB9700A7712F6E63783B7C145AFB0E8` |
| Game Development | 5 | 30.78s | 0 | `219F28FD8EFBEF466760A9043144204B0DFA1545E0429E1BD27DCE7FE66CF5F3` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：1369088 bytes / `34E7CB337962C9FC0BCA48196A4E12ABB7161FF0B4DDBF1895289DDB1B618835`；
- `demo_map.exe`：353997312 bytes / `642BCA9323652F9BC624BFAE980D37B1EAFEF03C869907FCA7EA490D0B52E776`；
- 未在源码未变化后重复 Editor 构建。

## 真实异常

本阶段首轮源码、UHT、focused、全部回归、门禁、Editor 与 Game 构建均成功，没有修正轮或失败日志。没有内存、页面文件、外层超时、并发缓存锁或 Win64 SDK 错误。

每份 Automation 日志含 13 行 UE 5.8 `UE::UnifiedErrorTest` 初始化自检噪声；selected tests 随后全部 Success，terminal marker 与进程码正常。其它平台 SDK validation 信息不影响 Win64 目标。

## P/F 边界

仅执行 P 阶段纯值实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P11.1 建议在同一 action lifecycle 上增加独立的 perfect-guard timing policy，并把普通 Guard 与 PerfectGuard projection 明确互斥。武器耐久必须通过现有 durable item authority 的 prepare/commit/cancel 事务接入；产品输入、方向夹角与手感参数应在后续产品纵切中逐层完成。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-0-weapon-guard-window/Docs/Report/Dev.D.UE.0.0.10.P11.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-0-weapon-guard-window/Docs/Log/Dev.D.UE.0.0.10.P11.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-0-weapon-guard-window>
