# Dev.D.UE.0.0.10.P20.2.r0 Report

## 1. 结论

P20.2 已让 P7 的既有暗器物品门、World adapter 与物理载体消费 P20.1 的统一 launch receipt。直线仍保持零重力与原速度上限；弧线则从不可变 plan 读取初速度与重力，并由同一个 Actor、同一个持久物品事务、同一个 emission、Impact ledger、Defense resolver 和终止路径完成交付。

本轮没有建立第二套弧线 Actor、库存扣除、伤害结算、碰撞回调或生命周期权威。弧线与直线的产品差异只存在于明确的 action/purpose 身份和运动配置中。

本轮只关闭 item/world carrier seam。RunHost、命令路由、产品控制器和玩家输入仍未接受 Arc action，因此不声称玩家已经能够在场景中发射弧线暗器。

本轮为 P 阶段。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`16e960a7fb34099c7a50976fea8c0e25e8c65df9`（P20.1）；
- 分支：`agent/0.0.10-p20-2-thrown-weapon-arc-world-delivery`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 持久物品门

`Fdemo_mapShanmenThrownWeaponItemAdapter` 现在只接受两个精确 action：

- `Combat.Action.ThrownWeapon.Straight01` → `Shanmen.ThrownWeapon.StraightLaunch.r1`；
- `Combat.Action.ThrownWeapon.Arc01` → `Shanmen.ThrownWeapon.ArcLaunch.r1`。

prepare replay 必须匹配相应 purpose，commit 仍要求同一 active Run、同一 exact item、同一 activation intent 和匹配的不可变 launch proof。其它 action 继续失败关闭；没有放宽为 generic projectile，也没有让 adapter 直接修改旧库存。

## 4. 单一 World staging 管线

原 `StagePreparedLaunch` 保持为直线兼容入口。新增 `StagePreparedArcLaunch`，但两者只把不同的 launch 操作传给同一个内部 staging 管线。

共享管线仍按原顺序验证：Run correlation、prepared item、Action runtime、唯一 execution、launch receipt、emission、item commit request、惰性 projectile staging。任何阶段失败都不会发布飞行；真正的持久扣除成功后才调用原 publication 路径激活载体。

因此 Arc 不拥有平行的 commit、contact、damage 或 cleanup 实现。

## 5. Receipt 驱动的载体运动

`Ademo_mapShanmenThrownWeaponProjectile` 新增内部运动配置投影：

- Straight：使用 receipt 的 initial velocity；`ProjectileGravityScale=0`；`MaxSpeed` 保持旧固定 launch speed；
- BallisticArc：使用 plan 的完整 initial velocity；要求有限、仅 Z 轴向下的 plan gravity；以 `PlanGravityZ / WorldGravityZ` 得到正的 `ProjectileGravityScale`；`MaxSpeed=0`，避免下降段自然增速被直线速度上限裁剪。

Arc staging 必须取得有效且向下的 World gravity authority；缺失 World、非向下 World gravity、横向 plan gravity 或非有限配置均失败关闭。source 与 carrier 还必须属于同一个 World。

staged 状态保持碰撞关闭与 movement inactive，但冻结完整运动配置。持久 commit 发布时重新应用同一配置后激活；取消 staging 时清空速度、最大速度与 gravity scale。飞行期间允许实时 velocity 随重力变化，但 immutable speed/gravity 配置必须保持一致。

## 6. 生命周期与直线兼容

弧线发布后继续进入原 `InFlight` 状态，使用原 projectile contact delegate、execution candidate、Impact ledger 与 `FinishFlightWithoutImpact`。新增测试证明弧线可通过原 durable publication 启动，并通过原无命中终止路径进入 `Spent`。

旧直线 API、purpose、零重力、固定速度、碰撞门和持久扣除顺序均未改变。Arc action 不能退回直线 staging 入口绕过 plan 验证。

## 7. 自动化结果

本轮新增 3 项测试：

- `ThrownWeaponItemAdapter.ArcLaunchCommit`；
- `ThrownWeaponWorldDelivery.ArcReceiptMotion`；
- `ThrownWeaponWorldDelivery.ArcFailClosed`。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `automation_world_first.log` | `Product.ThrownWeaponWorldDelivery` | 5/0 | `F3A86B50D05AEC93A67A48019C11F5D4CFEB89F5036BFFE33A19052037A68041` |
| `automation_item_final.log` | `Product.ThrownWeaponItemAdapter` | 5/0 | `887D22C80462A28B081F8A4B1D457F7A531CE4D2309F0893EEF2F48ECBC0FC68` |
| `automation_combat_runtime.log` | `CombatRuntime` | 140/0 | `3EB4F7BAA92D154D317B6416C09083BA7D52F9BFEB9C384A1927582E76099B15` |
| `automation_items.log` | `Items` | 77/0 | `E5C7EA0D5CB2CF5C4F40F9BF1697AFD0C05C368CFCDD4867068EC88E812890C9` |
| `automation_world_gameplay.log` | `WorldGameplay` | 10/0 | `CDDC39A860F98AE0C9E8AC685D0FCC246EB65E7C8C6C1375AEBEDDF88AAB73F9` |
| `automation_combat_run_coordinator.log` | `Product.CombatRunCoordinator` | 18/0 | `258A72C8F6E2C3ECE052A4601FC965507FECDE0CFFC35B9704296ACF45DEA5E8` |
| `automation_full.log` | `Shanmen.0_0_10` | 848/0 | `88146C4DA243357A401C67853C1617A9D50F03553BD7E7FF30979CEF5C10A9F7` |
| `automation_legacy_attributes.log` | `demo_map.V3.Attributes` | 4/0 | `168014C99CD2BC3D34D78D977690240ABB7723F35C2B331B564C9141CEB6673D` |
| `automation_legacy_enemy.log` | `demo_map.EnemySkillFramework` | 44/0 | `B1EF2A9D1E29FDDCD7F1759F6646FFC00DD945FECBF3F7D0EEA72C5D630A6F6D` |
| `automation_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | `9BD4199FEAFCABF3A196A1FD8FAED6207A92F7EC315539DCA51BF9DF2D818E3E` |
| `automation_legacy_item_use_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `872132A9297F970B7274C3ED0CCE123EA208F76777FDBCAF07DC0E831DC0D8F6` |

全量由 P20.1 的 845 增加到 848。证据审计确认 11 份健康日志均只有一个 canonical `RunTests` command、精确预期 Success、Fail 0、一个 native terminal、Fatal/Unhandled/Assertion/Ensure 0：`PASS Logs=11 RecordedSuccess=1219`，SHA-256 `9E66FE4A5FE3C2D6883BD8428AA4AE7A43A1AB85AF6C3C626ACB5D095474CB95`。

## 8. 首次失败、改动门禁与边界

首次 item exact 运行在测试 helper 的 `check(Execution.TryLaunchArc(...))` 停止。原因是测试定义仍保留直线的 800 速度上限，而 Arc plan 使用 1200 的规划 envelope；生产 execution 正确拒绝了不匹配配置。只把 Arc 测试定义改为 1200，Straight 继续为 800。首次失败日志已保留，SHA-256 `753590131BAC9619F89A93FE0273163BCFC871C5AA3F83705DF5AFF633DEE2A8`。

现有 regression map 已覆盖本轮全部 8 个改动文件，无需新增主题规则。真实 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=6 Logs=6
```

- gate SHA-256：`438BAD4A5A8EF45394E14C4B04D8BF1CD51233AE5B64126A20992D68407E68AC`；
- production boundary：`PASS Files=6 Matches=0`，SHA-256 `E4532B66C19ADE4900DA03827062B379421B475A9555E739BEB25590BEA9E650`；
- `git diff --check`：native 0，记录 SHA-256 `60B5D71EC66726A061B4F38FBDA46793ECB16D4AE1BD9765A9669873DB847733`。

## 9. 构建与产物

- 首次 Editor：43 actions / 74.85s / succeeded，SHA-256 `8A29D81C137F55471C436898A614EB462841190272785D260D3932EA8FCF02A7`；
- fixture 修正后 Editor：4 actions / 6.59s / succeeded，SHA-256 `59F51FA68AB70F308FCFDB459D4F01CA6967C4BC5456F933C56DB0BBC5553288`；
- Game final：40 actions / 69.98s / native 0，SHA-256 `52EDAA7BE2D5A465D3C8BB784C27EE2FABC216C68509BCCD483E797820B82AA1`；
- Editor final：up to date / 1.10s / native 0，SHA-256 `7E27B8E0D9DB6CD342E01DE6B2173D0C0BDEF861EC13A302373F6AD8DA05D55D`。

产物：

- `demo_map.exe`：357,086,208 bytes，SHA-256 `7CDEC05311EFA74AC7150B8B530E9F4CCDD19EB5B8FAF916CC624A4D01159211`；
- `UnrealEditor-demo_map.dll`：15,739,904 bytes，SHA-256 `E2D6DFDD8A1535EFD84D5B86A91F9D20DA65E7E429D6BC06B3C6A5CA62318459`。

## 10. P/F 边界与下一步

P20.2 证明的是“同一持久物品与 World carrier 如何发布直线或弧线 receipt”。它没有修改 RunHost 的 straight-only route，也没有解决现有 host 以 `MaximumDistance / Speed` 推导 lifespan 的直线假设。

建议 P20.3 为 RunHost 增加明确的 Arc route，并以 receipt 的 `FlightTimeSeconds` 建立终止上限；不得把弧线塞进直线 distance/speed lifespan。产品控制器、命令路由、滚轮弧度输入、可视轨迹与实际场景越障仍应后置到各自明确阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-2-thrown-weapon-arc-world-delivery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-2-thrown-weapon-arc-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P20.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-2-thrown-weapon-arc-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P20.2.r0_log.md>
