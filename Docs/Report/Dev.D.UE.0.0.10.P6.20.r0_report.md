# Dev.D.UE.0.0.10.P6.20.r0 Report

## 1. 结论

P6.20 已完成 Run Host 拥有的显式 Orbit threat sample 原子事务，结论为 **PASS**。

P6.19 已闭合 sample freshness，但产品调用方仍需依次执行 Begin、若干 Project、End 与 Finalize；中途返回时容易遗留活动窗口或花掉 detector ordinal。本阶段新增一个窄接口，在完整 Host 候选副本上组织既有五步链路，仅在几何、证据、策略、presence、authority consumption 与 Host 自检全部成功后一次提交。

## 2. 功能性

- 新增 `Fdemo_mapShanmenControlledWeaponOrbitThreatContact`，只承载调用方已采集的 overlap、接触点与法线；
- 新增 `TrySampleOrbitThreat()`，调用方提供 exact item、当前 Coordinator 与一组有界 contacts；
- Host 从 overlap 本身取得瞬时目标 Actor，不引入第二份目标身份输入；
- 整个 Host（所有 Controller 与 Run-scoped presence authority）先复制为候选；
- 候选依次执行 Begin、Project、End、evidence/policy/presence/consume finalization；
- 任一 contact 无法解析、候选重复、目标证据不完整、Coordinator/item 不匹配或 authority 拒绝时，候选整体丢弃；
- 成功时一次提交 Controller emission ordinal、latest-completed sample 与 authority checkpoint；
- 空 contact 数组是显式合法 sample，生成 `NoOp` consumption 并推进 sample checkpoint；
- 本接口不拥有 cadence，不查询 World，不产生伤害、控制、效果或 Actor。

## 3. 原子性不变量

自动化直接验证：

- 单 contact 事务返回完整 exact-item audit，并消费一个 presence intent；
- 同一 target 的第二个 contact 被既有 canonical candidate 规则拒绝，整个事务回滚；
- 回滚后没有活动窗口泄漏，authority revision、checkpoint 与累计 intent 数均不变；
- 随后的空 sample 复用失败事务未花掉的 ordinal；
- 无法解析的 overlap 与未知 item 同样不改变任何状态；
- 再次合法采样继续使用精确下一 ordinal；
- 一件 item 的失败事务不会推进另一件 item；第二件 item 的首个 sample 仍为 ordinal `0`；
- 所有采样前后目标 vitality 相等，ControlledWeapon impact ledger 仍为 `0`。

## 4. 兼容性与边界

P6.20 只组合既有公开语义，没有复制几何适配、target registry、threat policy 或 authority ledger。既有分步 API 保留，供低层测试和需要逐步审计的调用方使用；新 API 是产品 Host 的安全便捷路径。

`FOverlapResult` 是唯一目标来源：projection 使用它解析 canonical entity，finalization 再用同一 overlap Actor 捕获标签证据，因此不存在几何 target 与 evidence target 两条可漂移输入。最终结果仍复用 P6.18/P6.19 的 `Fdemo_mapShanmenControlledWeaponThreatFinalizationResult`。

## 5. 测试覆盖

最终无头自动化：

| Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 8 | 0 | 0 | `0.135s` | `4CC9FCB652EE70E9F87536C0E18F42C4A84A20F46C162B95335308C5BDE2B6D8` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 32 | 0 | 0 | `0.551s` | `F5749E91151C8313C889DD86D50DF592DA372F537BF4F404E33B37164F6ADBDD` |
| `Shanmen.0_0_10` | 165 | 0 | 0 | `8.485s` | `EB1BB8DE707907DEEECE5AEF7856BFD971A8CAA225DDA06FFA88C24BE3E025AD` |

每份最终日志只有一个实际 `Automation RunTests` 命令、一个 queue-empty 终止、Fail `0`，Fatal/unhandled/assert/ensure `0`，进程原生退出码均为 `0`。

## 6. 首次失败与修复

在首版 8/8、32/32、165/165 通过后，增加双 item 断言时，测试补丁上下文误把第二件 item 接入旧 `OrbitThreatRouting` 夹具，而 P6.20 新夹具没有接入它。结果为 RunHost `6/8`、产品 `30/32`、完整回归 `163/165`；三个 UE 进程原生退出码仍为 `0`，但测试结果明确为失败，因此门禁正确拒绝。

失败日志已保留：

| 日志 | Result | SHA-256 |
|---|---|---|
| `p620_runhost-backup-2026.08.29-06.54.47.log` | 6 Success / 2 Fail | `1B5A7BCE8C85592DF139FCA6A12B3BCC003620BA0E034D4CF369BC5178F52C7E` |
| `p620_product-backup-2026.08.29-06.55.08.log` | 30 Success / 2 Fail | `454B65B957E68672546EE3E0F38E7D0F2723072E747D11D6E02D575250353059` |
| `p620_full-backup-2026.08.29-06.55.39.log` | 163 Success / 2 Fail | `17587367F814B2E835658373D63EB5B8C5ACCA3500333C7070C03AB9E5E3FF7C` |

按测试名精确移动 attachment 后，旧测试与新增双 item 断言同时通过。该失败属于测试夹具编辑错误，不是产品源码、UE、内存或 SDK 故障。

## 7. 改动—回归与静态门禁

- `REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=11 Logs=3`；
- mapping self-test：`PASS 14/14`；
- `git diff --check`：native exit `0`；
- 新增生产可执行行扫描 Tick/Timer、damage、effect、spawn、World/RNG、Cooldown/Duration：命中 `0`；
- Source 新增 `250` 行、删除 `0` 行，其中测试新增 `165` 行。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor：`30/30` actions，`Result: Succeeded`，native exit `0`，`87.07s`；
- 双 item 测试增量 Editor：`4/4` actions，native exit `0`，`5.61s`；
- 修正测试夹具后的最终 Editor：`4/4` actions，native exit `0`，`5.40s`；
- 最终 Game：`27/27` actions，`Result: Succeeded`，native exit `0`，`94.32s`；
- 最终 Editor DLL UTC：`2026-08-29T06:56:38Z`；
- 最终 Game executable UTC：`2026-08-29T07:00:01Z`。

所有构建首次执行成功；没有源码、环境、内存、SDK、外层超时或链接失败。Game executable 仅构建，未启动。

## 9. 修改范围与 P/F 边界

- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.h`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.cpp`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`
- 本 Report 与同名 Development Log。

未修改 CombatRuntime 数学、威慑效果定义、cadence owner、item authority、Profile schema、存档、GameplayTags、输入、资产或 Build.cs；长期未跟踪的 0.0.9B 历史文件未纳入本阶段。

本 Report 只包含 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-20-atomic-threat-sampling/Docs/Report/Dev.D.UE.0.0.10.P6.20.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-20-atomic-threat-sampling/Docs/Log/Dev.D.UE.0.0.10.P6.20.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-20-atomic-threat-sampling>
