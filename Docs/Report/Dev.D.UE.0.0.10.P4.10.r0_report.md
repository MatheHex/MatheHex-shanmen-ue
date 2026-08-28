# Dev.D.UE.0.0.10.P4.10.r0 开发报告

## 结论

`PASS`。P4.10 已把 M01 authored `StandardBruiser` 与 `EliteBulwark` 的扇形重击接入 0.0.10 canonical combat。重型 Actor 在蓄力开始时预留 Run-local `uint64` action sequence；合法扇形、LOS 与阵营接触仍由既有 Actor 世界逻辑授权，随后由 coordinator 确定性派生 ActionId／ImpactId、捕获玩家 defense snapshot、经纯函数 resolver 结算，并只通过 vitality ledger 提交一次生命变化。

M01 重击进入产品 gate 后，即使 coordinator 未 ready、sequence／binding／source vitality／delivery 无效，也不会回落 legacy `ApplyDamage`；本次攻击仍按既有语义进入 recovery。非 M01 heavy继续保留原兼容路径，M01 Boss仍由独立 Actor与技能体系负责，没有被误接入本 family。

## 功能性

### 1. 冻结 HeavySector family

- family：`HeavySector`；
- Action：`Combat.Action.Enemy.Heavy.Sector`；
- detector：`Detector.Enemy.Heavy.Sector`；
- detector kind：`Shape`；
- formula：`Combat.Formula.Enemy.Heavy.Sector.r1`；
- content version：`0.0.10.P4.10`；
- content digest：`Shanmen.M01Enemy.Heavy.Sector.r1`；
- M01 配置静态审计确认共有 `3` 个 heavy definition：`2` 个 `StandardBruiser` 与 `1` 个 `EliteBulwark`，均使用空 SkillProfile、相同扇形行为和各自 tuning；
- family binding同时要求空 SkillProfile与 `Ademo_mapHeavyEnemyCharacter` 类型，因此空 profile的独立 Boss Actor不能借用该入口；
- receipt继续统一校验 family、action、content、detector、kind、formula、damage／target tags、`HitOrdinal == 0` 与伤害守恒。

### 2. action identity 与 Run 重置

- Heavy Actor新增独立的 `NextAttackSequence`／`ActiveAttackSequence`，没有复用既有 `ResolveCount` 诊断计数；
- sequence只在方向有效且正式进入 windup前预留并递增，因此 delayed resolve、取消和失败仍对应同一个已发生的 action；
- zero与 `MAX_uint64` 均失败关闭，耗尽时进入 cooldown退避，避免每个 AI tick持续重试；
- sequence在 encounter configuration与 M01 新 Run首次注册时重置为 `1`；同一 Run的幂等重复注册不会重置；
- RunId、稳定 source EntityId、冻结 ActionDefinitionId与 sequence确定性派生 ActivationId；ActivationId、detector、稳定 player EntityId与固定 ordinal派生 ImpactId；
- 身份不依赖帧号、计时器触发时间、对象地址或随机 GUID。

### 3. 世界授权与唯一生命写入

- 原有 sector radius、full angle、vertical tolerance、锁定方向、WorldStatic LOS与 faction filter继续在 Heavy Actor执行；
- canonical resolver不查询 World，shape contact仅接收已授权目标位置与 source-to-target normal；
- GameMode只在 M01地图声明产品所有权；该 gate为真后，Actor无条件调用 canonical入口，不提供 legacy fallback；
- coordinator重新验证 active Run、注册 source、authored binding、source vitality、目标 Pawn与 defense host；
- 玩家生命最终只由 `CommitCombatImpact` ledger写入；
- Heavy Actor文件保留恰好 `1` 个 `UGameplayStatics::ApplyDamage`，仅位于非 M01 compatibility `else` 分支。

### 4. exactly-once 与 Run 隔离

- standard heavy `2.0` raw经 `0.25` flat defense得到 `0.25` prevented与 `1.75` final／applied；
- elite bulwark复用同一公式但使用不同稳定 source identity，`3.0` raw得到 `2.75` final／applied；
- 完整 receipt重放返回 `AlreadyCommitted`，不重复扣血、revision、ledger或 damage broadcast；
- 同一 sequence在 authority revision变化后重新构造，得到相同 ImpactId但不同 snapshot，ledger以 `CommitRejected`失败关闭；
- sequence `2`产生独立 action；
- 旧 Run receipt以 `RunMismatch`拒绝；新 Run sequence `1`因 RunId不同而产生新身份；
- 致死伤害按目标剩余 vitality clamp，receipt记录真实 applied damage。

## 自动化证据

### CombatRunCoordinator 定向

新增 `Shanmen.0_0_10.Product.CombatRunCoordinator.M01EnemyHeavySectorProduct`，使 coordinator套件从 10条增至11条，覆盖：

- zero／exhausted sequence与 ranged-source/heavy-family mismatch在 mutation前拒绝；
- standard与elite authored source；
- exact action／detector／kind／formula／content；
- fractional defense、守恒与首次 commit；
- exact replay、same-sequence reconstruction reject与 distinct next sequence；
- old-Run receipt rejection、新 Run sequence reset、identity与 lethal clamp。

最终：`11/11 Success`、`0 Fail`、queue empty，原生退出码`0`。

### 0.0.10 全量

最终：`107/107 Success`、`0 Fail`、queue empty，原生退出码`0`。

### 兼容回归

- EnemySkillFramework：`44/44 Success`、`0 Fail`；
- V2RangedCompatibility：`22/22 Success`、`0 Fail`；
- ItemUseAndArmor：`46/46 Success`、`0 Fail`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_combat_run_automation_final.log`：`2F1DC28E814572AB6D93C103D74F4F2BEA38F30B3A56D0046405CAE1C82C9D68`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_full_automation_final.log`：`545053C252BDD1382BD77F8E885C90F4F12415F3DF99D063D8B1A1B2CE43C684`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_enemy_skill_automation_final.log`：`DC3BB155FB68114A641BF3B85418B49B4DF60CD9D67436E3BCE0B5B915D5ACA1`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_v2_ranged_automation_final.log`：`43841EA3DFC9D2B76A379A0FF3DA74D29878DBE0EA0770A06CEC73C64BD4AC0D`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.10.r0_item_armor_automation_final.log`：`AE05E5ED53598711C7B90C3728911363AA66A2E750D225041F134752373C1B2E`。

五份最终日志各包含 UE 5.8测试发现阶段既有的13条`Condition failed`负向自检诊断；目标测试全部成功，fatal、unhandled exception与 handled ensure均为`0`。

## 首次执行记录

- coordinator初跑：`11/11 Success`，退出码`0`；
- EnemySkillFramework初跑：`44/44 Success`，退出码`0`；
- V2RangedCompatibility初跑：`22/22 Success`，退出码`0`；
- ItemUseAndArmor初跑：`46/46 Success`，退出码`0`；
- 0.0.10全量初跑：`107/107 Success`，退出码`0`。

本阶段没有源码、测试基线或环境失败，也没有删除初始日志；初跑与最终复跑证据均保留在`Saved/Logs`。

## 构建与静态检查

- Editor Development：`25/25` actions，`Result: Succeeded`，原生退出码`0`；
- Game Development：`24/24` actions，`Result: Succeeded`，原生退出码`0`；
- `git diff --check`：原生退出码`0`；
- coordinator对 legacy damage、随机 GUID／RNG与地址身份API扫描：`0`匹配；
- `ShanmenCombatRuntime`对`demo_map`、`UWorld`、`AActor`、`ApplyDamage`与随机API扫描：`0`匹配；
- Heavy Actor中只有一个compatibility `ApplyDamage`，canonical分支为`0`；
- 未发生 Windows commit-memory／页面文件错误。

## 修改范围

- `Source/demo_map/demo_mapCombatRunCoordinator.h/.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
- `Source/demo_map/demo_mapGameMode.h/.cpp`
- `Source/demo_map/demo_mapHeavyEnemyCharacter.h/.cpp`
- `Source/demo_map/demo_mapEnemySkillFrameworkTests.cpp`
- 本 Report与同名 Development Log

未修改、删除或提交工作区中的无关长期未跟踪文件。

## 兼容性与剩余边界

- P4.7 ordinary melee、P4.8 melee dash与P4.9 ranged projectile继续复用同一 enemy attack receipt、resolver与 vitality ledger；没有建立第二套 heavy结算系统；
- 非 M01 heavy保留既有扇形行为与 legacy伤害入口；
- 本轮没有改写重型敌人的sector geometry、LOS、faction、AI追踪、windup、recovery、cooldown、VFX或攻击次数诊断；
- M01 Boss仍未迁移，继续保留独立攻击家族边界；
- Actor层真实世界接线已完成静态与NullRHI回归，真实时序／碰撞／视觉验收属于F阶段。

## P/F 边界

本 Report只包含P阶段源码开发、代码审查、静态扫描、NullRHI headless Automation、Editor Development与Game Development构建。未启动Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook或Package。

## 下一步

P4.11建议审计并迁移M01 Boss攻击：先按真实攻击形态拆分稳定family与detector，再复用现有enemy attack kernel；不得把Boss projectile或多阶段技能压入本轮HeavySector family。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-10-heavy-attack-product/Docs/Report/Dev.D.UE.0.0.10.P4.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-10-heavy-attack-product/Docs/Log/Dev.D.UE.0.0.10.P4.10.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-10-heavy-attack-product>
