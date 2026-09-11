# Dev.D.UE.0.0.10.P27.1.r0 Report

## 1. 结论

P27.1 已在 P27.0 阵法权威命令之前建立 Run 级唯一产品控制器：一次有效意图只冻结一次物品与
CombatRun 权威、只消耗一个阵法激活序号，并由控制器独占既有
`Fdemo_mapShanmenFormationProductHost`。相同意图的精确重试回放原始启动证据，不重复预留身份；
相同 ID 的不同载荷和第二个并发意图均在再次访问权威前失败关闭。

本阶段仍不连接 GameMode、玩家输入、UI 或世界锚点投放，不新增材料、范围、持续时间或影响数值。

## 2. 阶段问题与范围

P27.0 能签发确定性投放命令，但调用者每次重试都会取得新激活序号，也没有对象负责限制一个 Run
同时拥有多少阵法 ProductHost。P27.1 新增：

- `Fdemo_mapShanmenFormationIntent`：在接触权威前捕获设备无关请求；
- `Fdemo_mapShanmenFormationProductController`：绑定一个 Combat Run，独占一个冻结意图与 Host；
- 可验证的启动结果、拒绝状态和 Run 结束摘要；
- 精确重放、身份冲突、并发占用、Run 边界及清理测试；
- FormationProductController 的改动驱动回归映射与正反自测。

## 3. 意图捕获契约

调用点必须提供有效 `IntentId`、当前 `RunId`、已验证阵图、有限原点和非零平面前向。捕获过程把
前向投影到 XY 平面后归一化；纯垂直、非有限或无效输入在访问持久物品权威和消耗 CombatRun 序号
之前被拒绝。

意图是一次调用边界的不可变值，不包含输入键、Pawn、Controller、Widget 或 World 查询，因此后续
输入适配层只能采样并提交，不能绕过 P27.0 权威组合。

## 4. 单一所有权与幂等重放

控制器按以下状态机工作：

1. `TryBegin()` 绑定一个精确 Combat Run；
2. 首个有效意图调用 P27.0 `PrepareDeployment()` 一次并冻结命令；
3. 命令成功启动既有 ProductHost 后，控制器成为该 Host 的唯一所有者；
4. 相同 ID、相同完整载荷的重试返回同一个 CommandId 与 Begin receipt；
5. 相同 ID、不同载荷返回 `IntentIdConflict`；不同 ID 返回 `HostBusy`；
6. `TryCancelAndEnd()` 取消/清理 Host 后原子清空 Run、意图和 Host 状态。

若 Host 启动在权威预留之后失败，冻结命令仍保留，只有精确重试可以再次尝试启动，且不会消耗第二
个序号。

## 5. 权威与产品边界

控制器只通过 `Fdemo_mapShanmenFormationProductAuthority` 取得命令，并继续复用既有 ProductHost、
Session、材料适配和部署链。它不直接读写档案/库存，不自造 Owner、SourceEntity、内容版本或
ActivationId。

控制器只暴露 ProductHost 的 const 视图和冻结命令查询，避免调用点取得可变 Host 后建立第二条生命
周期路径。

## 6. 正反测试

新增 `Shanmen.0_0_10.Product.FormationProductController` 两项：

- `FrozenReplayAndEnd`：真实 ActiveRun + CombatRun 启动、平面归一化、首次单序号、精确重放、
  相同命令/回执、物品权威只读、取消与清理；
- `Fences`：纯垂直输入、外部 Run、同 ID 异载荷、第二意图、活动 Run 重绑定全部失败关闭，并验证
  拒绝路径不增加序号。

专项最终结果为 2 Success / 0 Fail。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationProductController` | 2 | 0 | `D6F945CA...CAEA649` |
| `Shanmen.0_0_10` | 1,324 | 0 | `D4F75FEE...C61CAC83` |
| `demo_map.V3.Attributes` | 4 | 0 | `28831620...E4D4110` |

所有最终日志均须具有原生退出码 0、0 Fatal/Unhandled/Ensure，且包含终端队列完成标记。

## 8. 改动驱动回归、构建与静态检查

本阶段 5 个实现/测试/映射路径命中 2 条规则，最终覆盖门：
`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=38 Logs=3`，SHA-256
`AA606A003E38A3ACF4B23DC68CC9C55DFE901C990FD810CFE51D4070A551EAEA`。

映射器正反自测 `461/461` PASS，SHA-256
`C9E4BDF084B77F0CAD4D55B6CC678589904C273AD54EBB735BCAF0677054C665`。

| Target | Result / native exit | Duration | Log SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 17.74s | `1E676731...B339A14` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 0.92s | `08659575...C9237EC` |

最终 `demo_map.exe` 为 360,028,672 bytes，SHA-256
`3C49E6320D075DC631625EBF9DDC5DD178A5B5656ABEC53DFEF943B5A4E318EC`；
`UnrealEditor-demo_map.dll` 为 19,352,064 bytes，SHA-256
`3863684BD450974F797FC0A3072E05FFDC39F9ECB8D9969DB3A527B44F6454E3`。

`git diff --check`、regression map JSON 解析通过；新增控制器的 scoped scan 未发现 Actor 创建/扫描、
RNG、`ApplyDamage`、输入或 UI 绑定。

## 9. P/F 边界与下一阶段

P 阶段证明了 Run 级单一所有权、精确重放、冲突关闭、序号幂等、物品只读、Host 清理、完整
0.0.10 回归及改动文件驱动覆盖。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、
Cook 或 Package。因此不声明玩家已经能在可见世界中放置阵法。下一阶段应把唯一控制器接到现有
产品生命周期，再单独连接输入采样和显式世界锚点操作，继续禁止第二套权威或轮询路径。

## 10. GitHub 交接

基线提交：`b6becab5c254a68c964ffb270b7234231e4a3277`（P27.0）。
分支：`agent/0.0.10-p27-1-formation-product-controller`。本阶段只提交 5 个实现/测试/回归映射文件、
本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.1`
原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-1-formation-product-controller>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-1-formation-product-controller/Docs/Report/Dev.D.UE.0.0.10.P27.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-1-formation-product-controller/Docs/Log/Dev.D.UE.0.0.10.P27.1.r0_log.md>
