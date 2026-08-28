# Dev.D.UE.0.0.10.P6.6.r0 Report

## 1. 结论

P6.6 已完成受控武器 Host 与真实 Combat Run owner 的有序生命周期接线，结论为 **PASS**。

`Ademo_mapGameMode` 继续作为唯一产品 Run owner：外部生成的 exact controlled weapon 可附加到其现有 `Fdemo_mapCombatRunCoordinator`，Preparation deactivation 与 `EndPlay` 则统一经过 `Fdemo_mapShanmenControlledWeaponRunLifecycle`。所有 active weapon Session 先中断、所有 terminal item 再退休，最后才释放 Coordinator 的 Run / entity identity。

本轮没有新增输入、Actor class、spawn、飞剑数量、编队、默认速度、物品定义或内容数值。

## 2. 功能性

- 新生命周期事务只接受 active Coordinator，并冻结其 exact `RunId`；非空 Host 必须结构有效且属于同一 Run；
- Host teardown 先在值副本上执行：按稳定 item 顺序中断全部 active item，再退休全部 terminal item；任何 Host 阶段失败都不修改真实 Host 或 Coordinator；
- Coordinator 接受 exact Run end 后才把清空后的 Host 副本提交给真实 owner；Coordinator 拒绝时真实 Host 保持原状态，便于诊断与精确重试；
- 结果收据记录 bound、active、interrupted、retired 数量以及逐 item interrupt receipt；`IsEnded` 同时校验 Run identity 与数量守恒；
- GameMode 暴露 `AttachControlledWeaponToActiveCombatRun`，只组合既有 P6.1 Prepared、P6.5 Host 和真实 Coordinator；Weapon Actor、collision root 与 motion 仍由外层显式注入；
- 新 Run 激活前拒绝残留的非空 controlled-weapon Host，避免上一 Run 的 item 被静默带入下一 Run；
- Preparation deactivation 与 `EndPlay` 共用一个 release helper；`EndPlay` 在销毁敌人、提取区及 Registry aliases 之前释放 controlled weapon / Run identity；
- 生命周期工具保留 Coordinator 拒绝后的真实 Host；GameMode 的最终错误分支仍执行显式 Host + Coordinator emergency reset，以保持既有产品 teardown 的 fail-safe 行为，并记录错误日志。

## 3. 完整性

新增 3 个产品级自动化：

1. `MixedStateTeardown`：一把已 Completed、一把仍 Directed，验证只中断 active item、两项均退休、Host 清空且 Coordinator identity 释放；
2. `RunMismatchFence`：Host 与 Coordinator 分属不同 Run 时在任何状态写入前拒绝，两个 Coordinator 与真实 Host 均保持不变；
3. `CoordinatorRejectionPreservesHost`：注入 player vitality identity 不一致，验证 Coordinator 拒绝后真实 Host 仍 active；修复 identity 后 exact retry 成功。

全量 `Shanmen.0_0_10` 从 P6.5 的 `146` 个增加到 `149` 个，最终 `149/149` Success。

## 4. 兼容性

- 未建立第二个 Run、entity、vitality、inventory 或 deployed-item authority；
- 未修改 P6.1 Adapter、P6.2 Session、P6.3 World Adapter、P6.4 Controller、CombatCore、CombatRuntime、WorldGameplay 或 Items 生产契约；
- P6.5 Host 仅新增只读 `RunId` / `SourceEntityId` 访问器，没有改变 attach、movement、contact 或 interrupt 语义；
- 新生命周期生产文件不调用 `ApplyDamage` / `TakeDamage`，不直接修改库存、资源或 Actor 世界状态；
- 未修改 Profile schema、存档格式、item definition、GameplayTags 或输入映射；
- 未把 legacy `SkillProjectile` 解释为 controlled weapon，也未冻结正式 flying-sword presentation。

## 5. 修改范围

- `Source/demo_map/demo_mapShanmenControlledWeaponRunLifecycle.h/.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunLifecycleTests.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.h`；
- `Source/demo_map/demo_mapGameMode.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Log。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponRunLifecycle` | 3 | 0 | 0 | `FC74FE05E86095B01BA4B6FF05A1B4B6F11B4BC06DDD101157A7DF1CFF5FD182` |
| `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 3 | 0 | 0 | `9880DAAD076FA7CD412E31F62B2BFE7E2F68E8926A46FCE1D7F2996FC695AE02` |
| `Shanmen.0_0_10.Product.ControlledWeaponController` | 3 | 0 | 0 | `82160F1E4304B4F2217BAE72158E22C50B4BBEEBC6CAE9029C43D73C385ABD64` |
| `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `19A339D74BFD0AE1B57D9D0D0DBB695F1347D5BA67C724E7DEB7591283640EBF` |
| `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `85E62D55C54AC9E7984C8E36979538A27D06C8128671B2809FA6A6B06940546A` |
| `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `3BE2B362B386F5D99DCFCC5147EE939C22C92B15BA32BEC6B69A82590429EE0C` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `E9DBB477C70D7400A574A0A4D81724E6E98B263BF0046981AA998054698B055D` |
| `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `50312E9AAE87838D1024AE84C46740BBEB35B6EF6734AC0AA377179B6CBE1C13` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `CCED4B9A4438764EDDDA34444A3CB34BF45CFB59CED1C38A9D06C7B275EB159A` |
| `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `0BC93E73178DE7E05B7A6986BC4CD35A5AF069312A1BC22E59EFB62BC97C7212` |
| `Shanmen.0_0_10` | 149 | 0 | 0 | `D7A9A34EF0941898BB4BD02191309530FE7F96197905964568D308B9B614F525` |

- 十一条最终日志均有唯一实际 `Cmd: Automation RunTests`、queue-empty、Fail `0`、退出状态 `0`，Fatal / unhandled / ensure `0`；
- changed-file gate：`PASS Changed=10 Rules=3 Required=11 Logs=11`；
- regression coverage self-test：`12/12 PASS`；
- regression JSON parse：PASS；
- 新生命周期生产/测试文件边界扫描，以及 GameMode 新增行扫描：damage / spawn / input / inventory / resource / RNG 命中 `0`；
- `git diff --check` 与最终 `git diff --cached --check`：native exit `0`。

## 7. 首次验证与修正

首次 Editor integration build 为 `26/26` actions、`Result: Succeeded`、native exit `0`、`121.86s`。

首次 focused 日志 `p66_run_lifecycle_first.log` 为 `3/3` Success、Fail `0`、queue empty、native exit `0`、SHA-256 `9B7249C97C28F365FEB7ACCDF5E61305A5A328879FF72753F4D3BEA916C3C30C`。

首次 full 日志 `p66_full_first.log` 为 `149/149` Success、Fail `0`、queue empty、native exit `0`、SHA-256 `BA759F9523DD50A4EABF5BDFFF2D1BD6084C8E95D41942601F7BC132DE562CD2`。

没有源码、测试或编译失败。最终契约复审确认 EndPlay 的 release 顺序早于敌人 / extraction / Registry alias 销毁，并确认 Coordinator rejection 测试覆盖真实 Host 不变与修复后重试。

## 8. 编译

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

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-6-controlled-weapon-run-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-6-controlled-weapon-run-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P6.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-6-controlled-weapon-run-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P6.6.r0_log.md>
