# Dev.D.UE.0.0.10.P19.1.r0 Report

## 1. 结论

P19.1 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P19.0 的纯神识扫描合同接到一次显式、受限的 World 采样：调用方提交一个已经选定的 Actor 集合和非负预算；适配器验证 World、Run、registry、来源与完整对象批次，按稳定实体 GUID 规范化采样顺序，通过注入的只读 evidence provider 获取标签、视线与权威 revision，最后把不可变 observation 交给 P19.0 resolver。结果不保留 UObject 指针，也不复制过滤权威。

```text
P19.1 exact:                         4 Success / 0 Fail
P19.0 Divine Sense dependency:       4 Success / 0 Fail
WorldGameplay dependency:           10 Success / 0 Fail
Shanmen.0_0_10 full:               796 Success / 0 Fail
Regression coverage:                PASS (Changed=5 / Rules=1 / Required=3 / Logs=3)
Regression gate self-test:           PASS 291/291
Boundary scan:                       PASS (Files=2 / Matches=0)
Game + Editor Development:           PASS / native status 0
```

没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件或真实输入；没有执行截图、Smoke、Cook 或 Package。本轮不宣称玩家输入、SpiritEnergy 消耗、自动 Actor 发现、真实 trace、UI、声画表现或产品闭环已经完成。

## 2. 模块位置与依赖方向

适配器实现于 `demo_map`，而不是 `ShanmenWorldGameplay`：现有 `ShanmenCombatRuntime` 已公开依赖 `ShanmenWorldGameplay`，若 WorldGameplay 反向包含 P19.0 的 CombatRuntime 合同会形成模块循环。`demo_map` 已合法依赖两者，适合作为产品组合层。

本轮没有修改模块依赖图，也没有把 World/Actor 引用下沉进纯运行时模块。新增文件：

- `demo_mapShanmenDivineSenseWorldObservationAdapter.h/.cpp`；
- `demo_mapShanmenDivineSenseWorldObservationAdapterTests.cpp`。

## 3. 世界观测合同

新增三组边界类型：

1. `Fdemo_mapShanmenDivineSenseWorldSubjectEvidence`：一次对象的 tags、LOS 和非负 authority revision；
2. `Idemo_mapShanmenDivineSenseWorldEvidenceProvider`：调用方注入的同步只读采样 seam；
3. `Fdemo_mapShanmenDivineSenseWorldObservationResult`：状态、诊断、预算、观测数量和最终 P19.0 receipt。

`SampleAndResolve` 显式接收 World、registry、source Actor、冻结 action、definition、scan ordinal、subject budget、Actor 数组和 evidence provider。调用方继续拥有扫描节奏和候选发现；适配器不隐式枚举 Actor。

## 4. 有界 admission 与失败关闭

执行顺序固定为：

1. 验证 action、definition、ordinal 与二者的 action definition identity；
2. 拒绝负预算；若 Actor 数量超过预算，在访问 World 或调用 provider 前拒绝；
3. 验证 World、active registry、Run 一致性；
4. 验证 source Actor 存活、同 World、精确注册，并与冻结 action source identity 一致；
5. 规范化并验证 source location，再由 P19.0 捕获 scan request；
6. 对完整 subject 批次先验证存活、World、注册、稳定实体唯一性和有限坐标；
7. 只有完整批次通过后，才按实体 GUID 排序并逐个调用 provider；
8. 捕获 P19.0 observations，并由 P19.0 resolver 产生最终 receipt。

任一阶段失败均返回精确状态和诊断，不发布有效 receipt。零预算加空 Actor 集合是合法的确定性空扫描。

## 5. 规范化采样与权威证据

调用方 Actor 数组的排列不影响 provider 调用顺序或最终 receipt：批次按稳定实体 GUID 排序，每个 canonical unique subject 恰好调用 provider 一次。

两个 Actor 若映射到同一稳定实体会在任何 evidence callback 前失败关闭，避免同一对象的两个瞬时 transform 进入一次扫描。来源位置和对象位置各采样一次，`-0.0` 规范化为 `0.0`，非有限坐标拒绝。

provider 可以读取同步调用期间的 live World/Actor，但接口明确禁止保留它们。成功结果仅保留不可变值回执、预算与计数。

## 6. 单一策略权威

P19.1 不重新实现 self、距离、必需/阻止标签、LOS、遮挡或容量规则。这些证据原样进入 P19.0，由其完成过滤、稳定排序、裁剪和 receipt identity 派生。

因此 World 层不会产生第二套神识策略，也不会把被 P19.0 拒绝的对象暴露给后续消费者。本轮同样不做 trace/sweep/overlap、damage、资源 Commit/Consume、Tick/timer、随机、Spawn、UI、声音或 Niagara 工作。

## 7. 精确自动化

新增 4 条 exact 用例：

1. `CanonicalSample`：canonical provider 顺序、每对象一次回调、调用方重排不变、来源 transform、最近优先容量和零预算空样本；
2. `PolicyDelegation`：self、范围、标签与 LOS 由 P19.0 统一过滤；
3. `AdmissionAndSourceFences`：运行时输入、预算、World、registry、Run、来源存活/World/注册/identity/坐标边界；
4. `SubjectEvidenceFences`：空对象、跨 World、未注册、实体别名、非有限坐标、provider 拒绝和无效 evidence。

测试还证明完整 Actor 批次会在第一次 provider 回调前完成结构验证，失败结果不会泄漏部分 receipt。

exact 实测 4/0。完整 `Shanmen.0_0_10` 从 P19.0 的 792 增至 796，实测 796/0；首末 Success 为 `2026-09-03 08:59:21.024 -> 09:34:10.375 UTC`，约 34m49.351s。最终日志包含原生 `TEST COMPLETE / EXIT CODE: 0`。

## 8. 改动驱动回归证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| P19.1 exact | 4 | 0 | `8D70F414CF63C1801D0F2C62508AAC4CCCA0295A5C47A3AFBD2C6B93B18CFC3C` |
| P19.0 Divine Sense runtime | 4 | 0 | `D0F63482A0CCEF6981562AF3F5C01DE6A29DC7523483076F8082ED4C5C56C5F3` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | `B23EC3DC43966B791950BF5688CB18D158BBB071B6C572704CA4F9D2739248A0` |
| `Shanmen.0_0_10` full | 796 | 0 | `40F877B76501D7850B2AB06E28FAA00C8C8B13A4B7661E2267A310C5A7E8F8CB` |
| regression coverage | PASS | 0 | `573FDA436C51C0F6521EB4B779158432C2EB97E7123280951A717C04F11A4C7E` |
| coverage self-test | 291 | 0 | `B0BDF49D9BC3B8B8A0D21D33514B5EB5313BDAFDD76D761129E06BFD422060C5` |

新增 changed-path 规则要求 P19.1 源码同时具有 exact、P19.0 Divine Sense 与 WorldGameplay 三组证据；正向与缺失证据 fixture 已加入自测。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=3 Logs=3
SELF_TEST: PASS 291/291
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

## 9. 构建、产物与边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded / native 0 | 7 / 18.20s | `05B36913D2DF1A1ED219946F126A4EF9B80F319D0A1BF31D328CA45FA53CEB34` |
| Game Development final | Succeeded / native 0 | 4 / 25.25s | `2B855A97F8210B3BBB26FC7B837304117712B8B3F91E5A0595D21C3607449C4C` |
| Editor Development final | Succeeded / native 0 | 0 / 0.94s | `2E56E874F8AB8FAC6FBF24137086B245664A8F44432DE6CA11D41993953819B7` |

最终产物：

- `demo_map.exe`：356,595,712 bytes，SHA-256 `F323C56D516728296C80E699A43CEC3F7474B932633495261F141DFF96C97965`；
- `UnrealEditor-demo_map.dll`：15,225,856 bytes，SHA-256 `DEF0CF01E409DE17EC02188CB580C906B51A2475469B7247C6D4B18FE60E0C7B`。

生产 header/cpp 的严格边界扫描为 0 命中；`git diff --cached --check` 通过。

## 10. 提交边界与后续

基线提交为 `3bafdaf16d5391a02718f08450cdc4e090760a4b`（P19.0）。本轮只提交 3 个适配器生产/测试源码、2 个回归映射文件、本 Report 与本 Development Log，共 7 个文件；长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存，`Saved/Codex/P19.1` raw logs 仅本地保留。

下一轮建议在审计现有 action/resource authority 后，为神识建立一个有界产品脉冲协调层：显式接收候选集合并组合 admission、资源和本轮 adapter，但仍不提前接入输入、UI 或表现，也不让适配器自行发现 Actor。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-1-divine-sense-world-observation-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-1-divine-sense-world-observation-adapter/Docs/Report/Dev.D.UE.0.0.10.P19.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-1-divine-sense-world-observation-adapter/Docs/Log/Dev.D.UE.0.0.10.P19.1.r0_log.md>
