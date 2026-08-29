# Dev.D.UE.0.0.10.P6.15.r0 Report

## 1. 结论

P6.15 已完成 Orbit `ThreatPresenceIntent` 的 Run 级消费权威，结论为 **PASS**。

P6.14 的 accepted-target intent 现在可以由 `FShanmenControlledWeaponThreatPresenceAuthority` 原子消费：首次批次获得连续 authority revision，完全相同的重放返回原 receipt 而不推进 revision，完成的空 sample 返回显式 `NoOp`。权威只记录“该确定性 intent 已被消费”，不施加伤害、控制、持续时间、频率、叠加、冷却或其他 gameplay effect。

产品层由 `Fdemo_mapShanmenControlledWeaponRunHost` 持有每个 Run 唯一 authority，并继续按 exact `ItemInstanceId` 核对 intent 所属 action。最后一把飞剑退出 Run Host 时，authority 与 Run 绑定一起清空。

## 2. 功能性

- 新增独立 Runtime 单元 `ShanmenControlledWeaponThreatPresenceAuthority`，与负责生产 intent 的 `ControlledWeaponExecution` 分离；
- authority 绑定 exact `RunId + SourceEntityId`，revision 从 `0` 开始；
- 首次消费一个有效批次时，为 canonical intent 顺序分配连续 revision，并以 intent ID 建立不可变审计 receipt；
- 完整批次先在候选 authority 上校验，成功后才提交，失败不会产生部分 ledger；
- 完全相同的批次重放返回 `AlreadyConsumed` 和原 revision，ledger 大小与 revision 均不变化；
- 有效空 receipt 返回 `NoOp`，不伪造一次消费；
- invalid receipt、未初始化 authority、Run 不匹配、来源实体不匹配、intent 冲突、部分重放冲突与 revision 溢出均失败关闭；
- Run Host 消费前逐字段核对 live controller action 的 owner、activation、source entity、source item、action definition、content version/digest 与 source tags；
- 未知 item 不能消费另一把飞剑的 presence；成功消费后原飞剑仍可 Launch 并继续 Directed contact；
- 消费前后目标 vitality 与 impact ledger 不变。

## 3. 完整性

新增 1 个 Runtime 自动化测试，并扩展 Run Host 产品测试：

1. `CombatRuntime.ControlledWeapon.ThreatPresenceAuthority`：双目标原子首次消费、canonical revision、完全重放、下一 sample 新 revision、空 sample `NoOp`、foreign Run/source 拒绝与零 effect；
2. `ControlledWeaponRunHost.OrbitThreatRouting`：exact-item 首次消费和重放、未知 item 拒绝、vitality/impact 不变、后续 Launch/Directed 可用、最后 item 退休时 authority 清空。

全量 `Shanmen.0_0_10` 由 `162` 增至 `163` 个测试并全部 Success。Focused Runtime ControlledWeapon 为 `8/8`，Focused Product ControlledWeapon 为 `30/30`。

## 4. 兼容性

- P6.11 geometry、P6.12 policy、P6.13 World evidence 与 P6.14 presence intent 仍分别是唯一上游事实；
- authority 是纯值类型，不引用 `UWorld`、`AActor`、Actor cache、World query、Registry 反向索引或产品 vitality；
- Run Host 只增加单向消费入口，不改变 Controller、Session、World Adapter、Impact 或 Action lifecycle 的既有所有权；
- 多把飞剑共享 Run 级 revision ledger，但各 intent 的确定性身份仍包含 source item，不能合并物品身份；
- 未修改 GameplayTags、Profile schema、存档、Item authority、inventory、input、资产或 Build.cs；
- 未冻结 presence 的 cadence、magnitude、duration、period、stack、cooldown 或具体效果；高频采样前仍需明确 ledger 生命周期/压缩策略。

## 5. 修改范围

- 新增 Runtime authority 头文件与实现文件；
- 扩展 Runtime ControlledWeapon 自动化；
- Run Host 新增 Run 级 authority 所有权、exact-item 消费路由与生命周期复位；
- 扩展 Run Host 产品自动化；
- 本 Report 与同名 Log。

Source 共 6 个文件：新增 `723` 行、删除 `4` 行；其中生产新增 `514` 行、删除 `2` 行，测试新增 `209` 行、删除 `2` 行。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p615_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 8 | 0 | 0 | `BEEB8556EEF5CBACCBC59FDCDD830522D34553F5F3A315E4909174DD2FF68D46` |
| `p615_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 30 | 0 | 0 | `7F9BB55E96A03AC2A0EA520B379670D7C06EBBBE411AC7B97FF74FC6556D51DD` |
| `p615_full.log` | `Shanmen.0_0_10` | 163 | 0 | 0 | `05890B2B87FE9CB7CB824A04CDD44B723832667D5B5031F2F213FAFB63309D40` |

- 三份日志各有唯一实际 RunTests 命令、queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`；
- 测试发现前各有 13 条既存 `LogAutomationTest: Error: Condition failed` 固定诊断噪声，正式目标测试全部 Success；
- changed-file gate：`PASS Changed=6 Rules=2 Required=11 Logs=3`；
- regression coverage self-test：`14/14 PASS`；
- authority 生产单元没有 damage/impact delivery、World/Actor、Spawn、RNG、inventory mutation；唯一 `Reserve` 为 `TArray::Reserve` 容量预留；
- `git diff --check`：native exit `0`；
- 最新测试源码早于最终 Editor DLL 与 Game executable。

## 7. 首次失败与复审

本阶段没有源码构建失败、环境失败或自动化失败。首版实现通过编译和全部测试后，收口复审发现 authority 实现与 intent 生产实现同处 `ControlledWeaponExecution.cpp`，会让消费边界继续膨胀并重新耦合生产/消费职责。

因此在提交前把 authority 拆为独立 Runtime 单元；行为与公开契约不变。拆分后重新执行 Editor 编译、三组自动化、changed-file gate、self-test 与 Game 构建，全部通过。

## 8. 编译

Editor 与 Game 均使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 拆分后 Editor：`43/43`、`147.88s`、`Result: Succeeded`、native exit `0`；
- Game：`40/40`、`135.21s`、`Result: Succeeded`、native exit `0`；
- 没有源码构建失败、环境错误、内存错误或外层超时；
- Game 只生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-15-threat-presence-authority>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-15-threat-presence-authority/Docs/Report/Dev.D.UE.0.0.10.P6.15.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-15-threat-presence-authority/Docs/Log/Dev.D.UE.0.0.10.P6.15.r0_log.md>
