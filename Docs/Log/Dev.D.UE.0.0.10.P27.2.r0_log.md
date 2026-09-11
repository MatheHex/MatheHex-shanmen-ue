# Dev.D.UE.0.0.10.P27.2.r0 Development Log

## 1. 目标

- 把 P27.1 唯一阵法产品控制器接入 Combat Run 终止生命周期；
- 固定“先产品清理、后共享 Run 身份释放”的唯一顺序；
- 在 Coordinator 后置释放失败时保存产品终止检查点，精确重试不重复副作用；
- 阻止待释放 Run 接受新的阵法意图或消耗新激活序号；
- 完成改动驱动回归、Report/Log 提交和 GitHub 推送。

## 2. 基线与范围

- 基线：`bc5eb32f6d3e74a29b5991358b289d798dcc9eec`（P27.1）；
- 分支：`agent/0.0.10-p27-2-formation-run-lifecycle`；
- 起始 tracked tree clean；既有未跟踪用户文件保持未暂存；
- 不改阵法定义、材料数量、影响数值、物品 schema、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. Formation Run Lifecycle

新增 `Fdemo_mapShanmenFormationRunLifecycle`：

1. `TryBegin()` 绑定一个精确 Ready Combat Run，并拥有唯一 P27.1 控制器；
2. `TrySubmit()` 只委托该控制器，继续复用既有权威、序号和 Host；
3. `TryEndRun()` 先执行阵法取消/产品清理，再释放 Combat Run；
4. 两步完成后才原子清空生命周期状态；
5. 只提供控制器 const 视图和 const 产品终止检查点查询。

无活动生命周期、无效状态、Coordinator 未活动、Run 错配、产品清理拒绝和 Coordinator 释放拒绝
分别返回明确状态，不以布尔值掩盖故障位置。

## 4. 前向恢复检查点

产品清理可能修改物品权威并移除世界对象，不能安全回滚。实现采用前向检查点：

- 首次产品清理成功后保存完整 `Fdemo_mapShanmenFormationControllerEndSummary`；
- Coordinator 若拒绝释放，生命周期保留 RunId，控制器必须为空；
- 检查点存在时拒绝新提交，不访问权威、不消耗序号；
- 同 Run 的重复 `TryBegin()` 只确认待释放状态；
- 精确 `TryEndRun()` 重试复用同一 teardown receipt，只重试 Coordinator；
- 成功释放后统一 `Clear()`，不留下可重入产品状态。

## 5. 产品测试

在既有真实物品权威/世界 fixture 中新增两项：

- `OrderedEnd`：产品启动、同 Run begin 幂等、产品优先 teardown、共享绑定后置释放及重复结束拒绝；
- `CoordinatorRecovery`：注入玩家实体改绑故障，使 Coordinator 后置释放失败；验证唯一检查点、
  控制器为空、新工作隔离、序号不变、身份修复后只释放 Coordinator、物品 snapshot 不变。

专项结果：2 Success / 0 Fail；SHA-256
`3E37530EBE8BFBF01459BACD978A31956A42497CFEFDF2199E8588FCDF723747`。

## 6. 自动化结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 2 | 0 | `3E37530EBE8BFBF01459BACD978A31956A42497CFEFDF2199E8588FCDF723747` |
| `Shanmen.0_0_10` | 1,326 | 0 | `6C60199CE3763D042614B5C52A200DA6203F845294ED0F9A6E06C35DD91C3A35` |
| `demo_map.V3.Attributes` | 4 | 0 | `EC19F7DFB0E6945FF7B7FA74D87D15546347CA8D16352ECA078D045BF7EAEEE3` |

每份最终日志均有至少一个 Success、0 Fail、0 Fatal/Unhandled/Ensure、终端队列完成标记和进程
退出码 0。

## 7. 改动驱动覆盖

新增 `FormationRunLifecycle` 映射规则，要求完整 0.0.10、focused lifecycle、P27.1 controller、
P27.0 authority、CombatRun、Formation Host/Session/material adapter、Items、WorldGameplay、
FormationDeployment 与 CombatCore。修改既有 Host 测试文件同时触发其历史阵法依赖组和旧版属性组。

最终门禁：`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=39 Logs=3`。

- 覆盖日志 SHA-256：
  `D7379D9573F9310988C56229901229E71DD9D20EC70B311F07F8359F262B140C`；
- 映射器自测：`463/463` PASS；SHA-256
  `7813A4064A20D651BEF0837D86D692BC8E17864065E2BED18C16D3472BE14EE6`。

## 8. 构建与静态检查

| Target | Result | Duration | SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 20.92s | `4E4EB36CC271375F0231EA6A9602CFA2782AEA4905A59C52D36FC561A83FC3CC` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 0.96s | `6889B80984A9839E00AD7AE7806C2162F030990BD2B4E4BEF010024A4BE1E14C` |

最终二进制：

- `demo_map.exe`：360,046,592 bytes；SHA-256
  `88009E89794CF4342E039E7B5731CC44C75EE81CCF2693AD370FFEB9B30E31A6`；
- `UnrealEditor-demo_map.dll`：19,373,056 bytes；SHA-256
  `D57EF9526BD5E043F45285CA759985997D8658BDAB8BF86D29104BFCC9F5ED3B`。

`git diff --check`、regression map JSON 解析与生命周期边界扫描均 PASS。边界扫描未发现 Actor
创建/扫描、RNG、`ApplyDamage`、输入、Widget 或 Tick 绑定。

## 9. 未执行项与后续边界

没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
本轮只声明 P 阶段 Run 终止顺序、单次产品清理与精确恢复契约成立，不声明阵法已通过玩家输入进入
可见世界。后续应先增加显式世界锚点操作网关，再单独增加输入采样适配；不得直接把生命周期耦合到
GameMode，也不得建立第二套产品或物品权威。

原始证据位于 `Saved/Codex/P27.2`，不进入 Git。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.h`
2. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.cpp`
3. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.2.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.2.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-2-formation-run-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-2-formation-run-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P27.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-2-formation-run-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P27.2.r0_log.md>
