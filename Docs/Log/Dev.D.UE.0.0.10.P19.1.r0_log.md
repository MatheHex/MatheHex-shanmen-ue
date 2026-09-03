# Dev.D.UE.0.0.10.P19.1.r0 Development Log

## 1. 目标与基线

- 基线提交：`3bafdaf16d5391a02718f08450cdc4e090760a4b`（P19.0）；
- 分支：`agent/0.0.10-p19-1-divine-sense-world-observation-adapter`；
- 目标：把一次显式、有界的 World Actor 样本转换为 P19.0 不可变 observation 与 scan receipt；
- 约束：不做 Actor discovery、trace、资源消费、输入/UI、目标 mutation、持久化或表现。

## 2. 架构决策

适配器放在 `demo_map` 产品组合层。`ShanmenCombatRuntime` 已公开依赖 `ShanmenWorldGameplay`，因此不能让 WorldGameplay 反向引用 P19.0 合同；放入 `demo_map` 避免模块循环，并维持纯 runtime 与 World registry 的原有职责。

未修改 Build.cs 或既有模块依赖。

## 3. 新增文件

- `Source/demo_map/demo_mapShanmenDivineSenseWorldObservationAdapter.h`；
- `Source/demo_map/demo_mapShanmenDivineSenseWorldObservationAdapter.cpp`；
- `Source/demo_map/demo_mapShanmenDivineSenseWorldObservationAdapterTests.cpp`。

同时更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

## 4. 适配器实现

新增同步无状态 `SampleAndResolve`：

- 先验证 action/definition/ordinal、显式 budget、World、registry 和 Run；
- source Actor 必须存活、属于同一 World、具有精确 registry binding，并匹配冻结 action source；
- source transform 采样一次并作为 P19.0 request origin；
- 完整 subject Actor 批次先验证存活、World、注册、唯一 stable entity 和有限位置；
- 批次按 stable entity GUID 排序后，每个对象恰好调用 evidence provider 一次；
- provider 提供 tags、LOS 和 authority revision；
- P19.0 独占 observation capture、过滤、排序、容量裁剪和 receipt 构造。

零预算空集合有效；超预算在 World/evidence 访问前拒绝；任何失败不发布部分 receipt。

## 5. 边界与确定性

调用方拥有候选发现和执行节奏，适配器不枚举 Actor、不重试、不 Tick。两个 Actor 映射到同一实体时，在 provider 回调前失败关闭。

Actor 输入顺序不会改变 provider 顺序或 receipt。成功结果不保留 World/Actor 指针，只暴露状态、诊断、预算、观测数和 P19.0 immutable receipt。

self、range、tag、LOS/occlusion 和 maximum-results 策略没有在 World 层复制，全部委托 P19.0。

## 6. 自动化改动

新增 4 条 exact 测试：

- `CanonicalSample`；
- `PolicyDelegation`；
- `AdmissionAndSourceFences`；
- `SubjectEvidenceFences`。

覆盖 canonical callback order、one-call-per-subject、输入重排、来源 origin、容量、空样本、策略委托，以及 runtime/budget/World/registry/source/subject/evidence 的失败关闭边界。

回归映射新增 `DivineSenseWorldObservationAdapter` 规则，要求：

- `Shanmen.0_0_10.Product.DivineSenseWorldObservation`；
- `Shanmen.0_0_10.CombatRuntime.DivineSense`；
- `Shanmen.0_0_10.WorldGameplay`。

自测从 289 增至 291，包含完整证据通过与缺失依赖证据拒绝。

## 7. 测试与门禁证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | P19.1 exact | 4/0 | `8D70F414CF63C1801D0F2C62508AAC4CCCA0295A5C47A3AFBD2C6B93B18CFC3C` |
| `automation_divine_sense_runtime.log` | P19.0 Divine Sense | 4/0 | `D0F63482A0CCEF6981562AF3F5C01DE6A29DC7523483076F8082ED4C5C56C5F3` |
| `automation_world_gameplay.log` | WorldGameplay | 10/0 | `B23EC3DC43966B791950BF5688CB18D158BBB071B6C572704CA4F9D2739248A0` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 796/0 | `40F877B76501D7850B2AB06E28FAA00C8C8B13A4B7661E2267A310C5A7E8F8CB` |
| `regression_coverage.log` | changed-path gate | PASS | `573FDA436C51C0F6521EB4B779158432C2EB97E7123280951A717C04F11A4C7E` |
| `regression_coverage_selftest.log` | gate self-test | 291/291 | `B0BDF49D9BC3B8B8A0D21D33514B5EB5313BDAFDD76D761129E06BFD422060C5` |

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=3 Logs=3
SELF_TEST: PASS 291/291
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

四个自动化日志均为 Fail=0 且具有 native terminal 0。全量首末 Success 间隔约 34m49.351s。

## 8. 诊断记录

新增源码第一次 Editor 构建即通过，4 条 exact 测试第一次运行即全部通过，没有编译修正或失败测试。

全量运行在既有 SwordRhythm checkpoint/journal 用例处出现约 1–2 分钟的历史性长 delta，同时伴随 `google.com/generate_204` HTTP timeout 警告；测试进程持续响应，这些用例最终全部 Success，后续用例继续执行，最终为 796/0 和 `TEST COMPLETE / EXIT CODE: 0`。因此该现象记录为环境噪声，不改写为产品失败。

## 9. 构建与产物

- Editor initial：7 actions，18.20s，native 0，log SHA `05B36913D2DF1A1ED219946F126A4EF9B80F319D0A1BF31D328CA45FA53CEB34`；
- Game final：4 actions，25.25s，native 0，log SHA `2B855A97F8210B3BBB26FC7B837304117712B8B3F91E5A0595D21C3607449C4C`；
- Editor final：up to date，0 actions，0.94s，native 0，log SHA `2E56E874F8AB8FAC6FBF24137086B245664A8F44432DE6CA11D41993953819B7`；
- `demo_map.exe`：356,595,712 bytes，SHA `F323C56D516728296C80E699A43CEC3F7474B932633495261F141DFF96C97965`；
- `UnrealEditor-demo_map.dll`：15,225,856 bytes，SHA `DEF0CF01E409DE17EC02188CB580C906B51A2475469B7247C6D4B18FE60E0C7B`。

## 10. P/F 与提交边界

本轮只完成 P 阶段 C++ 产品组合适配器、无头自动化、静态边界审计和 Development 构建。没有运行 Editor UI、PIE、Standalone 或游戏可执行文件，也没有执行真实输入、截图、Smoke、Cook、Package。

计划精确提交 3 个新增源码、2 个回归工具文件、本 Report 与本 Development Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 和用户文件不修改、不暂存；raw logs 仅保留在 `Saved/Codex/P19.1`。

下一轮先审计现有 action/resource authority，再决定 P19.2 的神识产品脉冲协调边界；不让本适配器隐式发现 Actor，也不提前接入输入、UI 或表现。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-1-divine-sense-world-observation-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-1-divine-sense-world-observation-adapter/Docs/Report/Dev.D.UE.0.0.10.P19.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-1-divine-sense-world-observation-adapter/Docs/Log/Dev.D.UE.0.0.10.P19.1.r0_log.md>
