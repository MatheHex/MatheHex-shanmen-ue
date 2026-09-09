# Dev.D.UE.0.0.10.P22.11.r0 Report

## 1. 结论

P22.11 在 P 阶段边界内完成，结论为 **PASS**。

本轮补齐 P22.9/P22.10 的玩家反馈闭环：当飞刀因释放终点或 Source-to-Origin 短距走廊被阻挡而拒绝暂存时，MainHUD 现在显示唯一、可证明的提示 `飞刀 · 释放路径受阻`。该提示只来自底层精确错误，不会把输入锁、空槽、无效请求、协议错误或一般暂存失败误报成墙体阻挡。

```text
Initial Editor Development build:       PASS (121 actions / native 0)
Focused launch/HUD/world/lifecycle:       19 Success / 0 Fail
Changed-file mapped regression:        1423 Success / 0 Fail (4 healthy logs)
Changed-file regression coverage:       PASS (16 files / 5 rules / 28 groups)
Regression gate self-test:               PASS 439/439
Game + Editor Development:               PASS (both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明类型化拒绝、真实 World 几何门、零提交边界和 HUD 只读投影，不宣称提示时长、字体观感或全部关卡夹缝已完成人工体验验收。

## 2. 玩家侧变化

此前贴墙或隔墙释放会被正确拒绝，但 HUD 没有说明原因。现在流程为：

```text
释放体积/走廊清空
  -> 暂存飞刀 -> 提交物品 -> 发布飞行

释放体积/走廊受阻
  -> 拒绝暂存 -> 不扣物品 -> 不发布飞行
  -> MainHUD：飞刀 · 释放路径受阻
```

提示会在下一次热栏路由结果到来时被替换，并在产品生命周期结束时清空；没有定时轮询或独立 UI 状态机。

## 3. 精确错误契约

Projectile 暂存新增只读输出 `Edemo_mapShanmenThrownWeaponProjectileStageError`：

- `ContractRejected`：输入、状态或暂存契约不成立；
- `ReleasePathBlocked`：唯一表示终点 overlap 或短距 corridor sweep 命中 blocking geometry；
- `None`：暂存成功或精确重放成功。

World Adapter 只把 `ReleasePathBlocked` 映射为同名 `Edemo_mapShanmenThrownWeaponLaunchError`；其余暂存失败继续使用 `ProjectileStageRejected`。因此表现层无需分析诊断字符串或猜测失败原因。

## 4. 权威拒绝链

HUD 投影要求下列真实证据同时成立：

```text
Session.ProductRejected
  -> Product.RouterRejected + captured action identity
  -> Command.LaunchRejectedCancelled | RecoveryRequired
  -> HostStart.LaunchRejected
  -> Launch.ReleasePathBlocked
  -> HostStart was never started
```

RunId、SelectionId、ItemInstanceId、ActivationId 必须跨 Session/Product/Command 完全一致。任何字段缺失、身份漂移、已启动 Host、一般 `ProjectileStageRejected` 或已接受结果都会失败关闭并清空输出。

## 5. 生命周期与事务边界

`Fdemo_mapShanmenThrownWeaponProductLifecycle` 只保留最新一份不可变热栏/恢复结果，供 HUD 读取；它不是第二个玩法权威：

- 每次热栏提交或取消恢复后覆盖为该次完整结果；
- 新生命周期首次绑定时清空；
- 生命周期结束或空状态重放时清空；
- 不序列化、不轮询、不驱动物品或动作状态。

几何拒绝仍发生在 Projectile 状态写入和 durable item commit 之前。失败时 Carrier 保持 Empty/inert，物品数量不变，飞行不发布。

## 6. HUD 合成

新增 renderer-neutral `Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation`，只产生文本和身份快照。MainHUD 先尝试投影精确 launch rejection；若没有该证据，才读取既有 terminal receipt。

Hint Stack 新增 `LaunchRejection` 行，固定使用既有 `Blocked` tone。Launch rejection 与 terminal feedback 同属唯一 outcome 槽，二者同时有效时合成失败关闭，避免同屏出现“释放受阻”和“命中/过期”等冲突结论。Straight 与 Ballistic Arc 共用同一合成路径。

## 7. 自动化证明

四个聚焦组共 `19/0`：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `ThrownWeaponLaunchRejectionPresentation` | 3 | 265,072 | `580AAB643273507573A8F7AA7676FEB6684597137B00E70434279AAA4698C3E1` |
| `ThrownWeaponMainHUDCombatHintStackPresentation` | 3 | 265,117 | `705CFD8153F2D8EC8A86BFFC71F940173743859A26DEDC9918CB44A3FFDCA6C7` |
| `ThrownWeaponWorldDelivery` | 8 | 270,993 | `C890CA143D54D56A1C7D1054DB3CD1044CA969E5BC9CF0364D8264994A901A34` |
| `ThrownWeaponProductLifecycle` | 5 | 268,902 | `B14D642264F66BDDF3139702E108789765D71920479F2B8840B48F0EEF0E9850` |

覆盖内容包括确定性文本、恢复证据、一般暂存错误隔离、Straight HUD 行、真实 World 终点/走廊阻挡、零提交、最新路由留痕、精确重放、冲突覆盖和结束清理。本轮首次构建与所有聚焦测试一次通过，没有需要隐藏或覆盖的首轮失败。

## 8. 改动文件回归与构建

最终四份映射日志合计 `1423 Success / 0 Fail`：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10` | 1,254 | 1,916,979 | `191D079BD1E1DD210D1D84CA5F052DBDA6AA53473094F3BD6380D452B94AB57E` |
| `demo_map.InputRestore` | 101 | 395,559 | `3887EF1BEA946E78CAFC0F8C508EDEE4944DE13FD0096A26F855BFF35B0039E2` |
| `demo_map.V2RangedCompatibility` | 22 | 285,575 | `39AB513B5F3203202989A2C59E56DB5B257D562EC581F8305704F26533FDF8C9` |
| `demo_map.ItemUseAndArmor` | 46 | 309,964 | `35C9BFD100E1281EFD5B4D8DDF0075B9A17F4110B23B3BAAD237C5142511C1F1` |

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.11_regression_coverage.log` | PASS 16/5/28/4 | 4,179 | `5565F45ABE6B58D4FDD9DD98FAEABA0982205FEE00D4B06471D655F8116D06C3` |
| `P22.11_regression_coverage_selftest.log` | PASS 439/439 | 43,303 | `7B0B3AA9A77B556B23D3A7AD8FFE70C8C05D3E1A3990B99963FEF1DF5EDB0C87` |
| `P22.11_GameBuild_final.log` | PASS / native 0 / 390.62s | 13,134 | `E9C4B7CC1686E0E85B869F37EB7B7AB4A6DF3F8E474860F6AD0DFEDF59934BA4` |
| `P22.11_EditorBuild_final.log` | PASS / native 0 / 1.23s | 1,035 | `8A856D971E005B966FE5A47F8001884F06DBDA3FABF1AC3362CDF1C23350AB67` |

产物：`demo_map.exe` 为 359,567,360 bytes，SHA-256 `27705FE11ABA349E69CA48C21E2C55E9FEB9CF44A7B9E476CB95B850AE89A73D`；`UnrealEditor-demo_map.dll` 为 18,806,272 bytes，SHA-256 `3B2C455AA00AEDFEF58940587026D06AF610DFBF34B4BD8085214DDE96130D73`。

## 9. P/F 与静态边界

PASS：两类真实释放几何阻挡均映射为精确错误；一般契约拒绝不会伪装成墙体；HUD 显示唯一 blocked outcome；失败不扣物品、不发布飞行；1,423 项映射回归零失败；覆盖门、自测和双构建通过。

未声明：自动寻找替代发射点、提示自动消隐动画、骨骼 Socket、网络复制、动态群体阻挡或人工手感验收已经完成。

非文档改动为 11 个生产文件 `+308/-24`、4 个测试文件 `+220/-2`、1 个回归映射文件 `+9/-0`。新 presentation 文件中的 Timer、SetTimer、自定义 Tick、Sleep、Random/Rand/RNG、UWorld、AActor、UPROPERTY、UFUNCTION 均为 0；没有新增资产、模块、Actor、Subsystem、持久格式或第二套权威；Regression Map JSON 可解析；`git diff --check` 为 0。

## 10. 提交边界与 GitHub

本阶段只提交 11 个生产文件、4 个测试文件、1 个回归映射文件、本 Report 与本 Development Log，共 18 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.11` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-11-thrown-weapon-release-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-11-thrown-weapon-release-feedback/Docs/Report/Dev.D.UE.0.0.10.P22.11.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-11-thrown-weapon-release-feedback/Docs/Log/Dev.D.UE.0.0.10.P22.11.r0_log.md>
