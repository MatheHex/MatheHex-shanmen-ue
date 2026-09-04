# Dev.D.UE.0.0.10.P20.2.r0 Development Log

## 1. 目标与基线

- 基线：`16e960a7fb34099c7a50976fea8c0e25e8c65df9`（P20.1）；
- 分支：`agent/0.0.10-p20-2-thrown-weapon-arc-world-delivery`；
- 目标：让 P7 既有 durable item gate、World adapter 与 projectile carrier 消费 P20.1 的统一 straight/arc launch receipt；
- 约束：不新建第二暗器权威，不修改 RunHost、命令路由、产品控制器、输入、伤害公式或碰撞决策。

## 2. 接入前审计

P20.1 已把 Arc plan 接入唯一 `FShanmenThrownWeaponExecution`，但产品层仍有三个 straight-only 假设：

1. item prepare purpose 固定为 `Shanmen.ThrownWeapon.StraightLaunch.r1`；
2. World adapter 只调用 `TryLaunchStraight`；
3. projectile 把 receipt direction 乘 speed，并把 gravity 留为零。

原 World pipeline 已正确实现 prepare → launch proof → emission → item commit request → inert staging → durable commit → publication，且 contact 与 termination 都回到唯一 execution。结论是扩展这条管线，而不是新增 Arc actor/adapter。

## 3. Action 对应的 durable purpose

item adapter 新增内部 exact mapping：

- Straight action → `Shanmen.ThrownWeapon.StraightLaunch.r1`；
- Arc action → `Shanmen.ThrownWeapon.ArcLaunch.r1`；
- 其它 action → fail closed。

prepare request 和 existing-prepare replay 都使用映射后的 purpose。commit 仍从 launch receipt 核验 action/activation/item/Run，Arc 不增加独立库存 API，也不直接操作 Runtime 或 legacy inventory。

## 4. 共享 staging 实现

把原直线 staging 的完整验证与计划构造抽为内部 `StagePreparedLaunchImpl`。它接收一个受限 launch 操作：

- 旧 `StagePreparedLaunch` 传入 `TryLaunchStraight`；
- 新 `StagePreparedArcLaunch` 传入 `TryLaunchArc` 和不可变 `FShanmenThrownWeaponArcPlan`。

除该调用外，两条路线执行完全相同的 correlation、preparation、runtime、execution、emission、commit-request 与 projectile-stage 检查。原 execution 只在 durable publication 成功后被 candidate 替换；staging 失败不会污染 authoritative execution。

## 5. Motion config 投影

projectile 内部新增 `FThrownWeaponMotionConfig`，只从有效 launch receipt 和当前 World 推导：

- `InitialVelocity`：两种轨迹都直接取 receipt；
- `InitialSpeed`：取 receipt speed；
- Straight `MaxSpeed=LaunchSpeed`、`GravityScale=0`，且 receipt gravity 必须为零；
- Arc `MaxSpeed=0`，保持下降段自然加速；
- Arc `GravityScale=PlanGravityZ / WorldGravityZ`。

Arc 的 plan gravity 必须有限、X/Y 为零、Z 向下；World gravity 必须有限且向下，所得 scale 必须为有限正 float。Arc 没有 World gravity authority时拒绝 staging。

`TryStageLaunch` 还要求 source 与 carrier 处于同一 World，设置 location/rotation、冻结 velocity/speed/gravity 后保持 collision off 与 movement inactive。`ActivateCommittedLaunch` 重放同一 frozen config 并激活。取消 staging 时完整清空运动配置。

## 6. 状态一致性

`IsStagedFor` 核对完整 initial velocity、speed、max speed 与 gravity scale。`IsInFlightFor` 核对 immutable speed/gravity 配置，但不把受重力持续改变的实时 velocity 误当作 receipt 漂移。

既有 contact delegate、Impact ledger、damage resolver、no-impact finish 与 spent-state 没有分叉。Arc action 通过 straight entrypoint 时由 execution 的 exact action boundary 拒绝。

## 7. 新增与扩展测试

Item suite 从 4 增至 5：

- 新 `ArcLaunchCommit` 验证独立 purpose、同一 exact intent、4→3 durable Quantity 和 repository invariant；
- helper 同时支持明确 Straight/Arc definition，Straight ceiling 仍为 800，Arc ceiling 为 1200。

World suite 从 3 增至 5：

- `ArcReceiptMotion` 在 transient UWorld 中生成真实 carrier，核验 initial velocity、gravity scale、unlimited MaxSpeed、durable publication 与原 no-impact termination；
- `ArcFailClosed` 核验 Arc 不能退回 straight staging，且没有 World gravity authority时 execution/emission/carrier 均不被污染。

原 3 项 straight World 测试和原 4 项 item 测试全部保留。

## 8. 首次失败与修正

首次 Editor 构建成功：43 actions / 74.85s。World exact 首次 5/5。

Item exact 首次在测试 helper 的 `check(Execution.TryLaunchArc(...))` 触发 assertion。Arc plan 的 maximum envelope 为 1200，而 helper 沿用了 Straight definition 的 800 ceiling；production execution 按 P20.1 契约正确拒绝。

修正范围仅为测试 fixture：Arc definition 使用 1200，Straight 继续使用 800。没有为了通过测试放宽 production validation。修正后 Editor 4 actions / 6.59s，Item exact 5/5。

- 首次失败 log：`automation_item_first.log`；
- SHA-256：`753590131BAC9619F89A93FE0273163BCFC871C5AA3F83705DF5AFF633DEE2A8`。

## 9. Changed-file regression gate

本轮改动 8 个文件：item adapter 2、projectile 2、World adapter 2、对应 tests 2。现有 map 中的 `ThrownWeaponItemAdapter`、`ThrownWeaponWorldDelivery` 和产品通用规则已完整覆盖，无需修改 map 或自测脚本。

门禁要求并取得 6 组证据：

- `Shanmen.0_0_10.CombatRuntime`；
- `Shanmen.0_0_10.Items`；
- `Shanmen.0_0_10.Product.CombatRunCoordinator`；
- `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter`；
- `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery`；
- `Shanmen.0_0_10.WorldGameplay`。

```text
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=6 Logs=6
```

gate SHA-256：`438BAD4A5A8EF45394E14C4B04D8BF1CD51233AE5B64126A20992D68407E68AC`。

## 10. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `automation_world_first.log` | 5/0 | `F3A86B50D05AEC93A67A48019C11F5D4CFEB89F5036BFFE33A19052037A68041` |
| `automation_item_final.log` | 5/0 | `887D22C80462A28B081F8A4B1D457F7A531CE4D2309F0893EEF2F48ECBC0FC68` |
| `automation_combat_runtime.log` | 140/0 | `3EB4F7BAA92D154D317B6416C09083BA7D52F9BFEB9C384A1927582E76099B15` |
| `automation_items.log` | 77/0 | `E5C7EA0D5CB2CF5C4F40F9BF1697AFD0C05C368CFCDD4867068EC88E812890C9` |
| `automation_world_gameplay.log` | 10/0 | `CDDC39A860F98AE0C9E8AC685D0FCC246EB65E7C8C6C1375AEBEDDF88AAB73F9` |
| `automation_combat_run_coordinator.log` | 18/0 | `258A72C8F6E2C3ECE052A4601FC965507FECDE0CFFC35B9704296ACF45DEA5E8` |
| `automation_full.log` | 848/0 | `88146C4DA243357A401C67853C1617A9D50F03553BD7E7FF30979CEF5C10A9F7` |
| `automation_legacy_attributes.log` | 4/0 | `168014C99CD2BC3D34D78D977690240ABB7723F35C2B331B564C9141CEB6673D` |
| `automation_legacy_enemy.log` | 44/0 | `B1EF2A9D1E29FDDCD7F1759F6646FFC00DD945FECBF3F7D0EEA72C5D630A6F6D` |
| `automation_legacy_v2_ranged.log` | 22/0 | `9BD4199FEAFCABF3A196A1FD8FAED6207A92F7EC315539DCA51BF9DF2D818E3E` |
| `automation_legacy_item_use_armor.log` | 46/0 | `872132A9297F970B7274C3ED0CCE123EA208F76777FDBCAF07DC0E831DC0D8F6` |

全量 canonical command 至 native terminal 约 33 分钟，848/848。约 692–708 项进入既有 Sword Rhythm checkpoint/manifest 长时单例；进程始终 Responding、CPU 持续增长并逐项成功，没有重启、跳过或用部分结果替代。

证据审计：

```text
EVIDENCE_AUDIT: PASS Logs=11 RecordedSuccess=1219
```

审计 SHA-256：`9E66FE4A5FE3C2D6883BD8428AA4AE7A43A1AB85AF6C3C626ACB5D095474CB95`。

## 11. 静态边界、构建与产物

对 6 个 production 文件扫描 direct damage、spawn/destroy、direct inventory mutation、RNG、homing enable/target、Enhanced Input、timer 与 Tick ownership：

```text
BOUNDARY_SCAN: PASS Files=6 Matches=0
```

- boundary SHA-256：`E4532B66C19ADE4900DA03827062B379421B475A9555E739BEB25590BEA9E650`；
- `git diff --check` native 0，记录 SHA-256 `60B5D71EC66726A061B4F38FBDA46793ECB16D4AE1BD9765A9669873DB847733`。

构建：

- first Editor：43 actions / 74.85s / succeeded / `8A29D81C137F55471C436898A614EB462841190272785D260D3932EA8FCF02A7`；
- after-fix Editor：4 actions / 6.59s / succeeded / `59F51FA68AB70F308FCFDB459D4F01CA6967C4BC5456F933C56DB0BBC5553288`；
- final Game：40 actions / 69.98s / native 0 / `52EDAA7BE2D5A465D3C8BB784C27EE2FABC216C68509BCCD483E797820B82AA1`；
- final Editor：up to date / 1.10s / native 0 / `7E27B8E0D9DB6CD342E01DE6B2173D0C0BDEF861EC13A302373F6AD8DA05D55D`。

产物：

- `Binaries/Win64/demo_map.exe`：357,086,208 bytes / `7CDEC05311EFA74AC7150B8B530E9F4CCDD19EB5B8FAF916CC624A4D01159211`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,739,904 bytes / `E2D6DFDD8A1535EFD84D5B86A91F9D20DA65E7E429D6BC06B3C6A5CA62318459`。

## 12. P/F 边界与后续判断

没有运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P20.2 没有修改 straight-only RunHost。当前 host 的 lifespan 仍来自 `MaximumDistance / Speed`，不能直接用于 Arc。建议 P20.3 增加 typed Arc host route，以 receipt 的 `FlightTimeSeconds` 驱动有界终止，并继续复用当前 spawn、publication、contact 和 cleanup。之后再由独立阶段接入 controller/router/滚轮弧度输入与可视反馈。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-2-thrown-weapon-arc-world-delivery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-2-thrown-weapon-arc-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P20.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-2-thrown-weapon-arc-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P20.2.r0_log.md>
