# Dev.D.UE.0.0.10.P16.5.r0 Report

## 1. 结论

P16.5 已为 P16.4 的 Meridian Shock treatment recovery proof 建立唯一的 **condition-domain durable store**。Store 以 Owner/Run 分区，严格校验版本化文档与 proof 集合，通过已验证临时文件、备份、同卷原子替换和提交后回读发布变更；损坏主文件可从有效备份恢复，未来 schema、读失败和无有效备份场景均失败关闭，不会静默清空或降级。

最终结果：

```text
Recovery Store focused:       4 Success / 0 Fail
Mapped legacy regressions:  348 Success / 0 Fail
Shanmen.0_0_10 full:        734 Success / 0 Fail
Regression coverage:        PASS (Changed=4 / Rules=1 / Required=10 / Logs=7)
Regression gate self-test:  PASS 271/271
Editor + Game Development:  PASS / native status 0
```

## 2. 唯一 owner 与真值边界

旧 `ProfileRepository` 已在 ShanmenItems 切换后被禁止继续承担高频产品写入：用陈旧 Profile 快照保存 condition proof 可能覆盖当前库存。ShanmenItems 则只拥有库存、prepare/commit ledger 与 processed request，不应反向拥有 condition history。

因此本轮新增独立的 `Fdemo_mapShanmenMeridianShockTreatmentRecoveryStore`，作为 treatment proof 的唯一持久化 owner。它只保存“某次 condition mutation 已提交”的证明，不保存：

- item quantity、inventory projection 或 item transaction truth；
- 当前 active condition、属性值或角色/世界状态；
- UI、输入、Actor、World、Timer 或随机状态。

这不是第二份库存，也不会从 proof 推测库存结果。P16.4 的 Product Route 仍以 ShanmenItems durable prepare/processed-request ledger 为唯一库存事实。

## 3. 文档与路径契约

生产路径固定为：

```text
<ProjectSavedDir>/ShanmenCombatConditions/MeridianShockTreatment/
  <OwnerId>/<RunId>.json
```

同目录使用 `<RunId>.json.bak` 与 `<RunId>.json.tmp`。空 Root、无效 OwnerId 或 RunId 会在路径落盘前被拒绝，不能退化到进程当前目录。

schema 1 文档精确包含：

```text
SchemaVersion, DocumentId, OwnerId, RunId, SaveGeneration,
CreatedUtc, LastSavedUtc, ProofSetId, Proofs
```

解析器要求字段集合精确匹配，proof 数量上限为 256。DocumentId 由 OwnerId + RunId 确定性派生；proof 按 TreatmentId 严格升序；proof identity 唯一且全部属于文档 Run；ProofSetId 由 schema、Owner、Run 和全部 canonical proof payload 重算。未知额外字段、重复或乱序 proof、跨 Run proof、错误 identity、非法时间或 generation 均被拒绝。

## 4. 写入、恢复与幂等语义

`RecordProof` 的发布顺序为：

```text
load/validate current generation
  -> canonicalize candidate
  -> write temp + full flush/close
  -> byte/document read-back verification
  -> verify current primary still matches loaded generation
  -> copy and byte-verify backup
  -> same-volume atomic replacement
  -> committed-primary byte/document read-back
```

首个 publish 要求 primary 与 backup 均不存在；后续 publish 会用已加载文档对当前 primary 做乐观一致性检查。精确 proof replay 返回 `AlreadyRecorded`，不增加 generation、不写磁盘；同一 TreatmentId 的不同 proof 返回 `Conflict`。`ForgetProof` 对不存在的 proof 幂等，对存在 proof 发布下一 generation；即使集合变空也保留可审计文档。

加载时优先验证 primary。primary 损坏且 backup 有效时，先将损坏原始字节保存到 `Corrupt` 并验证，再按 temp/flush/read-back/replace/read-back 流程恢复 backup，generation 不变。primary 缺失但 backup 有效时同样恢复。primary 或 backup 使用未来 schema 时拒绝降级和重置；存在不可读或非法 durable bytes 时不会误报 `Missing`/`AlreadyAbsent`。

Store 暴露明确的 save/load/mutation status 与 diagnostic，并在开发测试中提供有界 failure-stage injection。它不提供跨进程锁；唯一 owner 必须串行调用，跨进程并发协调留给后续宿主集成。

## 5. 新增验证与自查修正

新增 4 个 focused tests：

- `RecoveryStore.RoundTripReplayForget`：generation 1/2/3、canonical 排序、精确 replay 零写入、reload 与 forget；
- `RecoveryStore.FailureIsolationRetry`：原子替换前失败保持 primary，随后相同 proof 重试只提交一次；
- `RecoveryStore.BackupRecoveryFutureSchema`：损坏 primary 隔离、backup 恢复、未来 schema 不降级，并覆盖“只有未来 schema backup”场景；
- `RecoveryStore.IdentityConflictFence`：同 TreatmentId 不同 proof、跨 Run proof 与空 Root 全部失败关闭。

开发中保留了首次失败证据：focused 初跑为 3/1，原因是测试在成功重试之后才读取 primary，却把该读取用于断言“注入失败后的 primary 未变化”；调整证据采样顺序后通过，生产实现未因这条测试失败而放宽。失败日志 SHA-256：`2C6AD8F14223ABE727A6ABFC82B29B8BBF57F5471B65CF2729149F682B4DFAAA`。

另一次 full suite 在 579/0 时主动终止：自查发现空 Root 可被 `ConvertRelativePathToFull` 解释为进程目录。修复为先拒绝空 Root 后重跑。该非终态日志 SHA-256：`84048CA7B02DB7059F78D19E1A024E89E1FCC23789A029D98BE3080C061E6612`。

随后一次 734/0 完成后继续审查，发现“primary 缺失 + 仅 future-schema backup”会被误分类为 Missing；修正为 `FutureSchemaRejected` 并补测试，再执行最终 focused、full 与全部 mapped regressions。前一份 734/0 日志不作为最终证据，其 SHA-256 为 `EBF0EC26EE396C1788536B9F3DCA76A4C4F5F415C2597CE45C82E485536907B8`。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Recovery Store focused | 4 | 0 | `A83E9C9240F8AC237B7FC73EC1A90CA8DCD30BAA9B273A1E456A39BE8CDB094B` |
| `Shanmen.0_0_10` full | 734 | 0 | `3DF1261F9DC159A0C9D046523B95243A177BDD9C84BA3195F0D0998030CBCEBD` |
| CodeB | 60 | 0 | `B6812ADA84D4BB1A067C50ED3B44ED099D04B370AF215DA0FAC446C08EE353E5` |
| ItemEconomySchema | 24 | 0 | `53718F256C5523531545AF7CDB36C5B0DA3A35E075B4AA332E5A9F695013B3AC` |
| ItemUseAndArmor | 46 | 0 | `0357C8D2AAE5434928F5AAC079821C876818D7323F310E9884F277BDCEA9CF00` |
| P4 Hotbar | 7 | 0 | `00CE982027A1893A75F40B9D67964876D75EB1A5A16169369CEA0A1D127AFE95` |
| Profile | 211 | 0 | `A29B7BB2DC8A8581525AB913FBEB50EFBE4FB438C9D55748121AE4D79FDE59DA` |

mapped legacy 合计 348/0。Full suite 首末 Success 时间为 `10:51:52.504 -> 11:19:41.488 UTC`，约 27m48.98s。最终 7 份 Automation 日志均包含 native terminal-success，Fail、Fatal、Unhandled、Ensure 与网络错误计数均为 0。

## 7. 改动—回归门禁与静态边界

3 个 RecoveryStore 源码/测试文件与 regression map 共 4 个路径命中 `MeridianShockTreatmentProductFlow`。10 个必跑组全部由 7 份健康日志覆盖；mapping 已从 `RecoveryProof` 扩展为 `Recovery(?:Proof|Store)`，后续修改 store 不会绕过门禁。

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 271/271
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS HITS=0
GIT_DIFF_CHECK: PASS
```

流程文件 SHA：coverage gate output `905A0670A22938A427583566E3955245308F72C3F677217840EA00E548FE90E2`；self-test output `A34FB07154B570E39D7736BF27B33745DA4F6517015445ADCBA8ED7CA4004E6A`；mapping JSON `62E7F055919C0F1514AAFBCF83227BE57968F3087CB1687F9C8AA76F6FA786C7`。

生产 store 静态扫描确认不依赖旧 `demo_mapItemSubsystem`、`Fdemo_mapPersistentProfile`、`Fdemo_mapProfileRepository`、`UWorld`、`AActor`、Spawn/ApplyDamage、RNG、Timer、Enhanced Input、Widget 或 Slate。

## 8. 构建证据

最终源码使用 `-WaitMutex -NoUBA -MaxParallelActions=1` 构建。Game target 逐文件编译新增 store 与 tests；Editor target 随后复核为 up to date。两个命令均显示 `Result: Succeeded`，原生退出码均为 0。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 4 / 24.92s | `77F3522ADBD630A4FB93FBCB846DFDD8F875F63CFAD10A72C0BC7455F0404D53` |
| Editor Development final check | Succeeded | 0 (up to date) / 1.00s | `0193C32D0165DEE11FE8A8AED33913A74E645D0B76132DA74D3FD9C3FD25104F` |

最终产物：

- `demo_map.exe`：355,998,720 bytes，SHA-256 `238599C20A3DDAECFE93FDA809AA9BE9E69488869FF65453A2FF25556C32B5C4`；
- `UnrealEditor-demo_map.dll`：14,641,664 bytes，SHA-256 `9E7DAB5E76BEC384D541C39B688B94CF75BCFA29604966B76431544C9398F47C`。

## 9. 修改范围

本轮新增 RecoveryStore header/cpp、4 个 Automation tests，并更新 regression mapping；连同本 Report 与 Development Log，计划提交 6 个文件。没有修改 Profile、ShanmenItems、Product Route、GameMode、PlayerController、地图、资源、存档 schema、Windows、UE Engine 或用户配置。

raw logs 仅保存在本机 `Saved/Logs`。长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 10. P/F 边界与后续

本 Report 证明 P 阶段 durable store contract、真实磁盘原子写入/恢复 Automation、NullRHI full/legacy regressions、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P16.5 尚未把 store 自动接入 Product Route / Session / Lifecycle，因此不声称当前产品流程已经获得端到端 crash recovery。下一阶段应由 Product 生命周期在 condition mutation 后、item commit 前保存 proof，在恢复时加载并提供 proof，成功完成或安全终止后再 forget；同时必须保持唯一 condition-store owner、串行写入和 ShanmenItems 唯一库存权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-5-meridian-shock-proof-store>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-5-meridian-shock-proof-store/Docs/Report/Dev.D.UE.0.0.10.P16.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-5-meridian-shock-proof-store/Docs/Log/Dev.D.UE.0.0.10.P16.5.r0_log.md>
