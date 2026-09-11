# Dev.D.UE.0.0.10.P27.2.r0 Report

## 1. 结论

P27.2 已把 P27.1 的唯一阵法产品控制器接入 Combat Run 终止生命周期，并固定唯一顺序：先完成阵法
产品清理，再释放共享 Combat Run 身份。阵法取消可能写入材料权威并移除世界对象，无法通过复制控制器
安全回滚；因此本阶段采用前向恢复：产品清理成功后保存精确终止检查点，若 Combat Run 释放失败，
后续精确重试只释放 Coordinator，不重复取消阵法、不重复修改物品或世界。

本阶段仍不连接 GameMode、玩家输入、UI 或新的世界采样，不新增阵法材料、范围、持续时间、影响或
战斗数值。

## 2. 阶段问题与范围

P27.1 能确保一个 Run 只有一个阵法 ProductHost，但调用者仍可在错误顺序中先结束 Combat Run，
留下无法使用共享身份完成清理的阵法产品。P27.2 新增：

- `Fdemo_mapShanmenFormationRunLifecycle`：同时拥有一个精确 Run 身份和唯一产品控制器；
- 有序 `TryEndRun()`：产品清理成功后才调用既有 Combat Run Coordinator；
- 持久于对象内的产品终止检查点和“是否复用检查点”证明；
- Coordinator 拒绝后的封闭状态，只允许精确释放重试；
- FormationRunLifecycle 的改动驱动回归映射与正反自测。

## 3. 唯一生命周期所有权

生命周期按以下状态工作：

1. `TryBegin()` 只绑定一个已 Ready 的 Combat Run，并启动 P27.1 唯一控制器；
2. `TrySubmit()` 继续委托 P27.1 控制器完成意图冻结、权威采样、序号预留和 Host 启动；
3. 正常 Run 结束先调用控制器的 `TryCancelAndEnd()`，取得有效产品终止摘要；
4. 产品清理成功后才调用 `Fdemo_mapCombatRunCoordinator::TryEndRun()`；
5. 两步均成功才清空生命周期、控制器、RunId 和检查点。

生命周期只暴露控制器 const 视图，调用点不能绕过它取得可变 Host 或自行建立第二条终止路径。

## 4. 前向检查点与精确恢复

产品取消会消耗真实的状态迁移：材料适配器可能提交返还/结算，世界交付链可能删除锚点与影响对象。
因此 Coordinator 若在第二步拒绝，生命周期不会假装回滚已经发生的副作用，而是：

- 保留原始 RunId 和完整 `Fdemo_mapShanmenFormationControllerEndSummary`；
- 保持生命周期 Active，但确保产品控制器已经 Empty；
- 拒绝所有新阵法意图，并且在拒绝前不消耗新的激活序号；
- 同一 Run 的 `TryBegin()` 只识别为待释放重放，不重新启动产品；
- 下一次 `TryEndRun()` 复用相同 teardown receipt，只重试 Combat Run 释放。

这使不可逆产品副作用至多发生一次，同时允许临时 Coordinator 身份故障被原位修复。

## 5. 失败关闭边界

无活动生命周期、生命周期不变量失效、Coordinator 未活动、RunId 错配、产品清理拒绝和 Coordinator
释放拒绝均有独立状态与诊断。产品清理检查点存在期间，任何新提交统一返回 ControllerInactive，
避免“旧 Run 正在释放、同一对象又接受新产品工作”的混合状态。

本阶段没有新增持久化格式；检查点属于同一进程内的产品生命周期恢复证据。进程级持久化或跨启动恢复
不在本阶段声明范围内。

## 6. 正反测试

新增 `Shanmen.0_0_10.Product.FormationRunLifecycle` 两项：

- `OrderedEnd`：真实物品权威、世界、玩家生命组件和 Combat Run 下启动阵法，验证产品 teardown
  receipt 在共享身份释放前产生，结束后控制器、生命周期和玩家绑定均清空，重复结束被拒绝；
- `CoordinatorRecovery`：故意把玩家生命组件改绑到错误实体，使 Coordinator 在产品清理后拒绝；
  验证检查点保留、控制器为空、新提交不消耗序号、同 Run begin 不重启；修复身份后精确重试复用
  同一 teardown receipt，且物品权威快照完全不变。

专项最终结果为 2 Success / 0 Fail。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 2 | 0 | `3E37530E...F723747` |
| `Shanmen.0_0_10` | 1,326 | 0 | `6C60199C...91C3A35` |
| `demo_map.V3.Attributes` | 4 | 0 | `EC19F7D...7EAEEE3` |

三份最终日志均具有原生退出码 0、0 Fatal/Unhandled/Ensure，并包含终端队列完成标记。

## 8. 改动驱动回归、构建与静态检查

本阶段 5 个实现/测试/映射路径命中 2 条规则，最终覆盖门：
`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=39 Logs=3`，SHA-256
`D7379D9573F9310988C56229901229E71DD9D20EC70B311F07F8359F262B140C`。

映射器正反自测 `463/463` PASS，SHA-256
`7813A4064A20D651BEF0837D86D692BC8E17864065E2BED18C16D3472BE14EE6`。

| Target | Result / native exit | Duration | Log SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 20.92s | `4E4EB36C...A83FC3CC` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 0.96s | `6889B809...4BE1E14C` |

最终 `demo_map.exe` 为 360,046,592 bytes，SHA-256
`88009E89794CF4342E039E7B5731CC44C75EE81CCF2693AD370FFEB9B30E31A6`；
`UnrealEditor-demo_map.dll` 为 19,373,056 bytes，SHA-256
`D57EF9526BD5E043F45285CA759985997D8658BDAB8BF86D29104BFCC9F5ED3B`。

`git diff --check`、regression map JSON 解析通过；新增生命周期的 scoped scan 未发现 Actor 创建/扫描、
RNG、`ApplyDamage`、输入、Widget 或 Tick 绑定。

## 9. P/F 边界与下一阶段

P 阶段证明了阵法产品先于 Combat Run 释放、不可逆清理的单次检查点、Coordinator 故障精确恢复、
恢复期间新工作隔离、完整 0.0.10 回归及改动文件驱动覆盖。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、
Cook 或 Package。因此不声明玩家已经能在可见世界中放置或结束阵法。下一阶段应在本生命周期/
控制器上增加显式世界锚点操作网关；输入采样应继续作为后续独立适配层，不应直接接入 GameMode
或建立第二套权威。

## 10. GitHub 交接

基线提交：`bc5eb32f6d3e74a29b5991358b289d798dcc9eec`（P27.1）。
分支：`agent/0.0.10-p27-2-formation-run-lifecycle`。本阶段只提交 5 个实现/测试/回归映射文件、
本 Report 与本 Development Log；所有既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.2`
原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-2-formation-run-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-2-formation-run-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P27.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-2-formation-run-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P27.2.r0_log.md>
