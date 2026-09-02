# Dev.D.UE.0.0.10.P18.2.r0 Development Log

## 1. 目标与基线

- 基线提交：`cafce4bc01bf84da0e5409edc83e9195085daaa0`（P18.1 Sword Qi world delivery）；
- 分支：`agent/0.0.10-p18-2-sword-qi-run-host`；
- 目标：建立最小 Sword Qi Run Host，实际拥有载体创建、delegate binding、Action/execution 与终止策略；
- 约束：仅做 P 阶段，不接真实输入、正式视觉/声音、关卡投放、第二套伤害或生命权威。

## 2. 起始审计与策略修正

审计复用了现有 Thrown Weapon Run Host 的所有权模式以及 P18.1 的 Sword Qi projectile/world adapter，但没有把剑气伪装成暗器，也没有引入库存事务。

最初可选策略包括“合法目标穿透”。继续核对物理载体后确认当前载体是 blocking hit，ProjectileMovement 会在阻挡接触后停止；只让 execution 保持 InFlight 会制造与 Actor 不一致的伪穿透。因此 P18.2 冻结为 first-blocking-contact terminal policy：合法目标结算后消散，未解析阻挡无伤害消散；未来穿透必须先引入真实 overlap/sweep 载体语义。

## 3. 生产源码

新增：

- `Source/demo_map/demo_mapShanmenSwordQiRunHost.h`；
- `Source/demo_map/demo_mapShanmenSwordQiRunHost.cpp`。

Host 提供：

- 有界 `SpawnStagedCarrier`，验证 World/source/class/origin 与 inert carrier；
- `TrySpawnAndLaunch`、`TryLaunchCarrier` 和 `TryAdoptPublishedFlight`；
- 唯一 Action/execution/carrier delegate 所有权；
- launch receipt 派生的 maximum distance 与 lifespan；
- Impact、BlockingMiss、RangeExpired、Interrupted 四类终止审计；
- active reset/busy/binding 围栏与 GameThread 析构失败关闭。

命中仍通过 P18.1 Adapter 和 CombatRunCoordinator 提交唯一 vitality。未创建第二个 resolver、ledger、生命、伤害、技能或物品系统。

## 4. Automation 与 fixture

新增 `Source/demo_map/demo_mapShanmenSwordQiRunHostTests.cpp`，三条测试为：

1. `Shanmen.0_0_10.Product.SwordQiRunHost.SpawnLaunchAndOwnership`；
2. `Shanmen.0_0_10.Product.SwordQiRunHost.ImpactLifecycle`；
3. `Shanmen.0_0_10.Product.SwordQiRunHost.MissRangeAndFailClosed`。

创建测试使用最小 `EWorldType::GamePreview` World，关闭 physics scene、navigation、AI、audio、FX、trace collision 与 transactional 行为；没有启动产品或编辑器 UI。接触测试复用 transient Pawn、真实 CombatRunCoordinator 与现有 M01 enemy vitality host。

冻结数值为 `0.5 + 20 * 0.01 = 0.7` damage、`900` speed、`1400` maximum range。断言包括一次生命提交、revision +1、`1400/900` lifespan、阻挡/超距/中断收敛、delegate 解绑与析构清理。

## 5. 编译与测试过程

首次 Editor build 一次通过：5/5 actions、native 0、31.40s，日志 SHA `3CC09889E4E1287CC8006F6B6415EDA060DE70211462657A283042F0C1B382A6`。没有编译失败或语义修正轮。

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_sword_qi_run_host.log` | exact product group | 3/0 | `1294266A616144CC550072C4FE53904770C61269F26D44DF6D13D131033051D3` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 761/0 | `7F33030D97FC64D8F18EFDF1D9A54825CCFDB1769350886B830C37A534BB4858` |

两份日志都有 native test exit 0，Fail/Fatal/Unhandled/Ensure 为 0。full 首末 Success 为 `21:29:35.873 -> 22:00:08.890 UTC`，约 30m33.017s；测试计数相对 P18.1 精确增加 3。

## 6. 改动驱动回归与静态门禁

修改 `Scripts/ShanmenRegressionMap.json`，新增 `SwordQiRunHost` 路径映射，要求 exact Host、SwordQiWorldDelivery、CombatRunCoordinator、WorldGameplay、CombatRuntime 与 CombatCore 六组证据。

修改 `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据通过用例和仅有 Host 聚焦证据时必须失败的用例。self-test 从 275 增至 277 条。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=6 Logs=2
SELF_TEST: PASS 277/277
BOUNDARY_SCAN: PASS Files=3 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage SHA：`AEBFEE99C425DB9562D74EDF73692B551BBC089B6116DD84F0EAA2ADF864B875`；
- self-test SHA：`7BF1E601F011F44E696BB2F1B7CF09CD77CA11FEDFFCE4F88DE6723FC4A393C6`；
- map SHA：`02B88A5104E50AD9EEFDC593D17B6B434E785FF87EC04D641C05D9A4EABECBB7`；
- boundary SHA：`60B7F34F72883655FF6103A874009A2AEFF8A4AEF6B17979AE4CA41F304443DF`。

边界扫描禁止直接 damage API、旧 skill projectile、随机流与物品事务调用，结果为 0 命中。Host 自己负责 Actor spawn 与 lifespan，这两项是本阶段明确所有权，不属于越界。

## 7. 最终构建与产物

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

- Game Development：4/4 actions、native 0、25.24s、log SHA `4BD20F6F496ECE6FBD78FF05B125333C987E1B0DF3AC60D44D469AB6799BCBE7`；
- Editor Development：up to date、0 actions、native 0、1.00s、log SHA `75D43894B53A152D8088142E2A8A24D023E63E35FF212A30D138ADE8C75D1D26`；
- `demo_map.exe`：356,271,616 bytes、SHA `1383B6AE4B659982A0B5DF15427C69FEC8E96885DABC461C1A3697C66B229434`；
- `UnrealEditor-demo_map.dll`：14,925,824 bytes、SHA `8CE2F29CA4644B40633743E4C1ED2E83FBC1A2FFD98FD45CB331049EEAFA36BC`。

构建仅验证编译与链接，没有启动产品。

## 8. 边界与提交范围

本轮计划提交 7 个文件：3 个新增源码、2 个回归门禁文件、本 Report 与本 Development Log。未修改 Content、地图、资源、配置、Engine、Windows 或用户设置。

未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。长期未跟踪的 0.0.9B Prompt/Report、旧交接资料、PDF、handoff 与用户资料保持未暂存；`Saved/Codex/P18.2` raw logs 不入 Git。

## 9. 下一阶段建议

P18.3 建立最小产品命令路由，把既有剑动作授权与 Active commit 映射到一个稳定 Sword Qi launch request 和本 Host，并同步 Action termination。继续保持表现与真实输入后置，先关闭产品授权、Run owner 和物理生命周期之间的所有权链。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-2-sword-qi-run-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-2-sword-qi-run-host/Docs/Report/Dev.D.UE.0.0.10.P18.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-2-sword-qi-run-host/Docs/Log/Dev.D.UE.0.0.10.P18.2.r0_log.md>
