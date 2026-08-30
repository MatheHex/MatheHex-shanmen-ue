# Dev.D.UE.0.0.10.P9.6.r0 Report

## 1. 结论

P9.6 **PASS**。本阶段把 P9.3—P9.5 已存在但需要调用方手工拼装的 SpiritShield action、resource、capacity 与 deadline authority，收束为单次短护盾的确定性 aggregate session。产品调用方现在不能再发布“资源已提交但 capacity/deadline 未配置”或“deadline 只关闭 shield、action 仍为 Active”的半状态。

最终验证为 SpiritShieldSession `8/8`、SpiritShieldAction `10/10`、ActionResource `7/7`、ActionLifecycle `1/1`、SpiritShield 聚合 `35/35`、SpiritShieldCapacity `6/6`、SpiritShieldDeadline `6/6`、CombatCore `9/9`、CombatRuntime `76/76`、`Shanmen.0_0_10` 全量 `379/379`。十份相互重叠的证据日志合计 `537` 条 success、`0` fail；changed-file gate、mapping self-test、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

本阶段没有伪造产品 SpiritEnergy 字段或第三套资源账本。代码审查确认现有产品层尚无可安全写入、带 revision 且已冻结恢复/持久化语义的 SpiritEnergy authority；因此 P9.6 只组合既有 typed authority，真实余额与单调时间仍由未来产品 owner 注入。

## 2. 功能性

### 2.1 冻结 schedule

`FShanmenSpiritShieldSchedule::TryCapture` 冻结 timeline id、start tick 与 deadline tick，并确定性派生 schedule id。无效 timeline、负起点或 `deadline <= start` 均失败关闭；字段只读，外部不能在 Begin 后改写时间契约。

### 2.2 原子 Begin / Commit

- `Begin` 同时建立 action reservation 与 schedule 绑定；
- exact Begin replay 返回同一结果，不推进资源 revision；
- 同 activation 的不同 schedule 被判为冲突；
- `Commit` 在候选副本中完成资源提交、shield activation、capacity authority、deadline contract 与 deadline gate；
- 任一子契约失败时正式 session 和外部 resource authority 均不接收候选状态；
- Commit replay 返回原 activation terminal，不重复扣除资源。

### 2.3 Capacity 与防御投影

- session 只暴露只读 authority view，不提供可写内部 authority getter；
- 防御层投影来自同一已激活 shield instance；
- capacity commit 复用 P9.1 的 command/receipt/revision 契约；
- exact capacity replay 在 session closure 后仍可查询原 receipt；
- closure 后的新 capacity write 固定拒绝；
- capacity 归零只停止后续投影，不私自拥有 action lifecycle，保持 P9.1 契约。

### 2.4 Deadline 原子闭环

`ObserveDeadline` 先在 deadline gate 候选副本中证明时间到期，再由 action coordinator 复用同一 deactivation receipt 完成 action closure。结果同时携带 deadline receipt 与 closure receipt：

- deadline 未到返回 typed rejection，状态不变；
- deadline 到期时 shield 与 action 一次性闭合；
- exact observation replay 返回同一两份 receipt；
- timeline/observation 冲突失败关闭；
- 不存在只写入 deadline proof、却泄漏未闭 action 的公开路径。

### 2.5 显式结束与 commit 前中止

- `Abort` 仅服务 commit 前路径并释放 reservation；
- `Close(Explicit|Interrupted|OwnerEnded)` 服务 commit 后 action closure；
- `Close(DurationElapsed)` 被拒，调用方不能绕过 timeline proof；
- 显式闭环后迟到的 deadline observation 不改写 terminal history；
- exact Close replay 保持确定性。

## 3. 完整性与兼容性

- 复用 `FShanmenSpiritShieldActionCoordinator`、`FShanmenActionResourceAuthority`、`FShanmenSpiritShieldCapacityAuthority` 与 `FShanmenSpiritShieldDeadlineGate`；
- 未建立第二套 action、resource、capacity、shield 或 timeline authority；
- schedule 与 deadline closure result 为 BlueprintReadOnly typed proof；
- P9.0—P9.5 的独立 authority API 与既有测试保持兼容；
- product owner 仍负责真实 SpiritEnergy snapshot、余额提交、恢复、持久化与单调 timeline；
- 未依赖 `demo_map`、World、Actor、ApplyDamage、随机数、Timer 或 Tick callback；
- 未声明完成输入适配、快捷栏、HUD 或产品资源持久化。

## 4. 关键不变量

1. schedule 在 Begin 前冻结，activation 后不可改写；
2. session 内所有 authority 必须绑定同一 action activation、shield instance 与 timeline；
3. Begin exact replay 不产生第二次 reservation；
4. Commit 要么同时发布全部 authority，要么全部不发布；
5. Commit exact replay 不重复扣除资源或激活；
6. capacity command 必须匹配当前 shield instance 与 expected revision；
7. session closure 后只允许既有 capacity receipt exact replay；
8. deadline closure 必须同时持有 deadline proof 与 action closure proof；
9. `DurationElapsed` 不可通过普通 Close 伪造；
10. pre-commit Abort 与 post-commit Close 保持不同资源语义；
11. 子 authority 不通过 mutable getter 泄漏；
12. 等价输入生成相同 schedule、activation、capacity、deadline 与 closure identities。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.CombatRuntime.SpiritShieldSession` 八个测试：

- `ScheduleContract`；
- `AtomicActivation`；
- `CapacityBoundary`；
- `AtomicDeadlineClosure`；
- `ExplicitClosureBoundary`；
- `PreCommitAbort`；
- `FailClosedAtomicity`；
- `DeterministicReplay`。

依赖 authority 的 focused groups、CombatCore resolver、CombatRuntime 宽回归和 0.0.10 全量回归均单独执行。全量由 P9.5 的 `371` 增至 `379`。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritShieldSession.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldSession.cpp`。

测试与回归规则：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldSessionTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

实现为 `5` 个文件、`1323` insertions、`0` deletions；加入本 Report 与同名 Development Log 后 exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P9.6-SpiritShieldSession-final.log` | `SpiritShieldSession` | 8 | 0 | `5E0D9928769CCA7511C495A15AC7B803A5A978F5523B6DDB35CE50F4EF3610F1` |
| `P9.6-SpiritShieldAction-final.log` | `SpiritShieldAction` | 10 | 0 | `3BE242C4FDD0C346CBEA2B78304EE0AB05CCFCD38035E5898C6191DB00B6E6FC` |
| `P9.6-ActionResource-final.log` | `ActionResource` | 7 | 0 | `3025C88CA0B2F6C5E401E67A1EAAD9379B48BAD1E5ABCFAF722E12EE5C3BDEDC` |
| `P9.6-ActionLifecycle-final.log` | `ActionLifecycle` | 1 | 0 | `BCCF1BFCB2877107EB1B97779D1D0CB786F90D78625826B750A904FC687E4982` |
| `P9.6-SpiritShield-final.log` | `SpiritShield` | 35 | 0 | `26E403C524B316873578696CE4799F3042FEC5B3AAC72A23A1104C89698120C3` |
| `P9.6-SpiritShieldCapacity-final.log` | `SpiritShieldCapacity` | 6 | 0 | `89306F9F95C7CA3FBCC37B9F20664B46DDD253BDEAFBB5DB14DD802B2BE02F31` |
| `P9.6-SpiritShieldDeadline-final.log` | `SpiritShieldDeadline` | 6 | 0 | `0679A65F5D35CC195436B26F666D5387B4A73021610A2EF3A207C6C437726427` |
| `P9.6-CombatCore-final.log` | `CombatCore` | 9 | 0 | `1E9620CC714AB3013229696F7FADF246F223FD20400489C0E0D470ABEC2A9BE6` |
| `P9.6-CombatRuntime-final.log` | `CombatRuntime` | 76 | 0 | `AC0FBCC59DB379E8CC0B8D01C4C06914BF846711E22948296B7F51C32A1C28AB` |
| `P9.6-Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 379 | 0 | `A9E65196549EDEA483475203AF2232F6C0715C3E6586EA7B0A43971EAF26BEAF` |

每份日志均有一个目标 RunTests 命令与一个结构化 queue-empty；selected fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=75
SELF_TEST: PASS 112/112
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=9 Logs=10
git diff --check: PASS
BOUNDARY_SCAN: PASS hits=0 (world/actor/damage/RNG/timer/tick-callback)
```

- mapping SHA-256：`2DC746B76708DEEC4CD4F0655BA3289D6F5EDDF1AC308C56A50A80D327D4FE60`；
- self-test SHA-256：`E2002969DAEB7DF7B7D2861DF08087EADF12989191B29BCFC5D792E3650CB6C5`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 6 / 20.92s | 0 | `6FB7EC590862BC4F97A7AF055B4E9647369F4BDEDA4BD1B62C6403A0DB45011C` |
| Editor final | Succeeded, target up to date | 0 / 0.92s | 0 | `656BBD3BA93559B6C1646E17CF8A7CE17B4C0F47C192C092E91D0C1FD8C65041` |
| Game final | Succeeded | 5 / 28.59s | 0 | `5D50FBB9F800F42178804E75582C8F2F8496A7516E4299F41A6FE49EEB8C3153` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1179136` bytes，SHA-256 `07BC22344B93A6579E1DEA9DF8D4C75B9C3BE7D105E1A0852E7C4BB0EF319781`；
- `demo_map.exe`：`353550848` bytes，SHA-256 `CEF1C13C0FA101B7060982D1149F8E9961BC6DC090AA8F12B1B72D24C21510A2`。

## 9. 真实异常

本阶段没有源码、目标测试或构建失败。Automation 启动仍含 UE 5.8 自带 UnifiedError 基线诊断，但所有选中测试随后逐项 Success，gate 对 selected fail/fatal/unhandled/ensure 的计数均为 `0`。

首次静态扫描使用裸字符串 `Tick`，把合法的确定性 `StartTick/DeadlineTick` 数据字段误报为引擎 Tick。复核后将判据收紧为 `PrimaryActorTick`、`TickComponent`、`Tick(...)` callback、Timer/world/actor/damage/RNG 依赖，命中 `0`。这是扫描表达式误报，不是源码失败，也没有删除时间契约来规避检查。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值实现、代码审查、无头 Automation、静态扫描、changed-file gate、Editor/Game Development 构建与 Git 证据。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P9.7 不应直接发明临时 SpiritEnergy float。下一步应先冻结“谁拥有真实 SpiritEnergy、revision 如何推进、恢复与存档归谁”的产品 authority 契约；若数值政策仍未定，则转入另一条已明确的 0.0.10 战斗切片，保留本 session 作为可注入组合根。
