# Dev.D.UE.0.0.10.P7.9.r0 Report

## 1. 结论

P7.9 已把暗器产品链接入现有 1–9 快捷栏输入，结论为 **PASS**。

新增唯一的 `Fdemo_mapShanmenThrownWeaponInputAdapter`。它先从 ShanmenItems durable active-Run correlation 解析冻结快捷栏中的 exact item，再读取 canonical definition 的 typed `Item.Weapon.Thrown` semantic。普通消耗品和空槽位不采样瞄准，原样回落既有 `RequestUseBoundQuickSlot`；只有 typed 暗器才捕获起点、瞄准方向与稳定 `SelectionId`，并进入 P7.8 product lifecycle。

适配器仅保存当前 Run 内的瞬时输入序号，不保存第二份 Run、快捷栏、物品、瞄准、世界或库存真值。失败的 typed 暗器请求不会回落旧消费路径，避免物品在产品链未成立时被绕过事务直接扣除。

## 2. 输入分流契约

每次物理快捷栏输入按固定顺序处理：

1. 验证槽位范围；
2. 从 ready ShanmenItems authority 的 durable receipts 重建 active-Run correlation；
3. 取得冻结快捷栏槽位对应的 exact `ItemInstanceId`；
4. 捕获 authority snapshot 并验证 owner、Run scope 与 revision；
5. 从 canonical definition 检查 `Quantity` resource kind 与 exact `Item.Weapon.Thrown` tag；
6. 仅在确认 typed 暗器后验证 matching lifecycle、CombatRunCoordinator 与 canonical player Actor；
7. 最后才采样 aim、派生 identity、捕获 immutable intent 并提交产品链。

以下情况返回 `PassThrough`：没有 ready ShanmenItems authority、没有 durable active Run、槽位为空、或 exact item 不是 typed 暗器。其他 typed 暗器错误均被当前适配器处理并失败关闭，不允许旧通用消费路径继续执行。

## 3. 确定性身份与采样边界

`SelectionId` 使用命名空间：

```text
demo_map.ShanmenThrownWeapon.HotbarInputSelection.r1
```

canonical parts 为 correlation、Run、exact item、槽位、authority revision 与 input ordinal。等价证据精确重放同一 GUID；任一 canonical part 改变都会隔离 identity。

输入序号只在有限非零 aim 已成功捕获成 immutable `Fdemo_mapShanmenThrownWeaponHotbarIntent` 后推进。无效 aim、错 source、错 Run、未绑定 lifecycle、无效槽位或 stale item evidence 均不消费序号。Run 激活和释放是仅有的显式 reset 边界。

首个直线投掷产品参数沿用 P7.8 冻结策略：source Actor 位置上移 `50`，最大距离 `1400`；方向由现有 `GetLastValidAimDirection()` 采样并在 intent 边界规范化。适配器不创建 Tick、timer、轮询或持续设备状态。

## 4. GameMode 与旧快捷栏兼容

`Ademo_mapPlayerController::UseHotbarSlot` 保留原 1–9 绑定，只在原 `RequestUseBoundQuickSlot` 前询问 GameMode 的 typed adapter：

```text
physical slot press
  -> typed adapter
     -> PassThrough: existing RequestUseBoundQuickSlot exactly once
     -> handled: thrown product accepted or fail-closed; no legacy consume
```

GameMode 继续是 CombatRunCoordinator 与 thrown lifecycle 的唯一 owner。它持有一个非 UObject adapter，并在 Combat Run 成功激活后、产品 Run 完整释放后重置瞬时 ordinal。没有新增输入组件、第二个 GameMode、快捷栏副本或库存 authority。

## 5. 自动化证据

四份最终无头日志都只有一个实际 `RunTests` 命令、queue-empty、Fail `0`，进程原生退出码均为 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.ThrownWeaponInputAdapter` / `ThrownWeaponInputAdapter.log` | 3 | 0 | `BE62942FD613E805FD0CBB663EC2AFC63EF69390B4554AC690109A30B80A4147` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 207 | 0 | `90C777CB8A90DE90C1090C1DC9B065FFE4404CA1BD502AB161C1D51C1ADC8F24` |
| `demo_map.ItemUseAndArmor` / `ItemUseAndArmor.log` | 46 | 0 | `3B6579B37394AC0603C22D2D38FE73728696C2814E482B40D281E83E0727AEBB` |
| `demo_map.V2RangedCompatibility` / `V2RangedCompatibility.log` | 22 | 0 | `791699AFA0E16913764DD569BFB696AD423D9CEEE051426F87C1C6195BAD8E42` |

新增三项测试覆盖：

1. canonical identity 的确定性、重放与非法输入失败关闭；
2. 普通丹药和空槽位零 aim sampling、零 authority mutation，typed 飞刀单次采样并进入完整产品链；
3. 无效槽位、foreign source、未绑定 lifecycle 在采样前拒绝，无效 aim 不推进 identity、不改变 authority。

完整 0.0.10 suite 从 P7.8 的 `204` 增至 `207`。

## 6. 首次执行与重试

本阶段没有源码、自动化、链接、内存、SDK 或环境失败：

- 首次 Editor integration：直接成功，native exit `0`；
- 首次 targeted automation：`3/3`；
- 首次 full automation：`207/207`；
- 首次旧物品快捷栏回归：`46/46`；
- 首次远程兼容回归：`22/22`；
- 首次 Game integration：直接成功，native exit `0`。

没有通过重复构建、放宽断言或跳过旧测试获得成功。

## 7. 改动—回归与静态门禁

- regression map JSON parse：PASS；
- mapping self-test：`30/30 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=8 Rules=3 Required=15 Logs=4`；
- `M01GameMode` 与 `PlayerCombatController` 现在显式要求输入适配组；
- 新 `ThrownWeaponInputAdapter` 映射要求完整 thrown pipeline、Items、WorldGameplay、CombatRuntime 与旧 `ItemUseAndArmor`；
- boundary scan：无 Tick、timer、输入重新绑定、legacy item subsystem、`ApplyDamage` 或 RNG；
- `git diff --check`：native exit `0`。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| 构建 | Actions | Result | Native exit | 时间 | UBT SHA-256 |
|---|---:|---|---:|---:|---|
| Editor integration | 24/24 | Succeeded | 0 | 116.25s | `FAEB7785BC385FF22EA9DB488AE4F675C92D5EFADEA0A75DD42D546F83608EE9` |
| Game integration | 23/23 | Succeeded | 0 | 89.30s | `847F25033F05DCB401A8699A0C447D228EBDF9AF412582552453F155E8FFC0B0` |

- Editor DLL：`10739712` bytes，UTC `2026-08-29T14:19:54Z`；
- Game executable：`351923200` bytes，UTC `2026-08-29T14:25:51Z`。

## 9. 修改范围与完整性

生产与验证修改：

- `demo_mapShanmenThrownWeaponInputAdapter.h/.cpp`；
- `demo_mapShanmenThrownWeaponInputAdapterTests.cpp`；
- `demo_mapGameMode.h/.cpp`；
- `demo_mapPlayerController.cpp`；
- regression map 与 self-test；
- 本 Report 与同名 Development Log。

未修改 save schema、Content 资产、GameplayTags、Build.cs、Code B、legacy inventory authority 或旧物品定义。长期未跟踪的 0.0.9B 与用户文件未修改、未 stage。

## 10. P/F 边界、阶段判断与 GitHub

本轮只执行 P 阶段源码、静态检查、`-NullRHI` 无头 Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实设备输入、截图、Smoke、Cook 或 Package。

P7 暗器直线投掷纵切现已从纯 Runtime、物品事务、世界 Actor、Run host/router/controller/session、真实商品、GameMode lifecycle 到唯一输入分流完整闭合。真实键位手感、碰撞与视觉验收应留给 F 阶段。后续 P 阶段若继续新体系，建议进入 P8 阵法基础契约：先冻结阵图、阵眼、材料需求与部署状态机，不在首轮混入 Actor、UI 或正式配方数值。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-9-thrown-weapon-input-adapter/Docs/Report/Dev.D.UE.0.0.10.P7.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-9-thrown-weapon-input-adapter/Docs/Log/Dev.D.UE.0.0.10.P7.9.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-9-thrown-weapon-input-adapter>
