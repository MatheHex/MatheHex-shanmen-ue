# Dev.D.UE.0.0.10.P6.14.r0 Report

## 1. 结论

P6.14 已完成 Orbit 近身威胁的首个效果边界，结论为 **PASS**。

P6.13 产生的 accepted target policy 现在可以转换为确定性的 `ThreatPresenceIntent`：每个完成 sample、每个 accepted target 恰好一条，表示“该目标在本次环绕飞剑威胁包络内”。该 intent 是零数值、零持续时间、零副作用的值类型事实，不制造伤害、控制、减速、硬直、仇恨、防御修正或跨 sample 冷却。

本阶段没有猜测尚未冻结的威慑强度、采样频率或叠加规则。sample 的发起 cadence 继续由调用方显式拥有；后续系统可以使用 `IntentId` 去重完全相同的重放，但不同 detector ordinal 的新 sample 会得到新身份。

## 2. 功能性

- `FShanmenControlledWeaponThreatPresenceIntent` 保存确定性 `IntentId`、Run、来源物品和 canonical hit candidate；
- ID 使用独立命名空间 `Shanmen.ControlledWeapon.ThreatPresence.r1`，由 Run、Activation、来源实体、来源物品、Detector、DetectorKind、HitOrdinal 与目标实体共同派生；
- `FShanmenControlledWeaponThreatPresenceReceipt` 保留完整 policy audit，并按 policy 的 canonical target 顺序输出 accepted intents；
- rejected-self 与 rejected-missing-required-tags 目标不会形成 intent，仍完整保留在内嵌 policy 中；
- 完成的零目标 sample 形成有效的空 receipt，而不是被误判为失败；
- 完全相同的 policy 重放产生相同 ID；同一目标在下一次显式 sample 中因 ordinal 不同产生新 ID；
- 构建入口要求 exact action、definition、required tags、self policy、Orbiting 状态且当前无活动 emission；旧 receipt 在状态切换后失败关闭；
- Session、Product Controller 与 Run Host 只做单向转发；Run Host 继续按 exact `ItemInstanceId` 路由，未知物品不可借用另一把飞剑的 policy；
- 成功与失败路径均不写 vitality、impact ledger、item、inventory、World 或 Actor。

## 3. 完整性

新增 1 个 Runtime 自动化测试并扩展 2 个既有产品测试：

1. `CombatRuntime.ControlledWeapon.OrbitThreatPresenceIntents`：accepted-only、policy audit、确定性重放、下一 sample 新身份、空 sample、零 impact 与状态切换失败关闭；
2. `ControlledWeaponController.OrbitThreatProjection`：真实 World evidence 后生成 exact-item presence，目标 vitality 与 impact ledger 保持不变；
3. `ControlledWeaponRunHost.OrbitThreatRouting`：exact item 成功，未知 item 失败关闭，随后 Launch 与 Directed 流程继续可用。

全量 `Shanmen.0_0_10` 由 `161` 增至 `162` 个测试并全部 Success。Focused Runtime ControlledWeapon 为 `7/7`，Focused Product ControlledWeapon 保持 `30/30`。

## 4. 兼容性

- P6.11 geometry receipt、P6.12 target policy 与 P6.13 World evidence 仍分别是唯一几何、策略和产品证据权威；
- 未增加第二份候选缓存、Actor 缓存、World 查询、反向 Registry 或另一套敌我判断；
- intent 只携带既有 canonical candidate，不引入 GAS、Gameplay Effect、伤害 request 或产品 Actor 依赖；
- 未修改 Impact、Defense、Vitality commit、Item authority、Action lifecycle、GameplayTags、Profile schema、存档、input、资产或 Build.cs；
- 未冻结 magnitude、duration、period、stack、cooldown、faction、hostility 或消费端 authority。

## 5. 修改范围

- `ShanmenControlledWeaponExecution`：presence intent / receipt、确定性身份与 accepted-policy 转换；
- `demo_mapShanmenControlledWeaponSession`：活动 session 转发；
- `demo_mapShanmenControlledWeaponProductController`：Orbiting 产品入口；
- `demo_mapShanmenControlledWeaponRunHost`：exact-item 路由；
- Runtime、Controller、Run Host 三处自动化测试；
- 本 Report 与同名 Log。

生产源码 8 个、测试源码 3 个、文档 2 个。Source 新增 `397` 行、删除 `4` 行；其中生产新增 `244` 行，测试新增 `153` 行、删除 `4` 行。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p614_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 7 | 0 | 0 | `D4F8CE3CCE217A5731EA572CAB9441A4A3A8DD446B60AE9BC25DEC5A87904682` |
| `p614_product_initial_fail.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 29 | 1 | 0 | `894853042A5BA776DA501E67A65E061478208283D1B2E4E0CBCD37FFCF64859F` |
| `p614_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 30 | 0 | 0 | `7B16ACB74EC83D1EFBD50423A37FAF0BA352340590C707D1719B71CB2DFC8313` |
| `p614_full.log` | `Shanmen.0_0_10` | 162 | 0 | 0 | `C6521636727C0E2BC6B7047A6EE14C3256E26D70ACF40EB76331AEE9253A645D` |

- 三份最终日志各有唯一实际 RunTests 命令、queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`；
- 三份最终日志在测试发现前各有 13 条既存 `LogAutomationTest: Error: Condition failed` 固定诊断噪声；目标测试 ControllerErrors 为 `0` 且全部 Success；
- Source changed-file gate：`PASS Changed=11 Rules=4 Required=11 Logs=3`；
- 加入 Report / Log 后最终 gate：`PASS Changed=13 Rules=4 Required=11 Logs=3`；
- regression coverage self-test：`14/14 PASS`；
- 生产新增行的 ApplyDamage、TakeDamage、impact resolution / delivery、UWorld、AActor、Spawn、RNG 与 inventory Commit 命中 `0`；唯一 `Reserve` 命中为 `TArray::Reserve` 容量预留；
- `git diff --check`：native exit `0`；
- 最新测试源码早于最终 Editor DLL 与 Game executable。

## 7. 首次失败与修复

首轮 Product 自动化为 `29 Success / 1 Fail`，原生退出码仍为 `0`；失败测试是 `ControlledWeaponRunHost.OrbitThreatRouting`。失败日志已保留并计算 SHA-256，未被最终成功日志覆盖。

根因是测试把成功的 `ThreatPolicy` 同时传给未知 item 负例作为输出参数。Run Host 按失败关闭契约先清空输出，导致随后的 presence 正例收到空 policy。修复只是在负例使用独立 `RejectedPolicy`，保留“失败必须清空输出”的正确产品语义；生产代码未因该失败改动。增量 Editor 重编译后，Product 为 `30/30`，全量为 `162/162`。

## 8. 编译

Editor 与 Game 均使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- Editor 首次 `42/42`、`158.62s`、`Result: Succeeded`、native exit `0`；
- 测试修正后的 Editor 增量 `4/4`、`6.26s`、`Result: Succeeded`、native exit `0`；
- Game `39/39`、`144.88s`、`Result: Succeeded`、native exit `0`；
- 没有源码构建失败、环境失败、内存错误或外层超时；
- Game 只生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-14-orbit-threat-presence-intents>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-14-orbit-threat-presence-intents/Docs/Report/Dev.D.UE.0.0.10.P6.14.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-14-orbit-threat-presence-intents/Docs/Log/Dev.D.UE.0.0.10.P6.14.r0_log.md>
