# Dev.D.UE.0.0.10.P10.4.r0 Development Log

## 目标

把 P10.3 externally-clocked motion session 接入一个唯一 action lifecycle coordinator：驱动 elapsed、segment execution 与 receipt acceptance，并在 cancel、action end、interruption、owner end 或 execution unavailable 时留下显式 termination proof；不接 SpiritEnergy、输入或真实产品 host。

## 审计结论

1. P10.0 action authority 已能区分 Active、Recovery、Cancelled、Interrupted 与 terminal reason；
2. Spirit Evasion window 已绑定 exact action snapshot 和 Startup→Active commit receipt；
3. P10.3 session 能调度 command 与接纳 movement receipt，但原状态只有 Active/Completed/Blocked；
4. Character 丢失、action 失活或产品取消后，P10.3 的 pending command 缺少显式关闭语义；
5. P10.3 executor 已是新功能唯一 mutation seam，并复用 shared swept displacement；
6. 因此本阶段补 termination receipt，并让 coordinator 只编排现有 action/window/session/executor 权威。

## 设计决策

1. Termination 只允许从 Active session 发生；
2. termination reason 区分 explicit cancel、action end、interruption、owner end 与 execution unavailable；
3. termination receipt 记录待放弃 CommandId 和已提交 motion 水位；
4. receipt identity 绑定 exact session、reason、segment count、distance 与 elapsed bits；
5. 关闭成功后清空 pending command 并进入不可重开的 Terminated；
6. coordinator 必须绑定 exact motion plan、Ready preflight、window 与 runtime；
7. CoordinatorId 绑定 SessionId 与 WindowReceiptId；
8. 每次 TryAdvance 重新核对完整 action snapshot；
9. window 失活优先于 elapsed/segment execution，并映射为 action termination；
10. elapsed 仍由外部提供，coordinator 不拥有 World、clock、Timer 或 Tick；
11. 一步只发放和执行一个 command；
12. execution failure 也要关闭 candidate session，不能留下 pending；
13. typed execution port 是 coordinator 唯一可变能力；
14. fake port 用于无头 domain tests，生产 port 委托现有 P10.3 executor；
15. coordinator 本身不保存 Actor；
16. actual movement 继续只有 shared `MoveCharacterSwept`；
17. mapping 同时要求 coordinator、session、adapter、window/action 和旧 displacement 兼容证据；
18. SpiritEnergy 与真实产品 host 延后到各自唯一 owner。

## 执行序列

1. 审查 action orchestrator、Spirit Evasion window、P10.3 motion session 与 shared displacement。
2. 给 motion session 增加 Terminated 状态、五种原因和 deterministic termination receipt。
3. 实现事务性 `TryTerminate`，并在终态拒绝旧 receipt、重复关闭和新 command。
4. 定义 typed segment execution port 与 production Character adapter。
5. 实现 exact action/window/session coordinator 和 deterministic CoordinatorId。
6. 实现 Waiting、SegmentCommitted、Completed、Blocked、Terminated 与 Rejected step results。
7. 把 Recovery/Cancelled、Interrupted、owner unavailable 与 executor unavailable 映射为显式关闭。
8. 新增七个 coordinator tests 和一个 motion termination test。
9. regression map 增至 81 rules；self-test 增至 122 cases。
10. Editor candidate 7 actions，原生退出 0。
11. coordinator focused candidate `7/7`，未发生源码修正重跑。
12. 串行执行九组正式 Automation，共 `511` success、`0` fail。
13. changed-file gate 以 `Changed=8 / Rules=2 / Required=9 / Logs=9` 通过。
14. executor/mutation/World/clock/RNG/resource 静态边界扫描通过。
15. `git diff --check` 与精确八文件实现暂存区通过，`+1314/-1`。
16. Editor final 0 actions，Game final 6 actions，原生退出均为 0。
17. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
exact ActionRuntime + SpiritEvasionWindow
  + P10.3 MotionPlan + matching Ready Preflight
      -> TryOpen coordinator
      -> CoordinatorId(SessionId, WindowReceiptId)

product-owned monotonic elapsed + typed execution port
  -> validate exact action/window lifecycle
       inactive -> terminate(ActionEnded | ActionInterrupted)
       active   -> issue at most one due SegmentCommand
                    -> execution port
                    -> P10.3 ExecuteSwept
                    -> shared MoveCharacterSwept
                    -> SegmentReceipt
                    -> transactional acceptance
                         active    -> SegmentCommitted
                         complete  -> Completed
                         blocked   -> Blocked

cancel / owner end / execution unavailable
  -> TerminationReceipt(pending command + committed watermark)
  -> clear pending
  -> Terminated
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Product SpiritEvasionActionCoordinator | 7 | 0 | `42C4DEC2868666FA63B676A954F874F7B2BAC081FB34A6D9F115B15A9A999DDB` |
| Product SpiritEvasionMotionRuntime | 7 | 0 | `90BB52802DEDB1A42C378F16985075CAEADBC90825DB578B514248A81F32D486` |
| Product SpiritEvasionMovementAdapter | 5 | 0 | `F5D37A7355742974CF1539083D918F344387637F22F87EC2E24605B52D11C417` |
| CombatRuntime SpiritEvasionMovement | 5 | 0 | `4E875871F83B684674322F598C29D34BF2DD962D1AA16BFC74EFCE3F4B82856F` |
| CombatRuntime SpiritEvasion | 11 | 0 | `19E9F9E5CD206351489906F3FD21B464C5B4A52B981421B959C7CAD281E471B0` |
| CombatRuntime ActionLifecycle | 1 | 0 | `C32CE8E38658C362D8B06E0FEAAA2CA81D1810BB26770920CD56EBC72D032FA3` |
| EnemySkillFramework | 44 | 0 | `4BDCEBAE68F0BE713D6B69B8834586DA3DE1AFD3E7439F70201725B4599DA7FC` |
| V2RangedCompatibility | 22 | 0 | `65AFA31F573B40E810BD64B3CC51FF6F17F04E49ED8A0B971BCFD42152B80280` |
| Shanmen.0_0_10 | 409 | 0 | `27CC4D2878E384CF788FFB7EF957E1977FFDCB6A52697F718C4C21EAFAF4596D` |

所有正式进程原生退出码为 `0`；九份日志均有 selected queue-empty，且没有 selected fail、fatal、unhandled 或 ensure。

## 门禁与构建

```text
REGRESSION_MAP_JSON: PASS Rules=81
SELF_TEST: PASS 122/122
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=9 Logs=9
git diff --check: PASS
COORD_EXECUTOR_DELEGATIONS=1
COORD_DIRECT_MUTATION_HITS=0
RUNTIME_SHARED_MOVE_AUTHORITY_CALLS=1
WORLD_CLOCK_TIMER_TICK_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
Editor candidate: 7 actions / 32.69s / exit 0
Editor final: 0 actions / 0.94s / exit 0
Game final: 6 actions / 28.88s / exit 0
```

## 真实异常

没有源码、Automation、门禁或构建失败；没有环境内存错误、非零原生退出、外层超时或重试掩盖。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Fake execution port 验证的是 coordinator domain contract；production Character port 只完成编译，尚未在测试 World 中执行真实 swept movement。

## 下一步

P10.5 建立唯一 Spirit Evasion 产品 host/component，把 current action runtime、elapsed、Character execution port 和 cancel/end/destroy 信号路由到 coordinator；不复制 session 权威。真实 collision/手感进入 F 阶段，SpiritEnergy 等待真实资源 authority 后再接入。
