# Dev.D.UE.0.0.10.P11.13.r0 Report

## 1. 结论

P11.13 已完成并通过 P 阶段门禁。

本阶段复核了 `Source/demo_map` 中全部非测试 `ApplyDamage` / 直接 `TakeDamage` 入口，没有发现现有已创作 M01 攻击仍绕过 canonical Impact。审计同时识别出 shared SkillProjectile 的一个结构性缺口：未来若新增敌对投射物来源但未注册为 Boss、Ranged 或 canonical Player，M01 玩家受击会回落到旧 `ApplyDamage`，从而绕过 P11.12 已接入的 WeaponGuard 与统一 defense stack。

本轮以最小改动关闭该潜在旁路：

- 已选中的 canonical route 永远拥有伤害交付；
- M01 中未注册来源命中玩家生命宿主时，投射物被消费并记录明确错误，但不调用旧 writer；
- 非 M01 与非玩家目标继续保留兼容路径；
- 没有新增 resolver、生命提交器或第二套伤害系统；
- focused 3/3、0.0.10 全量 534/534、EnemySkill 44/44、V2Ranged 22/22；
- regression mapping 102 rules，自检 164/164；
- changed-file gate：`PASS Changed=5 Rules=2 Required=4 Logs=4`；
- Editor 与 Game Development 单并发构建均成功，原生退出码 0。

## 2. 伤害入口审计

仓库当前共有 47 个非测试 `UGameplayStatics::ApplyDamage` 调用，直接 `TakeDamage` 调用为 0。

| 分类 | 数量 | 结论 |
|---|---:|---|
| 产品兼容 writer | 7 | 均位于 canonical ownership 之后的互斥兼容分支 |
| GameMode automation / visible driver | 17 | 构造验收场景，不是产品伤害权威 |
| V3ProgressionManager automation driver | 23 | 构造自动化场景，不是产品伤害权威 |

7 个产品兼容 writer 的边界如下：

- ordinary enemy melee：M01 canonical wrapper 成功取得路由所有权后不进入旧 writer；
- heavy sector 与 Boss shape：M01 canonical 与 legacy 为互斥分支；
- player BasicSword：M01 product path 在 legacy loop 前原子返回；
- GroundCircle / SelfSector：两个旧 writer 只在未取得 M01 product path 时执行；
- shared SkillProjectile：本轮把唯一旧 writer 收窄为显式 `LegacyCompatibility` 决策。

因此，本轮不是修复一条当前 authored M01 攻击缺陷；它是把此前依赖“来源类型必须完整枚举”的隐式安全条件改成可测试、失败关闭的结构约束。

## 3. 路由策略

新增纯值策略 `Fdemo_mapSkillProjectileDamageRoutePolicy`，输入只有：

1. 是否已经选择 canonical product；
2. 当前是否由 M01 enemy attack product path 持有路由；
3. contact 目标是否携带 PlayerVitality。

决策表：

| Canonical | M01 | PlayerVitality | Route |
|---:|---:|---:|---|
| 1 | 任意 | 任意 | `CanonicalProduct` |
| 0 | 1 | 1 | `RejectedUnregisteredM01HostilePlayer` |
| 0 | 其它 | 其它 | `LegacyCompatibility` |

`HandleProjectileContact` 只在 `LegacyCompatibility` 分支调用保留的单一 `ApplyDamage`。拒绝路径在 writer 前终止，并输出 source、target、sequence 与 ordinal，便于定位漏注册来源。

## 4. WeaponGuard 与权威边界

- 已注册 Boss/Ranged 敌对投射物继续进入 CombatRunCoordinator 的 canonical Impact；
- P11.12 的 active WeaponGuard Session、resource defense、vitality snapshot、resolver 与幂等提交顺序不变；
- 未注册来源不能通过 shared projectile 直接扣除玩家生命；
- GameMode 只提供既有 M01 路由所有权判断，不新增伤害算法；
- route policy 不读取 World、Actor 状态、RNG、输入或时钟；
- 被拒绝的 contact 不生成伪造 Impact receipt，也不修改生命。

## 5. 兼容性

- 非 M01 地图继续使用旧 SkillProjectile compatibility writer；
- M01 中不携带 PlayerVitality 的兼容目标不受本轮影响；
- canonical route 一旦被选择，即使执行结果 fail closed，也不会回落旧 writer；
- 现有 Boss、Ranged 与 Player projectile metadata、日志和视觉消费行为保持不变；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与其它资料未修改、未暂存、未提交。

## 6. 修改范围

实现与门禁共 5 个文件，176 行新增、7 行删除（不含本 Report/Log 与原始证据日志）：

- `Source/demo_map/demo_mapSkillProjectile.h`
- `Source/demo_map/demo_mapSkillProjectile.cpp`
- `Source/demo_map/demo_mapSkillProjectileDamageRouteTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

## 7. 测试覆盖

| Group | Success | Fail | Queue | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SkillProjectileDamageRoute` | 3 | 0 | 1 | `53C8531802BF3A37425135B5404C6BE7B14D7479608E58E1914E631F62166E15` |
| `Shanmen.0_0_10` | 534 | 0 | 1 | `DEBD1014AD419FB31F89D0B6CC85D05083C6F776CE4C8232F723E05E367E0F5E` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 1 | `58697AC1EDFD9A8CF942040FB0A0CE4E74AAF2744E17B8E20D2A0726E870CD0F` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `36C37E7CBA89B5F59319C07188178E5154A7CC9B0C4E112AF13BD81575B14257` |

原始日志合计 603 Success、0 Fail；focused 3 项包含于 full 534，按 identity 去重为 600 项。所有日志均有 terminal queue marker，且无 Fatal、Unhandled Exception 或 Ensure。

## 8. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=102
SELF_TEST: PASS 164/164
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=4 Logs=4
APPLY_DAMAGE_AUDIT: Total=47 ProductCompatibility=7 AutomationDrivers=40
DIRECT_TAKE_DAMAGE_AUDIT: 0
SKILL_PROJECTILE_LEGACY_WRITER: 1, only LegacyCompatibility
ADDED_AUTHORITY_SCAN: PASS AddedLines=72 ForbiddenHits=0
git diff --check: PASS (native exit 0)
```

- mapping SHA-256：`8FD21C5DE71DD51D62BF54158745487279BDB11A331B7852F20B23A38DFDC283`；
- self-test script SHA-256：`4CD4DA8134D155659D05C0E46BE2E94CF5E8351D0AE7E85851A646A5A20D93E2`；
- self-test evidence SHA-256：`112C59C71A1385A6700D7884D84B74284836A90BD773D2489240D99BD82822CC`；
- coverage evidence SHA-256：`83E9061F5E5C88160E039A08ABA0845ED5E79927458F84E6102C675576C9B6D9`；
- hostile audit evidence SHA-256：`F2D677DAF5C40988B270F6A9C03DCC0D180D43A87F640B7A9B85E93F0BD0E5D5`。

## 9. 构建与真实异常

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / time | Exit |
|---|---|---|---:|
| Editor Development | Succeeded | 12 / 68.69s | 0 |
| Game Development | Succeeded | 11 / 58.61s | 0 |

- Editor DLL：12,878,848 bytes，SHA-256 `445C7FC81F6CA0ACFBC6BD749861946A6EBE05E285EDF4C27AAA3E6801C6625A`；
- Game EXE：354,390,016 bytes，SHA-256 `8B3AC186D3BB70D8EF0AEB59E30DAB3AE43D492F2149A2DE2B3F5E7D45F95054`；
- Editor build log SHA-256：`CD52ABDBCC81D347BE00D6E6F1D0FF73E5E26533175C5C70F978308E41A874D7`；
- Game build log SHA-256：`BF9AAF6427888AA9FC92F1EECAC11DE4840CE64B527CC0374CAF945B39466B9E`；
- native exit summary SHA-256：`CEB5C21D08D2C50A59C688E5A43439D35662152293F819F042D3383745391579`。

没有源码编译失败、Automation 失败、Fatal、Unhandled、Ensure 或内存环境错误。Editor-Cmd 启动时仍报告未安装 LinuxArm64 / VisionOS SDK，同时明确 Win64 VALID；不影响四组 Win64 测试的原生退出码 0。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、实现、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

下一阶段建议进入 WeaponGuard 的明确中断语义：把受击硬直、武器切换、Run teardown 与 active guard Session 的 release/interrupt 原因统一到现有唯一 command route，并保持 input、GameMode 与生命权威分离。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-13-hostile-damage-audit>
