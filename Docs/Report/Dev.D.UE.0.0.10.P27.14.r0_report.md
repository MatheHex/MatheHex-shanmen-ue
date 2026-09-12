# Dev.D.UE.0.0.10.P27.14.r0 Report

## 1. 结论

P27.14 已把 0.0.10 人工规划中的“阵法熟练度改变操作方式”落成独立、纯函数、失败关闭的能力契约 `FShanmenFormationMasteryPolicy`。

契约固定三档真实熟练度与三种不同操作：入门近身填充阵眼、中级远程投材、大成散材成阵。高级档位保留所有低级操作，不把熟练度降格为数值加成，也不在缺少产品决定时发明按键、距离、施法时间、耗材数量、UI 或 World 行为。

专项 2/2、既有 FormationDeployment 4/4、CombatCore 9/9、完整 `Shanmen.0_0_10` 1,357/1,357、Editor/Game 两目标构建、映射自测与改动驱动覆盖门全部通过。

## 2. 阶段问题与范围

截至 P27.13，项目已有真实阵法开始和逐锚点产品路由，但熟练度规划仍只存在于讨论文本中。若直接把三档逻辑散落进 PlayerController、UI、World Actor 或材料事务，后续会出现多个调用点各自解释“入门/中级/大成”的风险。

本轮只建立统一能力语义：

- `Beginner`：允许 `ProximityFill`；
- `Intermediate`：保留 `ProximityFill`，新增 `RemoteThrow`；
- `Master`：保留前两项，新增 `ScatterFormation`；
- `Invalid` 与伪造枚举值不是合法熟练度或操作；
- 查询无副作用，不读取 World、Actor、输入、物品、角色属性或随机数。

本轮不把策略接入现有产品入口，因此不会在熟练度来源尚未确定时制造第二权威。

## 3. 新增契约

新增 `EShanmenFormationMasteryTier`：

1. `Invalid`：未初始化哨兵；
2. `Beginner`：入门；
3. `Intermediate`：中级；
4. `Master`：大成。

新增 `EShanmenFormationMaterialDeliveryMode`：

1. `Invalid`：未初始化哨兵；
2. `ProximityFill`：玩家走到每个阵眼并填充材料；
3. `RemoteThrow`：远程把材料投入阵眼；
4. `ScatterFormation`：短动作后散出材料并一次成阵的操作资格。

两个 `Invalid` 仅用于失败关闭，不构成第四档熟练度或第四种玩法。

## 4. 不可变能力策略

`FShanmenFormationMasteryPolicy::TryCreate()` 只接受三个正式档位，并在失败前把输出重置为无效默认值，防止调用方误用上一份有效状态。

策略只冻结一个档位，外部只能通过只读 getter、`CanUseDeliveryMode()` 与 `GetHighestUnlockedDeliveryMode()` 查询。所有判断使用显式分支，不依赖枚举数值大小，因此未来调整枚举布局不会静默改变解锁关系。

`Master` 也必须先通过操作枚举白名单；无效或伪造操作不会因最高档位而被放行。

## 5. 能力矩阵证明

自动化逐格验证完整 3×3 能力矩阵：

| Mastery | ProximityFill | RemoteThrow | ScatterFormation | Highest |
|---|---:|---:|---:|---|
| Beginner | 是 | 否 | 否 | ProximityFill |
| Intermediate | 是 | 是 | 否 | RemoteThrow |
| Master | 是 | 是 | 是 | ScatterFormation |

测试另行证明默认策略无效、三个正式档位均可捕获、伪造档位失败、失败捕获清除旧状态、无效/伪造操作失败关闭，以及所有正式档位拒绝 `Invalid` 操作。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `9A51026286B4B20A685E7C41CD7BEE4EB2452B7EE38AD8CCF39410887C0FCA67` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `981E5D1EB50B7081BFD8A724A95090F6D33CD2BA0FCBFF88A36C62010D625AED` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `1539AF0AECA99F7108639722852C72A1FD68B3E7D192D74AFF7C875DAAD30D4A` |
| `Shanmen.0_0_10` | 1,357 | 0 | `18768854DF2EAB2D5DE95AC4BD0AB9152BF5A2F1DA28ABA0CB424C6B2C1C1813` |

四份日志均有 UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、测试进程原生退出 0、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组从 19:34:23.100 UTC 运行至 20:41:30.780 UTC，共 67 分 7.680 秒。

## 7. 改动驱动回归

新增 `FormationMasteryPolicy` 映射规则。三个契约路径必须有以下五组证据：完整 `Shanmen.0_0_10`、专项 FormationMastery、既有 FormationDeployment、CombatRuntime 与 CombatCore。

- regression map JSON：PASS；
- 映射器正反自测：`480/480` PASS；
- 负向自测证明只有 FormationMastery 专项不能替代部署、核心、广域与完整回归；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=5 Logs=4`；
- `git diff --check`：PASS。

## 8. 构建与静态边界

- Editor：8 actions，`SUCCEEDED`，原生退出 0，44.80 秒；stdout SHA-256 `67E33D484D3A0ECF864737090576BDD6EA30C3530055E9B0001DB3EE14AF0243`；
- `UnrealEditor-ShanmenCombatRuntime.dll`：2,057,216 bytes，SHA-256 `DFC994331B4FF0A195E73FCEAEE2D4FDB4588009C380706181A0AD66A65831DA`；
- Game：5 actions，`SUCCEEDED`，原生退出 0，33.37 秒；stdout SHA-256 `07FF811BC3FEBCF8BD3C8DD40D1D0090BC3578875520314CB4D34A90CE7B8119`；
- `demo_map.exe`：360,284,672 bytes，SHA-256 `4AEA6F5D61187FD8AA3F3E7EA8CB149127EF30F6C11EF14DA6F76EB37370D453`；
- 两次 stderr 均为空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增生产代码 118 行非空行，测试 104 行非空行。生产契约没有 `demo_map`、World、Actor、输入、控制器、GameMode、计时器、异步、随机 GUID/RNG、Profile/schema、伤害或物品依赖。

## 9. P/F 边界与下一阶段

P 阶段已证明三档能力矩阵、向上兼容、最高解锁操作、默认/伪造值失败关闭、失败捕获清空，以及完整回归、覆盖门和双目标构建。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。因此本轮不声明玩家已能实际投材或散材成阵。

下一阶段应先确定熟练度的唯一产品真值来源，再把只读策略接到现有 P27.12/P27.13 路由。物理键位、瞄准距离、动作时长、耗材批处理与表现仍须由正式产品决策定义，不能从本契约反推。

## 10. GitHub 交接

基线提交：`c3b889877423741a28602582af4c11ac3d9eee85`（P27.13）。分支：`agent/0.0.10-p27-14-formation-mastery-contract`。本阶段只提交 3 个新增源/测试文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.14` 与构建证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-14-formation-mastery-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-14-formation-mastery-contract/Docs/Report/Dev.D.UE.0.0.10.P27.14.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-14-formation-mastery-contract/Docs/Log/Dev.D.UE.0.0.10.P27.14.r0_log.md>
