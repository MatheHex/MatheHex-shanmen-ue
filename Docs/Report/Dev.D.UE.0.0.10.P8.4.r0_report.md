# Dev.D.UE.0.0.10.P8.4.r0 Report

## 1. 结论

P8.4 已建立 Formation product host，结论为 **PASS**。

新增 `Fdemo_mapShanmenFormationProductHost`，把 P8.2 material/deployment session 与 P8.3 World adapter 收束为单一 transient product owner。产品调用顺序固定为 prepare → durable commit → placement；commit 后的 World 故障只允许同一 PlacementId、World 与 Actor class 前向重试，不回滚已提交材料，也不创建第二 authority。

最终 focused `4/4`、0.0.10 full `227/227`、changed-file gate、41/41 映射自测、Editor Development 与 Game Development 均通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 三段式产品边界

Host 明确保留三条命令，而不是把不可逆步骤藏进一次调用：

1. `TryPrepareAnchor`：只调用 P8.2 material prepare，可在 durable commit 前取消；
2. `TryCommitPreparedAnchor`：提交材料与 P8.0 deployment，并发布唯一 canonical placement intent；
3. `TryPlaceCommittedAnchor`：只把该 intent 交给 P8.3 World adapter。

每步都返回嵌套 Session/World result 与 immutable receipts。exact commit replay 会重新暴露原 material evidence 与 deployment receipt，但不会再次进入 item authority。

## 3. Durable commit 与前向恢复

commit 成功后，Host 持有 sole pending placement。此时：

- 另一 anchor 的 prepare/commit 被 `PlacementPending` 拒绝；
- exact commit replay 返回同一 PlacementId 与 audit；
- 第一次有效 placement 调用冻结 World 与 Actor class；
- duplicate tags、spawn/adoption 或 adapter 冲突不会清除 pending；
- 重试若改变 World/class，返回 `PlacementBindingConflict`；
- exact retry 成功后才清除 pending。

World preflight 在调用 Adapter 前检查 live World 与 concrete Actor class，因此可预防的请求错误不会污染 first valid binding。P8.3 已发布 placement 的 exact call 只 replay/adopt，不重复 spawn。

## 4. Cancel、End 与 teardown

取消边界保持 P8.2 语义：

- prepare 后、commit 前取消：释放 reservation，不消费材料；
- 部分 anchor 已 durable commit、Session 仍 Deploying：允许 forward cancel，已消费材料不退款，随后 terminal teardown；
- final anchor commit 后 Session 已 Active：不能走 cancel；必须完成 pending placement，再 End。

`TryEndAndTeardown` 在有 pending placement 时硬拒绝，因此完整阵图不会出现“Action 已结束但最后阵眼从未进入 World”。Cancel/End 成功后统一调用 P8.3 deployment-tag teardown；若 World cleanup 失败，Session 保持 terminal，exact call 只继续 teardown，不重跑 lifecycle transition。

## 5. 权威与兼容性

Host 只拥有：

- 一个 P8.2 `FormationProductSession`；
- 一个 P8.3 `FormationWorldAdapter`；
- 一个 committed-but-unplaced intent 与 transient retry binding。

材料真值仍在 ShanmenItems authority，deployment/action 真值仍在 P8.0/P8.2，Actor identity/weak handle 仍在 P8.3。没有接入 legacy `Udemo_mapItemSubsystem`，没有直接仓库修改、伤害结算、RNG、Tick/timer、UI、输入、效果或正式 content。

## 6. 自动化证据

最终日志均为 one command、one queue-empty、Fail `0`、fatal/assert/ensure `0`，进程原生退出码为 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationProductHost` / `FormationProductHost.log` | 4 | 0 | `FABCB136CE3E3DDF7D724BAF2A6299AD70CCDD50187E376FB837FE731AB02534` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 227 | 0 | `35302F2CBE9B5C1A4721152F31BC173885CE885821E236DE4D689670A6FA3697` |

四项 focused 测试覆盖：

1. durable commit、authority snapshot 不变的 exact replay 与不同 anchor 串行 fence；
2. duplicate-tag World 失败、class drift 拒绝、同绑定 adoption/replay；
3. 两 anchor 全链、final placement End gate、terminal teardown/replay；
4. pre-commit reservation cancel 与 post-commit no-refund forward cancel。

完整 suite 从 P8.3 的 `223` 增至 `227`，此前测试全部继续通过。

## 7. 失败证据与修正

保留三份失败日志：

| 日志 | Success | Fail | Process exit | SHA-256 |
|---|---:|---:|---:|---|
| `FormationProductHost-first.log` | 1 | 3 | 0 | `0A19388C280365313425C4ADF114AD16F0347974D76E025ECC936555735DB790` |
| `FormationProductHost-second.log` | 2 | 2 | 0 | `935B3358D8CD0F4E84803D39CAE8D1A2493033534AEBD9C0FCC3FEB625C6E8B1` |
| `FormationProductHost-diagnostic.log` | 2 | 2 | 0 | `A8FDDFEE757BD61B8E102F90D801ECC84930C909CD24F6B50287E0B64ABD6510` |

首次失败同时暴露两个 fixture 问题：恢复完成后才读取 pending，导致时序断言失真；以及按 item DefinitionId 汇总 active-Run snapshot，得到恒定 `0`。前者改为在故障发生当刻取证；后者改用 material plan 冻结的 `ItemInstanceId/ExpectedQuantityBefore`、authority finalize receipt 与 commit/replay 完整 snapshot equality。生产 Host 无失败构建或运行时修正。

Automation 进程在测试失败时仍返回 `0`；成功判定始终使用 `Test Completed Result` 与 queue-empty，不把进程码单独当成测试成功。

## 8. 改动—回归与静态门禁

新增 `FormationProductHost` 映射，要求七组证据：focused Host、FormationWorldDelivery、FormationSession、FormationMaterialAdapter、Items、WorldGameplay 与 FormationDeployment。

- regression map JSON：PASS，`40` rules；
- mapping self-test：`41/41 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=7 Logs=2`；
- coverage SHA-256：`CB483252DE5BE63EE51DE97AF24555B8CEDB83142A4FAABCEFD8BA5930A40062`；
- self-test SHA-256：`813E982FD667B1E9A5725D4E67DBE3720256DD1A6F88F97496A74B63FA90563D`；
- production boundary scan：无 legacy inventory、damage、RNG、Tick/timer、UI 或 input 依赖；
- `git diff --check`：PASS；最终 staged `git diff --cached --check` 在提交前执行。

## 9. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Native exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor final | Succeeded | 0 | 8.45s | `19FC21B60C3717047F1BFB8311FBE7B01BC6F71D06A665FCC5FAA2896577B07F` |
| Game final | Succeeded | 0 | 27.77s | `D2DBA93AC4239E93A9ECCDA12824CF1AB81D6C5AB7BD396123A85CEABCD124FA` |

首次 Editor source build 也成功，原生退出码 `0`、总耗时 `33.35s`；最终构建基于最终源码。无 UHT、compile、link、commit-memory 或环境失败。

- Editor module：`10986496` bytes，UTC `2026-08-29T17:06:34Z`；
- Game executable：`352214528` bytes，UTC `2026-08-29T17:08:39Z`。

## 10. 修改范围、P/F 边界与下一步

新增：

- `demo_mapShanmenFormationProductHost.h/.cpp`；
- `demo_mapShanmenFormationProductHostTests.cpp`。

更新 regression map、自测、本 Report 与同名 Development Log。未修改 Build.cs、GameplayTags、Content、schema、P8.0/P8.1/P8.2/P8.3、GameMode、输入、UI 或旧产品链。长期未跟踪用户与 0.0.9B 工件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

P8.5 建议在 Host 之上建立纯区域规则 provider：只读取已 placement 的 anchor receipts，生成可测试的区域 membership/coverage，不直接施加伤害、Buff 或内容数值；正式交互、效果与 content 继续后置。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-4-formation-product-host/Docs/Report/Dev.D.UE.0.0.10.P8.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-4-formation-product-host/Docs/Log/Dev.D.UE.0.0.10.P8.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-4-formation-product-host>
