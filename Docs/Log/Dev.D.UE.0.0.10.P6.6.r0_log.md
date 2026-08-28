# Dev.D.UE.0.0.10.P6.6.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.6.r0`；
- 基线提交：`d4c31b4672802df5f558aeaa99021a48e5ee69d4`（P6.5）；
- 分支：`agent/0.0.10-p6-6-controlled-weapon-run-lifecycle`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.5 已有多实例 Run Host，但尚未接入真实产品 owner。若由未来的输入、Actor 或 presentation 自行决定 Run teardown，就会在 `Ademo_mapGameMode` 已持有的 `Fdemo_mapCombatRunCoordinator` 之外产生第二套生命周期，并可能先释放 entity Registry alias、后遗留仍 active 的 controlled-weapon Session。

现有 GameMode 已负责 Run activation、Preparation deactivation 与 EndPlay，因此 P6.6 只在这个唯一 owner 上增加有序 transaction；不为尚未冻结的 spawn、input 或 flying-sword content 作决定。

## 实现记录

### Ordered Host -> Coordinator transaction

`Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun` 先读取 active Coordinator 的 exact Run identity，再复制 Host。副本内按稳定 item 顺序中断所有 active Controller，并退休所有 terminal Controller；任一步拒绝时丢弃副本，真实 Host 与 Coordinator 均不变。

只有 `Coordinator.TryEndRun(exact RunId)` 成功后，真实 Host 才被清空后的副本替换。Coordinator 拒绝时 result 保留诊断，调用方可以修复外部 identity 后重试。Result 同时记录初始 bound / active 数量、interrupt receipts 与 retired 数量，防止 teardown 被一个模糊 bool 吞掉。

### Unique GameMode ownership

GameMode 新增 `ControlledWeaponRunHost` 成员，以及使用真实 Coordinator、Player Pawn 和外层注入 physical Actor / collision root / motion 的 attach 入口。它不 spawn Actor，不选择 item、数量、速度、编队或输入。

`TryActivateCombatRun` 在开始新 Run 前拒绝非空旧 Host。Preparation deactivation 和 EndPlay 统一调用 `ReleaseControlledWeaponCombatRun`；EndPlay 先完成 controlled-weapon interruption、terminal retirement 与 Coordinator identity release，再销毁敌人和 extraction content。

生命周期工具对 Coordinator rejection 保留 Host；GameMode 在记录精确错误后仍执行 Host + Coordinator emergency reset。这是产品 teardown 的最终 fail-safe，不改变工具层可诊断、可重试的事务语义。

### Regression routing

新增 `ControlledWeaponRunLifecycle` 路径规则，强制 Lifecycle、RunHost、Controller、Coordinator、Items、WorldGameplay 与 CombatRuntime。GameMode 既有规则要求完整 `Shanmen.0_0_10`；RunHost header 变更继续触发 P6.5 的 Adapter / Session / WorldDelivery 依赖链。最终并集为 11 个测试组。

Self-test 新增 Lifecycle full-suite pass 与 coordinator-only fail-closed 两条，结果从 `10/10` 增至 `12/12`。

## 自动化覆盖

- mixed terminal + active Host 的 ordered interrupt / retirement；
- 只为 active item 生成 interrupt receipt，所有 bound item 均退休；
- Host 清空、Coordinator inactive、player vitality entity unbound；
- 已释放 Run 的重复 teardown 被 CoordinatorNotActive 拒绝；
- Host / Coordinator Run mismatch 在任何 mutation 前失败；
- Coordinator identity mismatch 后 real Host 仍 valid / active；
- identity 修复后 exact retry 提交一次 interruption 并释放 Run；
- 全量回归覆盖 GameMode 编译接线与所有既有 0.0.10 契约。

## 首次验证

首次 Editor integration build：`26/26` actions，`Result: Succeeded`，native exit `0`，`121.86s`。

首次 focused 日志 `p66_run_lifecycle_first.log`：`3/3` Success、Fail `0`、queue empty、native exit `0`、SHA-256 `9B7249C97C28F365FEB7ACCDF5E61305A5A328879FF72753F4D3BEA916C3C30C`。

首次 full 日志 `p66_full_first.log`：`149/149` Success、Fail `0`、queue empty、native exit `0`、SHA-256 `BA759F9523DD50A4EABF5BDFFF2D1BD6084C8E95D41942601F7BC132DE562CD2`。

没有源码、测试或编译失败。契约复审确认最终源文件早于已测试 Editor DLL，且 EndPlay identity release 早于 actor / alias destruction。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p66_run_lifecycle_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunLifecycle` | 3 | 0 | 0 | `FC74FE05E86095B01BA4B6FF05A1B4B6F11B4BC06DDD101157A7DF1CFF5FD182` |
| `p66_run_host_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 3 | 0 | 0 | `9880DAAD076FA7CD412E31F62B2BFE7E2F68E8926A46FCE1D7F2996FC695AE02` |
| `p66_controller_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponController` | 3 | 0 | 0 | `82160F1E4304B4F2217BAE72158E22C50B4BBEEBC6CAE9029C43D73C385ABD64` |
| `p66_world_delivery_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `19A339D74BFD0AE1B57D9D0D0DBB695F1347D5BA67C724E7DEB7591283640EBF` |
| `p66_session_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `85E62D55C54AC9E7984C8E36979538A27D06C8128671B2809FA6A6B06940546A` |
| `p66_adapter_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `3BE2B362B386F5D99DCFCC5147EE939C22C92B15BA32BEC6B69A82590429EE0C` |
| `p66_coordinator_final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `E9DBB477C70D7400A574A0A4D81724E6E98B263BF0046981AA998054698B055D` |
| `p66_items_final.log` | `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `50312E9AAE87838D1024AE84C46740BBEB35B6EF6734AC0AA377179B6CBE1C13` |
| `p66_world_final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `CCED4B9A4438764EDDDA34444A3CB34BF45CFB59CED1C38A9D06C7B275EB159A` |
| `p66_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `0BC93E73178DE7E05B7A6986BC4CD35A5AF069312A1BC22E59EFB62BC97C7212` |
| `p66_full_final.log` | `Shanmen.0_0_10` | 149 | 0 | 0 | `D7A9A34EF0941898BB4BD02191309530FE7F96197905964568D308B9B614F525` |

十一条日志均存在一个实际 `Cmd: Automation RunTests`、至少一个 Success、Fail `0`、queue empty、引擎退出状态 `0`，且 Fatal / unhandled / ensure 为 `0`。全量唯一计数 `149/149`。

## Changed-file gate

精确暂存前的生产与脚本路径检查：

```text
REGRESSION_COVERAGE: PASS Changed=8 Rules=3 Required=11 Logs=11
```

加入本 Report / Log 后的最终 staged gate：

```text
REGRESSION_COVERAGE: PASS Changed=10 Rules=3 Required=11 Logs=11
```

Required groups 为 Lifecycle、RunHost、Controller、WorldDelivery、Session、Adapter、Coordinator、Items、WorldGameplay、CombatRuntime 与完整 `Shanmen.0_0_10`；11 份对应日志全部被 gate 读取并验证 SHA。

## 构建

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `26/26` actions；
- `Result: Succeeded`；
- native exit `0`；
- `121.86s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `25/25` actions；
- `Result: Succeeded`；
- native exit `0`；
- `102.00s`；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 静态与兼容性

- regression JSON parse：PASS；
- regression coverage self-test：`12/12 PASS`；
- 新 Lifecycle 文件及 GameMode 新增行中的 damage / spawn / input / inventory / resource / RNG：0；
- `git diff --check` 与最终 `git diff --cached --check`：native exit `0`；
- Profile schema、存档、item definition、GameplayTags 与 input mapping 未改；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

P6.7 可在策划冻结正式 flying-sword Actor / spawn 与输入语义后，将外部 presentation 创建的 exact item 调用本轮 GameMode attach 入口；仍不应让 presentation 复制 deployed-item、Run、vitality 或 impact authority。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-6-controlled-weapon-run-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P6.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-6-controlled-weapon-run-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P6.6.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-6-controlled-weapon-run-lifecycle>
