# Dev.D.UE.0.0.10.P19.9.r0 Development Log

## 1. 目标与基线

- 基线：`da884f6191f56d54e5f830e096a9f62903dc6160`（P19.8）；
- 分支：`agent/0.0.10-p19-9-divine-sense-logical-input`；
- 目标：在 P19.8 Product Route 前建立 device-independent、Run-scoped、capacity-one 的 Divine Sense logical input adapter；
- 约束：一次逻辑 use 至多委托一次；不接物理键位、UI、隐式 Actor discovery、产品策略、第二资源或 replay 权威。

## 2. 设计审计

审计了 P19.1 runtime、P19.2 pulse coordinator、P19.3 host、P19.4 command router、P19.5 session、P19.6 Controller、P19.7 authority 与 P19.8 route，并对照 Spirit Evasion、Weapon Guard 的同类逻辑输入边界。

P19.8 已拥有产品 preflight、canonical config、身份申请、attempt provenance 与重试入口；P19.6 已拥有 resource transaction、intent/command journal 和 accepted replay。P19.9 因而只负责“当前逻辑操作是否合法”和“是否已有一个待重试操作”，不复制下游状态。

## 3. Run binding 与状态不变量

`TryBegin` 只接受 active、valid、canonical Controller，以及同 RunId、同 player entity 的 ready Combat Run。重复绑定同一组合是幂等成功；active 状态不能切换 Controller 或 Run。

inactive canonical state 要求 ControllerId、RunId、pending attempt 全为空。active state 要求 ControllerId/RunId 有效；若有 pending attempt，则它必须精确绑定该 Controller、Run、config 与 source entity。

`Reset` 不允许跳过 active teardown。`TryEnd` 先生成包含 pending attempt 的 value-only summary，再释放内部绑定。

## 4. Pointer-free availability projection

availability projection 保存确定性 ProjectionId、typed state、ControllerId、RunId、P19.6/P19.8 product availability 与可选 pending attempt。

四个合法 shape：

1. `Inactive`：无产品投影、无 attempt；
2. `UseReady`：产品可捕获新 intent、无 attempt；
3. `ProductUnavailable`：产品拒绝新 intent、无 attempt；
4. `RetryRequired`：保留 exact route-issued attempt。

`ProjectionId` 由状态、Controller/Run、产品 AvailabilityId 与 attempt 的 IntentId/ActivationId 规范派生。`Matches` 除比较 ID 外再次比较完整下游投影与 attempt，防止只复制外层 GUID 的伪匹配。

## 5. Use、retry 与 cancel

`TryUse` 的顺序：

1. 验证 adapter active/valid；
2. 验证 exact Controller、canonical config、Combat Run 与 player binding；
3. 捕获 availability；
4. pending 时返回 Busy；产品不可用时返回 ProductUnavailable；
5. 仅 UseReady 时调用 P19.8 一次；
6. 仅 typed `ControllerRejected/RouteRejected` 且带有效 attempt 时占用 retry slot；
7. 捕获后态并验证完整 result。

`TryRetry` 不申请新 attempt，只复制 pending 值后调用 P19.8 retry。accepted 时释放 slot；相同 attempt 的可恢复拒绝继续保留；其它拒绝不转换成 fresh use。

`TryCancelPending` 生成绑定 ControllerId、RunId、exact attempt 的 cancellation proof，只有 proof 与 postcondition 都有效才清空 slot。

## 6. Typed result

`Fdemo_mapShanmenDivineSenseLogicalInputResult` 保留：

- status 与 diagnostic；
- 是否 retry 调用、是否真实委托 route、是否仍保留 pending；
- availability before/after；
- 完整 P19.8 route result。

结果自校验要求普通成功从 `CanUse()` 开始，retry 成功从 `CanRetry()` 开始；busy 必须保持前后投影相同；retry-required 必须让后态 attempt 与 route attempt 完整匹配。

## 7. 测试开发

新增 7 项 exact contract：

- `LifecycleAvailability`：inactive/active projection、幂等 begin、reset fence；
- `SingleUse`：一次逻辑 use 只委托一次并得到 accepted proof；
- `PreflightRejection`：live-input/preflight failure 不占 retry slot；
- `BusyRetry`：route rejection 占唯一 slot，new use fail-fast，explicit retry 不换身份；
- `AvailabilityFence`：产品资源/容量不可用时不进入 route；
- `CancelAndTeardown`：取消与 teardown 保留 value-only evidence；
- `BindingFences`：跨 Controller/Run 与非 canonical binding 失败关闭。

## 8. 首次失败与根因修正

首次 exact：

```text
Success=5 Fail=1 NativeExit=255
Fail=Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter.BusyRetry
Assertion=explicit retry recovers without new identity
```

根因不是 P19.8 retry 失败，而是新结果类型的 `IsValid()` 对 accepted 路径统一要求 `AvailabilityBefore.CanUse()`。显式 retry 的合法前态应为 `CanRetry()`，导致实际恢复成功的结果被自校验降级为 state desynchronization。

修正后按 `bRetryAttempt` 选择前态不变量，并补齐第 7 项 binding test。最终 exact 7/7、全量 835/835。首次失败日志保留于 `automation_exact_initial.log`，SHA-256 `916FCF7F6F41C3F30D02AD0506805805C9940DE0C7A23EED8AA1615EC3FA60B4`。

regression self-test 首次由 Windows PowerShell 5.1 启动，既有 PowerShell 7 行首管道语法被解析器拒绝。改用 `pwsh` 后 307/307 通过；没有修改 validator 来迁就错误 runner。首次 runner-mismatch 日志 SHA-256 `CD715414D92DF2CDE105820340DEC4592560E8AE686CFEF62ACBD8BFA521C1EA`。

## 9. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `automation_exact.log` | 7/0 | `CE92C18B73CAB624AAFBEBE689CF307129B8C593B7F9B817CADC049EAC26CD2D` |
| `automation_full.log` | 835/0 | `21E27D46D0CAFB1D75B3A042953833C52E69904D29374AEF615AC7F20943F4B8` |
| `automation_legacy_attributes.log` | 4/0 | `CF97757A726F59D3A1646CCA73568B603ADCDB1990CF6BB15DEDDDFD9244EAF8` |
| `automation_legacy_enemy_skill.log` | 44/0 | `EB3B9C1D4B83AA0EF0CD646C810B0BE2646D8840C01C7BDF6B5A03CDE0A7B3D1` |
| `automation_legacy_v2_ranged.log` | 22/0 | `2E0FBAED1A8DF943E21C6E730B6D001C2FA47E1060336A06B36115F77BC56F25` |
| `automation_legacy_item_armor.log` | 46/0 | `7BD9A0434924A705A287AFA62B9D041BD02B88C9D0FABD265D687FAC6E26173C` |

每份正式日志通过唯一 command、精确 Success、Fail 0、唯一 native terminal、Fatal/Unhandled/Ensure 0 检查：

```text
EVIDENCE_AUDIT: PASS Logs=6 RecordedSuccess=958
```

审计日志 SHA-256：`801D9DB775C158AD59F0D6138EE4C75FA4756E4A73E499584EAA49D8305922C9`。

## 10. Changed-file regression gate

新增 `DivineSenseLogicalInputAdapter` mapping，要求 Logical Input Adapter、Product Route、Authority、Controller、Session、Command Router、Host、Pulse Coordinator、World Observation、Combat Run、Divine Sense runtime、Action Resource、Action Lifecycle 与 WorldGameplay。

真实 gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=14 Logs=6
```

- gate SHA-256：`B534A43C0A6C14134D613719A07E2BE3512F729C7D88F825DE948D5E26CD6D08`；
- self-test：`307/307 PASS`；
- self-test SHA-256：`88E49FAA32E4B656E67D34D15F6F989027D778A9D0EEC1478DB0A939601B0EA8`。

## 11. 静态边界与构建

生产 adapter header/cpp 扫描禁止 InputAction/EnhancedInput/key binding、PlayerController/Widget、Actor enumeration、trace/sweep/overlap、spawn/destroy、RNG 与 Timer/Tick：

```text
BOUNDARY_SCAN: PASS Files=2 Matches=0
```

boundary log SHA-256：`E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`。`git diff --check` native 0。

最终构建：

- Game：4 actions / 40.68s / native 0，log SHA-256 `55AAB355C4D9A003D59454F636CF000603875197BF6DC4FA16943FCDFA6FAEF3`；
- Editor：up to date / 1.15s / native 0，log SHA-256 `D4B60A8E93A2985807DDDE67177ABD016974F26AC0FEFB1993AF4F81ABF3516F`。

产物：

- `demo_map.exe`：357,005,312 bytes / `51A893A271EB2331E80B6A2AA84FB1AC8AC103D8FE918401810140A46D3703D0`；
- `UnrealEditor-demo_map.dll`：15,718,912 bytes / `839F9B405984448E7E1397F98810E4D687141B6E40521A3E2AAB957EA7C45DDB`。

## 12. P/F 边界与后续判断

全量从 canonical command 到 native terminal 用时约 32 分 58 秒。Sword Rhythm checkpoint/envelope 既有慢段保持 responsive、CPU 与 Success 持续推进，最终 835/835；未中断或缩小范围。

未运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P19.9 已关闭 Divine Sense 的 P-stage logical-input seam。下一步不应继续堆叠 wrapper；应转向另一项未覆盖战斗能力，或等待正式 F-stage 要求后绑定真实输入、UI 与场景 Actor。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-9-divine-sense-logical-input>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-9-divine-sense-logical-input/Docs/Report/Dev.D.UE.0.0.10.P19.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-9-divine-sense-logical-input/Docs/Log/Dev.D.UE.0.0.10.P19.9.r0_log.md>
