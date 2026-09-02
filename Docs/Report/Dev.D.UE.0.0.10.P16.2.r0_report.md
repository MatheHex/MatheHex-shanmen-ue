# Dev.D.UE.0.0.10.P16.2.r0 Report

## 1. 结论

P16.2 已把 P16.1 的 Meridian Shock `prepare -> treat -> commit` 唯一路由安装到正式 hotbar 输入、active Run Session 与产品生命周期。

普通物品在识别阶段直接放行；一旦精确识别为 Meridian Shock 治疗物品，该次输入的成功、恢复中或拒绝结果都由治疗产品链处理，不能再落入通用回血入口。Run 结束前先完成治疗事务的 commit-only 恢复；恢复失败时保留 Session 与权威，不允许遗忘未完成事务。

最终结果：

```text
Meridian Shock focused:         11 Success / 0 Fail
Mapped legacy regressions:     519 Success / 0 Fail
Shanmen.0_0_10 full:           724 Success / 0 Fail
Regression coverage:           PASS (Changed=13 / Rules=4 / Required=54 / Logs=10)
Regression gate self-test:     PASS 271/271
Editor + Game Development:     PASS / native status 0
```

## 2. Session 与生命周期

新增 `Fdemo_mapShanmenMeridianShockTreatmentProductSession`：

- 精确绑定完整 active Run correlation 与对应 condition authority；
- 以 RequestId 保存不可变 treatment command；
- 完全相同的请求只重放既有 command；相同 RequestId 的另一 payload 在写入前失败关闭；
- 只标记并重试返回 `RecoveryRequired` 的 command；
- 按 GUID 稳定顺序恢复多个 pending request；
- `TryEnd` 必须先清空 route 的 unresolved recovery，随后才释放 command journal。

新增 `Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle`，把 Session 绑定到真实 ready ShanmenItems authority、由该 authority 重建的 active Run correlation，以及同 Run 的 condition component。生命周期不复制库存、条件、时间或健康真值。

重复绑定只有在 authority、完整 correlation 与 condition component 全部相同时才幂等成功；仅 RunId 相同但 correlation 不同会被拒绝。

## 3. Hotbar 输入所有权

新增 `Fdemo_mapShanmenMeridianShockTreatmentInputAdapter`。每次输入按下列边界处理：

1. 从 durable active Run correlation 读取冻结 hotbar slot；
2. 捕获 ShanmenItems snapshot，校验 content、revision、Owner、Run scope 与当前 product Definition；
3. 非 `MeridianShockTreatment` 语义物品在采样 timeline、分配 ordinal 或创建请求前放行；
4. 精确治疗物品只采样一次 fixed timeline；
5. RequestId 由 CorrelationId、RunId、ItemId、slot、authority revision、timeline sample 与单调 ordinal 确定性派生；
6. 请求只提交给 Lifecycle/Session，不直接写 condition 或 inventory。

`Ademo_mapPlayerController::UseHotbarSlot` 的顺序固定为：

```text
gameplay gate
  -> thrown-weapon typed route
  -> Meridian Shock treatment typed route
  -> generic RequestUseBoundQuickSlot
```

实测普通消耗品不采样 treatment timeline；治疗物品数量 `3 -> 2` 并清除 Meridian Shock；条件已 inactive 时再次按下仍被治疗链处理并拒绝，两个权威 snapshot 不变，不会误触发通用回血。

## 4. Run 接线与恢复顺序

GameMode 在 fixed timeline 与 condition authority 成功启动后绑定 treatment lifecycle，并在后续 Run 启动失败分支中执行对称清理。

Run release 把 treatment lifecycle 放在所有其它产品 authority 拆除之前：

```text
recover pending treatment commit
  -> end treatment route/session
  -> release remaining combat products
  -> release condition/timeline/Run
```

注入“condition 已治疗、item commit 尚未执行”的中断后，Lifecycle 保留 1 个 pending request。Run teardown 调用同一不可变 command，只执行 commit-only 恢复；最终 commit finalize 为 1、cancel finalize 为 0，之后 Session 才清空。

本阶段的 command journal 仍是运行时内存状态。它可防止同一产品运行期间及正常 Run teardown 丢失事务，但不声称进程崩溃后可恢复；持久化 checkpoint 属于后续阶段。

## 5. 新增验证

本轮在既有 7 个 Meridian Shock treatment/route tests 上增加 4 个 P16.2 tests：

- `Session.HotbarOwnership`：普通物品放行、治疗物品独占、inactive 拒绝不落入回血；
- `Lifecycle.TeardownRecovery`：治疗后中断由 teardown 执行 commit-only 恢复；
- `Input.DeterministicIdentity`：相同 canonical parts 得到相同 RequestId，ordinal 变化得到不同 ID；
- `Session.BindingFence`：完整绑定幂等，foreign correlation 失败关闭。

focused 最终为 11/0，原生终止标记存在，Fatal、Unhandled、Ensure 均为 0。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Meridian Shock focused | 11 | 0 | `4BD8B4EC5F8DD6E5A18C811F5DA999AD2AF52891F3F909CAA9A8AF0BDCB691A7` |
| `Shanmen.0_0_10` full | 724 | 0 | `4766651559E14502F5915896DDB69212BB6B07CD9E9E8CC2E684CC8055358905` |
| ItemEconomySchema | 24 | 0 | `6577AC4093E132083699C4515B1392DE498A7D9EC6FE31680401524AFDD157F6` |
| Profile | 211 | 0 | `D86EBE454850F445CEFC17A1CB01BBC60CA73B34796C05BEFF375699063AD690` |
| CodeB | 60 | 0 | `1B5E92B60437EE16ECD8DAECE76085094EFC48483CB0C9B53A6B656E1C643670` |
| ItemUseAndArmor | 46 | 0 | `A242F5FA2ED15CA10302D023F7292F7BB23094119BCA8C82E393367F12F72AED` |
| P4 Hotbar | 7 | 0 | `340778519D013940D51505A7C3A60EE5ED5D6E211CA63E2F7B7F3F495B393AA8` |
| InputRestore | 101 | 0 | `316A5B6D09E86DCE80C04C21FCE606A5FC1179A95673EC3BA0AF2B18EACC3B75` |
| V3 Attributes | 4 | 0 | `D9BEE8BD328875D4BBF8AB1DAA0C7357A05349F3E9D68C8706146B93175BDE9C` |
| EnemySkillFramework | 44 | 0 | `F6EFAFAA4F5D5A6922A94A4DE54C360A1B5C891E7D11FF32A0876A5EBD1541CE` |
| V2RangedCompatibility | 22 | 0 | `343004EDCF2D9F3E784D88B15100B1634834C60915A0A81A83D27A39F6DD614A` |

全量首末 Success 时间为 `05:54:00.257 -> 06:22:05.869 UTC`，用时约 `28m05.61s`；唯一 queue-empty 终止标记存在，原生退出码 0，Fatal、Unhandled、Ensure 为 0。

## 7. 改动—回归门禁与静态边界

回归映射把 route、Session、Lifecycle 与 InputAdapter 合并为 `MeridianShockTreatmentProductFlow`，并给 GameMode、PlayerController、InputRestore 接线补充 focused 与 legacy 证据要求。

```text
REGRESSION_COVERAGE: PASS Changed=13 Rules=4 Required=54 Logs=10
SELF_TEST: PASS 271/271
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS Files=6
GIT_DIFF_CHECK: PASS
```

流程 SHA：gate `4DBFD38148D5302E7CBC54686DD4CC569D37389695EF73DEC05C52CED52E9D7E`；self-test `A34FB07154B570E39D7736BF27B33745DA4F6517015445ADCBA8ED7CA4004E6A`；JSON `DA06834B2330D35329656BF228F7DB0E98064DAFD695940C400B96E4EFBC1E83`；boundary `6B2E1A69CAAFBCAEE47132A5A2718A46A41C1B98450E4D0D3983A3FC727402BD`。

边界扫描确认新三层不依赖旧 `demo_mapItemSubsystem`、通用 quick-slot 写入口、`UWorld`、`AActor`、Spawn/ApplyDamage、RNG、Tick/Timer、Enhanced Input 或 UI。正式 UI 仍只产生输入意图。

## 8. UE 5.8 测试环境诊断

第一次全量尝试在 579/0 后主动停止：UE 5.8 Editor HomeScreen 后台反复访问 `google.com/generate_204`，产生 69 条原始/转发记录与 53 次 large-delta，后段约 82 秒/项。第二次普通 `-HomeScreen.EnableHomeScreen=0` 参数未改变只读 CVar，确认无效后在 554/0 停止。

源码定位到 UE 5.8 `SHomeScreen::CheckInternetConnection`，随后使用引擎支持的进程级 `-ForceDPCVars=HomeScreen.EnableHomeScreen=0`。probe 1/0 且网络请求为 0；最终全量 724/0 且网络请求为 0。未修改 Windows、Engine 或用户配置。两份中断日志分别为：

- `7AC2CE3B7ED4AE2F851F2B5FA7D158B1A7154B5D60DFE5B941B9F363FD2DFBA7`；
- `B969781A56F5B84063477CB02D6BC8817CCF23B8E751DCA7C2DD58A907CDF9F9`。

它们缺少终止标记，只作为环境诊断保留，不作为通过证据。最终日志启动阶段仍有 UE 自带 UnifiedError/AutomationTest 的 13 条 `Condition failed` 自检输出；这些发生在 Engine 初始化前，不属于所选测试，最终 controller 仍为 724 Success / 0 Fail。

## 9. 构建证据

本轮使用 `-NoUBA -MaxParallelActions=1` 控制提交内存压力。

| Target | Result | Native status | Log SHA-256 |
|---|---|---:|---|
| Game Development | Succeeded (30 actions / 129.02s) | 0 | `766B93B977BF80A28FA924E22A796AE23D05C655123C53A93A854892DD263697` |
| Editor Development | Succeeded, up to date (0 actions / 0.99s) | 0 | `A376433754336706CF784184EDFB00C83F4DA65C38E618DEFDB1DAA42DF4ABFF` |

最终产物：

- `demo_map.exe`：355,881,984 bytes，SHA-256 `4CF99F45E7828B4E2CE20B6A53BB7EE0D28D6CE4AA3CD44233851B844D322DBA`；
- `UnrealEditor-demo_map.dll`：14,497,280 bytes，SHA-256 `08C24517AE06596CBB08A5DBACF348D340FACAF4DC3303E85B25464ADFD6A429`。

## 10. 修改范围与 P/F 边界

本轮只修改 treatment route getter、新 Session/Lifecycle/InputAdapter、GameMode/PlayerController hotbar 与 Run 接线、InputRestore 顺序测试、Meridian Shock route tests、回归映射，以及 Report/Log。

raw logs 仅保存在本机 `Saved/Logs`。长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

本 Report 只证明 P 阶段 C++ 产品接线、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

下一阶段建议为 treatment Session 增加可验证的 durable checkpoint/restore 契约，然后再进入 F 阶段真实世界输入与可视验收；不能把本轮运行时恢复描述为进程崩溃恢复。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-2-meridian-shock-treatment-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-2-meridian-shock-treatment-session/Docs/Report/Dev.D.UE.0.0.10.P16.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-2-meridian-shock-treatment-session/Docs/Log/Dev.D.UE.0.0.10.P16.2.r0_log.md>
