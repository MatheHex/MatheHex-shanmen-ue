# Dev.D.UE.0.0.10.P27.4.r0 Development Log

## 1. 目标

- 在 P27.2/P27.3 唯一阵法 Run 生命周期前增加无状态、设备无关输入适配层；
- 让调用方的一次事件身份与一次不可变样本稳定派生 Formation Intent/Anchor Attempt 身份；
- 在任何采样前执行 Gameplay、路由、Run 与事件身份门，并最多一次委托生命周期；
- 保留既有 Controller/Lifecycle/Host 的幂等、物品与世界权威，不建立第二套状态；
- 完成改动驱动回归、Report/Log 提交和 GitHub 推送。

## 2. 基线与范围

- 基线：`afe7ffd5966dea6a53c5a5bf2a87bd97aa018926`（P27.3）；
- 分支：`agent/0.0.10-p27-4-formation-input-adapter`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不改阵法定义、材料数量、效果、范围、持续时间、世界投放、物品 schema、地图或资产；
- 不接 GameMode、Pawn、Controller、Widget、Enhanced Input、Tick 或真实设备；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. 不可变输入样本

新增 `Fdemo_mapShanmenFormationStartInputSample`：通过 `TryCapture()` 冻结有效 Diagram、有限 Origin
和有限非零 Forward。Forward 在边界投影到 XY 平面并归一化，字段保持私有且只读。

新增 `Fdemo_mapShanmenFormationAnchorInputSample`：通过 `TryCapture()` 冻结非空
AnchorDefinitionId。适配器只调用传入的采样回调一次，不自行读取设备、相机、World 或 Actor。

## 4. 身份与路由

`Fdemo_mapShanmenFormationInputAdapter` 提供：

1. `MakeIntentId(RunId, InputEventId)`，命名空间
   `demo_map.Formation.StartInputIntent.r1`；
2. `MakeAnchorAttemptId(RunId, InputEventId)`，命名空间
   `demo_map.Formation.AnchorInputAttempt.r1`；
3. `RouteStartInput()`，构造既有 `Fdemo_mapShanmenFormationIntent` 后一次委托 `TrySubmit()`；
4. `RouteAnchorInput()`，构造既有 `Fdemo_mapShanmenFormationAnchorOperation` 后一次委托
   `TryExecuteAnchorOperation()`。

适配器不缓存结果；同一事件重放仍进入唯一生命周期，由既有 ledger 返回稳定回执。

## 5. 前置门与审计结果

固定顺序为：Gameplay input allowed -> lifecycle route available -> valid Run -> valid input event -> one sample
-> immutable intent/operation capture -> one lifecycle invocation。

启动与锚点结果分别记录状态、`SampleCount`、`LifecycleInvocationCount`、输入/Run/派生身份、冻结样本、
既有契约、下游原始结果和诊断。`IsValid()` 重算派生身份并交叉校验完整封装；前置失败必须没有后续
证据，Applied 必须拥有一致的下游成功证据。

## 6. 测试结果

新增 3 项适配器测试：

- `DeterministicIdentityAndPreflight`；
- `StartReplayAndConflict`；
- `AnchorReplayAndLifecycleFence`。

测试使用真实隔离 Profile、ItemAuthority、CombatRunCoordinator、FormationRunLifecycle 和无头 World。
它们证明前置拒绝不采样/不路由、身份跨命名空间稳定、启动 exact replay 只消耗一个序列、载荷漂移
冲突失败、锚点 exact replay 不改变物品快照、两个锚点进入 Active、正常 End 移除两个 Actor，以及
生命周期结束后拒绝新输入。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationInputAdapter` | 3 | 0 | `F1531489FE8A7FDDDC80DB6AAA473523812E6E707748951F46B8219C03B6A660` |
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 4 | 0 | `74FB3A9332E02B01B4D3099FD05DA1C55594388CF9C0C9A07A9C455002F8E576` |
| `Shanmen.0_0_10` | 1,331 | 0 | `72B31DCB72CDAF0B91144C1500AA760BEF620B3671D6224EFF9F99D644DE0FD4` |
| `demo_map.V3.Attributes` | 4 | 0 | `ADB2818BE4A7FC698228FE05395C70B41BF5DE60E7EB8E78B1306374E32321D2` |

所有测试进程原生退出 0；日志为 0 Fail、0 Fatal/Unhandled/Ensure/Assertion，并包含 UE 5.8 原生
队列完成证据。

## 7. 改动驱动覆盖

新增 `FormationInputAdapter` 路径映射，要求完整 0.0.10、适配器、生命周期、Controller、产品权威、
Combat Run、Host、Session、材料、Items、WorldGameplay、FormationDeployment 与 CombatCore 证据。
因为共享测试 fixture 位于 `FormationProductHostTests.cpp`，既有 ProductHost 规则同时命中。

最终门禁：`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=40 Logs=4`；SHA-256
`49D1A6C406DA4C48C032F06BEDE37999DFD6F02C29B8DF1FAA938D019359F833`。

映射器新增一项正例和一项仅聚焦证据不足的反例。自测 `465/465` PASS；SHA-256
`06E12DACF499A261B2E8EA4BB1BCCE82251CD49D60094C3D91BCA0AB5310DAE5`。

## 8. 构建与二进制

| Target | Result | Duration | Log SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 24.51s | `0756FBD1F382DD635C8BDD3492F731FF238F9949637366F2BD52FFB889222B51` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 1.14s | `DCF5C61FED637CCD0352D1631463C4BD4569E6152026FC25B3DC0A3D3D2883F0` |

二进制：

- `demo_map.exe`：360,090,112 bytes；SHA-256
  `E58BFD54E27A81E04C1537F96C2B801005660973F4250804EB8C6B3152103BEB`；
- `UnrealEditor-demo_map.dll`：19,427,840 bytes；SHA-256
  `C136754454AF0CC629C39997C88044A11155E1AFF93EAD45A153133A6C4CBA56`。

## 9. 静态边界与下一步

`git diff --check`、regression map JSON 解析均 PASS。适配器 `.cpp` scoped scan 未发现 `UWorld`、
Actor/Pawn/Character、GameMode、Spawn/Destroy/扫描、Enhanced Input、InputAction、Tick、键盘轮询、RNG、
`ApplyDamage`、Inventory、Sequence 或 Retry。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
下一阶段可在既有统一物理输入注册表内增加一条明确阵法动作，并只把稳定事件身份及采样回调交给
本适配器；不得绕开 RunLifecycle，也不得让物理输入持有材料或世界投放权威。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationInputAdapter.h`
2. `Source/demo_map/demo_mapShanmenFormationInputAdapter.cpp`
3. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.4.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.4.r0_log.md`

`Saved/Codex/P27.4` 原始证据不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-4-formation-input-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-4-formation-input-adapter/Docs/Report/Dev.D.UE.0.0.10.P27.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-4-formation-input-adapter/Docs/Log/Dev.D.UE.0.0.10.P27.4.r0_log.md>
