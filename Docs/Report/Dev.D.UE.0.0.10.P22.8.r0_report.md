# Dev.D.UE.0.0.10.P22.8.r0 Report

## 1. 结论

P22.8 在 P 阶段边界内完成，结论为 **PASS**。

本轮把飞刀的统一释放点从角色中心上方，调整为随角色朝向旋转的右手侧代理点：局部前方 `+55 cm`、局部右方 `+28 cm`、世界上方 `+50 cm`。直线投掷、Arc 请求与 Arc 预览源基准现在都读取同一个公式，因此投射物不会再从角色中心线起飞，轨迹预览也不会与实际命令使用不同起点。

```text
First focused run:                       7 Success / 3 Fail (test fixture expectation)
Final focused input-adapter run:        10 Success / 0 Fail
Changed-file mapped regression:        401 Success / 0 Fail (17 healthy logs)
Changed-file regression coverage:      PASS (7 files / 2 rules / 17 required groups)
Regression gate self-test:              PASS 439/439
Game + Editor Development:              PASS (Game 51 actions; Editor 0 actions; both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明释放点几何、方向旋转、直线/Arc 一致性以及既有命令链与下游回归，不宣称骨骼手部 Socket 绑定、动画贴手效果或玩家视觉手感已经人工验收。

## 2. 玩家侧变化

释放点定义为：

```text
SourceLocation
  + SourceForward * 55 cm
  + SourceRight   * 28 cm
  + WorldUp       * 50 cm
```

| 源姿态 | 源位置 | 释放点 | 可验证意义 |
|---|---|---|---|
| yaw `0°` | `(100, 200, 30)` | `(155, 228, 80)` | 向前、向右、向上离开中心线 |
| yaw `90°` | `(100, 200, 30)` | `(72, 255, 80)` | 前/右偏移随角色旋转，垂直高度保持世界上方 |

这是一条不依赖骨骼资源的稳定手侧代理点。它改善中心起飞问题，同时不引入动画、Socket 名称或角色网格依赖。

## 3. 单一事实链

```text
source actor transform sampled once
  -> MakeLaunchOrigin(source transform)
  -> straight command origin OR Arc request origin
  -> the same origin feeds Arc source basis / preview
  -> existing product lifecycle and Run host
  -> existing carrier spawn at command origin
```

释放点只负责几何起点，不选择轨迹、不推进动作、不修改物品、不解析命中，也不拥有第二份飞行或终态状态。

## 4. 实现

`Fdemo_mapShanmenThrownWeaponInputAdapter` 新增三个固定几何常量与一个共享纯函数：

- `GetLaunchOriginForwardOffset() = 55.0`；
- `GetLaunchOriginRightOffset() = 28.0`；
- `GetLaunchOriginHeight() = 50.0`；
- `MakeLaunchOrigin(const FTransform&)` 用单位 Forward/Right 与世界 Up 计算唯一释放点。

直线路由现在只采样一次 `SourceActor->GetActorTransform()` 并生成命令起点。`Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter` 删除原先独立的“仅抬高 50 cm”计算，改为调用同一函数；Arc 目标采样、预览和最终请求因此共享同一原点。

未修改速度、抛物线参数、碰撞、伤害、库存事务、Run 身份、终态或投射物表现结构。

## 5. 自动化证明

新增 `HandReleaseOrigin` 测试，使用两个固定 Transform 独立验证前/右偏移与 yaw 旋转。既有路由测试扩展为读取生命周期捕获的真实命令：

1. 直线投掷命令的 `Origin` 等于源 Actor 实际 Transform 推导出的手侧点；
2. Arc 命令内的 `FShanmenThrownWeaponArcRequest::Origin` 使用同一点；
3. Arc 无效目标、无效 apex 与动作冲突继续在既有边界失败关闭；
4. Arc source-basis 测试证明预览基准在 yaw `0°`、`90°` 和委托目标路径均使用相同手侧点；
5. 下游生命周期、Session、Controller、Run Host、World Delivery、物品、CombatRuntime 与旧物品兼容组均保持通过。

## 6. 首轮失败与修复

首次 Editor 构建为 `52 actions / PASS / native 0`。首次 InputAdapter 专项得到 `7/3`、进程退出码 `255`：

- `ArcFailClosedGeometryAndActionConflict`；
- `ArcTypedRouteAndPassThrough`；
- `TypedRouteAndPassThrough`。

根因是测试夹具使用无 RootComponent 的通用 `APawn`；夹具调用 `SetActorLocation(100, 200, 30)` 后，实际 Actor Transform 并未获得该硬编码位置。新纯函数的固定 Transform 测试当时已经通过，生产计算没有错误。修复只调整路由测试：期望值改由夹具 Actor 的实际位置、Forward 与 Right 独立推导，同时保留固定坐标的纯函数测试，避免把测试退化为复述生产函数。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.8_editor_build_initial.log` | 52 actions / PASS / native 0 | 6,162 | `85E1788C4345828B6AE70AED5CA2A7D672E573A625E0FE62BF342730523B2E24` |
| `Focused-ThrownWeaponInputAdapter.log` | 7/3 / native 255 | 276,293 | `865FE1E8D57DD38780BF35EE17078E91AD51A9E7FF64E52C115AA708B62F877F` |
| `P22.8_editor_build_repair.log` | 4 actions / PASS / native 0 | 2,484 | `EDDB7CDF4B2D4507FABEBEC26584D0A33CB1AC2E58AFF63B3B72DEDC138BEC16` |

## 7. 改动文件回归

最终 17 份精确日志合计 `401 Success / 0 Fail`：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Product.ThrownWeaponArcSourceBasisAdapter` | 6 | 267,836 | `35494255DBDB9D126DCD7A242FFC018424C07C7111AC5A512205CC02C983E831` |
| `Product.ThrownWeaponInputAdapter` | 10 | 274,135 | `74807153D0CE0CA8229C193669DD05C47EC5D4911D05E0981B6F4BFEAAC7993E` |
| `Product.ThrownWeaponArcChoiceInputComposition` | 5 | 266,293 | `B94756F3F052C40DCD4F70ABE044B6C5A618AB6A61BBD2150808A3057977E612` |
| `Product.ThrownWeaponArcChoiceProjection` | 5 | 266,777 | `AA91D587BC8662CEE30F51D211B3E6013F3D7008916DA7A5DB08C1C93CE6B16E` |
| `Product.ThrownWeaponInputChoice` | 39 | 304,925 | `E6EFE84FC4C48664FA767487133AFD18A4619E962543486F7DE1BAC16FA18BD0` |
| `Product.ThrownWeaponProductLifecycle` | 5 | 267,667 | `68DBBE7999CD5BC620DEC784145E6248CA2D512BACCC451996EAB4F235791428` |
| `Product.ThrownWeaponProductSession` | 6 | 269,290 | `A0A78CF88439E41CBCC1BD8B294FE3B1A17003699788E1B124633A52C70A65AA` |
| `Product.ThrownWeaponProductController` | 7 | 270,461 | `F0EC35C80AE7F2A2733F6370A026AC2DB8CAA0718481FFB9197CFB8CE5CD0D05` |
| `Product.ThrownWeaponRunCommand` | 6 | 268,218 | `4873F8911AD34CC1CBD7F00E431E805B676594C89483F981C5AC9FDB8658BBE4` |
| `Product.ThrownWeaponRunHost` | 4 | 265,489 | `1E5BD44E9B934CCAF3735A0B00FA8AC2836B1D2DF1F1570DDFF52CCDEC1D99FA` |
| `Product.ThrownWeaponWorldDelivery` | 6 | 268,307 | `AA5C6290018CD6C9EBB442DCF0C13031FAC7F050FC56F228FB27FFCA4F87D45C` |
| `Product.ThrownWeaponItemAdapter` | 5 | 266,531 | `E2712E50965C1CD48A95D3EA51F5547677B0049867197178C0C4047943C7EBC3` |
| `Product.CombatRunCoordinator` | 18 | 287,499 | `2E2BE78F948D014D146D63CC68DA2BF4ED285B372CCF58308C89D3DA171F1518` |
| `Shanmen.0_0_10.Items` | 77 | 351,077 | `EAB7BAD2A824CCB941D2EC19A8887C1D8235588B9C0261446297174CAE03C454` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 269,986 | `A52423C23574024CF1D6BE4756DCB2D1270FF9DF6D8504627538F2F99C89767A` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 412,122 | `5B5409E7DF32547A7AB2756A6769E863E80640D7F488BC92245C685772A14A65` |
| `demo_map.ItemUseAndArmor` | 46 | 310,415 | `0ED82347A4ED3385D118C673F36A220A83267541E52BC2589C9A8D4F9114E8DB` |

## 8. 覆盖门、广域诊断与构建

Arc source-basis 的旧映射额外要求完整 `Shanmen.0_0_10` 套件。实跑发现该套件包含 `1,249` 个测试，并在无关 SwordRhythm 重试/恢复链进入每例约 `6–55s` 的长耗时。该轮在 `709/0` 时有界停止，没有正常到达队列完成标记，因此只保留为诊断证据，不计入 PASS。

映射改为与改动文件直接对应的五个精确几何/选择组；InputAdapter 既有规则继续拉起完整下游权威链。覆盖门最终为 `REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=17 Logs=17`。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `Mapped-Shanmen-0_0_10.log` | bounded diagnostic only / 709/0 | 1,188,660 | `97CC562B93205A38F4B70F586B289408D4D015A3867772D27C25425714E0F784` |
| `P22.8_regression_coverage.log` | PASS 7/2/17/17 | 5,298 | `7A1DB514A7C77B0B9EEBA9B88F98F89862BB2E9B5CA5AFDCEB6389C9D9CD365A` |
| `P22.8_regression_coverage_selftest.log` | PASS 439/439 | 43,303 | `7B0B3AA9A77B556B23D3A7AD8FFE70C8C05D3E1A3990B99963FEF1DF5EDB0C87` |
| `P22.8_game_build_final.log` | 51 actions / PASS / native 0 / 195.31s | 5,938 | `5AAF5C58AEC27CB97B003677FE4AB0D05EAA3C5D01B8201A8C28FC54A4A7321E` |
| `P22.8_editor_build_final.log` | 0 actions / PASS / native 0 / 0.97s | 1,035 | `DCAE37223B945D2BE10C620DB1F65E03AE59C7006C838798B1B9754117321EDB` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,574,528 bytes；SHA-256 `13B9E39D890AFA4C174A4F3D9AFCD418F9168F504D666ED948C550835BAAA91C`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,782,720 bytes；SHA-256 `EF862E68658372F1536A1161751749E631E2DE24B74173135D2EA8A8E1EFC812`。

## 9. P/F 与静态边界

PASS：手侧点随源 Actor yaw 旋转；直线命令、Arc 请求与 Arc 预览使用同一公式；无效几何继续失败关闭；生命周期、Run、World、物品与 CombatRuntime 回归为零失败；覆盖门、自测与双目标构建全部通过。

未声明：释放点来自骨骼动画或真实右手 Socket；不同体型、持械动画、镜像角色和网络复制下的最终视觉均未验证；没有执行交互式产品验收。

当前非文档 diff 为 7 files / 80 insertions / 22 deletions，其中生产实现为 3 files / 19 insertions / 11 deletions。新增生产行中的 Timer、SetTimer、自定义 Tick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0；没有新增资产、模块依赖或第二条状态链；`git diff --check` 为 0。

## 10. 提交边界与 GitHub

本阶段只提交 5 个实现/测试文件、2 个回归覆盖脚本、本 Report 与本 Development Log，共 9 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.8` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-8-thrown-weapon-hand-release>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-8-thrown-weapon-hand-release/Docs/Report/Dev.D.UE.0.0.10.P22.8.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-8-thrown-weapon-hand-release/Docs/Log/Dev.D.UE.0.0.10.P22.8.r0_log.md>
