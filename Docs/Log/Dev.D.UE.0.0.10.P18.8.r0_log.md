# Dev.D.UE.0.0.10.P18.8.r0 Development Log

## 1. 目标与基线

- 基线提交：`58f298a0fe0c7bb0d7606b9277cce58246042e0d`（P18.7 Frozen Command Request）；
- 分支：`agent/0.0.10-p18-8-sword-qi-pending-retry-slot`；
- 目标：为明确可重试的 Sword Qi `HostBusy` 拒绝增加 Run-scoped、容量 1、仅显式 retry/cancel 的冻结请求槽；
- 约束：不自动循环、不重新采样、不绑定真实输入，不拥有产品 payload、物品、属性、库存、Actor、投射物、伤害或生命权威。

## 2. 拒绝分类复核

沿 P18.5 input -> P18.4 controller -> P18.3 product session 检查所有状态。只有 `ProductRejected / RouteRejected / HostBusy` 在 Session 记录 terminal receipt 之前返回，可在 Host 清空后重新提交同一 frozen command。

`ActionGateRejected` 及后续失败会 `RecordTerminal`；相同 CommandId 的 replay 返回既有终态，不是 retry。pre-route input 拒绝没有进入产品 Host，也不能自动制造 pending。分类因此采用四层状态与有效 request 联合判断，避免把枚举名相似但语义不同的失败纳入。

## 3. Pending retry slot

在既有 `Fdemo_mapShanmenSwordQiCommandEventOwner` 内增加一个私有 `PendingRetryRequest`，没有创建第二套 ledger。`Request::Matches` 核对完整事件身份和冻结 sample。

slot 非空时，fresh issue 和通用 replay 都在任何 sample/route callback 前返回 `PendingRetryOccupied`。这保证容量严格为 1，旧 request 不会被覆盖，也强制调用方选择显式 retry 或 cancel。

Owner validity 现在要求 pending request 属于 active Run 且 sequence 小于 next committed sequence；empty 状态同时要求 slot 为空。

## 4. 显式状态迁移

`TryRetryPending` 复制槽内 request 后调用 frozen route，不暴露 sampler。结果迁移规则：

```text
HostBusy again                 -> retain same pending request
pre-product rejection         -> retain same pending request
owner postcondition failure   -> retain same pending request
accepted product route        -> clear pending request
other invoked product result  -> clear pending request
```

`TryCancelPending` 先构造并验证 cancellation receipt，再清槽并验证 owner postcondition。空槽取消失败关闭。Run end summary 在 reset 前复制 pending request，用于 teardown 审计。

## 5. GameMode 组合

新增 `RetryPendingSwordQiStartCommand` 与 `CancelPendingSwordQiStartCommand`。retry 通过 frozen origin/aim 调用既有 `RouteSwordQiStartInput`，随后继续走唯一 `RouteSwordQiIntent`。Issue、P18.5 adapter 与 P18.4 controller 的权威没有复制。

`RunReleased` 日志增加 `SwordQiPendingRetryAtTeardown`；Run begin/end 顺序保持不变。

## 6. Automation 改动

`BusyRetry` 扩展为完整 slot 生命周期：

- 第 2 个命令 `HostBusy` 后进入 slot；
- fresh issue 与 generic replay 不调用回调；
- Host 仍忙时 explicit retry 不重新采样并保留 request；
- Host retirement 与 authority drift 后 explicit retry 复用 frozen item、AttackPower、CommandId 和 sample，随后清槽。

新增 `PendingCancelAndTeardown`：验证 cancellation receipt、禁止重复取消，以及带 pending 的 Run teardown summary。其余测试补充 non-retryable rejection 不进入 slot 的断言。exact 从 5 增至 6，full 从 782 增至 783。

## 7. 最终测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_sword_qi_command_event_owner.log` | exact | 6/0 | `0419DC4DA31D02C5B954D705CFAD0C7C866665BA927D9441057463C9240057BE` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 783/0 | `89AAF0193CCE5C3FE79254B9DC14A21E0E6D1AE2B72D9906133EF8FC7F50CFBA` |
| `automation_item_economy_schema.log` | legacy | 24/0 | `8E1EA4FBC63328E5126FFAAFB0422809B51BF9532F417FA3DB4E48692FFEBBC8` |
| `automation_profile.log` | legacy | 211/0 | `2CEC880C7C963FEEF9299A84CE8386DA619E8BC5E9DFC9750E3BBEE887772C96` |
| `automation_code_b.log` | legacy | 60/0 | `61C6E0782CB66C1E67595EA8D278D8FD114015E4BB54C24B56256AE0167911FF` |
| `automation_item_use_and_armor.log` | legacy | 46/0 | `0E3D45990911648604860CA95B067F2259F005AB04E97A5C909A3B96F37D8A79` |
| `automation_p4_hotbar.log` | legacy | 7/0 | `5559E34814A24BAD8676BA073BA76E12FCE6F646733E75AF246C0C0B215C151A` |
| `automation_v3.log` | legacy parent | 29/0 | `7036627CF6E67F1AF3A1C7ED75F23A5C0B5CC33EBB8684A7F96484E735AB5AA1` |
| `automation_enemy_skill_framework.log` | legacy | 44/0 | `A8C9E7D59FE3EEEA70D810E27F8CDA0542962952FCFAE61FBEE2A6888F41934C` |
| `automation_v2_ranged_compatibility.log` | legacy | 22/0 | `112E4AE4637FAEAB1969E34824BEDC3D46653CA1C2E7DFB82CECE9BC8617236B` |

全部采用日志 Result Fail 为 0，且各含一个 native terminal success marker。

## 8. 门禁与覆盖

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=55 Logs=10
SELF_TEST: PASS 287/287
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

对应 SHA：coverage `CAE616FD7987D78D8A071EF8451B15A3066EE0DEA18AEF594A519D25429E9162`；self-test `41D356DF8529E4075B30222EF4AD4AE041E1511EF5E4778AFE89D5CB2BEE26B2`；boundary `0D37EA04FB41FF5D5784CA3F2E447A2B54605F170FD31EE541661C2681B4C572`；diff-check `547436D81BD9D3FCECD61E5223E114ABA690765645EF62894DBD389D8B013FE0`。

## 9. 构建与产物

- Editor initial：27 actions、native 0、140.36s、SHA `2446E2D4CE0582AE8FFCCBF90945BDDA276A0BF56C3CEF9AB3FC1923DE5A6482`；
- Game final：26 actions、native 0、124.29s、SHA `CE4CB1C19010507478E6009342F763744A3B48D324A7F693944312A6C43FA8C7`；
- Editor final：up to date、0 actions、native 0、1.04s、SHA `A7EE555FAF6D05B2D0BB4B58874FC5984004945A93B0CA27932DA9295C82A225`；
- `demo_map.exe`：356,444,672 bytes、SHA `7BE55B2D6949EADDFB62477A458C0F4FCBC31C45AA27D92B08C3F40EC06AD9AF`；
- `UnrealEditor-demo_map.dll`：15,129,088 bytes、SHA `900E37A517D30B5F9A2D46F584B2882175177184DDE4C9AAFAA308D21D7061DA`。

## 10. 提交边界与后续

本轮精确提交 7 个文件：5 个修改源码、本 Report 与本 Development Log。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；`Saved/Codex/P18.8` raw logs 不入 Git。

未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。P18.9 建议增加只读 command availability projection，供未来输入/UI 查询 `CanIssue / CanRetry / CanCancel`，不绑定真实输入且不复制 owner 状态。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-8-sword-qi-pending-retry-slot>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-8-sword-qi-pending-retry-slot/Docs/Report/Dev.D.UE.0.0.10.P18.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-8-sword-qi-pending-retry-slot/Docs/Log/Dev.D.UE.0.0.10.P18.8.r0_log.md>
