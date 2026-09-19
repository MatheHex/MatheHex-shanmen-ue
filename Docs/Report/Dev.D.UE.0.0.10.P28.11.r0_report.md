# Dev.D.UE.0.0.10.P28.11.r0 Report

## 1. 状态与目标

`P28_11_COMPLETE`，2026-09-19。基线P28.10 / `9ce0e8126eb3469681db3670f9dc0fb00f25861e`。本阶段闭合普通Profile终局入口提前清理Manager容器、绕过既有战斗World释放确认的问题。完整新旧根、Editor/Game及精确改动路径覆盖门均通过；这是局部结构修复完成，不是0.0.10整体冻结或F阶段验收。

## 2. 实际反例

沿RequestSettlementAndReload读取实际调用：Runtime接受终局后，Manager无条件执行DestroyRuntimeContainers，随后才调用持久结算与DeactivateProfileWorld。后者虽已在战斗释放拒绝时返回false，但其上游已经销毁宝箱、清空Chests和M01RewardRunId。

在P28.10既有ManagerSettlementContinuation两变体中增加实际初始化、带内容和Run/Container身份的LootChest，由Manager持有；保留真实飞剑Actor销毁拒绝。修复前两条路径均提前丢失宝箱归属，注册测试0 Success / 1 Fail，包含5条对应保持断言失败。该进程原生退出0，因此不能只凭退出码宣称测试通过；原件保留在Development Log。

## 3. 最小修复

- Profile结算路由的容器清理交回既有DeactivateProfileWorld，先取得下层战斗World释放确认再清理。
- 路由判断复用原来的StartupMode与ProfilePreparationFlow存在条件；非Profile兼容路径维持原清理时机。
- 不增加持久schema、第二份权威、恢复记录或公共接口。关闭搜索、终局抑制及持久结算内容不改。
- 测试检查直接提交、两次存盘失败后的重试提交、World-only重试期间原宝箱/ContainerId/RunId保持；恢复后宝箱恰好一次销毁，Manager归属清空。既有原Run持久快照、武器/敌人一次释放和启动门检查继续保留。

## 4. 完成验证

修复后Editor构建4 actions / 16.86秒、Game构建4 actions / 27.81秒，均SUCCEEDED/原生0。WorldLifecycle专项7/0、ProductFlow专项5/0，均精确队列、原生0、崩溃指标0。映射自检529/529通过；首次自检启动命令的PowerShell解析错误已单独记录，其时并未执行测试，不能算作产品失败。

00:28:27UTC锁定1617输入，仅Manager.cpp和PreparationAdapterTests.cpp两个源码路径相对入场变化。同一串行运行器依次完成旧根1330/0、新根1430/0，两根队列分别精确为1330/1430，原生退出均0、崩溃指标均0。专项7/5是根组内的重复验证，不额外计入两根合计2760项。新根于01:45:15.414UTC结束，原运行器最终退出0；没有拼接运行中快照或重启另一个实例。

实际日志覆盖门：PASS，Changed=5、Rules=2、Required=7、Logs=2。ProductRunItemUse与ItemProductAdapters要求的Shanmen.0_0_10、demo_map.Profile、demo_map.CodeB、demo_map.V2RangedCompatibility、Shanmen.0_0_10.Items、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar均有本阶段健康完整根证据；不按主题裁剪。

02:21:14UTC核验1617输入、103原用户文件及两份保护文档未变，暂存为空，远端阶段前HEAD仍与9ce0e81一致。原始失败、最终日志路径与SHA-256详见Development Log。HTTP超时、大Tick间隔和未使用平台SDK警告保留，不宣称无警告、性能达标或其他平台构建通过。

## 5. 边界与交接

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)，只做编译和无头自动化，未启动Editor UI/PIE/Standalone、产品exe、物理输入、正式资产、玩法/UI或Smoke/Cook/Package。

本阶段证明的是Manager容器清理的上游次序，不是容器自身Destroy被拒绝后的恢复，也不承诺容器内容跨终局原子保持。Runtime.RequestSettlement/TeardownWorld中的散落物处理、强制EndPlay顺序、FZ-1剩余权威入口仍待审计。[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)已补齐P28.11证据，仍为FREEZE_AUDIT_IN_PROGRESS；不暂停监控或开展玩法层。

103份原未跟踪用户文件及两份OverallReadiness用户修改不纳入提交。发布范围仅本阶段两个源码、索引、Report/Log五路径；GitHub交接以包含本报告的提交为基线，原始Saved证据保留本地而非宣称已上传。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.11.r0_log.md)
