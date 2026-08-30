# Dev.D.UE.0.0.10.P9.5.r0 Report

## 1. 结论

P9.5 **PASS**。本阶段闭合了 P9.4 SpiritShield action composition 的提交后生命周期：已激活护盾现在可通过显式结束、deadline 到期、提交后中断或 owner teardown，原子地同步关闭 shield runtime 与 action runtime，并生成确定性 closure receipt。正常结束进入 `Completed`；提交后中断进入独立的 `Interrupted`，不会与 commit 前 `Aborted` 混淆。

最终验证为 SpiritShieldAction `10/10`、ActionResource `7/7`、ActionLifecycle `1/1`、SpiritShield 聚合组 `27/27`、SpiritShieldCapacity `6/6`、SpiritShieldDeadline `6/6`、CombatRuntime `68/68`、`Shanmen.0_0_10` 全量 `371/371`。八份日志合计 `496` 条 success、`0` fail；changed-file gate、mapping self-test、生产边界、`git diff --check`、Editor Development 与 Game Development 均通过。

本阶段没有接入产品输入、最终 SpiritEnergy 数值、恢复或持久化。审计发现 P9.4 尚无 `Activated -> terminal` 契约，直接接产品 Host 会迫使产品层复制 action closure 规则，因此先完成结构闭环；产品 adapter/host 顺延至 P9.6。

## 2. 功能性

### 2.1 原子 graceful completion

`Close(Explicit)` 在协调器候选副本中完成：

- shield `Active -> Deactivated(Explicit)`；
- action `Active -> Recovery -> Idle(Completed)`；
- closure receipt 绑定原 activation terminal、deactivation 与两次 transition；
- coordinator `Activated -> Completed`。

任一步失败时，正式协调器不接收候选状态。已提交的 SpiritEnergy 保持消费，不因护盾正常结束而退款。

### 2.2 Deadline 不可绕过

`Close(DurationElapsed)` 在 shield 仍为 Active 时固定拒绝，不能伪造“时间已到”。只有 P9.2 deadline gate 根据同一 timeline contract 写入 `Deactivated(DurationElapsed)` proof 后，协调器才复用该 proof 完成 action `Recovery -> Completed`。

因此产品层只负责提供外部时间观测，不需要也不能复制 deadline 判定。

### 2.3 提交后中断

`Close(Interrupted|OwnerEnded)` 固定执行：

- shield 以精确 reason 失活；
- action `Active -> Interrupted`；
- closure outcome 为 post-commit `Interrupted`；
- 不生成 graceful completion transition；
- 已提交资源不回滚。

该路径与 Begin 后、Commit 前的 `Abort(Cancelled|Interrupted)` 明确分离：pre-commit Abort 释放 reservation，post-commit Close 保留已消费事实。

### 2.4 外部 authority、重放与重新装填

- deadline/capacity/产品 owner 可先结束 shield；协调器只接受 exact deactivation reason 并复用原 receipt；
- mismatched external reason 返回 `ClosureConflict`，action 保持 Active，不伪造闭环；
- exact Close replay 返回原 closure receipt，不推进任何 revision；
- Close 后 Commit replay 仍返回原 activation terminal，不重复消费或激活；
- 不同 close reason 不能改写既有 terminal history；
- 闭环后必须显式 `Reset` 才能 Begin 新 action；新预留只推进一次 resource authority revision，旧消费不回滚。

## 3. 完整性与兼容性

- 继续复用 `FShanmenActionOrchestrator`、`FShanmenActionResourceAuthority`、`FShanmenSpiritShieldRuntime` 与 P9.2 deadline gate；
- 未建立第二套 action、shield、resource 或 time authority；
- closure receipt 为 BlueprintReadOnly USTRUCT，跨层只读；
- 既有 P9.4 Begin/Commit/Abort 和 exact replay 语义保持兼容；
- 已被 deadline gate 失活但尚未 action-close 的短暂状态仍为可验证、可恢复状态；
- 未修改 P9.0—P9.3 独立 authority 的生产实现；
- 未依赖 `demo_map`、World、Actor、ApplyDamage、Timer、Tick、轮询或随机数；
- 未声明完成产品 Host、快捷栏输入、SpiritEnergy 恢复、持久化或 HUD。

## 4. 关键不变量

1. pre-commit Abort 与 post-commit Close 是不同状态和不同资源语义；
2. shield deactivation、action terminal transitions 与 closure receipt 要么全部提交，要么全部不提交；
3. `DurationElapsed` 必须由 deadline gate 提供既有 deactivation proof；
4. externally deactivated shield 的 receipt/reason 必须精确匹配 closure 请求；
5. graceful close 固定为 `Active -> Recovery -> Idle(Completed)`；
6. interrupted close 固定为 `Active -> Interrupted`，不得携带 completion transition；
7. closure receipt 必须绑定同一 activation terminal、shield instance 与 action activation；
8. exact replay 不产生第二次 deactivation、transition 或 resource revision；
9. conflicting replay 不改变正式状态；
10. committed resource 在任何 post-commit close 中都不回滚；
11. closed coordinator 必须显式 Reset 才能承载下一 action；
12. closure identity 由冻结 receipts、transition sequence 与 outcome 确定性派生。

## 5. 测试覆盖

SpiritShieldAction focused group 从 7 个增至 10 个测试。新增：

- `ExplicitClosureReplay`：正常完成、closure/Commit exact replay、reason conflict 与 Reset 后 rearm；
- `PostCommitInterrupt`：Interrupted、OwnerEnded、无退款与 exact replay；
- `ExternalDeactivationConflict`：外部失活 reason 不匹配失败关闭、匹配 proof 正常闭环。

既有测试扩展：

- `CapacityDeadlineComposition`：直接伪造 DurationElapsed 被拒，真实 gate proof 后完成 action closure；
- `DeterministicReplay`：等价实例复现 deactivation、两次 action transition 与 closure receipt identity。

独立依赖组和宽回归均执行：ActionResource `7/7`、ActionLifecycle `1/1`、SpiritShield 聚合 `27/27`、Capacity `6/6`、Deadline `6/6`、CombatRuntime `68/68`、全量 `371/371`。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritShieldActionCoordinator.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldActionCoordinator.cpp`。

测试：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldActionCoordinatorTests.cpp`。

生产/测试为 `3` 个文件、`640` insertions、`16` deletions；加入本 Report 与同名 Development Log 后 exact stage 为 `5` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `SpiritShieldAction-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShieldAction` | 10 | 0 | `F1D74819F4C02905C9E5659D7595E47F53D90235CFE1587FE7F8B0750932AE10` |
| `ActionResource-final.log` | `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `AC167E5CAEEE5D437BF07FA3E1FB5A0B8F947E8D0049FBBCC03E5B416BFB64D8` |
| `ActionLifecycle-final.log` | `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `5ACF14B265329B840B006025A4640E8D76341DD7E3DB6B9B4908FF08E03E3B36` |
| `SpiritShield-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShield` | 27 | 0 | `0FEC10275818D20A852FB978C5F930AC9B272C6D37215C08A535A43FD8CDC22E` |
| `SpiritShieldCapacity-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity` | 6 | 0 | `D67406C7BCBA72EA215341AFBD9B6ED21B26D9BC6CB7F126D3EE9811A7EBC2D7` |
| `SpiritShieldDeadline-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline` | 6 | 0 | `D093A17C7F51E2D04553A9540E1F4FF9A2E2CC7552E78535280059085EFFE810` |
| `CombatRuntime-final.log` | `Shanmen.0_0_10.CombatRuntime` | 68 | 0 | `B7AB7D50A9D9CD9FF44934C737E68A6FA509F8FF88953E8EC676ABDCCF0AC94A` |
| `Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 371 | 0 | `6FE2A77FE844B58D2AD80BC420257EC141FAA5B95C611F6AF94DE75EADE91B56` |

每份日志均有且仅有一个目标 RunTests 命令与一个结构化 queue-empty；selected fail/not-run `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=74
SELF_TEST: PASS 110/110
REGRESSION_COVERAGE (implementation): PASS Changed=3 Rules=2 Required=7 Logs=8
REGRESSION_COVERAGE (exact stage): PASS Changed=5 Rules=2 Required=7 Logs=8
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
| Editor candidate | Succeeded | 6 / 36.67s | 0 | interactive capture |
| Editor test increment | Succeeded | 4 / 4.67s | 0 | interactive capture |
| Editor final | Succeeded, target up to date | 0 / 0.89s | 0 | `87E738A9A011AD60AA5DAB8F78D7657C30D41BF15BDC54DC84EC5224D0F040FE` |
| Game final | Succeeded | 5 / 30.02s | 0 | `FF5C54870DA4D9D3AC961CEEBFF45972C704CD11E13186843691F37DAA3B6751` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1111552` bytes，SHA-256 `D1F106CC7BD6EA370FAAC7E750C9F590592CF8C6551CD82774229FD71BB52245`；
- `demo_map.exe`：`353496064` bytes，SHA-256 `522D006FDAE99D9186E26D13C01CD9B2715B2A7867F0B0B9EDE9A384B5F3F6EF`。

## 9. 真实异常

本阶段没有源码、目标测试或构建失败。Automation 启动仍保留 UE 5.8 既有的非目标平台 SDK metadata 与测试发现前诊断；Win64 SDK 有效，所有选中目标随后逐项 Success。未发生 C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

首次文档级 exact-stage gate 调用误用 `*-final.log`，把 Editor/Game 两份 build log 混入 Automation evidence，parser 因缺少 RunTests/success/queue-empty 而按设计拒绝。随后改为显式八份 Automation 日志，未修改门禁规则，得到 `PASS Changed=5 Rules=2 Required=7 Logs=8`。这是验证调用参数错误，不是产品源码失败。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值开发、代码审查、无头 Automation、静态扫描、changed-file gate、Editor/Game Development 构建与 Git 证据。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P9.6 建立产品拥有的 SpiritEnergy adapter/host：从正式玩家资源 authority 捕获 typed snapshot，把内容定义的 cost 交给协调器，并由明确输入命令触发 Begin/Commit/Abort/Close/Reset。产品 owner、恢复与持久化规则未确定前，不应把临时 float 或第二套资源状态接入运行时。
