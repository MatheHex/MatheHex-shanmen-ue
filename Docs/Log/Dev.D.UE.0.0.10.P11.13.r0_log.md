# Dev.D.UE.0.0.10.P11.13.r0 Development Log

## 目标

按 P11.12 的下一步建议，复核剩余 hostile-damage 入口，区分真实产品路径与 automation/demo driver；只关闭仍可能绕过 canonical Impact 的结构性产品旁路，不为消除文本匹配而迁移测试驱动代码。

## 基线

- branch：`agent/0.0.10-p11-13-hostile-damage-audit`；
- base：`49f96aaae6b44c5e461bbabfd0f8162540fcd510`；
- P11.12：六类 M01 敌对 contact 已接入唯一 active WeaponGuard Session；
- P4.14：已有一次全仓 damage audit，结论为当前 authored M01 产品入口均已 canonical；
- P 阶段，不启动产品。

## 审计过程

### 全仓入口

对 `Source/demo_map` 非测试 `.cpp` 执行精确调用扫描：

```text
UGameplayStatics::ApplyDamage: 47 occurrences
direct TakeDamage calls: 0 occurrences
```

按调用图和预处理边界分类：

- 7 个产品兼容 writer：Enemy 1、Heavy 1、Boss 1、PlayerController 1、SkillComponent 2、SkillProjectile 1；
- GameMode 17 个：automation / visible acceptance driver；
- V3ProgressionManager 23 个：automation scenario driver。

现有 authored M01 product bypass 为 0。唯一需要处理的是 shared SkillProjectile 的未来来源退化：未知 hostile source 在 M01 命中 PlayerVitality 时，旧条件 `!bUsedCanonicalProduct` 会允许 legacy writer。

## 实现

### 纯路由策略

在现有 SkillProjectile 类型旁新增：

- `Edemo_mapSkillProjectileDamageRoute`；
- `Fdemo_mapSkillProjectileDamageRoutePolicy::Resolve`。

三种输出为 `CanonicalProduct`、`LegacyCompatibility`、`RejectedUnregisteredM01HostilePlayer`。策略无 World、Actor、时钟、RNG、输入或伤害计算依赖。

### Actor 接线

`HandleProjectileContact`：

1. 只读取一次 `ShouldUseM01EnemyAttackProductPath`；
2. 保留 Boss、Ranged、canonical Player 的既有分派；
3. contact 消费后，用纯策略选择最终 delivery route；
4. 唯一 `ApplyDamage` 只允许在 `LegacyCompatibility` 分支执行；
5. M01 未注册敌对来源命中 PlayerVitality 时记录精确 source/target/sequence/ordinal 并停止；
6. canonical 执行即使失败，也不回落到 legacy。

## 测试

新增 3 项纯策略测试：

- `CanonicalPrecedence`：4 种 M01/target 组合下 canonical 均优先；
- `M01HostilePlayerFailClosed`：未注册来源不能进入 legacy；
- `CompatibilityBoundary`：非 M01 与非 PlayerVitality 目标保持兼容。

最终证据：

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `..._automation_skill_projectile_damage_route_first.log` | focused | 3 | 0 | `53C8531802BF3A37425135B5404C6BE7B14D7479608E58E1914E631F62166E15` |
| `..._automation_shanmen_full_final.log` | full | 534 | 0 | `DEBD1014AD419FB31F89D0B6CC85D05083C6F776CE4C8232F723E05E367E0F5E` |
| `..._automation_enemy_skill_final.log` | EnemySkill | 44 | 0 | `58697AC1EDFD9A8CF942040FB0A0CE4E74AAF2744E17B8E20D2A0726E870CD0F` |
| `..._automation_v2_ranged_final.log` | V2Ranged | 22 | 0 | `36C37E7CBA89B5F59319C07188178E5154A7CC9B0C4E112AF13BD81575B14257` |

## 回归映射

新增 `SkillProjectileDamageRoute` rule：SkillProjectile header/cpp/test 必须同时提供 focused、full、EnemySkill 与 V2Ranged 健康日志。

Self-test 新增：

1. 完整四组证据通过；
2. focused 不能替代三组 broad compatibility 证据；
3. 既有 failed-log / missing-terminal fixture 补入 focused，使其仍验证原目标而不是先因缺组失败。

最终：

```text
REGRESSION_MAP_JSON: PASS Rules=102
SELF_TEST: PASS 164/164
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=4 Logs=4
```

## 静态门禁

```text
APPLY_DAMAGE_AUDIT: TotalOccurrences=47 ProductCompatibility=7 AutomationDrivers=40
DIRECT_TAKE_DAMAGE_AUDIT: Occurrences=0
SKILL_PROJECTILE_LEGACY_WRITER: Occurrences=1
SKILL_PROJECTILE_ROUTE_FENCE: PASS
ADDED_AUTHORITY_SCAN: PASS AddedLines=72 ForbiddenHits=0
REACHABILITY_CLASSIFICATION: AuthoredM01ProductBypass=0 LatentUnregisteredProjectileBypassClosed=1
GIT_DIFF_CHECK: PASS NativeExit=0
```

新增源码不包含 resolver、vitality commit、直接 TakeDamage 或第二套伤害权威。

## 构建

统一参数：`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Build | Result | Actions / time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development | Succeeded | 12 / 68.69s | 0 | `CD52ABDBCC81D347BE00D6E6F1D0FF73E5E26533175C5C70F978308E41A874D7` |
| Game Development | Succeeded | 11 / 58.61s | 0 | `BF9AAF6427888AA9FC92F1EECAC11DE4840CE64B527CC0374CAF945B39466B9E` |

## 真实异常

- 无源码、测试、门禁或构建失败；
- Editor-Cmd 平台探测打印 LinuxArm64 / VisionOS SDK unavailable，但 Win64 为 VALID，四组测试退出码均为 0；
- 一次临时 `rg` 命令使用 Windows 不支持的文件通配参数并返回路径语法错误；最终统计改用目录 + `--glob` / literal path，未进入正式证据结论。

## 修改统计与保护

实现与门禁：5 files，176 insertions，7 deletions；另新增本 Report、Development Log 与 10 份原始证据日志。

没有执行 `git add .`；长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与其它资料保持原状。

## P/F 边界

仅执行 P 阶段实现、静态审查、无头 Automation 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-13-hostile-damage-audit>
