# Dev.D.UE.0.0.10.P22.8.r0 Development Log

## 1. 基线与目标

- base：`2f3c882246e9d09c596ef2e55c886d587efac64b`（P22.7 thrown-weapon edge cue）；
- branch：`agent/0.0.10-p22-8-thrown-weapon-hand-release`；
- 目标：让直线与 Arc 飞刀从角色右手侧代理点释放，而不是从角色中心上方释放；
- 边界：不改轨迹参数、碰撞、伤害、库存、Run、终态或投射物表现，不启动 UI/PIE/产品 executable。

## 2. 链路审计与取舍

既有直线路由在 `RouteTypedHotbarInput()` 中构造 `Origin`，Arc 预览则在 `Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter::Sample()` 中独立构造只抬高 `50 cm` 的 `Origin`。两处重复使后续手侧偏移容易漂移。

选择把释放点收敛到 InputAdapter 的一个纯函数。它只依赖源 Transform，不依赖 World、骨骼、Socket、动画或库存权威；直线命令与 Arc source basis 都调用它。这样不增加 wrapper、Actor 或状态所有者，也让预览和发射天然一致。

## 3. 几何契约

```text
LaunchOrigin = Location
             + UnitForward * 55.0
             + UnitRight   * 28.0
             + WorldUp     * 50.0
```

Forward/Right 使用 `FTransform::GetUnitAxis()`，因此角色缩放不会放大偏移；垂直量使用 `FVector::UpVector`，不随角色俯仰/翻滚倾斜。该点是稳定代理位置，不冒充骨骼 Socket。

## 4. 生产改动

`demo_mapShanmenThrownWeaponInputAdapter.h/.cpp`：

- 新增 forward/right 常量与 `MakeLaunchOrigin()`；
- `RouteTypedHotbarInput()` 从源 Actor 采样一个 Transform，并据此产生唯一 Origin；
- 其余直线/Arc 分支继续消费既有输入、身份和生命周期接口。

`demo_mapShanmenThrownWeaponArcSourceBasisAdapter.cpp`：

- 删除本地 `Location + (0,0,50)` 公式；
- 使用共享函数建立 Arc basis；
- 保持一次 Transform 采样、一次 basis 捕获以及既有失败关闭计数不变。

## 5. 测试改动

`demo_mapShanmenThrownWeaponInputAdapterTests.cpp`：

- 新增固定 Transform 的 `HandReleaseOrigin`；
- yaw `0°` 与 `90°` 分别验证 `(155,228,80)`、`(72,255,80)`；
- 直线路由读取捕获命令并验证 Origin；
- Arc 路由读取捕获 ArcRequest 并验证 Origin；
- Arc 失败关闭夹具使用实际源 Transform 推导输入原点。

`demo_mapShanmenThrownWeaponArcSourceBasisAdapterTests.cpp`：

- 更新两个方向的 source-basis 原点；
- 更新委托目标，使目标继续由“共享 Origin + 既有投影”构成；
- 保留变换采样次数、稳定身份与失败关闭断言。

## 6. 首轮失败闭环

首次 Editor 构建完成 `52 actions / native 0`。首次 InputAdapter 专项为 `7/3`，失败集中在三个依赖通用 `APawn` 夹具位置的测试。通用 Pawn 没有 RootComponent，测试设置的硬编码位置没有成为实际 Actor Transform，导致新增路由 Origin 断言与 Arc 几何输入不一致。

生产纯函数固定 Transform 测试已经通过，故没有修改生产公式。测试期望改为从夹具 Actor 的实际位置、Forward 与 Right 独立计算；修复构建 `4 actions / native 0`，最终同组 `10/0`。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.8_editor_build_initial.log` | 52 actions / PASS / native 0 | 6,162 | `85E1788C4345828B6AE70AED5CA2A7D672E573A625E0FE62BF342730523B2E24` |
| `Focused-ThrownWeaponInputAdapter.log` | 7/3 / native 255 | 276,293 | `865FE1E8D57DD38780BF35EE17078E91AD51A9E7FF64E52C115AA708B62F877F` |
| `P22.8_editor_build_repair.log` | 4 actions / PASS / native 0 | 2,484 | `EDDB7CDF4B2D4507FABEBEC26584D0A33CB1AC2E58AFF63B3B72DEDC138BEC16` |
| `Final-ThrownWeaponInputAdapter.log` | 10/0 / native 0 | 274,135 | `74807153D0CE0CA8229C193669DD05C47EC5D4911D05E0981B6F4BFEAAC7993E` |

## 7. 改动文件映射

Arc source-basis 映射改为五个精确组：

- `ThrownWeaponArcSourceBasisAdapter`；
- `ThrownWeaponArcChoiceInputComposition`；
- `ThrownWeaponArcChoiceProjection`；
- `ThrownWeaponInputChoice`；
- `ThrownWeaponInputAdapter`。

InputAdapter 的既有规则继续要求 lifecycle、session、controller、Run command/host、World delivery、Item adapter、CombatRun、Items、WorldGameplay、CombatRuntime 与 ItemUseAndArmor。两个规则去重后为 17 个组。

完整 `Shanmen.0_0_10` 广域运行发现 1,249 项；在无关 SwordRhythm 长耗时链中运行至 `709/0` 后有界停止。该日志没有队列完成标记，不作为成功证据。精确映射日志均自然完成，最终合计 `401/0`。

## 8. 覆盖门与构建

覆盖脚本自测为 `439/439`。开发自测期间修正了两处映射夹具：先恢复误触的 Hotbar 规则，再以五份精确 synthetic logs 代替不能满足 delimiter-aware 前缀匹配的 umbrella fixture。最终映射只移除 Arc source-basis 规则中的冗余整套要求。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `Mapped-Shanmen-0_0_10.log` | bounded diagnostic / 709/0 | 1,188,660 | `97CC562B93205A38F4B70F586B289408D4D015A3867772D27C25425714E0F784` |
| `P22.8_regression_coverage.log` | PASS Changed=7 Rules=2 Required=17 Logs=17 | 5,298 | `7A1DB514A7C77B0B9EEBA9B88F98F89862BB2E9B5CA5AFDCEB6389C9D9CD365A` |
| `P22.8_regression_coverage_selftest.log` | PASS 439/439 | 43,303 | `7B0B3AA9A77B556B23D3A7AD8FFE70C8C05D3E1A3990B99963FEF1DF5EDB0C87` |
| `P22.8_game_build_final.log` | 51 actions / PASS / native 0 / 195.31s | 5,938 | `5AAF5C58AEC27CB97B003677FE4AB0D05EAA3C5D01B8201A8C28FC54A4A7321E` |
| `P22.8_editor_build_final.log` | 0 actions / PASS / native 0 / 0.97s | 1,035 | `DCAE37223B945D2BE10C620DB1F65E03AE59C7006C838798B1B9754117321EDB` |

产物：`demo_map.exe` 为 359,574,528 bytes，SHA-256 `13B9E39D890AFA4C174A4F3D9AFCD418F9168F504D666ED948C550835BAAA91C`；`UnrealEditor-demo_map.dll` 为 18,782,720 bytes，SHA-256 `EF862E68658372F1536A1161751749E631E2DE24B74173135D2EA8A8E1EFC812`。

## 9. 静态边界

提交前非文档 diff 为 7 files / 80 insertions / 22 deletions：生产实现 `3 files / +19/-11`，测试 `2 files / +41/-7`，覆盖脚本 `2 files / +20/-4`。

新增生产行中 Timer、SetTimer、自定义 Tick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0。没有新增资产、模块依赖、World 访问或第二套飞行状态；`git diff --check` 为 0。

## 10. 提交边界与 GitHub

精确提交 5 个实现/测试文件、2 个覆盖脚本与本 Report/Log，共 9 个文件。103 个用户原有 untracked 文件不暂存；所有 raw evidence 留在 `Saved/Codex/P22.8` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-8-thrown-weapon-hand-release>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-8-thrown-weapon-hand-release/Docs/Report/Dev.D.UE.0.0.10.P22.8.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-8-thrown-weapon-hand-release/Docs/Log/Dev.D.UE.0.0.10.P22.8.r0_log.md>
