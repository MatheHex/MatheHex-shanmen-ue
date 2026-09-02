# Dev.D.UE.0.0.10.P16.6.r0 Report

## 1. 结论

P16.6 已把 P16.5 的 Meridian Shock condition-domain durable proof store 接入现有 treatment Product Route / Session / Lifecycle，形成唯一顺序：

```text
prepare item durable
  -> treat condition
  -> record condition proof durable
  -> commit exact item durable
  -> forget condition proof durable
```

调用方不再能向恢复入口注入任意 proof 数组。Lifecycle 从已绑定的 ShanmenItems storage root、OwnerId 与 ActiveRunId 派生唯一 proof 分区；Route 直接加载、校验并协调该 store。ShanmenItems 仍是库存与 item ledger 唯一真值，condition component/store 仍是 condition 状态与恢复证明唯一真值。

最终结果：

```text
Treatment flow focused:      23 Success / 0 Fail
Mapped legacy regressions:  348 Success / 0 Fail
Shanmen.0_0_10 full:        736 Success / 0 Fail
Regression coverage:        PASS (Changed=7 / Rules=1 / Required=10 / Logs=7)
Regression gate self-test:  PASS 271/271
Editor + Game Development:  PASS / native status 0
```

## 2. 唯一 owner 与绑定边界

Product Lifecycle 现在要求以下 identity 同时匹配才允许开始：

- item authority 的 bound OwnerId 等于 Run correlation OwnerId；
- condition component 的 RunId 等于 ActiveRunId；
- proof storage 的 OwnerId/RunId 等于同一 correlation；
- proof root 来自 item authority 已绑定 storage root，不接受调用方自由指定路径。

Route/Session 的重复 begin 仅在完整 correlation、condition component 与 proof root/Owner/Run 全部相同的情况下幂等成功。活动会话不能切换到另一个 owner、Run 或 root。该约束阻止同一 TreatmentId 跨账号、跨局或跨磁盘分区恢复。

本轮没有把 proof 放入 Profile，也没有复制 item quantity、inventory projection 或 processed request。恢复时 item prepare/commit 只由 ShanmenItems ledger 判定；proof 只证明指定 TreatmentId 的 condition mutation 已完成。

## 3. 正常执行顺序与日志状态

Route journal 新增 canonical `RecoveryProof` 以及 `bProofRecorded`、`bProofCleanupPending`。处理顺序固定为：

1. ShanmenItems 创建 durable prepare；
2. condition component 执行 exact treatment；
3. 从成功 receipt 捕获 canonical proof；
4. `RecordProof` 成功后才允许提交 item；
5. ShanmenItems 对同一个 prepare 执行 commit；
6. `ForgetProof` 成功后，本命令才算完全 resolved。

proof 捕获或写入失败时，item commit 不会发生，journal 保持 recovery-required。item commit 已成功但 proof cleanup 失败时，journal 仍保持 unresolved；重试只清理 proof，不会重复扣除物品。已 treatment 的命令绝不回退到 cancel 路径。

Route 的测试中断点由旧的“condition treatment 后”改为“proof durable publish 后、item commit 前”，使进程丢失测试与真实可恢复边界一致。

## 4. 重启恢复与失败关闭

恢复入口现在只接受 item authority，不再接受外部 `TArray<RecoveryProof>`。每次恢复先加载唯一 proof store；损坏、future schema、读失败或恢复失败均直接失败关闭。

恢复会交叉核对：

- proof 的 Run、target、timeline 与 Meridian Shock definition；
- proof TreatmentId 是否存在唯一 exact item prepare；
- proof ItemInstanceId 是否与 prepare 一致；
- pending item 是否仍为 proof 指定的 definition；
- terminal item receipt 是否唯一、已 committed，并与 TreatmentId、item、amount、purpose、Run 和 prepare request 一致。

若 item commit 已经 durable、只剩旧 proof，恢复仅验证 terminal receipt 并删除 proof；不会再次 treatment 或 commit。若存在 pending prepare，恢复会建立/恢复 condition receipt，重新得到 canonical proof，确保 proof durable 后提交 item，再删除 proof。orphan proof、重复 prepare、多个 terminal receipt、identity 冲突或 cancelled terminal evidence 都不会被猜测性修复。

## 5. 新增测试与覆盖扩展

新增 2 个 focused tests：

- `Lifecycle.ProofPersistenceBeforeItemCommit`：注入 proof publish 失败，证明 item 未提交；解除故障后重试只提交一次并清空 proof；
- `Lifecycle.CommittedProofCleanupRestart`：在 proof publish 后中断，再注入 cleanup 失败，证明 item 已且仅提交一次；重建 Lifecycle 后只验证 receipt 并清理 proof。

现有 P16.4 process recovery 与 conflict tests 已改为把 proof 写入真实 P16.5 store，再通过无参数恢复入口加载；不再把 proof 作为可信调用参数。Session binding test 补充 wrong-owner root 与 active-root switch 拒绝断言。

最终 focused 23/0；全套 `Shanmen.0_0_10` 从 P16.5 的 734 增至 736，最终 736/0。Full suite 首末 Success 时间为 `2026.09.02 11:59:11.003 -> 12:26:49.452 UTC`，约 27m38.45s。日志中的一次约 103 秒 scheduler delta 被 Automation Controller 明确忽略，对应测试最终通过，不构成失败。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Treatment flow focused | 23 | 0 | `25D508B18B877ED6AFE356A8B5C7CE7018BBBAB5610DB7CC5DBB54C878A2FE13` |
| `Shanmen.0_0_10` full | 736 | 0 | `CCF976E083548D3318854D0C1775C4734BEBFB927C77F7AE9864F3C36D0E6BAE` |
| CodeB | 60 | 0 | `C8AFED53DE4373C90A4E2BF3C16D556617828CA8E08580E80C41777841715C51` |
| ItemEconomySchema | 24 | 0 | `728729210AA195A5C61BE39534A4734A3C1D25801EAB51805690F0784C166DE8` |
| ItemUseAndArmor | 46 | 0 | `F1DB2C81BC77E72C0F9243402052C136513C2B72918727FF71E3CC454F7C9154` |
| P4 Hotbar | 7 | 0 | `2555AEDB05F3D58479B5A15A5A54978E351F45BADE383F81D99E8BF1DA3C2B02` |
| Profile | 211 | 0 | `3F0B0D932F996EC87B942B255B7CCCD9169F86808F00C30C2EE24E047C981623` |

mapped legacy 合计 348/0。7 份最终 Automation 日志的 Fail、Fatal、Unhandled、Ensure 与网络错误计数均为 0，并包含 UE 5.8 native terminal-success 证据。

## 7. 改动—回归门禁与静态边界

7 个修改过的 Product Route / Session / Lifecycle / tests 路径全部命中现有 `MeridianShockTreatmentProductFlow` 规则。10 个必跑组由 7 份健康日志完整覆盖：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 271/271
JSON_PARSE: PASS
OLD_PROOF_API_SCAN: PASS HITS=0
BOUNDARY_SCAN: PASS HITS=0
GIT_DIFF_CHECK: PASS
```

coverage gate output SHA 为 `5C68B63013D5EE1326F26819567C641FF9EF95F5AF23E10BF2A7820F0A530D05`；self-test output SHA 为 `A34FB07154B570E39D7736BF27B33745DA4F6517015445ADCBA8ED7CA4004E6A`；regression map SHA 为 `62E7F055919C0F1514AAFBCF83227BE57968F3087CB1687F9C8AA76F6FA786C7`。

修改过的生产 flow 文件静态扫描确认不依赖旧 ItemSubsystem/Profile、World/Actor、UI、Enhanced Input、Timer 或 RNG；旧 proof-array recovery overload、旧 interruption enum 与 setter 的扫描结果为 0。

## 8. 构建证据

最终源码使用 `-WaitMutex -NoUBA -MaxParallelActions=1` 逐目标构建。两个目标都重新编译本轮修改文件并成功链接，原生退出码均为 0。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 29 / 121.97s | `29FA9E95745C93B69568E712B73F677719FEA02B493AA052E0FF79C910DC7492` |
| Editor Development | Succeeded | 30 / 114.55s | `91435EB5ADAD60622AA780CE5621A88C83E954F07A15DCACF32298E1B10A78C3` |

最终产物：

- `demo_map.exe`：356,012,032 bytes，SHA-256 `219B4C4C6F7135AC5D8235472DDF92603E966A2850D567DB0D76DE5A533ED740`；
- `UnrealEditor-demo_map.dll`：14,660,096 bytes，SHA-256 `C43DC8DFF1B6AF5CC817A4E6470886C1104C69AA7A932B5873BE69AD0EE92841`。

## 9. 修改范围

本轮修改 7 个 Product Route / Session / Lifecycle / tests 源文件，并新增本 Report 与 Development Log，计划提交 9 个文件。没有修改 P16.5 store 实现、Profile、ShanmenItems、GameMode、PlayerController、地图、资源、存档 schema、Windows、UE Engine 或用户配置。

raw logs 仅保存在本机 `Saved/Logs`。长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 10. P/F 边界与后续

本 Report 证明 P 阶段 Product flow 集成、真实磁盘 proof store、故障注入、进程边界恢复 Automation、NullRHI full/legacy regressions、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

当前 store 不提供跨进程锁；Product Lifecycle 必须继续作为同一 Owner/Run proof 分区的唯一串行写入者。另有一个不能夸大为“完全原子”的边界：condition component 的内存 mutation 与 proof store 的 durable publish 属于两个 authority，二者不是同一数据库事务，因此两步之间仍存在极小进程崩溃窗口。P16.6 保证 proof publish 失败时不扣 item，并保证 proof 已 durable 后的重启可确定性完成或安全失败；若要覆盖该极小窗口，后续需要 condition authority 自身的 write-ahead intent/receipt 持久化，而不是把库存真值复制进 proof store。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-6-meridian-shock-proof-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-6-meridian-shock-proof-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P16.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-6-meridian-shock-proof-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P16.6.r0_log.md>
