# Dev.D.UE.0.0.10.P3.1.r0 开发报告

## 结论

`PASS`。P3.1 已完成第一式基础剑击 `Combat.Action.Sword.Basic01` 的无头纵切：P3.0 动作阶段权威、P2.1 Detector Emission、显式目标策略、冻结公式输入、CombatCore Impact 契约、幂等 Ledger 与有序防御结算已经连成一条可确定性重放的执行链。

本轮没有把 Actor、ASC 属性读取、动画通知、世界查询或生命写入塞进纯内核。具体 GAS Ability 仍保持抽象，等待 P3.2 的产品侧 Source Snapshot／动画任务／目标与生命 Adapter。最终伤害只形成可审计 receipt，不直接修改角色生命；库存与资源事务也未被触碰。

## 实现内容

### 冻结的一式基础剑定义

新增可编辑的 `FShanmenBasicSwordDefinitionCapture` 与只读的 `FShanmenBasicSwordDefinition`：

- 动作身份固定为 `Combat.Action.Sword.Basic01`；
- Detector、Formula、基础伤害、攻击力系数、伤害标签与目标标签均来自内容捕获；
- 正式定义要求 `Damage.Physical.Slash`、`Target.Living` 与拒绝自目标策略；
- 捕获成功后字段只读，后续装备或内容变化不能回写本次 activation。

新增 `FShanmenBasicSwordOffenseSnapshot`，显式区分“尚未捕获”与合法的零攻击力。伤害公式归属本纵切的公式适配层：

`RawDamage = BaseDamage + CapturedAttackPower × AttackPowerCoefficient`

计算先使用 double 检查有限性与 float 上界，再生成 `FShanmenDamagePacket`。测试内容值为 `20 + 60 × 0.5 = 50`，运行时没有烧死这组平衡数值。

### 确定性纵切执行器

新增 `FShanmenBasicSwordExecution`：

1. 绑定同一份 frozen Action、Sword Definition 与 Offense Snapshot；
2. 只有 P3.0 ActionRuntime 的 Active 阶段可以开启、接收或正常关闭 P2.1 WeaponTrajectory emission；
3. Candidate 必须匹配 Activation、Source、Detector、DetectorKind，并通过非自身与目标标签策略；
4. 策略或输入失败发生在 emission dedupe 之前，不会错误消费合法候选；
5. 完整 `FShanmenImpactRequest` 通过规范 ImpactId 校验后，依次进入 emission 去重与 activation 内 Ledger；
6. CombatCore 纯函数 Resolver 输出守恒的 `FShanmenBasicSwordImpactReceipt`；
7. 相同冻结输入与 emission 序列可重放相同 ImpactId 与伤害结果；
8. Ability 在判定窗口仍开启时终止，会先关闭 emission，再由 P3.0 记录 Cancel／Interrupt，避免把 active 判定状态遗留给 P3.2 适配层。

### GAS 边界

新增抽象 `UShanmenBasicSwordGameplayAbility`：

- 继承 `UShanmenCombatGameplayAbility` 的每次执行实例与阶段权威；
- 增加原生 tag `Shanmen.Ability.Combat.Action.Sword.Basic01`，同时保留通用 Combat Action tag；
- 只提供准备、开启判定、提交候选和结束判定的受保护入口；
- 准备时先安装 SwordExecution，再启动通用 ActionRuntime，保证 Blueprint transition 回调看到完整纵切状态；
- 不读取 Avatar、ASC、World、Actor、动画、库存或旧 `demo_map` 战斗状态。

## 自动化覆盖

新增 4 条 `Shanmen.0_0_10.CombatRuntime.BasicSword` 测试：

| 测试 | 结果 |
|---|---|
| `VerticalSlice` | PASS；Startup 禁止判定、Active 发射、50 点 Slash Impact、重复回调去重、Recovery／Completed 闭合 |
| `TargetPolicyAndOrdinals` | PASS；拒绝自身与缺失 Living tag，策略拒绝不消费候选，多目标／多 emission 生成不同 ImpactId，终止清理关闭窗口 |
| `DefenseReplay` | PASS；20% Guard 将 50 结算为 10 prevented／40 final，相同输入重放相同 ImpactId 与 receipt |
| `AbilityBoundary` | PASS；错误动作定义失败关闭、未捕获 Offense 无效、具体产品 Adapter 尚未接入、GAS tags 正确 |

最终结果：

- `Shanmen.0_0_10.CombatRuntime`：`8/8 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`84/84 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P3.1.r0_combatruntime_automation.log`：`C6218DB5355734C1A490FE75CDD448D1460C2D11812173553287E878E8B168BC`；
- `Saved/Logs/Dev.D.UE.0.0.10.P3.1.r0_full_automation.log`：`24142DF9AAB82B17DF2C3B75E73448577D2E1BEBB39F64540100941AEB1F79E3`。

两份最终日志各保留 UE 5.8 在测试发现前打印的既有 13 条 `LogAutomationTest: Error: Condition failed` 启动诊断；目标测试均成功、无 handled ensure。平台检查认定 Win64 SDK 为 VALID；非目标 LinuxArm64／VisionOS 仍报告缺少 `MainVersion` metadata。

## 构建与静态检查

- `git diff --check`：退出码 `0`；
- Editor：首次 13/13 actions、终止清理后增量 7/7 actions，均 `Result: Succeeded`、原生退出码 `0`；
- Game：11/11 actions，`Result: Succeeded`、原生退出码 `0`；
- 构建统一使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`；
- 排除 Tests 后，本轮产品源对 `demo_map`、World／Actor、damage API、`ShanmenItems`、RNG 与 LineTrace／SweepMulti／OverlapMulti 的匹配均为 `0`；
- `ShanmenWorldGameplay` 是新增的显式公共依赖，只用于消费 P2.1 的纯 `FShanmenDetectorEmissionSession` 与候选类型；CombatRuntime 本身不执行世界查询。

## 修改范围

- `Source/ShanmenCombatRuntime/ShanmenCombatRuntime.Build.cs`
- `Source/ShanmenCombatRuntime/Public|Private/ShanmenCombatRuntimeTags.*`
- `Source/ShanmenCombatRuntime/Public|Private/ShanmenBasicSwordExecution.*`
- `Source/ShanmenCombatRuntime/Public|Private/ShanmenBasicSwordGameplayAbility.*`
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenBasicSwordExecutionTests.cpp`
- 本 Report 与同名 Development Log

工作区长期未跟踪的 Prompt、旧 Report、自动化文档和用户资料均未修改、删除或加入提交。

## P/F 边界

只执行源码开发、静态审查、headless Automation 与必要 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

进入 P3.2：由产品侧 Adapter 从 ASC／装备权威捕获攻击快照，以 Montage／AbilityTask 的显式信号驱动 Startup／Active／Recovery，并把 WorldGameplay 候选、目标标签／生命／防御快照送入本纵切；生命扣减仍应消费 receipt 后单独提交，不能让 Ability 绕过 CombatCore。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p3-1-basic-sword-vertical-slice/Docs/Report/Dev.D.UE.0.0.10.P3.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p3-1-basic-sword-vertical-slice/Docs/Log/Dev.D.UE.0.0.10.P3.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p3-1-basic-sword-vertical-slice>

`READY_FOR_0_0_10_P3_2`
