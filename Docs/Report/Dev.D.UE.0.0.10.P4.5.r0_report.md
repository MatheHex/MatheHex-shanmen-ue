# Dev.D.UE.0.0.10.P4.5.r0 开发报告

## 结论

`PASS`。P4.5 已把真实产品左键 `PrimaryAttack` 的既有球形轨迹查询接入 0.0.10 canonical BasicSword 链。M01 地图现在是原子路由：输入采样一次轨迹后，以当前 Run、稳定玩家 EntityId、已装备武器实例 GUID、冻结攻击力和 Run 内单调 activation sequence 构造 action；世界命中只能经共享 Registry 转为 candidate，再经 BasicSword 纯结算形成 receipt，并由 P4.4 coordinator 提交到目标 vitality ledger。

M01 路径不会再落入同一次攻击的 `UGameplayStatics::ApplyDamage` 旧写入。产品链未就绪、武器未装备、攻击快照无效或交付不一致时均失败关闭；非 M01 旧地图继续保留原攻击兼容路径。本轮只允许 P4.4 已迁移的 7 个近战宿主接受 canonical BasicSword，远程、重甲与 Boss 仍等待后续 vitality 迁移。

## 功能性

### 1. 真实输入与轨迹接线

- `PrimaryAttack` 仍由现有统一输入注册表绑定到 `StartBasicAttack`；
- 保留产品既有朝向、冷却、起点、距离、半径和 `SweepMultiByObjectType` 几何；
- 查询完成后，M01 由 `ShouldUseM01BasicSwordProductPath` 抢占本次攻击；
- M01 分支无论成功、miss 或失败都立即返回，不会继续执行下方 legacy `ApplyDamage`；
- 非 M01 内容不受该切换影响。

### 2. 冻结 action source

每次合法 M01 攻击从 Code A 当前权威状态捕获：

- `RunId`：活动 ItemAuthority Run；
- `OwnerId`／`SourceEntityId`：P4.4 Registry 中的稳定玩家 EntityId；
- `SourceItemInstanceId`：当前 `WeaponSlot` 中的精确实例 GUID；
- `AttackPower`：玩家派生属性的有限、非负快照；
- `ActionDefinitionId`：`Combat.Action.Sword.Basic01`；
- `ActivationId`：`RunId + SourceEntityId + ActionDefinitionId + Run内单调序号` 的确定性派生值。

Run 精确结束或 coordinator reset 时序号回到 1；新 Run 即使再次从 1 开始，也因 RunId 不同而得到不同 ActivationId。未使用时间、随机 GUID、Actor 地址、对象名或碰撞回调顺序。

### 3. canonical BasicSword 执行

`ExecutePlayerBasicSwordSweep` 完整闭合一轮动作：

1. 捕获只读 action、BasicSword 内容定义和 offense；
2. `Idle -> Startup -> Active` 进入唯一 commit point；
3. 开启一次 `WeaponTrajectory` emission；
4. 每个 UE `FHitResult` 通过 Run Registry 解析 authored target EntityId；
5. 从目标唯一 vitality ledger 同时捕获生命与 revision；
6. 使用 `Target.Living` 与 `Damage.Physical.Slash` 生成守恒 receipt；
7. 只调用 `DeliverBasicSwordImpactToM01Enemy` 提交 final damage；
8. 关闭 emission 并完成 `Active -> Recovery -> Idle`。

同一 emission 中同一目标的重复几何接触会解析为相同 target，但只产生一次 receipt／commit。不同目标仍可各自获得独立 ImpactId。合法 miss 也会正常完成动作并消耗一个 activation sequence，不伪装成执行失败。

### 4. 可审计结果

新增 `Fdemo_mapBasicSwordProductExecutionResult`，记录：

- ActivationId；
- 世界接触数；
- Registry 已解析候选数；
- 已交付 Impact 数；
- 首次 commit 数；
- replay 数；
- 发生在哪个前置／生命周期／交付门失败。

GameMode 为每次 M01 产品 sweep 输出一条结构化运行日志，后续 F 阶段可据此核对真实输入、轨迹和生命变化。

## 完整性与兼容性

- M01 左键不再双写 canonical vitality 与 legacy `ApplyDamage`；
- 未注册碰撞、非近战宿主和缺失 vitality binding 均被忽略且不发生 legacy 伤害；
- 缺失已装备武器时不会制造虚假 `SourceItemInstanceId`；
- 旧地图、旧训练目标与既有自动化仍走原兼容分支；
- `CaptureAttackPower` 显式区分动作冻结输入与旧 `CaptureOutgoingDamage` 结果；
- P4.4 的 Run mismatch、target mismatch、revision 与 receipt 幂等门保持不变；
- 本轮未声称远程、重甲、Boss 已接 canonical vitality，也未声称已进行真实鼠标输入验收。

## 自动化证据

`Shanmen.0_0_10.Product.CombatRunCoordinator` 从 4 条增至 6 条：

- `ProductBasicSwordSweep`：真实 `FHitResult` 数组、确定性 first activation、重复接触只提交一次、合法 miss、Run 结束序号复位、新 Run 身份隔离；
- `ProductBasicSwordFailClosed`：未激活 coordinator、缺失武器、NaN offense、未注册 Actor 均在产品边界失败或无害完成，并验证相同首轮输入可确定性重放同一 ActivationId；
- 既有 `RunLifecycle`、`PlayerDelivery`、`M01AuthoredIdentity`、`M01BasicSwordDelivery` 全部保持通过。

最终结果：

- 产品定向：`6/6 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`102/102 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.5.r0_combat_run_automation_final.log`：`656F2775855FBBEE5AA319C8E5A1D1370992D83FF451DFD278825D51B756BD9D`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.5.r0_full_automation_final.log`：`FACE8501C379A34E2376AB247805A57F5014A7B0FCAA7C8B3C01AF100733730A`。

两份日志各保留 UE 5.8 在测试发现前输出的既有 13 条 `Condition failed` 自检诊断；目标测试全部成功，无 fatal 或 handled ensure。

## 构建与静态检查

- Editor 首次完整构建：测试使用了 UE `TNumericLimits<float>` 不提供的 `QuietNaN()`，`OtherCompilationError`，原生退出码 `1`；该失败只属于新增测试代码；
- 改用 `std::numeric_limits<float>::quiet_NaN()` 后 Editor 增量 `4/4` 成功，退出码 `0`；最终 Editor 增量 `4/4` 成功，退出码 `0`；
- Game 首次完整 `24/24` 成功，退出码 `0`；最终增量 `3/3` 成功，退出码 `0`；
- `git diff --check`：原生退出码 `0`；
- coordinator 对 `FGuid::NewGuid`、`GetUniqueID`、`PointerHash`、`reinterpret_cast`、`FRand`、`RandRange` 的扫描为 0 匹配；
- coordinator 对 `ApplyDamage`、`TakeDamage`、`ApplyIncomingDamage` 的扫描为 0 匹配。

本轮没有 Windows commit memory／页面文件错误，也没有产品源码编译失败或测试失败。

## 修改范围

- `Source/demo_map/demo_mapCombatRunCoordinator.h`
- `Source/demo_map/demo_mapCombatRunCoordinator.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
- `Source/demo_map/demo_mapGameMode.h`
- `Source/demo_map/demo_mapGameMode.cpp`
- `Source/demo_map/demo_mapPlayerCombat.h`
- `Source/demo_map/demo_mapPlayerCombat.cpp`
- `Source/demo_map/demo_mapPlayerController.cpp`
- 本 Report 与同名 Development Log

未修改、删除或提交工作区中的无关长期未跟踪文件。

## P/F 边界

本 Report 只包含 P 阶段源码开发、静态审查、NullRHI headless Automation、Editor Development 与 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实鼠标输入、截图、Smoke、Cook 或 Package。真实左键代码入口已接线，但交互验收留在 F 阶段。

## 下一步

P4.6 建议抽出统一的 M01 vitality host 产品适配面，并按宿主类型迁移远程、重甲与 Boss。迁移后同一 BasicSword trajectory 不再需要硬编码近战 Actor 类型，14/14 authored enemies 都能在不恢复 `ApplyDamage` 双写的前提下消费 canonical receipt；各宿主原有死亡、掉落、任务与表现链继续只由首次 commit 触发。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-5-basic-sword-product-path/Docs/Report/Dev.D.UE.0.0.10.P4.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-5-basic-sword-product-path/Docs/Log/Dev.D.UE.0.0.10.P4.5.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-5-basic-sword-product-path>
