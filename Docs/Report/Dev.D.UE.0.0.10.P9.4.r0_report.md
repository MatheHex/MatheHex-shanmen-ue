# Dev.D.UE.0.0.10.P9.4.r0 Report

## 1. 结论

P9.4 **PASS**。本阶段把既有 action lifecycle、P9.3 typed resource transaction 与 P9.0 SpiritShield runtime 组合为一个原子协调器：Begin 同时建立 Startup、Prepared shield 与 SpiritEnergy reservation；Commit 同时跨越 action commit point、消费 reservation 并激活 shield；commit 前 Cancelled/Interrupted 只释放 reservation，不激活护盾。

最终验证为 SpiritShieldAction `7/7`、ActionResource `7/7`、ActionLifecycle `1/1`、SpiritShield 聚合组 `24/24`、SpiritShieldCapacity `6/6`、SpiritShieldDeadline `6/6`、CombatRuntime `65/65`、`Shanmen.0_0_10` 全量 `368/368`。八份日志合计 `484` 条 success、`0` fail；changed-file gate、mapping self-test、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

本阶段仍未硬编码 SpiritShield 的最终灵力消耗，也未声明完成产品输入或玩家属性接线。协调器只消费调用方已经冻结的 typed cost 与 resource authority；正式数值和 owner 由后续产品层决定。

## 2. 功能性

### 2.1 原子 Begin

`FShanmenSpiritShieldActionCoordinator::Begin` 复用现有 action orchestrator、shield runtime 与 resource authority，在候选副本中依次完成：

- action `Idle -> Startup`；
- shield `Uninitialized -> Prepared`；
- 精确 `Shanmen.Resource.SpiritEnergy` reservation；
- session/startup receipt 的确定性派生与完整校验。

只有三项都成功且候选状态整体有效时，协调器和外部资源 authority 才一起替换正式状态。余额不足、foreign owner、非法定义或任一 proof 不一致均不会留下半启动、半准备或半预留状态。

### 2.2 原子 Commit 与激活

`Commit` 在候选副本中完成：

- action `Startup -> Active` 且首次跨越 commit point；
- P9.3 reservation disposition `Commit`；
- P9.0 shield `Prepared -> Active`；
- terminal receipt 绑定 action transition、resource finalization 与 shield activation。

任一步拒绝时，原协调器与传入 resource authority 均不接收候选状态。成功后资源只消费一次，shield activation 与同一 action/session 的 terminal proof 绑定。

### 2.3 Commit 前终止

`Abort(Cancelled|Interrupted)` 只接受 Reserved 状态：

- action 从 Startup 进入相应 terminal phase；
- reservation disposition 固定为 `Release`；
- shield 保持 Prepared，不生成 activation receipt；
- current resource 不减少，reserved 归零。

已激活历史不能被改写为取消；已取消历史也不能被改写为激活。

### 2.4 重放、冲突与后续 authority

- exact Begin replay 返回原 startup receipt，不重复 reservation；
- exact Commit/Abort replay 返回原 terminal receipt，不重复消费或释放；
- 同一历史的不同 terminal outcome 返回 `FinalizationConflict`；
- 外部改写 reservation finalization 后再 Commit 会失败关闭，不能半激活；
- Active shield 继续由 P9.1 capacity authority 投影；
- P9.2 deadline gate 可经协调器的 shield runtime 精确结束已激活护盾；
- deadline 后协调器仍保持有效历史，资源消费不会被回滚。

## 3. 完整性与兼容性

- 复用 `FShanmenActionOrchestrator`，没有建立第二套动作状态机；
- 复用 `FShanmenActionResourceAuthority`，没有建立第二套资源账本；
- 复用 `FShanmenSpiritShieldRuntime`、capacity authority 与 deadline gate；
- startup/terminal proof 为 BlueprintReadOnly USTRUCT，跨层只读；
- operation result 暴露 typed status、coordinator error 与嵌套 resource error；
- 未修改 P9.0—P9.3 既有生产实现；
- 未依赖 `demo_map`、World、Actor、ApplyDamage、Timer、Tick、输入、UI 或随机数；
- 未宣称完成产品 host、快捷栏输入、资源恢复、持久化或 HUD。

## 4. 关键不变量

1. Coordinator 只组合既有三套 authority，不复制其内部规则；
2. Begin 的 action Startup、shield Prepared 与 resource reservation 要么全部提交，要么全部不提交；
3. Commit 的 action transition、resource consumption 与 shield activation 要么全部提交，要么全部不提交；
4. pre-commit Cancelled/Interrupted 只能 Release，不能 Commit；
5. terminal outcome 与 action transition、resource disposition、shield receipt 必须相互一致；
6. exact replay 不推进 resource revision，不重复激活；
7. conflicting replay 不改变任何正式状态；
8. startup、session 与 terminal identities 由冻结输入和既有 receipts 确定性派生；
9. 外部 authority 历史不匹配时失败关闭；
10. capacity/deadline 后续 mutation 不改写已冻结的 activation 与消费历史。

## 5. 测试覆盖

新增 7 个 focused tests：

- `BeginAndReplay`：原子 Startup/Prepared/Reserve 与 exact Begin replay；
- `AtomicCommitActivation`：commit-point 消费与同 action shield 激活；
- `PreCommitAbortRelease`：Cancelled/Interrupted 均 release 且不激活；
- `TerminalReplayConflict`：terminal exact replay 与 outcome 改写拒绝；
- `FailClosedAtomicity`：余额不足、foreign owner、外部 history rewrite 均无半状态；
- `CapacityDeadlineComposition`：已激活 receipt 接入 P9.1 capacity 与 P9.2 deadline；
- `DeterministicReplay`：等价冻结输入复现 startup/terminal identities。

独立依赖组和宽回归均执行，避免只靠新测试自证：ActionResource `7/7`、ActionLifecycle `1/1`、SpiritShield 聚合 `24/24`、Capacity `6/6`、Deadline `6/6`、CombatRuntime `65/65`、全量 `368/368`。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritShieldActionCoordinator.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldActionCoordinator.cpp`。

测试与流程：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldActionCoordinatorTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

代码与脚本为 `5` 个文件、`1533` insertions、`0` deletions；加入本 Report 与同名 Development Log 后 exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `SpiritShieldAction-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShieldAction` | 7 | 0 | `22D5E7B3C88974599367B46F134878E965AA389AAA52ADB9953CF304AF351DC2` |
| `ActionResource-final.log` | `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `2F4CA124403D48C303A62D04DEC5568E48474D2D90972A4E8BD22602880915F7` |
| `ActionLifecycle-final.log` | `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `31A14CDF79DBC178D6F490204AAF907E822BC8EBD176D0DC570B108A68E15EFF` |
| `SpiritShield-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShield` | 24 | 0 | `A8D6AC3B04C4239D03894F9716631F4F4454697C945D35DACC3019EF4D04B6E5` |
| `SpiritShieldCapacity-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity` | 6 | 0 | `17A14522E58098291ED3B65EBF878E21E95A104E2CBFC61D92EEC431852D3564` |
| `SpiritShieldDeadline-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline` | 6 | 0 | `16C666CE7041845655476DCFC51603221CED93D70425CBC3572544E10F016AC6` |
| `CombatRuntime-final.log` | `Shanmen.0_0_10.CombatRuntime` | 65 | 0 | `391805573B5AC00C920157B729DF911F89C42CE7C43AC4CA94B99D9B466E18C2` |
| `Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 368 | 0 | `1B4C2F6C247543510B63A6451BE326339DB74DC7E43AB6BB5A46232EC457A053` |

每份日志均有且仅有一个目标 RunTests 命令，并具备 queue-empty；selected Fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=74
SELF_TEST: PASS 110/110
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=2 Required=7 Logs=8
git diff --check: PASS
BOUNDARY_SCAN_MATCHES=0
```

- mapping SHA-256：`85EFE6B1CBAA610D6D5428DB2F74AA41120293683545BDB204C4644DA2B6FAB7`；
- self-test SHA-256：`1B5D05C596E8B6DAA5B317BF61696F6FCF757FFD9FDC527E55549FCD961AB504`。

## 8. 构建证据

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 6 / 40.75s | 0 | `E7158594878A54AE089CCDAC6013500ACFF04B8F10E20278192C70F9191A6907` |
| Editor final | Succeeded, target up to date | 0 / 0.91s | 0 | `587AC392E511FBDC44EA17CC59B65BFC4D97EAD9DC16920989642B954CE14CB6` |
| Game final | Succeeded | 5 / 31.58s | 0 | `DDA5F5DADBD89B520FED2294B2DCC8638CBA58663AE8268AA4238896ACD53C68` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1079808` bytes，SHA-256 `7D24680AA3A399F9E5A992795B3146C48358206B0199791DABF16AB65F171727`；
- `demo_map.exe`：`353473024` bytes，SHA-256 `328DD46D2699FB19AD2087691692DE3AFA75485A9DD2E9C27D534B0F07B39A2A`。

## 9. 真实异常

本阶段没有源码、构建或目标测试失败。Automation 启动仍保留 UE 5.8 既有的非目标平台 SDK metadata 与测试发现前诊断；Win64 SDK 有效，所有选中目标随后逐项 Success。未发生 C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值开发、代码审查、无头 Automation、静态扫描、changed-file gate、Editor/Game Development 构建与 Git 证据。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P9.5 建立产品拥有的 SpiritEnergy adapter/host：从正式玩家资源 authority 捕获 typed snapshot，把内容定义的 cost 交给本协调器，并由明确输入命令触发 Begin/Commit/Abort。产品 owner、恢复与持久化规则未确定前，不应把临时 float 或第二套资源状态接入运行时。
