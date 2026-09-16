# Dev.D.UE.0.0.10.P28.1.r0 Development Log

## 1. 基线与范围

- 日期：2026-09-16 UTC；heartbeat codex-10 18:22:55.345Z。
- 入场 HEAD：`aaca1f4cc422f2cc59370a053c890e0693224e91`，分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 入场 tracked/index 干净；103 个既有未跟踪用户文件保持原路径及 SHA-256，不暂存。最近产品 P27.31，最近索引 P28.0；总体报告刚完成文档交付，本轮转回明确授权的 P 阶段框架收尾。
- 读取当前共享 P 基线、冻结索引、最新总体 Report/Log 及 P28.0 阶段证据，沿已记录 FZ-1 候选风险取证，不新增玩法范围。

## 2. 静态核验

1. ProfilePreparationFlow 的 FindBoundShanmenAuthority 限制 Ready + 同一 owner/root；原 UsesShanmenItemLifecycle 只返回该指针是否非空。
2. V3ProgressionManager 共四个生产调用点：启动后/终局 Code B observer、快捷使用、库存使用。新谓词保留归属后，两处 observer 继续禁止旧写入，两处消费进入已有新 Flow 拒绝门；无新增 Manager 路由。
3. Flow 自身调用点包括 Widget Start、Direct Start、Commit、Retry 和技术激活回滚。保留 ready-only 命令查找，分离生命周期归属；启动和技术回滚新增前置拒绝。
4. AuthoritySubsystem 会在实际 service RecoveryRequired 时同步该状态。原服务只有保存结果无法从磁盘重开判定时进入恢复，不能把所有注入保存失败都当同一种状态。
5. 仓库 legacy 写入围栏与 Runtime 局内投影是不同边界；因此仅证明旧存盘写入被围栏拒绝，不能代替外层路由和 Runtime 保持测试。

## 3. 原始失败与修复顺序

- 先只增加 AuthorityRecoveryRouting 测试及两项 health/attribute include，生产代码未修改；Red Editor 4 actions、17.74s、native 0。
- 唯一测试目录来自既有 NewFlowRoot/InitializeExplicit 自动化边界。真实 seed Profile 含三颗丹，经正常 cutover、整备和持久启动；Runtime 生命设为 1，非空/非零保持可观测。
- 仅向该隔离根的新物品主文件/备份写无效内容，使用既有 WriteTemp 故障点，调用真实 durable 库存消耗进入 RecoveryRequired。未损坏、删除或覆盖生产存档。
- 原代码 0 Success / 1 Fail，三条失败断言分别是归属保持、回滚保留状态、非零 Runtime/Profile 联合保持。原生 0 不能掩盖 Fail；也不能从一个联合断言直接推断 Profile 文件一定被改写。
- 再修改 Flow.cpp/.h，保留已有 materialized/pending 身份或匹配绑定的 RecoveryRequired 归属，命令仍需 Ready；Start 明确拒绝，技术回滚在 Runtime 变更前拒绝。
- 最终测试新增第二个 AuthorityRecoveryBeforeStart，覆盖尚未 materialize 的整备保存失败。原 red 证明的是第一个测试；不声称两次完整测试文件逐字相同。
- Fixed Editor 完成后，四项 ProductFlow 专项全过，再锁定 1,131 个 Source/Config/Scripts 输入，串行执行 Items、整个旧 demo_map 根和 Game 构建。

## 4. 自动化原日志

路径相对 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`。原 Saved 文件保留本地，不上传整目录。

| 组 | 原日志路径 | Success / Fail | 原字节 SHA-256 |
|---|---|---|---|
| RedProof | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.1.r0/RedProof/20260916T182606876Z-9365262e/UnrealEditor.log` | 0 / 1 | `BE570297C6BD2C61FCAFD4C5CBDC7DF18E716ABCAF9CCC67C85B7744DDF4D536` |
| ProductFlowFocused | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.1.r0/ProductFlowFocused/20260916T183027758Z-6a73c85b/UnrealEditor.log` | 4 / 0 | `5A64E0078015D607F7084794762280862A33B8326DC052CBF63837F97E4B3E67` |
| ItemsFullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.1.r0/ItemsFullRoot/20260916T183153473Z-bb628c19/UnrealEditor.log` | 80 / 0 | `FA06D567A191576E59931A05B02BC57FEF7867653CD2096D90E08C29B0933559` |
| LegacyFullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.1.r0/LegacyFullRoot/20260916T183224086Z-6bde1598/UnrealEditor.log` | 1330 / 0 | `6C83F335D64673CA6B405EB61DDDC37F395F18ACC9A5211E6C3A048AC57D7A40` |

四组同目录 run-state 均 native 0；均有精确队列结束数，RedProof 仍判失败。四项专项包含在 Items 八十项中，不重复计数。四组各有既有 13 条启动 Condition failed，Fatal error / Ensure condition failed / Unhandled Exception 匹配均为 0；不宣称原日志零错误。

自动化参数为 Unattended / NullRHI / NoSound / NoCompile；无物理输入、可见 Editor UI 或真实游戏验收。最后两组及 Game 由忽略目录下 `Saved/Automation/P28.1/validate_final.ps1` 串行执行；完成判断同时检查原生码、Result、队列和崩溃指标，不靠退出 0 单独判定。

## 5. 构建与回归映射

| 构建 | 原目录 | 原生结果 / stdout SHA-256 |
|---|---|---|
| Red Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.1.r0.RedProof/BuildEditor/20260916T182525504Z-9f605bc7` | 0；`4F9139ABCF8B2EE67FFF982EB93731AB4E9A842A8367E34198DA658B5EA4A904` |
| Fixed Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.1.r0.Fixed/BuildEditor/20260916T182802731Z-ae139f26` | 0；`60C5238AB258D99E8395DFF1E74E93A4A81D1E902ABDF3945658E5329D48580A` |
| Game | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.1.r0.Validation/BuildGame/20260916T183324850Z-0cbef605` | 0；`E44CD3B9520BB98986D1DA99743B4FC02AD897D9A5EFE26EB91D7F0B22C5097E` |

通过统一 Invoke-Shanmen 入口，MaxParallelActions=1、NoUBA。Fixed Editor 29 actions、126.74s；Game 28 actions、137.02s，两者原 run-state 均 SUCCEEDED/native 0。未将输出 binary 路径解释成启动产品。

映射新增精确 Flow 文件规则，保留 ProfileAuthority，三个必跑组为 demo_map.Profile、demo_map.ItemEconomySchema、Shanmen.0_0_10.Items.ProductFlow；本轮两个完整领域根覆盖它们，不用旧证据替代变动后的代码。

- 自检原件 `Saved/Automation/P28.1/regression-selftest.log`：517/517；SHA-256 `96B77D69044EEA0655A097D38D0629A20A894FF21CE4352562003F17ABCE6A5F`。
- 七个新增映射用例：三个真实路径各验证“旧组单独不足”和“两代证据齐全通过”，另验证“新组单独不足”。它们是 PowerShell 检查器自检，不计入 UE 用例。
- 初次五文件覆盖门：PASS Changed=5 Rules=2 Required=3 Logs=2。原件 `Saved/Automation/P28.1/regression-coverage.log`，SHA-256 `E5AFEE2591AC5F89F9E222C1E3F7958947B39CC54BE452D860B98B0C04C546B9`。最终八文件及文档/输入核验见第 8 节。

## 6. 锁定输入

| 文件 | 验证基线 SHA-256 |
|---|---|
| Source/demo_map/demo_mapProfilePreparationFlow.cpp | `51E65079C32DBDB3B4884DFEFA28A66F282B9BEFB184F9F0199D89A8C570A0E7` |
| Source/demo_map/demo_mapProfilePreparationFlow.h | `F3475C2F7F2E0CD1840FE2789D888A8D736CCC1C0812F738EB16AF3D8ABF9600` |
| Source/demo_map/demo_mapProfilePreparationFlowTests.cpp | `FF3862152CF61F522B050BDA4BB5559FC9FEAAD6776B8FAE706D63032BB635A0` |
| Scripts/ShanmenRegressionMap.json | `66A657C8BE9580EEE05FBA84561CEE37C5D84697340DEE2DEDEDBB1D21C8DD82` |
| Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1 | `137FD8F0967300F1C03EC1CE6E6BF21754603EA1895BACC52CEA057E34179A7C` |

## 7. 交接与剩余范围

精确八文件：上表五文件、冻结索引、Report、本 Log。原用户文件与 Saved 证据不提交。FZ-1 仍需完成其余入口审计，FZ-2 仍有有限激活/最终释放核对，FZ-3 尚未执行最终全产品冻结验证；不暂停监控。

未接物理输入，未改正式地图/内容资产、玩法数值、手感、敌人、关卡、UI 表现；未启动 Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook 或 Package。不是整个 World 跨进程原子恢复或实际玩家损失证明。

- [Report](../Report/Dev.D.UE.0.0.10.P28.1.r0_report.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)

## 8. 发布前最终核验

- 八文件覆盖门：`REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=3 Logs=2`；原件 `Saved/Automation/P28.1/regression-coverage-final-scope.log`，SHA-256 `C615974498817D3655779615F1CEEDADB970B0F9D2A0769DD9DDE26D077FA5F5`。
- 三份 Markdown 共 40 个本地链接目标存在；差异卫生检查通过。两个新增文档也在精确暂存后的差异检查范围内，不依赖未跟踪文件的空 diff 当通过。
- 1,131 个验证输入原字节与锁定基线一致；103 个原用户文件的路径集合和 SHA-256 一致。除这两份新阶段文档，没有增加其他待提交文件；不改总体报告或私人文档。
- 发布前本地 HEAD 和远端分支均为 `aaca1f4cc422f2cc59370a053c890e0693224e91`，index 无原暂存内容。只提交第 7 节八文件并普通推送，不强推。
- 修复后专项、两个领域根及 Game 已正常完成，原测试失败与构建输出保留。没有重跑整个 Shanmen 全产品根，不能把本阶段受影响验证当 FZ-3 最终冻结。
