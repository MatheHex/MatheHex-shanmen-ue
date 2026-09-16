# Dev.D.UE.0.0.10.P28.2.r0 Report

## 1. 结论与范围

修复 FZ-1 的携入物品身份缺口：新获得的同类堆叠不得并入或吞掉携入实例，重新拾取携入物不能把它重标为本 Run 新获物。修复维持现有持久消费与终局校验，不增加物品权威、schema、玩法或 UI。

本阶段验证及交付核对完成：专项 10/0、完整 Items 84/0、完整旧 demo_map 根 1330/0；Editor/Game 均 SUCCEEDED/native 0。2026-09-16 21:37 heartbeat 恢复交付时核对原始日志与锁定输入，不重复执行已完成构建。FZ-1/2/3 尚未全部关闭，不宣布整体冻结。

## 2. 基线及原始失败

- 日期：2026-09-16 UTC；入场 HEAD `f89605577b6e07e30c3e87fd949b89144297bf57`，最新产品为 P28.1。
- 恢复交付 HEAD `37959331e7481704fd930d94a2b623dafaf60a96`；期间只新增总体 Report/Log 文档提交，P28.2 的 1,131 个锁定验证输入未变化。保留总体报告，不混入本阶段提交。
- 先只增加 AcquiredStackIdentity 测试，未修改生产代码。使用隔离 Profile，实际 cutover、整备三颗丹、StartPreparedRun，再调用 Runtime.AddDefinition 获得一颗同类丹；生命以 1 为基准。
- 原代码按定义和奖励元数据合并，将新获丹并入携入 ID，再把该 ID 标为新获来源；局内四颗与持久预留三颗不一致。
- 自动化结果 0 Success / 1 Fail，身份/数量保持、持久使用、原样终局交接三条断言失败。原生退出 0 不能把 Fail 当成功；原日志保留。
- 没有手工改写本轮测试的结算摘要，也没有通过新 RequestId 或放宽校验伪造可接受输入。

## 3. 实现

1. Core Runtime authority 的 AddDefinition、PickupWorld、ExecutePlayerItemDrop 接收只在调用期间借用的携入 ID 集合；默认未提供时维持无携入上下文的旧调用行为。
2. Runtime Subsystem 传递已有 DeployedItemIds，不复制一份长期身份表。统一合并条件同时检查源和目标，任意一方为携入实例均不合并。
3. 自动获得/拾取的容量预检排除携入堆叠中的余量；有空格时保留独立堆叠，无合法容量则拒绝且保持原数据。
4. 玩家格子拖拽沿既有整体 Swap 路径处理不可合并实例；容器两方向不再进入破坏身份的 Merge。不能合法存回容器时仍按原不变量拒绝，未新增携入物存箱规则。
5. TagAffectedForActiveRun 只给非携入 ID 标注新获来源；原物丢下再捡回保留 ID、数量和原 provenance。
6. 普通新获物之间仍可按原规则合并。ShanmenItems 的预留、消费、终局与持久文档均未修改。

## 4. 新增验证

| 用例 | 证明范围 |
|---|---|
| AcquiredStackIdentity | 三颗携入＋一颗新获分别保留；生命 1 时持久使用一颗后为 2；原样撤离得到原物 2＋新获 1；精确重放不重复保存；重开持久权威状态相同，旧 Profile 字节不变 |
| PreparedStackMergeGuards | 两个不同携入 ID 与新获 ID 的两方向/原物之间拖拽不合并；新获之间仍合并；满包获得/拾取拒绝且 revision 不变，释放一格后正常拾取 |
| PreparedWorldPickupIdentity | 隔离无头 World、真实碰撞地面、Pawn/Controller 和绑定 WorldItem，经生产拾取入口获得同类物；原物经生产丢弃/拾取入口返回仍保留原身份；原样结算成功 |
| PreparedContainerMergeIdentity | 实际开箱/识别后的容器两方向拖拽均不能吞并携入物，非法操作保持数量/来源及双方 revision；取到空格仍成功，原样结算成功 |

World 测试使用瞬态物理夹具，不修改地图/资产、不发送物理输入，也不证明正式游戏可玩或视觉播放已验收。

## 5. 回归与构建

- 修复后 RunLifecycle 专项：10 Success / 0 Fail，native 0，队列 10，崩溃指标 0。
- Editor：84 actions，250.11 秒，SUCCEEDED/native 0。
- 完整 Items：84 Success / 0 Fail；完整旧 demo_map 根：1330 Success / 0 Fail，均 native 0 且队列正常结束。专项 10 项包含在 Items 84 项内，不重复累计。
- Game：83 actions，250.81 秒，SUCCEEDED/native 0；构建完成不等于启动或验收产品体验。
- 既有回归映射覆盖四个源码路径，要求 Items、ItemUseAndArmor、P4.Hotbar；执行整个旧根覆盖受影响的旧物品、拖拽、容器、整备和存档测试，不只按本轮新主题取样。
- 回归检查器未改动，现有自检 517/517 通过。原始失败与成功日志分别保留，不拼接计数。
- 精确七文件回归映射 PASS（Changed=7 / Rules=1 / Required=3 / Logs=2）；完整新 Shanmen 根未在本阶段执行，最终冻结仍需 FZ-3 另行验证。

## 6. 剩余问题与 P/F

本轮没有改变“撤离不能静默丢失 DeploymentLock 装备”的既有约束。完整丢弃/存箱路由与终局政策、未整备新获物使用、FZ-2 激活/释放仍需有限审计；不把堆叠身份修复称为全入口权威已冻结。

没有扩充技能、敌人、手感、关卡、UI 或数值。仅允许的 Editor/Game 编译和 UnrealEditor-Cmd 无头测试；未启动 Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook 或 Package。

本阶段只精确提交四个源码文件、冻结索引及本 Report/Log；103 个既有未跟踪用户文件保持不变。Saved 原日志保留本地，GitHub Log 提供索引和哈希。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.2.r0_log.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
