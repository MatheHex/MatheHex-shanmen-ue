# Dev.D.UE.0.0.10.P16.3.r0 Report

## 1. 结论

P16.3 已补齐 Meridian Shock 治疗链在**临时 Product Session / command journal 丢失**后的恢复入口：新的 Session 会从 ShanmenItems 既有 durable processed-request ledger 中识别尚未 finalize 的治疗 prepare，并且只有在 condition authority 能独立证明同一个 `TreatmentId` 时才继续事务。

恢复决策固定为：

```text
durable prepare + 同 revision 的 active condition
  -> 重建 intent -> treat -> commit

durable prepare + 已处理 treatment receipt
  -> 重建 intent -> commit only

durable prepare + 无 condition proof
  -> fail closed；不 commit、不 cancel、不改 item authority
```

最终结果：

```text
Meridian Shock focused:        14 Success / 0 Fail
Mapped legacy regressions:    352 Success / 0 Fail
Shanmen.0_0_10 full:          727 Success / 0 Fail
Regression coverage:          PASS (Changed=9 / Rules=3 / Required=16 / Logs=7)
Regression gate self-test:    PASS 271/271
Editor + Game Development:    PASS / native status 0
```

## 2. 恢复权威与真实边界

本轮复用 ShanmenItems 已有的 durable prepare/finalize ledger，没有建立第二份 checkpoint、事务信封或库存真值。恢复候选来自 item authority snapshot 中满足以下全部条件的 receipt：

- operation 为 `PreparePreparedRunQuantityIntent`；
- phase 为 `Reserved`，receipt 自身有效且成功；
- purpose 精确等于 `Shanmen.Condition.MeridianShock.Treatment.r1`；
- active Run identity 与当前 correlation 完全一致；
- 尚无引用该 prepare request 的成功 finalize receipt。

本阶段**不声称完整进程崩溃恢复**。item ledger 是 durable 的，但 active condition state 与 processed treatment receipt 仍是运行时 authority：若进程重启后这两项证明都不存在，系统会保留 pending item prepare 并失败关闭，而不是猜测 treatment 已发生或擅自取消库存。

## 3. 精确重建与防误恢复

`Fdemo_mapShanmenMeridianShockTreatmentAdapter::RestorePreparedFromLedger` 会从 receipt 重建原 prepare request，并重新调用 durable prepare API。只有返回 `Replayed` 且 receipt 与原记录逐字段相等，恢复才被接受。

恢复同时校验：

- ready item authority、Game Thread 与完整 Run correlation；
- item 必须仍属于同 Owner、同 Run scope、同 Definition；
- content stamp 必须属于当前产品 authority；
- receipt 的 Quantity、before/after、available、purpose 与 reservation identity 必须自洽；
- prepare 不得已被 commit 或 cancel finalize；
- treatment intent 派生出的 opaque `TreatmentId` 必须等于 ledger reservation identity。

同一 condition authority 若出现多个未决治疗 prepare，会被判定为歧义并失败关闭。恢复路径不会把“缺少证明”解释为取消许可。

## 4. Condition proof 与 Session 顺序

condition component 新增只读的 `TryGetProcessedMeridianShockTreatment`，仅返回与原 intent 精确匹配的运行时 receipt，不暴露可写 journal。

Product Session 的固定顺序为：

```text
reconcile durable item ledger
  -> replay runtime pending commands in stable GUID order
  -> require route and session recovery sets both empty
  -> accept new hotbar work or allow Run teardown
```

因此，新 hotbar 请求不会越过旧 pending prepare；Run teardown 也不能遗忘未决 item transaction。durable reconciliation 成功后不向 Session 伪造一份历史 hotbar command，Session 的 captured-request 数保持为 0。

## 5. 新增验证

在既有 11 个 Meridian Shock treatment tests 上增加 3 个 P16.3 场景：

- `Recovery.ActiveCondition`：无 transient Session 的 durable prepare，可由仍 active 的同 revision condition 重建 `treat -> commit`；
- `Recovery.CommitOnlyProof`：condition 已处理、Session 丢失时，只依据 exact runtime receipt 执行 item commit；
- `Recovery.MissingProofFence`：condition proof 丢失时，恢复失败且 item authority snapshot 前后完全相等，commit/cancel 均为 0。

最终 focused 为 14/0。关键日志分别记录 `recovered=1, commit=1, cancel=0`、`recovered=1, commit=1, cancel=0` 与 `recovered=0, commit=0, cancel=0, unchanged=1`。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Meridian Shock focused | 14 | 0 | `01B14A1FC87B4F07AD097EAD7BCE8F9B0A5CBADA7FCCB5E5EA82D3F58A3B1434` |
| `Shanmen.0_0_10` full | 727 | 0 | `A8DBBD4B570698866AC4DEE2AD4A0CF87AC9698572B715202D8D9E49B7C5CAE5` |
| CodeB | 60 | 0 | `0A32D7779CEF17C8E3E3B9851FA58776C38EDB0FCD5CB7B04C88D48BEF613113` |
| ItemEconomySchema | 24 | 0 | `1B42842493194B62AD48DBE7012AD16BD9063F22E6B976CCD51A2921EFAD81AD` |
| ItemUseAndArmor | 46 | 0 | `52FC46C549D856D3202E3250E5543698838C64CC8681017FCCDFBE3AA0AA07CD` |
| P4 Hotbar | 7 | 0 | `3F379A4180179FAF0CD760D06D5FDE9CF364D51ADC07B0DC6D27C692C76F3685` |
| Profile | 211 | 0 | `80342B152FAD819357549AE8D450CDD5F255D1E33D8D146287D820F90FE4502F` |
| V3 Attributes | 4 | 0 | `5C0372AC17D33EEDCD78431AD5DA6CEA51B248D555397E19044578F49F343B76` |

全量首末 Success 时间为 `07:23:21.123 -> 07:51:09.915 UTC`，用时约 `27m48.79s`；唯一 queue-empty 终止标记存在，原生退出码 0，Fatal、Unhandled、Ensure 与 `generate_204` 均为 0。

UE 5.8 启动阶段仍输出 13 条引擎自带 AutomationTest `Condition failed` 自检记录；它们发生在所选测试开始前。最终 controller 结果为 727 Success / 0 Fail，且没有 fatal、unhandled 或 ensure 标记。

## 7. 改动—回归门禁与静态边界

9 个改动文件命中 `CombatCondition`、`MeridianShockTreatmentAdapter` 与 `MeridianShockTreatmentProductFlow` 三条规则。16 个必跑组全部由 7 份健康日志覆盖：Shanmen 全量覆盖新产品链，六份 legacy 日志覆盖 CodeB、schema、item use、hotbar、profile 与 attributes。

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=16 Logs=7
SELF_TEST: PASS 271/271
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS Files=8 Patterns=12
GIT_DIFF_CHECK: PASS
```

流程文件 SHA：gate `F08FF5DD79288D963B9C1D4217C1AE8D9B0EDCE1866ABD96DED3C023B2E98B4C`；self-test `633D910E13342A4270666EB4F949163E3E91F28713E333758CF51D1333BC0EF6`；mapping JSON `DFE18FDD15197821D47AB1DAA6FC6D8C98BA56702E58FB2C6D009626558AF2A7`。

边界扫描确认恢复实现不依赖旧 `demo_mapItemSubsystem`、通用 quick-slot 写入口、`UWorld`、`AActor`、Spawn/ApplyDamage、RNG、Tick/Timer、Enhanced Input 或 UI。

## 8. UE 5.8 环境诊断

第一次全量命令的 DPC 参数未整体引用，被 PowerShell 拆成 `HomeScreen` 与 `.EnableHomeScreen=0`。该次运行在 578/0 时主动停止，缺少终止标记并出现 63 条 `generate_204` 记录；日志 SHA 为 `CFBBC7EBD320C123961BEC424076D169948005E028DEEDED8B64B89758EC5D2F`，只保留为失败环境诊断，不计入通过证据。

最终命令将 `-ForceDPCVars=HomeScreen.EnableHomeScreen=0` 作为单一参数传入；日志确认该 CVar 生效，网络探测为 0，完整 727/0 自然结束。未修改 Windows、UE Engine 或用户级配置。

## 9. 构建证据

本轮使用 `-NoUBA -MaxParallelActions=1` 控制提交内存压力。UBT 自身写入最终日志，命令原生退出码均为 0。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 33 / 145.12s | `6A2FDC4F2872477459CBC655EFF59A5D951BA4DF71EC35A2AE56CAFA8EE06E16` |
| Editor Development | Succeeded | 30 / 115.44s | `11874FC016722947DF9DD2962D9364FDE69017483625A2EC249293CCC4BE3270` |

最终产物：

- `demo_map.exe`：355,909,120 bytes，SHA-256 `7D05E9CC7024E0F645D1C2078521CECABCC3612C393CFB04E6E373D838EA2C68`；
- `UnrealEditor-demo_map.dll`：14,526,464 bytes，SHA-256 `73C03BC5779A8921686BB727C25FE579C15928908EA60A0DC393A7CBA7C9BD39`。

全量回归后仅澄清了两个 header comment；最终 Game/Editor builds 与 focused 14/0 均在精确最终工作树上重新执行。

## 10. 修改范围与 P/F 边界

本轮只修改 combat condition 的只读 treatment proof getter、Meridian Shock treatment adapter/route/session 及其 Automation tests；共 9 个生产/测试文件。没有修改 GameMode、PlayerController、地图、资源、存档 schema、Windows 或 Engine。

raw logs 仅保存在本机 `Saved/Logs`。长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

本 Report 只证明 P 阶段 C++ recovery contract、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

下一阶段若要支持真实进程崩溃恢复，必须先为 condition truth / treatment receipt 定义唯一、版本化且可验证的持久化来源；不能通过复制 item ledger 或猜测 condition 结果实现。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-3-meridian-shock-ledger-recovery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-3-meridian-shock-ledger-recovery/Docs/Report/Dev.D.UE.0.0.10.P16.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-3-meridian-shock-ledger-recovery/Docs/Log/Dev.D.UE.0.0.10.P16.3.r0_log.md>
