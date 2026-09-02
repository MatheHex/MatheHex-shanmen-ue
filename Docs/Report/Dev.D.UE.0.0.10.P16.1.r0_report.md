# Dev.D.UE.0.0.10.P16.1.r0 Report

## 1. 结论

P16.1 完成 Meridian Shock 专用物品治疗的唯一产品路由。新路由在 Game Thread 上独占以下顺序：

```text
capture immutable command
  -> durable prepare item quantity
  -> treat exact active condition revision
  -> durable commit item consumption
```

同一请求可确定性重放；同一 RequestId 的不同 payload 失败关闭。治疗前失败只允许依据同一实时 active condition revision 取消；治疗一旦成功，后续恢复永久收窄为 commit-only，不能退回取消分支。

最终结果：

```text
Route focus:                    3 Success / 0 Fail
Meridian Shock treatment:       7 Success / 0 Fail
Mapped legacy regressions:    348 Success / 0 Fail
Shanmen.0_0_10 full:          720 Success / 0 Fail
Regression coverage:          PASS (Changed=8 / Rules=2 / Required=10 / Logs=7)
Editor + Game Development:    PASS / native status 0
```

## 2. 唯一产品路由

新增 `Fdemo_mapShanmenMeridianShockTreatmentProductRoute`：

- `TryBegin` 绑定一个精确 active Run correlation 与对应 condition component；
- `TryCaptureCommand` 在任何权威写入前冻结 RequestId、Run、目标、条件修订、物品实例、数量与时间线样本；
- `TryExecute` 是唯一允许串联 P16.0 item adapter 与 condition authority 的产品入口；
- route journal 以 RequestId 保存不可变 command 与阶段回执；
- `TryEnd` 拒绝遗忘任何 ambiguous prepare、prepared 或 treated-but-uncommitted 工作；
- begin、capture、execute 与 end 全部限制在 Game Thread。

路由不复制库存、条件、健康、时间或输入真值：ShanmenItems receipt 仍是物品真值，condition component receipt 仍是条件真值，fixed timeline sample 仍是时间证据。

## 3. 有序事务与重放

首次执行严格经过三阶段：

1. `PrepareActiveRun` 持久化同一 Run 内的 Quantity intent；
2. `TryTreatMeridianShock` 只接受 command 冻结的条件修订与时间线；
3. `CommitTreated` 只接受与准备 intent 完全匹配的成功 Treatment Receipt。

成功路径实测：物品数量 `3 -> 2`、条件修订 `1 -> 2`、Meridian Shock inactive、commit finalize 精确一次。随后重放同一 command 返回原回执，两个权威 snapshot 不变，且不会产生第二次 finalize。

同一 RequestId 搭配另一 timeline sample 时，路由在任何新权威写入前返回 `RequestIdConflict`。

## 4. 失败与恢复边界

治疗失败时，路由重新读取实时 Meridian Shock status。只有该状态仍是同一 active revision，P16.0 adapter 才能执行 durable cancel；否则保留 prepared work 并返回 `RecoveryRequired`。

治疗成功后，journal 先固定成功 Treatment Receipt，再尝试 item commit：

- 注入 treatment 后中断时，`TryEnd` 被阻止；
- item store `WriteTemp` 失败时，路由仍保留治疗成功证据；
- 重启并重新绑定 ShanmenItems authority 后，精确 command 只重试 commit；
- 恢复后物品仍只从 `3 -> 2`，cancel finalize 为 0。

本阶段证明的是“同一运行时 route journal + 持久化 item authority 重新绑定”的恢复，不声称 route journal 或 condition receipt 已具备完整进程崩溃持久化。该边界留给后续正式 session/lifecycle 接线阶段。

## 5. P16.0 集成缺口与修正

首次用真实 GameInstance item authority 执行路由时，3 个 focused tests 全部失败。追加诊断后复现并定位为 P16.0 adapter 的内容身份混用：

- ShanmenItems authority snapshot 使用稳定持久化身份 `Shanmen.Items.0.0.10 / Shanmen.Items.LegacyAuthority.Schema1.v1`；
- product catalog 使用随内容变更的 `CodeB.Content.0.0.10.P16.0` 身份；
- adapter 错把前者要求成后者，导致真实生产 authority snapshot 永远被判 stale；
- P16.0 的纯合成 fixture 也沿用了错误 catalog identity，因此未暴露问题。

修正后，authority snapshot 精确比对 `Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp()`；物品 Definition 仍独立比对当前 product catalog。对应 P16.0 fixture 同步改为真实 authority content stamp，没有放宽 revision、catalog 或语义检查。

保留的首次失败日志：

- 原始失败 `0/3`：`35DC9421BBED357FA929002ACFDE601CFA08EF4223D8D1A6C049E6520F957867`；
- 增加诊断后的可复现失败 `0/3`：`3693B2320EE28EA8413784AEE2C2730E13B008A5E3D5B2546A9208B38C7A94BB`；
- 修正后 focused `3/0`：`A845D6AEC44B90E4A7990461193141A5850A889B49840449616089B3EF31F724`。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Route focused final | 3 | 0 | `A845D6AEC44B90E4A7990461193141A5850A889B49840449616089B3EF31F724` |
| Meridian Shock treatment final | 7 | 0 | `21F5BBC66E59FB4D154FA6166612205918D90A687C6187FF19865DBAD3076CA3` |
| ItemEconomySchema | 24 | 0 | `E8AA2992CA1ABA2425619EA626F32735460B888DADDF3F307F7D4F3B63A4732A` |
| Profile | 211 | 0 | `F9819EB641610920CABC965C39485A215D5400DDDFC9CDB77FFB9CBED708BF23` |
| CodeB | 60 | 0 | `3AF17969F53C63F275BAE41800DF95CCBA062BB65BD50784DB5CA6E500FD347F` |
| ItemUseAndArmor | 46 | 0 | `3388905E4B36C8212855B84540700B38CE03B7E69A0CB1BB6290AB23BAEE1463` |
| P4 Hotbar | 7 | 0 | `3C4D74C618954279EF8097B560475A8DA3D7F6EC345AC862730D3D6CE852641E` |
| `Shanmen.0_0_10` full | 720 | 0 | `5CB84D8332FCEDAC774910FB32089D5CAC76A0B746D25742037BC710DA5A358A` |

全量首末 Success 时间为 `04:21:21.022 -> 04:49:09.574 UTC`，用时约 `27m48.55s`；唯一 `TEST COMPLETE / EXIT CODE 0` 终止标记存在，Fatal error、Unhandled Exception 与 Ensure condition failed 均为 0。

## 7. 改动—回归与静态边界

`ShanmenRegressionMap.json` 新增 `MeridianShockTreatmentProductRoute` 映射。route 源文件要求 full、route/treatment/condition、Items，以及 ItemEconomySchema、Profile、CodeB、ItemUseAndArmor、P4 Hotbar 证据。

```text
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=10 Logs=7
SELF_TEST: PASS 271/271
JSON_PARSE: PASS
GIT_DIFF_CHECK: PASS
BOUNDARY_SCAN: PASS
```

流程 SHA：gate `7660AA75C16FCDC0FBB62E2BFFC68B1BB08839AA07F055ECEAA7D8490AC523F9`；self-test `A34FB07154B570E39D7736BF27B33745DA4F6517015445ADCBA8ED7CA4004E6A`；JSON `48C32879BFB681137257F2FA697C74FED7A350541D953E584A70FE8C7B067345`；diff `6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`；boundary `5A69B655C3CC5E2DEF3EA05ACA4BF71E4047158A0D274603761E01D51F9B257C`。

首次最终 gate 读取本轮 UE 原生日志时发现 `Cmd:` 行使用 `<group>;Quit`。旧解析器把 `;Quit` 误并入组名，导致有效证据被判为缺失。现已让组名在分号前终止，并新增该格式的自检；271 项检查全部通过。该修正不改变产品测试结果或门禁覆盖要求。

边界扫描确认新 route 不依赖 `UWorld`、`AActor`、GameMode、Enhanced Input、Tick/Timer、RNG、ApplyDamage 或 SpawnActor。

## 8. 构建证据

本轮使用 `-NoUBA -MaxParallelActions=1` 控制主机提交内存压力。

| Target | Result | Status | Log SHA-256 |
|---|---|---:|---|
| Game final | Succeeded (6 actions / 30.62s) | 0 | `763EDFA6EF3CEDE2EBA23BC60F9D39C648EB757F72ECACFAE4D5C4BB12EBC3D4` |
| Editor final | Succeeded, up to date (0 actions / 0.91s) | 0 | `6C86AD2192A15C9472E6C03C97EFE6D038DC3E34FF8F001141EE409E63E35965` |

最终产物：

- `demo_map.exe`：355,839,488 bytes，SHA-256 `A9FE339BD4D6CC9457CBD7BB302C4B9ABD1071A5CF79526EE54B4002F9D191C9`；
- `UnrealEditor-demo_map.dll`：14,447,616 bytes，SHA-256 `8C54D714AB13B68811349D727C93FFFFD3A1FDF9CFD87AA8DDED6852FE43D74A`。

## 9. 修改范围与 P/F 边界

修改范围仅包括新 treatment product route、3 个 route Automation tests、P16.0 authority content-stamp 集成修正、对应 fixture、回归映射、UE `;Quit` 证据解析兼容与自检，以及 Report/Log。

raw logs 仅保存在本机 `Saved/Codex/P16.1`。长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

本 Report 仅包含 P 阶段 C++ 契约、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. 后续

下一阶段应把该 route 安装到唯一正式 hotbar/input session 与 Run lifecycle：session 生成 RequestId、保存未完成 route checkpoint，并在 Run end 前恢复 commit-only 工作。UI 只提交 command，不直接调用 condition 或 item authority。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-1-meridian-shock-treatment-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-1-meridian-shock-treatment-route/Docs/Report/Dev.D.UE.0.0.10.P16.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-1-meridian-shock-treatment-route/Docs/Log/Dev.D.UE.0.0.10.P16.1.r0_log.md>
