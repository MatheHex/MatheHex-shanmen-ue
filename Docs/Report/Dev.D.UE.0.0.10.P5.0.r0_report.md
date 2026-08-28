# Dev.D.UE.0.0.10.P5.0.r0 Report

## 1. 结论

P5.0 完成，结论为 **PASS**。

本轮把准备阶段带入局内的快捷栏消耗品接入 `ShanmenItems` 持久权威：治疗药使用先写入可重放的 durable consumption receipt，再投影到局内 Runtime。运行时提交失败不会回滚已持久化的消耗；相同未变化意图可重试并重放同一权威 receipt，不会产生第二次持久扣减。重启后 Runtime 从准备快照减去全部 durable consumption 重建剩余数量。

最终 Editor Development、五组改动映射 Automation、回归覆盖门禁、Game Development 与静态检查全部通过。未发现新的模块边界或旧物品双写路径。

## 2. 持久消费契约

`ShanmenItems` 新增：

- `ConsumePreparedRunItem` 事务操作；
- `RunItemQuantityConflict` 明确 CAS 冲突；
- `FShanmenItemRunConsumeRequest`；
- Repository、AuthorityService 与产品 AuthoritySubsystem 的 durable command 路由。

每个消费请求包含 scope、owner、ActiveRunId、ItemInstanceId、数量、预期消费前数量、purpose 与 content stamp。Repository 只接受：

1. 已开始且未终结的精确 ActiveRun；
2. 属于该 Run 与 owner 的 committed Quantity reservation；
3. 由全部既有 consumption receipt 重建出的当前数量；
4. 与 `ExpectedQuantityBefore` 完全一致的 CAS 输入；
5. 未与既有 RequestId 的 fingerprint 冲突的命令。

成功 receipt 记录 `ResourceBefore`、`Amount`、`ResourceAfter`、authority revision 与稳定 purpose。相同 RequestId 和相同 payload 精确重放；相同 RequestId 携带不同 payload 在 mutation 前拒绝。

## 3. 不变量与终局对账

Authority snapshot 验证新增 consumption chain 审计：

- receipt 必须位于 start/claim 之后、finalize 之前；
- 每个消费 identity 必须对应且只对应一个准备 Quantity reservation；
- 按 authority revision 排序后，每一条 `ResourceBefore` 必须等于上一条 `ResourceAfter`；
- `ResourceAfter = ResourceBefore - Amount`；
- 消费总量不能超过冻结 reservation 数量；
- finalization 恢复数量不能超过 `reservation amount - durable consumed`。

因此 extraction 不能把已经持久消费的单位恢复到局外；death/abandon 继续按既有 destructive policy 关闭整条 Run。

## 4. 产品快捷栏路由

`Udemo_mapItemSubsystem` 抽出无写入的 `PreviewHotbarSlotUse`，原有 `UseHotbarSlot` 复用同一 preflight 后才执行局内 item、health 与 cooldown 原子事务。

准备原件在 Runtime 中保留局外 identity，因而不伪造 `OriginRunId`；当前 Run ownership 由 active Run 与 `DeployedItemIds` 的 canonical materialization evidence 联合确认。局内新掉落仍使用 `OriginRunId`，两种来源不会混用。

`Fdemo_mapShanmenRunLifecycleAdapter::UsePreparedRunHotbarSlot` 的顺序为：

1. 检查 authority/Runtime/ActiveRun correlation；
2. 只读预检快捷栏、ownership、定义、stack、health 与 cooldown；
3. 用 owner、scope、ActiveRun、item、slot、before-stack 与 purpose 派生确定性 RequestId；
4. 持久提交一单位消费；
5. 校验 receipt 与 Runtime CAS 预览一致；
6. 执行局内 item/heal/cooldown 投影。

`ProfilePreparationFlow` 与 `V3ProgressionManager` 新增通用 quick-slot 路由。存在 Shanmen lifecycle 时严格使用新路径并 fail-closed；只有没有 Shanmen lifecycle 的旧流程才进入 Code B compatibility。

## 5. 失败、重试与恢复

自动化注入 `AfterItemMutation` Runtime 失败后验证：

- durable receipt 保留；
- Runtime stack 与 health 完整回滚；
- 原意图再次执行时 authority 返回 `Replayed`；
- Runtime effect 只成功一次；
- snapshot 中只有一条成功 consumption receipt。

重启并重新绑定 authority 后，ActiveRun rematerialization 从准备数量减去 consumption history，药品由 `3` 恢复为 `2`，快捷栏仍绑定原 identity。数量耗尽时重建计划移除对应 Runtime item 与快捷栏 binding。

## 6. 兼容性与历史测试债修正

改动文件映射回归暴露并修正了既有测试对当前契约的陈旧假设；这些修正没有放宽生产不变量：

- Profile/schema 测试改为读取当前 schema，不再硬编码旧版本 `3/4`；
- Wind Talisman 使用现行 SpatialRing slot；装备 fixture 覆盖 Weapon、Armor、Accessory、SpatialRing、Backpack 五槽；
- Backpack 现行容量按 `36` 验证；
- structural limit 使用 `MaxRunInventoryItems + 1`，不再把第四个 material 当作越界；
- source scans 跳过测试文件并匹配真实 mutation 调用，避免类型名文本产生假阳性；
- Code B loaded spatial graph move 按现行 whole-graph identity 契约验证成功、revision 与 child placement；
- SessionSubsystem allowlist 补入已审计的正式 cutover/framework 引用。

回归映射同时补入 `demo_mapShanmenPreparationAdapterTests.cpp`，防止产品准备适配器测试再次成为未映射路径。

## 7. 自动化与证据

最终五份独立日志均为 queue empty、`0 Fail`、进程退出码 `0`：

- `Shanmen.0_0_10`：`114/114 Success`；SHA-256 `52676BCAB8E76A1877C5791F9A67A1EC7BB5A7867E0407E6D46A56440B85088B`；
- `demo_map.Profile`：`211/211 Success`；SHA-256 `FF72C99EF55D246C610B3BE1DD8C2B35C14291694F5F4FA82F2FE2E2492B7ABB`；
- `demo_map.ItemEconomySchema`：`23/23 Success`；SHA-256 `7B9D76C9974F78D7F73F386B39E33FC93906E5E3DAC1AC7C27A30B1665FAA427`；
- `demo_map.CodeB`：`60/60 Success`；SHA-256 `54FA96E2A1A9337B25E07F3B78EFBFA639DB4462F83ABA997AC0CC63816B5158`；
- `demo_map.V2RangedCompatibility`：`22/22 Success`；SHA-256 `35AC19A15846877F197BCDF2AB794BD12AE1069D4F59BC91D8441B1AEAB5461D`。

回归门禁：

- self-test：`7/7 PASS`；
- 首次真实检查正确拒绝未映射的 preparation adapter test；补映射后通过；
- 最终：`REGRESSION_COVERAGE: PASS Changed=30 Rules=6 Required=6 Logs=5`；
- required groups：`Shanmen.0_0_10`、`Shanmen.0_0_10.Items`、`demo_map.Profile`、`demo_map.ItemEconomySchema`、`demo_map.CodeB`、`demo_map.V2RangedCompatibility`。

## 8. 首次失败记录

本轮保留并如实区分以下失败：

1. 最早一次 Automation 命令漏写 `-NoCompile`，进程退出码 `1`，只到 SDK/build validation，未生成有效测试日志；属于调用参数错误，不是源码失败。
2. 首次 Items 日志：`58 Success / 1 Fail`。准备原件因 `OriginRunId` 为空被旧 Runtime ownership 规则拒绝；修正为使用 canonical `DeployedItemIds` evidence。
3. 首次完整映射运行：Profile `198 Success / 13 Fail`，Code B `57 Success / 3 Fail`；均为测试对 schema、槽位、容量或 whole-graph move 的旧断言。
4. ItemEconomySchema 首次运行：`19 Success / 4 Fail`；同源旧槽位/容量断言，修正后 `23/23`。
5. 首次真实回归门禁因 preparation adapter test 无映射而失败；补充显式映射后通过。

UE Automation 进程在测试断言失败时仍可能返回 `0`，因此本轮不以进程码代替测试结果；最终证据由门禁同时检查 Success/Fail、queue completion、fatal/exception/ensure 与 SHA。

失败日志保留于 `Saved/Logs`，包括 `items_initial`、`profile_final`、`codeb_final` 与 `itemeconomy_final`，未改名伪装为成功日志。

## 9. 构建与静态检查

Editor Development：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 最终源码完整构建：`57/57` actions，Succeeded，原生退出码 `0`，`179.49s`；
- 修正 test-only ItemEconomy 断言后的必要增量构建：`4/4` actions，Succeeded，原生退出码 `0`，`6.39s`。

Game Development：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `86/86` actions，Succeeded，原生退出码 `0`，`259.60s`；
- 产物：`Binaries/Win64/demo_map.exe`，未启动。

静态检查：

- `git diff --check`：退出码 `0`；
- `ShanmenItems` 中 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage` 与运行时 RNG：`0` 匹配；
- 回归覆盖门禁：退出码 `0`；
- 最终改动统计：`30` 个 Source/Scripts 文件，约 `1099 insertions / 62 deletions`（Report/Log 另计）。

## 10. 修改范围

生产改动集中在：

- `Source/ShanmenItems`：事务类型、repository、authority service、测试；
- `Source/demo_map/demo_mapItemSubsystem.*`：只读预检与现有局内提交复用；
- `Source/demo_map/demo_mapShanmenRunLifecycleAdapter.*`：durable-first 使用与恢复投影；
- `Source/demo_map/demo_mapProfilePreparationFlow.*`、`demo_mapV3ProgressionManager.*`、`demo_mapPlayerController.cpp`：产品快捷栏路由；
- `Source/demo_map/demo_mapShanmenItemAuthoritySubsystem.*`：产品持久命令桥；
- Profile、Code B、ItemEconomy 与 preparation adapter 测试；
- `Scripts/ShanmenRegressionMap.json`。

未纳入工作区长期未跟踪的旧 Prompt、Report、自动化交接文档或其它用户文件。

## 11. P/F 边界

本 Report 只包含 P 阶段开发、代码审查、静态检查、无头 `-NullRHI` Automation、Editor Development 与 Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。真实产品体验、输入和系统回归继续留给 F 阶段。

## 12. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-0-authority-quick-use/Docs/Report/Dev.D.UE.0.0.10.P5.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-0-authority-quick-use/Docs/Log/Dev.D.UE.0.0.10.P5.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-0-authority-quick-use>
