# Dev.D.UE.0.0.10.P27.5.r0 Report

## 1. 结论

P27.5 已把 P27.2-P27.4 的阵法产品生命周期接入 `Ademo_mapGameMode` 所拥有的唯一 Combat Run
组合根。阵法现在与其他战斗产品共享同一个 `RunId`，由 GameMode 统一开始；结束时先完成阵法自有的
材料与 World 清理，再由既有最终所有者释放共享 Coordinator，最后由阵法生命周期确认释放并清空。

本阶段没有新增第二次 `Coordinator.TryEndRun()`，也没有新增按键、输入动作、正式阵图、配方或平衡
数值。现有 `TryEndRun()` 仍可独立使用，并以相同的两阶段原语保持向后兼容。

## 2. 阶段问题与范围

P27.4 已将设备事件稳定转换成阵法 Intent/Anchor Operation，但阵法生命周期仍只能由测试或未来调用方
独立持有。若直接把它接到物理输入，产品可能绕开 GameMode 的统一 Run 开始/结束顺序，或与受控武器
生命周期竞争释放同一个 Coordinator。

P27.5 只解决组合所有权：

- 将阵法生命周期纳入 GameMode 的陈旧状态门、Run 绑定和 Run 释放；
- 把阵法结束拆成“产品清理”和“共享 Run 已结束确认”；
- 保留可重试的产品清理 checkpoint；
- 增加外部 Coordinator 组合与真实 GameMode 组合测试；
- 更新改动驱动回归映射，覆盖 GameMode 共享组合根的既有产品契约。

## 3. 两阶段结束契约

`TryTeardownProduct()` 只执行阵法拥有的材料与 World 清理。成功后返回
`ProductTeardownComplete`，保存完整 `ProductTeardownCheckpoint`，并明确保持共享
`Fdemo_mapCombatRunCoordinator` 活跃。精确重试复用 checkpoint，不重复返还材料、不重复移除 Actor。

`TryAcknowledgeCoordinatorEnded()` 只在以下条件全部成立时清空阵法生命周期：生命周期有效且活跃、
产品清理 checkpoint 已存在、期望 Run 身份与阵法 Run 精确一致、共享 Coordinator 已不再活跃。
Coordinator 仍活跃、Run 身份漂移或 checkpoint 缺失都会失败关闭并保留证据。

原有 `TryEndRun()` 现在按“产品清理 -> Coordinator 精确结束 -> 结束确认”组合以上原语，因此独立调用方
的公开行为不变，同时 GameMode 可以安全地共享现有最终 Run 所有者。

## 4. GameMode 组合顺序

`TryActivateCombatRun()` 在既有战斗产品完成绑定后以当前 Coordinator 启动唯一
`FormationRunLifecycle`。若阵法绑定失败，GameMode 进入已有的有界回滚路径，不留下半绑定 Run。

`ReleaseCombatProductRun()` 在既有最终 Coordinator 所有者之前调用阵法产品清理；随后继续由
`Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun()` 唯一释放共享 Run。该调用成功后，
GameMode 以返回的精确 `RunId` 确认阵法释放，再结束固定时间线及其余外层状态。

GameMode 的陈旧状态检测、Coordinator 已结束后的空状态判定、`RunBound` 与 `RunReleased` 审计日志均已
包含阵法生命周期。没有新增并行 Run、隐藏 tick、第二本 ledger 或替代物品/World 权威。

## 5. 重试与恢复

若阵法产品清理成功、但后续共享 Run 释放失败，阵法保留耐久 checkpoint。再次释放时不会重做材料或
World 副作用。若外层失败路径已经使 Coordinator 变为 inactive，下一次调用可先验证 checkpoint 与
精确 Run，再清空阵法生命周期并完成既有空状态恢复。

这使“产品清理已经提交”和“共享身份已经释放”成为两个可审计状态，不会把后半段失败误当成需要重新
执行前半段，也不会让阵法自己抢占 GameMode 的最终 Coordinator 所有权。

## 6. 正反测试

`Shanmen.0_0_10.Product.FormationRunLifecycle` 新增两项测试，总数由 4 增至 6：

- `ExternalCoordinatorComposition`：创建真实锚点 Actor，证明产品清理移除 Actor、保留活跃
  Coordinator，精确重试不改变物品快照；活跃 Coordinator 确认被拒绝，错误 Run 被拒绝，外部精确
  结束后确认成功并清空生命周期；
- `GameModeComposition`：使用 transient GameMode、真实 ItemAuthority、Combat Run、固定时间线、
  玩家绑定与锚点 Actor，证明 GameMode 一次释放会移除阵法 Actor、清空阵法生命周期和时间线、结束
  Coordinator，并解除玩家战斗实体绑定。

既有四项生命周期测试继续覆盖 Begin、正常 End、Coordinator 恢复、锚点操作与顺序结束，没有回归。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 6 | 0 | `8BB2A3947D01932C013066D78AB7C9BFC3D72533A469992393C831E2F0D17ABC` |
| `Shanmen.0_0_10` | 1,333 | 0 | `D845A51FE829C1C430E304A4DD78A8FB6F5D4CC8A74D8B6BC1628EAED8840CF0` |
| `demo_map.V3.Attributes` | 4 | 0 | `B6CFB71403341643AA0CE25C14409812111BB2B2F12606E35C963B2054C28FC3` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `74EB5D90E44AF269CA74D91533D8A25350993BA45D97EF38C146E1CC66A58960` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `9C472863A94E54932A0668AE0AD339A075C7AE88B016BD6F880A5F7D42265985` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `237CB6FEFD503025D564A47AD0AF996FF9AC5FBF2A3A4715EDA49FFDE4815807` |

六份日志均为 0 Fail、0 Fatal/Unhandled/Ensure/Assertion，并包含 UE 自动化队列完成证据；对应测试
进程原生退出码均为 0。四组既有 GameMode 改动回归合计 116/116 Success。

## 8. 改动驱动回归、构建与静态检查

6 个实现/测试/映射路径命中 3 条规则，最终覆盖门为：
`REGRESSION_COVERAGE: PASS Changed=6 Rules=3 Required=98 Logs=5`。
映射器正反自测 `465/465` PASS，regression map JSON 解析通过。

| Target | Result / native exit | Final incremental duration |
|---|---|---:|
| `demo_map` Win64 Development | Up to date, Succeeded / 0 | 0.91s |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 0.92s |

最终 `demo_map.exe` 为 360,107,008 bytes，SHA-256
`D6805EC36CD99B0ACEB7E8F3610B00F3729B361F5B027A71158FC44509B13603`；
`UnrealEditor-demo_map.dll` 为 19,447,808 bytes，SHA-256
`A850EC53712C907330F72429C23B7852D527898423D9D606F77812B9CDD7E8B5`。

`git diff --check` 通过；新增生产代码行 scoped scan 未发现 `FMath::Rand`、`FGuid::NewGuid`、
`EKeys`、`InputAction` 或测试阵图身份。没有新增正式内容资产或修改地图。

## 9. P/F 边界与下一阶段

P 阶段证明了 GameMode 唯一 Run 组合、阵法两阶段结束、checkpoint 重放、错误确认拒绝、实际 Actor
清理、玩家绑定释放、完整 0.0.10 回归和改动驱动覆盖。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、
Cook 或 Package。因此不声明玩家已经能从正式内容选择阵图或通过按键部署阵法。规划中的正式阵图配方、
效果与数值仍未冻结；下一阶段应先接入经确认的正式阵图选择/内容端口，再把物理输入路由到 P27.4
适配器，不能把测试 fixture 身份当成 shipping 内容。

## 10. GitHub 交接

基线提交：`6496be22f2450c13e7235d93b1a490e6ef958621`（P27.4）。
分支：`agent/0.0.10-p27-5-formation-run-composition`。本阶段只提交 6 个实现/测试/映射文件、本 Report
与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.5` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-5-formation-run-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-5-formation-run-composition/Docs/Report/Dev.D.UE.0.0.10.P27.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-5-formation-run-composition/Docs/Log/Dev.D.UE.0.0.10.P27.5.r0_log.md>
