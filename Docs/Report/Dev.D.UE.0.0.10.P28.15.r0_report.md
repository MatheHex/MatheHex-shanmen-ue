# Dev.D.UE.0.0.10.P28.15.r0 Report

## 1. 状态与范围

`P28_15_COMPLETE`。基线P28.14 / `37136226805de31e2fe7c4c47f6c52f97867baec`，2026-09-19。已闭合技术激活回滚中真实散落物拒绝销毁后，既有Manager续接不能接管已接受Runtime前缀的结构缺口。专项、Editor/Game构建、双完整根与实际改动驱动覆盖门全部通过；本阶段完成不等于底层整体冻结。

## 2. 实际反例

P28.14已令Runtime在逻辑终局后保留拒绝销毁的Actor绑定及Destroyed身份；但技术激活失败使用的Flow.CancelActiveRunForActivationFailure仍将PrepareForPersistentRun的精确InvalidWorldBinding拒绝记为FatalProfileError/RecoveryRequired。Manager因此无法记录原有World-only续接，重试又被Flow阶段拒绝；故障解除后仍无法完成原回滚。

这条路径必须保持持久ActiveRun以便原身份恢复，不得复用正常终局的持久结算提交。既有ManagerRollbackContinuation保留普通启动/框架启动两种变体，再各加入两单位SpiritDust的真实散落物与ROLE_SimulatedProxy销毁拒绝。首次Red为0成功/1失败、精确队列1、原生退出0、崩溃指标0，39条最终Expected断言失败；首次失败原件保留，退出0不等于测试通过。

## 3. 有界修复

- Flow只在Prepare返回InvalidWorldBinding、Runtime已Settled、Summary有效且Run/ActivationFailure原因及原Runtime快照匹配时，将已接受Runtime工作交给原Manager续接；其他失败仍拒绝。该状态不是整个回滚成功，也不授权新Run。
- Manager保留原Flow/Runtime/Session/World/Owner/Run/提交计数等绑定校验，允许同Run、同ActivationFailure终局的保留Runtime参与原续接。先完成World释放，再Prepare原Runtime，最后才清pending并确认上层原attempt。
- 不重复Runtime逻辑终局，不写持久结算、不替换durable ActiveRun；不新增schema、公共接口、通用恢复记录或第二权威。
- 三处源码共110新增/13删除；没有改正式资产、地图、玩法或UI。

## 4. 验证范围与最终结果

四变体检查连续两次飞剑拒绝、原绑定不匹配和新启动入口拒绝；新增散落物变体在飞剑故障解除后再连续两次仅剩散落物拒绝。精确保持Destroyed身份、非零数量、原绑定、ActivationFailure损失行和持久ActiveRun快照；成功飞剑/敌人释放前缀不重复，SettlementSubmitCount保持1。最后解除散落物故障，物品恰好销毁一次，原Runtime才清空，上层原attempt才完成，不在同一次调用开启新玩法。

测试沿既有真实Manager端口及既有保留activation-attempt夹具，不将其表述为实际M01完整BeginActivation或物理玩家输入端到端证明。没有新增测试注册、友元或故障注入端口。瞬态Floor只为既有安全物品投影检查服务。

| 验证 | 最终结果 |
|---|---|
| Editor编译 | 5 actions / 18.77秒，SUCCEEDED/原生0 |
| WorldLifecycle专项 | 7成功/0失败，精确队列7、原生0、崩溃指标0 |
| ProductFlow专项 | 5成功/0失败，精确队列5、原生0、崩溃指标0 |
| Game编译 | 5 actions / 28.43秒，SUCCEEDED/原生0；未运行产品程序 |
| 旧完整根 | 1330成功/0失败，精确队列1330、SUCCEEDED/原生0、崩溃指标0 |
| 新完整根 | 1430成功/0失败，精确队列1430、SUCCEEDED/原生0、崩溃指标0 |
| 映射自检 | 独立执行529/529，原生0 |
| 实际改动驱动覆盖门 | PASS，6路径、4规则、9必跑组、2份健康完整根日志 |

20:17:38.361UTC锁定1617输入与103原用户文件；原exec90855串行验证至新根21:29:03.139UTC结束、原生退出0，没有重复启动健康进程或更改锁定产品输入。双完整根合计2760成功、0失败，专项7+5不重复计入。22:02:41UTC再次核验锁定输入、原用户文件、两份保护文档及此前八项完成原件一致；专项/双根的测试计数、精确队列、原生退出和崩溃指标均独立复核。

实际覆盖包含demo_map.CodeB、ItemEconomySchema、ItemUseAndArmor、P4.Hotbar、Profile、V2RangedCompatibility及Shanmen.0_0_10、Shanmen.0_0_10.Items、Shanmen.0_0_10.Items.ProductFlow。十项完成原件及SHA-256见[Development Log](../Log/Dev.D.UE.0.0.10.P28.15.r0_log.md)，包括首次Red及RedBuild；不把静态匹配或自检代替实际回归覆盖。原始引擎诊断、HTTP超时警告保留；无头通过不等于F阶段、实际表现或性能/内存验收。

## 5. 交接与未闭合边界

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。只运行Editor/Game编译与UE-Cmd无头契约验证，不启动Editor UI、PIE、Standalone或产品exe，不接物理输入、改正式内容或进行Smoke/Cook/Package。

本阶段交接精确六路径：三处源码、本Report/Log及[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)，索引最新完成阶段更新为P28.15。包含这六路径的Git提交标识阶段基线。FZ-1/2继续开放；本阶段不是其他外部Adapter启动、旧兼容终局、空间包全部部分释放、其他容器或强制EndPlay已完成审计的证明。

两份OverallReadiness用户修改与103原未跟踪文件保持不变；Saved原始证据仅本地保留，不宣称已上传GitHub。
