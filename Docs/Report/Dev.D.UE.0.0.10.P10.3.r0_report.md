# Dev.D.UE.0.0.10.P10.3.r0 Report

## 1. 结论

P10.3 **PASS**。本阶段在 P10.2 的 immutable movement plan 与只读 WorldStatic preflight 之上，加入了确定性分段轨迹、显式 motion session、逐段 command、实际 swept movement receipt，以及唯一的产品 mutation seam。

产品策略必须显式提供持续时间和分段数；session 不拥有 World、Character、Tick、Timer 或时钟，而是消费调用方提供的单调 elapsed time。每个到期段生成重放稳定的 command；在匹配 receipt 被接纳前，同一段不能重复发放。实际位移继续复用 `Fdemo_mapCombatDisplacement::MoveCharacterSwept`，没有新增第二套移动算法。

最终验证为 motion runtime `6/6`、P10.2 movement adapter `5/5`、P10.1 movement `5/5`、EnemySkillFramework `44/44`、V2RangedCompatibility `22/22`、0.0.10 全量 `401/401`。六份正式日志合计 `483` 条 Success、`0` Fail；changed-file gate、mapping self-test、静态边界扫描、`git diff --check`、Editor Development 和 Game Development 均通过。

## 2. 功能性

### 2.1 显式轨迹策略

`Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture` 要求：

- `DurationSeconds` 有限且大于零；
- `SegmentCount` 大于零；
- 不提供隐式持续时间、帧率或分段默认值；
- capture 成功后只暴露只读访问器。

`Fdemo_mapShanmenSpiritEvasionMotionPlanner::TryBuildPlan` 把 P10.2 movement plan 与轨迹 snapshot 绑定为一个 motion plan。MotionPlanId 由 P10.2 PlanId、duration 的 IEEE bits 和 segment count 经 `FShanmenDeterministicId` 派生；`IsValid` 会重新派生并比对身份。

### 2.2 确定性分段调度

session 只接受与 movement plan 精确匹配且状态为 `Ready` 的 P10.2 preflight。目标距离取自 preflight 的实际 resolved distance，不重新读取 World，也不恢复原 requested distance。

第 `n` 段的计划边界为：

```text
scheduled_elapsed = duration * n / segment_count
cumulative_target = target_distance * n / segment_count
segment_distance = cumulative_target(n) - cumulative_target(n - 1)
```

用相邻累计目标之差生成分段距离，避免简单 float 除法把舍入误差重复累积。调用方可在时间跳跃后以相同 elapsed 连续提取已到期段，但每次只能存在一个 pending command。

### 2.3 command 与 receipt

每个 command 冻结：

- SessionId、MotionPlanId；
- segment ordinal 与总段数；
- scheduled elapsed；
- requested distance；
- P10.1 冻结的 canonical planar direction。

CommandId 绑定上述全部字段。receipt 再绑定 CommandId、实际 resolved distance 和 blocked 状态；不保存 Actor、World、movement component 或可变指针。

未阻挡 receipt 必须提交完整请求距离；短位移只有在 `bBlocked=true` 时才可被记录。session 以候选副本接纳 receipt，完整验证通过后才原子替换当前状态，避免错误 receipt 造成半提交。

### 2.4 状态与产品执行边界

motion state 为 `Invalid / Active / Completed / Blocked`：

- 所有段 receipt 完整接纳后进入 `Completed`；
- 任一合法 blocked receipt 被接纳后进入 `Blocked`；
- pending command 会阻止重复发放；
- 错误 session、plan、ordinal、command 或 receipt identity 均失败关闭；
- elapsed 必须有限且单调不减。

`Fdemo_mapShanmenSpiritEvasionSegmentExecutor::ExecuteSwept` 是新代码唯一的产品 mutation seam。它先检查 command、Character 与 movement component，再调用已有 `MoveCharacterSwept`，并立刻把位移结果转换成 actor-free receipt。

## 3. 完整性与兼容性

- 复用 P10.1 action-bound direction 与 P10.2 policy/preflight，不复制这些权威；
- 复用现有 shared swept displacement，不新增碰撞或移动实现；
- 不修改旧 enemy skill、knockback、V2 或 V3 位移调用；
- EnemySkillFramework 与 V2RangedCompatibility 正式回归通过；
- motion plan、session、command、receipt 均可由相同输入确定性重放；
- session 不持有 Actor、World、Timer、Tick 或真实时钟；
- 没有 RNG、SpiritEnergy、无敌帧、恢复窗口或输入映射；
- 不修改 formation、weapon、item、profile、CodeB 或 0.0.9B schema。

本阶段没有把 P10.3 session 接入 action lifecycle owner；取消、角色销毁与技能中断时应由后续 coordinator 明确关闭或丢弃 session。也没有创建临时 SpiritEnergy 余额权威。

## 4. 关键不变量

1. duration 与 segment count 必须由产品 policy owner 显式注入；
2. motion plan 必须绑定一个有效 P10.2 movement plan；
3. session 只能从 matching `Ready` preflight 启动；
4. target distance 必须使用 preflight resolved distance；
5. elapsed 必须有限且单调不减；
6. 同时最多存在一个 pending command；
7. command ordinal 必须严格等于 accepted count + 1；
8. command/receipt identity 必须可重新派生；
9. 不匹配 receipt 不得改变 session；
10. unblocked receipt 不得隐藏短位移；
11. blocked receipt 终止后不得继续发段；
12. 实际产品移动只能经过 shared swept authority。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionMotionRuntime` 六个测试：

- `TrajectoryCapture`：有效 capture，以及零/负/非有限 duration 和无效 segment count；
- `PlanBinding`：movement plan 与轨迹绑定、确定性 identity 和输入分离；
- `DeterministicSchedule`：边界前不发段、边界发段、pending 防重复、时间跳跃追赶与重放一致；
- `CompletionLedger`：逐段 receipt、错误 receipt 拒绝、实际距离累计和 completed 终态；
- `BlockedTermination`：短距离必须显式 blocked、blocked receipt 终止且不能继续调度；
- `ExecutionBoundary`：invalid command 与 unavailable Character 不产生 receipt。

0.0.10 全量由 P10.2 的 `395` 增至 `401`。路径门禁要求并实际验证六组回归，而非只运行本轮 focused tests。

## 6. 修改范围

新增：

- `demo_mapShanmenSpiritEvasionMotionRuntime.h/.cpp`；
- `demo_mapShanmenSpiritEvasionMotionRuntimeTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

生产、测试与门禁共 `5` 个文件、`1142` insertions、`0` deletions；加入本 Report 与同名 Log 后 exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt/Report 和用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.3-SpiritEvasionMotionRuntime-final.log` | Product motion runtime | 6 | 0 | `B64626F3D9671AFDF3F9CE138D552796E4C3FE1F1C318F15516502D54FAE8C61` |
| `P10.3-SpiritEvasionMovementAdapter-final.log` | P10.2 adapter | 5 | 0 | `B489E6EEAB132BAEB80CF4067106A28C2E9FE2B94C2F0DE873A542308206A4AD` |
| `P10.3-SpiritEvasionMovement-final.log` | P10.1 movement | 5 | 0 | `096E557A75C26016350DA07DFF9448D6AAA26BCE4A8A09E71F105319AA417A2F` |
| `P10.3-EnemySkillFramework-final.log` | Enemy skills | 44 | 0 | `1D9F928E9C3EBEC2C837FAC1EA71A6B2A966B14F6A08607F62F4F4188F74438C` |
| `P10.3-V2RangedCompatibility-final.log` | V2 displacement | 22 | 0 | `D968783B0DC08B02EA84F27D527DC3E3D4623984FF63681984A3C9EB563FDEEC` |
| `P10.3-Shanmen-0_0_10-final.log` | 0.0.10 full | 401 | 0 | `F19B6944004D08616386FBCD1774D2B3B906A634397CEF29EEBD3C45C29FA561` |

所有正式进程原生退出码均为 `0`，每份日志都有 selected queue-empty，并且没有 selected fail、fatal、unhandled 或 ensure。

```text
REGRESSION_MAP_JSON: PASS Rules=80
SELF_TEST: PASS 120/120
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=6 Logs=6
git diff --check: PASS
MOVE_AUTHORITY_CALLS=1
DIRECT_MUTATION_HITS=0
WORLD_CLOCK_TIMER_TICK_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
```

mapping SHA-256：`435D932C9EE5E4DF6666E6A377521E7E260CE618642D33B660F30F2563B8BB4A`；self-test SHA-256：`3353C7BE611DFD7501A636806D657A1290211FB147A6BAD08EB0A24AA4E4F254`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 5 / 29.99s | 0 | `5617F08825B01AD8E3A1C91B9C44496145A284201E32F91C642E7593E13C9B08` |
| Editor final | Succeeded, up to date | 0 / 0.91s | 0 | `B26F677182A66967205223EAA93F28EE72B0748D36CD55D8F8FC26924E264B48` |
| Game final | Succeeded | 4 / 23.61s | 0 | `793D8A08C5BB6D938876151CABE6E3B88AAE7277BD4581D5275F6364D6EDCF1B` |

`UnrealEditor-demo_map.dll`：`12212224` bytes / SHA-256 `834B101E34666B9DE6FEDE6E3EC09EA105BC7DFBCF027D42FE865B16EFEA6A13`；`demo_map.exe`：`353692160` bytes / SHA-256 `7891D1FF12705A92E5721C122CDC6E3D78CC4B71B20FBB8C8FDBF62BBB68DA73`。

## 9. 真实异常

changed-file gate 第一次通过 Windows PowerShell 调用 `pwsh -File` 时，数组参数被原生进程边界拆成额外位置参数，脚本在进入校验逻辑前报 `A positional parameter cannot be found`。改为当前 PowerShell 进程直接调用同一脚本并传入 typed arrays 后，gate 以 `Changed=5 / Rules=1 / Required=6 / Logs=6` 通过。

这是调用方式错误，不是 mapping、源码、Automation 或构建失败。没有 C3859、C1076、系统代码 1455、UBT 非零退出、Automation failure 或外层超时。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Automation 验证了纯调度/账本及 unavailable Character 边界，没有在测试世界中执行真实 Character 位移。真实 collision、连续画面手感、取消/销毁、输入与 action lifecycle 联动属于后续产品接线及 F 阶段验证。

P10.4 建议增加 action lifecycle coordinator：由唯一产品 owner 驱动 elapsed、执行 segment、接纳 receipt，并在 cancel/end/destroy 时显式关闭 session；SpiritEnergy 仍等待真实 balance、revision、恢复与持久化 owner，不能在 coordinator 中临时创建。
