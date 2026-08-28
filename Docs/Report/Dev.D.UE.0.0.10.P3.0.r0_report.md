# Dev.D.UE.0.0.10.P3.0.r0 开发报告

## 结论

`PASS`。P3 已建立第一段独立 GAS 编排边界：新 Runtime 模块 `ShanmenCombatRuntime` 启用 GameplayAbilities 插件，并以抽象 `UShanmenCombatGameplayAbility` 承载每次 activation 的隔离状态；真正的动作阶段合法性由无 World、无 Actor 的 `FShanmenActionOrchestrator` 统一裁决。

动作现在只能按 `Idle -> Startup -> Active -> Recovery -> Idle/Completed` 推进。`Startup -> Active` 是唯一提交点，只有 Active 阶段允许发射命中候选；取消只允许发生在提交点之前，提交后的终止必须记录为 Interrupt。每次成功转换都会生成带 ActivationId、单调 Sequence、前后阶段、提交事实和终止原因的不可变 receipt，重复或过期阶段回调失败关闭。

本轮没有接入旧 `demo_map` 战斗、没有创建具体剑击 Ability、没有执行 World query、没有结算／应用伤害、没有提交 `ShanmenItems` 事务，也没有启动产品。P3.0 只冻结 GAS 与纯函数战斗内核之间的生命周期协议，为 P3.1 一式基础剑击纵切提供稳定入口。

## 实现内容

### 独立 CombatRuntime 模块

- 新增 `ShanmenCombatRuntime` Runtime 模块并加入 Game／Editor Target 与 uproject；
- 显式启用 UE 5.8 `GameplayAbilities` 插件；
- 模块只依赖 Engine／GameplayAbilities／GameplayTags／GameplayTasks 与 `ShanmenCombatCore`；
- 不依赖 `ShanmenWorldGameplay`、`ShanmenItems` 或旧 `demo_map`，避免编排层取得世界、库存或旧产品权威。

### 确定性动作阶段权威

新增 `FShanmenActionOrchestrator`：

1. `TryStart` 只接受合法的 frozen `FShanmenCombatActionSnapshot`，输出 `Idle -> Startup` 的 sequence 0 receipt；
2. `TryAdvance(ExpectedPhase)` 要求调用方声明预期当前阶段，过期 timer／notify 不能推进新状态；
3. `Startup -> Active` 唯一设置 commit fact，且只在该次 receipt 标记 `CrossedCommitPointNow`；
4. `CanEmitCandidates` 只在 Active 返回 true；Startup、Recovery 与所有 terminal 状态均返回 false；
5. `TryCancel` 只允许 Startup；Active／Recovery 只能 `TryInterrupt`，receipt 会保留资源已经提交的事实；
6. 完成、取消或中断后均为 terminal，拒绝后续状态转换；
7. 相同 Action 与相同转换序列可重放完全相同的 receipts。

### GAS 薄适配基类

新增抽象 `UShanmenCombatGameplayAbility`：

- 固定 `InstancedPerExecution`，每次 activation 不共享 runtime 状态；
- 通过 `Shanmen.Ability.Combat.Action` 原生 tag 进行 GAS 分类；
- 只暴露 Start／Advance／Cancel／Interrupt 的受保护入口，全部委托给 orchestrator；
- 每次合法转换向派生 Ability／Blueprint 发出只读 receipt；
- GAS `EndAbility` 若发生在未终结 runtime，会失败关闭为 Cancel（Startup 且取消）或 Interrupt（已提交／异常正常结束），不会静默丢失生命周期；
- 不在基类中访问 Avatar、ASC 属性、World、动画、库存、目标或伤害公式。

## 自动化覆盖

新增命名空间 `Shanmen.0_0_10.CombatRuntime`：

| 测试 | 结果 |
|---|---|
| `ActionLifecycle` | PASS；完整阶段链、唯一提交点、Active-only emission、过期阶段拒绝、terminal 闭合与 output alias 安全复用 |
| `CancellationBoundary` | PASS；Startup 可取消且未提交，Active 取消拒绝并只能记录为已提交中断 |
| `DeterministicReplay` | PASS；相同 Action／转换序列产生字段完全一致的 receipts |
| `GameplayAbilityBoundary` | PASS；基类不可直接授予、每 activation 独立实例、GAS 原生分类 tag 有效 |

最终结果：

- `Shanmen.0_0_10.CombatRuntime`：`4/4 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`80/80 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P3.0.r0_combatruntime_automation.log`：`CFDBEC869C0A04B92B128A98983B7A3C8C88DE5F5B25929CCD1DD77BE0384C5C`；
- `Saved/Logs/Dev.D.UE.0.0.10.P3.0.r0_full_automation.log`：`C289A7224CC6AF98D2E43EC1644C62DF1FEEFF1B4046CB65FDE8D6FA8EF5FFEE`。

两份最终日志各保留 UE 5.8 测试发现前既有 13 条 `LogAutomationTest: Error: Condition failed` 启动诊断；目标测试无 fail、无 handled ensure。平台检查仍只认定 Win64 SDK 为 VALID，并报告非目标 LinuxArm64／VisionOS 缺 `MainVersion` metadata。

## 构建与静态检查

- `git diff --check`：退出码 `0`；
- Editor 首次：13/13 actions；最终增量：7/7 actions；`Result: Succeeded`，原生退出码 `0`；
- Game 首次：8/8 actions；最终增量：6/6 actions；`Result: Succeeded`，原生退出码 `0`；
- 构建统一使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`；
- 排除 Tests 后，CombatRuntime 产品源中 `demo_map`、World／Actor、damage API、`ShanmenItems`、`ShanmenWorldGameplay`、RNG 与 world-query API 匹配均为 `0`。

## 修改范围

- `demo_map.uproject`
- `Source/demo_map.Target.cs`
- `Source/demo_mapEditor.Target.cs`
- `Source/ShanmenCombatRuntime/**`
- 本 Report 与同名 Development Log

工作区长期未跟踪的 Prompt、旧 Report、自动化文档和用户资料均未修改、删除或加入提交。

## P/F 边界

只执行源码开发、静态审查、headless Automation 与必要 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

进入 P3.1：实现 `Combat.Action.Sword.Basic01` 的具体 Ability／动作定义，以显式 Startup／Active／Recovery 信号驱动 P2.1 detector emission，并加入目标策略和公式输入适配；仍由 CombatCore 独占 Impact 数学，生命与物品只在 receipt 指定的 commit point 通过各自 Adapter 提交。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p3-0-action-orchestration/Docs/Report/Dev.D.UE.0.0.10.P3.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p3-0-action-orchestration/Docs/Log/Dev.D.UE.0.0.10.P3.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p3-0-action-orchestration>

`READY_FOR_0_0_10_P3_1`
