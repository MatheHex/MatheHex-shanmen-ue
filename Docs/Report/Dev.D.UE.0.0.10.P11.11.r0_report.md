# Dev.D.UE.0.0.10.P11.11.r0 Report

## 1. 结论

P11.11 已完成并通过 P 阶段门禁。

本阶段把 P11.9 的无状态输入适配器、P11.10 的唯一活动 Session 与实际 Shipping 输入入口闭合：在既有统一输入注册表中新增 `WeaponGuard`，默认绑定鼠标右键；由 Combat Run 持有唯一 30 Hz fixed timeline；PlayerController 的 press/release 只采样时间并委托产品 Session，不保存武器、Host、窗口或伤害状态。

最终结果：

- `WeaponGuard` 成为统一输入注册表第 23 个动作，默认键为 `RightMouseButton`；
- 输入配置格式由 Version 3 升为 Version 4，旧覆盖项保留，新动作自动补默认键；
- 当右键已被旧配置占用时，迁移使用既有空闲键分配规则，不制造重复绑定；
- press 经 P11.9 adapter 采样 Run-owned timeline，再调用 P11.10 Session start；
- release 经 typed release adapter 调用 Session release；
- release 不读取 gameplay/UI lock，避免按下后打开 UI 导致 guard 永久卡住；
- GameMode 在 Combat Run 激活时创建固定时间线，在 Tick 中只累计 30 Hz 整数 tick，在 Run teardown 时销毁；
- timeline ID 由 Run ID 与 canonical tick rate 确定性派生，不使用新随机 GUID、墙钟或 frame counter；
- 既有 5-tick perfect window 现在对应 30 Hz 下半开区间 `[start, start + 5)`，即约 166.667 ms；
- 新增 11 个 focused tests，并扩充 1 个 input-adapter release test；
- 0.0.10 全量达到 528/528；
- 14 份健康 Automation 日志合计记录 662 次 Success、0 次 Fail；
- regression mapping 达到 100 rules，自检 160/160；
- changed-file gate：`PASS Changed=21 Rules=5 Required=43 Logs=14`；
- `git diff --check` 与精确模块边界扫描通过；
- Editor Development 与 Game Development 最终单并发构建均成功，原生退出码均为 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 Run-owned fixed timeline

新增 `Fdemo_mapShanmenWeaponGuardFixedTimeline`。它由既有 `Ademo_mapGameMode` 持有，并与 Combat Run 同生共灭：

```text
Combat Run begin
  -> deterministic TimelineId(RunId, 30 Hz)
  -> tick 0
  -> GameMode Tick supplies DeltaSeconds
  -> timeline emits monotonic whole ticks
  -> input press captures {TimelineId, ActiveStartTick}
Combat Run end
  -> timeline validates matching RunId
  -> reset
```

时间线只保留 `RunId`、`TimelineId`、`CurrentTick` 与不足一 tick 的余量。它不拥有 Actor、World、Timer、input、inventory、frame number 或 wall clock。不同的合法 DeltaSeconds 分片会得到相同的 30 Hz 整数 tick 结果。

负数、NaN、Infinity、溢出与错误 Run identity 全部原子失败；拒绝不会部分推进真实状态。

### 2.2 输入注册与迁移

统一注册表新增：

```text
ActionId: WeaponGuard
DefaultKey: RightMouseButton
RequiresReleasedEvent: true
Category: 战斗
```

注册表精确数量由 22 更新为 23。序列化版本由 3 更新为 4，但读取仍走既有通用迁移：

- 保留所有有效的旧动作覆盖；
- 为旧文件缺失的 `WeaponGuard` 补默认键；
- 默认键冲突时选择可用 fallback；
- 迁移结果仍通过 exact registry validation；
- Live remap 后重新构建 press 与 release 两条 binding。

所有依赖精确动作数量的既有 FullSystemLoop、InputRestore、P7Integration、RuntimeInterface 与 SpiritEvasion 测试已同步更新。

### 2.3 物理 press/release 路由

PlayerController 新增实际绑定：

```text
RightMouseButton Pressed
  -> IsGameplayInputAllowed
  -> capture one opaque fixed-timeline sample
  -> P11.9 RouteStartInput
  -> GameMode RouteWeaponGuardStartIntent
  -> P11.10 ProductSession.TryStart

RightMouseButton Released
  -> P11.9 typed release adapter
  -> GameMode RouteWeaponGuardReleaseIntent
  -> P11.10 ProductSession.TryRelease
```

press 仍受正常 gameplay input gate 约束。release 故意不受该 gate 约束：即使按住 guard 后打开库存或其它 UI，松键仍会清理 active Session，避免 stuck hold。

按键处理器不选择装备、不创建 action identity、不持有 Host、不推进 guard lifecycle，也不直接修改 Impact 或玩家生命。

### 2.4 Run teardown 与故障关闭

新 Run 激活会同时拒绝遗留 guard Session 与遗留 fixed timeline。Run 释放顺序为：

1. interrupt active guard Session；
2. 结束既有 controlled/thrown weapon 产品生命周期；
3. 释放 Combat Run Coordinator；
4. 使用相同 Run ID 结束 fixed timeline；
5. 清理所有 Run-owned 状态。

时间线创建失败会回滚刚启动的 thrown lifecycle 与 Combat Run Coordinator。时间线 teardown 不匹配时记录精确诊断并 fail closed，不把错误状态伪装成成功。

## 3. 完整性

新增 release 结果显式区分：

- `Applied`；
- `ProductRouteUnavailable`；
- `ProductRejected`。

成功必须同时满足 typed status、产品 route 确实调用以及 Session transition proof 有效。start 继续沿用 P11.9 的 gameplay gate、route availability、timeline sample 与 product rejection 状态。

时间线生命周期覆盖：

- 确定性 identity 与 begin/end；
- 固定频率分片等价；
- sub-tick carry；
- 非法 delta 原子拒绝；
- opaque sample 与 Run mismatch；
- overflow fence。

物理输入覆盖：

- registry default；
- Version 3 -> 4 migration；
- migration conflict fallback；
- press/release 双 binding；
- live remap；
- UI input lock 下 release 仍执行。

## 4. 兼容性与权威边界

- InputActionRegistry 继续是唯一物理键注册真值；
- InputBindingSettings 继续拥有序列化、迁移、冲突与 remap；
- GameMode 只拥有 Run-scoped timeline 与既有 Product Session；
- PlayerController 只负责设备事件到 typed adapter 的转换；
- P11.9 adapter 仍无状态，不持有 clock 或 Host；
- P11.10 Session 继续是唯一 active guard Host 所有者；
- Combat Run Coordinator 继续拥有 Run/activation identity 与 sequence；
- ItemAuthority 继续拥有当前装备真值；
- Product Host 继续拥有 timing、arc、defense 与 action lifecycle；
- 本阶段未增加第二套输入、Run、item、clock、damage 或 lifecycle 系统；
- 未调用 `ApplyDamage`、RNG、`FGuid::NewGuid`、wall clock 或 `GFrameCounter`；
- 物理输入测试使用最小 `UWorld` fixture 构造真实 PlayerController binding，但纯时间线与输入 adapter 的生产边界不依赖 World/Actor。

## 5. 修改范围

实现与门禁共 21 个文件、1,068 行新增、19 行删除（不含本 Report/Log）：

- `Source/demo_map/demo_mapShanmenWeaponGuardFixedTimeline.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardFixedTimeline.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardFixedTimelineTests.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardPhysicalInputTests.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardInputAdapter.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardInputAdapter.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardInputAdapterTests.cpp`
- `Source/demo_map/demo_mapPlayerController.h`
- `Source/demo_map/demo_mapPlayerController.cpp`
- `Source/demo_map/demo_mapGameMode.h`
- `Source/demo_map/demo_mapGameMode.cpp`
- `Source/demo_map/demo_mapInputActionRegistry.h`
- `Source/demo_map/demo_mapInputActionRegistry.cpp`
- `Source/demo_map/demo_mapInputBindingSettings.cpp`
- `Source/demo_map/demo_mapFullSystemLoopTests.cpp`
- `Source/demo_map/demo_mapInputRestoreTests.cpp`
- `Source/demo_map/demo_mapP7IntegrationTests.cpp`
- `Source/demo_map/demo_mapRuntimeInterfaceSliceTests.cpp`
- `Source/demo_map/demo_mapShanmenSpiritEvasionPhysicalInputTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 文档与其它资料未修改、未暂存、未提交。

## 6. 测试覆盖

最终健康 Automation 证据：

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardFixedTimeline` | 5 | 0 | 1 | `76D32DADE6481430E5D67470EB49E1BAA3615ADAC87EA00A25CCF9314A11FC7C` |
| `Shanmen.0_0_10.Product.WeaponGuardPhysicalInput` | 6 | 0 | 1 | `0A58AF8D5CB48966858032A9BBCB0439625D9922A8C47AFF57DE7D17101DEB40` |
| `Shanmen.0_0_10.Product.WeaponGuardInputAdapter` | 6 | 0 | 1 | `9BE627E7855DA2569D1C39FD362E10F4EBE7EC9C627B84A5A92A39B125FD43C4` |
| `Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput` | 6 | 0 | 1 | `1CCE00563B7A562677FDE8EC665630FD75C218C7DBD61178794DDCEB8C16BFB3` |
| `Shanmen.0_0_10` | 528 | 0 | 1 | `3C8903300FD3B77E1836B4A43B97F21770C83A625439DBE285C47A36BEBF5B4D` |
| `demo_map.FullSystemLoop.41` | 1 | 0 | 1 | `8BC6D9163F085236BEA62FCA858C07FCA0BC15BDB1C637697723025DB2F1F0AA` |
| `demo_map.FullSystemLoop.47` | 1 | 0 | 1 | `5AA0430B0AC4F9DDE27629DB8FC87210E1F28C05622BDF1B11F7F85D8B121514` |
| `demo_map.P7Integration` | 9 | 0 | 1 | `450E2313BF435B4E3D2CE0C49A803FE7B8771871779FA861A0562171CA0B970A` |
| `demo_map.P5RuntimeInterface.06` | 1 | 0 | 1 | `88982A8C0DC73503FE77BC661BD97B7AE5A8190B56CA5EB9CD4AD4E0A4CC302B` |
| `demo_map.InputRestore.32` | 1 | 0 | 1 | `6E2DDFA88F964918FEDC6102AC4F783107AB928CDB1C318BCB0E0A98B2AF111F` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `F3F9B595FFC1D031D715DFBF1CA6FD6EFFAD0EFF72845C9C81B4B6269C0F911C` |
| `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `8E1657144369A611DB8F12C4E3FB8B2D8378CAD4894AC803F5BDF52C76FFD066` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `BF09D94C403CA454C2198895368EACED7C8BC209043AFAF30D77B36A54451E68` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | `58ABE0C9CDBA32CEE4445BB0D7EC88F8FADE9072A7C5C7300256D69F4315D178` |

合计 662 Success / 0 Fail；四份 focused 日志中的 23 项同时包含于 528 项 full suite，因此合计包含重复覆盖。按测试 identity 去重后，本轮覆盖 639 项。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=100
SELF_TEST: PASS 160/160
REGRESSION_COVERAGE: PASS Changed=21 Rules=5 Required=43 Logs=14
git diff --check: PASS (native exit 0)
PURE_BOUNDARY_SCAN: PASS
TEST_DETERMINISM_SCAN: PASS
```

- mapping SHA-256：`42D7912BC74DC655726201CAC7A020599F580AF6C6C4807A92294CFFB1A9C5AD`；
- self-test SHA-256：`0022FA800060A23FD50D7CF3A7DCA74EEC379142290FB4F5B4BCFCB5DA85198E`；
- 新增 FixedTimeline rule，要求 Run、Session、input 与完整 WeaponGuard 产品链证据；
- UnifiedInput rule 覆盖新物理输入、SpiritEvasion、registry migration 与全部既有输入兼容组；
- GameMode/PlayerController rule 同步要求 WorldGameplay、Run、items、hotbar 与旧 runtime contracts；
- 纯生产边界扫描未发现 World、Actor、wall clock、frame counter、随机 GUID、ApplyDamage 或 RNG。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Final actions / time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development（首次） | Failed / test-only API mismatch | 38/41 / 151.87s | 6 | `CF80DB1E4B9BDF6C56CF75BA461ADE650AC94BDEC54857D58E01430DCA0AA056` |
| Editor Development（最终） | Succeeded | 4 / 5.28s | 0 | `654C0E8EFABC861EFE6EB84527388CFE28ED7A64E5ECA4AA6AAE35B48A4F77E6` |
| Game Development（最终） | Succeeded | 40 / 147.17s | 0 | `5E53B4568DF8B4A602C8450AC3BA04EF5A7F125B6E7FAFCD7ED78D47AE5A179E` |

最终产物：

- `UnrealEditor-demo_map.dll`：12,841,984 bytes，SHA-256 `950F46A5D84FFA8E2C91850FB56DDF70B3150BBD25A454E4D587374EEA967FCE`；
- `demo_map.exe`：354,359,808 bytes，SHA-256 `DC2D4C4967F55D928B4390E9C08CBF1AC3BB9B01603C797134A3BA17FF8AABF0`。

## 9. 真实异常

- 首次 Editor 构建原生退出码 6，UBT 为 `OtherCompilationError`。唯一错误位于新测试：UE 5.8 的 `TNumericLimits<double>` 不提供 `QuietNaN()` / `Infinity()`。
- 测试改用标准库 `std::numeric_limits<double>::quiet_NaN()` / `infinity()` 后，Editor 最终构建原生退出码 0。该错误不是产品源码失败、内存环境错误或平台 SDK 错误。
- 首次静态扫描把需要构造真实 PlayerController 的物理输入测试 fixture 纳入“纯值模块不得使用 UWorld”边界，因此命中 fixture 的 `UWorld`。按实际架构拆分为纯生产边界与测试确定性边界后均通过；没有为迎合扫描而移除真实 binding 测试。
- 14 份最终 Automation 日志均有原生 terminal marker、至少一个 Success、0 Fail，且无 Fatal、Unhandled Exception 或 Ensure。
- UE SDK 检查仍打印与本 Win64 目标无关的平台 metadata invalid；Win64 SDK `10.0.22621.0` 有效，两个最终目标成功。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段实现、代码审查、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或 F 阶段产品回归。

下一阶段 P11.12 建议闭合“真实敌对 Impact -> active guard Session -> canonical defense receipt”产品缝：

1. 从既有敌对命中/Impact 路由采集 attacker/target world observation 与同一 fixed-timeline observed tick；
2. 只在 active Session 上调用既有 WeaponGuard Host/DefenseCoordinator；
3. 把 qualified guard/perfect-guard layer 合入 canonical `FShanmenDefenseSnapshot`；
4. 由既有 Impact resolver 结算并保留 timing、arc、item、Run 与 layer receipts；
5. 不让 PlayerController、输入层或 GameMode 直接扣血、制造防御结果或绕过幂等 ledger。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-11-weapon-guard-physical-input>
