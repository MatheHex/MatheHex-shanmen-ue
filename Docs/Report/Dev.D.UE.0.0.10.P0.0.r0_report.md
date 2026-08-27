# Dev.D.UE.0.0.10.P0.0.r0 开发报告

## 结论

`PASS`。0.0.10 已从 0.0.9B 单模块运行时中建立第一组独立生产边界：`ShanmenCore` 与 `ShanmenCombatCore`。本轮交付确定性操作身份、不可变动作快照、统一候选命中／Impact 契约、按固定顺序执行的纯函数防御结算，以及每 Run 重置的 Impact 幂等门。

新代码没有接入旧 `demo_map` 战斗入口，也没有修改玩家、敌人、技能、物品、Profile、Run、地图或 UI 的现有行为。该限制是刻意的：P0 先冻结无 Actor、无 World、无库存和无随机依赖的战斗语义，避免在迁移期间形成第二套运行时权威。

## 采用的 0.0.10 架构基线

本轮参考同项目“代码结构”任务的最新结论执行：

- 维持一个 Git 仓库，逐步拆分 UE Runtime 模块。
- 0.0.10 目标框架优先于旧接口；0.0.9B 仅作为迁移来源和行为参考。
- UE5／Chaos 只负责空间查询，候选接触不能直接造成伤害。
- 所有命中使用稳定 `ActivationId`／`ImpactId`，重复回调必须幂等拒绝。
- 战斗数学保持纯函数；Actor、GAS、物品和 UI 只能在更高层编排。
- 后续资源消耗必须采用 `Validate → Reserve → Commit Point → Commit/Cancel → Receipt`，不得让技能直接修改库存。

## 实现内容

### ShanmenCore

- 新增独立 Runtime 模块及日志类别。
- `FShanmenContentStamp`：冻结动作使用的内容版本与 digest。
- `FShanmenOperationContext`：跨模块命令的 Run、Owner、Request 与内容身份信封。
- `FShanmenDeterministicId`：使用带长度前缀的规范字符串和 SHA-1 前 128 位生成稳定 GUID；相同规范输入可重放为相同身份，空命名空间失败关闭。

### ShanmenCombatCore

- 定义动作阶段：`Idle / Startup / Active / Recovery / Cancelled / Interrupted`。
- 定义六类统一检测器：武器轨迹、形状、投射物、受控物、持续区域和规则目标。
- `FShanmenCombatActionSnapshot`：在激活点冻结 Run、Owner、来源实体／物品、动作定义、内容版本、基础强度和 Gameplay Tags。
- `FShanmenHitCandidate`：只表达几何候选，不提前决定敌我、自目标或最终命中合法性。
- `FShanmenImpactRequest/Result`：保存完整输入与每层减免结果，便于 receipt、回放和审计。
- `FShanmenCombatIdFactory`：稳定生成 Activation 与 Impact 身份。
- `FShanmenImpactLedger`：同一个 ImpactId 只接受一次，Run 边界显式清空。
- `FShanmenDefenseResolver`：固定执行 `闪避 → 精准格挡 → 普通格挡 → 护盾 → 护甲 → 最终生命伤害`，不访问 World、Actor、库存或 RNG。

## 自动化覆盖

测试命名空间：`Shanmen.0_0_10.CombatCore`

| 测试 | 结果 |
|---|---|
| `DeterministicIdentity` | PASS；相同规范输入稳定重放，HitOrdinal 改变身份，自目标候选保留给策略层裁决 |
| `ImpactLedger` | PASS；首次接受、重复拒绝、非法 GUID 拒绝、Run 清理 |
| `DefenseOrder` | PASS；100 伤害按 25% 格挡、10 护盾、20% 护甲后得到 52 最终伤害 |
| `ActiveDefensePrecedence` | PASS；闪避优先于精准格挡，精准格挡优先于普通格挡 |
| `InvalidRequestFailsClosed` | PASS；NaN 输入不接受且不能产生伤害 |

最终命令行测试找到并完成 `5/5`，原生退出码 `0`。日志：`Saved/Logs/Dev.D.UE.0.0.10.P0.0.r0_automation.log`；SHA-256：`3004CA2BB8E11A4670F0B6F1BAFCCF0CB28B3D98A7ECA29089B0AF21D3DA2C20`。

## 构建与静态检查

- `git diff --check`：退出码 `0`。
- `demo_mapEditor Win64 Development -MaxParallelActions=1 -NoUBA`：最终退出码 `0`，`Result: Succeeded`。
- `demo_map Win64 Development -MaxParallelActions=1 -NoUBA`：最终退出码 `0`，`Result: Succeeded`。
- 边界扫描：新模块中没有 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage`、`UGameplayStatics` 或随机数调用。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、Smoke、Cook 或 Package。

## 修改范围

- `demo_map.uproject`
- `Source/demo_map.Target.cs`
- `Source/demo_mapEditor.Target.cs`
- `Source/ShanmenCore/**`
- `Source/ShanmenCombatCore/**`
- 本 Report 与同名开发 Log

工作区原有未跟踪 Prompt、Report、自动化文档及其它用户文件均未加入提交。

## 下一阶段计划

1. `P1`：建立 `ShanmenItems` 的通用资源预留、提交、取消和 receipt 契约，并以单向 Adapter 对接现有 Code B；不双写旧物品权威。
2. `P2`：建立 `ShanmenWorldGameplay` 候选命中适配层，将 Sweep／Overlap／Projectile 统一输出 `FShanmenHitCandidate`。
3. `P3`：建立 GAS 编排层与动作阶段运行时，完成一式基础剑击的首个纵切。
4. `P4`：接入主动闪避／格挡、最终生命 Adapter、真实装备快照和完整幂等 Impact 路径。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p0-combat-contracts/Docs/Report/Dev.D.UE.0.0.10.P0.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p0-combat-contracts/Docs/Log/Dev.D.UE.0.0.10.P0.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p0-combat-contracts>

`READY_FOR_0_0_10_P1`
