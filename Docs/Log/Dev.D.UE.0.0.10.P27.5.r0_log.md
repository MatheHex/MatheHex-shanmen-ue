# Dev.D.UE.0.0.10.P27.5.r0 Development Log

## 1. 目标

- 把 P27.2-P27.4 的阵法生命周期接入 GameMode 唯一 Combat Run 组合根；
- 分离阵法自有产品清理与共享 Coordinator 释放，消除竞争结束；
- 保留可重试 checkpoint，避免后段失败导致材料/World 副作用重做；
- 使用真实外部组合和 GameMode 组合测试证明完整开始/结束；
- 完成改动驱动回归、Report/Log 提交及 GitHub 推送。

## 2. 基线与范围

- 基线：`6496be22f2450c13e7235d93b1a490e6ef958621`（P27.4）；
- 分支：`agent/0.0.10-p27-5-formation-run-composition`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不新增按键、Enhanced Input Action、正式阵图、配方、效果、数值、地图或资产；
- 不改变物品、World、Combat Run 或既有战斗产品的权威归属；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. 生命周期拆分

在 `Edemo_mapShanmenFormationRunLifecycleEndStatus` 增加
`ProductTeardownComplete`，并在结束结果增加 `IsProductTeardownComplete()`。

新增 `TryTeardownProduct()`：

1. 校验阵法生命周期、Coordinator 与精确 Run 身份；
2. 调用既有 ProductHost teardown 完成材料与 World 清理；
3. 保存耐久 `ProductTeardownCheckpoint`；
4. 重试时复用相同 receipt，不重复执行副作用；
5. 不结束共享 Coordinator。

新增 `TryAcknowledgeCoordinatorEnded()`：只有 checkpoint 存在、期望 Run 与生命周期一致且 Coordinator
已 inactive 时才 `Clear()`。所有不一致均失败关闭并保留生命周期。

原 `TryEndRun()` 改为顺序组合 teardown、Coordinator end、acknowledgement，继续服务独立调用方。

## 4. GameMode Run 绑定

`Ademo_mapGameMode` 新增唯一成员 `FormationRunLifecycle`，并把它加入 Run 激活前的陈旧状态检查。
既有产品绑定完成后，GameMode 以当前 `CombatRunCoordinator` 调用 `TryBegin()`；失败时使用已有
`ReleaseCombatProductRun("FormationProductBindFailure")` 路径做有界回滚。

`RunBound` 审计事件新增 `FormationLifecycle` 标志，能直接证明阵法是否与同一个 Run 完成绑定。

## 5. GameMode Run 释放

释放顺序新增以下阶段：

1. 在最终共享 Run 所有者之前执行 `FormationRunLifecycle.TryTeardownProduct()`；
2. 由既有 `Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun()` 唯一结束 Coordinator；
3. 用其返回的精确 `RunId` 调用阵法 acknowledgement；
4. 继续结束固定时间线和外层路由状态。

若重试时 Coordinator 已 inactive，只有带有效 checkpoint 的阵法生命周期可以被确认清空。陈旧状态门、
inactive 空状态检查以及 `RunReleased` 日志同步包含阵法状态和 teardown 结果。

## 6. 测试结果

新增：

- `FormationRunLifecycle.ExternalCoordinatorComposition`；
- `FormationRunLifecycle.GameModeComposition`。

第一项以真实 ItemAuthority、World 和锚点 Actor 证明两阶段清理、checkpoint 重放、活跃 Coordinator
拒绝、错误 Run 拒绝和精确外部结束确认。第二项以 transient GameMode、玩家健康组件、固定时间线和
锚点 Actor 证明统一释放顺序及完整清空。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 6 | 0 | `8BB2A3947D01932C013066D78AB7C9BFC3D72533A469992393C831E2F0D17ABC` |
| `Shanmen.0_0_10` | 1,333 | 0 | `D845A51FE829C1C430E304A4DD78A8FB6F5D4CC8A74D8B6BC1628EAED8840CF0` |
| `demo_map.V3.Attributes` | 4 | 0 | `B6CFB71403341643AA0CE25C14409812111BB2B2F12606E35C963B2054C28FC3` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `74EB5D90E44AF269CA74D91533D8A25350993BA45D97EF38C146E1CC66A58960` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `9C472863A94E54932A0668AE0AD339A075C7AE88B016BD6F880A5F7D42265985` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `237CB6FEFD503025D564A47AD0AF996FF9AC5FBF2A3A4715EDA49FFDE4815807` |

所有测试进程原生退出 0；日志为 0 Fail、0 Fatal/Unhandled/Ensure/Assertion，并包含自动化完成证据。

## 7. 改动驱动覆盖

`Scripts/ShanmenRegressionMap.json` 的 `M01GameMode` 规则增加阵法生命周期测试要求。实现、测试和
映射共 6 个 changed path 命中 3 条规则；五份覆盖输入日志证明 98 个必需组：

`REGRESSION_COVERAGE: PASS Changed=6 Rules=3 Required=98 Logs=5`

映射器正反自测 `465/465` PASS；JSON 解析通过。GameMode 修改触发四组旧系统回归，合计
116/116 Success，而不只依赖本轮主题测试。

## 8. 构建与二进制

最终顺序执行：

| Target | Result | Duration |
|---|---|---:|
| `demo_map` Win64 Development | Up to date, Succeeded / native 0 | 0.91s |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / native 0 | 0.92s |

二进制：

- `demo_map.exe`：360,107,008 bytes；SHA-256
  `D6805EC36CD99B0ACEB7E8F3610B00F3729B361F5B027A71158FC44509B13603`；
- `UnrealEditor-demo_map.dll`：19,447,808 bytes；SHA-256
  `A850EC53712C907330F72429C23B7852D527898423D9D606F77812B9CDD7E8B5`。

## 9. 静态边界与下一步

`git diff --check`、regression map JSON 解析均 PASS。新增生产代码行未出现 `FMath::Rand`、
`FGuid::NewGuid`、`EKeys`、`InputAction` 或测试阵图身份；没有新增输入轮询、正式内容或第二个
Coordinator 结束调用。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
下一阶段先建立经确认的正式阵图选择/内容端口，再将物理输入事件交给 P27.4 适配器。规划尚未冻结的
阵图配方、效果和数值不得从测试 fixture 推导为 shipping 内容。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.h`
2. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.cpp`
3. `Source/demo_map/demo_mapGameMode.h`
4. `Source/demo_map/demo_mapGameMode.cpp`
5. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
6. `Scripts/ShanmenRegressionMap.json`
7. `Docs/Report/Dev.D.UE.0.0.10.P27.5.r0_report.md`
8. `Docs/Log/Dev.D.UE.0.0.10.P27.5.r0_log.md`

`Saved/Codex/P27.5` 原始证据不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-5-formation-run-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-5-formation-run-composition/Docs/Report/Dev.D.UE.0.0.10.P27.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-5-formation-run-composition/Docs/Log/Dev.D.UE.0.0.10.P27.5.r0_log.md>
