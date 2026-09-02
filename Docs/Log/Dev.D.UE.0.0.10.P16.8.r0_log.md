# Dev.D.UE.0.0.10.P16.8.r0 Development Log

## 1. 目标

把 P16.5 proof-only recovery store 从 schema 1 安全升级为 schema 2，在同一 Run-scoped durable document 中保存 P16.7 prepared intents 与 committed proofs；提供严格 N-1 migration、幂等 intent mutation 和原子 intent-to-proof promotion，但暂不改变生产 Product Route/Lifecycle。

## 2. 实现

- 文档 schema 从 1 升至 2，增加 canonical IntentSetId 与 sorted intents；
- 保留 schema-1 exact parser 和旧 ProofSetId 重新派生校验；
- 有效 schema-1 primary/backup 经 verified temp、full flush、backup、same-volume replace 与 committed read-back 迁移，generation 精确增加一次；
- 新增 `RecordIntent`、`PromoteIntentToProof` 与 `ForgetIntent`；
- exact intent replay、promotion replay 与 cancellation replay 都不重复写盘或增加 generation；
- promotion 在一次 document mutation 中删除 intent并添加 matching proof；
- `RecordProof` 拒绝绕过 durable intent；
- current/legacy corruption、identity conflict、future schema、跨 Run 与超出 bounds 全部失败关闭，不静默 reset/downgrade；
- 文档总 entry 上限固定为 256，intent/proof 同 TreatmentId 重叠无效。

## 3. 新增测试

```text
Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.PreviousSchemaMigration
Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.IntentPromotionAtomicity
Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryStore.IntentFailureConflictFence
```

重点覆盖 primary schema-1 migration、backup-only migration、legacy bytes backup、单次 generation advance、exact replay、atomic promotion、durable cancellation、conflict/bypass fence、pre-commit replacement failure、retry 与 corruption fail-closed。

## 4. 最终验证

| Group | Result | SHA-256 |
|---|---|---|
| Recovery store focused | 7/0 | `A3CA2C1B341B3DE2669A462E41742B2E2679FE5F94900A71F154878EFF1E0300` |
| Meridian treatment focused | 28/0 | `0A1BC1848D97BA933B7D482CC8AE06B481A168D6BD8557D5C6B8E6C52E7DC1C4` |
| Shanmen.0_0_10 full | 741/0 | `5BE38A64A01C949416BCC1A721B11F2690B7757AF1263E3E61D61958F7A56043` |
| CodeB | 60/0 | `48F9F48C48F0170A228D8A438AEFDE6D0469C02FFE1819ADBDA2304FCA7924E1` |
| ItemEconomySchema | 24/0 | `7C92E6DED1C4EA6018F3E79789A7997DE65B3794C031F4AA5131B82401470F82` |
| ItemUseAndArmor | 46/0 | `AD2C53A1AF99B331D7C774ADDF7DF49C321D119EEF0FC6AD01AF612D0C181AFE` |
| P4.Hotbar | 7/0 | `F2F974A4A63868BA2E6F5642BBFD25E01645CCBCA1694E98F8C1A9FF5F2AB398` |
| Profile | 211/0 | `FE97437C0815717636574DB6D069454F21C3068AB8EE5BCEBDCF75C52699B94C` |

mapped legacy 合计 348/0；最终 Automation 日志 Fail/Fatal/Ensure 均为 0，并包含 native terminal-success evidence。

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 273/273
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS HITS=0
CONTRACT_SCAN: PASS schema=1->2 durable-intent promotion=yes lifecycle-integration=no
GIT_DIFF_CHECK: PASS
```

regression map 未修改，SHA `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。

## 5. 构建

- Game Development：Succeeded / 31 actions / 122.91s / native status 0 / log SHA `131745C6A71BC8B51CDAA450332207BFB5BA1F0D64A968C5A46E89908D472480`；
- Editor Development：Succeeded / up to date / 1.01s / native status 0 / log SHA `6038B09D3E3AEE9DC39A98C563B887D9968F56142800B5B20768FC2907820070`；
- Game EXE：356,084,224 bytes / SHA `9362E4EED071884664742427DE81C967C761884F2EF8C3BCAB5989973878CA91`；
- Editor DLL：14,758,912 bytes / SHA `6B279C1B72F3FFCB95996105568944325684F9AB75EEC64FD4ABFF778DD54FA1`。

## 6. 边界与后续

本轮没有修改 Product Route/Lifecycle，因此新 intent 尚未进入生产执行顺序，P16.6 crash window 仍未关闭。P16.9 应接入 `item prepare -> durable intent -> condition mutation -> intent-to-proof promotion -> item commit -> proof cleanup`，并在 reopen 时根据 expected revision 与 condition authority 状态做确定性恢复，不能盲目重放 mutation。

store 不负责跨进程锁；Lifecycle 继续作为唯一串行 owner。仅执行 P 阶段 C++、NullRHI Automation、静态检查与 Development builds；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户资料未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-8-meridian-shock-durable-intents>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-8-meridian-shock-durable-intents/Docs/Report/Dev.D.UE.0.0.10.P16.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-8-meridian-shock-durable-intents/Docs/Log/Dev.D.UE.0.0.10.P16.8.r0_log.md>
