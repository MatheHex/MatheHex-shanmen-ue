# Dev.D.UE.0.0.10.P6.12.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.12.r0`；
- 基线提交：`e7efa4df1457bbe5e6f7d7b8ba50ab7dc3d50541`（P6.11）；
- 分支：`agent/0.0.10-p6-12-orbit-threat-policy-receipts`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

人工规划把“飞剑环绕”定义为攻击准备、近身威胁、防御准备与发射准备，但没有冻结半径、频率、伤害或防御数值。P6.11 已完成几何候选回执，下一最小闭环因此不是立即造成效果，而是让既有飞剑 Definition 的目标规则显式消费完成回执。

代码结构复审要求几何、目标合法性、Impact/效果和资源保持分层。本阶段只完成“几何 receipt + target tags evidence -> policy receipt”，不跨入 effect 或 authority mutation。

## 契约设计

### 复用唯一策略

ControlledWeapon Definition 已有 `RequiredTargetTags` 和 `bRejectSelf`，Directed 路径也使用它们。本阶段没有引入 faction bool、hostile enum 或另一份 target definition；Orbit policy 使用同一冻结值。

### 精确证据闭合

调用方提供 `TargetEntityId + TargetTags`。Runtime 要求 evidence 数量与 emission candidate 数量相同、ID 全部有效且唯一，并逐个匹配 canonical candidate。输入排列不影响结果；任何 missing、duplicate 或替换成 extra ID 都失败关闭。

### 可审计决策

逐目标结果只有三个显式状态：`Accepted`、`RejectedSelf`、`RejectedMissingRequiredTags`。总 receipt 保存原 geometry receipt、required tags、自目标规则和完整决策数组；私有字段与 `IsValid()` 防止调用方伪造不一致结果。

### 时序和副作用

策略只允许同 action、同 detector、同 exact source item、已结束 emission 且仍处于 Orbiting 时执行。exact replay 不改变 state、ordinal、command sequence 或 impact ledger；空 sample 是合法的 0 项 receipt；Launch 后旧 Orbit receipt 失败关闭。

### 产品透传

Session、Controller 与 Run Host 只做 const delegate。Host 继续按 exact `ItemInstanceId` 找 Controller；未知 item 不可借用另一把剑的 receipt。没有在任何产品层缓存 target evidence 或 policy result。

## 自动化覆盖

### Runtime

- B/A/self 的几何候选与 B/A/self 反序 target evidence；
- canonical join 与三种 decision；
- missing / duplicate evidence fail closed；
- exact replay 与 impact ledger `0`；
- 空 sample 形成有效 0 项 receipt；
- Launch 后旧 receipt 不可消费。

### Product / Host

- Controller 输出 Living target accepted receipt，同时 vitality 不变；
- Host exact item policy 路由；
- unknown item 无法消费 receipt；
- policy 完成后既有 Launch / Directed 路径继续工作。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p612_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 6 | 0 | 0 | `9E75688FAF12E79A52B6E018D8331A1DCB965F24EC1E21389B82AFF7A76A312E` |
| `p612_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 29 | 0 | 0 | `0BD6357D6909094CC83548AE71CB5D02575FE69A233F25BD701C78E3421455BD` |
| `p612_full.log` | `Shanmen.0_0_10` | 160 | 0 | 0 | `36B89CDA9F25D3B581ACAE409E0513646A1F106E0BAB6A199BC23BA17CFD0BD5` |

最终日志位于 `Saved/Automation/P612/`。每份日志有一个实际 RunTests 命令、一个 queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与 native exit `0`。

三份日志在测试发现前各包含 13 条既存 `LogAutomationTest: Error: Condition failed` 诊断噪声；目标测试随后均以 Success 完成。该噪声是已知的测试发现信噪比债务，本阶段未扩大范围修复，也没有在 Report 中隐去。

## Changed-file gate

源码与测试完成后：

```text
REGRESSION_COVERAGE: PASS Changed=11 Rules=4 Required=11 Logs=3
```

完整 `Shanmen.0_0_10` 覆盖 CombatRuntime 与 ControlledWeapon Product 链映射出的 11 个必跑组；Runtime 和 Product focused 日志提供直接证据。regression coverage self-test：`14/14 PASS`。

加入 Report / Log 后最终 staged gate：

```text
REGRESSION_COVERAGE: PASS Changed=13 Rules=4 Required=11 Logs=3
```

## 构建

首次 Editor integration build：

- `42/42` actions；
- `Result: Succeeded`；
- native exit `0`；
- `158.51s`。

首次 Game build：

- `39/39` actions；
- `Result: Succeeded`；
- native exit `0`；
- `135.51s`。

补入空 sample 回归断言后，最终增量 Editor build 为 `4/4`、`9.80s`、exit `0`；最终增量 Game build 为 `3/3`、`11.28s`、exit `0`。没有失败、内存错误、外层超时或源码重试；Game executable 未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- Source 新增 `512` 行，其中生产新增 `328` 行；
- 新增 Source 行的 Damage / Impact / World query / Spawn / RNG / inventory Commit 扫描命中 `0`；
- Runtime 边界扫描 `demo_map / UWorld / AActor / world query / RNG` 命中 `0`；
- 最新源码 UTC `2026-08-29T02:43:09.2431378Z`；
- Editor Runtime DLL UTC `2026-08-29T02:43:27.8996863Z`；
- Game executable UTC `2026-08-29T02:45:17.5007499Z`；
- 未修改 target policy 默认值、GameplayTags、Profile schema、存档、item authority、input 或资产；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

只有在产品明确目标标签来源、采样 cadence、每目标冷却与输出效果后，才把 accepted policy receipt 转换为威慑、防御或伤害 intent。P6.12 不预设这些仍未冻结的规则。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-12-orbit-threat-policy-receipts/Docs/Report/Dev.D.UE.0.0.10.P6.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-12-orbit-threat-policy-receipts/Docs/Log/Dev.D.UE.0.0.10.P6.12.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-12-orbit-threat-policy-receipts>
