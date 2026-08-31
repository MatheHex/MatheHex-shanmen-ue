# Dev.D.UE.0.0.10.P12.24.r0 Development Log

## 1. 目标

把 P12.23 deterministic plan 与 P12.22 dispatch 组合为 single-read planned-current seam：一次冻结 current projection、一次捕获 plan、一次 frozen dispatch，同时保留旧 current API，且不引入 Session/Host/executor/retry 所有权。

## 2. 实现

- P12.22 新增 `TryDispatchProjection`，调用方可派发已冻结 projection；
- 原 `TryDispatchCurrent` 只捕获一次 current projection，再复用同一内部 helper；
- 新 `ProductPlannedDispatch` 固定执行 current capture → plan capture → frozen dispatch；
- 成功结果交叉核验 projection、plan 与 transaction request 的 Host、sequence、command、consumer 和 attempt identity；
- Completed/Resumed/Replayed 保持一一映射，其他状态作为 DispatchIncomplete 透明返回；
- invalid projection/seed 在 Host/executor 前拒绝；terminal replay、durable Create resume 与 retry-pending 继续由既有权威链处理；
- 新层无 reflection、World、资产、timer、thread、RNG、loop 或 mutable authority。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `ProductPlannedDispatch` | 5 | 0 | 0 | `F6074B7A2FD589E653451EEF9F585A10F44E359FFAB410DF9C582DF4A98B6934` |
| `ProductDispatchPlan` | 5 | 0 | 0 | `B7A0AFF3835313896C5F8F9797452B7815FA21252731AC680C10FBF91A52F193` |
| `ProductDispatch` | 10 | 0 | 0 | `71C063F37C207EE8E53788A598A6266FC05BD812870539FFA292997CAA75CB16` |
| `ProductTransaction` | 5 | 0 | 0 | `794908C27B63B0EC6D97C28C071C734DABE521BC7AB72D6608916A31FF32EFFB` |
| `ProductRoute` | 5 | 0 | 0 | `B05F6A2B2B7830EF35D0DB020F5C548FBD415F0BA8F4927AC262E8C1E506CA33` |
| `CommandHost` | 5 | 0 | 0 | `6FB7AA07E807A5EC32C8AE63E44CD97D2E5F18FE8B98CE7B37BFFA5B34FED64D` |
| `CommandRouter` | 5 | 0 | 0 | `09493600DBCB8C1ACD5E690350996ABB410572D4C96CA7820263658F6BF5A646` |
| `ExecutionSession` | 5 | 0 | 0 | `F023D4E8A4376684DFFAAB947BFEB64CD7CB80202CBF9D06C686EBADE068565C` |
| `ExecutionHost` | 5 | 0 | 0 | `907DADB32873C75F890BB2939DF1AE1D83B2F1C390FD81C96B4E46546B2CD469` |
| `ExecutionDriver` | 5 | 0 | 0 | `84711A8DCBBE62C4BF668ED3D58ACF7820ADC01E46E754D3F067B8124D9A60AC` |
| `ExecutorAdapter` | 5 | 0 | 0 | `9AB1EB6E007116AABA0449DDB643C42D8460909D43DF9FAE2B03791A22525337` |
| `ConsumerAttempt` | 5 | 0 | 0 | `063D305F3311821ABE8667641094B10DC4D68F63CC06507077B418A548869D7A` |
| `Delivery` | 4 | 0 | 0 | `F88943A8A0372A4130BA5C9265AD4F970A5DAB03FB82390948AB6D059726BED0` |
| `EffectCue` | 67 | 0 | 0 | `1AAAAE127809B3D5512841C3BA30F59AF74C9979EB004C05A9B1EFCA7D300BAB` |
| `Presentation` | 4 | 0 | 0 | `C1C746A1272AE6BC9EBDE818D0DC94E18C240CC305DC350D2B3A83CF9FC20F26` |
| `ProductSession` | 1 | 0 | 0 | `A90DFA6E01DFC4942142E298B47665570E2ED8BEAD35CCF45BB9326AC48319A7` |
| `EvaluationRoute` | 1 | 0 | 0 | `F24D398AED66992BF24AF82F1E8B9EA22AB6ACD403F5C95D640D4764A0526D37` |
| `Shanmen.0_0_10` | 633 | 0 | 0 | `982DC00445237409ED56D6F79816D0E55E775EB3BD6A1C4E9F84119A479A12A8` |

最终 18 份 selected phase 均为 Fail `0`、error/Fatal/Unhandled/Ensure `0`，并含唯一 terminal marker。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=25 Logs=18
SELF_TEST: PASS 210/210
COMPOSITION_CALLS: PASS 1/1/1
LEGACY_CURRENT_CAPTURE: PASS 1
RUNTIME_BOUNDARY_SCAN: PASS
AUTHORITY_SCAN: PASS
JSON_PARSE: PASS
git diff --cached --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：9 actions / 38.92s / exit `0`；
- Game final：8 actions / 32.36s / exit `0`；
- Editor final：0 actions / 0.93s / exit `0`；
- Editor DLL：13,715,968 bytes / SHA `E3798C6FD62488E1E727B0E39573D22AD393D427F0782DAE99B63555A8E3278A`；
- Game EXE：355,263,488 bytes / SHA `7F473EFFECFF27EECB03F85FBF64DC054DFD77373044C413E0901CD901C7663A`；
- 有效构建无源码失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

前三次 focused 运行均为 `4/5`：合并断言、拆分断言、数值断言依次把问题定位为错误的“新 revision 必然调用 executor”测试假设。SHA 分别为 `90151EF1F08B9A8A2F2F87F85191E48AC5714B754D81CC67C118657FC3C8270F`、`14B372BA8E2E51010FDB184167F8DEE76571AB21291090404800E93B14042059`、`3D27814234E3AFA9A72E5888E99290EF50ABC300A7658855817997009D779368`。

实际新 projection 为 Visual/Audio 双通道零命令路由，按既有契约应提交 `NoOpSucceeded` 且不进入 executor。仅修正测试期望并增加 no-op 证据断言，生产代码未改；最终 focused `5/5`。三次失败进程本身 exit `0`，本轮按 selected test Result 正确判失败。raw 日志本地保留。

## 7. 修改、兼容性与边界

Report/Log 前 7 个代码/流程文件净变更 `+903 / -69`。原 P12.22 current API 兼容；新层只做一次同步 composition，不拥有 Session、Host、executor 或 retry。ProductSession/ProductHost/GameMode/CombatRunCoordinator 未修改。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。长期未跟踪资料保持未暂存。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-24-sword-rhythm-cue-execution-planned-dispatch>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-24-sword-rhythm-cue-execution-planned-dispatch/Docs/Report/Dev.D.UE.0.0.10.P12.24.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-24-sword-rhythm-cue-execution-planned-dispatch/Docs/Log/Dev.D.UE.0.0.10.P12.24.r0_log.md>
