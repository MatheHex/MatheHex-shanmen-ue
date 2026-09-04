# Dev.D.UE.0.0.10.P19.9.r0 Report

## 1. 结论

P19.9 已完成 Run-scoped Divine Sense Logical Input Adapter。

新适配器位于 P19.8 Product Route 之前，只接收一次设备无关的逻辑 `use`，并把它至多委托一次。它不读取键位、不绑定 Enhanced Input、不持有 UI，也不搜索 Actor。P19.8 及其下游仍分别拥有产品身份、Controller、资源、World evidence、journal 与 replay 权威。

当 P19.8 返回带有效 route-issued attempt 的 typed Controller rejection 时，适配器只保留这一份 attempt，并进入容量为一的 `RetryRequired` 状态。此时新的 `use` 在访问 World/provider 或申请新身份前返回 `Busy`；调用方必须显式 `retry` 或 `cancel`。重试严格复用原 attempt，不循环、不创建第二个身份。取消和 teardown 都输出 value-only 证据。

本轮为 P 阶段。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`da884f6191f56d54e5f830e096a9f62903dc6160`（P19.8）；
- 分支：`agent/0.0.10-p19-9-divine-sense-logical-input`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 新增与修改

新增：

- `demo_mapShanmenDivineSenseLogicalInputAdapter.h`：243 行；
- `demo_mapShanmenDivineSenseLogicalInputAdapter.cpp`：733 行；
- `demo_mapShanmenDivineSenseLogicalInputAdapterTests.cpp`：799 行。

修改：

- `ShanmenRegressionMap.json`：增加 Logical Input Adapter 的 14 组改动路径证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：增加完整正例与缺少下游证据时失败关闭的反例。

## 4. 生命周期与可用性

`Fdemo_mapShanmenDivineSenseLogicalInputAdapter` 提供：

1. `TryBegin`：绑定一个 active canonical Controller 与同一 Combat Run；
2. `TryUse`：接收一次逻辑使用，并至多调用一次 P19.8；
3. `TryRetry`：只重放适配器保留的 exact attempt；
4. `TryCancelPending`：显式放弃 pending attempt，并返回不可变取消证据；
5. `TryProjectAvailability`：投影下一项合法逻辑操作；
6. `TryEnd`：释放 Run binding，并保留 teardown 前的 pending 状态证据；
7. `Reset`：只允许清理 canonical inactive state，不能绕过 active teardown。

可用性是带确定性 `ProjectionId` 的 pointer-free read model，状态严格区分：

- `Inactive`；
- `UseReady`；
- `ProductUnavailable`；
- `RetryRequired`。

投影重新验证 ControllerId、RunId、P19.8 product availability 与 pending attempt 的绑定关系，不能由调用方拼出另一个有效状态。

## 5. 单次委托、Busy 与失败关闭

普通 `TryUse` 先验证适配器、Controller、canonical config、Combat Run 与 exact player binding，再读取现有产品可用性：

- 有 pending attempt 时直接返回 `Busy`，不访问 World/provider，不推进 Run sequence；
- 产品当前不可用时返回 typed `ProductUnavailable`，不进入 P19.8；
- 只有 `UseReady` 才委托 P19.8 一次；
- live-input/preflight rejection 不占 retry slot；
- 只有携带有效 attempt 的 `ControllerRejected/RouteRejected` 才进入 `RetryRequired`。

显式 retry 复用同一 IntentId、ActivationId 与 RouteCommandId。成功后释放 pending slot；再次收到同一类可恢复拒绝时继续保留 exact attempt；其它异常结果失败关闭，不能静默改成新 use。

## 6. 结果与审计证据

逻辑结果区分 applied、replayed、inactive、invalid、binding mismatch、busy、product unavailable、retry unavailable、retry required、product rejected 与 state desynchronization。

Accepted 结果必须同时满足：调用路径类型与 `AvailabilityBefore` 相符、P19.8 结果有效且 accepted、retry-slot 后置状态一致。取消证据绑定 ControllerId、RunId 和 exact attempt；teardown summary 在释放内部状态之前冻结同样的 value-only 证据。

适配器不复制 P19.8 的资源、World、扫描、receipt、journal、replay 或 lifecycle 状态，因此没有形成第二权威。

## 7. 自动化结果

新增 exact tests 7 项：

- `LifecycleAvailability`；
- `SingleUse`；
- `PreflightRejection`；
- `BusyRetry`；
- `AvailabilityFence`；
- `CancelAndTeardown`；
- `BindingFences`。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | `Product.DivineSenseLogicalInputAdapter` | 7/0 | `CE92C18B73CAB624AAFBEBE689CF307129B8C593B7F9B817CADC049EAC26CD2D` |
| `automation_full.log` | `Shanmen.0_0_10` | 835/0 | `21E27D46D0CAFB1D75B3A042953833C52E69904D29374AEF615AC7F20943F4B8` |
| `automation_legacy_attributes.log` | `demo_map.V3.Attributes` | 4/0 | `CF97757A726F59D3A1646CCA73568B603ADCDB1990CF6BB15DEDDDFD9244EAF8` |
| `automation_legacy_enemy_skill.log` | `demo_map.EnemySkillFramework` | 44/0 | `EB3B9C1D4B83AA0EF0CD646C810B0BE2646D8840C01C7BDF6B5A03CDE0A7B3D1` |
| `automation_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | `2E0FBAED1A8DF943E21C6E730B6D001C2FA47E1060336A06B36115F77BC56F25` |
| `automation_legacy_item_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `7BD9A0434924A705A287AFA62B9D041BD02B88C9D0FABD265D687FAC6E26173C` |

全量由 P19.8 的 828 增加到 835。六份正式日志均只有一个 canonical `RunTests` command、精确预期 Success、Fail 0、一个 native terminal、Fatal/Unhandled/Ensure 0。证据审计：`PASS Logs=6 RecordedSuccess=958`，SHA-256 `801D9DB775C158AD59F0D6138EE4C75FA4756E4A73E499584EAA49D8305922C9`。

## 8. 门禁、边界与构建

真实 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=14 Logs=6
```

- gate SHA-256：`B534A43C0A6C14134D613719A07E2BE3512F729C7D88F825DE948D5E26CD6D08`；
- self-test：`307/307 PASS`，SHA-256 `88E49FAA32E4B656E67D34D15F6F989027D778A9D0EEC1478DB0A939601B0EA8`；
- production boundary scan：`PASS Files=2 Matches=0`，SHA-256 `E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`；
- `git diff --check`：native 0。

构建：

- Game final：4 actions / 40.68s / native 0，SHA-256 `55AAB355C4D9A003D59454F636CF000603875197BF6DC4FA16943FCDFA6FAEF3`；
- Editor final：up to date / 1.15s / native 0，SHA-256 `D4B60A8E93A2985807DDDE67177ABD016974F26AC0FEFB1993AF4F81ABF3516F`。

产物：

- `demo_map.exe`：357,005,312 bytes，SHA-256 `51A893A271EB2331E80B6A2AA84FB1AC8AC103D8FE918401810140A46D3703D0`；
- `UnrealEditor-demo_map.dll`：15,718,912 bytes，SHA-256 `839F9B405984448E7E1397F98810E4D687141B6E40521A3E2AAB957EA7C45DDB`。

## 9. 异常与修正

首次 exact 自动化为 5 Success / 1 Fail / native 255。失败仅在 `BusyRetry` 最终断言：结果自校验错误地要求成功 retry 的前态必须 `CanUse()`，而其正确前态是 `CanRetry()`。修正为按 `bRetryAttempt` 选择前态不变量后，exact 7/7 与全量 835/835 均通过。首次失败日志被保留，SHA-256 `916FCF7F6F41C3F30D02AD0506805805C9940DE0C7A23EED8AA1615EC3FA60B4`。

首次 regression self-test 被 Windows PowerShell 5.1 解析器拒绝，因为既有校验脚本使用 PowerShell 7 的行首管道语法；改用项目已要求的 PowerShell 7 后 307/307 通过。该问题没有通过修改校验语义规避；首次 runner-mismatch 日志被保留，SHA-256 `CD715414D92DF2CDE105820340DEC4592560E8AE686CFEF62ACBD8BFA521C1EA`。

全量 835 项从 canonical command 到 native terminal 用时约 32 分 58 秒。既有 Sword Rhythm checkpoint/envelope 段出现长时间单项运行，但进程始终响应、CPU 与 Success 持续推进，最终未重启、未裁剪，也没有用部分结果替代完成证据。

## 10. P/F 边界与下一步

本轮关闭了 Divine Sense 的 P-stage 前端逻辑操作边界，但没有声称真实玩家输入或可玩验收。物理键位、Enhanced Input asset、UI、Actor discovery 与实际手感都仍属于 F 阶段或其明确产品任务。

不建议继续在 Divine Sense 前面叠加新的 P-stage wrapper。后续应转向尚未覆盖的 0.0.10 战斗能力，或在正式 F-stage 任务中把本适配器绑定到真实输入与场景，并执行对应的 UI/PIE/真实输入验收。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-9-divine-sense-logical-input>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-9-divine-sense-logical-input/Docs/Report/Dev.D.UE.0.0.10.P19.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-9-divine-sense-logical-input/Docs/Log/Dev.D.UE.0.0.10.P19.9.r0_log.md>
