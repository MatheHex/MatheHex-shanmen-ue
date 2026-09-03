# Dev.D.UE.0.0.10.P19.2.r0 Report

## 1. 结论

P19.2 在 P 阶段边界内完成，结论为 **PASS**。

本轮在 P19.0 纯神识扫描合同与 P19.1 显式 World 观测适配器之上，新增一个 Run-scoped、有界、可重放的产品脉冲协调器。协调器复用既有动作编排器与 `FShanmenActionResourceAuthority`，把动作启动、SpiritEnergy 预留/提交、World 观测、动作恢复/完成先执行在私有副本上；只有全链成功才一次性发布资源状态和脉冲回执。任一步拒绝都不消耗资源、不占用脉冲容量，也不留下半完成动作证明。

```text
P19.2 exact:                         4 Success / 0 Fail
P19.1 World observation:             4 Success / 0 Fail
P19.0 Divine Sense runtime:          4 Success / 0 Fail
ActionResource dependency:           7 Success / 0 Fail
ActionLifecycle dependency:          1 Success / 0 Fail
WorldGameplay dependency:           10 Success / 0 Fail
Shanmen.0_0_10 full:               800 Success / 0 Fail
Regression coverage:                PASS (Changed=5 / Rules=1 / Required=6 / Logs=6)
Regression gate self-test:           PASS 293/293
Boundary scan:                       PASS (Files=2 / Matches=0)
Game + Editor Development:           PASS / native status 0
```

没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件或真实输入；没有执行截图、Smoke、Cook 或 Package。本轮不宣称玩家输入、自动 Actor 发现、持续扫描、UI/声画表现、最终 SpiritEnergy 数值平衡或产品验收已经完成。

## 2. 组合位置与单一权威

新增协调器位于 `demo_map` 产品组合层：

- `demo_mapShanmenDivineSensePulseCoordinator.h/.cpp`；
- `demo_mapShanmenDivineSensePulseCoordinatorTests.cpp`。

没有新增资源账本。SpiritEnergy 继续由调用方持有的既有 `FShanmenActionResourceAuthority` 单写；神识策略继续由 P19.0 resolver 单写；World 身份和显式样本观测继续由 P19.1 与 `FShanmenWorldEntityRegistry` 负责。协调器只编排这些既有权威并发布组合证明。

每次脉冲的 cost 由内容层提交，协调器只要求其资源通道精确为 `Resource.SpiritEnergy`；测试中的 10 点消耗是验证样本，不是冻结的产品平衡值。

## 3. 原子脉冲事务

新请求按固定顺序执行：

1. 验证 coordinator、Run、神识 action/definition、SpiritEnergy cost、scan ordinal 和显式 Actor budget；
2. 在私有 action runtime 上产生 `Idle -> Startup`（sequence 0）；
3. 从私有资源权威捕获 snapshot，并预留 cost；
4. 产生 `Startup -> Active`（sequence 1，越过 commit point），再把预留提交为实际消耗；
5. 调用 P19.1 对显式 Actor 集合执行一次有界 World 观测；
6. 产生 `Active -> Recovery`（sequence 2）和 `Recovery -> Idle/Completed`（sequence 3）；
7. 校验完整不可变回执与确定性 ID；
8. 仅此时把私有资源权威和私有 processed-pulse ledger 发布给调用方。

测试证明初始 100 点 SpiritEnergy 在一次 10 点成功脉冲后为 90，authority revision 从 0 增至 2，临时 reservation 清零，且只记录一个已完成 activation。

## 4. 失败关闭与回滚边界

资源不足在任何 evidence callback 前拒绝，余额、revision 和 coordinator ledger 均不变。World/provider 在资源已于私有副本中 staged commit 后拒绝时，私有副本被丢弃，因此调用方资源仍为 100/revision 0，同一 ActivationId 可以在问题解除后安全重试并只提交一次。

P19.1 的精确失败状态与诊断保留在 `WorldFailure` 中，外层同时给出协调器错误分类。错误不会伪装成成功回执。

原子性覆盖协调器与资源权威的内部状态；注入的 evidence provider 合同仍必须是同步只读，不得把回调当作可回滚外部副作用。协调器不会尝试撤销 provider 外部状态。

## 5. 幂等重放与有界容量

成功 activation 以不可变脉冲回执存入显式容量的 Run ledger。相同 ActivationId 只有 action、definition、cost、scan ordinal 和 subject budget 全部一致时才可重放；任一不可变输入变化都会返回 `ActivationConflict`。

精确重放还会在资源权威副本上验证既有 reservation/finalization 证明必须分别返回 `AlreadyReserved` 和 `AlreadyFinalized`，且回执 ID 完全一致。资源账本与 coordinator 不一致时返回 `StateDesynchronized`，不会采样 World，也不会修改传入账本。

重放直接返回已提交证明，允许传入空 World、空 source 和空 Actor 集合；不会重新调用 provider、重新消耗 SpiritEnergy 或推进 revision。ledger 达到创建时声明的容量后，新 activation 失败关闭，既有 activation 仍可精确重放。

## 6. 不可变回执

`Fdemo_mapShanmenDivineSensePulseReceipt` 绑定：

- coordinator 与 Run；
- action activation 和 Divine Sense definition；
- SpiritEnergy cost、reservation 与 finalization；
- P19.1 World observation receipt；
- 四个动作 transition token；
- scan ordinal、subject budget 与实际观测数量。

回执 ID 从上述 canonical parts 确定性派生。`IsValid()` 重新验证动作种类、资源通道、四段固定 transition、reservation/finalization 关联、预算边界与最终 ID，避免仅凭非空 GUID 接受不一致证明。

## 7. 精确自动化

新增 4 条 exact 用例：

1. `AtomicCommit`：完整动作/资源/World 成功、一次 provider 调用、一次 reveal、100 -> 90、revision 0 -> 2 和固定 transition 序列；
2. `ReplayAndCapacity`：零 World I/O 精确重放、输入冲突、容量拒绝和资源账本失同步；
3. `AtomicRollback`：资源不足前置拒绝、World evidence 失败后的 staged rollback，以及同 activation 修复后重试；
4. `AdmissionFences`：无效 coordinator/Run/capacity、跨 Run、非神识 action、错误资源通道、错误资源 owner 与 P19.1 预算拒绝。

exact 首次运行即为 4/0。完整 `Shanmen.0_0_10` 从 P19.1 的 796 增至 800，实测 800/0；首末 Success 为 `2026-09-03 09:58:38.027 -> 10:31:41.540 UTC`，约 33m03.513s。最终日志包含原生 `TEST COMPLETE / EXIT CODE: 0`。

## 8. 改动驱动回归证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| P19.2 exact | 4 | 0 | `902C219E88B652CA14BD42E75EC2D9C6625B0402D1156B4D76F6626795BF683A` |
| P19.1 World observation | 4 | 0 | `FAF1F76852C4622D879C18F88682530C5F36DC1993C490E38E2B41F499676677` |
| P19.0 Divine Sense runtime | 4 | 0 | `A297FDAADB85765B22C97FA1C412BD86281B9A82F8CC534529D000AF2E651CDB` |
| ActionResource | 7 | 0 | `08EC00CDDD421FA8982704DC0332A99E48C8AC023E2E36CCF44D15CC0DC1882C` |
| ActionLifecycle | 1 | 0 | `766B5CB9AA716E3F940C75900DBED97FBA32A11CC3B2998C59A7A9F33A0A4D25` |
| WorldGameplay | 10 | 0 | `254A5D3BF1B98402C670CD56A079A112DB3E56356D04FB99A2CBD8E808CA5F83` |
| `Shanmen.0_0_10` full | 800 | 0 | `1F3BED185721167A50DDB14B4BD38FACA87859CE95B77750C4AC0EC799D06CF0` |
| regression coverage | PASS | 0 | `EDDC169A52CD911D2E8B725817CE0C7399D7F89FB301D7A8E70B21A0714FC2C1` |
| coverage self-test | 293 | 0 | `2C05FA17F1C9FF95EE3FC62088B0AFC98729C9C40CBB2D58592A54FEE6E6F8AB` |

新增 changed-path 规则要求 P19.2 源码同时具有协调器 exact、P19.1 World observation、P19.0 Divine Sense、ActionResource、ActionLifecycle 与 WorldGameplay 六组证据；正向与缺失组合证据 fixture 已加入自测。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=6 Logs=6
SELF_TEST: PASS 293/293
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

## 9. 构建、产物与边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded / native 0 | 5 / 15.23s | `3975B3100B4B9E3A6F0F64A0CAC49D2D9B2DC6602219474DBE6A816DB893F5B1` |
| Game Development final | Succeeded / native 0 | 4 / 22.19s | `2BA2193E96A3842339170564AE06E5E9F16F0A47D304576D88D023F5A82726FF` |
| Editor Development final | Succeeded / native 0 | 0 / 0.98s | `4379F9D11145E8663CD05488E6E933292D791F9E59B03BC4BC6DE7A52C65F1D3` |

最终产物：

- `demo_map.exe`：356,654,080 bytes，SHA-256 `96E993847AEA21563B5F5B23AD24B1A820C74DEAC69990A8D7D4C2F38BECBAE8`；
- `UnrealEditor-demo_map.dll`：15,296,000 bytes，SHA-256 `C6A1662F192DDE5DEFBCE5CC11A8874D7DA1B3CA533765BF92187F984581E755`。

生产 header/cpp 的严格扫描未发现 Actor 全局枚举、trace/sweep/overlap、直接伤害、Tick/timer、随机、Spawn、UI、声音或 Niagara 调用；`git diff --cached --check` 在提交前执行并通过。

## 10. 提交边界与后续

基线提交为 `005897e1b6a9bdc2b0d520a1d9b1083df4df7c35`（P19.1）。本轮只提交 3 个协调器生产/测试源码、2 个回归映射文件、本 Report 与本 Development Log，共 7 个文件；长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存，`Saved/Codex/P19.2` raw logs 仅本地保留。

下一轮建议建立 P19.3 Run-scoped Divine Sense product host：持有本轮 coordinator 和调用方资源权威，接收显式 pulse command 与候选集合并返回不可变结果，但仍不隐式枚举 Actor、不接物理输入、不接 UI/表现，也不提前冻结平衡数值。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-2-divine-sense-pulse-coordinator>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-2-divine-sense-pulse-coordinator/Docs/Report/Dev.D.UE.0.0.10.P19.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-2-divine-sense-pulse-coordinator/Docs/Log/Dev.D.UE.0.0.10.P19.2.r0_log.md>
