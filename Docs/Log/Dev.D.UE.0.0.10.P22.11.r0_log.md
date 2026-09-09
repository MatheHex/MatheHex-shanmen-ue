# Dev.D.UE.0.0.10.P22.11.r0 Development Log

## 1. 基线与目标

- base：`a7969dee83bd94bcea327610addc1a434541965e`（P22.10 release-corridor clearance）；
- branch：`agent/0.0.10-p22-11-thrown-weapon-release-feedback`；
- 目标：把可证明的飞刀释放路径阻挡投影到既有 MainHUD 提示栈；
- 边界：不改库存、动作、轨迹、伤害、碰撞查询或释放点，不新增计时/轮询/第二权威，不启动 UI/PIE/产品 executable。

## 2. 审计结论

P22.9/P22.10 已在 durable commit 前拒绝终点占用和短距走廊阻挡，但 `TryStageLaunch()` 只返回 bool。上层因此只能看到通用 `ProjectileStageRejected`，HUD 无法安全区分真实墙体、状态冲突与协议错误。

两条玩家输入链最终都汇聚到同一个 `Fdemo_mapShanmenThrownWeaponSessionResult`。最小闭环应在底层产生类型化原因，由 ProductLifecycle 保留最新不可变结果，再由现有 MainHUD 读取；无需为 Straight/Arc 建两套反馈或把诊断字符串变成协议。

## 3. 底层类型化拒绝

`TryStageLaunch()` 增加可选 `OutError`，默认兼容原调用方。失败前默认为 `ContractRejected`；只有 `IsLaunchCorridorClear()` 的 endpoint overlap / Source-to-Origin sweep 阻挡写入 `ReleasePathBlocked`；成功和精确 staged replay 写入 `None`。

World Adapter 把该精确值映射为 `Edemo_mapShanmenThrownWeaponLaunchError::ReleasePathBlocked`。既有两个真实 World 测试的期望值同步收紧；世界不可用等一般暂存失败仍为 `ProjectileStageRejected`。

## 4. 生命周期只读证据

`ProductLifecycle` 新增 `LastHotbarRouteResult`：

- `TrySubmitHotbar()` 与 `TryRecoverCancellation()` 总是保存其返回结果；
- 首次成功绑定新 Session 时清空旧结果；
- 绑定失败清理、有效结束和空结束重放均清空；
- `IsValid()`、Host、Session、Authority 与 durable transaction 所有权不变。

生命周期测试新增 accepted、replay、SelectionId conflict 和 teardown 留痕断言。

## 5. 只读表现与 HUD

新增 `LaunchRejectionPresentation`，只有完整 Session→Product→Command→HostStart→Launch 身份链与 `ReleasePathBlocked` 同时成立才返回 `飞刀 · 释放路径受阻`。一般 stage rejection、action failure、accepted result 与身份缺失全部失败关闭。

MainHUD Hint Stack 增加 `LaunchRejection` kind，复用 `Blocked` tone。它与既有 terminal feedback 共用 rank 5 的唯一 outcome 槽，API 明确拒绝两者同时有效。HUD 优先读取 launch rejection；无精确拒绝时才投影 terminal receipt。

## 6. 聚焦验证

首次 Editor 构建一次通过：`121 actions / native 0 / 413.82s`，SHA-256 `7B62EF3C2298F9C6C7F4C323045A3F8B5EF731B7D176575043C8ACB2F5A5ACA2`。

聚焦测试一次通过，共 `19/0`：

- LaunchRejectionPresentation `3/0`；
- MainHUDCombatHintStackPresentation `3/0`；
- WorldDelivery `8/0`；
- ProductLifecycle `5/0`。

没有首轮失败或测试夹具修复。原始日志 SHA-256 依次为 `580AAB...C3E1`、`705CFD...A6C7`、`C890CA...1A34`、`B14D64...9850`。

## 7. 映射回归

16 个改动路径命中 5 条映射规则并产生 28 个必跑组。四份健康日志覆盖全部要求：

- `Shanmen.0_0_10`：1,254/0；
- `demo_map.InputRestore`：101/0；
- `demo_map.V2RangedCompatibility`：22/0；
- `demo_map.ItemUseAndArmor`：46/0；
- 合计：`1,423/0`。

覆盖门：`PASS Changed=16 Rules=5 Required=28 Logs=4`；门禁自测：`PASS 439/439`。总组中的联网探测超时只被 Automation Controller 记录为 warning；逐项结果、终止标记、原生退出码均成功，且无 fatal/unhandled/ensure 标记。

## 8. 构建与产物

- initial Editor：121 actions / native 0 / 413.82s；
- final Game：120 actions / native 0 / 390.62s；
- final Editor：up-to-date / native 0 / 1.23s。

最终 `demo_map.exe` SHA-256 为 `27705FE11ABA349E69CA48C21E2C55E9FEB9CF44A7B9E476CB95B850AE89A73D`；`UnrealEditor-demo_map.dll` SHA-256 为 `3B2C455AA00AEDFEF58940587026D06AF610DFBF34B4BD8085214DDE96130D73`。

## 9. 静态边界

非文档改动：生产 `+308/-24`，测试 `+220/-2`，回归映射 `+9/-0`。新表现层无 Timer、SetTimer、自定义 Tick、Sleep、Random/Rand/RNG、UWorld、AActor、UPROPERTY 或 UFUNCTION。

Regression Map JSON 可解析；`git diff --check` 为 0；无新增资产、模块依赖、持久数据、Actor、Subsystem 或第二套 UI/玩法权威。

## 10. 提交边界与 GitHub

精确提交 16 个实现/测试/映射文件与本 Report/Log，共 18 个文件。103 个用户原有 untracked 文件不暂存；raw evidence 留在 `Saved/Codex/P22.11` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-11-thrown-weapon-release-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-11-thrown-weapon-release-feedback/Docs/Report/Dev.D.UE.0.0.10.P22.11.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-11-thrown-weapon-release-feedback/Docs/Log/Dev.D.UE.0.0.10.P22.11.r0_log.md>
