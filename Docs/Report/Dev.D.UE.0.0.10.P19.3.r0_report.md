# Dev.D.UE.0.0.10.P19.3.r0 Report

## 1. 结论

P19.3 在 P 阶段边界内完成，结论为 **PASS**。

本轮在 P19.2 原子神识脉冲协调器之上新增 Run-scoped `Fdemo_mapShanmenDivineSenseProductHost`。Host 是该 Run/source 的 SpiritEnergy authority 与 processed-pulse coordinator 的唯一产品层持有者；调用方只提交不可变 pulse command、显式 source Actor、显式有界 subject Actor 数组和只读 evidence provider。Host 在私有副本上执行完整脉冲，只有资源状态与脉冲证明同时有效时才整体发布。

```text
P19.3 exact:                         4 Success / 0 Fail
P19.2 pulse coordinator:             4 Success / 0 Fail
P19.1 World observation:             4 Success / 0 Fail
P19.0 Divine Sense runtime:          4 Success / 0 Fail
ActionResource dependency:           7 Success / 0 Fail
ActionLifecycle dependency:          1 Success / 0 Fail
WorldGameplay dependency:           10 Success / 0 Fail
Shanmen.0_0_10 full:               804 Success / 0 Fail
Regression coverage:                PASS (Changed=5 / Rules=1 / Required=7 / Logs=7)
Regression gate self-test:           PASS 295/295
Boundary scan:                       PASS (Files=2 / Matches=0)
Game + Editor Development:           PASS / native status 0
```

没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件或真实输入；没有执行截图、Smoke、Cook 或 Package。本轮不宣称自动 Actor 发现、持续扫描、UI/声画表现、最终 SpiritEnergy 平衡、真实玩家操作或产品验收已经完成。

## 2. 产品 Host 与单一权威

新增：

- `Source/demo_map/demo_mapShanmenDivineSenseProductHost.h`；
- `Source/demo_map/demo_mapShanmenDivineSenseProductHost.cpp`；
- `Source/demo_map/demo_mapShanmenDivineSenseProductHostTests.cpp`。

`TryOpen` 固定 RunId、source entity、初始/最大 SpiritEnergy、资源 revision 与正数 pulse capacity，并创建：

- 一个既有 `FShanmenActionResourceAuthority`，资源通道固定为 `Resource.SpiritEnergy`；
- 一个 P19.2 `Fdemo_mapShanmenDivineSensePulseCoordinator`；
- 一个不可变 opening resource snapshot；
- 一个由 coordinator ID 与 opening snapshot ID 派生的确定性 HostId。

Host 不暴露可写资源 authority，也不复制资源算法。余额、预留、revision 与事务记录仍由既有 authority 单写；动作、扫描、World 身份和观测分别继续由既有 P19.2/P19.1/P19.0 合同负责。

Host 的 `IsValid()` 要求资源通道、owner、最大值、opening snapshot、HostId、pending reservation、事务数量、processed pulse 数量和 revision 增量全部一致。每个成功新脉冲严格对应一条已完成资源事务和两次 revision 推进；创建时还预留足够 revision headroom，避免声明容量在 `MAX_int64` 附近失真。

## 3. 不可变 Pulse Command

新增 `Fdemo_mapShanmenDivineSensePulseCommand`。捕获成功后字段私有且只读，内容包括：

- 完整 action snapshot；
- Divine Sense definition；
- SpiritEnergy cost；
- scan ordinal；
- subject Actor budget；
- 确定性 CommandId。

CommandId 不只信任 ActivationId，而是绑定 action 的 Run/owner/activation/source/source-item/action-definition/content version/content digest/规范化 source tags，以及 definition ID、cost ID、ordinal 和 budget。`IsValid()` 每次重新派生并比较 CommandId；非神识 action、definition 不匹配、非 SpiritEnergy cost、负 ordinal 或负 budget 均拒绝捕获。

## 4. 原子发布与失败关闭

`ExecutePulse` 先验证 Host、command、Run 和 source，再捕获 `ResourceBefore`。新脉冲在 Host 副本中调用 P19.2：

1. 动作 Startup；
2. SpiritEnergy Reserve；
3. 动作 Active commit 与资源 Finalize；
4. P19.1 显式 World observation；
5. 动作 Recovery/Completed；
6. processed-pulse proof 写入临时 coordinator；
7. 校验临时 Host 与 `ResourceAfter`；
8. 构造并自校验产品结果后，才整体替换正式 Host。

任一步失败时，正式 Host 不变；结构化拒绝返回相同 `ResourceBefore`/`ResourceAfter` snapshot ID，并保留 P19.2 原始错误、资源错误或 World failure。测试证明 evidence provider 在 staged resource commit 之后拒绝时，正式余额仍为 100、revision 仍为 0、processed count 仍为 0；修复 provider 后同一 command 可重试并只消费一次。

资源不足在 evidence provider 调用前拒绝。跨 Run、跨 source、无效 command 和 Actor budget 超限同样不改变余额、revision 或 ledger。

## 5. 幂等重放与容量

Host 把 P19.2 的 exact replay 提升为产品结果：

- exact replay 可传空 World、空 source Actor 和空 subject 数组；
- 不再次访问 evidence provider；
- `ResourceBefore` 与 `ResourceAfter` 完全相同；
- 不二次消费、不推进 revision、不增加 processed count；
- 返回原 pulse receipt，并标记 `AlreadyApplied`。

同 ActivationId 但 budget 或其它不可变载荷变化时，CommandId 随之变化，P19.2 返回 `ActivationConflict`。声明容量耗尽后，新 activation 失败关闭且不访问 provider；已提交 activation 仍可 exact replay。

## 6. 产品结果自校验

`Fdemo_mapShanmenDivineSenseHostPulseResult` 区分 Applied、AlreadyApplied 与 Rejected，并携带 HostId、Host RunId、command、前后资源 snapshot 和嵌套 pulse result。

`IsValid()` 不把非空字段直接当证明：

- 成功结果要求 command 的 Run/source 与 Host Run/resource owner 一致；
- command 必须与嵌套 pulse receipt 的 action/definition/cost/ordinal/budget 完全一致；
- Applied 要求 `ResourceBefore` 精确等于 reservation 输入 snapshot，最终 snapshot 精确匹配 commit receipt，revision 恰好增加 2；
- AlreadyApplied 要求嵌套 pulse 为 replay，且前后 snapshot ID 相同；
- Run/source mismatch 必须由结果内事实重新证明；
- PulseRejected 必须携带有效且非成功的 P19.2 结果；
- InvalidCommand 必须实际携带无效 command。

## 7. 精确自动化

新增 4 条 exact 用例：

1. `OpenAndApply`：确定性 HostId、command/Host 绑定、一次显式观测、100 -> 90、revision 0 -> 2、单 pulse 发布；
2. `ReplayAndCapacity`：空 World 零 I/O replay、载荷冲突、第二个容量槽、容量耗尽拒绝；
3. `RollbackAndRecovery`：World evidence 拒绝后的完整回滚、同 command 修复重试、资源不足前置拒绝；
4. `AdmissionFences`：无效 Run/source/resource/revision/capacity、revision headroom、非神识 action、错误资源通道、负 ordinal/budget、跨 Run/source、默认 Host 与 Actor budget 拒绝。

最终 exact 为 4/0。完整 `Shanmen.0_0_10` 从 P19.2 的 800 增至 804，实测 804/0；首末 Success 为 `2026-09-03 10:57:07.617 -> 11:28:57.150 UTC`，约 31m49.533s。全量日志包含 UE 5.8 原生 `TEST COMPLETE / EXIT CODE: 0`。

## 8. 改动驱动回归证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| P19.3 exact | 4 | 0 | `9ABE55D9DF839433A6FF553B8D7EF050DA10A3C96306683EC045A117A311CF95` |
| P19.2 pulse coordinator | 4 | 0 | `A4EBB240D7C755DA2C3EE69D298FF648B399345E578FC477D44438B1534C7BB5` |
| P19.1 World observation | 4 | 0 | `CD2E84C5D558BD4DD4D71FE27CE6C8328CD41F000FE9ADF6BA8929C72DA6A3EF` |
| P19.0 Divine Sense runtime | 4 | 0 | `2E9F06BAC1DF43A727D2C56900BCF9FC9329BAF94EAD357553EAD5C442438DFA` |
| ActionResource | 7 | 0 | `B05D19032F2172F1E807ADB2051AFCA1375EFFBCD86E581C70FFB7C4F480095A` |
| ActionLifecycle | 1 | 0 | `647BEF5FF73284D1FC09D5F3349EE9477F423A2EF8808F92F9A09B6D34CEEEB1` |
| WorldGameplay | 10 | 0 | `8403D3C9F555F574D5ED4E385C8A9A17F68A2C92EE56054F662816C90F6DFA08` |
| `Shanmen.0_0_10` full | 804 | 0 | `94FF12DECD5B3F74D83B3F42A81542DD4D61ABE86569FADD4EB64BFB7513F3A5` |
| regression coverage | PASS | 0 | `AEDFEFD4736F1A999E557A4F29B16CF8845C66B349DA75FEC95BF99C3DE9D060` |
| coverage self-test | 295 | 0 | `62B2317F10C2EE326F2B3BE71D6E7A9A165C9B5A4114E17BC55B1C8C4A1E1FAE` |

新增 `DivineSenseProductHost` changed-path 规则，要求 Host exact、P19.2 coordinator、P19.1 World observation、P19.0 Divine Sense、ActionResource、ActionLifecycle 与 WorldGameplay 七组证据。正向完整证据与 focused-only 失败关闭 fixture 使门禁自测从 293 增至 295。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=7 Logs=7
SELF_TEST: PASS 295/295
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

## 9. 构建、产物与边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development integration | Succeeded / native 0 | 5 / 6.17s | `25330133D62CDFEBEB58375C7148DEECFEBDCA67211F62D4D3217EA3FCE846E1` |
| Game Development final | Succeeded / native 0 | 4 / 21.86s | `CA7FC367EED4CCCBA00AF68BD1C6251E69C6B41B7C5DB806D04204CF368A2703` |
| Editor Development final | Succeeded / native 0 | 0 / 1.00s | `2884985F910331B307EA7E6B60E7EDDC833BDFD6629B6B267669EFBB01CF9556` |

最终产物：

- `demo_map.exe`：356,706,816 bytes，SHA-256 `FAB98E43ACA5C1DE67E48BF018AC885E15ACA9B368EDB9F576E3702994BDC2D9`；
- `UnrealEditor-demo_map.dll`：15,361,024 bytes，SHA-256 `2F2665D1D0DA0CDC62241848AEE14C5BF6979FAA17ADD14745614F5BCA6D05C2`。

生产 header/cpp 的严格扫描未发现 Actor 全局枚举、trace/sweep/overlap、直接伤害、Tick/timer、随机、Spawn、UI、声音或 Niagara 调用。测试 fixture 的 transient Actor 创建不属于生产边界。

## 10. 提交边界与后续

基线提交为 `f23147ca2fed77dd498334583260955383ab2b72`（P19.2）。本轮只提交 3 个 Host 生产/测试源码、2 个回归映射文件、本 Report 与本 Development Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；raw logs 仅保留在 `Saved/Codex/P19.3`。

P19.4 建议建立显式 Divine Sense command router：把上层已准备好的动作事件、内容定义、资源费用和显式 Actor 集合路由到本 Host，并保留 Run/source/command replay fence；仍不接物理输入、不隐式发现 Actor、不接 UI/表现，也不冻结最终数值。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-3-divine-sense-product-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-3-divine-sense-product-host/Docs/Report/Dev.D.UE.0.0.10.P19.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-3-divine-sense-product-host/Docs/Log/Dev.D.UE.0.0.10.P19.3.r0_log.md>
