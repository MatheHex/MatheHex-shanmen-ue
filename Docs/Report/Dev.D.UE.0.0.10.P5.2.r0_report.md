# Dev.D.UE.0.0.10.P5.2.r0 Report

## 1. 结论

P5.2 在本轮边界内完成，结论为 **PASS**。

本轮为 P0.1 已冻结的资源型防御 receipt 补齐了 ActiveRun 物品权威提交点：同一 Impact 触发的 Durability／Charges 预约必须以精确 LayerId、SourceInstanceId 和 ActiveRun 身份组成一个有序批次，在一个 repository candidate、一个 authority revision 和一次 durable write 中全部提交或全部拒绝。

本轮没有虚称端到端战斗接入。现有产品防御尚未创建这类预约，也没有产品调用点调用新适配器；跨生命权威与物品权威的恢复编排、未触发预约取消和真实防御装备内容属于后续阶段。

## 2. 事务契约

新增 `CommitPreparedRunResources` 操作及两个值类型：

- `FShanmenItemRunResourceCommitLine`：精确携带 ReservationId 与 ItemInstanceId；
- `FShanmenItemRunResourceCommitRequest`：携带 owner/scope、ActiveRunId、有序触发行和 PurposeId，并拒绝重复 reservation。

Repository 只接受：

1. 当前未终结 ActiveRun 的 claim；
2. claim 部署集合中的精确物品实例；
3. claim 之后创建、仍为 Reserved 的 Durability 或 Charges 预约；
4. owner、scope、reservation、source item 全部一致的请求。

所有行先在原始状态上完成验证，再在单一 `Candidate` 上调用 `ApplyCommit`。任一行失败时不会发布 Candidate，因此不存在部分磨损或部分扣次数。

## 3. 重放、持久化与状态验证

请求 fingerprint 包含 context、ActiveRun、Purpose 和全部有序行。Exact retry 在 Run 终结检查前处理，因此一次已成功提交的请求在重启或终结后仍返回原 receipt，不会二次消耗。

成功 receipt 保存 ActiveRunId 和有序 ReservationIds。`ValidateState` 会重新证明：

- commit revision 位于 claim 与 finalize 之间；
- 每条资源预约属于 claim 部署物品；
- reserve revision 晚于 claim、早于批量 commit；
- 每条预约最终为 Committed，且只出现一次。

AuthorityService 与 GameInstance item subsystem 均新增同名 durable API；写盘失败保持内存快照、authority revision 与 save generation 不变，重试后仅产生一次成功状态变化。

## 4. CombatCore 产品边界

新增 `Fdemo_mapShanmenDefenseResourceAdapter`：

- 只消费 accepted、conserved 的 `FShanmenImpactResult`；
- 只映射 `bRequiresCommit` 的 triggered layer；
- LayerId 映射到 ReservationId；
- SourceInstanceId 必须是当前 Run correlation 中的精确部署装备；
- owner、scope、ActiveRun、ImpactId、触发行数及有序身份共同派生稳定 RequestId；
- 无资源触发时返回 `NoCommitRequired`，不制造空命令；
- authority、correlation、来源或请求任一不合法均 fail-closed。

适配器不读取 Code A／Code B，不修改 Runtime item，不调用伤害 API。当前只有声明、实现和 contract test 三处符号，没有产品调用点；这是刻意保留的边界，而不是遗漏的成功声明。

## 5. 自动化证据

新增或扩展三条核心测试：

- `Items.PreparedRunDefenseResourceCommit`：Durability + Charges 双行原子提交、单 revision、重启重放、Extraction 保留磨损、终结后拒绝新提交、来源不匹配时全批回滚；
- `Items.AuthorityService.PreparedRunResourceCommitDurability`：写盘故障回滚、重试持久化、单 generation、重启 exact replay；
- `Items.DefenseResourceAdapter.CanonicalRequest`：确定性请求、无提交层不造命令、未部署来源与重复 reservation 失败关闭。

最终 changed-file 回归：

| 组 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Items` | 63 | 0 | `630E5C67C73DDFDC2E76A6A1315850EEB07E976A7857C06FDA32AF879DCA0CFA` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `0E429A99B2E0CF3808C9EB3DFC0956514A9F72AA1E536E314E62D06F6CF74957` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `D61E55AE808507E26693E9CD750D1A1C3E9E58950F24A408EE7EE5BE76607F48` |
| `demo_map.P4.Hotbar` | 7 | 0 | `4C64B2386BE94D6EE6910AD4CFB745D17810FA179AE29D1D12E72C0C657F915A` |

合计 `125 Success / 0 Fail`；四份日志均有精确 RunTests 组、queue empty，fatal／unhandled／ensure 为 `0`，进程原生退出码均为 `0`。

## 6. Changed-file 回归门禁

回归映射新增 `DefenseResourceAdapter` 规则，并要求同时验证 Items 与 CombatCore。最终结果：

```text
SELF_TEST: PASS 7/7
REGRESSION_COVERAGE: PASS Changed=14 Rules=3 Required=4 Logs=4
```

14 个 Source/Scripts 改动路径全部有映射；四个要求组全部由独立健康日志覆盖。

## 7. 首次失败与恢复

1. 前两次 Items 启动均原生退出码 `1`。控制台同时打印非 Win64 SDK 探测噪声；第二份完整日志证明实际阻塞是 test-only `TArray` 自引用 `Add` assertion，位置为 `demo_mapShanmenDefenseResourceAdapterTests.cpp:113`。改为先复制元素再添加后，Editor 增量构建成功。UE 自动保留的两份失败日志 SHA 分别为 `1BED9BE1196CB3B0371FA27EA484C737E023AB621FE2454B25BE45469A3A2927` 与 `1A3E58AD7C1308233004CD73CD16C55FCE18281D2DB00EB9A05F903017916619`。
2. 修正后首轮四组测试均为 `125 Success / 0 Fail`、进程退出码 `0`，但命令把 `;Quit` 放入 RunTests 参数，日志组名成为 `…;Quit` 且缺少门禁要求的正式 queue-empty 终止证据。门禁拒绝这四份日志；它们未被计作最终证据。
3. 移除显式 `Quit`，仅由 `-TestExit="Automation Test Queue Empty"` 收口后，重新生成上述四份最终日志并通过门禁。

没有把 SDK 探测警告写成源码失败，也没有把测试数量成功但证据格式不合格的日志写成最终通过。

## 8. 构建与静态检查

Editor Development：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次完整：`32/32`，Succeeded，原生退出码 `0`，`116.50s`；
- test-only 修正后：`4/4`，Succeeded，原生退出码 `0`，`5.26s`。

Game Development：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `29/29`，Succeeded，原生退出码 `0`，`102.54s`；
- 产物：`Binaries/Win64/demo_map.exe`，未启动。

静态结果：

- `git diff --cached --check`：退出码 `0`；
- `ShanmenItems` 对 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage`、`UGameplayStatics`、`FMath::Rand`、`FRandomStream`：`0` 匹配；
- 新产品适配器对 Code A、Code B、存档写入口、伤害 API 与运行时 RNG：`0` 匹配；
- 代码／脚本范围：`14 files, 1105 insertions, 5 deletions`。

## 9. 修改范围与兼容性

- `ShanmenItemTypes`：追加 transaction operation 与批量请求类型；既有枚举值未重排；
- `ShanmenItemRepository`：ActiveRun 资源批量提交与持久状态证明；
- `ShanmenItemAuthorityService`／`demo_mapShanmenItemAuthoritySubsystem`：durable forwarding；
- `demo_mapShanmenDefenseResourceAdapter`：CombatCore receipt 到 Items command 的单向边界；
- Items／AuthorityService／Adapter tests；
- `Scripts/ShanmenRegressionMap.json`：changed-file 回归映射。

没有改变旧存档字段、Code A／Code B 写权威、现有防御数值、装备内容或当前产品战斗调用点。工作区长期未跟踪的旧 Prompt、Report、自动化文档及用户文件未纳入提交。

## 10. P/F 边界

本 Report 只包含 P 阶段源码、静态审查、无头 `-NullRHI` Automation、Editor Development 与 Game Development build。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 11. 后续阶段

P5.3 应建立资源预约／取消与生命提交之间的可恢复协调：在 resolve 前锁定资源，未触发则取消，触发后即使进程在生命提交与物品提交之间中断也能由 durable intent 恢复。完成该跨权威恢复链之前，不把新适配器接入现有 M01 产品伤害入口。

## 12. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-2-defense-resource-commit/Docs/Report/Dev.D.UE.0.0.10.P5.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-2-defense-resource-commit/Docs/Log/Dev.D.UE.0.0.10.P5.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-2-defense-resource-commit>
