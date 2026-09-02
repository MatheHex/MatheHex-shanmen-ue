# Dev.D.UE.0.0.10.P16.6.r0 Development Log

## 1. 目标

把 P16.5 的唯一 condition-domain durable proof store 接入 P16.4 Meridian Shock treatment Product Route / Session / Lifecycle；删除任意外部 proof 注入入口，固定 `item prepare -> condition treatment -> proof publish -> item commit -> proof cleanup` 的恢复顺序，同时保持 ShanmenItems 为库存唯一真值、condition component/store 为 condition 与 proof 唯一真值。

## 2. 实现

- Lifecycle 从 item authority bound storage root、OwnerId 与 ActiveRunId 派生 proof storage context，并拒绝 owner/Run/condition 不一致；
- Route/Session begin 绑定 exact root/Owner/Run，活动绑定不能切换 proof store；
- 删除 Route、Session、Lifecycle 接受 `TConstArrayView<RecoveryProof>` 的 recovery overload；
- Route journal 增加 canonical proof、proof-recorded 与 cleanup-pending 状态，只有 item commit 且 proof cleanup 完成后才 resolved；
- condition treatment 成功后捕获 canonical proof，proof 未 durable 前禁止 item commit；
- item commit 成功后执行 proof cleanup；cleanup 失败保留 recovery-required，重试不重复扣物品；
- durable recovery 直接加载 store，并把 proof 与 exact item prepare、Run、target、timeline、definition、item identity 和 terminal receipt 交叉核验；
- committed stale proof 只验证并清理，pending proof 恢复 condition receipt 后完成一次 item commit；orphan、重复或冲突 evidence 失败关闭；
- 测试中断点调整为 durable proof publish 后、item commit 前；增加 proof store failure injection 的 Route/Session/Lifecycle 转发。

## 3. 新增与迁移测试

新增：

```text
Lifecycle.ProofPersistenceBeforeItemCommit
Lifecycle.CommittedProofCleanupRestart
```

关键断言：

- proof 写失败时 condition 已处理，但 item commit/cancel 均为 0，解除故障后仅 commit 一次；
- proof durable 后中断可跨 Lifecycle 重建恢复；
- item 已 commit 而 cleanup 失败时，重启只验证 terminal receipt 并删除 proof；
- 清理重试不会二次 treatment、commit 或 cancel；
- wrong-owner proof context 与 active-root switch 均被拒绝。

P16.4 process recovery / proof conflict tests 改为先写入真实 store，再走无参数 recovery API，证明调用者不能自报可信 proof。

## 4. 最终验证

| Group | Result | SHA-256 |
|---|---|---|
| Treatment flow focused | 23/0 | `25D508B18B877ED6AFE356A8B5C7CE7018BBBAB5610DB7CC5DBB54C878A2FE13` |
| Shanmen.0_0_10 full | 736/0 | `CCF976E083548D3318854D0C1775C4734BEBFB927C77F7AE9864F3C36D0E6BAE` |
| CodeB | 60/0 | `C8AFED53DE4373C90A4E2BF3C16D556617828CA8E08580E80C41777841715C51` |
| ItemEconomySchema | 24/0 | `728729210AA195A5C61BE39534A4734A3C1D25801EAB51805690F0784C166DE8` |
| ItemUseAndArmor | 46/0 | `F1DB2C81BC77E72C0F9243402052C136513C2B72918727FF71E3CC454F7C9154` |
| P4.Hotbar | 7/0 | `2555AEDB05F3D58479B5A15A5A54978E351F45BADE383F81D99E8BF1DA3C2B02` |
| Profile | 211/0 | `3F0B0D932F996EC87B942B255B7CCCD9169F86808F00C30C2EE24E047C981623` |

mapped legacy 合计 348/0；最终 Automation 日志全部 Fail/Fatal/Unhandled/Ensure/network=0，并包含 native terminal-success。

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 271/271
JSON_PARSE: PASS
OLD_PROOF_API_SCAN: PASS HITS=0
BOUNDARY_SCAN: PASS HITS=0
GIT_DIFF_CHECK: PASS
```

gate output SHA `5C68B63013D5EE1326F26819567C641FF9EF95F5AF23E10BF2A7820F0A530D05`；self-test output SHA `A34FB07154B570E39D7736BF27B33745DA4F6517015445ADCBA8ED7CA4004E6A`；mapping SHA `62E7F055919C0F1514AAFBCF83227BE57968F3087CB1687F9C8AA76F6FA786C7`。

## 5. 构建

- Game Development：Succeeded / 29 actions / 121.97s / native status 0 / log SHA `29FA9E95745C93B69568E712B73F677719FEA02B493AA052E0FF79C910DC7492`；
- Editor Development：Succeeded / 30 actions / 114.55s / native status 0 / log SHA `91435EB5ADAD60622AA780CE5621A88C83E954F07A15DCACF32298E1B10A78C3`；
- Game EXE：356,012,032 bytes / SHA `219B4C4C6F7135AC5D8235472DDF92603E966A2850D567DB0D76DE5A533ED740`；
- Editor DLL：14,660,096 bytes / SHA `C43DC8DFF1B6AF5CC817A4E6470886C1104C69AA7A932B5873BE69AD0EE92841`。

## 6. 边界

修改过的生产 flow 不依赖旧 ItemSubsystem/Profile、World/Actor、UI、输入、Timer 或 RNG；旧外部 proof-array API 已移除。store 没有跨进程锁，Product Lifecycle 必须保持唯一串行 owner。

condition 内存 mutation 与 proof durable publish 不是同一数据库事务，二者之间仍有极小 crash window；本轮不虚称完整原子性。当前已证明 proof publish 失败不扣 item，以及 proof durable 后所有中断均可确定性完成或安全失败。覆盖前述窗口需要后续为 condition authority 增加自身 durable write-ahead 语义。

仅执行 P 阶段 C++、真实磁盘 Automation、NullRHI full/legacy regressions、静态检查与 Development builds；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户资料未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-6-meridian-shock-proof-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-6-meridian-shock-proof-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P16.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-6-meridian-shock-proof-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P16.6.r0_log.md>
