# Dev.D.UE.0.0.10.P28.7.r0 Report

## 1. 状态和范围

`P28_7_PASS`。普通激活失败的三个调用点已统一遵守“回滚和World释放全部完成才返回整备”的既有契约。完整新根1429/0、旧根1330/0、Editor/Game双构建及改动文件驱动覆盖全部通过。

基线为P28.6 / `bc4210071b73fb013e47d6a3893ee149b8ace299`，仍是有限FZ-2的局部闭合，不是整体冻结或F阶段验收。代码改动仅Manager及其既有测试，不引入新状态、schema、权威或通用恢复服务。

本阶段核对普通StartPreparedProfileRun → ActivatePreparedProfileWorld的失败分支。修复前，任务内容激活失败、固定拾取物生成失败、控制恢复失败三处均直接调用Flow.Cancel后继续去激活与返回整备，没有使用P28.6的完成确认；Start结果还无条件声称已回滚。先扩展既有真实隔离故障测试得到Red，再实施最小修复。

## 2. 证据边界

扩展ManagerRollbackRetention的既有两种变体：直接回滚/普通激活失败，各含健康与真实权威RecoveryRequired条件。普通激活夹具使用无AuthGameMode的瞬态World，走真实首个失败分支；已有两单位World物品、三颗丹、生命1及实际持久Run，不创建控制器、正式M01、UI或输入。

另外两个失败分支不以本测试冒充真实生成故障或输入验收；仅对共用处理的调用点做静态核对。

Red Editor 4 actions / 38.61秒 / native0；RedProof实际0/1、队列1/native0、崩溃指标0。普通激活的健康变体仍遗留pending，故障变体连续两次观测到Manager物品集合及World owner/绑定丢失；原直接回滚变体、三颗丹/生命1和持久字节保持断言未失败。首次失败原件保留。

最小修复已写入：三个分支共用局部失败处理，先锁定pending、调用P28.6既有回滚端口，仅完整成功后显示整备，不再独立执行第二次去激活；Start返回不再无条件声称已经回滚。最终测试还补充普通激活初始pending=false，要求失败处理自己建立保持标志。

## 3. 最终验证

| 验证 | 实际结果 |
|---|---|
| 修复后Editor | 5 actions / 26.35秒，SUCCEEDED/native0 |
| Game | 4 actions / 44.77秒，SUCCEEDED/native0；未运行产品exe |
| ProductFlow专项 | 5 Success / 0 Fail；原测试四种变体均通过 |
| WorldLifecycle专项 | 6 Success / 0 Fail |
| 完整旧根demo_map | 1330 Success / 0 Fail；队列1330、native0、崩溃指标0 |
| 完整新根Shanmen.0_0_10 | 1429 Success / 0 Fail；队列1429、native0、崩溃指标0 |
| 文件驱动映射 | 5个阶段路径、3条规则、6个必跑组，由本阶段两份完整根日志覆盖 |
| 映射自检 | 529/529通过，native0 |

两组专项也分别具备精确队列、native0及崩溃指标0；专项已包含于完整新根，不重复累加为额外用例。测试注册数未增加。新根在2026-09-18 01:07:00UTC正常退出，同一串行运行器最终退出0；没有拼接多次运行或复用P28.6日志替代本阶段完整验证。

新根日志存在HTTP探测超时及大Tick间隔警告，实际耗时约97分45秒。保留这些警告，不将它们误作测试失败，也不把本次环境下的墙钟耗时当成游戏性能结果。无头启动包含引擎默认初始化，不代表正式地图/玩家流程验收。

新根原始日志SHA-256：`3AEC7675085028644E231AFEF04E647173DDFB6172CE7CBA5B699AD9DE87CAE3`；旧根：`DDE503C935EAA4DAA6686FD15B142788CABA0DE3C44A00996246D2A92F6FF724`。首次Red、专项、双构建及映射的准确路径和SHA见Log；原始日志保留本地，不宣称上传到GitHub。

## 4. 文件与保护

本阶段精确交付5个文件：`demo_mapV3ProgressionManager.cpp`、`demo_mapProfilePreparationFlowTests.cpp`、有限冻结索引、本Report和Development Log。1617产品/脚本输入与最终验证输入锁一致，相对入场只有上述两处源码变化；103原未跟踪用户文件、两份用户修改的OverallReadiness文档均保持原哈希，不纳入本阶段提交。未修改回归映射或以减少断言换取通过。

## 5. 剩余有限审计

- FZ-1：携入装备丢弃/容器转移与DeploymentLock终局的边界，以及未整备新获消耗品的权威路由仍待闭合；不恢复旧消费旁路。
- FZ-2：普通启动外层如何续接未完成清理、终局持久提交与World释放的先后确认、强制EndPlay的证据边界仍待审计。本阶段仅修普通激活失败调用方，不声称完成所有生命周期。
- FZ-3：必须在FZ-1/2关闭后重新固定输入、执行最终冻结验证；本次全量通过不替代最终冻结。

## 6. P/F边界

不改地图或内容资产，不启动Editor UI、PIE、Standalone或产品exe，不做物理输入、UI/数值/敌人行为开发、截图、Smoke、Cook或Package。允许的验证仅Editor/Game编译及UnrealEditor-Cmd无头契约测试。终局和强制EndPlay仍属有限待审计项，未在本阶段宣布关闭。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.7.r0_log.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
