# Dev.D.UE.0.0.10.P4.12.r0 开发报告

## 结论

`PASS`。P4.12 已把玩家 GroundCircle 与 SelfSector 两种真实范围技能接入 0.0.10 canonical combat。`Udemo_mapSkillComponent` 继续拥有瞄准、圆形／扇形几何、垂直容差、阵营过滤、冷却和命中特效；合法 overlap 进入 `CombatRunCoordinator` 后，统一完成 Run-local action identity、稳定目标排序、纯函数防御结算以及 M01 vitality ledger 的 exactly-once 生命写入。

M01 产品 gate 一旦声明所有权，canonical 执行失败即关闭本次施法，不回落旧 `ApplyDamage`，也不消费技能冷却；非 M01 compatibility 路径保持原行为。本轮没有迁移 Straight Projectile，它的发射／飞行／命中身份属于下一独立 family，未被错误压入 overlap shape action。

## 功能性

### 1. 两个冻结的玩家 Shape family

- GroundCircle：`Combat.Action.Player.Skill.GroundCircle`、`Detector.Player.Skill.GroundCircle`、`Combat.Formula.Player.Skill.GroundCircle.r1`；
- SelfSector：`Combat.Action.Player.Skill.SelfSector`、`Detector.Player.Skill.SelfSector`、`Combat.Formula.Player.Skill.SelfSector.r1`；
- 两者 content version 均为 `0.0.10.P4.12`；
- digest 分别为 `Shanmen.Player.Skill.GroundCircle.Shape.r1` 与 `Shanmen.Player.Skill.SelfSector.Shape.r1`；
- source／damage／target tags 分别冻结为 `Player`、`Physical`、`Living`；
- detector kind 为 `Shape`，每个目标的 hit ordinal 固定为 `0`。

### 2. 独立、确定性的 Run-local identity

- GroundCircle 与 SelfSector 分别拥有独立的 next activation sequence，互不消耗对方编号；
- action 构造与 runtime startup 成功后才消费 sequence；zero／`MAX_uint64`、非法 family、非有限伤害或无效 contact origin 均在消费前失败关闭；
- Run 结束、reset 和新 Run 初始化会把两个 sequence 恢复为 `1`；
- ActivationId 只由 RunId、稳定玩家 EntityId、冻结 action definition 与 family sequence 派生；
- ImpactId 只由 RunId、ActivationId、detector、稳定目标 EntityId 与 ordinal 派生，不依赖对象地址、帧号、时间或随机数。

### 3. 世界授权与唯一生命写入

- GroundCircle 保留既有 capsule overlap、半径、垂直容差与目标过滤；
- SelfSector 保留既有 capsule broad phase、sector XY 几何、角度、半径、垂直容差与目标过滤；
- `SkillComponent` 只提交已经通过世界规则的 overlaps；coordinator 不查询 World；
- World adapter 将 Actor／Component alias 解析为稳定 EntityId，忽略未注册、自目标与重复目标；
- 候选按 EntityId 稳定排序，输入 overlap 顺序不会改变 receipt 顺序；
- 每个合法目标捕获 vitality snapshot，构造 physical damage packet，经纯函数 resolver 结算后只通过 M01 vitality ledger 提交；
- exact receipt replay 返回 `AlreadyCommitted`，不重复扣血、revision、ledger 或 broadcast。

### 4. M01 fail-closed 与 compatibility

- coordinator 未就绪、非法 family／damage、sequence 耗尽、action／hit context 构造失败、vitality snapshot 失败、impact resolve 失败、delivery reject 或 runtime completion 失败均返回结构化错误；
- M01 canonical 失败时 `SkillComponent` 返回失败，不设置本次 cooldown，且绝不调用 legacy damage；
- 无合法注册目标仍可作为一次有效空命中动作完成并消费 sequence／cooldown；
- `demo_mapSkillComponent.cpp` 仅保留 `2` 个 `UGameplayStatics::ApplyDamage`，分别位于 GroundCircle 与 SelfSector 的 `!bUseM01ProductPath` compatibility 分支。

## 改动文件驱动的回归范围

本轮先以 `git diff --name-only b876d1c5e825827c8c201b06bcdf389816520b19` 取得实际改动，再按路径映射必跑组：

| 改动路径 | 必跑测试组 | 最终结果 |
| --- | --- | --- |
| `demo_mapCombatRunCoordinator*` | `Shanmen.0_0_10.Product.CombatRunCoordinator`、`Shanmen.0_0_10` | `14/14`、`110/110` |
| `demo_mapGameMode*` | `Shanmen.0_0_10` | `110/110` |
| `demo_mapSkillComponent*` | `demo_map.EnemySkillFramework`、`demo_map.V2RangedCompatibility`、`Shanmen.0_0_10` | `44/44`、`22/22`、`110/110` |

本轮未修改 Profile、ItemEconomy、CodeB 或 ShanmenItems 生产路径，因此没有追加无改动文件依据的存档／物品组。所有映射测试组均出现在本轮 Automation 日志中。

## 自动化证据

- CombatRunCoordinator 首轮：`13/14 Success`、`1 Fail`、queue empty、原生退出码 `1`；
- CombatRunCoordinator 修正测试基准后：`14/14 Success`、`0 Fail`、queue empty、原生退出码 `0`；
- EnemySkillFramework：`44/44 Success`、`0 Fail`、queue empty、原生退出码 `0`；
- V2RangedCompatibility：`22/22 Success`、`0 Fail`、queue empty、原生退出码 `0`；
- 0.0.10 全量：`110/110 Success`、`0 Fail`、queue empty、原生退出码 `0`。

新增 `PlayerShapeSkillsProduct` 与 `PlayerShapeSkillsFailClosed`，覆盖两个 family 的冻结 spec、独立 sequence、稳定 EntityId 排序、重复 overlap 合并、exactly-once commit／replay、Run reset、非法输入不消费 sequence、未注册 overlap 无生命写入以及跨 fixture 的 deterministic replay。

首次失败不是产品源码失败：测试把两个既有 M01 enemy 的当前 vitality 错误硬编码为固定值，实际 fixture 的 authored vitality 不同，导致两条预期失败。修正为先捕获非零真实基准，再断言基准减去 committed damage；仅修改测试断言，随后增量 Editor 编译和全部映射回归通过。首次失败日志保留。日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_combat_run_automation_initial.log`：`25E4AE7F69D5F7D753E3C5DED9AF3B902E429C65D5FBDFA6A3BBBA650F074612`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_combat_run_automation_final.log`：`6256F4585205150C3E6207E08C29DE84503949AC00A7F62F6D0E22C182B687E6`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_enemy_skill_automation_initial.log`：`8A762DB65CC274270419BBB9C9BDAF6E088CC620FD0460B6C6B9DEC481F90A9D`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_v2_ranged_automation_initial.log`：`5992D6A4E5F6CE15C191DC9C446DB193A384537FB7F1170B1142F73A7A9A2A4B`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_full_automation_initial.log`：`3099805E820149F57E3E4291BC371303BFA5F747BECCE5A4C9AA4B8DBBB60D00`。

五份日志各包含 UE 5.8 测试发现阶段既有的 `13` 条 `Condition failed` 负向自检诊断；最终目标测试全部成功，fatal、unhandled exception 与 handled ensure 均为 `0`。

## 构建与静态检查

- Editor Development 首次：`24/24` actions，`Result: Succeeded`，原生退出码 `0`，总执行时间 `107.22s`；
- 测试断言修正后的 Editor 增量构建：`4/4` actions，`Result: Succeeded`，原生退出码 `0`，总执行时间 `5.54s`；
- Game Development 最终：`23/23` actions，`Result: Succeeded`，原生退出码 `0`，总执行时间 `96.76s`；
- `git diff --check`：原生退出码 `0`；
- coordinator 对 `ApplyDamage`、随机 GUID／RNG 与地址身份 API 扫描：`0` 匹配；
- `ShanmenCombatCore`／`ShanmenCombatRuntime`／`ShanmenItems` 对 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage` 与随机 API 扫描：`0` 匹配；
- `SkillComponent` 中 `ApplyDamage` 共 `2` 处，均只在非 M01 compatibility 分支；
- 未发生 Windows commit-memory／页面文件错误；首次失败属于测试基准，不是源码或环境故障。

## 修改范围

- `Source/demo_map/demo_mapCombatRunCoordinator.h/.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
- `Source/demo_map/demo_mapGameMode.h/.cpp`
- `Source/demo_map/demo_mapSkillComponent.h/.cpp`
- 本 Report 与同名 Development Log

未修改、删除或提交工作区中的无关长期未跟踪文件。

## 兼容性与剩余边界

- P4.7 至 P4.11 的玩家 BasicSword、普通敌人 melee／dash／ranged、Heavy sector 与 Boss 三类攻击继续复用原 canonical resolver 和 vitality ledger；没有建立第二套伤害系统；
- 非 M01 GroundCircle／SelfSector 保留 legacy damage；
- 技能参数、瞄准、几何、阵营策略、冷却和视觉反馈仍由 `SkillComponent` 拥有；
- 玩家 Straight Projectile 仍是独立 legacy family，计划在 P4.13 按发射 sequence、projectile flight 与 impact ordinal 语义迁移；
- Actor 层真实世界接线已完成静态与 NullRHI 回归；真实输入、碰撞、视觉与冷却体验验收属于 F 阶段。

## P/F 边界

本 Report 只包含 P 阶段源码开发、代码审查、静态扫描、NullRHI headless Automation、Editor Development 与 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P4.13 建议迁移玩家 Straight Projectile：保留现有发射与飞行世界语义，冻结独立 projectile action／detector／formula，以施法 sequence 派生 ActivationId，并以合法命中 ordinal 派生 ImpactId；M01 canonical gate 内失败关闭，非 M01 compatibility 保留。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-12-player-shape-skills/Docs/Report/Dev.D.UE.0.0.10.P4.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-12-player-shape-skills/Docs/Log/Dev.D.UE.0.0.10.P4.12.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-12-player-shape-skills>
