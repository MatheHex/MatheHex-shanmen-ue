# Dev.D.UE.0.0.10.P27.10.r0 Development Log

## 1. 目标

- 将阵图已冻结的激活成本接入唯一共享 Run `SpiritEnergy` 权威；
- 让 ProductHost 启动与 reserve/commit 形成单一发布事务；
- 对余额不足和证明失配失败关闭，不留下 Host、扣款或 pending reservation；
- 保证同 Intent 恢复与成功重放不再次占号或扣费；
- 完成改动驱动回归、两目标构建、Report/Log 提交与 GitHub 推送。

## 2. 基线与范围

- 基线：`5ee80ac6a1ecf935383ac5bc2f4944872af6367f`（P27.9）；
- 分支：`agent/0.0.10-p27-10-formation-energy-transaction`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不修改 Profile/schema、迁移、物品真值、正式内容、按键、UI、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、Cook 或 Package。

## 3. 确定性外部事务身份

Formation ProductAuthority 新增 `MakeActivationEnergyTransactionId` 与 `MakeActivationEnergyCommandId`。两者都从
有效冻结 Command 的 CommandId 与阵图 CostId 派生，命名空间分别为
`demo_map.Formation.SharedSpiritEnergyTransaction.r1` 和
`demo_map.Formation.ActivationEnergyCommand.r1`。代码路径不使用随机 GUID/RNG。

## 4. Controller 原子组合

Formation Controller/RunLifecycle 的 `TrySubmit` 签名增加必需的
`Fdemo_mapShanmenDivineSenseProductController&`；不存在保留旧行为的无资源重载。

`StartCaptured` 先构造 ProductHost 候选，再复制灵力控制器并调用既有共享事务入口。回调在资源候选上：

1. 捕获共享灵力快照；
2. 用冻结 Action、Startup、阵图 Cost 创建 reservation request；
3. reserve 并保留 reservation receipt；
4. 用 Active commit-point 创建 finalization request；
5. commit，要求恰好一个已完成事务且 pending 数量为 0。

结果核对完成后才同时发布两个候选。Controller result 保留 reserve、commit 和共享事务三层证明。

## 5. 失败、恢复与重放

`SharedResourceRejected` 保留已冻结 Intent，但不发布产品/资源候选；`StateDesynchronized` 拒绝任何无法闭合的
复合证明。余额不足自动化验证 5 点资源拒绝 10 点成本时余额和外部事务数不变。替换为同一 Run 的 100 点共享
控制器后，完全相同 Intent 成功恢复至 90；再次提交只返回同一收据，序号、余额和事务数不变。

成功启动后激活成本已 Commit，不由普通产品结束退款。提交前的任意局部失败依靠候选副本丢弃实现原子回滚。

## 6. 测试结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationProductController` | 3 | 0 | `01E348295BD1E6CD5A512F71A16FA3E30ADA2AEDF3123148A207333DE4FFC7D8` |
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 6 | 0 | `DC4FA4475913E4DC1ACE2B68795C78059CFFC7AC5FF56AC839725E6D041019FF` |
| `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `11D71C4B3CFDD74498A5CD63064635411863937AB437551EEF29DC31844BEA49` |
| `Shanmen.0_0_10.Product.DivineSenseProductController` | 5 | 0 | `B5EAFE3C24B5F1EE7D995BB98B1A26505DF82B8187642EDBE8320644DF89DB3B` |
| `Shanmen.0_0_10.Product.SpiritShieldProductSession` | 9 | 0 | `6322BCDF3FE7C1F12EFAB331DD9D03F41A24096BB852F85B90EE83A7CCF9F089` |
| `demo_map.V3.Attributes` | 4 | 0 | `A531B842A244867C098EB4078A6D1E12727F614E3FA871F805E52898400556EA` |
| `Shanmen.0_0_10` | 1,345 | 0 | `548D400ADF08F9CD3EE0ED4ADE6F6819F62045FD6F750D3A4C6677A897BBCCFB` |

所有进程均为原生退出 0、0 Fail、0 Fatal/Unhandled/Ensure，并带 UE 5.8
`TEST COMPLETE. EXIT CODE: 0` 原生终止证据。根组发现并完成 1,345 项。完整串行根组约耗时 79 分 20 秒；
慢点集中在未改动的 SwordRhythm/ThrownWeapon 多层 checkpoint/恢复证明链，其反复深度 `IsValid/Matches`
产生 AutomationController large-delta 记录，但最终没有挂起或失败。

## 7. 回归与静态检查

- regression map JSON：PASS；
- 映射器正反自测：`472/472` PASS；
- 回归映射为 Controller/Lifecycle 新增 ActionResource、DivineSense Controller/Host/Router 与
  SpiritShieldProductSession；
- 最终覆盖门：PASS（Changed=9，Rules=4，Required=44，Logs=7）；
- 新增生产 diff 240 行；随机 GUID/RNG、物理输入、UI、Profile/schema、`UWorld`/`AActor`、正式能量金额
  字面量扫描均为 0；
- `git diff --check`：PASS。

## 8. 构建与启动器修复

统一启动器首次在源码编译前退出 1。保留证据：

- `run-state.json` SHA-256：`4C8368274E7CDF1621436475D9943C491511279048DFADC4C2884FE2FE30644E`；
- `stderr.log` SHA-256：`268188A59181B478A147EC72B1BCFA5AA62B3BB7B4F4A946CEFC615CD74D5074`；
- 原因：Build.bat 已引用的命令被 `Start-Process` 再次编码，`cmd.exe` 把起始引号当成路径字符；
- 修复：`Invoke-ShanmenBuild` 直接启动 Build.bat，参数继续保持数组，不再拼接 `/s /c` 字符串。

修复前已用原生 Build.bat 证明本轮源码可编译。修复后的统一启动器先用 Windows 批处理入口探针确认直接启动
可用，再完成：

- Editor 38/38，`SUCCEEDED`，原生退出 0，89.246 秒；stdout SHA-256
  `F7078A6A563C2307FFD954D67AD772A8660E55F2149AD34D99AFF78517BC397F`；
- `UnrealEditor-demo_map.dll` 19,573,760 bytes，SHA-256
  `C05C42FE6B4DAD9DB6AFEB2441101CD78DB2B8391D44BBE0A63CA559E1953A31`；
- Game 39/39，`SUCCEEDED`，原生退出 0，76.264 秒；stdout SHA-256
  `40061DAF204F2D2C6EB9A00EA959DF76F5C45F02332729C0B08B6AD49A88FFC3`；
- `demo_map.exe` 360,210,432 bytes，SHA-256
  `E4561412FD7965304342850CA05906B8D81B12C5E1EF2450A6E247EEF2A924E0`；
- 两次 stderr 均为空；其 SHA-256 均为
  `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

## 9. 边界与下一步

未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。下一阶段应把
已选择阵图组合进既有玩家输入端口，并强制沿本轮新增的共享灵力参数调用；不得恢复无扣费入口或建立第三套
资源余额。

## 10. 精确提交清单

1. `Scripts/Shanmen.Foundation.psm1`
2. `Scripts/ShanmenRegressionMap.json`
3. `Source/demo_map/demo_mapShanmenFormationProductAuthority.h`
4. `Source/demo_map/demo_mapShanmenFormationProductAuthority.cpp`
5. `Source/demo_map/demo_mapShanmenFormationProductController.h`
6. `Source/demo_map/demo_mapShanmenFormationProductController.cpp`
7. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.h`
8. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.cpp`
9. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
10. `Docs/Report/Dev.D.UE.0.0.10.P27.10.r0_report.md`
11. `Docs/Log/Dev.D.UE.0.0.10.P27.10.r0_log.md`

`Saved/Codex/P27.10` 与首次失败证据目录不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-10-formation-energy-transaction>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-10-formation-energy-transaction/Docs/Report/Dev.D.UE.0.0.10.P27.10.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-10-formation-energy-transaction/Docs/Log/Dev.D.UE.0.0.10.P27.10.r0_log.md>
