# Dev.D.UE.0.0.10.P22.0.r0 Development Log

## 1. 基线与目标

- base：`23f10888f547b00d35115af507e32043b8432c24`（P21.15 flying-sword travel facing）；
- branch：`agent/0.0.10-p22-0-visible-thrown-weapon-carrier`；
- 目标：让既有投掷物 World Actor 具有随真实速度朝向的可见飞刀原型，并让显示寿命服从既有耐久发布状态机；
- 边界：不新建运动、碰撞、命中、资源、Combat Run、Impact 或计时权威，不启动 UI/PIE/产品 executable。

## 2. 审计与决策

既有 `Ademo_mapShanmenThrownWeaponProjectile` 已拥有球形根碰撞、`UProjectileMovementComponent`、直线/抛物线发射、`bRotationFollowsVelocity=true`、命中与射程终态，但没有任何网格组件，实际游戏中载体不可见。

没有创建新的表现管理器或平行 Actor。最小修改直接落在 canonical 投掷物 Actor：添加一个无碰撞的 Engine Cube 子网格，让可见性严格跟随 Empty / Staged / InFlight / Spent 既有状态，并继续由运动组件旋转 Actor。

本轮也审计了飞剑防御方向，但现有需求尚未冻结减免值、角度、消耗与触发优先级，因此没有猜测并提前实现防御数值。

## 3. 实现

Actor 构造函数新增 `ThrownWeaponVisual`：

- 附着到根 `USphereComponent`；
- 加载 `/Engine/BasicShapes/Cube.Cube`；
- 缩放为 `FVector(0.30f, 0.045f, 0.012f)`；
- 关闭碰撞与重叠；
- 初始隐藏。

`TryStageLaunch()` 要求网格存在并保持隐藏；`ActivateCommittedLaunch()` 在既有运动正式激活后显示网格；`CancelStagedLaunch()` 与 `MarkSpent()` 隐藏网格。`IsStagedFor()` / `IsInFlightFor()` 把相应可见状态纳入权威状态核验。

新增两个只读观察入口供真实 Actor 测试使用：`IsPresentationVisible()` 与 `GetPresentationForwardDirection()`。没有新增表现写入口。

## 4. 生命周期与边界

网格不会在预备阶段提前出现，也不会在耐久提交拒绝、取消或终态后残留。它不产生碰撞或重叠，实际命中继续来自球形根组件；直线和抛物线方向继续来自既有运动配置。

表现缺失时预备发射失败关闭，避免“逻辑投出但玩家看不见”的不完整载体。实现没有添加 Tick、Timer、RNG、`ApplyDamage`、额外 Spawn 或 Destroy。

## 5. 测试开发与首失败

扩展既有 `DurableLaunchGate` 与 `ArcWorldMotion`：

- 预备状态隐藏；
- 耐久提交拒绝后隐藏；
- 发布后显示；
- 直线发射改为非默认 `+Y`，同时断言速度与网格前向；
- 抛物线发布后断言网格前向等于初始弧线速度方向；
- 无命中终态隐藏。

第一次专项为 `4 Success / 1 Fail`。`DurableLaunchGate` 的旧 `NewObject` Actor 没有注册进 World，非默认方向无法把 Actor transform 传播给子网格。修复改用测试文件中已有的 `FThrownSpawnWorldFixture` 与 `SpawnStagedCarrier()`，在真实 World 中生成 Actor 与 source；断言未被删除或放宽。重新编译后最终专项为 `5 Success / 0 Fail`。

## 6. 全量运行诊断

第一次全量在 712/0 时主动停止并保留诊断日志；当时 `google.com/generate_204` 的 3 秒失败重试持续出现。禁用 Fab 后 6/0 探针仍记录 10 次该 URL，证明 Fab 不是来源。

引擎源码唯一字面引用位于 UE 5.8 `MainFrame/Private/HomeScreen/SHomeScreen.cpp::CheckInternetConnection()`；`HomeScreen.EnableHomeScreen` CVar 的源码说明允许配置关闭，且 `TabManager` 要求它在创建 Home Screen 前完成设置。使用命令行临时覆盖：

```text
-ini:Engine:[ConsoleVariables]:HomeScreen.EnableHomeScreen=0
```

第二个 6/0 探针中 `generate_204` 为 0。覆盖只作用于测试进程，没有修改项目、Engine 或 Windows 配置。探针仍记录测试自身的大帧间隔，故二者在结论中分开处理。最终全量从头运行且自然完成，结果为 1,247/0 / native 0。

## 7. 自动化与门禁

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.0_focused_thrown_world_initial.log` | 4/1 | 265,856 | `E96AF3890759F305B63DE0CC1936896A8F10E25E278BF7CBAAB658B767401992` |
| `P22.0_focused_thrown_world_final.log` | 5/0 | 264,639 | `ABB4393E8B063D83D794C8ADE0D5265214AAD0B5D6A42C32C577C2828663A5A3` |
| `P22.0_full_diagnostic_fab_timeout_708_success.log` | 712/0 (intentionally stopped) | 1,190,257 | `16F0A82807DE5A0643D115D09BF57C008053792F152541F36CACDE38350C9575` |
| `P22.0_fab_disabled_probe.log` | 6/0 | 269,356 | `E04236A82A954F0F1ADEE436FBA1694980A1300FEEA9092AB8935EEE752E62BD` |
| `P22.0_homescreen_disabled_probe.log` | 6/0 | 264,213 | `C2B72FE408D236ABF4887C065556F2B05AA9FCD846DA309AE643E1B4198B17F3` |
| `P22.0_full_final.log` | 1,247/0 | 1,857,382 | `3A4D3BE56AF4C6CBD9CFDC96DEC7B4C7B092B1D78216865DBFB2AB42580FE4FE` |
| `P22.0_regression_gate_final.log` | PASS Changed=3 Rules=1 Required=6 Logs=1 | 798 | `C21E1F98A2A725DF4B76C2A66B03CD767469F014341206D9BABA9D5BEC781F03` |
| `P22.0_regression_gate_selftest.log` | PASS 437/437 | 43,088 | `BD30B0701682789BB228FB6E7FC062E2C9BB5D870082227B6E91D4B242966C88` |

覆盖门读取三个实际实现/测试路径，由完整 `Shanmen.0_0_10` 日志覆盖映射得到的 6 个 required groups；没有用专项组替代跨模块证据。

## 8. 构建与静态检查

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.0_editor_build_initial.log` | 118 actions / PASS | 12,650 | `E1259B2BF3838D9104DA922AA8F45978E3ACC20B84EF6DFFAE824B320B3C53BA` |
| `P22.0_editor_build_after_fixture_fix.log` | 4 actions / PASS | 2,354 | `69309AEB141BB33A18346F7D08A55E336BA6E195B0379C20C2AFAF2D71E926FD` |
| `P22.0_game_build_final.log` | 117 actions / PASS | 11,444 | `B6EE98ABA8AB46AA8448F865309D9BC6879BA5E2F84F20B102E96D5D2B785D83` |
| `P22.0_editor_build_final.log` | 0 actions / PASS | 1,036 | `C3784CC4BA60098180CAD0A8E2A5D6B5E4D1A5BA69D3414D92C90D303ACD74A2` |

`git diff --check` native 0。实现/测试为 3 files / 77 insertions / 9 deletions；新增生产行无 Timer、SetTimer、RNG、ApplyDamage、SpawnActor 或 Destroy。最终 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程数均为 0。

## 9. 提交边界

精确提交以下五个文件：

- `Source/demo_map/demo_mapShanmenThrownWeaponProjectile.h`；
- `Source/demo_map/demo_mapShanmenThrownWeaponProjectile.cpp`；
- `Source/demo_map/demo_mapShanmenThrownWeaponWorldAdapterTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P22.0.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P22.0.r0_log.md`。

103 个用户原有 untracked 文件不暂存；raw evidence 保留于 `Saved/Codex/P22.0` 且不进入 Git。未修改 Content、地图、Engine、Windows、存档 schema、物品权威、Impact resolver 或 target vitality。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-0-visible-thrown-weapon-carrier>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-0-visible-thrown-weapon-carrier/Docs/Report/Dev.D.UE.0.0.10.P22.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-0-visible-thrown-weapon-carrier/Docs/Log/Dev.D.UE.0.0.10.P22.0.r0_log.md>
