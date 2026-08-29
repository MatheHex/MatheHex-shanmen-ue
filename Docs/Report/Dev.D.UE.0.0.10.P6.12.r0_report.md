# Dev.D.UE.0.0.10.P6.12.r0 Report

## 1. 结论

P6.12 已完成 Orbit 近身威胁候选的确定性目标策略回执，结论为 **PASS**。

P6.11 的完整几何候选回执现在可以与调用方显式提供的目标标签证据按 `TargetEntityId` 一一闭合。系统复用飞剑 Definition 已冻结的 `RequiredTargetTags + bRejectSelf`，为每个规范排序候选输出 `Accepted / RejectedSelf / RejectedMissingRequiredTags`，没有建立第二套敌我规则。

本阶段仍不产生威慑效果、伤害、防御层或冷却，也不决定采样半径、频率、碰撞通道、阵营标签来源或产品数值。

## 2. 功能性

- `FShanmenControlledWeaponThreatTargetEvidence` 冻结一个 `TargetEntityId` 与调用方提供的目标标签；
- `FShanmenControlledWeaponThreatPolicyReceipt` 保留原始 P6.11 emission receipt、精确 required tags、自目标规则和逐目标决策；
- 证据输入顺序不影响输出：策略结果继续严格对齐 P6.11 已按 GUID 规范排序的候选；
- 每个几何候选必须恰好有一份同 ID 证据；缺失、重复、额外替换或无效 ID 全部失败关闭；
- 自目标优先形成 `RejectedSelf`，缺少 `TargetLiving` 等必需标签形成 `RejectedMissingRequiredTags`，符合既有 Definition 的同一判定语义；
- 空候选 emission 与空证据形成合法 0 项策略回执，明确表示一次完成但未观察到目标的 sample；
- exact evidence replay 产生同一决策且不推进 ordinal、command sequence 或 impact ledger；
- 旧 Orbit receipt 在飞剑 Launch 离开 Orbiting 后不可重新消费；
- API 沿 `CombatRuntime -> Product Session -> Product Controller -> Run Host` 透传，Host 仍按 exact `ItemInstanceId` 路由；
- 所有完成回执字段私有，只通过 const getter 暴露，并可重新自校验。

## 3. 完整性

新增 1 个 Runtime 自动化场景并扩展 2 个产品场景：

1. `CombatRuntime.ControlledWeapon.OrbitThreatTargetPolicy`：反序证据 join、自目标拒绝、Living 接受、缺标签拒绝、缺失/重复失败关闭、空 sample、exact replay、Launch 后旧回执拒绝；
2. `Product.ControlledWeaponController.OrbitThreatProjection`：Product 层输出 accepted target policy receipt，且 vitality 与 impact ledger 保持不变；
3. `Product.ControlledWeaponRunHost.OrbitThreatRouting`：exact item 可评估自己的 receipt，未知 item 不可借用。

全量 `Shanmen.0_0_10` 由 `159` 增至 `160` 个测试并全部 Success。

## 4. 兼容性

- 复用既有 `FShanmenControlledWeaponDefinition.RequiredTargetTags` 与 `bRejectSelf`，未新增 faction、hostility 或默认 target policy；
- 未修改 Directed damage、ImpactId、DefenseResolver、Vitality、Item authority、Action lifecycle 或 detector ordinal；
- 未把目标标签从 Actor、FactionComponent 或 legacy targeting 自动推导，标签来源仍由后续产品契约显式决定；
- 未新增 World query、overlap cadence、每目标 cooldown、effect intent、damage packet 或 inventory transaction；
- 未修改 GameplayTags、Profile schema、存档、input mapping、资产或 Build.cs；
- Runtime 新增代码对 `demo_map / UWorld / AActor / world query / RNG` 的边界扫描命中 `0`。

## 5. 修改范围

- `ShanmenCombatRuntime`：ControlledWeapon threat evidence、decision 与 policy receipt；
- `demo_map`：Session、Product Controller、Run Host 的只读策略透传；
- Runtime、Controller、Run Host 三处既有自动化测试；
- 本 Report 与同名 Log。

生产源码 8 个、测试源码 3 个、文档 2 个。Source 新增 `512` 行，其中生产新增 `328` 行。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 6 | 0 | 0 | `9E75688FAF12E79A52B6E018D8331A1DCB965F24EC1E21389B82AFF7A76A312E` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 29 | 0 | 0 | `0BD6357D6909094CC83548AE71CB5D02575FE69A233F25BD701C78E3421455BD` |
| `Shanmen.0_0_10` | 160 | 0 | 0 | `36B89CDA9F25D3B581ACAE409E0513646A1F106E0BAB6A199BC23BA17CFD0BD5` |

- 三份最终日志各有唯一实际 RunTests 命令、queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`；
- 每份日志在测试发现前仍有既存的 13 条 `LogAutomationTest: Error: Condition failed` 固定诊断噪声；本轮未隐藏该事实，也未把它误报为本轮测试失败；
- changed-file gate：`PASS Changed=11 Rules=4 Required=11 Logs=3`；
- 加入 Report / Log 后最终 staged gate：`PASS Changed=13 Rules=4 Required=11 Logs=3`；
- regression coverage self-test：`14/14 PASS`；
- 新增 Source 行的 Damage / Impact / World query / Spawn / RNG / inventory Commit 扫描命中 `0`；
- `git diff --check`：native exit `0`；
- 最新源码早于最终 Editor DLL 与 Game executable。

## 7. 首次验证与契约复审

首次 Editor integration build 为 `42/42` actions、`Result: Succeeded`、native exit `0`、`158.51s`。首次 Game build 为 `39/39`、`Result: Succeeded`、native exit `0`、`135.51s`。

实现后的三组首次自动化均通过。随后契约复审补入“空候选 sample 也必须形成合法 0 项策略回执”的断言，因此对仅测试源变化执行了最终增量重编译：Editor `4/4`、`9.80s`、exit `0`；Game `3/3`、`11.28s`、exit `0`。最终自动化再次全部通过，没有源码失败、测试失败、内存错误或超时。

设计时拒绝了两种方案：在 Product 再缓存逐 callback target 会复制 detector authority；直接读取 legacy faction 会在未冻结标签来源前固化第二套敌我规则。最终方案只消费 P6.11 完成回执和显式证据，保持几何、策略与效果三层分离。

## 8. 编译

Editor 与 Game 均使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- Editor 首次 `42/42`，最终增量 `4/4`，均 `Result: Succeeded`、native exit `0`；
- Game 首次 `39/39`，最终增量 `3/3`，均 `Result: Succeeded`、native exit `0`；
- Game 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-12-orbit-threat-policy-receipts>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-12-orbit-threat-policy-receipts/Docs/Report/Dev.D.UE.0.0.10.P6.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-12-orbit-threat-policy-receipts/Docs/Log/Dev.D.UE.0.0.10.P6.12.r0_log.md>
