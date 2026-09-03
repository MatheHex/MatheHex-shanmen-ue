# Dev.D.UE.0.0.10.P19.3.r0 Development Log

## 1. 目标与基线

- 基线提交：`f23147ca2fed77dd498334583260955383ab2b72`（P19.2）；
- 分支：`agent/0.0.10-p19-3-divine-sense-product-host`；
- 目标：建立 Run-scoped 神识 Product Host，持有唯一 SpiritEnergy authority 与 P19.2 pulse coordinator，并以不可变 command/结果完成产品层原子发布；
- 约束：不做 Actor discovery、持续 Tick、Timer、物理输入、UI/表现、目标 mutation、存档或数值平衡定案。

## 2. 设计审计

先审计既有 `SpiritEvasionProductHost`、`SwordQiRunHost`、`FormationProductHost`、`WeaponGuardProductSession`、P19.2 pulse coordinator 与 `FShanmenActionResourceAuthority`。

结论是 P19.3 不应新增资源 ledger，也不应让调用方继续持有可写 authority。Host 因此直接拥有既有 authority 与 coordinator，并保留 opening snapshot；外部只可读取余额、revision、capacity、processed count 和只读 coordinator。资源 amount 仍由 command 的内容 cost 给出，Host 只固定 channel 必须为 SpiritEnergy。

## 3. 实现文件

新增：

- `Source/demo_map/demo_mapShanmenDivineSenseProductHost.h`；
- `Source/demo_map/demo_mapShanmenDivineSenseProductHost.cpp`；
- `Source/demo_map/demo_mapShanmenDivineSenseProductHostTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

## 4. Host 打开与一致性

`TryOpen` 验证 Run/source、资源状态、revision、capacity 及剩余 revision 空间，随后创建 SpiritEnergy authority、opening snapshot 和 P19.2 coordinator。HostId 由 coordinator ID 与 opening snapshot ID 确定性派生。

Host invariant 要求：

- opening/current authority 的 owner、channel、maximum 一致；
- opening reserved 为零，当前无 pending reservation；
- resource transaction 数等于 processed pulse 数；
- 当前 revision 等于 opening revision 加 `2 * processed count`；
- HostId 可重新派生。

## 5. Command 与结果证明

`Fdemo_mapShanmenDivineSensePulseCommand` 捕获完整 action、definition、cost、ordinal 与 Actor budget。CommandId 绑定完整 action identity 与规范化 tags，而非只绑定 ActivationId；definition/cost 通过其自校验 ID 进入身份。

`Fdemo_mapShanmenDivineSenseHostPulseResult` 携带 HostId、RunId、command、前后资源 snapshot 和嵌套 P19.2 result。实现后第一次 Editor 编译和 4 条 exact 测试均通过。随后代码自查发现产品结果还能更强地证明跨 Run/source 拒绝，因此补入 Host RunId，并让 `IsValid()` 对成功、RunMismatch 与 SourceMismatch 分别复核事实；再次编译和 exact 仍为 4/0。全程没有失败测试或产品崩溃。

## 6. 原子执行与重放

新 command 在 Host 副本中执行。P19.2 资源提交、World observation、动作完成和 processed receipt 全成功，且候选 Host/结果自校验通过后，才替换正式 Host。

P19.2 拒绝时，正式 Host 不变并返回相同前后 snapshot；嵌套错误保留。Exact replay 在副本上验证既有资源证明后返回 stored receipt，不访问 World/provider，也不改变正式 Host。changed payload、capacity、资源不足、World evidence、Run/source 与 budget 边界均有自动化覆盖。

## 7. 自动化与回归映射

新增 4 条：

- `OpenAndApply`；
- `ReplayAndCapacity`；
- `RollbackAndRecovery`；
- `AdmissionFences`。

回归映射新增 `DivineSenseProductHost` 规则，要求：

- `Shanmen.0_0_10.Product.DivineSenseProductHost`；
- `Shanmen.0_0_10.Product.DivineSensePulseCoordinator`；
- `Shanmen.0_0_10.Product.DivineSenseWorldObservation`；
- `Shanmen.0_0_10.CombatRuntime.DivineSense`；
- `Shanmen.0_0_10.CombatRuntime.ActionResource`；
- `Shanmen.0_0_10.CombatRuntime.ActionLifecycle`；
- `Shanmen.0_0_10.WorldGameplay`。

新增完整证据 PASS 与 focused-only FAIL 两个 fixture，自测从 293 增至 295。

## 8. 测试与门禁证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | P19.3 exact | 4/0 | `9ABE55D9DF839433A6FF553B8D7EF050DA10A3C96306683EC045A117A311CF95` |
| `automation_pulse_coordinator.log` | P19.2 coordinator | 4/0 | `A4EBB240D7C755DA2C3EE69D298FF648B399345E578FC477D44438B1534C7BB5` |
| `automation_world_observation.log` | P19.1 World observation | 4/0 | `CD2E84C5D558BD4DD4D71FE27CE6C8328CD41F000FE9ADF6BA8929C72DA6A3EF` |
| `automation_divine_sense_runtime.log` | P19.0 Divine Sense | 4/0 | `2E9F06BAC1DF43A727D2C56900BCF9FC9329BAF94EAD357553EAD5C442438DFA` |
| `automation_action_resource.log` | ActionResource | 7/0 | `B05D19032F2172F1E807ADB2051AFCA1375EFFBCD86E581C70FFB7C4F480095A` |
| `automation_action_lifecycle.log` | ActionLifecycle | 1/0 | `647BEF5FF73284D1FC09D5F3349EE9477F423A2EF8808F92F9A09B6D34CEEEB1` |
| `automation_world_gameplay.log` | WorldGameplay | 10/0 | `8403D3C9F555F574D5ED4E385C8A9A17F68A2C92EE56054F662816C90F6DFA08` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 804/0 | `94FF12DECD5B3F74D83B3F42A81542DD4D61ABE86569FADD4EB64BFB7513F3A5` |
| `regression_coverage.log` | changed-path gate | PASS | `AEDFEFD4736F1A999E557A4F29B16CF8845C66B349DA75FEC95BF99C3DE9D060` |
| `regression_coverage_selftest.log` | gate self-test | 295/295 | `62B2317F10C2EE326F2B3BE71D6E7A9A165C9B5A4114E17BC55B1C8C4A1E1FAE` |

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=7 Logs=7
SELF_TEST: PASS 295/295
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

八个自动化日志均为 Fail=0，具有 UE 5.8 原生 terminal 0，且没有 Fatal/Unhandled/Ensure 标记。全量首末 Success 间隔约 31m49.533s。期间日志出现引擎后台 `google.com/generate_204` 网络探测超时，但测试控制器持续推进、无测试失败、无致命标记，未把无关联网告警误报为产品失败。

## 9. 构建、诊断与产物

- Editor integration：5 actions，6.17s，native 0，log SHA `25330133D62CDFEBEB58375C7148DEECFEBDCA67211F62D4D3217EA3FCE846E1`；
- Game final：4 actions，21.86s，native 0，log SHA `CA7FC367EED4CCCBA00AF68BD1C6251E69C6B41B7C5DB806D04204CF368A2703`；
- Editor final：up to date，0 actions，1.00s，native 0，log SHA `2884985F910331B307EA7E6B60E7EDDC833BDFD6629B6B267669EFBB01CF9556`；
- `demo_map.exe`：356,706,816 bytes，SHA `FAB98E43ACA5C1DE67E48BF018AC885E15ACA9B368EDB9F576E3702994BDC2D9`；
- `UnrealEditor-demo_map.dll`：15,361,024 bytes，SHA `2F2665D1D0DA0CDC62241848AEE14C5BF6979FAA17ADD14745614F5BCA6D05C2`。

生产 Host 边界扫描为 `Files=2 / Matches=0`。所有 Actor 创建只存在于 transient 自动化 fixture；生产 Host 仅把显式 Actor 参数转交 P19.1。

## 10. P/F 与提交边界

本轮只完成 P 阶段 C++ 产品 Host、无头自动化、静态边界审计和 Development 构建。没有运行 Editor UI、PIE、Standalone 或游戏可执行文件，也没有执行真实输入、截图、Smoke、Cook、Package。

计划精确提交 3 个新增源码、2 个回归工具文件、本 Report 与本 Development Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 和用户文件不修改、不暂存；raw logs 仅保留在 `Saved/Codex/P19.3`。

P19.4 建议接入显式 command router，把上层已准备的动作事件和显式 Actor 集合路由至本 Host；仍不接物理输入、UI/表现或隐式 Actor discovery。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-3-divine-sense-product-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-3-divine-sense-product-host/Docs/Report/Dev.D.UE.0.0.10.P19.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-3-divine-sense-product-host/Docs/Log/Dev.D.UE.0.0.10.P19.3.r0_log.md>
