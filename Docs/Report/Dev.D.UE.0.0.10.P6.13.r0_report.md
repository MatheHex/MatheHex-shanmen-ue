# Dev.D.UE.0.0.10.P6.13.r0 Report

## 1. 结论

P6.13 已完成 Orbit 近身威胁策略的产品侧 World 证据捕获，结论为 **PASS**。

P6.12 不再要求产品调用方手工构造 `TargetEntityId + TargetTags`。完成的 P6.11 geometry receipt 现在可以与本轮实际采样到的 Actor 列表一次性闭合：World Adapter 通过既有 Run EntityRegistry 核对身份，并只从 Directed 路径已经使用的 `Idemo_mapCombatVitalityHost` 契约推导 `TargetLiving`，然后交给 P6.12 同一冻结策略评估。

本阶段没有增加 `EntityId -> UObject` 反向 Registry、持久 Actor 缓存、第二份候选缓存或另一套敌我规则；也不产生威慑效果、伤害、防御层、冷却或资源事务。

## 2. 功能性

- `CaptureOrbitThreatTargetEvidence` 接收完成的 canonical emission receipt 与调用方本轮采样到的 Actor 列表；
- Actor 输入顺序不影响输出，证据严格按 emission candidate 的规范顺序生成；
- 每个候选必须恰好对应一个已注册 Actor；缺失、重复、空指针、未注册 Actor 或 receipt 外实体全部失败关闭；
- receipt Run 与当前 Coordinator Run 不一致时明确拒绝；
- Actor 实现 `Idemo_mapCombatVitalityHost` 时，其绑定 EntityId 必须与候选完全一致，否则以 `TargetIdentityMismatch` 拒绝；
- 匹配的既有 vitality host 只提供 `TargetLiving`。没有该接口的已注册对象得到空标签，由 P6.12 的 required-tag policy 明确拒绝，而不是在 World 层偷偷丢弃 geometry；
- 捕获结果只保存值类型 evidence，不保存 Actor、Component 或弱指针；
- Controller 新增 `TryEvaluateOrbitThreatActors`，完成 capture 后复用原有 P6.12 policy API；
- Run Host 继续按 exact `ItemInstanceId` 路由，未知 item 不可消费另一把飞剑的 receipt；
- 成功和所有失败路径均不修改 vitality、impact ledger、detector ordinal、command sequence、item 或 inventory。

## 3. 完整性

新增 1 个 World Adapter 自动化场景，并扩展 2 个既有产品场景：

1. `ControlledWeaponWorldDelivery.ThreatEvidenceCapture`：未完成 receipt、空 Actor、未注册 Actor、receipt 外 Actor、缺失、重复、成功 Living 捕获及零副作用；
2. `ControlledWeaponController.OrbitThreatProjection`：缺失、重复、用 Player Actor 替换 Enemy 均失败，真实 Enemy Actor 自动形成 Living evidence 并得到 accepted policy receipt；
3. `ControlledWeaponRunHost.OrbitThreatRouting`：exact item 完成 World evidence + policy，未知 item 无法借用。

全量 `Shanmen.0_0_10` 由 `160` 增至 `161` 个测试并全部 Success。Focused Product ControlledWeapon 由 `29` 增至 `30` 个测试并全部 Success。

## 4. 兼容性

- 复用 `FShanmenWorldEntityRegistry` 的既有 object-to-entity 查询，未修改 Registry 或添加有歧义的 entity-to-object 反向索引；
- 复用 `Idemo_mapCombatVitalityHost` 与 `FShanmenCombatNativeTags::TargetLiving()`，与 Directed World delivery 的证据来源一致；
- 保留 P6.11 geometry receipt 与 P6.12 target policy 为唯一候选和策略权威；
- 旧显式 evidence API 保持可用，新 API 是产品 World 适配入口；
- 未修改 Impact、Defense、Vitality commit、Item authority、Action lifecycle、GameplayTags、Profile schema、存档、input、资产或 Build.cs；
- 没有冻结 faction、hostility、采样 cadence、半径、频率、每目标冷却或 effect intent。

## 5. 修改范围

- `demo_mapShanmenControlledWeaponWorldAdapter`：World Actor batch 到 canonical target evidence 的捕获结果与错误分类；
- `demo_mapShanmenControlledWeaponProductController`：capture + frozen policy 的组合入口；
- `demo_mapShanmenControlledWeaponRunHost`：exact item 路由；
- World Adapter、Controller、Run Host 三处自动化测试；
- 本 Report 与同名 Log。

生产源码 6 个、测试源码 3 个、文档 2 个。Source 新增 `456` 行、删除 `18` 行；其中生产新增 `267` 行，测试新增 `189` 行、删除 `18` 行。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeapon` | 30 | 0 | 0 | `770631C299EFADB5AB6289D18B5E1ABEDBABE40ACA69990CC6CA9753C9D5DC3D` |
| `Shanmen.0_0_10` | 161 | 0 | 0 | `7FD287F0DC9D774DEEFF5EE6783F8519294D4B608D5B9F1729F86160E296B7F5` |

- 两份最终日志各有唯一实际 RunTests 命令、唯一 queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`；
- 两份日志在测试发现前各有 13 条既存 `LogAutomationTest: Error: Condition failed` 固定诊断噪声；目标测试随后全部 Success，本轮未隐藏该事实；
- Source changed-file gate：`PASS Changed=9 Rules=3 Required=11 Logs=2`；
- 加入 Report / Log 后最终 staged gate：`PASS Changed=11 Rules=3 Required=11 Logs=2`；
- regression coverage self-test：`14/14 PASS`；
- 新增 Source 行的 ApplyDamage / TakeDamage / Impact request / delivery / world query / Spawn / RNG / inventory Commit 扫描命中 `0`；
- `git diff --check`：native exit `0`；
- 最新源码早于最终 Editor DLL 与 Game executable。

## 7. 首次验证与契约复审

首次 Editor integration build 为 `36/36` actions、`Result: Succeeded`、native exit `0`、`124.15s`。实现后的 Product `29/29` 与全量 `160/160` 首次自动化均通过。

契约复审后补入 World Adapter 直属测试，覆盖未完成 receipt、null、unregistered、outside-emission、missing、duplicate 与成功捕获，因此执行最终 Editor 增量重编译：`4/4`、`6.27s`、exit `0`。最终自动化为 Product `30/30`、全量 `161/161`。

设计时拒绝了 Registry 反向查询：既有 Registry 明确允许 Actor 与 Component 等多个对象映射同一 EntityId，反查单一 UObject 会制造歧义。也拒绝在 Controller 持久缓存 callback Actor 或 target evidence，因为那会形成与 detector receipt 并行的第二份状态。最终采用完成 receipt 与显式 Actor batch 的一次性 join，结果不保留 UObject。

## 8. 编译

Editor 与 Game 均使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- Editor 首次 `36/36`、最终增量 `4/4`，均 `Result: Succeeded`、native exit `0`；
- Game `31/31`、`Result: Succeeded`、native exit `0`、`105.65s`；
- 没有源码失败、环境失败、内存错误、外层超时或构建重试；
- Game 只生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-13-orbit-threat-world-evidence>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-13-orbit-threat-world-evidence/Docs/Report/Dev.D.UE.0.0.10.P6.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-13-orbit-threat-world-evidence/Docs/Log/Dev.D.UE.0.0.10.P6.13.r0_log.md>
