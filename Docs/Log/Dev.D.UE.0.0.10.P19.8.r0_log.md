# Dev.D.UE.0.0.10.P19.8.r0 Development Log

## 1. 目标与基线

- 基线：`928abd8fcac57a23ffbf9e469ff448e04bc309db`（P19.7）；
- 分支：`agent/0.0.10-p19-8-divine-sense-product-route`；
- 目标：建立一个 device-independent、Run-scoped Divine Sense Product Route，组合 P19.7 authority 与 P19.6 Controller；
- 约束：不接物理输入、UI、隐式 Actor discovery、持续 Tick/Timer、存档或最终数值。

## 2. 设计审计

审计了 P19.1 runtime、P19.2 pulse coordinator、P19.3 host、P19.4 command router、P19.5 session、P19.6 Controller、P19.7 authority，以及 Spirit Evasion 与 Weapon Guard 的 product route。

P19.6 已经是唯一 Controller、intent journal、resource transaction 与 lifecycle 所有者；P19.7 已经是唯一 config 与 Run activation identity 来源。因此 P19.8 采用无状态 route，而不是再建一个长期对象。路由只排序现有权威调用并关闭上游注入入口。

Combat Run 当前不拥有 SpiritEnergy action-resource balance。`TryBegin` 因而继续接收既有权威产生的 opening snapshot；把余额复制进 Coordinator 会制造第二资源真值，本轮明确没有这样做。

## 3. Product Route API

`TryBegin(Controller, Coordinator, OpeningSpiritEnergy, Diagnostic)` 内部创建 canonical config。调用方不能传 config。

`TryUse(Controller, Coordinator, World, SourceActor, SubjectActors, Provider)` 中唯一的产品意图是“use”。World 与 subject batch 是既有 P19.2-P19.6 同步适配链要求的显式现场输入，不携带产品策略或稳定身份。

`TryRetry(..., Attempt, ...)` 只接受 route-issued attempt，不调用 P19.7 `PrepareIntent`，因此不会消耗新 activation sequence。

`TryEnd(Controller, Coordinator)` 读取 Controller 自身 ID 后委托 teardown；公共 API 不接受 ExpectedControllerId。

## 4. Preflight 顺序

新 use 的执行顺序固定为：

1. 验证 active/valid/canonical Controller；
2. 验证 ready Coordinator 及 exact Run/player binding；
3. 捕获 Controller availability，并检查 capacity 与 SpiritEnergy；
4. 验证 World/source/subject batch 的 live、World、finite location、registry、unique 与 self fences；
5. P19.7 创建 canonical config、Run reservation 与 Intent；
6. route 捕获 immutable attempt；
7. P19.6 Controller 捕获 command 并 route。

步骤 1–4 不修改 Coordinator 或 Controller。只有全部通过后才允许 P19.7 推进 Run sequence。Gameplay tags 与 LOS 不在 preflight 读取，仍由 evidence provider 与既有 World adapter 负责。

## 5. Attempt provenance

`Fdemo_mapShanmenDivineSenseProductUseAttempt` 保存：

- ControllerId；
- RunId；
- source entity；
- canonical ConfigId；
- P19.7 prepare result（config、reservation、Intent）。

字段为 private，只有 route friend 可写。`IsValid` 重新检查 canonical config、reservation config、Run/owner/source 与 Intent binding；`Matches` 再比较 activation sequence、ActivationId 和完整 Intent。

跨 Controller/Run attempt 在调用 Controller/provider 前被拒绝。Caller 能保存和复制 proof，但不能逐字段重写产品身份。

## 6. Retry 与 replay 语义

第一次 provider rejection 发生在 P19.6 已冻结 command 之后，所以 route result 保留有效 attempt 与 structured Controller rejection。修复 provider 后：

- 同一 IntentId；
- 同一 ActivationId；
- 同一 RouteCommandId；
- Coordinator sequence 不再推进；
- SpiritEnergy 从 100 只降到 90 一次。

`TryRetry` 不做 fresh availability 或 live preflight。这样暂时错误可用修正后的现场输入重试，同时不会破坏 P19.6 的 accepted replay：已完成 attempt 可在 null World/source、空 subjects 与拒绝 provider 下直接读取 journal proof。

## 7. 测试开发

新增 6 项 exact contract：

- `CanonicalBegin`：unready fence、canonical-only begin、exact idempotence、active reset fence；
- `UseAndReplay`：一次 use、资源/序列变化与无现场读取 replay；
- `PreflightFences`：null World、错误 source、自目标、重复、未注册、跨 World、超预算全部在身份前拒绝；
- `RejectedRecovery`：provider 拒绝后复用 frozen attempt 恢复；
- `AvailabilityFence`：余额不足与 16 次容量耗尽均不消费第 17 个身份；
- `CrossBindingAndTeardown`：跨 Controller/Run 拒绝与 caller-free identity teardown/replay。

首次 Editor Development 构建：5 actions，21.15 秒，native 0。

## 8. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `automation_exact.log` | 6/0 | `21E6082E2FC1543380EAF8934D123E0DA0C8EF94B19AFD5DBE90A821C292E77E` |
| `automation_full.log` | 828/0 | `2A480D06E3A8DC4EBB19D7C2CBF276EA94D138072DCB89C928D6ABCAA774002E` |
| `automation_legacy_attributes.log` | 4/0 | `77E0DC96465CC3F5997BF41E90A39F96766DC14FE77BEA126A1CC125772085A8` |
| `automation_legacy_enemy_skill.log` | 44/0 | `A512410C4CF8C543A7F618381DDA8A224C27D66CD223094E50EA422065801D20` |
| `automation_legacy_v2_ranged.log` | 22/0 | `9637A24E0E48DA1888D54E918A6BEB50B3E761C2FE3545B1FFDBA307032BF531` |
| `automation_legacy_item_armor.log` | 46/0 | `7AC9A524A19D38CFE84B8A550C3C75E7D22334BF4803DEBCC5E97654CCBEBB22` |

每份日志均通过唯一 command、Success 非零、Fail 0、terminal 存在、Fatal/Unhandled/Ensure 0 检查：

```text
EVIDENCE_AUDIT: PASS Logs=6 RecordedSuccess=950
```

审计日志 SHA-256：`694EAC8D443E270DF9BE09CD6E21760387D130F2BE056AE4C3B08A849F26D3D6`。

## 9. Changed-file regression gate

新增 `DivineSenseProductRoute` mapping，要求 Product Route、Authority、Controller、Session、Command Router、Host、Pulse Coordinator、World Observation、Combat Run、Divine Sense runtime、Action Resource、Action Lifecycle 与 WorldGameplay。

真实 gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=13 Logs=6
```

- gate SHA-256：`60318BF25B35F883C76A7ABE323FBE3579963EDBD45938D1FF1DF2B9790CF248`；
- self-test 新增一正一反后：`305/305 PASS`；
- self-test SHA-256：`0B55D8F60ADE6CC8EAB1A6C9F9451138CE84E05E46931E02B712092B721CC5F6`。

## 10. 静态边界与构建

生产 route header/cpp 扫描禁止 input component/binding、PlayerController、Widget、Actor enumeration、trace/sweep/overlap、spawn/destroy、RNG、Timer/Tick：

```text
BOUNDARY_SCAN: PASS Files=2 Matches=0
```

boundary log SHA-256：`E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`。`git diff --check` native 0。

构建：

- Editor initial：5 actions / 21.15s / native 0；
- Game final：4 actions / 23.76s / native 0；
- Editor final：up to date / 1.04s / native 0。

产物：

- `demo_map.exe`：356,955,136 bytes / `E6D5792565A48FCD3FFE1826B5A3011717E012179271D8788CDA325FA2706423`；
- `UnrealEditor-demo_map.dll`：15,652,864 bytes / `73E3CF4016A14A873F436D64E1CC2C68D059F9236C12A1FE62EBEB18A0513563`。

## 11. 异常与 P/F 边界

全量运行在既有 Sword Rhythm checkpoint/envelope 段出现 16–97 秒 large-delta 提示。只读进度检查确认 Success 从 648 持续增长、Fail 始终为 0，进程 Responding 且 CPU 时间持续增加。相同区域在 P19.7 全量日志中也存在长运行；本轮最终用约 33 分 46 秒完成 828/828，native 0、唯一 queue-empty terminal。

没有中断、重跑、缩小范围或用部分结果替代完成证据。raw logs 只保存在 `Saved/Codex/P19.8`。

本轮没有运行 Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

提交只包含 3 个新 route 文件、2 个回归流程文件、本 Report 与本 Log。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未修改、未暂存。

下一阶段建议 P19.9：logical input adapter 将一次逻辑 use event 交给本 route，并保留 immutable retry attempt；仍不绑定物理按键、不枚举 Actor、不接 UI。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-8-divine-sense-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-8-divine-sense-product-route/Docs/Report/Dev.D.UE.0.0.10.P19.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-8-divine-sense-product-route/Docs/Log/Dev.D.UE.0.0.10.P19.8.r0_log.md>
