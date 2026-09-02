# Dev.D.UE.0.0.10.P18.1.r0 Development Log

## 1. 目标与基线

- 基线提交：`1d5b4725f7a38fb5a1b4931796635791dc5a69b1`（P18.0 Sword Qi runtime contract）；
- 分支：`agent/0.0.10-p18-1-sword-qi-world-delivery`；
- 目标：把 P18.0 剑气 execution 接入最小 UE 世界载体和唯一生命交付路径；
- 约束：仅做 P 阶段，不接实际输入、正式剑动作 host、表现资源、自动 Spawn 或第二套生命/伤害权威。

## 2. 起始审计与设计决定

现有项目已经有可复用的 `FShanmenWorldHitAdapter`、EntityRegistry、CombatRunCoordinator vitality delivery，以及暗器的 staged physical carrier 模式。P18.0 已提供剑气定义、Launch ID、Projectile candidate、Impact ledger 与 defense resolver。

因此本轮不复制算法：建立剑气专属 Actor 与 Adapter，复用通用身份/生命 seam。剑气不复用旧技能弹体，也不从暗器库存事务派生。成功 contact 后保持 InFlight，把穿透/命中消散交给后续产品 host 决定。

## 3. 生产源码

新增：

- `Source/demo_map/demo_mapShanmenSwordQiProjectile.h/.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiWorldAdapter.h/.cpp`。

修改：

- `Source/demo_map/demo_mapCombatRunCoordinator.h/.cpp`。

载体实现 `Empty/Staged/InFlight/Dissipated` 状态机、惰性 Stage、QueryOnly sphere、零重力 ProjectileMovement、source ignore、native contact/range seam 与显式 dissipate。Adapter 实现 copy-on-write launch plan、注册来源校验、原子 publication、world-hit candidate、vitality snapshot、P18.0 resolve、canonical coordinator delivery、range/block finish 与 action termination cleanup。

Coordinator 新增 `DeliverSwordQiImpactToM01Enemy` 类型安全入口，内部委托既有 `DeliverResolvedPlayerImpactToM01Enemy`，没有形成另一条生命写入路径。

## 4. Automation 与 fixture

新增 `Source/demo_map/demo_mapShanmenSwordQiWorldAdapterTests.cpp`，三条测试为：

1. `Shanmen.0_0_10.Product.SwordQiWorldDelivery.LaunchGateAndFlight`；
2. `Shanmen.0_0_10.Product.SwordQiWorldDelivery.ContactToVitalityAndReplay`；
3. `Shanmen.0_0_10.Product.SwordQiWorldDelivery.FailClosedRangeAndTermination`。

fixture 使用 transient Pawn、现有 M01 enemy identity/vitality host 与真实 CombatRunCoordinator。伤害公式为 `0.5 + 20 * 0.01 = 0.7`；断言 current vitality 减少 0.7、authority revision 增加 1，并证明等价 contact 重放不产生第二次提交。

## 5. 首次失败与修正

首次 Editor build 编译 105 actions，新增生产文件均通过，但新增测试翻译单元通过 `USphereComponent` 前置声明调用 API，触发 C2027 并连带触发 C2661。UBT 最终为 `OtherCompilationError`，原生退出码 6，总耗时 351.25s。原始失败日志保留，SHA 为 `DDEE110606B8D6AB7259BB8CFD6523966D5393AC3AE0CC579282667923876517`。

只在测试文件加入 `Components/SphereComponent.h` 后重试：4/4 actions、native 0、7.63s，日志 SHA `069FF96A2435FFA6BF5EDEC4CB70112892EEF6862260483B2B54D997B3401642`。没有为了测试结果改变生产语义。

## 6. 测试执行

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_sword_qi_world_delivery.log` | exact product group | 3/0 | `3DF5DE8F9E3B2423F993A3D7DF5CD4ADE52F2BCE27701D581F1158C4FDDE2324` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 758/0 | `5FD51B421EEF61BC7BDF9A0616F07A9F3A3CC731B10C50ECCC75460BCC65C6C0` |
| `automation_legacy_v3_attributes.log` | `demo_map.V3.Attributes` | 4/0 | `380E07FB6EBB3755A32494A174F0F4D563F0851B8956D76487AD3B7198198D21` |
| `automation_legacy_enemy_skill_framework.log` | `demo_map.EnemySkillFramework` | 44/0 | `C3ED47BB27CE4B5423EAB71BEE835C43DFE1C61F029A82BCBA60C606E10059DA` |
| `automation_legacy_v2_ranged_compatibility.log` | `demo_map.V2RangedCompatibility` | 22/0 | `FDC854A03B57417DEF216197B50D1251B25B93B7B7E778E8B60FE9ACC7880A7D` |
| `automation_legacy_item_use_and_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `5A5D839B5B19556E27003CF2799FC42BEDA5AD9EF150DEC52FA92B42A08C69F7` |

所有日志均为 native test exit 0，Fail/Fatal/Unhandled/Ensure 为 0。full 首末 Success 为 `20:28:40.727 -> 20:56:58.304 UTC`，约 28m17.577s。

## 7. 改动驱动回归与静态门禁

修改 `Scripts/ShanmenRegressionMap.json`，新增 `SwordQiWorldDelivery` 路径规则，要求 exact product、CombatRunCoordinator、WorldGameplay、CombatRuntime 与 CombatCore。修改 self-test，加入完整证据通过 fixture 与聚焦证据不足的预期失败 fixture。

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=18 Logs=6
SELF_TEST: PASS 275/275
BOUNDARY_SCAN: PASS Files=5 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage log SHA：`B50DF28A4A8C4DD0B1C480120A4168271DF69D661288EC02559C0ED3343DA892`；
- self-test log SHA：`4F3870AF407EAE69CF9F4F93124E543C5B55B37587B4933787FAE32CD9296733`；
- regression map SHA：`1E4E12A0418630F1FBDCA35D25A4CD60C7C34447D1116643BAFDAA1EC9DB94D3`。

静态扫描没有发现直接伤害调用、旧技能弹体依赖、随机流、timer 或 Adapter 内自建 Spawn。

## 8. 最终构建与产物

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

- Game Development：102/102 actions、native 0、303.26s、log SHA `D87E042BEFBB03CE95C7D420531FBA2739DFC77B582A8011ACEBDFD18FDFA984`；
- Editor Development：up to date、0 actions、native 0、1.00s、log SHA `FEEEFF8C93E98AE23732F76E70BD714F4024159ABEB95901D0B3B7868706057C`；
- `demo_map.exe`：356,239,872 bytes、SHA `BE2F902E8206C7179850A4CA6E4E8CBD5887568AD1DA95377423ECD3E5121097`；
- `UnrealEditor-demo_map.dll`：14,891,520 bytes、SHA `A7D70F0F73A6CE5E0150D814CFEF966FE56F19706CA14837196397CD84BF141A`。

构建只验证编译与链接，没有启动产品。

## 9. 边界与提交范围

计划提交 5 个新增源码文件、2 个 Coordinator 文件、2 个回归门禁文件与本 Report/Log，共 11 个文件。未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料保持未暂存；`Saved/Codex/P18.1` raw logs 不入 Git。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-1-sword-qi-world-delivery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-1-sword-qi-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P18.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-1-sword-qi-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P18.1.r0_log.md>
