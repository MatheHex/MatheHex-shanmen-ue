# Dev.D.UE.0.0.10.P18.8.r0 Report

## 1. 结论

P18.8 在 P 阶段边界内完成，结论为 **PASS**。

本轮在 P18.7 的 Sword Qi 逻辑命令所有者中加入 Run-scoped、容量严格为 1 的 pending retry slot。只有产品路由明确返回 `HostBusy` 时，首次有效样本形成的冻结请求才会进入该槽；后续只能由上层显式 retry 或 cancel，不会自动循环、重新采样或覆盖成另一个命令。

```text
Sword Qi Command Event Owner exact:    6 Success / 0 Fail
Shanmen.0_0_10 full:                 783 Success / 0 Fail
Required legacy groups:              443 Success / 0 Fail
Regression coverage:                  PASS (Changed=5 / Rules=2 / Required=55 / Logs=10)
Regression gate self-test:             PASS 287/287
Boundary scan:                         PASS (Files=2 / Matches=0)
Game + Editor Development:             PASS / native status 0
```

没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件或真实输入，也没有执行截图、Smoke、Cook 或 Package。结论只覆盖无头代码、自动化、构建与静态边界，不宣称键位、表现、手感或人工验收通过。

## 2. 可重试拒绝边界

本轮逐层复核了 Input Adapter、Product Controller 与 Product Session 的拒绝语义。严格分类条件为：

```text
CommandEvent = InputRejected
Input        = ProductRejected + ProductRouteInvoked
Controller   = RouteRejected
Route        = HostBusy
Request      = valid frozen request
```

`HostBusy` 发生在 Product Session 记录终态之前，Host 清空后可以继续提交同一冻结命令。`ActionGateRejected` 及其后的失败均已写入 terminal receipt，精确重放只会返回既有终态，因此不得进入 retry slot。Gameplay gate、无效空间样本、Intent capture 拒绝和其它产品拒绝同样不会新建 pending retry。

## 3. 容量 1 的唯一所有权

`Fdemo_mapShanmenSwordQiCommandEventOwner` 继续是逻辑事件与冻结请求的唯一所有者，并新增一个私有 `PendingRetryRequest`：

- 槽为空时，新逻辑命令按 P18.7 规则执行；
- `HostBusy` 后保存原始 EventId、sequence、origin 与 aim；
- 槽非空时，`TryIssue` 在采样前返回 `PendingRetryOccupied`；
- 槽非空时，通用 `TryReplay` 也在 route callback 前拒绝，避免绕开显式 pending 协议；
- 同一个槽不能被第二个冻结请求替换。

请求新增 `Matches`，以 RunId、InputEventId、sequence 与规范化空间样本共同判断身份。Owner invariant 同时验证 pending request 属于当前 Run 且 sequence 已提交。

## 4. 显式 retry / cancel

新增 `TryRetryPending` 与 `TryCancelPending`：

- retry 只读取槽内冻结请求，调用 callback 时不接受任何外部 sampler；
- 再次 `HostBusy` 或产品调用前拒绝会保留同一 pending request；
- 成功提交或明确不可重试的产品结果会清空槽；
- owner postcondition 失败时保守保留槽；
- cancel 生成包含 RunId 与冻结 Request 的 `Fdemo_mapShanmenSwordQiPendingRetryCancellation`，随后清空槽；
- 空槽不能重复 cancel，也不能执行 pending retry。

结果新增 `bPendingRetryAttempt` 与 `bPendingRetryStored`，区分“这是显式 pending retry”与“请求在本次结果后仍被保存”。这里没有计时器、Tick 重试或循环。

## 5. GameMode 与 Run 清理

`Ademo_mapGameMode` 新增两个设备无关入口：

```text
RetryPendingSwordQiStartCommand(GameplayAllowed)
CancelPendingSwordQiStartCommand(OutCancellation, OutDiagnostic)
```

retry 仍沿既有 `RouteSwordQiStartInput -> RouteSwordQiIntent` 唯一路径执行，只把 owner 保存的冻结 origin/aim 交给 P18.5 adapter。没有新增产品 payload、物品、属性、Actor、投射物、伤害或生命权威。

Run teardown 在清空 owner 前把未消费 pending request 写入 `Fdemo_mapShanmenSwordQiCommandEventEndSummary`，`RunReleased` 日志新增 `SwordQiPendingRetryAtTeardown`，因此取消遗漏不会静默消失。

## 6. 聚焦测试

exact 从 5 增至 6：

1. `BusyRetry` 验证首次 `HostBusy` 保存第二个逻辑命令；
2. pending 存在时，新 issue 与通用 replay 都在 callback 前失败，sequence 不变；
3. Host 仍忙时显式 retry 保留同一 request，不调用原始 sampler；
4. Host 清空、装备与属性发生漂移后再次 retry，仍复用冻结 item、AttackPower、CommandId、origin 与 aim，并成功清槽；
5. `PendingCancelAndTeardown` 验证取消收据、禁止重复取消，以及 Run teardown 对未消费 pending 的审计和清理；
6. 既有 pre-route、cross-Run、applied replay 与 lifecycle 测试新增 no-pending 断言。

Fixture 继续使用 unattended GamePreview world、NullRHI 和真实 ItemAuthority、AttributeComponent、CombatRunCoordinator、P18.4 Product Controller 与 P18.5 Input Adapter。

## 7. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi Command Event Owner exact | 6 | 0 | `0419DC4DA31D02C5B954D705CFAD0C7C866665BA927D9441057463C9240057BE` |
| `Shanmen.0_0_10` full | 783 | 0 | `89AAF0193CCE5C3FE79254B9DC14A21E0E6D1AE2B72D9906133EF8FC7F50CFBA` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `8E1EA4FBC63328E5126FFAAFB0422809B51BF9532F417FA3DB4E48692FFEBBC8` |
| `demo_map.Profile` | 211 | 0 | `2CEC880C7C963FEEF9299A84CE8386DA619E8BC5E9DFC9750E3BBEE887772C96` |
| `demo_map.CodeB` | 60 | 0 | `61C6E0782CB66C1E67595EA8D278D8FD114015E4BB54C24B56256AE0167911FF` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `0E3D45990911648604860CA95B067F2259F005AB04E97A5C909A3B96F37D8A79` |
| `demo_map.P4.Hotbar` | 7 | 0 | `5559E34814A24BAD8676BA073BA76E12FCE6F646733E75AF246C0C0B215C151A` |
| `demo_map.V3` | 29 | 0 | `7036627CF6E67F1AF3A1C7ED75F23A5C0B5CC33EBB8684A7F96484E735AB5AA1` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `A8C9E7D59FE3EEEA70D810E27F8CDA0542962952FCFAE61FBEE2A6888F41934C` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `112E4AE4637FAEAB1969E34824BEDC3D46653CA1C2E7DFB82CECE9BC8617236B` |

每份采用日志都有一个原生 `TEST COMPLETE / EXIT CODE 0`，Result Fail 为 0。完整套件首末 Success 为 `2026.09.03 05:11:14.565 -> 05:43:54.475 UTC`，约 32m39.910s；相对 P18.7 的 782 条增加 1。

## 8. 改动驱动回归与静态门禁

实际 5 个改动源码触发 2 条路径规则和 55 个必跑组：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=55 Logs=10
SELF_TEST: PASS 287/287
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage SHA：`CAE616FD7987D78D8A071EF8451B15A3066EE0DEA18AEF594A519D25429E9162`；
- self-test SHA：`41D356DF8529E4075B30222EF4AD4AE041E1511EF5E4778AFE89D5CB2BEE26B2`；
- boundary SHA：`0D37EA04FB41FF5D5784CA3F2E447A2B54605F170FD31EE541661C2681B4C572`；
- diff-check SHA：`547436D81BD9D3FCECD61E5223E114ABA690765645EF62894DBD389D8B013FE0`。

边界扫描确认生产 owner 仍不依赖 GameplayStatics/ApplyDamage/TakeDamage、旧 SkillProjectile、UWorld/AActor、RNG、库存写事务、物品/属性读取、物理输入绑定、声音或 Niagara。

## 9. 构建、产物与 P/F 边界

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded / native 0 | 27 / 140.36s | `2446E2D4CE0582AE8FFCCBF90945BDDA276A0BF56C3CEF9AB3FC1923DE5A6482` |
| Game Development final | Succeeded / native 0 | 26 / 124.29s | `CE4CB1C19010507478E6009342F763744A3B48D324A7F693944312A6C43FA8C7` |
| Editor Development final | Succeeded / native 0 / up to date | 0 / 1.04s | `A7EE555FAF6D05B2D0BB4B58874FC5984004945A93B0CA27932DA9295C82A225` |

最终产物：

- `demo_map.exe`：356,444,672 bytes，SHA-256 `7BE55B2D6949EADDFB62477A458C0F4FCBC31C45AA27D92B08C3F40EC06AD9AF`；
- `UnrealEditor-demo_map.dll`：15,129,088 bytes，SHA-256 `900E37A517D30B5F9A2D46F584B2882175177184DDE4C9AAFAA308D21D7061DA`。

未修改 Content、地图、资产、配置、Windows、UE Engine 或存档 schema，也未改变装备、库存、属性、伤害或生命权威。真实 Enhanced Input、键位、动画、声音、特效、手感和产品运行继续属于明确授权后的 F 阶段。

## 10. 提交边界、下一阶段与 GitHub

基线提交为 `58f298a0fe0c7bb0d7606b9277cce58246042e0d`。本轮提交边界为 5 个修改源码、本 Report 与本 Development Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；`Saved/Codex/P18.8` raw logs 不进入 Git。

P18.9 建议增加一个只读、设备无关的 Sword Qi command availability projection，把 `CanIssue / CanRetry / CanCancel` 与当前 pending request identity 投影给未来输入/UI 层；它不绑定按键、不触发自动 retry，也不复制 owner 状态。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-8-sword-qi-pending-retry-slot>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-8-sword-qi-pending-retry-slot/Docs/Report/Dev.D.UE.0.0.10.P18.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-8-sword-qi-pending-retry-slot/Docs/Log/Dev.D.UE.0.0.10.P18.8.r0_log.md>
