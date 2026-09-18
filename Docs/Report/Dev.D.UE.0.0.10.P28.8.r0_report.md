# Dev.D.UE.0.0.10.P28.8.r0 Report

## 1. 状态与有限范围

`P28_8_COMPLETE`，基线P28.7 / `1dacc52c1fdc528b4a3c702e95caa7ea6451d87a`。普通Start入口对原World清理的续接已完成修复与完整验证；不处理终局、强制EndPlay或扩充恢复系统，不宣称全框架冻结。

普通StartPreparedProfileRunFromSect转发到StartPreparedProfileRun；修复前遇PendingProfileWorldRollback仅返回SessionNotReady。框架Coordinator已有原尝试续接。实际Red已证明普通入口在释放故障解除后仍无法完成原清理；框架变体的原有保持断言未失败。

## 2. 验证设计

扩展既有ManagerRollbackContinuation测试为框架/普通两种路由，共用真实隔离Profile和物品权威、已启动持久Run、实际飞剑World Actor和非零时间线。通过引擎真实销毁拒绝形成未完成清理，不伪造回滚成功。普通变体从底层回滚建立真实pending，之后调用普通公开Start入口；它不是普通激活全过程测试，首次激活失败的证据仍见P28.7。

框架变体保留P28.6原Coordinator/Adapter尝试及归属断言；Manager初始化路由标志与框架实际初始化一致。普通变体不创建Controller、不开UI。故障恢复后，续接只能清理原Run，不能同一次调用启动新Run；框架尝试仍须由其Coordinator确认。

## 3. 修复与实际验证

Red Editor 4 actions / 43.74秒 / native0；RedProof实际0 Success / 1 Fail、精确队列1/native0、崩溃指标0。普通续接/完成诊断、原飞剑与敌人恰好一次释放、Manager/GameMode清理共五条断言失败；其他保持断言未失败。原件保留，不能以native0认定测试成功。

已实施12行最小产品修补：待回滚的普通Start复用RollbackPreparedProfileRunFor0909B并投影真实诊断/快照，成功才调用已有返回整备处理；本次仍返回SessionNotReady，不开始新Run。Framework-host Manager保持拒绝，由原Coordinator/Adapter继续确认。无新增状态、接口或schema。

修复Editor 4 actions / 22.39秒、Game 4 actions / 45.55秒，均SUCCEEDED/native0。ProductFlow专项5/0、WorldLifecycle专项6/0均精确队列、native0、崩溃指标0，普通和框架两种变体通过，未增加注册测试数。映射自检529/529通过。完整旧根1330/0；完整新根于2026-09-18 04:12:02UTC结束，1429/0。两根均核验精确队列、run-state原生退出0、崩溃指标0；合计2759条根测试成功，专项属于根内覆盖，不重复累计。

新根最终原件SHA-256：`CEAB2B581C9F906441CF7A98A0B2392813E3590209D405C5391432947E216A1F`。详细原件位置、首次Red与其余哈希见Development Log。原始日志只保留本地，不宣称已上传GitHub。保留HTTP超时及大Tick间隔警告，不宣称日志无警告或将本次耗时作为性能测量。

## 4. 覆盖、输入与发布范围

02:25:47UTC固定1617最终输入，仅Manager和既有PreparationAdapter测试改变；19:08UTC重新核验全部输入及103用户哈希一致，两份OverallReadiness文档哈希不变。原外层exec会话已不可查询，其最终退出码不补写；直接以各原生run-state、完整日志及本次重新核验作为证据，无需重跑已完成构建/测试。

精确五个阶段路径通过文件驱动覆盖门：2条映射规则、7个必跑组，由本轮两个完整根日志覆盖。分别为demo_map.CodeB、ItemUseAndArmor、P4.Hotbar、Profile、V2RangedCompatibility及Shanmen.0_0_10、Shanmen.0_0_10.Items。源码仅两文件，另更新本Report、Log和有限冻结索引；不提交用户文档、103原未跟踪文件或Saved原始日志。

## 5. 剩余边界

FZ-2的普通启动外层续接已闭合；终局持久提交与World释放确认、强制EndPlay次序仍需有限审计。FZ-1的携入装备丢弃/容器转移与终局规则、新获未整备消耗品使用边界仍未全部闭合。FZ-3最终冻结回归尚未开始，不能据本轮全根通过暂停监控或宣布全框架完成。

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：无物理输入、正式资产变更、UI/玩法调试、Editor UI、PIE、Standalone、产品exe、Smoke、Cook或Package。103原用户未跟踪文件及两份OverallReadiness用户文档保持不变。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.8.r0_log.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
