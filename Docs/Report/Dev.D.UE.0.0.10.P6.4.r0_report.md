# Dev.D.UE.0.0.10.P6.4.r0 Report

## 1. 结论

P6.4 已完成受控武器的产品 Actor / movement owner，结论为 **PASS**。

`Fdemo_mapShanmenControlledWeaponProductController` 现在把 P6.1 的 exact deployed item 证据、P6.2 的唯一 Session、P6.3 的 World delivery 与外部提供的真实飞剑 Actor 收进一个产品边界。它使用产品冻结的速度执行 swept movement，保管唯一接触窗口，并继续只经 Coordinator 写入 vitality；没有臆造正式飞剑物品、按键、生成规则、碰撞频率或数值内容。

## 2. 功能性

- 启动时要求 Coordinator ready、Prepared Run 一致、来源 Actor 已注册为当前 player entity；
- 来源 Actor 与飞剑 Actor 必须不同，碰撞根必须由该飞剑 Actor 拥有且为实际 RootComponent；
- movement capture 在一次 Session 内冻结 `DirectedSpeed` 与 `MaximumStepSeconds`，不把测试数值写入产品默认值；
- Launch / Redirect 继续使用 P6.2 单调 command sequence，不允许产品层绕过 Recall 完成路径；
- Directed step 对同一个飞剑 Actor 调用 sweep-enabled `SetActorLocation`，返回请求位置、实际位置、方向、命令序号与 blocking-hit receipt；
- 过大、非有限或非正 DeltaSeconds 在移动前失败关闭；
- Controller 保管当前 `FShanmenWorldHitContext`，不允许重复开窗、旧窗口复用或开放窗口期间 Recall；
- sweep / overlap 只转交 P6.3 World adapter，Session 只在 canonical vitality delivery 成功后前移；
- Interrupt 同时关闭 action、emission 与 Controller context；Completed / Interrupted 后拒绝移动和控制。

## 3. 完整性

新增 3 个产品级自动化：

1. `BindingAndMotion`：覆盖 exact item / Actor 绑定、Orbiting 禁止移动、Launch、Redirect、冻结速度、swept movement receipt、超长 step 零副作用；
2. `ContactAndCompletion`：覆盖 Controller-owned window、sweep 与 overlap 的 canonical vitality、重复 callback 幂等、窗口/Recall 竞争与终态移动拒绝；
3. `AtomicFences`：覆盖 source identity mismatch、source/weapon Actor 混用拒绝、开放窗口 Interrupt 与终态控制拒绝。

全量 `Shanmen.0_0_10` 从 P6.3 的 140 个增加到 143 个，最终 `143/143` Success。

## 4. 兼容性

- 未修改 CombatCore、CombatRuntime、WorldGameplay、Items 或既有 `demo_map` 生产文件；
- 未修改 Profile schema、存档格式、物品定义、资源事务或既有输入映射；
- 未新增 Actor class、spawn 路径、PlayerController 分支或第二套 inventory/vitality authority；
- 未把 legacy flying-sword 内容当作正式 0.0.10 内容；
- 新 Controller 不调用 `ApplyDamage` / `TakeDamage`，不直接写库存、资源或 vitality；
- movement 产生世界位置变化，命中结算仍完全复用 P6.3 的 Session-copy / Coordinator commit 规则。

## 5. 修改范围

- `Source/demo_map/demo_mapShanmenControlledWeaponProductController.h/.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponProductControllerTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- 本 Report 与同名 Log。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponController` | 3 | 0 | 0 | `03A5C5440A2D04DB296E17AD5BFC526E180E2776DC851BBEE2001694E81AA7B8` |
| `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `CABC43241BE70DCFE97B34F5D4248EAF3E826F61CCE137540581546F70E42BFF` |
| `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `4D94F6EC3C987B16ECAA9C1EE9436AB4B5F9F53376BFF2942440BAE62020F5CB` |
| `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `184CA25917892C77ACE9C1003E974A025A1BEE823173D3C598409EB28157EDDE` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `13D3261CE5B5395BBE32309E7E252C81C3BF87079043ACC235F5FB985FBBCBA5` |
| `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `859FBCCD78227EE6748BACEEDBE0B24035810EB5AD5DB9E08E4CE70F90F64023` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `A7CDEB30526078E2EC6AFD8B1E08B32A5FC24CE54B29F8EED9ACD55CA188CF8D` |
| `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `D63982A05B16AD25EE12C2CAE449653D12DD91F3847DB761688167FC3D79AE86` |
| `Shanmen.0_0_10` | 143 | 0 | 0 | `29CDC11D0CEBFE2F050447CD91FC60C3170EB443B7CD688D42ECC8DE89759CA3` |

- 九条最终日志均有唯一 RunTests command、queue-empty、fail 0，Fatal / unhandled / handled ensure 0；
- changed-file gate：`PASS Changed=4 Rules=1 Required=8 Logs=9`；
- regression coverage self-test：`8/8 PASS`；
- regression JSON parse：PASS；
- `git diff --cached --check`：native exit `0`。

## 7. 首次失败与修正

无测试或编译失败。

首次新组运行即为 `3/3` Success、native exit `0`、SHA-256 `F13AF32BD27C5C7B94E33908826FD326CA33550F9250371EE195BC7087E5E364`。随后静态复审发现 Controller 的 Launch 前置状态判断会遮蔽 P6.2 已支持的 exact replay；删除该重复判断并增加产品级重放断言。该修正不改变首次运行结论，但修正后的全部最终日志重新生成。

## 8. 编译

Editor 命令：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `5/5` actions；
- `Result: Succeeded`；
- native exit `0`；
- `8.87s`（重放修正后的最终增量构建）。

Game 命令：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `4/4` actions；
- `Result: Succeeded`；
- native exit `0`；
- `14.93s`（重放修正后的最终增量构建）；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头自动化、Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-4-controlled-weapon-product-controller>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-4-controlled-weapon-product-controller/Docs/Report/Dev.D.UE.0.0.10.P6.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-4-controlled-weapon-product-controller/Docs/Log/Dev.D.UE.0.0.10.P6.4.r0_log.md>
