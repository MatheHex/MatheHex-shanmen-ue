# Dev.D.UE.0.0.10.P5.3.r0 Report

## 1. 结论

P5.3 在本轮边界内完成，结论为 **PASS**。

本轮关闭了 P5.2 明确留下的跨权威窗口：资源型防御现在先把全部 Durability／Charges 预约冻结为唯一 durable intent，再提交或恢复玩家 vitality，最后在一个 Items 事务中提交触发前缀并取消其余预约。进程若中断在 vitality 与 Items 终态之间，下一次 Run 绑定或下一次资源型 Impact 会从持久 intent 恢复；当前生命值既不是精确 before 也不是精确 after 时失败关闭。

P5.2 的产品级 `CommitTriggered` 直提交通道已删除，避免未来绕过恢复协议。底层 Items 批量提交能力保留用于仓库契约测试，但 demo_map 产品边界只暴露 P5.3 的 prepare／recover／finalize 路径。

## 2. Durable intent 状态机

新增两个 Items 操作：

- `PreparePreparedRunResourceIntent`：验证 ActiveRun、部署物品和全部资源预约，保存 `IntentId`、有序预约、触发前缀长度与外部 CAS 元数据；不扣耐久、不扣次数；
- `FinalizePreparedRunResourceIntent`：外部提交成功时提交触发前缀并取消其余行，外部明确拒绝时取消全部行。

Repository 保证：

1. 全局最多存在一个未终结外部 intent；
2. pending intent 的每个预约必须保持 Reserved 且绑定同一元数据；
3. Run 在 intent 未终结时不能 finalize；
4. finalize 只能引用精确 prepare request、IntentId 与 ActiveRun；
5. exact retry 返回原 receipt，不重复修改资源；
6. prepare／finalize 均通过单一 Candidate、单一 authority revision 与单次 durable write 发布。

## 3. Vitality 恢复契约

`FShanmenVitalityCommitCommand` 新增受验证的 durable rehydrate 入口。产品适配器把 GUID、预期 authority revision、五个 float 的精确 bit pattern 和 defense outcome 编码为 `SMV1` 元数据，不重新运行 resolver。

`RecoverPendingExternalCommit` 的判定只有三种：

- 当前值等于精确 before：把丢失的 revision 重基到当前 ledger，并提交一次；
- 当前值等于精确 after：导入 `AlreadyCommitted` receipt，不再次扣血；
- 其它状态：返回 stale／ambiguous，不改生命、不终结资源。

恢复仍要求 target、maximum vitality、命令守恒和 ledger 同步全部成立。已有 processed impact 时继续走原生 exactly-once replay，不使用重基路径。

## 4. 产品编排

`Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact` 固定执行：

1. 先恢复任何旧 pending intent；
2. 从 canonical request/result 构造稳定 prepare 与 vitality command；
3. durable prepare Items；
4. commit/recover vitality；
5. durable finalize Items。

`CombatRunCoordinator` 在发布新 Run 绑定前尝试恢复 pending intent；敌人到玩家的 Impact 仅在防御快照含 `bRequiresCommitOnTrigger` 时进入协调器，否则保留既有直接 vitality ledger 路径。因此当前未配置资源型防御的产品数值与行为不变。

本轮没有新增真实防御装备内容或预约创建入口；它完成的是可恢复产品提交面，而不是虚称现有装备已经消耗耐久或次数。

## 5. 自动化证据

新增四条核心自动化：

- `Shanmen.0_0_10.Items.PreparedRunDefenseResourceIntentRecovery`；
- `Shanmen.0_0_10.Items.AuthorityService.PreparedRunResourceIntentDurability`；
- `Shanmen.0_0_10.CombatRuntime.VitalityLedger.DurableIntentRecovery`；
- `Shanmen.0_0_10.Items.DefenseResourceAdapter.RecoverableIntent`。

最终 changed-file 回归：

| 组 | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10` | 121 | 0 | 1 | 0 | `3CBDF33BE5355A806E3A9C6263FF4439409C802B5E1A714C5F3BE5FB9EF704E6` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | 0 | `B4172ECA6ED56C3F208BA8919B748473E63458A98DE1A863CE68C0B095080C3D` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | 0 | `E3E58071C731AA86CC8127C178ECCB5A1051CEAAB6A807EAABA709F219B8003F` |

合计 `174 Success / 0 Fail`，三组进程原生退出码均为 `0`。

## 6. Changed-file 回归门禁

22 个 Source/Scripts 改动路径推导出 8 个要求组：Items、CombatCore、CombatRuntime、CombatRunCoordinator、PlayerVitality、0.0.10 全量、ItemUseAndArmor 与 Hotbar。最终结果：

```text
SELF_TEST: PASS 8/8
REGRESSION_COVERAGE: PASS Changed=22 Rules=6 Required=8 Logs=3
```

同时修正 Player vitality 映射：真实生产文件名是 `demo_mapPlayerHealthComponent`，旧规则误写为不存在的 `demo_mapPlayerVitalityComponent`。新增正向自测保证 PlayerHealth 改动必须具备 focused vitality 与 0.0.10 全量证据。

## 7. 首次失败与修复

第一轮 adapter focused 进程原生退出码为 `1`。保留日志：

- `Saved/Automation/Dev.D.UE.0.0.10.P5.3.r0_adapter_focused.log`；
- SHA-256：`10080F846A27D5CE4BAA1AD7A043E87DE8ED12DCAC728CC11785D45D4AB6DB70`。

失败点是新增测试夹具在 `demo_mapShanmenDefenseResourceAdapterTests.cpp:127` 构造了缺少 mandatory `LayerTags` 的防御层，`Request.IsValid()` 正确失败。补齐 `Defense.Shield` 与 `Defense.LethalIntercept` 标签后 focused 测试通过；产品代码未为测试放宽验证。

## 8. 构建与静态检查

Editor Development 使用单并发、NoUBA：

- 完整编译 `42/42`，Succeeded，原生退出码 `0`，`124.75s`；
- 移除旧直提交通道后的最终增量 `18/18`，Succeeded，退出码 `0`，`66.47s`。

Game Development 使用同一低并发策略：

- 完整编译 `61/61`，Succeeded，原生退出码 `0`，`184.04s`；
- 最终增量 `17/17`，Succeeded，退出码 `0`，`69.92s`；
- 产物 `Binaries/Win64/demo_map.exe`，未启动。

静态结果：

- `git diff --check`：退出码 `0`；
- ShanmenItems 对产品／Engine actor、伤害 API、运行时 RNG：`0` 匹配；
- ShanmenCombatRuntime 对 Items／demo_map、Actor/World、伤害 API、运行时 RNG：`0` 匹配；
- 新 adapter 对 Code A／Code B／ProfileRepository／ItemSubsystem、伤害 API与运行时 RNG：`0` 匹配；
- demo_map 中旧 `BuildCommitRequest`、`CommitTriggered`、direct durable commit：`0` 引用。

## 9. 修改范围与兼容性

- ShanmenItems：durable intent 类型、repository 状态证明、service forwarding 与故障注入测试；
- ShanmenCombatRuntime：vitality 命令重建与 exact before/after 恢复；
- demo_map：health 恢复入口、资源协调器、Run 绑定恢复与条件式产品交付；
- Scripts：修正 PlayerHealth 回归映射并把 self-test 增至 8 条。

既有枚举值未重排，持久 snapshot 未增加第二套文档或旁路文件。当前旧防御没有资源层，因此 legacy Impact 路径、生命数值与视觉行为不变。长期未跟踪的旧 Prompt、Report、自动化交接文档和用户文件均未纳入提交。

## 10. P/F 边界

本 Report 只包含 P 阶段源码、静态审查、无头 `-NullRHI` Automation、Editor Development 与 Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 11. 后续阶段

P5.4 应把第一种真实防御内容接入该唯一通道：在 ActiveRun 中创建对应 Durability／Charges 预约，捕获为 resource-backed defense layer，并补产品级“触发消耗／未触发取消／重启恢复”测试。不得重新引入直接资源提交或先扣资源后扣血的旁路。

## 12. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-3-defense-resource-coordination/Docs/Report/Dev.D.UE.0.0.10.P5.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-3-defense-resource-coordination/Docs/Log/Dev.D.UE.0.0.10.P5.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-3-defense-resource-coordination>
