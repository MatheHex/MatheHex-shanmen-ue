# Dev.D.UE.0.0.10.P16.9.r0 Report

## 1. 结论

P16.9 已把 P16.8 的 schema-2 durable intent 接入唯一 Meridian Shock 治疗 Product Route / Session / Lifecycle。生产顺序现在固定为：

```text
item durable prepare
  -> condition-domain durable intent
  -> condition mutation
  -> atomic intent-to-proof promotion
  -> exact item durable commit
  -> durable proof cleanup
```

恢复入口不再把“存在未完成 item prepare、但缺少 proof”解释为可以再次治疗。重开后只接受三种可证明状态：精确 processed receipt 表示治疗已发生，可提升 proof 并只提交物品；精确未变化的 active condition revision 表示治疗未发生，可安全取消物品；两种证据都不存在时保留 durable intent 并失败关闭，不猜测、不重放 condition mutation。

最终结果：

```text
Meridian Shock treatment focused:   32 Success / 0 Fail
Shanmen.0_0_10 full:               745 Success / 0 Fail
Mapped legacy regressions:         348 Success / 0 Fail
Regression coverage:               PASS (Changed=6 / Rules=1 / Required=10 / Logs=7)
Regression gate self-test:         PASS 273/273
Game + Editor Development:         PASS / native status 0
```

## 2. Write-ahead 生命周期

Route journal 新增 canonical `RecoveryIntent`、`bIntentRecorded` 与 `bIntentCleanupPending`。item prepare 成功后，Route 必须先从 exact treatment intent 与 fixed-timeline sample 捕获并持久化 recovery intent；只有 `IntentRecorded` 或 exact `IntentAlreadyRecorded` 才允许调用 condition authority。

condition treatment 成功后，Route 从 committed receipt 捕获 canonical proof，并通过一次 `PromoteIntentToProof` 文档 mutation 删除 intent、加入 matching proof。promotion 未成功前禁止 item commit；item commit 已成功但 proof cleanup 失败时，重试只清理 proof，不重复扣除物品。

静态顺序复核定位为：

```text
RecordIntent              line 509
TryTreatMeridianShock     line 571
PromoteIntentToProof      line 696
CommitTreated             line 742
ForgetProof               line 769
```

durable recovery 函数范围内 `TryTreatMeridianShock` 调用数为 0。condition mutation 只存在于正常、持有 immutable command 的 Route 执行路径。

## 3. 取消与清理顺序

若正常执行中的 condition treatment 被拒绝，Route 只有在 condition authority 仍能证明同一个 active revision 时才允许取消 item prepare。取消先由 ShanmenItems durable ledger 提交，随后才删除 recovery intent。

若取消已落盘而 intent cleanup 写入失败，journal 保持 recovery-required。重试或重建 Lifecycle 会验证 exact cancelled terminal receipt，只删除 stale intent；不会再次取消、治疗或提交物品。`ForgetIntent` 只接受 `IntentForgotten` 与 `IntentAlreadyAbsent`，不会把“已提升为 proof”误当作取消清理成功。

## 4. 重开恢复矩阵

| Durable / authority evidence | 决策 | 允许的副作用 |
|---|---|---|
| Intent + exact processed treatment receipt | 恢复成功 | intent 原子提升为 proof，item 精确提交一次，proof 清理 |
| Intent + exact unchanged active revision | 恢复成功 | item 精确取消一次，intent 清理；不治疗 |
| Intent + cancelled terminal item receipt | 恢复成功 | 仅清理 stale intent |
| Proof + pending item prepare | 恢复成功 | 仅恢复/核对 condition receipt，item 精确提交一次，proof 清理 |
| Proof + committed terminal item receipt | 恢复成功 | 仅清理 stale proof |
| Intent 既无 exact receipt，也无 exact unchanged revision | 失败关闭 | 无 item commit/cancel，无 condition mutation，intent 保留 |
| orphan、重复、跨 Run/target/timeline、definition 或 terminal receipt 冲突 | 失败关闭 | 无业务 mutation |

同一进程中仍由 transient journal 拥有的 pending prepare 不会被 durable scanner 抢先取消；Session 会在扫描后用原 immutable command 恢复该 journal。多个 pending treatment prepare 对同一个 condition authority 仍被视为歧义并拒绝。

历史上没有 schema-2 intent/proof 的 legacy pending prepare 也不再盲目执行治疗：若已有 exact runtime receipt，则先建立 proof 再 commit；若 condition 仍是 exact pre-mutation revision，则安全 cancel；其它状态失败关闭。

## 5. Automation 覆盖

新增 4 个测试，并升级 1 个既有故障注入测试：

- `Lifecycle.IntentJournalRetry`：intent 已持久、transient journal 仍存活时重试 exact command，最终 treatment/commit 各一次；
- `Lifecycle.IntentPersistenceBeforeConditionMutation`：intent 写入失败时 condition 与 item finalization 均不发生；解除故障后完整提交；
- `Recovery.IntentAfterMutationPromote`：condition 已有 exact receipt、intent 尚未提升时，重建 authority 后 promotion + commit-only；
- `Recovery.IntentBeforeMutationCancel`：condition 仍是 exact revision 时 cancel-only；并注入 cancel 后 cleanup 失败，第二次恢复只清理；
- `Recovery.IntentAmbiguityFence`：condition 证据丢失时 snapshot 不变、commit/cancel 均为 0、intent 保留且 teardown 被拒绝。

focused suite 从 P16.8 的 28 增至 32，结果 32/0；完整 `Shanmen.0_0_10` 从 741 增至 745，结果 745/0。Full suite 首末 Success 时间为 `2026.09.02 15:24:46.695 -> 15:52:35.050 UTC`，约 27m48.36s。日志中的长 scheduler delta 均被 Automation Controller 明确忽略，对应用例最终成功，不构成失败。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Meridian Shock treatment focused | 32 | 0 | `C6B8443446130D7D6168EF032A61BCABF32E4423C925EE264AF354ED30F569A8` |
| `Shanmen.0_0_10` full | 745 | 0 | `9B41A052F6495F747D3DFB0FC9D81F39908C8E2D81235EA6BDFF9DBF9D8D608A` |
| CodeB | 60 | 0 | `3C7BD5B57F6015F2FDE9866ABFFCF70F8B54C20EAC81DAA2499CB5BC3E5A44A2` |
| ItemEconomySchema | 24 | 0 | `5CC5ADCE7EBF6EB74A7156676C041E2F8A64BB93A8C5D8D2F07569836005F1C3` |
| ItemUseAndArmor | 46 | 0 | `6466D765BE4F26F72BBD53105D0AC3750003D4540AC4BF7EC0C2FDEEDBE9AFCD` |
| P4.Hotbar | 7 | 0 | `F48BC170FD7E35AD4D614B9BFCB42A69056CE0E2077A7CAA6DFE2E57FE8A94A3` |
| Profile | 211 | 0 | `03C86FB076C797AF9AB3180F6D59B9FCE4A4A803F56934094B2FC26668F12567` |

mapped legacy 合计 348/0。七份最终 Automation 日志均包含 UE 5.8 native terminal-success marker，Fail、Fatal、Unhandled 与 Ensure 计数为 0。

## 7. 改动—回归门禁与静态边界

6 个修改过的 Product Route / Session / Lifecycle / tests 路径全部命中 `MeridianShockTreatmentProductFlow`。10 个必跑组由 7 份健康日志完整覆盖：

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 273/273
JSON_PARSE: PASS Schema=1 Rules=161
PRODUCTION_BOUNDARY_SCAN: PASS HITS=0
CONTRACT_SCAN: PASS Order=509<571<696<742<769 DurableRecoveryMutationCalls=0
TEST_CONTRACT_SCAN: PASS Missing=0
GIT_DIFF_CHECK: PASS
```

regression map 未修改，SHA-256 为 `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。生产 flow 静态扫描确认不依赖旧 ItemSubsystem/Profile、World/Actor、Spawn/ApplyDamage、UI、Enhanced Input、Timer 或 RNG。Automation fixture 仍有意创建旧 Profile/ItemSubsystem，以验证真实迁移后的 ShanmenItems 权威与 mapped legacy 回归；它们不是生产依赖。

## 8. 构建证据

最终源码使用 `-WaitMutex -NoUBA -MaxParallelActions=1` 逐目标构建。Game 与 Editor 都重新编译受改动头文件影响的源文件并成功链接，原生退出码均为 0。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 29 / 116.57s | `9552F6C99DB32957A275E6BC5D25BD6F7E89B2EDEE184BDDB60E01F854019FAB` |
| Editor Development | Succeeded | 28 / 122.66s | `F63EC16A0C8981F5F14102CFE4F65673157BA1AA2699BC4DC14581B21BC17976` |

最终产物：

- `demo_map.exe`：356,109,824 bytes，SHA-256 `6E9A5731C945AA595072A2191AEFB3FC3056EB31345F8B8267FE53FD5AD8CCB0`；
- `UnrealEditor-demo_map.dll`：14,790,656 bytes，SHA-256 `145AA64498196B410655A4650E4293D3109F500200FEED2A848CB777094D28D3`。

## 9. 修改范围与 P/F 边界

本轮修改 Product Route、Session、Lifecycle 及 route Automation tests 共 6 个源码文件，并新增本 Report 与 Development Log，计划提交 8 个文件。没有修改 recovery store schema/codec、condition component、ShanmenItems、Profile、GameMode、PlayerController、地图、资源、Windows、UE Engine 或用户配置。

本 Report 证明 P 阶段 write-ahead ordering、磁盘故障注入、Lifecycle/authority 重建恢复、unattended Automation、mapped regressions、静态检查与 Development builds。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package；没有执行真实断电或强杀进程测试。

歧义 intent 是可见的安全阻塞，不是自动成功：condition authority 若既不能提供 exact processed receipt，也不能证明 exact unchanged revision，系统会保留 intent 并拒绝 teardown，等待后续显式恢复策略。Recovery store 仍不提供跨进程锁；Product Lifecycle 必须继续作为同一 Owner/Run 分区的唯一串行 owner。

## 10. 下一阶段

P16 的治疗事务纵切已经具备从物理 hotbar 到双权威 durable recovery 的完整 P 阶段闭环，不应继续堆叠 wrapper。下一阶段应先审计 0.0.10 规划与现有产品纵切，选择 P17 的最小真实玩法目标；若要继续强化本链，应优先定义“歧义 intent 的可审计人工/策略化处置契约”或 condition authority 的独立持久 receipt，而不是允许恢复路径猜测性治疗。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-9-meridian-shock-recovery-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-9-meridian-shock-recovery-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P16.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-9-meridian-shock-recovery-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P16.9.r0_log.md>
