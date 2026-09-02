# Dev.D.UE.0.0.10.P16.9.r0 Development Log

## 1. 目标与基线

- 基线提交：`417a4efa35d3a8ca8b45dbea0cd631ec0b48006d`（P16.8 durable intents）；
- 分支：`agent/0.0.10-p16-9-meridian-shock-recovery-lifecycle`；
- 目标：把 schema-2 intent/proof store 接入唯一 Meridian Shock treatment Product flow，关闭 proof publish 前的盲重放窗口；
- 保持 ShanmenItems 为 item/ledger 唯一真值，condition component/store 为 condition/evidence 唯一真值。

## 2. 审计结果

P16.8 前生产顺序为 `prepare -> condition mutation -> RecordProof -> item commit -> ForgetProof`。若 transient journal 丢失且 item ledger 只有 pending prepare、store 又没有 proof，旧恢复路径会重新构造 treatment intent 并调用 condition mutation；该分支无法区分“mutation 尚未发生”和“mutation 已发生但 proof 尚未发布”。

确认的既有安全接缝：

- treatment identity 由 Run、target、timeline、item、condition revision 确定性派生；
- condition authority 可按 TreatmentId 读取 exact processed receipt；
- item cancel 只接受仍 active 的 exact pre-treatment revision；
- ShanmenItems prepare/finalize ledger 可在 authority 重建后恢复；
- P16.8 store 已提供 `RecordIntent`、`PromoteIntentToProof`、`ForgetIntent` 与 schema 1 -> 2 migration。

## 3. 实现记录

### 正常 Route

1. durable item prepare；
2. capture canonical recovery intent；
3. `RecordIntent` durable；
4. condition treatment；
5. capture canonical proof；
6. `PromoteIntentToProof` atomic document mutation；
7. exact item commit；
8. `ForgetProof`。

新增 Route error / test interruption 状态：

- `IntentCaptureRejected`、`IntentPersistenceRejected`、`IntentCleanupRejected`；
- `InterruptedAfterIntentPersistence`、`InterruptedAfterConditionMutation`；
- 保留 `InterruptedAfterProofPersistence` 作为 commit-only 窗口。

### Durable recovery

- 先索引并验证所有 exact prepare 与唯一 terminal receipt；
- committed proof 与 cancelled intent 只做 stale evidence cleanup；
- pending intent + exact processed receipt：restore prepare、promote proof、commit、cleanup；
- pending intent + exact unchanged active revision：cancel、cleanup；
- pending proof：保持 commit-only recovery；
- ambiguous intent：不 mutation、不 finalize、保留 intent、返回失败；
- live transient journal 继续拥有自己的 prepare，durable scanner 不抢占；
- legacy no-evidence prepare 只允许 exact receipt commit 或 exact revision cancel，不再 blind treatment replay。

## 4. 故障与修正

首次接口编译发现 recovery helper lambda 错误地尝试以局部变量语法捕获 Route member `Correlation`。修正为显式捕获 `this` 后重新构建通过；未改变 identity 比较内容。

代码复核时又发现 intent cleanup 与 proof cleanup 不能共用宽泛 `IsSuccess()` 语义。新增 `IsIntentForgetSuccess`，只接受 `IntentForgotten` / `IntentAlreadyAbsent`，防止 promotion 状态被误判为取消清理完成。所有 `ForgetProof` 路径继续使用 proof mutation 的成功语义。

## 5. Automation 变更

- 新增 `Lifecycle.IntentJournalRetry`；
- 将既有 proof-write fence 升级为 `Lifecycle.IntentPersistenceBeforeConditionMutation`；
- 新增 `Recovery.IntentAfterMutationPromote`；
- 新增 `Recovery.IntentBeforeMutationCancel`；
- 新增 `Recovery.IntentAmbiguityFence`；
- `Recovery.ActiveCondition` 改为验证 legacy exact pre-mutation prepare 被 cancel，而非重放治疗；
- pre-mutation 场景额外注入 cancel 后 AtomicReplace cleanup 失败，再次恢复只清理一次。

关键实测状态：

```text
live-intent-retry:   recovered=1 processed=1 commit=1 cancel=0 intents=0 proofs=0
intent-write-fence:  persistence rejected before mutation; retry commit=1
post-mutation:       recovered=1 processed=1 commit=1 cancel=0 intents=0 proofs=0
pre-mutation:        cleanupBlocked=1 recovered=1 processed=0 commit=0 cancel=1 intents=0 proofs=0
ambiguous-intent:    recovered=0 unchanged=1 processed=0 commit=0 cancel=0 intents=1 proofs=0
```

## 6. 最终测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `P16_9_Treatment.log` | `Shanmen.0_0_10.Product.MeridianShockTreatment` | 32/0 | `C6B8443446130D7D6168EF032A61BCABF32E4423C925EE264AF354ED30F569A8` |
| `P16_9_Full.log` | `Shanmen.0_0_10` | 745/0 | `9B41A052F6495F747D3DFB0FC9D81F39908C8E2D81235EA6BDFF9DBF9D8D608A` |
| `P16_9_CodeB.log` | `demo_map.CodeB` | 60/0 | `3C7BD5B57F6015F2FDE9866ABFFCF70F8B54C20EAC81DAA2499CB5BC3E5A44A2` |
| `P16_9_ItemEconomySchema.log` | `demo_map.ItemEconomySchema` | 24/0 | `5CC5ADCE7EBF6EB74A7156676C041E2F8A64BB93A8C5D8D2F07569836005F1C3` |
| `P16_9_ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `6466D765BE4F26F72BBD53105D0AC3750003D4540AC4BF7EC0C2FDEEDBE9AFCD` |
| `P16_9_P4_Hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | `F48BC170FD7E35AD4D614B9BFCB42A69056CE0E2077A7CAA6DFE2E57FE8A94A3` |
| `P16_9_Profile.log` | `demo_map.Profile` | 211/0 | `03C86FB076C797AF9AB3180F6D59B9FCE4A4A803F56934094B2FC26668F12567` |

七份日志均为 native exit 0，Fail/Fatal/Unhandled/Ensure 为 0。mapped legacy 合计 348/0。

## 7. 门禁与构建

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 273/273
JSON_PARSE: PASS Schema=1 Rules=161
PRODUCTION_BOUNDARY_SCAN: PASS HITS=0
CONTRACT_SCAN: PASS Order=509<571<696<742<769 DurableRecoveryMutationCalls=0
TEST_CONTRACT_SCAN: PASS Missing=0
GIT_DIFF_CHECK: PASS
```

- Game Development：Succeeded / 29 actions / 116.57s / native 0 / log SHA `9552F6C99DB32957A275E6BC5D25BD6F7E89B2EDEE184BDDB60E01F854019FAB`；
- Editor Development：Succeeded / 28 actions / 122.66s / native 0 / log SHA `F63EC16A0C8981F5F14102CFE4F65673157BA1AA2699BC4DC14581B21BC17976`；
- `demo_map.exe`：356,109,824 bytes / SHA `6E9A5731C945AA595072A2191AEFB3FC3056EB31345F8B8267FE53FD5AD8CCB0`；
- `UnrealEditor-demo_map.dll`：14,790,656 bytes / SHA `145AA64498196B410655A4650E4293D3109F500200FEED2A848CB777094D28D3`。

## 8. 边界

本阶段未修改 store schema、condition component、ShanmenItems、Profile、GameMode、PlayerController、地图或资源。未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或真实强杀测试。raw logs 仅保存在本地 `Saved/Logs`。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。歧义 intent 保持 durable 且阻止 teardown；这是一条显式 failure fence，不计作自动恢复成功。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-9-meridian-shock-recovery-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-9-meridian-shock-recovery-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P16.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-9-meridian-shock-recovery-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P16.9.r0_log.md>
