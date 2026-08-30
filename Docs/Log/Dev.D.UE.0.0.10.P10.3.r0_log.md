# Dev.D.UE.0.0.10.P10.3.r0 Development Log

## 目标

把 P10.2 的 movement plan/preflight receipt 推进为确定性分段轨迹与逐段 swept movement receipt；冻结持续时间、分段、命令身份和实际位移账本，但不接 SpiritEnergy、无敌帧、输入或完整 action lifecycle。

## 审计结论

1. P10.1 已冻结 action、active window、movement policy id 与 planar direction；
2. P10.2 已冻结 requested/minimum/clearance，并以只读 WorldStatic sweep 生成 plan-bound preflight；
3. 旧 P6 连续位移按 elapsed 比例计算本段目标距离，再调用 shared `MoveCharacterSwept`；
4. 旧实现把调度状态、command identity 和实际位移 receipt 隐含在 Tick 状态里，不能独立重放或审计；
5. shared displacement 已是 enemy skill、knockback 与产品验证共用的唯一 swept authority；
6. 因此 P10.3 保留线性语义，但把 trajectory、session、command、receipt 和 mutation seam 显式拆分。

## 设计决策

1. duration 与 segment count 必须显式 capture，不提供 gameplay 默认值；
2. MotionPlanId 绑定 P10.2 PlanId 和 exact trajectory values；
3. session 必须消费 matching `Ready` preflight；
4. target distance 取 preflight resolved distance；
5. elapsed 由外部产品 owner 提供，session 不拥有 clock/World/Tick/Timer；
6. 使用累计目标差计算每段请求距离；
7. 一次最多发放一个 pending command；
8. 相同 elapsed 可在 receipt 接纳后追赶多个到期段；
9. CommandId 绑定 session、plan、ordinal、schedule、distance 和 direction；
10. ReceiptId 绑定 command、actual distance 和 blocked 状态；
11. 未阻挡短位移失败关闭；
12. receipt 以 session 候选副本事务性接纳；
13. blocked receipt 立即终止 session；
14. executor 是新代码唯一 mutation seam；
15. executor 复用 shared `MoveCharacterSwept`，不另造移动算法；
16. actor-free receipt 不持有产品对象；
17. mapping 覆盖本轮 runtime、P10.2 adapter、P10.1 movement、enemy 与 V2 兼容；
18. SpiritEnergy 与 action cancellation 留给后续唯一 owner。

## 执行序列

1. 审查 P10.1、P10.2、旧 P6 segmented displacement 和 shared movement authority。
2. 定义 trajectory capture/snapshot 与 deterministic motion plan。
3. 定义 segment command/receipt 和 replay-stable identities。
4. 实现 externally-clocked motion session、pending 防重复和完成/blocked 状态。
5. 实现唯一 `ExecuteSwept` 产品 mutation seam。
6. 自查 receipt 接纳路径，改为候选 session 全量验证后提交。
7. 新增六个 focused automation tests。
8. regression map 增至 80 rules；self-test 增至 120 cases。
9. Editor candidate 5 actions，原生退出 0。
10. focused candidate `6/6`，未发生源码失败或修正重跑。
11. 串行执行六组正式 Automation，共 `483` success、`0` fail。
12. mapping JSON `80` rules，self-test `120/120`。
13. gate 首次因 PowerShell 到 pwsh 的数组参数转发方式错误未进入校验。
14. 同一 PowerShell 进程传 typed arrays 后，changed-file gate `5/1/6/6` 通过。
15. mutation/World/clock/RNG/resource 边界扫描通过。
16. `git diff --check` 与精确五文件实现暂存区通过，`+1142/-0`。
17. Editor final 0 actions，Game final 4 actions，原生退出均为 0。
18. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
P10.2 MovementPlan + matching Ready Preflight
  + explicit TrajectoryCapture(duration, segment_count)
      -> immutable TrajectorySnapshot
      -> deterministic MotionPlan / MotionPlanId
      -> MotionSession(target = preflight resolved distance)

external monotonic elapsed
  -> due boundary?
       no  -> no command
       yes -> one deterministic pending SegmentCommand
                 -> ExecuteSwept(Character, command)
                 -> shared MoveCharacterSwept
                 -> actor-free SegmentReceipt(actual distance, blocked)
                 -> transactional session acceptance
                      full final segment -> Completed
                      blocked segment    -> Blocked
                      otherwise          -> next ordinal
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Product SpiritEvasionMotionRuntime | 6 | 0 | `B64626F3D9671AFDF3F9CE138D552796E4C3FE1F1C318F15516502D54FAE8C61` |
| Product SpiritEvasionMovementAdapter | 5 | 0 | `B489E6EEAB132BAEB80CF4067106A28C2E9FE2B94C2F0DE873A542308206A4AD` |
| CombatRuntime SpiritEvasionMovement | 5 | 0 | `096E557A75C26016350DA07DFF9448D6AAA26BCE4A8A09E71F105319AA417A2F` |
| EnemySkillFramework | 44 | 0 | `1D9F928E9C3EBEC2C837FAC1EA71A6B2A966B14F6A08607F62F4F4188F74438C` |
| V2RangedCompatibility | 22 | 0 | `D968783B0DC08B02EA84F27D527DC3E3D4623984FF63681984A3C9EB563FDEEC` |
| Shanmen.0_0_10 | 401 | 0 | `F19B6944004D08616386FBCD1774D2B3B906A634397CEF29EEBD3C45C29FA561` |

所有正式进程原生退出码为 `0`，日志均有 selected queue-empty，且没有 selected fail、fatal、unhandled 或 ensure。

## 门禁与构建

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
Editor candidate: 5 actions / 29.99s / exit 0
Editor final: 0 actions / 0.91s / exit 0
Game final: 4 actions / 23.61s / exit 0
```

## 真实异常

- 第一次 changed-file gate 调用经 `pwsh -File` 转发两个数组时，native process boundary 把后续数组元素当成位置参数，脚本在校验前报参数绑定错误；
- 改为当前 PowerShell 进程直接调用原脚本后，`Changed=5 / Rules=1 / Required=6 / Logs=6` 通过；
- 这是调用方式错误，没有修改 gate 逻辑，也不属于源码、Automation 或构建失败；
- 没有 C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P10.4 由唯一 action lifecycle coordinator 驱动 elapsed、segment executor 和 receipt acceptance，并明确 cancel/end/destroy 终止语义。真实 collision 连续运动与手感留到 F 阶段；SpiritEnergy 继续等待真实 balance/revision/recovery/persistence owner。
