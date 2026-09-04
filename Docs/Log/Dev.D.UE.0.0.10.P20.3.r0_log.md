# Dev.D.UE.0.0.10.P20.3.r0 Development Log

## 1. 目标与基线

- 基线：`e0edf8d9ca8d4799faa8a5bd45dadc4d0a20ebd8`（P20.2）；
- 分支：`agent/0.0.10-p20-3-thrown-weapon-arc-run-host`；
- 目标：让现有 RunHost 接受 P20 Arc plan，并以 receipt 的精确飞行时间管理 Actor 寿命；
- 约束：不建立第二 Actor、库存、伤害、碰撞或生命周期权威，不接产品 controller/input/UI。

## 2. 接入前审计

P20.2 已让 item/world carrier 发布统一 Straight/Arc launch receipt，但 RunHost 仍有两个直线假设：

1. 所有启动入口都要求 `AimDirection + MaximumDistance`；
2. Actor lifespan 固定为 `MaximumDistance / LaunchSpeed`，寿命回调一律记作 `RangeExpired`。

弧线 receipt 已携带求解器的精确 `FlightTimeSeconds`。若继续套用直线公式，目标高度、重力和 apex 对飞行时间的影响会被丢失。结论是扩展原 Host，而不是在 controller 或 Actor 中重算弹道。

## 3. Lifetime value contract

新增 `Edemo_mapShanmenThrownWeaponHostLifetimeKind` 与 `Fdemo_mapShanmenThrownWeaponHostLifetime`。

`TryCreateRange` 只接受 Straight receipt，验证距离和 speed 后冻结距离及换算后的 Actor lifespan。`TryCreateArc` 只接受 BallisticArc receipt，冻结 receipt 的 flight time 及其 float lifespan 投影。

`IsValidFor` 会针对当前 launch receipt 重建期望值并逐字段核对。Host `IsValid` 和 adoption binding 都使用这项检查，避免恢复路径绕过轨迹判别。

## 4. 共享 Host 管线

新增三个 Arc public seam：spawn-and-launch、launch-existing-carrier、adopt-published-flight。Straight API 保持原签名。

内部只保留一套：

- Empty/HostBusy gate；
- action、execution、prepared item、source 与 coordinator 绑定；
- execution candidate staging；
- durable item commit；
- published flight adoption；
- delegate binding、terminal publication 与 cleanup。

轨迹差异通过受限 staging operation 与 immutable lifetime 参数注入。Arc 调用 `StagePreparedArcLaunch`，Straight 调用原 `StagePreparedLaunch`；两者的 commit 和 adoption 顺序相同。

## 5. 寿命回调与终态

Host-owned carrier 在 adoption 后使用 lifetime 的 `ActorLifeSpanSeconds` 调用 `SetLifeSpan`。

保留 projectile 的唯一 lifespan delegate。回调读取冻结的 lifetime kind，将 Straight 映射为 `RangeExpired`，Arc 映射为新增的 `FlightTimeExpired`。两种终态都调用同一 no-impact completion：execution 进入 `Spent`，Action 经过 Recovery 到 Completed，terminal receipt 保留 launch identity。

`TryExpireRange` 与 `TryExpireFlightTime` 都有轨迹 gate；错误轨迹或已终止 Host 返回 false。

## 6. 自动化扩展

在已有真实持久 authority 的 Router fixture 中新增 `ArcFlightTimeLifecycle`：

1. 创建 Arc action、definition、execution 与 solver plan；
2. durable prepare 后故意调用 Straight Host API；
3. 验证 staging 拒绝、Host 仍 Empty、authority snapshot 不变；
4. 用同一 prepared intent 调用 Arc Host API；
5. 验证持久 Quantity `3 → 2`、Host/receipt 为 BallisticArc、lifespan 等于 solver flight time；
6. 验证 Arc 拒绝 range expiry；
7. 触发唯一 lifespan delegate，验证 `FlightTimeExpired`、execution Spent、Action Completed、0 accepted impacts；
8. 验证终态不能重复执行。

原 Straight RunCommand 测试增加 lifetime 判别和错误 Arc-expiry fence。RunHost suite 由 3 增至 4，全量由 848 增至 849。

## 7. 首次结果与修正记录

首次 Editor 构建：37 actions / 66.90s / succeeded。首次 RunHost exact：4/4，原生退出码 0。

没有 production 或 test failure。实现审读后仅把新 getter 从易误解的 `GetMaximumFlightTimeSeconds` 改为准确的 `GetFlightTimeSeconds`；随后对最终源码重新编译并重跑 exact、全量与兼容回归。没有删除或覆盖失败日志来美化结果。

## 8. Changed-file regression gate

最终改动 3 个文件：RunHost header/implementation 与 RunCommandRouter tests。现有 map 命中 `ThrownWeaponRunHost` 和 `ThrownWeaponRunCommandRouter` 两条规则，无需修改映射。

要求组：

- `Shanmen.0_0_10.Product.ThrownWeaponRunHost`；
- `Shanmen.0_0_10.Product.ThrownWeaponRunCommand`；
- `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery`；
- `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter`；
- `Shanmen.0_0_10.Product.CombatRunCoordinator`；
- `Shanmen.0_0_10.Items`；
- `Shanmen.0_0_10.WorldGameplay`；
- `Shanmen.0_0_10.CombatRuntime`。

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=8 Logs=2
```

最终 gate 使用 RunHost exact 与最终 `Shanmen.0_0_10` 全量日志，SHA-256 `03586DCF73F15B012F3F4EE1088503EACE49E3EC3BCE4B8F9476F3840CA3DB18`。

## 9. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `runhost_final.log` | 4/0 | `6E4E9DA2D814C9A027A6C42705819E5BC627D1EDE366DA7FCDC3F52BCBDCDDF5` |
| `full_0_0_10_final.log` | 849/0 | `1FB196755D6AAB2DC8BFA11B4D89A46541E8F7B7EE86ECE7DF725F74A5F1E4D6` |
| `legacy_v3_attributes_final.log` | 4/0 | `F2CBDE93E7452CA5DAF5A00EA7CAE82E4D1905F32C5339A5CFD4F592B6D7126A` |
| `legacy_enemy_skill_final.log` | 44/0 | `CE31CFA5F93AAFC88111CF7372E056F3BAC989CE82F51C167F6D7F85017DDD56` |
| `legacy_v2_ranged_final.log` | 22/0 | `180951B270A0A65A2EF37EADFCB774B074F7E2C6A6833B8B68F455851BFD90E3` |
| `legacy_item_armor_final.log` | 46/0 | `5C9F7CDFBE1D1F1ED26BDFE1942EB2CF0AF385EB94B78939B55FC4E00930D2EF` |

全量 canonical command 从发现 849 项到 native terminal 约 33 分 30 秒。约 632–708 项进入既有 Sword Rhythm retry/checkpoint/manifest 长时单例；日志持续产生开始/完成记录，所有项目最终成功，没有重启、裁剪或用部分结果代替。

```text
EVIDENCE_AUDIT: PASS Logs=6 RecordedSuccess=969
```

审计 SHA-256：`37652C0C2EE6A5C0C882AA94D9A364BAF8E38980DFAE142F567FD990BF0A23EE`。

## 10. 静态边界与构建

production boundary：

```text
BOUNDARY_SCAN: PASS Files=2 Matches=0
```

- boundary SHA-256：`E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`；
- `git diff --check`：PASS / native 0，记录 SHA-256 `E2A7201E00C37A4AFB08A769D2961539AE404FAECE0CE0043F1206019DE80640`。

构建：

- first Editor：37 actions / 66.90s / `1DA451285CBB8DC1E154D5DDF917D608C603712F08E06C67441F67CACEE6C02A`；
- final Editor：37 actions / 47.88s / `DB1BFFE28136F31E26EF2D65DEB17A538422EDE99085C87D426103DD2BA6BD86`；
- final Game：36 actions / 65.51s / `23ECF45E9838FA810CB42C87E9E1628D000F4DF4480AED706472E60B45F1DE42`。

产物：

- `Binaries/Win64/demo_map.exe`：357,097,984 bytes / `E9B6D4A7F78062D3A4E9757D60706D692B11F71A20D828D9E1803BCE47C5B258`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,753,216 bytes / `089B960B77614C2EEA779F25A7E57C32558695C4822F20B294301BA96180D289`。

## 11. P/F 边界与后续判断

没有运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

当前 Arc 只能由已持有 plan 的调用方进入 RunHost；现有 `Fdemo_mapShanmenThrownWeaponRunCommandRouter` intent 仍只有 origin、aim direction 与 maximum distance。下一阶段应新增 typed Arc command contract，冻结目标/plan 身份并调用本轮 Host API；不得让 Straight command 隐式推断弧线参数。controller、session、lifecycle、滚轮弧度输入和视觉反馈继续分阶段接入。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-3-thrown-weapon-arc-run-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-3-thrown-weapon-arc-run-host/Docs/Report/Dev.D.UE.0.0.10.P20.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-3-thrown-weapon-arc-run-host/Docs/Log/Dev.D.UE.0.0.10.P20.3.r0_log.md>
