# Dev.D.UE.0.0.10.P6.11.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.11.r0`；
- 基线提交：`318ff8e1153e32a61a16c3251a43674d05ec5549`（P6.10）；
- 分支：`agent/0.0.10-p6-11-deterministic-threat-receipts`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.10 已把显式 Orbit overlap 转成统一候选，但 End 只清空 detector session。调用方若要在窗口关闭后审计或消费候选，只能自行缓存每次 projection result；这会在唯一去重 authority 之外形成第二份状态，并使不同目标的回调排列泄漏到后续策略。

P6.11 因此只补 emission completion receipt：候选仍由唯一 session 接受和去重，End 时一次性输出规范排序的完整证据。目标合法性与候选效果仍等待显式产品策略。

## 契约设计

### 唯一候选 authority

`FShanmenDetectorEmissionSession` 继续拥有 Begin / Accept / End、ordinal 与每目标去重。原 `TSet<FGuid>` 替换为 `TMap<FGuid, FShanmenHitCandidate>`，键仍是同一 TargetEntityId；没有在 Runtime 或 Product 新增另一份持久缓存。

### 完成回执

`FShanmenDetectorEmissionReceipt` 的字段私有，只读 getter 暴露冻结 Context 与候选。`IsValid()` 要求每个候选与 Context 的 Activation、Source、Detector、Kind、ordinal 完全一致，并要求 TargetEntityId 严格递增，因此同时证明规范顺序与无重复。

End 先构造并自校验回执，再清空 active state 并递增原 ordinal。构造失败时 session 不提交部分状态。旧无参数 End 仅忽略同一新回执，不保留第二条实现路径。

### 确定性边界

规范排序采用项目既有的 GUID Digits 序，与 Run Host 的 exact-item 排序习惯一致。它只消除不同 TargetEntityId 的回调排列差异；同一目标仍保留第一个被唯一 session 接受的几何样本。

### 产品透传

Orbit receipt 经 ControlledWeapon Execution、Product Session、Controller、Run Host 原样返回。各层沿用 copy-validate-commit：下层 End、receipt validity 与层内 state validity 全部成立后才替换 owner 状态。Context Action 已携带 exact SourceItemInstanceId，不新增包装 identity。

## 自动化覆盖

### WorldGameplay

- B/A 与 A/B 两种跨目标回调顺序；
- receipt Context ordinal；
- strict GUID order 与无重复；
- 两次重放输出相同 TargetEntityId 序列；
- 后续 emission 继续单调递增。

### Runtime

- Orbit 接受两个目标、拒绝重复；
- End 返回 exact item、两个规范候选；
- ImpactLedger 保持 0；
- 后续 Directed emission 使用下一个共享 ordinal 并正常结算。

### Product / Host

- Controller receipt 保留 exact target 与 source item；
- Host exact-item End 返回同一 canonical target；
- 完成 receipt 后既有 Launch 与 Directed delivery 路径保持通过。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p611_world.log` | `Shanmen.0_0_10.WorldGameplay.DetectorEmissionSession` | 1 | 0 | 0 | `3AC98284FDACE49E8023D3A915647B529AA6BF25965878258A05485C3B1F95E1` |
| `p611_runtime.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 5 | 0 | 0 | `D612405CB70E8A26066AF56F5010DC40EC399D68DFDA6A662272B6B3B10813B9` |
| `p611_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 29 | 0 | 0 | `8CAE8866C2E053D917F3DBCDD087CF629215B8D38D00CF9EA5C93C3ACF3406C6` |
| `p611_full.log` | `Shanmen.0_0_10` | 159 | 0 | 0 | `34EE75CE33BC76DB16852619ECE00C5497521888080A07B5B644121890653AA0` |

日志位于 `Saved/Automation/P611/`，各有一个实际 RunTests 命令、queue-empty、Fail 0、Fatal / unhandled / ensure 0 与原生退出 0。

## Changed-file gate

生产与测试源码完成后：

```text
REGRESSION_COVERAGE: PASS Changed=14 Rules=5 Required=11 Logs=1
```

完整 `Shanmen.0_0_10` 覆盖 WorldGameplay、CombatRuntime 及 ControlledWeapon Product 链映射出的 11 个必跑组；另外三份 focused 日志提供直接证据。regression coverage self-test：`14/14 PASS`。

加入 Report / Log 后最终 staged gate：

```text
REGRESSION_COVERAGE: PASS Changed=16 Rules=5 Required=11 Logs=1
```

## 构建

Editor：

- `52/52` actions；
- `Result: Succeeded`；
- native exit `0`；
- `178.18s`。

Game：

- `47/47` actions；
- `Result: Succeeded`；
- native exit `0`；
- `154.58s`；
- 只生成 executable，未启动。

没有首次失败、内存错误、外层超时或源码重试。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- Source 新增 `251` 行，其中生产新增 `190` 行；
- 新增生产路径未调用 World query、Spawn、Damage/Impact、Vitality、库存事务、RNG 或 input；
- 最新源码 UTC `2026-08-29T02:01:00.3894407Z`；
- Editor DLL UTC `2026-08-29T02:04:29.7566238Z`；
- Game executable UTC `2026-08-29T02:09:59.3663925Z`；
- source 早于两个最终构建产物；
- 未修改 Profile schema、存档、item definition、GameplayTags、input mapping 或资产；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

下一阶段可让显式 content / product policy 消费这份规范候选回执，但必须先冻结采样来源、半径、频率、目标合法性、每目标冷却及输出效果。P6.11 不预设这些规则，也不把 receipt 自动转换为威慑、防御或伤害。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-11-deterministic-threat-receipts/Docs/Report/Dev.D.UE.0.0.10.P6.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-11-deterministic-threat-receipts/Docs/Log/Dev.D.UE.0.0.10.P6.11.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-11-deterministic-threat-receipts>
