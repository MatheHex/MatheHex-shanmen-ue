# Dev.D.UE.0.0.10.P12.29.r0 Development Log

## 1. 目标

把 P12.28 的 caller-requested one-shot retry step 封装成一条不可变 command，并返回结构自校验 receipt。command 必须绑定 exact prepared root、policy/count 与 capture-time Host cursor；每次 command 仍只推进一个 step，不引入 episode、loop、scheduler、timer、queue 或后台 ownership。

## 2. 实现

- 新增 private-field immutable retry-step command；
- capture 冻结 command id、prepared dispatch、request 与 Host id/run/batch/sequence/count/terminal；
- stale 或 foreign Host cursor 在 P12.28 前返回 `HostCursorMismatch`；
- adapter 恰好调用一次 P12.28 `TryRunStep`，不复制 decision/retryability/root 逻辑；
- 新增 handled、decision-rejected、execution-rejected receipt statuses；
- deterministic receipt id 绑定 command、prepared identity、policy/count、capture cursor、step statuses、retry seed 与 final Host state；
- mutating retry 后旧 command 失效，caller 必须用 next count 与新 cursor 捕获下一 command；
- non-mutating Stop 的 exact replay 返回相同 receipt id 且不调用 executor；
- production adapter 无直接 Host route、loop、scheduler/timer/thread、World/UObject 或长期状态。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `RetryStepCommand` | 5 | 0 | 0 | `6B5076916AE7D465DF60098F6EC1DECF64EA58F40A5CA0263D8895D66B45F549` |
| `RetryStep` | 10 | 0 | 0 | `6B26C6C6F28DC7701FD7E546BB5BD0A86F7823863C40182A6DF3AD2F2F65F8D4` |
| `RetryDecision` | 5 | 0 | 0 | `90268F9F07E0EA7672ACAB54A547D9D9B08C346B8966FF5B905B73C3DCD17196` |
| `PreparedRetry` | 6 | 0 | 0 | `351C50E8B595C11ED1759D166E3BE60F723F0245BD5AC9ED4806984C25C08548` |
| `PreparedDispatch` | 5 | 0 | 0 | `01D4A291EBA0D6B50AEA0036339465302A67D89D4115BE5CE7CA9F3DD24739F8` |
| `PlannedDispatch` | 5 | 0 | 0 | `C918E3DDB17CB7A7D52FAB2F2FF4E02C01144A786D1177E05E7255AB9A190347` |
| `DispatchPlan` | 5 | 0 | 0 | `CCC729657D5F9DB1BAAB8BDA66D7F10E66B16DA515CA1DAD5834047B63E22B05` |
| `Dispatch` | 10 | 0 | 0 | `5CBB3BE8170A9225651FA306B37CFA12E733B877C9767735D86A83E5A882022A` |
| `Transaction` | 5 | 0 | 0 | `571E2DFE47F5F7EC2D5A09010700BDDCBFE11DE023580C23E7E38A7C3C51EB5E` |
| `Route` | 5 | 0 | 0 | `DED3E456E2DA26CE358BED9D921906DC3BBDBE46771496E385E4ED6254012A56` |
| `CommandHost` | 5 | 0 | 0 | `A651A45453EAF1FAFF7F3F289CB3CF887C50BEE19A99C6B9CA57E2FD45F0D4BC` |
| `CommandRouter` | 5 | 0 | 0 | `0AAFC7B694458343E521603F32B08CA22F1EC4E14D6FAB1547F769EDF81169EA` |
| `ExecutionSession` | 5 | 0 | 0 | `B0DF62AFDA33B130922399B007A5828947F470375D884CD6A283E9E389555582` |
| `ExecutionHost` | 5 | 0 | 0 | `D8A620AD0AE38C5A585C5165C57476E50ED237842AD7CE72187AA3718886AA34` |
| `ExecutionDriver` | 5 | 0 | 0 | `5CB45AE04665258070041692AA58FAC45D2268FC2C8C552B34C0053553A1A600` |
| `ExecutorAdapter` | 5 | 0 | 0 | `C5A18FFF0FD50F8BF96E15637CB0F832F54BCCA8697C97A10A2D81A158791CF6` |
| `ConsumerAttempt` | 5 | 0 | 0 | `08A22CE4360A6CFF9FE7AB63435186A9C1635CCC90AD7BBBEC6E9FB798F277D2` |
| `Delivery` | 4 | 0 | 0 | `26BDE2E65C711ABCC8CD59F9AFB9FEB80AF9C6B40705EC52C209C0B67FCE2432` |
| `EffectCue` | 93 | 0 | 0 | `A435338AC2C214219AA7774E38873257DA87C12FA7F91B021A70F48962E57045` |
| `Presentation` | 4 | 0 | 0 | `C264F39ACE5C95A7B7A114C54D832DCFF334174F3B3B7F012E487F6E584FDE0B` |
| `ProductSession` | 1 | 0 | 0 | `3FB9A2E7A9969F572888227C67A5FD658E83FBE82CA126DFEFAD78AC4382B357` |
| `EvaluationRoute` | 1 | 0 | 0 | `452F8732E5ACBAF731C702C4BA6B5A07B75D358ECBC269827408DA70B605BA1D` |
| `Shanmen.0_0_10` | 659 | 0 | 0 | `BBFFDE74E1944D6462679609EB53CEB5C7CE0435E621AA10BB6883DE510EAB61` |

最终 23 份 selected phase 日志全部 Fail `0`、terminal `1`、Fatal/Unhandled/Ensure `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=30 Logs=23
SELF_TEST: PASS 220/220
COMMAND_BOUNDARY_SCAN: PASS TryRunStep=1 Loop=0 SchedulerTimerThread=0 DirectHostRoute=0 WorldObject=0
AUTHORITY_SCAN: PASS
JSON_PARSE: PASS
git diff --cached --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：5 actions / 14.83s / exit `0`；
- Game final：4 actions / 20.12s / exit `0`；
- Editor final：0 actions / 1.02s / exit `0`；
- Editor DLL：13,878,272 bytes / SHA `84A57A7EC6DB2AE8BBB9B5149AAA39A390124C0F456841FF16700210EF1ABBE2`；
- Game EXE：355,387,904 bytes / SHA `CBB1D517C67ED6E8D5426295CDC58F7E152195BBD1B5A51321502D5D34460B73`；
- 所有有效构建无源码失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

首次 compile 与 focused Automation 均成功；focused initial `5/5`、exit `0`、SHA `6B5076916AE7D465DF60098F6EC1DECF64EA58F40A5CA0263D8895D66B45F549`。本轮无源码、断言或构建失败。

首次 gate 因一份 `ExecutorAdapter` 日志缺 terminal marker 而失败关闭；该次五条测试 Success、UE 进程 exit `0`，本地备份 SHA 为 `2B8DDCC109F11D0762AD32E4C34E6CA0903803CEA0A525E95FCC5E9873EDA93C`。单独重跑后最终日志 `5/5`、terminal `1`。随后一次宽泛 glob 又把旧备份纳入 gate，改用显式 23 路径后通过。两者均为证据生成/选择问题，不是源码或测试失败。

UE selected phase 前存在既有 diagnostics；宽组的 `generate_204` 网络超时/large-delta warning 未阻止推进。LinuxArm64/VisionOS SDK 提示不影响 Win64。

## 7. 修改、兼容性与边界

Report/Log 前五个代码/流程文件净变更 `+1017 / -0`。既有 RetryStep/Decision/PreparedRetry/PreparedDispatch/Host/Router/Session API 未改；新层只追加 caller-owned immutable command/receipt。长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-29-sword-rhythm-cue-retry-command>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-29-sword-rhythm-cue-retry-command/Docs/Report/Dev.D.UE.0.0.10.P12.29.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-29-sword-rhythm-cue-retry-command/Docs/Log/Dev.D.UE.0.0.10.P12.29.r0_log.md>
