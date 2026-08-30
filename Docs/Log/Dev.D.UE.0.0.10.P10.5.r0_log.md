# Dev.D.UE.0.0.10.P10.5.r0 Development Log

## 目标

把 P10.4 action coordinator 接入一个唯一 Spirit Evasion 产品 host：由 host 组装 action/window/request/preflight/motion/coordinator，接收绝对时间样本，并统一路由 recovery、cancel、interruption、owner end 与 execution failure；不复制 session 权威，不接临时 SpiritEnergy，不启动真实产品。

## 审计结论

1. P10.0 已有 exact action orchestrator 与 Spirit Evasion window；
2. P10.1 已有 direction intent 和 movement request；
3. P10.2 已有 policy plan 与 WorldStatic preflight；
4. P10.3 已有 externally-clocked motion session、command/receipt 与 shared swept mutation seam；
5. P10.4 已有 exact action/motion coordinator 与显式 termination proof；
6. 缺口是没有一个产品 owner 同时持有 action runtime 和 coordinator，也没有 start time、Recovery 完成和 owner teardown 的统一入口；
7. 现阶段直接新增 Tick component 会把产品生命周期与 domain contract 混在一起，因此先建立纯 host，真实 component 保持下一层薄适配器。

## 设计决策

1. Host 是 action runtime 与 coordinator 的唯一 owner；
2. Host 不新增第二个 phase/state enum 作为权威；
3. TryStart 在候选对象上事务性组装 P10.0–P10.4 全链；
4. StartResult 保留 startup、active commit、window 与 preflight proof；
5. HostId 绑定 CoordinatorId、ActivationId 与 start-time bits；
6. 产品传入 absolute time，host 只转换为 coordinator elapsed；
7. time sample 必须有限、不早于 start、不得 rewind；
8. normal completion/block 只进入 Recovery；
9. Recovery 必须显式 finish；
10. cancel/interruption/owner end/execution failure 原子关闭 action 与 active motion；
11. terminal signal 幂等；
12. preflight port 是启动期唯一 World capability；
13. execution port 继续使用 P10.4 typed boundary；
14. Character adapters 只在调用期存在，host 不保存 Actor；
15. actual movement 继续由 P10.3 executor 委托 shared `MoveCharacterSwept`；
16. 不接 input、Tick、Timer、clock source 或 SpiritEnergy；
17. regression mapping 要求 host 以及它组合的九个上下游/兼容测试组；
18. 真实 component 与 World/collision 验收延后。

## 执行序列

1. 对照 Spirit Shield、Formation、Thrown Weapon 的 host/lifecycle 结构。
2. 确定纯 ProductHost + 窄 ports 方案，不在本阶段建立 Tick 状态机。
3. 定义 start/step status、error 与 proof result。
4. 实现一次性 Character preflight adapter。
5. 实现 action start、commit、window、intent、request、plan、preflight、motion、coordinator 的事务性启动。
6. 实现 deterministic HostId 与 exact `IsValid` 重放检查。
7. 实现 absolute-time advance、Recovery、显式 completion 与 typed Character convenience。
8. 实现 cancel、interrupt、owner end、execution unavailable 的原子收口。
9. 新增七个 focused tests。
10. regression map 增至 82 rules；self-test 增至 124 cases。
11. 首次 Editor candidate 发现测试 API 兼容错误并保留失败日志。
12. 用标准库 quiet NaN 修正测试，Editor corrected candidate 成功。
13. 补 production Character convenience 后再次 candidate 编译成功。
14. focused candidate `7/7`。
15. 串行执行十组正式 Automation，共 `525` success、`0` fail。
16. changed-file gate 以 `Changed=5 / Rules=1 / Required=10 / Logs=10` 通过。
17. 静态 Actor/World/mutation/clock/RNG/resource 边界扫描通过。
18. `git diff --check` 与精确五文件实现暂存区通过，`+1517/-0`。
19. Editor final 0 actions，Game final 4 actions，原生退出均为 0。
20. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
frozen action + definition + policy + trajectory + direction + start time
  -> ActionRuntime Start -> Startup -> Active commit
  -> SpiritEvasionWindow
  -> movement intent/request
  -> P10.2 movement plan
  -> read-only preflight port
  -> P10.3 motion plan
  -> P10.4 coordinator
  -> deterministic ProductHost

absolute now + typed execution port
  -> validate host identity and monotonic time
  -> coordinator(now - start)
      waiting/segment -> remain Active
      completed/blocked -> action Recovery
      owner/execution failure -> motion Terminated + action Interrupted

cancel/interruption/owner end
  -> close active motion when present
  -> action Interrupted

finish recovery
  -> action Idle/Completed
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Product SpiritEvasionProductHost | 7 | 0 | `4C1FE60CC9D470878CCC0AE9F860D0329F8C031662DD83FB28EA4AE6E2731724` |
| Product SpiritEvasionActionCoordinator | 7 | 0 | `A33C03323F84DFAC108D594AD93C916E7826C2EE52F6EC2EA77BA41D43FEE9F3` |
| Product SpiritEvasionMotionRuntime | 7 | 0 | `4DD34A180D0696341F2CD00FCD1E3D6A92D0699D483B5B8003FA17932FF613EB` |
| Product SpiritEvasionMovementAdapter | 5 | 0 | `EA737127AA30DD9FC4C98DA2092506B720DC2E1959A2F611576FA888D1ACD90B` |
| CombatRuntime SpiritEvasionMovement | 5 | 0 | `CC448B591D0B6E3F9BB3B0D006498C100CE0AD165ADA549A591B1E9B15F6E205` |
| CombatRuntime SpiritEvasion | 11 | 0 | `0304016BC4DA9FAB01DE24B9E08EC3CD0FD5ED087A7EBCB625BBBB2F4DEEF2D8` |
| CombatRuntime ActionLifecycle | 1 | 0 | `A6BF5B86073612BA56E57E2720081D8A7F9C2D80805BE082F1B1B2D7C323C160` |
| EnemySkillFramework | 44 | 0 | `A9CA679DF34E005578D48107385B95C29DCDAFCB7032B01371963CF10A504588` |
| V2RangedCompatibility | 22 | 0 | `A37461D63B78CADB0481C3E818729874CD45414DF09ED096914097F070DAE574` |
| Shanmen.0_0_10 | 416 | 0 | `EA53E6B7801BA837EA4998FB0AE677ECA54240EBF01B9E5C7A7AE21B171AB8BB` |

所有正式进程原生退出码为 `0`；十份日志均有 selected queue-empty，且没有 selected fail、fatal、unhandled 或 ensure。

focused candidate r1：`7/7`，SHA-256 `834AB47032B73B5A743728F31359CE1B99F1E7E074F47BB115ECF685392A8C19`。

## 门禁与构建

```text
REGRESSION_MAP_JSON: PASS Rules=82
SELF_TEST: PASS 124/124
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=10
git diff --check: PASS
DIRECT_MUTATION_HITS=0
WORLD_CLOCK_TIMER_TICK_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
HOST_STORED_ACTOR_POINTERS=0
PRODUCTION_DELEGATIONS=2
Editor first candidate: 5 actions / 21.73s / exit 6
Editor corrected candidate: 4 actions / 7.28s / exit 0
Editor convenience candidate: 5 actions / 8.58s / exit 0
Editor final: 0 actions / 0.90s / exit 0
Game final: 4 actions / 23.46s / exit 0
```

mapping SHA-256：`B306DC9BDC8FFFD05A425F7F907359326BC1D7A63A3A96EF58DBA7F30DCD9B08`；self-test SHA-256：`D0162B313C0F70D3FFF892875D192DC718C926BCEF9B030F62E818F3B222C3A9`。

最终 `UnrealEditor-demo_map.dll`：`12303360` bytes / SHA-256 `397C9A6EEA2BFFBFD5EB192ECA8B409FB72C0EBBFD43F9B944E99580B7CE6145`；`demo_map.exe`：`353776640` bytes / SHA-256 `1E24D0E65260D330A52054792BC4B3541C2CFD93AEEA7ACC94711D75E2533ACC`。

## 真实异常

首次 candidate 的新测试使用 `TNumericLimits<double>::QuietNaN()`；UE 5.8 该类型没有此 API，产生 C2039/C3861，UBT 为 `OtherCompilationError`、原生退出 `6`。生产代码未报错。

修正为 `std::numeric_limits<double>::quiet_NaN()` 后 candidate 原生退出 `0`。该失败是测试源码兼容错误，不是 Windows commit memory、页面文件、C3859/C1076、系统代码 1455 或外层超时。后续 focused/formal Automation、门禁与双目标构建均通过。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Fake ports 验证的是 host/domain contract；production Character adapters 仅完成编译，尚未在测试 World 中执行真实 preflight、collision 或 swept movement。

## 下一步

P10.6 建立薄 `UActorComponent`/输入桥，把真实 owner 的 action snapshot、policy/content、absolute time、Character、cancel/end/destroy 信号委托给本 host；不复制 action/motion 状态。真实碰撞与手感进入 F 阶段，SpiritEnergy 等待真实资源 authority 后再接入。
