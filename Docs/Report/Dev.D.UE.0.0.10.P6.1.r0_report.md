# Dev.D.UE.0.0.10.P6.1.r0 Report

## 1. 结论

P6.1 在 P 阶段边界内完成，结论为 **PASS**。

本轮为 P6.0 受控飞剑纯契约增加单向产品权威门：只有当前 active Run 的 exact weapon-slot `ItemInstanceId`，在 ShanmenItems 快照中仍是 `Deployed`、带有已提交的 `DeploymentLock`，且其权威定义带精确 `Shanmen.Item.Weapon.FlyingSword` 标签时，才可生成 P6.0 的不可变 action 与 execution。

本轮没有把 `TrainingBlade` 或任意旧物品临时解释为飞剑，没有新增第二套库存真值，也没有修改库存、耐久、生命、Actor、输入或表现。

## 2. 权威边界

新增 `Fdemo_mapShanmenControlledWeaponAdapter`，提供两层入口：

- `PrepareActiveRun`：在 Game Thread 上读取 Ready item authority、重建 durable Run correlation，并核对 transient Runtime 仍处于同一 active Run；
- `PrepareFromEvidence`：只消费已捕获的 authority snapshot、immutable Run correlation 与产品 activation 输入，执行可无头测试的纯证据门。

通过条件同时要求：

1. 请求物品等于 correlation 的 exact weapon-slot identity，并存在于 prepared item 集合；
2. authority snapshot 的 revision 不早于 lifecycle receipt；
3. 物品为当前 owner/scope 下的 `Deployed` singleton；
4. 物品的 `DeploymentReservationId` 指向同 item/owner/scope 的 committed `DeploymentLock`；
5. 部署后 item revision 已前进；
6. 权威 Definition 支持 deployment，并带 exact FlyingSword semantic tag。

任一证据不成立均返回明确状态并失败关闭，不生成有效 execution/evidence。

## 3. 战斗契约接入

适配器不复制 P6.0 的控制状态机。证据通过后，它：

- 冻结产品提供的受控武器 Definition 与 ControlPower；
- 以 active Run、source entity、canonical action 和 activation sequence 派生确定性 ActivationId；
- 将 exact `SourceItemInstanceId` 写入 `FShanmenCombatActionSnapshot`；
- 将 authority Definition 的 item tags 合入冻结的 action source tags；
- 通过 `FShanmenControlledWeaponExecution::TryCreate` 进入既有 Launch / Redirect / Recall 与 ControlledObject Impact 管线。

整个过程只有读取与值捕获；没有调用 Reserve、Commit、Cancel、Start/End Run、存档或产品资源写入。

## 4. 失败关闭与确定性

新增测试覆盖：

- 错误物品不能借用 weapon slot；
- Stored 物品不能在局内受控；
- generic deployable 缺 FlyingSword tag 时不能冒充飞剑；
- 未提交 DeploymentLock、陈旧 authority snapshot、未前进 item revision 均拒绝；
- 相同 active Run、证据与 sequence 重放产生相同 ActivationId；
- 后续 sequence 产生不同 ActivationId，但保留相同物理物品身份；
- 未绑定/未 Ready 的产品 authority 不能绕过 facade。

正例还验证 snapshot 与 correlation 在准备前后完全相等，证明适配器无副作用。

## 5. 自动化结果

最终证据：

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p61_controlled_weapon_adapter_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `365065E0B8C76F0D61441457A6DD137016970143A3DA3E20EA371A730187F1F9` |
| `p61_items_regression_final.log` | `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `84A58A8AF0EE4113732DC88C0F7D0DFD13C078B4F21DF45E463498E589EF0A58` |
| `p61_combat_runtime_regression_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `AA4DA38EB64E381E9FD83A9ACFB0E0412EA24D260405F67119D09D055AE09828` |
| `p61_full_regression_final.log` | `Shanmen.0_0_10` | 133 | 0 | 0 | `CE79AD03964CBF06A2585025AE8F3FDA2EA44C443FDBA21593E83335B9521982` |

最终唯一计数为 `133 Success / 0 Fail`；前三组均包含于最终父组，不重复计数。四次最终队列均正常清空，原生退出码均为 `0`，没有新增 handled ensure、assert 或 Fatal。

父组日志保留 UE 5.8 在测试发现前既有的 13 行 `LogAutomationTest: Error: Condition failed` 启动诊断；目标队列随后为 133/133 Success。

## 6. 首次失败与修复

本轮保留两项首次失败：

1. 首次 Editor build 因测试代码尝试对 `FShanmenContentStamp` 使用不存在的 `operator==`，UBT `Result: Failed (OtherCompilationError)`，原生退出码 `1`，约 `34.07s`。修复为分别比较 `Version` 与 `Digest`；生产适配器本身已在该次构建中编译。
2. 首次专项目标日志 `p61_controlled_weapon_adapter_focused.log` 为 `3 Success / 1 Fail`，SHA-256 `80CDD3270394533DFD90A168ABEA6CC5A3F34E62EADC32110DBF46DB5E441297`。失败来自测试夹具以错误 Outer 直接构造 `UGameInstanceSubsystem`，触发 UObject ensure；业务证据门三例均已通过。修复为创建真实 transient `UGameInstance`，由其解析 Subsystem 并执行规范 Shutdown。

两个修复后均以新日志重新验证；失败日志未覆盖，也未把进程退出码 `0` 误写成测试通过。

## 7. Changed-file 回归门禁

本轮为新的 `demo_mapShanmenControlledWeaponAdapter` 路径增加精确映射，要求 Product adapter、Items 与 CombatRuntime 三组。门禁结果：

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=1 Required=3 Logs=4
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=p61_combat_runtime_regression_final.log,p61_full_regression_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Items Evidence=p61_items_regression_final.log,p61_full_regression_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponAdapter Evidence=p61_controlled_weapon_adapter_final.log,p61_full_regression_final.log
```

映射工具自身 self-test 为 `8/8 PASS`。

## 8. 构建与静态检查

最终 Editor Development：

- 命令：`Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`；
- `4/4` actions，`Result: Succeeded`；
- 原生退出码 `0`，总执行时间 `6.99s`。

Game Development：

- 命令：`Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`；
- `4/4` actions，`Result: Succeeded`；
- 原生退出码 `0`，总执行时间 `27.22s`；
- 生成 `Binaries/Win64/demo_map.exe`，未启动。

静态结果：

- `git diff --check`：原生退出码 `0`；
- `ShanmenRegressionMap.json` 解析成功；
- 生产适配器中 `UWorld` / `AActor` / Pawn / Input / Spawn / ApplyDamage / RNG 命中 `0`；
- 生产适配器中资源 mutation call 命中 `0`；
- `TrainingBlade` / fallback 映射命中 `0`。

## 9. 修改范围与兼容性

新增：

- `Source/demo_map/demo_mapShanmenControlledWeaponAdapter.h`；
- `Source/demo_map/demo_mapShanmenControlledWeaponAdapter.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponAdapterTests.cpp`；
- 本 Report 与 Development Log。

修改：

- `Scripts/ShanmenRegressionMap.json`：为上述 adapter 路径增加 changed-file 测试映射。

兼容性：不改变 Profile schema、既有 item Definition、authority document、P6.0 纯契约、通用 Projectile 或保存格式；历史未跟踪 Prompt、Report、PDF 与用户文档不进入提交。

## 10. P/F 边界与后续

本轮只执行源码、静态审查、无头 `-NullRHI` Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

后续产品接线可以把真实 Actor 运动和输入转换为 P6.0 命令，把碰撞转换为 ControlledObject candidate；但具体可用飞剑内容、旧物品迁移、耐久/修理成本必须在内容策略冻结后通过正式物品事务接入，不能靠名称 fallback 或直接改库存。

## 11. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-1-controlled-weapon-authority-adapter/Docs/Report/Dev.D.UE.0.0.10.P6.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-1-controlled-weapon-authority-adapter/Docs/Log/Dev.D.UE.0.0.10.P6.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-1-controlled-weapon-authority-adapter>
