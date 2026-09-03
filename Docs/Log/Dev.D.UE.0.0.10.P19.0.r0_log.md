# Dev.D.UE.0.0.10.P19.0.r0 Development Log

## 1. 目标与基线

- 基线提交：`49640b6617d3e27ecc9ee0d21feee8ad086ca131`（P18.10）；
- 分支：`agent/0.0.10-p19-0-divine-sense-scan-contract`；
- 目标：为神识脉冲建立确定性、不可变、消费者安全的纯运行时扫描合同；
- 约束：不做 World 查询、trace、资源消费、输入/UI、目标 mutation、持久化或表现。

## 2. 新增文件

- `Source/ShanmenCombatRuntime/Public/ShanmenDivineSenseScan.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenDivineSenseScan.cpp`；
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenDivineSenseScanTests.cpp`。

没有修改既有产品代码、模块依赖、GameMode、回归映射或测试工具。

## 3. 合同实现

新增 immutable definition、scan request、observation、reveal 和 receipt：

- definition 只接受 canonical Divine Sense action，并冻结范围、容量、遮挡及标签策略；
- request 绑定完整 action、definition、origin 与 ordinal；
- observation 绑定 ScanId、subject、location、tags、LOS 与 authority revision；
- resolver 完成验证、去重、歧义拒绝、过滤、排序和容量裁剪；
- receipt 只暴露通过过滤的 reveals，不携带 rejected evidence。

所有身份通过既有 deterministic ID utility 派生；标签先排序，浮点按位编码，负零规范化。默认/未知/非有限/冲突输入均失败关闭，输出在失败时清空。

## 4. 解析规则

执行顺序固定为：

1. 验证 request 与所有 observation 的不可变身份；
2. 按 subject GUID 幂等去除完全重复证据；
3. 同 subject 的不同证据直接拒绝整轮；
4. 计算 finite distance squared；
5. 执行 self、inclusive radius、required/blocked tags 与 occlusion policy；
6. 按距离、subject GUID 稳定排序；
7. 按 MaximumResults 裁剪并派生 receipt identity。

## 5. 自动化改动

新增 4 条 exact 测试：

- `DefinitionAndRequest`；
- `ObservationEvidence`；
- `FilteringAndVisibility`；
- `DeterminismAndAmbiguity`。

覆盖无效规则、非有限值、canonical zero、重复扫描身份、revision/LOS 身份、self/range/tag/occlusion 过滤、边界距离、空结果、最近优先、重排重放、重复去重、冲突证据和 foreign ScanId。

全量测试从 788 增至 792。

## 6. 测试与门禁证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | Divine Sense exact | 4/0 | `6BD46583611F2A338ED58BA634E823FD791378B6671C4F65375955A0097E8E77` |
| `automation_combat_runtime.log` | `Shanmen.0_0_10.CombatRuntime` | 130/0 | `5415BC095282F18142B9846DC443AD1E274F28765B0B097A5C52C7EB4C0C40F5` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 792/0 | `B0923219C16C2590C173A0068A8BAB9E3D5D0A5BCBD2B28BE3C6D5B88C0343B2` |
| `regression_coverage.log` | changed-path gate | PASS | `20286C404C442FB6E0134AFF45B263643F3C5A7D569D7BED32B65852C5154514` |
| `regression_coverage_selftest.log` | gate self-test | 289/289 | `5D88EF74BC9E6F9FC610DB687385C9B4B16C4FFB869C9D3FE99C3F48A891A7D4` |

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 289/289
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

三个自动化日志均为 Fail=0，且包含原生 `TEST COMPLETE / EXIT CODE: 0`。全量首末 Success 间隔约 32m13.995s。

## 7. 诊断记录

新增源码第一次 Editor 构建即通过，没有编译修正或失败测试。

自动化日志在 Shanmen 用例开始前包含 UE 5.8 自带 `UE::UnifiedErrorTest` 的 13 条条件测试噪声；与 P18.10 日志逐项对照后确认数量、位置和文字相同。它不计入 Shanmen Fail；最终 4/0、130/0、792/0 及三个 native terminal 0 均独立成立。

## 8. 构建与产物

- Editor initial：8 actions，27.33s，Succeeded，log SHA `0627F837A497D211E1BBDC2F30620A6ACF5A3A74BBA4485672E445EA1575A66C`；
- Game final：5 actions，27.47s，native 0，log SHA `1367F48F360538E601F6F4858F5C0A9E8311F440DF3F2BF12A763E89B642571C`；
- Editor final：up to date，0 actions，1.05s，native 0，log SHA `434CDDDD3E5959B5EA624558A2541FFA7103A10D386751FA495E882F2798797C`；
- `demo_map.exe`：356,548,608 bytes，SHA `7183751AD5DA03B399F363D94C2716FD8E892FCF5708FB016267149536BD8693`；
- `UnrealEditor-ShanmenCombatRuntime.dll`：1,922,560 bytes，SHA `6A2F7996B13FB192D9C4B25A54E74B48AAFFF5F4C161C64F67D034CAAD7F756A`。

## 9. P/F 边界

本轮只完成 P 阶段 C++ 值合同、无头自动化、静态边界审计和 Development 构建。没有运行 Editor UI、PIE、Standalone 或游戏可执行文件，也没有执行真实输入、截图、Smoke、Cook、Package。

因此本轮不验证最终范围平衡、SpiritEnergy 消耗、世界 trace、UI 可读性、特效/声音、玩家手感或产品运行闭环。

## 10. 提交边界与后续

计划精确提交 3 个源码文件、本 Report 与本 Development Log。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 和用户文件不修改、不暂存；raw logs 仅保留在 `Saved/Codex/P19.0`。

建议 P19.1 只增加有界 World observation adapter，让世界层采样后调用本轮纯 resolver；不复制过滤权威，也不提前接入资源、输入、UI 或表现。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-0-divine-sense-scan-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-0-divine-sense-scan-contract/Docs/Report/Dev.D.UE.0.0.10.P19.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-0-divine-sense-scan-contract/Docs/Log/Dev.D.UE.0.0.10.P19.0.r0_log.md>
