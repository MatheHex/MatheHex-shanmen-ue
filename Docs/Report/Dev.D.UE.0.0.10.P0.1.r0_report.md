# Dev.D.UE.0.0.10.P0.1.r0 开发报告

## 结论

`PASS`。P0 复审指出的 A--G 七项契约缺口已在 `ShanmenCombatCore` 内完成结构性修正，且没有接入或改变 0.0.9B 的产品运行路径。

固定字段防御快照已改为有序、带身份和标签条件的通用防御层；Impact 请求现在显式携带公式输出、伤害标签与目标生命快照；Resolver 支持致死拦截并保证所有接受结果满足 `RawDamage = PreventedDamage + FinalDamage`。任意伪造的 ImpactId、重复防御层身份和缺少精确来源的消耗型防御层均失败关闭。

同时新增 0.0.10 物品唯一权威 ADR：P1 不再以“只对接 Code B”为方向，而是建立 `ShanmenItems` 单一权威；Code A 与 Code B 都只作为迁移来源，禁止在线双写。

## 复审缺口闭环

| 缺口 | 修正结果 |
|---|---|
| A：固定字段与烧死顺序 | `FShanmenDefenseSnapshot` 改为 `TArray<FShanmenDefenseLayer>`；按 `Order`、再按稳定 `LayerId` 确定性排序。阵法、飞剑、护盾、灵器等来源可添加层而无需修改 Resolver 分支结构。 |
| B：护心镜无法表达 | 新增 `FShanmenTargetVitalitySnapshot` 与 `PreventLethal` 操作；层可指定生命下限，并在 receipt 中保留来源实例与提交要求。 |
| C：标签惰性 | 新增原生伤害／来源／目标／防御标签；防御层支持 required/blocked Damage、Source、Target 标签。父级 `Damage.Physical` 已验证可匹配 `Damage.Physical.Slash`。 |
| D：闪避不守恒 | 结果统一输出 `PreventedDamage` 与逐层 `TriggeredLayers`；闪避、精准格挡、普通减免和致死拦截均通过守恒断言。 |
| E：幂等门可绕过 | `FShanmenImpactRequest::IsValid` 重新派生并比对 canonical ImpactId；Ledger 只接收完整合法 Request，不再接收裸 GUID。 |
| F：快照可被蓝图改写 | 新增可写 `FShanmenCombatActionCapture`；成功捕获后生成私有字段、`BlueprintReadOnly + EditConst` 的 Snapshot。反射测试覆盖全部 8 个字段。 |
| G：BasePower 归属不清 | CombatCore 移除 BasePower；未来攻击公式层独占数值计算，并输出带 `FormulaId`、`RawDamage`、`DamageTags` 的 `FShanmenDamagePacket`。DefenseResolver 只消费该包。 |

## 关键契约

### 有序防御层

每层包含：

- 稳定 `LayerId`、规则 `RuleId` 和可选 `SourceInstanceId`；
- `PreventAll / ReduceFraction / AbsorbPoints / PreventLethal` 操作；
- 显式 `Order` 与 Gameplay Tag 条件；
- 是否在触发后要求外部资源提交。

相同 Order 使用 LayerId 排序，消除容器输入顺序对回放结果的影响。要求提交的层若没有精确 SourceInstanceId，整个请求失败关闭。

### 致死拦截与物品提交边界

护心镜示例在目标 30 生命、100 原始伤害、生命下限 1 时输出：

- FinalDamage：29
- PreventedDamage：71
- SourceInstanceId：保留
- bRequiresCommit：true

CombatCore 只给出触发事实，不修改耐久、次数或库存。后续 `ShanmenItems` 根据该 receipt 在 Commit Point 原子提交。

### 唯一物品权威

ADR 已确认：

- 0.0.10 的唯一可变物品权威是新模块 `ShanmenItems`。
- Code A `Udemo_mapItemSubsystem` 与 Code B 持久图都是迁移来源，不是在线并行权威。
- 活动中的 0.0.9B Run 不做中途热迁移。
- 来源冲突失败关闭，禁止静默合并。
- 所有消费遵循 `Validate -> Reserve -> Commit Point -> Commit/Cancel -> Receipt`。

## 自动化覆盖

测试命名空间：`Shanmen.0_0_10.CombatCore`

| 测试 | 结果 |
|---|---|
| `ActionSnapshotReadOnly` | PASS；8/8 字段均 BlueprintReadOnly 且 EditConst |
| `ActiveDefensePrecedence` | PASS；闪避／精准格挡顺序与守恒 |
| `DamageTagFiltering` | PASS；物理父标签匹配 Slash，Spirit 层不误触发 |
| `DefenseOrder` | PASS；乱序输入规范化为格挡／护盾／护甲，100 -> 52 |
| `DeterministicIdentity` | PASS；canonical ID 可重放，HitOrdinal 参与身份 |
| `ImpactIdentityIntegrity` | PASS；伪造 ImpactId 被 Request、Ledger、Resolver 共同拒绝 |
| `ImpactLedger` | PASS；首次接受、重复拒绝、Run 边界清空 |
| `InvalidRequestFailsClosed` | PASS；NaN、缺提交来源、重复 LayerId 均失败关闭 |
| `LethalInterception` | PASS；护心镜生命下限、审计来源与提交标志 |

最终命令行测试原生退出码 `0`，`9/9 Success`、`0 Fail`，队列正常清空。日志 SHA-256：`4A4E7D1B6D7CE3E5238DEE0017E1ACAD20B0DB1A3BD9FC3203A03B3102E62B96`。

UE 5.8 在目标测试开始前仍输出 13 条内置 `LogAutomationTest: Error: Condition failed` 启动诊断；上一份 P0.0 日志中数量相同。它们不属于 `Shanmen.0_0_10.CombatCore`，目标套件随后逐项 9/9 成功，原始日志予以保留。

## 构建与静态检查

- `git diff --check`：退出码 `0`。
- `demo_mapEditor Win64 Development -MaxParallelActions=1 -NoUBA`：最终退出码 `0`，5 actions，`Result: Succeeded`。
- `demo_map Win64 Development -MaxParallelActions=1 -NoUBA`：退出码 `0`，6 actions，`Result: Succeeded`。
- 新模块边界扫描：`demo_map / UWorld / AActor / ApplyDamage / UGameplayStatics / FMath::Rand / FRandomStream` 匹配数 `0`。
- CombatCore 中 `BasePower` 引用数 `0`。

## 修改范围

- `Source/ShanmenCombatCore/Public/ShanmenCombatTypes.h`
- `Source/ShanmenCombatCore/Public/ShanmenCombatResolver.h`
- `Source/ShanmenCombatCore/Public/ShanmenCombatTags.h`
- `Source/ShanmenCombatCore/Private/ShanmenCombatResolver.cpp`
- `Source/ShanmenCombatCore/Private/ShanmenCombatTags.cpp`
- `Source/ShanmenCombatCore/Private/Tests/ShanmenCombatCoreTests.cpp`
- `Docs/Architecture/Dev.D.UE.0.0.10_ItemAuthority_ADR.md`
- 本 Report 与同名 Log

工作区原有未跟踪 Prompt、Report、自动化文档和用户文件未纳入本轮提交。

## 已知前置债务

`demo_mapProfileRepository.cpp::IsExactLegacySourceFor` 仍限制 `Legacy.SchemaVersion <= 5`，因此 Schema 6 -> 7 的 N-1 迁移缺陷尚未修复。本轮只记录并冻结其前置关系：首次 0.0.10 持久数据导入和端到端运行验证之前必须修复并补回归测试；不得用放宽 provenance 校验绕过。

## P/F 边界

本轮仅进行契约开发、静态扫描、命令行 Automation、Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook、Package 或大规模回归。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p0-combat-contracts/Docs/Report/Dev.D.UE.0.0.10.P0.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p0-combat-contracts/Docs/Log/Dev.D.UE.0.0.10.P0.1.r0_log.md>
- 物品权威 ADR：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p0-combat-contracts/Docs/Architecture/Dev.D.UE.0.0.10_ItemAuthority_ADR.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p0-combat-contracts>

`READY_FOR_0_0_10_P1_CONTRACTS`
