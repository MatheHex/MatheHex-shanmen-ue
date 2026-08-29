# Dev.D.UE.0.0.10.P6.14.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.14.r0`；
- 基线提交：`3d599a2f0fbef6f4531a41301662b5256f59b37b`（P6.13）；
- 分支：`agent/0.0.10-p6-14-orbit-threat-presence-intents`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.11 已冻结 Orbit geometry receipt，P6.12 冻结 target policy，P6.13 将真实 Actor batch 闭合为 World evidence。规划要求环绕飞剑形成近身威慑，但尚未冻结强度、持续时间、频率、叠加、冷却或控制语义。因此本轮只实现第一种无歧义输出：accepted target 在某个完成 sample 中处于威胁包络的 `ThreatPresenceIntent`。

## 契约冻结

### Sample-scoped，而非持续效果

一条 intent 对应一个 accepted candidate。它没有 magnitude、duration、period、stack 或 cooldown；调用方是否以及何时发起下一 sample 仍是显式决策。这样不会用执行层的默认 tick 频率偷偷冻结玩法数值。

### 确定性身份

命名空间为 `Shanmen.ControlledWeapon.ThreatPresence.r1`。canonical parts 包含 RunId、ActivationId、SourceEntityId、SourceItemInstanceId、DetectorId、DetectorKind、HitOrdinal 与 TargetEntityId。完全相同的 receipt 重放得到相同 ID；下一 sample 的 ordinal 不同，因此得到新 ID。

### Policy 保持可审计

presence receipt 内嵌完整 policy，并只按其 canonical 顺序提取 accepted targets。被 self 或 required tags 拒绝的目标不生成 intent，但不会从审计记录消失。零目标完成 sample 返回有效空 receipt。

### 无产品副作用

构建函数为 const 值转换，不调用 impact resolver、World delivery、vitality commit、item transaction 或 Actor API。状态、Action、Definition、tags 与 self policy 不匹配时清空输出并失败关闭。

## 实现链路

1. Runtime 新增 presence intent / receipt 与确定性 ID 派生；
2. Runtime 将完成的 exact policy 转换成 accepted-only canonical intent 列表；
3. Session 只在活动 action 上转发；
4. Product Controller 要求 Orbiting 且没有活动 contact window；
5. Run Host 通过 exact item 查找 Controller，不新增全局或反向索引；
6. Runtime 新增直属测试，Controller 与 Run Host 既有产品场景加入零副作用及 exact-item 断言。

## 自动化覆盖

### Runtime

- accepted-only：Living 生成 intent，self 与 missing tags 不生成；
- receipt 保留 policy，来源 item / Run / target 完全匹配；
- 完全相同 policy 重放得到同一 `IntentId`；
- 下一显式 sample 以新 ordinal 得到不同 `IntentId`；
- 零目标 sample 得到有效空 receipt；
- 全流程 impact ledger 为 0，Execution 仍处于 Orbiting；
- Launch 后旧 policy 不可再生成 intent。

### 产品层

- Controller 在真实 Actor evidence + policy 后生成 exact-item presence；
- presence 前后目标 vitality 不变、impact ledger 为 0；
- Run Host exact item 成功，未知 item 无法借用 policy；
- presence 生成后 Launch 与下一 Directed contact 仍正常。

## 首轮失败审计

首轮 Product 日志：

```text
Success=29 Fail=1 NativeExit=0
Failed=Shanmen.0_0_10.Product.ControlledWeaponRunHost.OrbitThreatRouting
Assertion=Host emits threat presence for the exact routed item
SHA256=894853042A5BA776DA501E67A65E061478208283D1B2E4E0CBCD37FFCF64859F
```

根因不是生产路由错误。测试先把有效 `ThreatPolicy` 作为未知 item 负例的输出；失败关闭会按契约清空输出，后续正例因此使用了空 policy。测试改为独立 `RejectedPolicy`，没有修改生产逻辑。失败日志保留为 `Saved/Automation/P614/p614_product_initial_fail.log`。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 运行段 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p614_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 7 | 0 | 0 | `5.66s` | `D4F8CE3CCE217A5731EA572CAB9441A4A3A8DD446B60AE9BC25DEC5A87904682` |
| `p614_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 30 | 0 | 0 | `6.00s` | `7B16ACB74EC83D1EFBD50423A37FAF0BA352340590C707D1719B71CB2DFC8313` |
| `p614_full.log` | `Shanmen.0_0_10` | 162 | 0 | 0 | `14.68s` | `C6521636727C0E2BC6B7047A6EE14C3256E26D70ACF40EB76331AEE9253A645D` |

每份最终日志有一个实际 RunTests 命令、一个 queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与 native exit `0`。每份在测试发现前有 13 条既存 `LogAutomationTest: Error: Condition failed` 诊断噪声；正式目标测试 ControllerErrors 为 `0` 且全部 Success。

## Changed-file gate

Source 状态：

```text
REGRESSION_COVERAGE: PASS Changed=11 Rules=4 Required=11 Logs=3
```

完整 `Shanmen.0_0_10` 覆盖 CombatRuntime、Items、CombatRunCoordinator、ControlledWeapon Adapter / Controller / RunCommandRouter / RunHost / RunLifecycle / Session / WorldDelivery 与 WorldGameplay 共 11 个必跑组。Runtime 与 Product focused 日志提供直接证据。Regression coverage self-test：`14/14 PASS`。

加入 Report / Log 后最终 gate：

```text
REGRESSION_COVERAGE: PASS Changed=13 Rules=4 Required=11 Logs=3
```

## 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- Editor 首次：`42/42` actions，`Result: Succeeded`，native exit `0`，`158.62s`；
- 测试修正后 Editor 增量：`4/4`，`Result: Succeeded`，native exit `0`，`6.26s`；
- Game：`39/39`，`Result: Succeeded`，native exit `0`，`144.88s`；
- 没有源码构建失败、环境错误、内存错误或外层超时；
- Game executable 未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- Source 新增 `397` 行、删除 `4` 行；生产新增 `244` 行；
- 生产新增行的 ApplyDamage / TakeDamage / impact resolution / delivery / UWorld / AActor / Spawn / RNG / inventory Commit 扫描命中 `0`；
- `Reserve` 唯一命中是 `TArray::Reserve` 容量预留；
- Registry、GameplayTags、Profile schema、存档、Item authority、input、资产和 Build.cs 均未修改；
- 最新测试源码 UTC `2026-08-29T03:41:42.2415882Z`；
- Editor DLL UTC `2026-08-29T03:41:55.2607110Z`；
- Game executable UTC `2026-08-29T03:45:59.7799064Z`；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

下一阶段应先冻结 presence intent 的消费 authority 与幂等 ledger，再决定 cadence / per-target cooldown。若没有正式数值依据，继续保持消费结果无伤害、无硬控，不应把每帧 overlap 默认解释为游戏效果。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-14-orbit-threat-presence-intents/Docs/Report/Dev.D.UE.0.0.10.P6.14.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-14-orbit-threat-presence-intents/Docs/Log/Dev.D.UE.0.0.10.P6.14.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-14-orbit-threat-presence-intents>
