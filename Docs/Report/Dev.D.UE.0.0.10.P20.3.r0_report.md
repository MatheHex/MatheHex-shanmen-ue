# Dev.D.UE.0.0.10.P20.3.r0 Report

## 1. 结论

P20.3 已让既有 `Fdemo_mapShanmenThrownWeaponRunHost` 接受 P20.2 发布的弹道弧线，不新增 Actor、库存、伤害、碰撞或生命周期权威。

弧线 Host 现在从不可变 launch receipt 读取求解器的精确 `FlightTimeSeconds`，把它作为 Actor lifespan，并在寿命回调中发布独立的 `FlightTimeExpired` 终态。直线继续只使用 `MaximumDistance / LaunchSpeed` 和 `RangeExpired`，两种轨迹不能互相进入对方的启动或终止入口。

本轮只关闭 RunHost seam。`Fdemo_mapShanmenThrownWeaponRunCommandRouter`、产品控制器、会话、生命周期和玩家输入仍只路由直线动作，因此不声称玩家已能从场景输入发射弧线暗器。

本轮为 P 阶段。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`e0edf8d9ca8d4799faa8a5bd45dadc4d0a20ebd8`（P20.2）；
- 分支：`agent/0.0.10-p20-3-thrown-weapon-arc-run-host`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 显式生命周期契约

新增 `Fdemo_mapShanmenThrownWeaponHostLifetime`，以判别类型固定两种互斥语义：

- `RangeDistance`：只接受 Straight receipt，验证有限正距离与速度，并计算 `MaximumDistance / Speed`；
- `ArcFlightTime`：只接受 BallisticArc receipt，直接保留求解器的有限正飞行时间；
- `None` 或轨迹/数据不匹配：失败关闭。

Host 的有效性检查与恢复绑定都会针对 launch receipt 重建并核对该生命周期值，不能把弧线飞行时间伪装成直线距离，也不能向直线 flight 注入 Arc lifetime。

## 4. 单一启动与发布路径

保留原三个 Straight API，新增对应的显式 Arc API：

- `TrySpawnAndLaunchPreparedArc`；
- `TryLaunchPreparedArcCarrier`；
- `TryAdoptPublishedArcFlight`。

Straight 与 Arc 仅在 staging 操作和 lifetime 构造上不同。两者复用同一个 Host busy gate、action/execution/item/source/coordinator 绑定校验、惰性载体创建、durable commit、adoption、delegate binding 和终态清理实现。

因此 Arc 不拥有第二套库存扣减或 publication 流程。只有持久物品 commit 成功后，Host 才接管已发布的唯一 execution 与 projectile。

## 5. Actor 寿命与终态

Host-owned Arc carrier 的 `SetLifeSpan` 直接使用 receipt 的 `FlightTimeSeconds`。Actor 原有 lifespan delegate 保持唯一入口，但 Host 根据冻结的 lifetime kind 映射为：

- Straight → `RangeExpired`；
- BallisticArc → `FlightTimeExpired`。

两者都复用原 `FinishFlightWithoutImpact`、Action Recovery/Completed 转换、execution `Spent` 与 terminal receipt publication。Arc 的 `TryExpireRange` 和 Straight 的 `TryExpireFlightTime` 均失败关闭；终态不能重复执行。

## 6. 直线兼容与失败关闭

原 Straight 调用签名、距离语义、durable commit 顺序、contact 路径与 interruption 路径未改变。

新增自动化先用有效 Arc action 调用旧 Straight Host API。World staging 正确拒绝该轨迹，Host 保持 Empty，持久物品 authority snapshot 完全不变；随后同一 prepared intent 经 Arc API 成功发布并只扣减 1 件物品。这证明错误入口既不会降级为直线，也不会提前消费资源。

## 7. 自动化结果

本轮新增 1 项完整产品测试：

- `Shanmen.0_0_10.Product.ThrownWeaponRunHost.ArcFlightTimeLifecycle`。

它使用 transient `UWorld`、真实 `UGameInstance` item authority、active Run correlation、Action runtime、唯一 execution 与真实 projectile，验证错误直线路由不变更权威、Arc durable commit `3 → 2`、精确 lifespan、轨迹专属 expiry 和终态去重。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `runhost_final.log` | `Product.ThrownWeaponRunHost` | 4/0 | `6E4E9DA2D814C9A027A6C42705819E5BC627D1EDE366DA7FCDC3F52BCBDCDDF5` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | 849/0 | `1FB196755D6AAB2DC8BFA11B4D89A46541E8F7B7EE86ECE7DF725F74A5F1E4D6` |
| `legacy_v3_attributes_final.log` | `demo_map.V3.Attributes` | 4/0 | `F2CBDE93E7452CA5DAF5A00EA7CAE82E4D1905F32C5339A5CFD4F592B6D7126A` |
| `legacy_enemy_skill_final.log` | `demo_map.EnemySkillFramework` | 44/0 | `CE31CFA5F93AAFC88111CF7372E056F3BAC989CE82F51C167F6D7F85017DDD56` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | 22/0 | `180951B270A0A65A2EF37EADFCB774B074F7E2C6A6833B8B68F455851BFD90E3` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | 46/0 | `5C9F7CDFBE1D1F1ED26BDFE1942EB2CF0AF385EB94B78939B55FC4E00930D2EF` |

全量从 P20.2 的 848 增至 849。证据审计确认 6 份日志均只有一个 canonical `RunTests` command、Fail 0、一个 native terminal、Fatal/Unhandled/Ensure 0：`PASS Logs=6 RecordedSuccess=969`，审计 SHA-256 `37652C0C2EE6A5C0C882AA94D9A364BAF8E38980DFAE142F567FD990BF0A23EE`。

## 8. 改动门禁与静态边界

最终 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=8 Logs=2
```

8 个要求组为 RunHost、RunCommand、WorldDelivery、ItemAdapter、CombatRunCoordinator、Items、WorldGameplay 与 CombatRuntime。gate SHA-256：`03586DCF73F15B012F3F4EE1088503EACE49E3EC3BCE4B8F9476F3840CA3DB18`。

对 2 个 production 文件扫描 direct damage、direct inventory mutation、RNG、trace/sweep ownership、输入与 GameMode 接入：`PASS Files=2 Matches=0`，SHA-256 `E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`。

`git diff --check` 原生通过；记录 SHA-256 `E2A7201E00C37A4AFB08A769D2961539AE404FAECE0CE0043F1206019DE80640`。

本轮首次 Editor 构建与首次 Host exact 测试均成功，没有失败测试需要隐藏或修写；后续仅做公开读取器命名澄清，并对最终源码重新构建、复测和全量验证。

## 9. 构建与产物

- first Editor：37 actions / 66.90s / succeeded，SHA-256 `1DA451285CBB8DC1E154D5DDF917D608C603712F08E06C67441F67CACEE6C02A`；
- final Editor：37 actions / 47.88s / succeeded，SHA-256 `DB1BFFE28136F31E26EF2D65DEB17A538422EDE99085C87D426103DD2BA6BD86`；
- final Game：36 actions / 65.51s / succeeded，SHA-256 `23ECF45E9838FA810CB42C87E9E1628D000F4DF4480AED706472E60B45F1DE42`。

产物：

- `Binaries/Win64/demo_map.exe`：357,097,984 bytes，SHA-256 `E9B6D4A7F78062D3A4E9757D60706D692B11F71A20D828D9E1803BCE47C5B258`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,753,216 bytes，SHA-256 `089B960B77614C2EEA779F25A7E57C32558695C4822F20B294301BA96180D289`。

## 10. P/F 边界与下一步

P20.3 证明的是“已准备并规划的 Arc action 如何通过唯一 RunHost 发布、按求解器飞行时间到期并完成”。它没有让现有 command intent 携带 Arc plan，也没有把产品 controller、session、lifecycle 或 input adapter 接到 Arc Host API。

建议 P20.4 增加明确的 Arc RunCommand intent/router：命令必须冻结 action、Arc plan 与身份，复用同一 item prepare/commit 和 Host；不得让旧 Straight intent 猜测目标点、弧高或飞行时间。产品控制器和滚轮弧度输入继续后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-3-thrown-weapon-arc-run-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-3-thrown-weapon-arc-run-host/Docs/Report/Dev.D.UE.0.0.10.P20.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-3-thrown-weapon-arc-run-host/Docs/Log/Dev.D.UE.0.0.10.P20.3.r0_log.md>
