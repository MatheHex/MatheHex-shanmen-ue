# Dev.D.UE.0.0.10.P9.4.r0 Development Log

## 目标

在 P9.0 SpiritShield lifecycle、P9.1 capacity、P9.2 deadline 与 P9.3 typed action-resource authority 之上建立一个原子 composition，使 action Startup、SpiritEnergy reservation、commit-point consumption 与 shield activation 共享同一份确定性历史，并保证失败路径不泄漏半状态。

## 审计结论

1. action orchestrator 已提供 Startup、commit 与 terminal receipts，应继续作为唯一动作事实源；
2. P9.3 resource authority 已提供 reserve/commit/release，不应复制余额或 reservation ledger；
3. P9.0 shield runtime 已提供 prepare/activate/deactivate，不应创建第二个 shield 状态机；
4. 三套 authority 分别正确，但调用方逐步修改会在中途失败时留下跨 authority 半状态；
5. 本阶段需要 candidate-copy transaction 把三者组合，而不是改写各自内部语义；
6. 正式 SpiritShield cost 与产品 resource owner 仍未冻结，只能接受调用方 typed input。

## 设计决策

1. `FShanmenSpiritShieldActionCoordinator` 持有既有 action runtime 与 shield runtime，resource authority 由外部单写者传入；
2. Begin 在 coordinator/resource 候选副本上 start、prepare、reserve，整体有效后再替换正式状态；
3. Commit 在候选副本上 transition、finalize Commit、TryActivate，整体有效后再提交；
4. Abort 只接受 Cancelled/Interrupted，并固定执行 Release；
5. startup receipt 冻结 action、definition、transition、reservation 与 session identity；
6. terminal receipt 冻结 startup、terminal transition、resource finalization、可选 shield activation 与 outcome；
7. exact replay 返回原 receipt，outcome 改写拒绝；
8. 暴露现有 shield runtime 的受控引用，使 P9.1 capacity 与 P9.2 deadline 可继续工作；
9. deadline deactivation 不使 coordinator 历史失效，也不回滚已消费资源；
10. 不硬编码 cost、不读取产品属性、不加入 World/Actor/Timer/Input。

## 执行序列

1. 审查 action/resource/shield 三套 receipt、revision、replay 与错误边界。
2. 冻结 coordinator state、status、error、outcome 和两类 immutable receipts。
3. 实现确定性 definition binding、session、startup 与 terminal IDs。
4. 实现原子 Begin，覆盖首次成功与 exact replay。
5. 实现原子 Commit，绑定 commit transition、resource Commit 与 shield activation。
6. 实现 Cancelled/Interrupted Abort，绑定 resource Release 且不激活。
7. 实现 terminal replay、conflict 与外部 resource history rewrite 失败关闭。
8. 接入 P9.1 capacity projection 与 P9.2 deadline gate 组合测试。
9. 新增 7 个 focused tests。
10. 新增 `SpiritShieldActionCoordinator` changed-path 规则，强制 7 个组成 authority 测试组。
11. mapping 从 `73` 增至 `74`；self-test 从 `108` 增至 `110`。
12. Editor candidate 6 actions，原生退出 `0`。
13. 执行八组 Automation，合计 `484` success、`0` fail，全量 `368/368`。
14. implementation gate `Changed=5 / Rules=2 / Required=7 / Logs=8` 通过。
15. 静态边界、`git diff --check`、cached diff check 通过。
16. Editor final up-to-date success；Game final 5 actions，原生退出 `0`。
17. 生成同名 Report/Log，执行 exact-stage、commit、push 与远端 SHA 核验。

## 原子状态流

```text
Begin
  action Idle -> Startup
  shield Uninitialized -> Prepared
  resource available -> Reserved
  all valid -> commit both candidates
  any reject -> commit neither

Commit
  action Startup -> Active (cross commit point)
  resource Reserved -> Commit
  shield Prepared -> Active
  all valid -> commit both candidates + terminal receipt
  any reject -> commit neither

Abort(Cancelled | Interrupted)
  action Startup -> terminal
  resource Reserved -> Release
  shield remains Prepared; no activation receipt

exact replay -> original receipt; no extra revision
conflicting/external-rewritten proof -> rejected; no half activation
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `SpiritShieldAction-final.log` | 7 | 0 | yes | 0 | `22D5E7B3C88974599367B46F134878E965AA389AAA52ADB9953CF304AF351DC2` |
| `ActionResource-final.log` | 7 | 0 | yes | 0 | `2F4CA124403D48C303A62D04DEC5568E48474D2D90972A4E8BD22602880915F7` |
| `ActionLifecycle-final.log` | 1 | 0 | yes | 0 | `31A14CDF79DBC178D6F490204AAF907E822BC8EBD176D0DC570B108A68E15EFF` |
| `SpiritShield-final.log` | 24 | 0 | yes | 0 | `A8D6AC3B04C4239D03894F9716631F4F4454697C945D35DACC3019EF4D04B6E5` |
| `SpiritShieldCapacity-final.log` | 6 | 0 | yes | 0 | `17A14522E58098291ED3B65EBF878E21E95A104E2CBFC61D92EEC431852D3564` |
| `SpiritShieldDeadline-final.log` | 6 | 0 | yes | 0 | `16C666CE7041845655476DCFC51603221CED93D70425CBC3572544E10F016AC6` |
| `CombatRuntime-final.log` | 65 | 0 | yes | 0 | `391805573B5AC00C920157B729DF911F89C42CE7C43AC4CA94B99D9B466E18C2` |
| `Shanmen-0_0_10-final.log` | 368 | 0 | yes | 0 | `1B4C2F6C247543510B63A6451BE326339DB74DC7E43AB6BB5A46232EC457A053` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=74
SELF_TEST: PASS 110/110
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=2 Required=7 Logs=8
git diff --check: PASS
BOUNDARY_SCAN_MATCHES=0
```

- required groups：SpiritShieldAction、ActionResource、ActionLifecycle、SpiritShield、SpiritShieldCapacity、SpiritShieldDeadline、CombatRuntime；
- mapping SHA-256：`85EFE6B1CBAA610D6D5428DB2F74AA41120293683545BDB204C4644DA2B6FAB7`；
- self-test SHA-256：`1B5D05C596E8B6DAA5B317BF61696F6FCF757FFD9FDC527E55549FCD961AB504`；
- code/scripts：`5 files / +1533 / -0`。

## 构建证据

| Build | Actions | Time | Exit | SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | 6 | 40.75s | 0 | `E7158594878A54AE089CCDAC6013500ACFF04B8F10E20278192C70F9191A6907` |
| Editor final | 0 | 0.91s | 0 | `587AC392E511FBDC44EA17CC59B65BFC4D97EAD9DC16920989642B954CE14CB6` |
| Game final | 5 | 31.58s | 0 | `DDA5F5DADBD89B520FED2294B2DCC8638CBA58663AE8268AA4238896ACD53C68` |

## 真实异常

没有源码、构建或目标测试失败。Automation 启动保留既有非目标平台 SDK metadata 与测试发现前诊断；所有目标组随后 Success，进程和 UBT 原生退出码均为 `0`。

## P/F 边界

仅执行 P 阶段纯值实现、无头 Automation、静态/门禁和 Editor/Game Development 构建。没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P9.5 应建立产品拥有的 SpiritEnergy adapter/host，把正式玩家资源 authority、内容定义 cost 与输入命令接到本 coordinator；在 resource owner、恢复和持久化规则冻结前，不创建临时数值或第二套产品账本。
