# Dev.D.UE.0.0.10.P19.8.r0 Report

## 1. 结论

P19.8 已完成 Run-scoped Divine Sense Product Route。

新路由是无状态产品组合缝：调用方只能表达一次“使用神识”，并提供既有 SpiritEnergy 开场快照、当前 World/source 与显式 subject Actor 批次；不能注入产品 config、Action、ActivationId、IntentId、scan ordinal、subject budget 或 ControllerId。路由内部只使用 P19.7 canonical authority 取得身份，再委托 P19.6 Controller 执行。

每个可观察的现场结构错误都在 Run 激活序列消费前失败关闭。现场 evidence 暂时不可用时，路由返回私有字段的不可变 use attempt；重试复用同一 Intent 和 command，不申请第二个身份。已接受尝试的精确回放继续保持 P19.6 的无 World、Actor、provider 读取保证。

本轮没有新建资源、World observation、扫描、receipt、journal、replay 或生命周期权威。Controller 仍是唯一 Run-scoped 状态所有者，路由自身不持有状态。

本轮为 P 阶段。没有接物理输入、UI、隐式 Actor 搜索或最终平衡，也没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 基线与分支

- 基线提交：`928abd8fcac57a23ffbf9e469ff448e04bc309db`（P19.7）；
- 分支：`agent/0.0.10-p19-8-divine-sense-product-route`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 新增与修改

新增：

- `demo_mapShanmenDivineSenseProductRoute.h`：125 行；
- `demo_mapShanmenDivineSenseProductRoute.cpp`：450 行；
- `demo_mapShanmenDivineSenseProductRouteTests.cpp`：721 行。

修改：

- `ShanmenRegressionMap.json`：增加 Product Route 的 13 组直接证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：增加完整正例与缺证失败关闭反例。

## 4. 单一产品入口

`Fdemo_mapShanmenDivineSenseProductRoute` 暴露四个同步入口：

1. `TryBegin`：内部创建 canonical config，只将既有 SpiritEnergy 开场快照交给 Controller；
2. `TryUse`：完成可用性与现场预检，向 P19.7 请求一次身份，再委托 Controller；
3. `TryRetry`：只接受路由签发的 immutable attempt，不请求新身份；
4. `TryEnd`：内部使用 Controller 自身身份 teardown，调用方不能传入 ControllerId。

开场 SpiritEnergy 快照仍来自既有资源权威；路由不发明余额，不读 UI，也不保存第二份资源状态。

## 5. 身份消费前的失败关闭

`TryUse` 在 `PrepareIntent` 之前检查：

- Controller active、内部有效且绑定 exact canonical config；
- Combat Run ready，并与 Controller 共享 RunId 和 player entity；
- Controller 尚有 Intent 容量且当前 SpiritEnergy 可支付 canonical cost；
- World/source live、同 World、位置有限，并且 source 精确解析为当前 Run player；
- subject 数量不超过 canonical budget 32；
- 每个 subject live、同 World、位置有限、已注册、stable entity 唯一且不是 source entity。

这些检查只验证身份与结构，不读取 gameplay tags、LOS 或其它 evidence。测试证明 null World、错误 source、自目标、重复、未注册、跨 World 与超预算批次全部不推进 `NextPlayerDivineSenseActivationSequence`，不捕获 Intent、不扣资源、不调用 provider。

## 6. Immutable attempt、重试与回放

`Fdemo_mapShanmenDivineSenseProductUseAttempt` 的写字段全部私有，仅 Product Route 可写。它绑定 ControllerId、RunId、source entity、canonical ConfigId 与完整 P19.7 prepare proof，并在 `IsValid` 中重验这些关系。

首次 World/provider 路由失败后，P19.6 Controller 已保留同一 frozen command，路由将该 attempt 返回给调用方。修复 provider 后，`TryRetry` 使用同一 IntentId、ActivationId 和 RouteCommandId 成功；Run 序列不再推进，资源只提交一次。

已接受 attempt 的再次 `TryRetry` 可传 null World、null source、空 subject 与拒绝 provider，仍从 P19.6 journal 返回相同 proof，不发生 provider 读取、身份消费或资源变更。其它 Controller 或其它 Run 无法使用该 attempt。

## 7. Typed result 与所有权边界

Route result 区分 applied、replayed、Controller/Coordinator unavailable、Run mismatch、live-input rejection、availability rejection、authority preparation rejection、attempt rejection、Controller rejection 与 state desynchronization。

Accepted result 必须同时验证 immutable attempt 与 P19.6 Controller result 的 ControllerId、RunId、Intent 和 replay 状态。Provider 拒绝仍是有效的 typed failure，而不是丢失身份的布尔失败。

生产路由不枚举 Actor，不做 trace/sweep/overlap，不 spawn/destroy，不持有 input、UI、clock、Timer/Tick 或 RNG 状态。

## 8. 自动化结果

新增 exact tests 6 项：

- `CanonicalBegin`；
- `UseAndReplay`；
- `PreflightFences`；
- `RejectedRecovery`；
- `AvailabilityFence`；
- `CrossBindingAndTeardown`。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | `Product.DivineSenseProductRoute` | 6/0 | `21E6082E2FC1543380EAF8934D123E0DA0C8EF94B19AFD5DBE90A821C292E77E` |
| `automation_full.log` | `Shanmen.0_0_10` | 828/0 | `2A480D06E3A8DC4EBB19D7C2CBF276EA94D138072DCB89C928D6ABCAA774002E` |
| `automation_legacy_attributes.log` | `demo_map.V3.Attributes` | 4/0 | `77E0DC96465CC3F5997BF41E90A39F96766DC14FE77BEA126A1CC125772085A8` |
| `automation_legacy_enemy_skill.log` | `demo_map.EnemySkillFramework` | 44/0 | `A512410C4CF8C543A7F618381DDA8A224C27D66CD223094E50EA422065801D20` |
| `automation_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | `9637A24E0E48DA1888D54E918A6BEB50B3E761C2FE3545B1FFDBA307032BF531` |
| `automation_legacy_item_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `7AC9A524A19D38CFE84B8A550C3C75E7D22334BF4803DEBCC5E97654CCBEBB22` |

全量由 P19.7 的 822 增加到 828。六份正式日志均只有一个 canonical RunTests command、Fail 0、一个 queue-empty terminal、Fatal/Unhandled/Ensure 0。证据审计：`PASS Logs=6 RecordedSuccess=950`，SHA-256 `694EAC8D443E270DF9BE09CD6E21760387D130F2BE056AE4C3B08A849F26D3D6`。

## 9. 门禁、边界与构建

真实 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=13 Logs=6
```

- gate SHA-256：`60318BF25B35F883C76A7ABE323FBE3579963EDBD45938D1FF1DF2B9790CF248`；
- self-test：`305/305 PASS`，SHA-256 `0B55D8F60ADE6CC8EAB1A6C9F9451138CE84E05E46931E02B712092B721CC5F6`；
- production boundary scan：`PASS Files=2 Matches=0`，SHA-256 `E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`；
- `git diff --check`：native 0。

构建：

- Editor initial：5 actions / 21.15s / native 0，SHA-256 `1BCD91FC05A7D0941F4D0DBB9A1070AE3483623E1C834A3CDB5F557D9DEC3574`；
- Game final：4 actions / 23.76s / native 0，SHA-256 `CBF6CD63FA423C57AF2A3E11A6EC2733FDE2AC48A2DF9924BDBCB0C8C5231561`；
- Editor final：up to date / 1.04s / native 0，SHA-256 `429E1B4EDA66EC4A880B63731D8581121999AE38391B23AF381C668A21955252`。

产物：

- `demo_map.exe`：356,955,136 bytes，SHA-256 `E6D5792565A48FCD3FFE1826B5A3011717E012179271D8788CDA325FA2706423`；
- `UnrealEditor-demo_map.dll`：15,652,864 bytes，SHA-256 `73E3CF4016A14A873F436D64E1CC2C68D059F9236C12A1FE62EBEB18A0513563`。

## 10. 异常、P/F 边界与下一步

全量 828 项从首项到 queue-empty 用时约 33 分 46 秒。既有 Sword Rhythm checkpoint/envelope 测试在高提交内存环境中多次出现 16–97 秒的 Automation large-delta 提示；进程始终响应、CPU 持续推进，最终 828 Success / 0 Fail / native 0。该轮没有中断、重启、裁剪或把部分日志当成成功。

本轮没有运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P19.9 建议建立 Divine Sense logical input adapter：只把一次逻辑 use event 交给本路由，并明确 busy/availability/retry 行为；物理键位、UI 与自动 Actor discovery 继续留到后续阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-8-divine-sense-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-8-divine-sense-product-route/Docs/Report/Dev.D.UE.0.0.10.P19.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-8-divine-sense-product-route/Docs/Log/Dev.D.UE.0.0.10.P19.8.r0_log.md>
