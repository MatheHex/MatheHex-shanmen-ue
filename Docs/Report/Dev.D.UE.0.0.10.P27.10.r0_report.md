# Dev.D.UE.0.0.10.P27.10.r0 Report

## 1. 结论

P27.10 已把 P27.9 的阵图激活能量成本接入既有 Run 级共享 `SpiritEnergy` 权威。阵法控制器现在只能通过
显式传入的共享灵力控制器启动；产品 Host 启动和资源 reserve/commit 都先在候选副本中完成，且只有两侧证明
一致时才一起发布。余额不足、账本不匹配或证明失配时，两侧均不落地，冻结的同一 Intent 可以原样重试。

同一成功 Intent 的重放返回原产品/资源收据，不再占用阵法序号，也不会重复扣费。没有创建阵法专用余额、
第二条资源账本或无扣费的兼容入口；正式能耗金额仍由阵图内容拥有。

## 2. 阶段问题与范围

P27.9 只能表达阵图需要多少能量，阵法启动仍未消费资源。若在后续输入层临时扣费，会产生产品已启动但资源
失败、资源已扣但产品未发布，或重放重复扣费三类分叉。

本轮只封闭启动事务：

- 复用 `Fdemo_mapShanmenDivineSenseProductController` 已持有的共享 Run 灵力权威；
- 复用 `FShanmenActionResourceAuthority` 的 snapshot、reserve 与 finalize/commit；
- 把现有 Formation ProductHost 启动与资源提交组合为一个发布点；
- 保留失败 Intent，允许资源条件恢复后精确重试；
- 扩展改动驱动回归映射，使阵法入口改动必须覆盖共享资源及既有消费者；
- 不接物理输入、UI、地图、Profile/schema 或正式内容资产。

## 3. 原子启动事务

`Fdemo_mapShanmenFormationProductController::TrySubmit` 现在要求调用者提供同一 Combat Run 的共享
`SpiritEnergyController`，旧的无资源参数入口已删除。冻结命令先在局部 `ProductHost` 候选上产生 Startup、Active
与 Deployment 证明；随后复制共享灵力控制器，在其候选账本中捕获快照、按阵图自带 Cost reserve，并使用
Active commit-point 收据 finalize 为 Commit。

共享 Host 原有的外部事务适配器只接受一次完整、余额不增加、revision 精确 `+2`、无 pending reservation 的
变更。回调、资源证明或产品证明任一失败，候选对象直接丢弃；只有全部一致才同时赋回正式 ProductHost 和共享
灵力控制器。

## 4. 身份与幂等

每次阵法能量事务均从冻结 Formation CommandId 与 CostId 派生两条稳定身份：

- `demo_map.Formation.SharedSpiritEnergyTransaction.r1`：共享账本事务身份；
- `demo_map.Formation.ActivationEnergyCommand.r1`：共享 Router 命令身份。

成功结果逐项核对行动 ActivationId、Startup/Active 序列、commit point、CostId、资源 owner/channel、前后快照、
revision、余额差和零保留量。同一 Intent 成功后由控制器直接返回保留证明，外部共享账本事务数量保持 1。

## 5. 失败关闭与恢复

新增 `SharedResourceRejected` 与 `StateDesynchronized` 状态。余额不足时，测试中的 5 点灵力无法支付 10 点阵图
测试成本：阵法 Host 不发布、共享余额仍为 5、外部事务数仍为 0，且阵法激活序号只在首次冻结时使用一次。
测试随后只替换为同一 Run 的已注资共享控制器，并以完全相同 Intent 重试；结果从 100 降到 90，产品启动，
再次重放后仍保持 90 和单一事务。

这里的 5、10、100 仅为自动化 fixture，不是正式平衡参数。成功激活后的成本是 Commit，不在普通阵法结束时
退款；提交前失败依靠候选副本丢弃保证无部分 reservation。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationProductController` | 3 | 0 | `01E348295BD1E6CD5A512F71A16FA3E30ADA2AEDF3123148A207333DE4FFC7D8` |
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 6 | 0 | `DC4FA4475913E4DC1ACE2B68795C78059CFFC7AC5FF56AC839725E6D041019FF` |
| `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `11D71C4B3CFDD74498A5CD63064635411863937AB437551EEF29DC31844BEA49` |
| `Shanmen.0_0_10.Product.DivineSenseProductController` | 5 | 0 | `B5EAFE3C24B5F1EE7D995BB98B1A26505DF82B8187642EDBE8320644DF89DB3B` |
| `Shanmen.0_0_10.Product.SpiritShieldProductSession` | 9 | 0 | `6322BCDF3FE7C1F12EFAB331DD9D03F41A24096BB852F85B90EE83A7CCF9F089` |
| `demo_map.V3.Attributes` | 4 | 0 | `A531B842A244867C098EB4078A6D1E12727F614E3FA871F805E52898400556EA` |
| `Shanmen.0_0_10` | 1,345 | 0 | `548D400ADF08F9CD3EE0ED4ADE6F6819F62045FD6F750D3A4C6677A897BBCCFB` |

最终日志均需满足原生退出 0、UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、0 Fail 与 0
Fatal/Unhandled/Ensure；上述日志全部满足。完整根组发现并串行完成 1,345 项。该额外安全网约耗时 79 分
20 秒；可复现的长停顿集中在既有 SwordRhythm/ThrownWeapon 多层 checkpoint 与恢复证明测试的重复深度
`IsValid/Matches` 校验，未出现挂起或失败，也不在本轮阵法改动路径内。

## 7. 改动驱动回归

Formation ProductController 与 RunLifecycle 的映射新增共享行动资源、Divine Sense Controller/Host/Router 和
Spirit Shield Session 五组要求，防止阵法入口只验证自身而漏测共享账本既有消费者。完整根组覆盖所有
`Shanmen.0_0_10.*` 要求，`demo_map.V3.Attributes` 继续以独立日志覆盖旧属性边界。

- regression map JSON：PASS；
- 映射器正反自测：`472/472` PASS；
- 最终覆盖门：PASS（Changed=9，Rules=4，Required=44，Logs=7）；
- `git diff --check`：PASS。

## 8. 构建与流程修复

首次经统一启动器构建时，在源码编译前因 `cmd.exe /s /c` 对已引用 Build.bat 命令再次加引号而退出 1；原始
失败状态与 stderr 已保留，SHA-256 分别为
`4C8368274E7CDF1621436475D9943C491511279048DFADC4C2884FE2FE30644E` 与
`268188A59181B478A147EC72B1BCFA5AA62B3BB7B4F4A946CEFC615CD74D5074`。这不是源码失败。

启动器已改为直接启动 Build.bat 并传递结构化参数，避免完整命令被二次引用；同一路径先通过 Windows 批处理
入口探针，再完成两目标真实复验：

- Editor：38/38，`SUCCEEDED`，原生退出 0，89.246 秒；stdout SHA-256
  `F7078A6A563C2307FFD954D67AD772A8660E55F2149AD34D99AFF78517BC397F`；
- `UnrealEditor-demo_map.dll`：19,573,760 bytes，SHA-256
  `C05C42FE6B4DAD9DB6AFEB2441101CD78DB2B8391D44BBE0A63CA559E1953A31`；
- Game：39/39，`SUCCEEDED`，原生退出 0，76.264 秒；stdout SHA-256
  `40061DAF204F2D2C6EB9A00EA959DF76F5C45F02332729C0B08B6AD49A88FFC3`；
- `demo_map.exe`：360,210,432 bytes，SHA-256
  `E4561412FD7965304342850CA05906B8D81B12C5E1EF2450A6E247EEF2A924E0`；
- 两次 stderr 均为空，空文件 SHA-256 为
  `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

## 9. P/F 边界与下一阶段

P 阶段证明了启动与共享灵力的一次性原子发布、余额不足无部分状态、同 Intent 恢复、成功重放不重复消费、
共享消费者兼容、改动驱动覆盖和两目标构建。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或
Package。下一阶段可以把 P27.8 的安全阵图选择组合进既有玩家输入端口；仍须复用本轮强制的共享灵力参数，
不得恢复无扣费入口或新增资源权威。

## 10. GitHub 交接

基线提交：`5ee80ac6a1ecf935383ac5bc2f4944872af6367f`（P27.9）。
分支：`agent/0.0.10-p27-10-formation-energy-transaction`。本阶段只暂存实现、测试、回归/构建流程修复、本
Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.10` 原始证据不进入
Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-10-formation-energy-transaction>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-10-formation-energy-transaction/Docs/Report/Dev.D.UE.0.0.10.P27.10.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-10-formation-energy-transaction/Docs/Log/Dev.D.UE.0.0.10.P27.10.r0_log.md>
