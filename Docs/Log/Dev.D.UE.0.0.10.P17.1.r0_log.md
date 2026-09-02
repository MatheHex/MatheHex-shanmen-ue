# Dev.D.UE.0.0.10.P17.1.r0 Development Log

## 1. 目标与基线

- 基线提交：`facd28f688d3c5ea89613ef586ac5cb902d8bd07`（P17.0 Heart-Protecting Mirror lethal interception）；
- 分支：`agent/0.0.10-p17-1-heart-mirror-product-lifecycle`；
- 目标：完成 P17.0 预定的 Coordinator 级无头产品纵切，证明真实 M01 enemy attack 能消费护心镜、重放幂等、耗尽后不再拦截，且持久状态可在 World teardown 后恢复；
- 约束：只做 P 阶段；不新增设计规则，不建立第二套物品、生命、伤害或持久化权威。

## 2. 起始审计

P17.0 已在 DefenseResourceAdapter fixture 中证明护心镜资源准备、致死触发、charge commit、非致死 cancel、法袍共存和 orphan recovery，但尚未把全部真实产品对象接成一条链。既有 CombatRunCoordinator 测试覆盖真实 M01 melee、生命提交与 Run 重绑，却没有从 Profile、准备栏和 ShanmenItems authority 部署护心镜。

因此本轮不改生产逻辑，而是在 Coordinator 测试文件内建立最小世界级 fixture，把两个既有测试岛连接起来。若该路径暴露生产缺陷再修生产；如果生产行为正确，则只提交测试闭环。

## 3. Fixture 实现

新增 `FHeartMirrorCombatRunFixture`：

1. 创建 fresh Profile，并放入正式护心镜定义；
2. 通过现有 warehouse/cutover/start-prepared-run 路径建立 active Run 与物品 authority；
3. 创建 transient `GamePreview` World 和真实 `UGameInstance`；
4. 生成玩家 Pawn 与 `Udemo_mapHealthComponent`；
5. 生成正式 `Ademo_mapEnemyCharacter`，装配 M01 melee definition 与 authored identity；
6. 用 `Fdemo_mapCombatRunCoordinator` 绑定 Run、注册玩家和敌人；
7. 提供有界 World teardown、GameInstance shutdown 与 authority restart。

fixture 不手工构造护心镜 defense layer，不直接调用 resolver，不直接修改生命或 charge。所有状态变化都必须由产品入口和现有权威完成。

## 4. 新增产品 Automation

新增 `Shanmen.0_0_10.Product.CombatRunCoordinator.HeartMirrorProductLifecycle`，顺序断言：

```text
initial:  vitality 3, mirror Deployed/charge 1
strike 1: raw 5 -> prevent 3 -> final 2 -> vitality 1, charge 0
replay:   AlreadyCommitted -> vitality/revision/broadcast/charge unchanged
strike 2: distinct action, no mirror layer -> applied 1 of raw 5 -> vitality 0
restart:  complete authority snapshot equals post-strike-2 snapshot
```

第一击同时要求唯一镜 reservation 已提交；第二击必须拥有新 ActivationId / ImpactId、没有任何相同 SourceInstanceId 的防御层，并只提交目标当时剩余的 1 点生命。

## 5. 首次失败、根因与恢复

首次 Editor 编译通过：4 actions / 29.14s。首次 exact Automation 得到 0/1；玩家从 3 HP 直接降至 0，说明 Coordinator 没有取得资源 authority。

诊断确认：测试创建了 WorldContext 并设置 `Context.OwningGameInstance`，但没有调用 `World->SetGameInstance(GameInstance)`。Actor 的产品查询使用 `GetGameInstance()`，所以生产代码观察到 null，并按 P17.0 的 fail-closed 规则拒绝伪造资源防御。失败原始日志：

- `P17.1_HeartMirrorProductLifecycle.log`
- SHA-256 `1998F2A896ED6277DC7C0BD5AD1E4406A8C9945CC35E899CB72754822C383223`

只在测试 fixture 补充 `World->SetGameInstance(GameInstance)`。没有放宽生产依赖或添加测试专用产品分支。增量 Editor 重编译 4 actions / 7.40s；exact retry 随后 1/0。

## 6. 最终测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `P17.1_HeartMirrorProductLifecycle_retry1.log` | exact HeartMirror product lifecycle | 1/0 | `804B9B3558EF643973939E440C5020A89DAD567D7908CAF827A99ED4A788D0AE` |
| `P17.1_CombatRunCoordinator.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 18/0 | `C8CFDBF28364876CBBE7E6FC40A409B229D5F277DC2535C185406243143E8D4F` |
| `P17.1_ShanmenFull.log` | `Shanmen.0_0_10` | 750/0 | `5010BFA277587D49C521D9810BADC7E78E5C0CA16EC19FA04EB8F9244F0FAC0E` |
| `P17.1_V3_Attributes.log` | `demo_map.V3.Attributes` | 4/0 | `C7E004BE61BA0281A7F678A139C7EB8408E24EE6465037A3508CE965512AA9F0` |
| `P17.1_EnemySkillFramework.log` | `demo_map.EnemySkillFramework` | 44/0 | `C1AE12606E616633CCF006FD1B576B8089368F6A02D3946F0C2BF6E0C96FDB53` |
| `P17.1_V2RangedCompatibility.log` | `demo_map.V2RangedCompatibility` | 22/0 | `FE7A58D4A48A84D0950E510DF88448C2990F4798F862B30DC1C0525C0196E28C` |
| `P17.1_ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `C7EAE0F8DFCAB9933D3159ECC09188CEEF9887720577C5B50A601F79266B7532` |

最终日志全部为 native success，Fail/Fatal/Unhandled/Ensure 为 0。full suite 首末 Success 时间为 `2026.09.02 17:54:00.203 -> 18:21:39.278 UTC`，约 27m39.08s；mapped legacy 合计 116/0。

## 7. 回归门禁

本轮唯一修改源码路径是 `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`。映射规则要求完整 Shanmen 测试及四个相邻 legacy 组；五份最终日志全部进入覆盖校验：

```text
REGRESSION_COVERAGE: PASS Changed=1 Rules=1 Required=16 Logs=5
SELF_TEST: PASS 273/273
GIT_DIFF_CHECK: PASS
```

`Scripts/ShanmenRegressionMap.json` 未修改，SHA-256 `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。

## 8. 最终构建

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

- Game Development：3 actions / 24.67s / native 0 / log SHA `EBD44F494DCB5B590918D38FBDD5A5D42026C00CF06866B94BF3F7121BEE78D3`；
- Editor Development：up to date / 0 actions / 0.98s / native 0 / log SHA `5FB50F9F76F16F7BFF022B6DE3DF7DEB28695DCBE14956B19F20D46278C66664`；
- `demo_map.exe`：356,148,224 bytes / SHA `30DE72E07832D7754E2FBDB1E6274CE8CD30DE61D9C7C3D0DA0863FAA099315D`；
- `UnrealEditor-demo_map.dll`：14,837,760 bytes / SHA `3D1571EF47A09F0DCFEE45A9FE14BBE1BBD85249F9241030C33045F6D4577665`。

## 9. 边界与提交范围

计划提交一个测试源码文件与本 Report/Log，共 3 个文件。生产代码、内容、资源、地图和配置均未改。没有运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料保持未暂存；`Saved/Codex/P17.1` raw logs 不入 Git。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p17-1-heart-mirror-product-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-1-heart-mirror-product-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P17.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-1-heart-mirror-product-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P17.1.r0_log.md>
