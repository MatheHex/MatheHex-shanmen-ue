# Dev.D.UE.0.0.10.P6.2.r0 Report

## 1. 结论

P6.2 在 P 阶段边界内完成，结论为 **PASS**。

本轮新增受控飞剑产品会话层，把 P6.1 的 exact item-authority 准备结果与既有 `FShanmenActionOrchestrator`、P6.0 `FShanmenControlledWeaponExecution` 绑定为一个原子生命周期。产品代码不能再单独回收飞剑却遗留 Active action，也不能在开放接触窗口时进入 Recall。

本轮仍未新增飞剑 Actor、输入、移动、VFX 或具体 legacy 物品映射；没有修改库存、耐久、生命或存档。

## 2. 产品会话生命周期

新增 `Fdemo_mapShanmenControlledWeaponSession`，状态固定为：

```text
Empty -> Active -> Completed
                -> Interrupted
```

`TryStart` 只接受完整的 P6.1 prepared result，并在候选副本内依次执行：

1. Action `Idle -> Startup`；
2. Action `Startup -> Active`，跨越 canonical commit point；
3. 绑定同一 authority evidence 与 P6.0 execution；
4. 最终不变量通过后才提交整个 session。

Active session 才允许控制命令和接触窗口。Completed/Interrupted 均为只读终态，不可重新 Launch。

## 3. 原子控制与终止

`TryIssueControl` 只接受 Launch / Redirect。Recall 不暴露为普通命令，而必须调用 `TryRecallAndComplete`：

- 先确认没有开放 contact window；
- 在 session 副本中发出 exact-sequence Recall；
- Action `Active -> Recovery -> Idle/Completed`；
- 只有全部成功且最终不变量成立才提交。

`TryInterrupt` 同样在副本中关闭遗留 emission，再将 Action 置为 Interrupted，避免 Actor 销毁、Run 结束或外部取消留下 detector。

Launch、Redirect、begin/end contact、candidate resolve、Recall 和 Interrupt 均采用整会话 copy-validate-commit；失败不会消费 sequence、写入 ledger 或留下半步 phase。

## 4. 接触窗口与结算边界

会话层复用 P6.0 的 `ControlledObject` emission 与 Impact ledger：

- 只有 Directed 状态可打开 contact window；
- 同一窗口同目标只接受一次；
- 后续窗口稳定递增 hit ordinal，同一目标可再次命中；
- Impact 保留 exact `SourceItemInstanceId` 并满足伤害守恒；
- 会话只产生纯 Impact receipt，不直接写目标生命。

真实 UE sweep/overlap、World Entity Registry 解析及 vitality commit 继续留给下一层产品 Adapter。

## 5. P6.1 防篡改强化

P6.1 `IsPrepared()` 原先主要校验 Action 与 evidence 的 Run/Owner/Item。P6.2 增加完整跨边界一致性：

- evidence content 必须等于 Action content；
- Action 必须保留 exact FlyingSword authority tag；
- result Action 必须逐字段等于 execution Action；
- result Definition 必须逐字段等于 execution Definition；
- result Offense 必须精确等于 execution Offense。

为此 P6.0 execution 新增只读 `GetOffense()`，没有开放 mutation。对 content、Action、Definition 或 Offense 的任意有效值替换都会失败关闭，不能启动 session。

## 6. 自动化结果

最终证据：

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p62_controlled_weapon_session_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `14E94F19A2C63CAC76B142B74A1ED98FD358BEA7125E12E23ECBCE4671587E84` |
| `p62_controlled_weapon_adapter_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `A3E399F130F248C36E531F79CDAD37E88051FEAC6BA10BCA8F1B52DAADC415BB` |
| `p62_combat_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `1A6763F913FD4209BB0B2AC03986AF15E03485D2195C4CAA2AB5654305E3114B` |
| `p62_items_final.log` | `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `9F451BB16C2E1DEB74C6BC6EA902174AFE6D2109B4CF1CB0377B9990FB6C6996` |
| `p62_full_final.log` | `Shanmen.0_0_10` | 137 | 0 | 0 | `5DDE0A93E9484669A43ADB8664ACD75911BC313E6BDE3674A9CADB560F775830` |

最终唯一计数为 `137 Success / 0 Fail`；前四组均包含于父组，不重复计数。五条最终日志均 queue empty、原生退出码 `0`，没有目标 test fail、handled ensure、assert 或 Fatal。

各日志保留测试发现前 UE 5.8 既有的 13 行 `LogAutomationTest: Error: Condition failed` 启动诊断；目标队列随后全部成功。

新增 4 个 Session 测试：

1. `LifecycleAndCommands`；
2. `ContactWindows`；
3. `AtomicFailure`；
4. `TamperFence`。

## 7. Changed-file 回归门禁

新增 Session 路径映射；本轮 Runtime getter、Adapter 强化与 Session 新文件共同要求 CombatRuntime、Items、Product Adapter、Product Session 四组：

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=3 Required=4 Logs=5
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=p62_combat_runtime_final.log,p62_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Items Evidence=p62_items_final.log,p62_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponAdapter Evidence=p62_controlled_weapon_adapter_final.log,p62_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponSession Evidence=p62_controlled_weapon_session_final.log,p62_full_final.log
```

映射工具 self-test：`8/8 PASS`。

## 8. 构建与静态检查

最终 Editor Development：

- 命令：`Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`；
- `5/5` actions，`Result: Succeeded`；
- 原生退出码 `0`，总执行时间 `8.93s`。

Game Development：

- 同参数构建 `demo_map Win64 Development`；
- `9/9` actions，`Result: Succeeded`；
- 原生退出码 `0`，总执行时间 `41.90s`；
- 生成 `Binaries/Win64/demo_map.exe`，未启动。

静态结果：

- `git diff --check`：原生退出码 `0`；
- regression JSON parse：PASS；
- Session 生产代码中的 World/Actor/Input/Spawn/ApplyDamage/RNG 命中 `0`；
- Session 生产代码中的外部 item/resource mutation call 命中 `0`；
- 首次 Editor build、首次 focused test 与所有最终验证均成功，没有需掩盖的首次失败。

## 9. 修改范围与兼容性

新增：

- `Source/demo_map/demo_mapShanmenControlledWeaponSession.h`；
- `Source/demo_map/demo_mapShanmenControlledWeaponSession.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponSessionTests.cpp`；
- 本 Report 与 Development Log。

修改：

- `Source/ShanmenCombatRuntime/Public/ShanmenControlledWeaponExecution.h`：增加 offense 只读 getter；
- `Source/demo_map/demo_mapShanmenControlledWeaponAdapter.cpp`：强化 prepared-result 跨边界不变量；
- `Scripts/ShanmenRegressionMap.json`：增加 Session changed-file 映射。

不改变 item Definition、Profile schema、authority document、资源事务、通用 Projectile 或保存格式；历史未跟踪文件不进入提交。

## 10. P/F 边界与后续

本轮只执行源码、静态审查、无头 `-NullRHI` Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

下一阶段可建立 World contact/delivery Adapter：把飞剑 Actor 的 sweep/overlap 证据映射为本 Session 的 `ControlledObject` candidate，并通过现有 CombatRunCoordinator 将合法 receipt 提交到注册敌人的 vitality authority。具体飞剑内容和旧物品迁移仍不能靠名称 fallback。

## 11. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-2-controlled-weapon-product-session/Docs/Report/Dev.D.UE.0.0.10.P6.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-2-controlled-weapon-product-session/Docs/Log/Dev.D.UE.0.0.10.P6.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-2-controlled-weapon-product-session>
