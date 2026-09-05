# Dev.D.UE.0.0.10.P20.27.r0 Report

## 1. 结论

P20.27 已把 P20.26 的 Ballistic Arc target、apex adjustment 与 target clear 三种设备无关入口接入现有可重映射物理输入系统。第一套默认策略为：鼠标中键从既有顶视角 pointer/aim 方向设定 Arc target，右/左方括号以固定 `0.25` 步长提高/降低 apex，Delete 清除 target。

四个物理 handler 均只调用一次 P20.26 对应入口；没有直接访问 GameMode choice authority、session、reducer 或 command，也没有新增 intent、request、result、identity、revision、timer、retry 或 UI 状态。输入配置由 v5 迁移为 v6；旧键位保留，新默认键冲突时沿用既有唯一键迁移策略。

新增聚焦自动化 `10/0`，完整 0.0.10 由 `969` 增至 `979/0`。按改动文件映射补齐旧系统测试后，7 份最终自动化证据合计 `1180/0`；首次 `P7Integration 8/1` 因一个旧注册表数量断言仍为 24，修为 28 后该组 `9/0`，首次失败日志已保留。

本轮仅执行 Development 编译、无头合成键输入自动化与静态检查；未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实鼠标/键盘输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`2730376f42ba478c069220e390ae49292a19c9ca`（P20.26）；
- 分支：`agent/0.0.10-p20-27-thrown-weapon-arc-editing-physical-input`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 可重映射 Arc 编辑动作

现有统一输入注册表新增四个 press-only action：

| Action | 默认键 | 语义 |
|---|---|---|
| `ThrownWeaponArcTargetSet` | `MiddleMouseButton` | 读取既有 pointer/aim 方向并设定 target |
| `ThrownWeaponArcApexIncrease` | `RightBracket` | apex `+0.25` |
| `ThrownWeaponArcApexDecrease` | `LeftBracket` | apex `-0.25` |
| `ThrownWeaponArcTargetClear` | `Delete` | 清除 target |

注册表从 24 项增至 28 项，28 个默认物理键保持全局唯一。`BindProductInputActions()` 继续在绑定时从 `Fdemo_mapInputBindingSettings` 解析当前键位，因此四个动作自动继承已有 remap、swap、持久化与 rebuild 行为，没有第二套键位真值。

## 4. Pointer/Aim 与控制器路由

`SetThrownWeaponArcTargetFromPointerAim()` 复用 `GetLastValidAimDirection()`，只取 XY 分量交给 `RouteThrownWeaponArcTargetInteraction()`。归一化、零向量拒绝、trajectory capability、stale read-model fence 与最终 choice 写入仍由 P20.25–P20.21–P20.19 既有链负责。

apex 两个 handler 只向 `RouteThrownWeaponArcApexAdjustmentInteraction()` 传入 `+0.25` 或 `-0.25`；clear handler 只调用 `RouteThrownWeaponArcTargetClearInteraction()`。四者共用一组 non-shipping 调用次数与最后结果证据，不扩散四套测试状态字段。

## 5. v5→v6 配置迁移

序列化头升级为 `Version=6`。加载旧 v5 配置时：

- 所有已有 action override 原样保留；
- 四个缺失 Arc action 由既有迁移逻辑补入；
- 默认键空闲时使用中键、方括号与 Delete；
- 旧 override 已占用中键时，不覆盖旧绑定，并为 target action 分配可验证的唯一替代键；
- 最终 28 项通过统一 `ValidateBindings()`，不存在重复键。

## 6. 新增自动化覆盖

`Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput` 新增 10 项 transient GamePreview World/PlayerController/GameMode 自动化：

1. `RegistryDefaults`：四个动作、默认键、press-only 与全局唯一性；
2. `VersionFiveMigration`：无冲突 v5 配置补齐四个 v6 默认；
3. `VersionFiveConflictMigration`：旧 override 占用中键时保留旧值并迁移唯一新键；
4. `PointerTargetRoundTrip`：中键把 `(3,4,7)` 的既有 aim 转为 target `(0.6,0.8)`；
5. `ApexStepRoundTrip`：右/左方括号各路由一次 `±0.25`；
6. `ClearRoundTrip`：Delete 经 P20.26 route 清除已有 target；
7. `InvalidAimFailsClosed`：零 aim 返回既有 typed `RequestInvalid`，下游 route 计数为 0；
8. `LiveRemap`：target 改绑 H 并 rebuild 后，旧中键失效、H 生效且落盘为 v6；
9. `InputLock`：物理输入到达既有 gameplay fence 后拒绝，权威状态不变；
10. `StraightModeFailsClosed`：Straight 下四个 Arc 键全部失败关闭且不改 revision。

测试发送的是自动化合成按键；没有声明真实设备、人机操作或可见产品验收。

## 7. 改动文件回归映射与修复记录

`UnifiedInput` 与 `PlayerCombatController` 规则均加入新物理输入组；新测试文件有独立规则，覆盖 P20.26 controller route、P20.25 composition、request coordinator、interaction port、intent/controller adapters、choice session/reducer、trajectory toggle、InputRestore、V2Ranged 与完整 0.0.10。

映射正/反自测由 `347` 增至 `349/349`。最终 changed-file gate 对 15 个生产、测试与流程路径求规则并集：`Changed=15 Rules=7 Required=31 Logs=7`，全部具备健康证据。

第一次补跑 `demo_map.P7Integration` 得到 `8/1`：`RegistryCoversProductActions` 的 `Actions.Num()` 仍断言 24，而生产注册表及同文件迁移断言已为 28。这是测试期望漏迁移，不是随机环境错误。修正后重新编译，该组稳定为 `9/0`；首次失败日志 SHA-256 为 `A9739121B25741F85756B23F2C317C64C3F4DD9DB8E0F4F6A9B708C2A62E6E2A`。

## 8. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_editing_physical_input_final.log` | `Product.ThrownWeaponArcEditingPhysicalInput` | `10/0` | `CC538D34BFE6C9C95807AF9F6194256BA7C1EB2B389DB0E4DCAA347EBBF42247` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `979/0` | `C23BBEFD301860C87E444A5F3647387B908A89C6B11730F13561101F4DBAF4D8` |
| `legacy_full_system_loop_final.log` | `demo_map.FullSystemLoop` | `50/0` | `7DBBEBD806BE8D3C92B6B57AF4A1248736F97F88F5390DD91DCF1B38A6F449DE` |
| `legacy_p5_runtime_interface_final.log` | `demo_map.P5RuntimeInterface` | `9/0` | `55ADEB8AAB9DAA305E694C99BB7FAA18DAF3E345B6B494B0FF448CEC3C8C54E6` |
| `legacy_p7_integration_final.log` | `demo_map.P7Integration` | `9/0` | `573470B030D9FFC5E158D8C2069063C72C47CD4B346DC7834FF55F333EB6F9BD` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `5B7C676BFE01354083CD6451B242DBAE91358AACE45831C13BCD02CA5675D695` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `AA92E02B319866707ABEB9D8E43878565FA0308E3572DE54F33C94220E4F8E57` |

最终证据合计 `1180/0`。日志审计逐份核对唯一 RunTests 命令、精确成功数、零失败、UE 5.8 原生退出码 0、终止标记及零 fatal/unhandled/ensure；审计日志 SHA-256 为 `605F6073B576DF778335CB2E62AB486754FC6B83107544F6776935E96D0351B7`。

## 9. 静态、流程、构建与产物

- regression self-test：`349/349`，SHA-256 `3FACBF4D044380BEE2478C86060930B4CB159B3993FB62A4A1BE227850B8C9D7`；
- static audit：`30/30`，验证 28 项唯一默认键、四个 remappable binding、v6 序列化、四个 handler 单次委托及无直接 authority/UI/timer/retry/loop，SHA-256 `D6B99D1744E31ED7F789B83268A60928968D0AA2AF7942A361C984E56697967A`；
- changed-file gate：`PASS Changed=15 Rules=7 Required=31 Logs=7`，SHA-256 `B64978E52EC08728197CD48144D78AD4199C116A8BD547B8DE730408E42E29A8`；
- `git diff --check`：PASS；
- P7 修复后 Editor：native 0 / 5.95 秒，SHA-256 `152094669032D501C9E44EC882565FC7DB97103323731C89B94B46C24A3AB220`；
- final Editor：up to date / native 0 / 0.92 秒，SHA-256 `0B7B489658435BA9152B06376F29726E1B89562B66AE5FC0B3851AFB50BC1D54`；
- final Game：36 actions / native 0 / 36.11 秒，SHA-256 `90776680828C8F0B3B3E11A484C79B4C25AFABAD024404047CCF215151A49D68`。

产物：

- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,424,960 bytes，SHA-256 `9ECE45B8014876E4B8C02ACD8E06B7B2407480274D3DFCEE601D169F526BA178`；
- `Binaries/Win64/demo_map.exe`：357,665,280 bytes，SHA-256 `A5611DDC772564FDEA92982EA491FE2BCA82E9CC639354784EEF23FDD58D809E`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：可重映射四动作注册、v5→v6 无冲突/冲突迁移、现有 pointer/aim 到 Arc target、apex `±0.25`、clear、live remap、Straight/零 aim/input lock 失败关闭、完整 0.0.10 与受影响旧输入/控制器回归。

未验证：真实鼠标/键盘事件、OS 焦点与实际 cursor hit、Editor UI/PIE/Standalone、可见轨迹/落点、真实投掷、World trace/碰撞/命中、库存扣减、伤害、Smoke、Cook 或 Package。无头合成输入与 Development 构建不能描述为产品可见验收。

建议 P20.28 在既有 HUD presentation 内显示当前 remap 后的 target/apex/clear 键名与 Ballistic Arc 模式提示；只读取统一 input settings 与现有 P20.24 presentation model，不建立第二套 UI 状态或输入权威。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-27-thrown-weapon-arc-editing-physical-input>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-27-thrown-weapon-arc-editing-physical-input/Docs/Report/Dev.D.UE.0.0.10.P20.27.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-27-thrown-weapon-arc-editing-physical-input/Docs/Log/Dev.D.UE.0.0.10.P20.27.r0_log.md>
