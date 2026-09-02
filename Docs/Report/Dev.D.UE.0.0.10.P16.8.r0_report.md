# Dev.D.UE.0.0.10.P16.8.r0 Report

## 1. 结论

P16.8 已把 Meridian Shock treatment recovery store 从 proof-only schema 1 升级为同时保存 prepared intents 与 committed proofs 的 schema 2，并提供严格的 N-1（schema 1 -> 2）持久迁移。

本轮新增三项 durable 能力：prepared intent 可幂等写盘、未执行 intent 可幂等取消、已执行 intent 可在同一个 recovery document generation 中原子替换为 exact proof。`RecordProof` 不能绕过同 TreatmentId 的 durable intent；调用方必须使用 intent-to-proof promotion，因此磁盘状态不会同时存在同一次治疗的 intent 与 proof。

最终结果：

```text
Recovery store focused:      7 Success / 0 Fail
Treatment flow focused:     28 Success / 0 Fail
Mapped legacy regressions: 348 Success / 0 Fail
Shanmen.0_0_10 full:       741 Success / 0 Fail
Regression coverage:       PASS (Changed=3 / Rules=1 / Required=10 / Logs=7)
Regression gate self-test: PASS 273/273
Editor + Game Development: PASS / native status 0
```

P16.8 只完成 durable store 与迁移边界，没有把新 intent API 接入 Product Route/Lifecycle。因此 P16.6 记录的 condition mutation 与 proof publish 之间的生产路径 crash window 尚未宣称关闭；该运行时接入属于下一阶段。

## 2. Schema 2 文档契约

`Fdemo_mapShanmenTreatmentRecoveryDocument` 当前 schema 为 2，固定保存：

- DocumentId、OwnerId、RunId、SaveGeneration 与 UTC 创建/保存时间；
- canonical `IntentSetId` 与按 TreatmentId 排序的 prepared intent 数组；
- canonical `ProofSetId` 与按 TreatmentId 排序的 committed proof 数组；
- intents 最多 256、proofs 最多 256，且二者合计最多 256。

文档校验要求所有 entry 属于同一 Run，IntentId、ProofId 与 TreatmentId 均有效且唯一；排序必须严格递增，同一个 TreatmentId 不得同时出现在 intent 和 proof 数组。两个 set ID 都从 schema、Owner、Run 与各自 canonical payload 重新派生，任何字段、顺序或编码篡改均失败关闭。

当前 schema 2 JSON 使用 exact field set：

```text
SchemaVersion, DocumentId, OwnerId, RunId, SaveGeneration,
CreatedUtc, LastSavedUtc, IntentSetId, Intents, ProofSetId, Proofs
```

额外字段、缺失字段、非整数 generation、无效 GUID、跨 Run entry、非 canonical payload、future schema 与损坏 set identity 均不会被静默接受或重置。

## 3. Schema 1 -> 2 持久迁移

读取器保留 schema 1 的 exact legacy field set，并先按旧规则完整验证 DocumentId、Owner/Run、generation、时间戳、proof codec、proof 排序和旧 `ProofSetId`。只有完整有效的 schema-1 文档才投影到 schema 2；迁移时：

- 保留 DocumentId、OwnerId、RunId、CreatedUtc 与全部 proof；
- 初始化空 intents，并派生 schema-2 IntentSetId/ProofSetId；
- SaveGeneration 精确增加一次，LastSavedUtc 更新一次；
- 通过同一套 temp write、full flush、read-back、backup、same-volume replace 与 committed read-back 路径写盘；
- 原 schema-1 primary bytes 被逐字节保留为 `.bak`；
- 第二次 load 直接读取 schema 2，不重复迁移或增加 generation。

primary 缺失但 schema-1 backup 有效的情况，会先逐字节恢复 primary，再执行同一次有界迁移。损坏 schema 1、损坏 schema 2 或 future schema 均不迁移、不降级、不创建空文档；没有有效 backup 时保持原故障证据。

## 4. Durable intent 与原子 promotion

新增 store API：

- `RecordIntent`：第一次 durable 写入 prepared intent；exact replay 不写盘、不增加 generation；相同 TreatmentId 的不同 intent 返回 conflict；若 exact proof 已存在则返回 already promoted。
- `PromoteIntentToProof`：要求调用方同时提供 exact intent 与 matching proof；只有磁盘中存在 exact durable intent 才能执行。一次 document mutation 删除 intent、添加 proof并增加一个 generation。
- `ForgetIntent`：取消未执行 intent；重复取消为幂等成功。若 proof 已存在，只返回 already promoted，不会误删 proof。
- `RecordProof`：继续兼容没有 prepared intent 的旧调用路径；一旦同 TreatmentId intent 已持久化，就拒绝旁路写 proof。

promotion 在 atomic replace 前失败时，已提交 primary 仍保持 durable intent；同一请求重试后只生成一个 proof 和一个新 generation。post-commit 状态可通过 reopen 与 already-promoted 语义消除重复提交歧义。

## 5. 新增测试

新增 3 个 Automation tests：

- `RecoveryStore.PreviousSchemaMigration`：验证 schema-1 primary generation 7 -> schema-2 generation 8、legacy bytes 原样进入 backup、第二次 load 无额外写入、backup-only generation 11 -> 12，以及损坏旧 ProofSetId 不迁移不重置；
- `RecoveryStore.IntentPromotionAtomicity`：验证 intent generation 1、exact replay byte/generation 稳定、promotion generation 2、promotion backup 等于 pre-promotion intent 文档、already-promoted replay，以及 intent cancellation generations 3/4；
- `RecoveryStore.IntentFailureConflictFence`：验证同 TreatmentId 不同采样 intent 冲突、mismatched proof 被拒、`RecordProof` 旁路被拒、AtomicReplace 失败保留 durable intent、重试只提交一次，以及损坏 current intent 无 backup 时失败关闭。

RecoveryStore focused 从 4 增至 7，完整 Meridian Shock treatment flow 从 25 增至 28；全套 `Shanmen.0_0_10` 从 738 增至 741，最终全部通过。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Recovery store focused | 7 | 0 | `A3CA2C1B341B3DE2669A462E41742B2E2679FE5F94900A71F154878EFF1E0300` |
| Treatment flow focused | 28 | 0 | `0A1BC1848D97BA933B7D482CC8AE06B481A168D6BD8557D5C6B8E6C52E7DC1C4` |
| `Shanmen.0_0_10` full | 741 | 0 | `5BE38A64A01C949416BCC1A721B11F2690B7757AF1263E3E61D61958F7A56043` |
| CodeB | 60 | 0 | `48F9F48C48F0170A228D8A438AEFDE6D0469C02FFE1819ADBDA2304FCA7924E1` |
| ItemEconomySchema | 24 | 0 | `7C92E6DED1C4EA6018F3E79789A7997DE65B3794C031F4AA5131B82401470F82` |
| ItemUseAndArmor | 46 | 0 | `AD2C53A1AF99B331D7C774ADDF7DF49C321D119EEF0FC6AD01AF612D0C181AFE` |
| P4 Hotbar | 7 | 0 | `F2F974A4A63868BA2E6F5642BBFD25E01645CCBCA1694E98F8C1A9FF5F2AB398` |
| Profile | 211 | 0 | `FE97437C0815717636574DB6D069454F21C3068AB8EE5BCEBDCF75C52699B94C` |

mapped legacy 合计 348/0。Full suite 的 741 项全部成功、0 项失败，首末 Success 间约 28m08.56s。日志中的长 scheduler delta 均被 Automation Controller 明确忽略，对应用例最终成功，不构成失败。

## 7. 改动—回归门禁与静态边界

3 个修改源文件全部命中 `MeridianShockTreatmentProductFlow` 映射。10 个必跑组由 7 份健康日志完整覆盖：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 273/273
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS HITS=0
CONTRACT_SCAN: PASS schema=1->2 durable-intent promotion=yes lifecycle-integration=no
GIT_DIFF_CHECK: PASS
```

regression map 未修改，SHA-256 为 `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。静态扫描确认 store 不依赖旧 ItemSubsystem/Profile、World/Actor、Spawn/ApplyDamage、UI、输入、Timer 或 RNG；本轮也没有修改 Product Route 或 Lifecycle。

## 8. 构建证据

最终源码使用 `-WaitMutex -NoUBA -MaxParallelActions=1` 构建。Game 目标执行 31 个动作并成功链接；随后 Editor 目标确认同一源码图已 up to date。两个目标原生退出码均为 0。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 31 / 122.91s | `131745C6A71BC8B51CDAA450332207BFB5BA1F0D64A968C5A46E89908D472480` |
| Editor Development | Succeeded | 0 / 1.01s | `6038B09D3E3AEE9DC39A98C563B887D9968F56142800B5B20768FC2907820070` |

最终产物：

- `demo_map.exe`：356,084,224 bytes，SHA-256 `9362E4EED071884664742427DE81C967C761884F2EF8C3BCAB5989973878CA91`；
- `UnrealEditor-demo_map.dll`：14,758,912 bytes，SHA-256 `6B279C1B72F3FFCB95996105568944325684F9AB75EEC64FD4ABFF778DD54FA1`。

## 9. 修改范围与 P/F 边界

本轮修改 recovery store header、implementation 与 Automation tests，并新增本 Report 与 Development Log，计划提交 5 个文件。没有修改 Product Route/Lifecycle、Profile、ShanmenItems、GameMode、PlayerController、地图、资源、存档 schema、Windows、UE Engine 或用户配置。

本 Report 证明 P 阶段 durable document contract、N-1 migration、intent idempotency/promotion/failure fences、NullRHI focused/full/legacy regressions、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

raw logs 仅保存在本机 `Saved/Logs`。长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 10. 下一阶段

P16.9 应把 schema-2 store 接入唯一的 Meridian Shock treatment Product Lifecycle，并保持明确顺序：

```text
item durable prepare
-> durable RecordIntent
-> condition authority mutation
-> atomic PromoteIntentToProof
-> item commit
-> proof cleanup
```

reopen 时必须用 durable intent 的 expected condition revision 与当前 condition authority 状态判定 mutation 尚未发生或已经发生，再分别取消 intent/回滚 prepared item，或完成 exact proof promotion 与 item commit；不能盲目重放 condition mutation。Lifecycle 仍应是唯一串行 owner，store 本身不新增跨进程锁。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-8-meridian-shock-durable-intents>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-8-meridian-shock-durable-intents/Docs/Report/Dev.D.UE.0.0.10.P16.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-8-meridian-shock-durable-intents/Docs/Log/Dev.D.UE.0.0.10.P16.8.r0_log.md>
