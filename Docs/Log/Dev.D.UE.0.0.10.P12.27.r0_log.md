# Dev.D.UE.0.0.10.P12.27.r0 Development Log

## 1. 目标

在 P12.26 prepared retry 上提供调用方显式 budget + latest durable receipt 的纯 `Retry / Stop` 决策。决策层只生成一份 continuation 或终止证据，不执行 retry、不持有计数器、不增加 scheduler、loop 或 executor ownership。

## 2. 实现

- 新增 caller-owned policy：deterministic `PolicySeed` + `MaxRenewals`；
- 新增 request：policy + explicit `RenewalsUsed`，无内部可变计数；
- 从 prepared plan/projection、Host root、最新 durable dispatch/command 与 consumed count 派生 retry seed；
- 复用 P12.26 `PrepareRetry` 作为唯一 root/retryability validator；
- budget available + retryable receipt 返回 `Retry` 与一份 prepared continuation；
- budget exhausted 返回 `StopBudgetExhausted`，continuation 必须 invalid；
- terminal 与 non-retryable durable receipt 分别返回 `StopCompleted` / `StopNotRetryable`；
- invalid/foreign/state failures 使用 rejection status，不伪装为 Stop；
- immutable decision 固定 observed record/sequence/terminal、preparation status 与 next count；
- production decision layer 无 Host mutation、executor 参数、execute/route 调用、World、timer、thread、RNG 或 loop。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `ProductRetryDecision` | 5 | 0 | 0 | `B30862590E2EA23C35DF94895C3376328A6F354D29654925326560CC8060D7BC` |
| `ProductPreparedRetry` | 6 | 0 | 0 | `BEB0CBF5FE631BFBF72B9C4C65EA395ED782FD2CAADD955421BBABB765346B77` |
| `ProductPreparedDispatch` | 5 | 0 | 0 | `741B17348C00FCDCB00D58327EAFDBAD3032D7C103487710A13BC6FC422585F5` |
| `ProductPlannedDispatch` | 5 | 0 | 0 | `964D6FC6546E9B844B4431FAF616EDB108BAD6155F3691412B64D0F37E73D35E` |
| `ProductDispatchPlan` | 5 | 0 | 0 | `8260563800CC2F5D297DC8A7868B38F8BA7E0F38E70E0E4852A668BF721C3486` |
| `ProductDispatch` | 10 | 0 | 0 | `0C33EB63B43DE21740E0A7C53D22EAEFEAA612D2C3F732DA36F3AC5CBA2EF88E` |
| `ProductTransaction` | 5 | 0 | 0 | `B6FD1B39BCE5CAC7C67F747BB6CA7BE3C61CD034FA04D4670912961547A3263A` |
| `ProductRoute` | 5 | 0 | 0 | `6F2BEDDAD25E75A004747A89F33339EA28325E40EA385A36BD4B71E3216B4CED` |
| `CommandHost` | 5 | 0 | 0 | `2D4DD907604B1A906F2A3A95690775073694ABA31B0D6B2B3461F93F83E0E550` |
| `CommandRouter` | 5 | 0 | 0 | `21FF4C630EF84AD1F71BEDEE398FF165C235210596598A56508E6069E6C6D81B` |
| `ExecutionSession` | 5 | 0 | 0 | `752056A3CA55F21E28D10A1DE851D22CE0C4B997EA6EEF5DFB27207A7D499D2B` |
| `ExecutionHost` | 5 | 0 | 0 | `D3F4187EED6BA7D99918AE9C2DBEA6243DD11AD73699B54B6D1B2BF1D80CD908` |
| `ExecutionDriver` | 5 | 0 | 0 | `4E34945CEDEF627C1E61BFF5F86D3062C5849D292148428C57DD0172E82CB41B` |
| `ExecutorAdapter` | 5 | 0 | 0 | `F46B50410B314BFAF445C05A7CA4B532A9AEBEC303345143BE007F3CDFF450F6` |
| `ConsumerAttempt` | 5 | 0 | 0 | `10716DDF9969F43F89386FE7D1E62BC5005A44E01A37DF3F431D99C3C3835A1C` |
| `Delivery` | 4 | 0 | 0 | `D9683F7D32703CA99A8E9E0E26AE2D88C2A6329722AE8A60E424729BB4AF161E` |
| `EffectCue` | 83 | 0 | 0 | `692260C84F0C8DD889CC7E26A4E79887C26FBC18A3025FEAB5B4DC19092BDFE9` |
| `Presentation` | 4 | 0 | 0 | `EA3EA017E4F8A099EB5EEA5B60333A88577C115301C5D6A38F431521FE3BB51B` |
| `ProductSession` | 1 | 0 | 0 | `262ACB9ACA1F28E1BE1AAA60E12BCE1A0C587C9860A4165341B0FA7CB35E81EB` |
| `EvaluationRoute` | 1 | 0 | 0 | `C07E5786C49A768DD662854067B9E67C53C221658813CFF24519FB610608E4F9` |
| `Shanmen.0_0_10` | 649 | 0 | 0 | `1B0A9787A0E5E3D62037BCE7B2739F5D895FD60920A19501AB528C644EC49603` |

最终 21 份 selected phase 全部 Fail `0`、terminal `1`、error/Fatal/Unhandled/Ensure `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=28 Logs=21
SELF_TEST: PASS 216/216
DECISION_BOUNDARY: PASS Host mutation=0 Executor invocation=0
EXECUTION_BOUNDARY_SCAN: PASS
RUNTIME_BOUNDARY_SCAN: PASS
AUTHORITY_SCAN: PASS
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：5 actions / 10.11s / exit `0`；
- Game final：4 actions / 23.63s / exit `0`；
- Editor final：0 actions / 0.89s / exit `0`；
- Editor DLL：13,818,368 bytes / SHA `F108C39602F2FE72A09BEFE9904F3FD650F5E71EC2245018E12558E296794C5F`；
- Game EXE：355,343,360 bytes / SHA `696C38087F792C5F6FBBF37C6F0F80D87395E515875B0B783270D88C2D65E29B`；
- 所有有效构建无源码失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

首次 compile 与 focused Automation 均成功；focused initial `5/5`、exit `0`、SHA `CC80655853DED4B4ACB7811A1C7030EC9D4040891AF87A1E8F5E85AD66FEC212`。本轮无源码、测试或构建失败。

UE selected phase 前仍有 13 条既有 diagnostics。宽组有 `generate_204` 网络探测超时/large-delta warning，但持续推进并以原生 exit `0` 完成。LinuxArm64/VisionOS SDK 提示不影响 Win64。

## 7. 修改、兼容性与边界

Report/Log 前 5 个代码/流程文件净变更 `+1065 / -0`。既有 PreparedRetry/PreparedDispatch/transaction/Host/Router/Session API 未改；新层只追加 immutable decision 与 stateless service。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。长期未跟踪资料保持未暂存。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-27-sword-rhythm-cue-retry-decision>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-27-sword-rhythm-cue-retry-decision/Docs/Report/Dev.D.UE.0.0.10.P12.27.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-27-sword-rhythm-cue-retry-decision/Docs/Log/Dev.D.UE.0.0.10.P12.27.r0_log.md>
