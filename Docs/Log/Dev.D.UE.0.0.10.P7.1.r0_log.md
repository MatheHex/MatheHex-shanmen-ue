# Dev.D.UE.0.0.10.P7.1.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.1.r0`；
- 基线提交：`aac3fecaddce2f9bf19b514f36b7756b42862d5e`（P7.0）；
- 分支：`agent/0.0.10-p7-1-thrown-item-transaction`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 问题与目标

P7.0 已提供 exact physical item 的纯 Runtime 直线 launch，但没有权利修改持久物品。旧 `ConsumePreparedRunItem` 是单步 direct consume：若在真实 launch 前调用，后续 Actor/物理失败会提前扣除；若在 launch 后调用，进程在两者之间退出又会恢复已投掷物品。

P7.1 将该跨权威窗口改为两阶段 saga：先持久 prepare 外部 action intent，真实不可逆 launch 后再持久 commit，launch 前取消则 cancel。所有 balance 仍由 `ShanmenItems` 单一权威持有。

## 核心数据编码

没有新增 receipt 字段或 schema。既有字段按操作解释：

```text
Prepare:
  ReservationId  = IntentId / ActivationId
  ItemInstanceId = exact thrown item
  ReservationIds = [ActiveRunId]

Finalize:
  ReservationId  = IntentId / ActivationId
  ItemInstanceId = exact thrown item
  ReservationIds = [ActiveRunId, PrepareRequestId]
  Phase          = Committed | Cancelled
```

prepare 的 `ResourceBefore == ResourceAfter`，`AvailableAfter` 扣除 pending amount；commit 才令 `ResourceAfter = ResourceBefore - Amount`。cancel 的 before/after 相等。

## Repository 与持久服务

新增 request validity、command fingerprint、repository prepare/finalize、authority service durable wrapper 和 GameInstance subsystem facade。

repository 会：

- 从 committed preparation Quantity reservation 与按 revision 排序的 direct/intent commit receipts 重建 active-Run balance；
- 校验 claim、Run 未 final、exact item/owner/scope、CAS 前值和 reservation 唯一性；
- 禁止同 item 并发 pending intent；
- 禁止 pending 间隙的 direct consume 与 Run finalize；
- 验证 prepare/finalize 的 revision 顺序、守恒、终态唯一性和 reload invariants；
- 通过现有 copy-on-write durable service 在写失败时回滚内存 document。

terminal Run release 后，历史 Quantity reservation 合法状态为 `Released`；invariant 只在存在匹配 Run finalize 时接受该状态。

## Product adapter

`Fdemo_mapShanmenThrownWeaponItemAdapter` 分为纯 evidence builders 与 Game Thread authority facade：

- `BuildPrepareRequest / PrepareActiveRun`；
- `BuildCommitRequest / CommitLaunched`；
- `BuildCancelRequest / CancelBeforeLaunch`。

它重建 active-Run correlation，要求 exact `Item.Weapon.Thrown + Quantity` definition，把 P7.0 Action ActivationId 用作 IntentId。只有匹配 Action 的 immutable launch receipt 打开 commit point。适配器没有 World、Actor、输入、移动或 inventory mutation API。

## 重启恢复交叉审查

在最终回归前检查所有 `ConsumePreparedRunItem` 的 balance consumer，发现 `demo_mapShanmenRunLifecycleAdapter::BuildRuntimePlan` 只减 direct receipts。若两阶段 commit 后重启，authority ledger 正确但 transient Runtime 会重建旧数量。

修复后 plan 同时读取 exact active Run 的：

- successful direct consume；
- successful `FinalizePreparedRunQuantityIntent` with `Committed` phase。

`QuantityIntentRestartProjection` 通过真实 cutover、Start、durable prepare/commit、GameInstance restart、BindExisting 和 resumed materialization 验证 `3 -> 2`。

## 自动化增量

新增核心测试：

- `Shanmen.0_0_10.Items.PreparedRunQuantityIntent`；
- `Shanmen.0_0_10.Items.AuthorityService.PreparedRunQuantityIntentDurability`；
- `Shanmen.0_0_10.Items.RunLifecycle.QuantityIntentRestartProjection`。

新增产品测试：

- `EvidenceAndDeterminism`；
- `LaunchCommitAndReplay`；
- `PreLaunchCancel`；
- `FailClosed`。

## 最终自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p71_restart_projection_final.log` | `Shanmen.0_0_10.Items.RunLifecycle.QuantityIntentRestartProjection` | 1 | 0 | 0 | `418F19CF44F1880E51D9869EDEA3D5134F5BDF5791EC18DEC30F91528F839097` |
| `p71_thrown_adapter_final.log` | `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter` | 4 | 0 | 0 | `F468A1178D42307705BCFF8BCA8B9E5F506C1D67382DC6D3AF78D377BAEA662E` |
| `p71_items_final.log` | `Shanmen.0_0_10.Items` | 72 | 0 | 0 | `2915D4412BB9626E3CA3DA983F9C5B1E75C62320A374A4D27F1381A12BCFEC61` |
| `p71_combat_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 30 | 0 | 0 | `89D9577D992040A1F84952D080925C479F1F32C652CE0649D15EC5C3ACFBE285` |
| `p71_itemuse_final.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `D948F972307BE86AFA0BA7DFB97EBFB3707FF2D2223E2CDB0CAB6753024BDECC` |
| `p71_hotbar_final.log` | `demo_map.P4.Hotbar` | 7 | 0 | 0 | `09CD63DCD0EEC119EBD19FBCEFCE98E6B19C741627BB51700D6157E19A9C3327` |
| `p71_profile_final.log` | `demo_map.Profile` | 211 | 0 | 0 | `714A89B2634E83EFD584EA13AF13931E7BA6B591C40D97C941E31DFBFD6FEA19` |
| `p71_codeb_final.log` | `demo_map.CodeB` | 60 | 0 | 0 | `8E8F33A15867F488909B29207AC73EC531ABFFE1CF87DE87218825AB643EECF6` |
| `p71_v2ranged_final.log` | `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `84EEA7106E92076AB8AA567E8ACA5F8689021CFC2F4C433DB185D505A91FE127` |
| `p71_full_final.log` | `Shanmen.0_0_10` | 182 | 0 | 0 | `D611CB58A163EF1EF8A2B73A3FD97158CA72DAA34F71E1BB61919A651493FC07` |

每份最终日志只有一个实际 RunTests、queue-empty、Fail `0` 和 bad terminal pattern `0`。测试发现前保留项目既存的 Automation discovery 噪声，但目标 ControllerResults 全部 Success。

## 首次失败与修复

1. `p71_quantity_intent_initial.log`：`0/1`，SHA-256 `77BCD940FE466A3CA1926A8B0CE5A3836DC45CE67B8434A84AD0136D93BE1B00`。首次 terminal extraction/reload 暴露 invariant 只接受 `Committed` reservation，未接受匹配 Run finalize 后的合法 `Released`。修正后 focused、Items 和 full 均通过。
2. 首次 product Editor compile：native exit `1`、`OtherCompilationError`。确定性 canonical parts 把 `FName Version` 当作 `FString`；改为 `.Version.ToString()` 后构建成功。
3. `p71_focused_initial.log`：`4 Success / 1 Fail`，SHA-256 `FC82222558F9544344374A4ABF7FA13C654BD243ABEE64B1087CD14C391D5A42`。FailClosed 测试以 package 作为 `GameInstanceSubsystem` Outer，触发 handled ensure；改用 rooted transient `UGameInstance.Init/Shutdown` 后解决。
4. 最终静态交叉审查发现 restart projection 漏计 intent commit；补生产修复与真实 restart 回归后重新运行全部最终日志。

## Changed-file gate

最终生产改动路径命中 Items、ItemProductAdapters、ProductRunItemUse 与 ThrownWeaponItemAdapter 四条映射：

```text
REGRESSION_COVERAGE: PASS Changed=17 Rules=4 Required=9 Logs=9
SELF_TEST: PASS 17/17
```

九个 required group 为：`Shanmen.0_0_10`、Items、CombatRuntime、ThrownWeaponItemAdapter、`demo_map.Profile`、CodeB、ItemUseAndArmor、P4.Hotbar、V2RangedCompatibility。

## 构建时间线

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

1. core Editor integration：`68/68`，Succeeded，native exit `0`，`191.56s`；
2. product adapter 首次 compile：Failed，native exit `1`，类型转换源码错误；
3. 最终 Editor：`6/6`，Succeeded，native exit `0`，`11.73s`；
4. 最终 Game：`65/65`，Succeeded，native exit `0`，`185.19s`。

最终 Editor product DLL UTC：`2026-08-29T10:02:28Z`；Game executable UTC：`2026-08-29T10:10:20Z`。Game 只构建，未启动。

## 静态、范围与兼容性

- `git diff --check`：native exit `0`；
- mapping self-test `17/17`；
- 无临时 debug marker、TODO、FIXME 或 HACK；
- 无 schema version、Build.cs、GameplayTags、资产、GameMode 或 input 改动；
- 新 product adapter 无 World/Actor/ApplyDamage/Spawn/Tick/timer/RNG；
- 旧 direct consumable、Profile、CodeB 与 Hotbar 全部回归通过；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 下一阶段

P7.2 应建立一次性 straight thrown World delivery：消费 immutable launch receipt，生成/推进一个无 steering/recall 的 transient projectile，把命中回送 P7.0 candidate，并在真实 launch commit point 调用 P7.1。输入、弧线和自动路径继续分阶段后置。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-1-thrown-item-transaction/Docs/Report/Dev.D.UE.0.0.10.P7.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-1-thrown-item-transaction/Docs/Log/Dev.D.UE.0.0.10.P7.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-1-thrown-item-transaction>
