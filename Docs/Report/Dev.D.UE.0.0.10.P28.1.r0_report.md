# Dev.D.UE.0.0.10.P28.1.r0 Report

## 1. 本阶段结论

修正 FZ-1 中已复现的一个结构缺口：新物品权威进入 RecoveryRequired 时，ProfilePreparationFlow 不再把暂时不可执行误判成旧流程；技术激活回滚先检查权威可用性，不能清掉仍需恢复的 Runtime。

本阶段完成受影响回归与双目标构建，不声称整个 FZ-1 或底层框架已经冻结。交付范围与边界见第 7 节。

## 2. 基线与故障证明

- 日期：2026-09-16 UTC；入场 HEAD `aaca1f4cc422f2cc59370a053c890e0693224e91`。
- 最近产品基线 P27.31；P28.0 的冻结索引和总体报告已提交。
- 原实现的 UsesShanmenItemLifecycle 只判断 FindBoundShanmenAuthority 非空；后者同时检查 Ready、Profile owner 和存储根。
- V3 库存/快捷使用以及旧 Code B observer 使用这个谓词决定新/旧路由；这是静态调用图事实。
- 隔离测试实际迁移一份含三颗治疗丹的 Profile，整备、持久启动 Run，设置生命为 1；仅损坏该独立测试根的新物品主/备文件，并使用既有 WriteTemp 注入，使真实保存后的重开判定进入 RecoveryRequired。
- 原代码得到 0 Success / 1 Fail：路由归属保持、技术回滚保持、非零 Runtime/Profile 保持三条断言失败。原生退出为 0，仍是测试失败，原日志未改写。

本轮证明的是真实 Flow/服务状态转换和技术回滚保持缺口。没有构造玩家 UI 输入或通过真实 Manager 库存入口演示重复消费，不宣称真实玩家损失；Manager 的受影响分支由源码调用图确认。

## 3. 最小实现

1. UsesShanmenItemLifecycle 表达流程归属：已 materialized 或持有 pending settlement 时保留归属；启动前则要求匹配 owner/root 且服务 Ready 或 RecoveryRequired。
2. FindBoundShanmenAuthority 保持原来的 Ready 限制，用于判断命令能否执行。没有让处于恢复状态的服务接受资源变更。
3. StartPreparedRunDirect 选中新流程后若找不到 ready authority，明确 SessionNotReady/RecoveryRequired，禁止落回旧 Start。
4. 新流程的技术激活回滚在修改 Runtime 之前检查 ready authority；不可用时拒绝并保留原 Run 身份与 Runtime。
5. Unbind 仍清理既有生命周期状态；未绑定或等待稳定旧来源的正常 legacy 流程不被改成新权威。

未新增持久 schema、权威副本、RequestId 命名空间、存储服务、玩法能力或测试专用产品接口。其他调用者仍使用同一 Flow 入口。

## 4. 测试与改动驱动回归

新增两个注册测试：

- AuthorityRecoveryRouting：三颗丹、生命 1、已启动 Run；真实服务恢复失败后，归属仍为新流程，重复库存/快捷命令拒绝，技术回滚保留 Runtime、Run 和旧 Profile；显式 Unbind 后归属释放。
- AuthorityRecoveryBeforeStart：迁移后整备操作遇到真实保存恢复失败；未 materialize Run 也保持新归属，Start 明确失败关闭，旧 Profile 不重写。

原两个 ProductFlow 正常启动/终局和 Runtime 失败重建测试保留，专项合计 4 项。

新增精确 ProfilePreparationFlow 源码/头文件/测试文件映射到 Shanmen.0_0_10.Items.ProductFlow，叠加原有 demo_map.Profile 与 ItemEconomySchema 要求。新增七项检查器正反例，拒绝只有旧组或只有新组的证据；517/517 自检通过。

## 5. 验证状态

- 修复后 Editor：SUCCEEDED，native 0，29 actions，126.74 秒。
- ProductFlow 专项：4 Success / 0 Fail，native 0，队列精确完成 4 项。
- 完整 Shanmen.0_0_10.Items：80 Success / 0 Fail，native 0，队列精确完成 80 项。
- 完整旧 demo_map 根：1,330 Success / 0 Fail，native 0，队列精确完成 1,330 项。
- Game：SUCCEEDED，native 0，28 actions，137.02 秒。
- 五个源码/脚本文件映射：PASS Changed=5 Rules=2 Required=3 Logs=2；发布范围另含三份文档，最终八文件检查见 Log。

专项四项包含在 Items 八十项中，不相加作为独立总数。每轮测试启动阶段存在既有 13 条 Condition failed 文本，不把日志称为零 Error/Warning。实际结果、队列完成、原生退出、Fatal/Ensure/Unhandled 联合核验；原失败单独保留。

## 6. 剩余范围

- FZ-1：本轮仅关闭 Ready 与路由归属混用及其回滚保持问题；整备/拾取/终局的全部入口与旧兼容边界仍需完成有限审计。
- FZ-2：激活早期回滚、最终释放与 EndPlay 的可达性及 owner 保持审计仍未整体关闭。
- FZ-3：前两项完成后才固定全产品输入，做最终完整新旧根和双目标证据，再发布冻结基线并暂停。
- 该修复不是整个 World 的跨进程原子恢复，也未实现历史裁剪、大档性能优化或玩家恢复 UI。

## 7. 交接与 P/F

精确八文件：Flow.cpp/.h、FlowTests.cpp、回归映射及自检、冻结索引、本 Report 和 Development Log。不混入总体报告、Saved 原日志、103 个既有未跟踪用户文件。

仅底层契约修复和无头验证；未接物理输入、修改正式地图/资产/玩法/数值/敌人/UI，未启动 Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook 或 Package。当前不暂停监控、不进入实际游戏性开发。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.1.r0_log.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
