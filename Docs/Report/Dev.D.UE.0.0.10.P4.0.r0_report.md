# Dev.D.UE.0.0.10.P4.0.r0 开发报告

## 结论

`PASS`。P4.0 已冻结并实现版本化、幂等的最终生命提交协议：CombatCore 解析出的规范 Impact 先转换为不可变 `FShanmenVitalityCommitCommand`，再由单目标 `FShanmenVitalityAuthority` 以稳定 `TargetEntityId`、精确快照和单调 `AuthorityRevision` 执行一次性提交，并产出不可变 mutation receipt。

P3.1 的基础剑击现已在无头链路中贯通到最终生命变化：测试公式形成 50 点原始伤害，20% Guard 阻止 10 点，剩余 40 点只提交一次，生命由 100 变为 60；重复投递返回原 receipt，不再次扣血或推进 revision。

本轮没有直接接入旧 `demo_map` 角色／属性组件，也没有让新旧生命路径同时写入产品状态。新增 authority 当前是独立模块内的契约与参考实现；P4.1 产品 Adapter 必须选择单一写权威并迁移，禁止在调用旧伤害路径后再提交本协议。

## 实现内容

### 版本化生命快照

`FShanmenTargetVitalitySnapshot` 新增 `AuthorityRevision`：

- 与当前／最大生命在同一时刻捕获；
- 只接受非负单调 revision；
- commit 同时比对 revision、当前生命和最大生命；
- 任一字段不再等于 authority 当前值时，以 `StaleSnapshot` 失败关闭。

快照浮点值采用位级一致校验，而不是近似相等。由于值来自同一 authority 的直接复制，不需要容差；该约束防止细微漂移或伪造结果越过确定性提交门。

### 不可变提交命令与解析身份

新增 `FShanmenVitalityCommitCommand`，只能通过 `TryCreate(Request, Result)` 构造。工厂会：

1. 验证 Request、ImpactId、结果接受状态和伤害守恒；
2. 重新运行 `FShanmenDefenseResolver::Resolve(Request)`；
3. 位级比较原始／阻止／最终伤害及每个触发防御层；
4. 使用规范字段派生 `ResolutionId`，覆盖 Impact、activation、目标、authority revision、生命快照、内容版本／摘要、公式、伤害标签、结果和有序防御层；
5. 将身份、期望快照、伤害和防御结果冻结为只读字段。

这使“算术仍守恒但不是规范 Resolver 输出”的伪造结果无法进入最终生命写入。

### Exact-once 生命权威

新增 `FShanmenVitalityAuthority`：

- 每个实例只服务一个稳定 `TargetEntityId`；
- 首次规范命令在快照匹配时提交，revision 只推进一次；
- 同一 `ImpactId + ResolutionId` 再次到达时返回 `AlreadyCommitted` 和原始 receipt；
- 同一 ImpactId 携带不同 ResolutionId 时返回 `ImpactConflict`；
- 目标错配、未初始化、过期快照、无效命令和 revision 上限均返回结构化拒绝，且不改变生命或消费 Impact；
- 过量伤害保留 requested damage，但 applied damage 钳制为提交前可用生命；
- 成功 receipt 证明 revision 前后值、生命前后值、请求伤害与实际伤害。

### 产品边界

本轮没有把 authority 隐式绑定到 Actor、ASC、World 或旧组件。这样可先冻结跨系统提交语义，再在 P4.1 明确迁移单一生命真值。产品 Adapter 只能采用以下一种模式：

- 新 authority 成为唯一写权威，旧组件仅投影其 receipt；或
- 旧组件继续唯一写入，但必须原子实现同一 revision／idempotency 协议，不再实例化第二份可写生命状态。

禁止同时调用旧 `ApplyDamage`／独立扣血逻辑和本 authority。

## 自动化覆盖

新增 5 条 `Shanmen.0_0_10.CombatRuntime.VitalityAuthority` 测试：

| 测试 | 结果 |
|---|---|
| `CommitLifecycle` | PASS；快照 revision、30 点提交、receipt、状态发布及 30→20 的 overkill 钳制正确 |
| `IdempotencyAndConflict` | PASS；完全重复返回原 receipt，不二次扣血；同 ImpactId 的分歧解析被拒绝 |
| `StaleSnapshotIsolation` | PASS；旧 revision 不消费 Impact；重新捕获并重算后可用同一 ImpactId 合法提交 |
| `CanonicalResultGate` | PASS；伪造 Resolver 输出、未初始化 authority、目标错配与 revision 溢出均失败关闭 |
| `BasicSwordIntegration` | PASS；P3.1 剑击经 Guard 后将 40 点最终伤害 exact-once 提交到生命权威 |

最终结果：

- `Shanmen.0_0_10.CombatRuntime`：`13/13 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`89/89 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.0.r0_combatruntime_automation.log`：`AF3E48D3C395D6952F5ECC98CEB52BE39CC82F8FFDC6B3295ADDDB390119727E`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.0.r0_full_automation.log`：`DB8C9842073448D49F865931AD16E44FACC952D9EF4B6F67A780F31B728A34E7`。

两份最终日志各保留 UE 5.8 在测试发现前打印的既有 13 条 `LogAutomationTest: Error: Condition failed` 启动诊断；目标测试全部成功、无 handled ensure。平台检查认定 Win64 SDK 为 VALID；非目标 LinuxArm64／VisionOS 仍报告缺少 `MainVersion` metadata。

## 构建与静态检查

- `git diff --check`：退出码 `0`；
- Editor：首次 28/28 actions、最终增量 5/5 actions，均 `Result: Succeeded`、原生退出码 `0`；
- Game：22/22 actions，`Result: Succeeded`、原生退出码 `0`；
- 构建统一使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`；
- 排除 Tests 后，本轮产品源对 `demo_map`、World／Actor、旧 damage API、`ShanmenItems`、RNG 与 LineTrace／SweepMulti／OverlapMulti 的匹配均为 `0`；
- Runtime 仅增加对 `ShanmenCore` 的私有依赖，用于规范确定性 ID 派生。

## 修改范围

- `Source/ShanmenCombatCore/Public/ShanmenCombatTypes.h`
- `Source/ShanmenCombatCore/Private/ShanmenCombatResolver.cpp`
- `Source/ShanmenCombatRuntime/ShanmenCombatRuntime.Build.cs`
- `Source/ShanmenCombatRuntime/Public/ShanmenVitalityAuthority.h`
- `Source/ShanmenCombatRuntime/Private/ShanmenVitalityAuthority.cpp`
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenVitalityAuthorityTests.cpp`
- 本 Report 与同名 Development Log

工作区长期未跟踪的 Prompt、旧 Report、自动化文档和用户资料均未修改、删除或加入提交。

## P/F 边界

只执行源码开发、静态审查、headless Automation 与必要 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

进入 P4.1：审计产品当前真实生命写入点，以单向 Adapter 迁移一个实际目标到本协议；先证明旧路径不会双写，再接入主动闪避／格挡输入和真实装备防御快照。任何产品迁移都应保留 `ImpactId + ResolutionId + AuthorityRevision` 的冲突与重放语义。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-0-vitality-authority/Docs/Report/Dev.D.UE.0.0.10.P4.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-0-vitality-authority/Docs/Log/Dev.D.UE.0.0.10.P4.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-0-vitality-authority>

`READY_FOR_0_0_10_P4_1`
