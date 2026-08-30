# Dev.D.UE.0.0.10.P9.5.r0 Development Log

## 目标

补齐 P9.4 SpiritShield action composition 的提交后闭环：让已激活护盾能够以正常结束、deadline 到期、提交后中断或 owner teardown 的明确原因，原子地同步关闭 shield runtime 与 action runtime；冻结 closure proof、重放与冲突语义，为后续产品 Host 提供可复用、可重装且不泄漏半状态的完整生命周期。

## 审计结论

1. P9.4 已原子组合 Begin、resource reservation、Commit 与 shield activation，但协调器没有从 `Activated` 进入 terminal state 的入口；
2. P9.2 deadline gate 可以把内部 shield runtime 变成 `Deactivated`，但 action runtime 仍停留在 `Active`；
3. 如果直接建立产品 Host，Host 必须在外部猜测如何推进 action、如何判断 exact replay，以及何时允许下一次 activation；
4. 该缺口属于 runtime composition 契约，而不是产品输入或最终 SpiritEnergy 数值问题；
5. 因此 P9.5 先闭合 action/shield 生命周期，产品 Host 顺延到 P9.6，避免把不完整契约固化进产品层；
6. P9.1 capacity depletion 仍由其调用方显式结束 shield，本阶段不发明新的 deactivation reason 或产品数值。

## 设计决策

1. 新增 coordinator terminal states `Completed` 与 post-commit `Interrupted`，不复用 pre-commit `Aborted`；
2. `Explicit` 与 `DurationElapsed` 映射为 graceful `Completed`；`Interrupted` 与 `OwnerEnded` 映射为 post-commit `Interrupted`；
3. graceful close 固定执行 action `Active -> Recovery -> Idle(Completed)`；中断 close 固定执行 `Active -> Interrupted`；
4. shield deactivation、action transition 与 closure receipt 全部在 coordinator 候选副本完成，整体有效后才替换正式状态；
5. `DurationElapsed` 不允许直接调用普通 deactivation 绕过 deadline gate；只有 P9.2 gate 已写入 matching deadline receipt 后才允许 action closure；
6. 若 shield 已由外部 authority 结束，closure 必须复用同一 deactivation receipt 且 reason 精确匹配；
7. closure receipt 绑定原 activation terminal、deactivation proof、退出 Active transition、可选 completion transition 与 closure outcome；
8. exact Close replay 返回原 closure receipt；不同 reason/outcome 改写返回 `ClosureConflict`；
9. Commit 在闭环后仍可重放原 terminal receipt，不重复消费资源或激活；
10. committed SpiritEnergy 不因正常结束或提交后中断回滚；
11. 闭环后必须显式 `Reset` 才能 Begin 下一动作，避免隐式覆盖历史；
12. 不加入产品 owner、输入、Timer、World、Actor、资源恢复或持久化规则。

## 执行序列

1. 审查 P9.4 coordinator、P9.0 shield deactivation、P9.2 deadline gate 与 action orchestrator terminal phase。
2. 冻结 closure state/status/error/outcome 与 immutable receipt。
3. 实现 closure receipt 的确定性 identity 与结构校验。
4. 实现 candidate-copy `Close`，覆盖 active shield 与 externally deactivated shield 两条路径。
5. 实现 graceful completion、post-commit interruption、exact replay 与 conflict。
6. 保留 closed coordinator 的 original Commit replay。
7. 补充显式 Reset 后重新 Begin 的资源 revision 不变量。
8. 新增 3 个 focused tests，并扩展 deadline 与 determinism 既有测试。
9. Editor candidate 首次编译 UHT/源码共 6 actions，原生退出 `0`。
10. 最终测试增量再次编译 4 actions，原生退出 `0`。
11. 执行八组 Automation，合计 `496` success、`0` fail，全量 `371/371`。
12. mapping self-test `110/110`；implementation gate `Changed=3 / Rules=2 / Required=7 / Logs=8`。
13. 生产边界扫描、`git diff --check` 通过。
14. Editor final up-to-date success；Game final 5 actions，原生退出 `0`。
15. 生成同名 Report/Log；exact-stage gate 首次因调用方把两份 build log 混入 Automation evidence 而按设计拒绝。
16. 改用精确八份 Automation 日志后，exact-stage gate `Changed=5 / Rules=2 / Required=7 / Logs=8` 通过。
17. 执行 exact-stage、commit、push 与远端 SHA 核验。

## 闭环状态流

```text
Activated + Explicit
  shield Active -> Deactivated(Explicit)
  action Active -> Recovery -> Idle(Completed)
  coordinator -> Completed

Activated + DurationElapsed
  direct Close rejected
  deadline gate writes Deactivated(DurationElapsed)
  action Active -> Recovery -> Idle(Completed)
  coordinator -> Completed

Activated + Interrupted | OwnerEnded
  shield Active -> Deactivated(reason)
  action Active -> Interrupted
  coordinator -> Interrupted

already externally deactivated
  exact reason proof -> close action and reuse receipt
  different reason -> ClosureConflict, action unchanged

exact Close replay -> original closure receipt
different close reason -> conflict
Commit replay after close -> original activation terminal; no extra resource revision
Reset -> permits a new Begin; old committed cost remains committed
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `SpiritShieldAction-final.log` | 10 | 0 | yes | 0 | `F1D74819F4C02905C9E5659D7595E47F53D90235CFE1587FE7F8B0750932AE10` |
| `ActionResource-final.log` | 7 | 0 | yes | 0 | `AC167E5CAEEE5D437BF07FA3E1FB5A0B8F947E8D0049FBBCC03E5B416BFB64D8` |
| `ActionLifecycle-final.log` | 1 | 0 | yes | 0 | `5ACF14B265329B840B006025A4640E8D76341DD7E3DB6B9B4908FF08E03E3B36` |
| `SpiritShield-final.log` | 27 | 0 | yes | 0 | `0FEC10275818D20A852FB978C5F930AC9B272C6D37215C08A535A43FD8CDC22E` |
| `SpiritShieldCapacity-final.log` | 6 | 0 | yes | 0 | `D67406C7BCBA72EA215341AFBD9B6ED21B26D9BC6CB7F126D3EE9811A7EBC2D7` |
| `SpiritShieldDeadline-final.log` | 6 | 0 | yes | 0 | `D093A17C7F51E2D04553A9540E1F4FF9A2E2CC7552E78535280059085EFFE810` |
| `CombatRuntime-final.log` | 68 | 0 | yes | 0 | `B7AB7D50A9D9CD9FF44934C737E68A6FA509F8FF88953E8EC676ABDCCF0AC94A` |
| `Shanmen-0_0_10-final.log` | 371 | 0 | yes | 0 | `6FE2A77FE844B58D2AD80BC420257EC141FAA5B95C611F6AF94DE75EADE91B56` |

每份日志均恰有一个目标 `Automation RunTests` 命令、一个结构化 queue-empty 终止标志、`0` selected fail/not-run、`0` fatal/unhandled/ensure；八个进程原生退出码均为 `0`。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=74
SELF_TEST: PASS 110/110
REGRESSION_COVERAGE (implementation): PASS Changed=3 Rules=2 Required=7 Logs=8
REGRESSION_COVERAGE (exact stage): PASS Changed=5 Rules=2 Required=7 Logs=8
git diff --check: PASS
BOUNDARY_SCAN_MATCHES=0
```

- required groups：SpiritShieldAction、ActionResource、ActionLifecycle、SpiritShield、SpiritShieldCapacity、SpiritShieldDeadline、CombatRuntime；
- mapping SHA-256：`85EFE6B1CBAA610D6D5428DB2F74AA41120293683545BDB204C4644DA2B6FAB7`；
- self-test SHA-256：`1B5D05C596E8B6DAA5B317BF61696F6FCF757FFD9FDC527E55549FCD961AB504`；
- 生产/测试：`3 files / +640 / -16`。

## 构建证据

| Build | Actions | Time | Exit | Log SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | 6 | 36.67s | 0 | interactive capture |
| Editor test increment | 4 | 4.67s | 0 | interactive capture |
| Editor final | 0 | 0.89s | 0 | `87E738A9A011AD60AA5DAB8F78D7657C30D41BF15BDC54DC84EC5224D0F040FE` |
| Game final | 5 | 30.02s | 0 | `FF5C54870DA4D9D3AC961CEEBFF45972C704CD11E13186843691F37DAA3B6751` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1111552` bytes，SHA-256 `D1F106CC7BD6EA370FAAC7E750C9F590592CF8C6551CD82774229FD71BB52245`；
- `demo_map.exe`：`353496064` bytes，SHA-256 `522D006FDAE99D9186E26D13C01CD9B2715B2A7867F0B0B9EDE9A384B5F3F6EF`。

## 真实异常

没有源码、目标测试或构建失败。Automation 启动保留既有非目标平台 SDK metadata 与测试发现前诊断；Win64 SDK 有效，所有选中目标随后 Success。未发生 C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

文档生成后的首次 exact-stage gate 调用使用 `*-final.log` 通配符，把 `Editor-final.log` 与 `Game-final.log` 两份 build log 一并传给 Automation evidence parser；门禁正确报告两份日志缺少 RunTests/success/queue-empty 并失败。该问题是验证调用参数错误，不是源码或产品失败；改为显式列出八份 Automation 日志后原生 PowerShell 成功，输出 `PASS Changed=5 Rules=2 Required=7 Logs=8`。没有放宽 parser 或修改 mapping 绕过失败。

## P/F 边界

仅执行 P 阶段纯值实现、无头 Automation、静态/门禁和 Editor/Game Development 构建。没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P9.6 可在完整 action lifecycle 上建立产品拥有的 SpiritEnergy adapter/host：从正式玩家资源 authority 捕获 typed snapshot，由内容定义提供 cost，由明确输入命令触发 Begin/Commit/Abort/Close/Reset。正式 resource owner、恢复与持久化规则冻结前，不创建临时 float 或第二套产品账本。
