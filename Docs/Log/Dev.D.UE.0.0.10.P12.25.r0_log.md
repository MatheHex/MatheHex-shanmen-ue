# Dev.D.UE.0.0.10.P12.25.r0 Development Log

## 1. 目标

把 P12.24 的 single-read immediate composition 拆成 caller-owned immutable prepared-dispatch value：`PrepareCurrent` 冻结 projection+plan，`TryDispatchPrepared` 可在之后或 retry 时消费同一冻结值，不重新读取 live Session，也不引入 Host/executor/background retry 所有权。

## 2. 实现

- 新增 private-field `ProductPreparedDispatch` value，只含匹配的 frozen projection 与 deterministic plan；
- 新增 `PrepareCurrent`：恰好一次 current capture、一次 plan capture，无 Host/executor 副作用；
- 新增 `TryDispatchPrepared`：签名无 Session，恰好一次进入既有 frozen dispatch；
- 新增 prepared/dispatch 双结果证据与 Dispatched/Resumed/Replayed 状态映射；
- source advance 不改变旧 prepared；延迟执行、terminal replay、durable Create resume 均消费原冻结身份；
- retry-pending 重复调用只重放两条 durable Host record，不隐藏 loop、End 或 executor re-entry；
- DispatchPlan 新增 `MatchesRequest`，统一校验 projection 与八类 transport/execution identity；
- P12.24 planned-current 入口改为复用 `PrepareCurrent → TryDispatchPrepared`，移除重复局部 matcher；
- 新层无 reflection、World、资产、timer、thread、RNG、loop 或 mutable authority。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `ProductPreparedDispatch` | 5 | 0 | 0 | `61A2A05B8ABF6DEDF98D76FC31B182C0798642E11F12DD7142499ACA5B2E1D23` |
| `ProductPlannedDispatch` | 5 | 0 | 0 | `C72629CCDDB11E252AEA9E3F05D3E437E78F57CEFDA18B9EC88CCCE3B70367FD` |
| `ProductDispatchPlan` | 5 | 0 | 0 | `412380153349392D947EC87D059BA2DBBCE30A3B0C569E1634EC3A48560039FE` |
| `ProductDispatch` | 10 | 0 | 0 | `4C7AF786A4BA4965EF90736AB019A6D6FD34F98299EEDC05C220EE4B68F7D51A` |
| `ProductTransaction` | 5 | 0 | 0 | `CA4AA3137E4B338AE462308B2E2DC11E868B1231E1ECA9564941BC5349C7DF8D` |
| `ProductRoute` | 5 | 0 | 0 | `DAD2EB2C35921124E9EB5313D9D2CDE4680CD79322DC965B7BBED5416D3DA6B1` |
| `CommandHost` | 5 | 0 | 0 | `34711ED181F42F331CD33811AC9BCD98F8B530F3987E1155C8868867EBC57640` |
| `CommandRouter` | 5 | 0 | 0 | `B466FE6105526E63C9887083964120F97C20D149FD44217F7F995E292D82DCAB` |
| `ExecutionSession` | 5 | 0 | 0 | `DBF2EF122E54FB89125DA73932BC147386D8C58DF3AAC57F29214F6AC3DBC21A` |
| `ExecutionHost` | 5 | 0 | 0 | `DE609C86FEE6F6421DC8DEC42A26CEB90BE21961F7366DDF16EFDA696B72EEB7` |
| `ExecutionDriver` | 5 | 0 | 0 | `BF212BCCEE641BA09F4FAF9B616B4A2423DF06AB291F648609B31B9961584BBD` |
| `ExecutorAdapter` | 5 | 0 | 0 | `E5F8F42A54121E1A6D3762D21D330FB016BFB927256A20DE3E6814E9F380FA89` |
| `ConsumerAttempt` | 5 | 0 | 0 | `AABA733F6985A12D96D6EF51B2F962A1D46E7F40782CCB0DF8DDE52463C8FB91` |
| `Delivery` | 4 | 0 | 0 | `2493ADBF8BDBE0B03AD377FF9769544DD22E6FCCC2441D7008728E529DEE1D27` |
| `EffectCue` | 72 | 0 | 0 | `9A23A279651738731F2BAF2CFC15B9B3D03A0AF81085EEDAB15660A052813BB4` |
| `Presentation` | 4 | 0 | 0 | `55C3B6D801856249716D7DB19391DB7AC97537AE05625D43C59FF4A112DE86D5` |
| `ProductSession` | 1 | 0 | 0 | `FD1A54ED91D6BBFD84CE8CDEC70109FB72D3FF7CE04FDE78CEFE57FEB157DD7C` |
| `EvaluationRoute` | 1 | 0 | 0 | `61FC28138AC25CC87C6A328D1E65776FFE0DC77BCF94A6546DA3A8D1BD8E89CA` |
| `Shanmen.0_0_10` | 638 | 0 | 0 | `342D0AFE5F4759187BFD017F6D44D0B250E7349AE3233AAE7619C49C08465672` |

最终 19 份 selected phase 均为 Fail `0`、error/Fatal/Unhandled/Ensure `0`，并含唯一 terminal marker。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=10 Rules=3 Required=26 Logs=19
SELF_TEST: PASS 212/212
PREPARE_CALLS: PASS 1/1/1
COMPAT_CALLS: PASS 1/1 and 0/0/0 direct calls
RUNTIME_BOUNDARY_SCAN: PASS
AUTHORITY_SCAN: PASS
JSON_PARSE: PASS
git diff --cached --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：9 actions / 36.95s / exit `0`；
- Game final：8 actions / 32.95s / exit `0`；
- Editor final：0 actions / 0.91s / exit `0`；
- Editor DLL：13,743,616 bytes / SHA `9FB5B825BBF3EF617EAB70721A1EFD2237C5B53F97FAB6E7865DE46D07802258`；
- Game EXE：355,284,992 bytes / SHA `0D4E438EC777D8588A2322A48C47A9BCF5AB600AA29BBB57B52CE5D50963808F`；
- 有效构建无源码失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

首次 focused Automation 即为 `5/5`，exit `0`，SHA `3E6C0FFD368391611998A7087AA4C2CE63048F22DDCA6355292A126A039D662E`；本轮无源码、测试或构建失败。

通用静态扫描最初命中 P12.23 plan 既有的两个固定八项 identity uniqueness `for` 循环；它们是有限纯值验证，不是执行/retry loop。最终按新增 prepared/planned ownership layer 扫描通过。UE selected phase 前的 13 条既有 diagnostics 与跨平台 SDK 提示如实保留；Win64 有效。

## 7. 修改、兼容性与边界

Report/Log 前 10 个代码/流程文件净变更 `+903 / -73`。P12.24 public API 兼容并复用新 service；新层只持有 immutable value，不拥有 Session、Host、executor 或 retry。ProductSession/ProductHost/GameMode/CombatRunCoordinator 未修改。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。长期未跟踪资料保持未暂存。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-25-sword-rhythm-cue-execution-prepared-dispatch>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-25-sword-rhythm-cue-execution-prepared-dispatch/Docs/Report/Dev.D.UE.0.0.10.P12.25.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-25-sword-rhythm-cue-execution-prepared-dispatch/Docs/Log/Dev.D.UE.0.0.10.P12.25.r0_log.md>
