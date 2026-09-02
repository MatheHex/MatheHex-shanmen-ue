# Dev.D.UE.0.0.10.P17.1.r0 Report

## 1. 结论

P17.1 在 P 阶段边界内完成，结论为 **PASS**。

本轮关闭 P17.0 留下的产品纵切缺口：护心镜现已通过真实 `Profile -> Cutover -> ShanmenItems -> StartPreparedRun -> GameInstance/World -> CombatRunCoordinator -> M01 enemy melee` 无头路径得到验证。第一次致死攻击把玩家从 3 HP 降至 1 HP，并只消费唯一 charge；相同 Impact 的精确重放不重复伤害或扣费；下一次新攻击在 charge 耗尽后不再获得护心镜层，玩家从 1 HP 正常降至 0。

战斗 World 拆除后重启物品权威，完整 authority snapshot、活动 Run 关联和已耗尽的护心镜状态保持一致。本轮没有发现生产集成缺陷，因此没有修改生产代码、内容、资源、地图或配置；只新增一条世界级产品 Automation 及其专用 fixture。

最终结果：

```text
HeartMirror exact product slice:       1 Success / 0 Fail
CombatRunCoordinator full:            18 Success / 0 Fail
Shanmen.0_0_10 full:                 750 Success / 0 Fail
Mapped legacy regressions:           116 Success / 0 Fail
Regression coverage:                  PASS (Changed=1 / Rules=1 / Required=16 / Logs=5)
Regression gate self-test:             PASS 273/273
Game + Editor Development:             PASS / native status 0
```

## 2. 产品纵切

测试使用 fresh Profile，把正式定义 `Prototype.Item.Accessory.HeartProtectingMirror` 放入准备栏，经既有 cutover 与 `StartPreparedRun` 生成真实 active Run 和 ShanmenItems authority。随后创建 transient `GamePreview` World，绑定真实 `UGameInstance`，生成玩家 Pawn、生命组件与正式 `Ademo_mapEnemyCharacter`，再由 `Fdemo_mapCombatRunCoordinator` 完成 Run 绑定和 M01 敌人注册。

执行路径没有直接调用 CombatCore resolver，也没有手工扣除 charge：

```text
fresh Profile with Heart Mirror
  -> authoritative cutover
  -> StartPreparedRun
  -> UGameInstance + transient UWorld
  -> CombatRunCoordinator bind
  -> real M01 enemy registration
  -> ExecuteM01EnemyBasicMeleeStrike
  -> resource defense preparation
  -> pure Impact resolution
  -> vitality + item durable finalize
```

因此本轮证明的是已部署产品组合，而不是底层函数的重复单元测试。

## 3. 状态转移

| 步骤 | Impact / 生命 | 护心镜资源 |
|---|---|---|
| 初始 | 玩家 3 HP；mirror charge 1 | `Deployed`，charge 1 |
| 第一次 M01 近战 5 damage | `PreventLethal` 防 3，最终伤害 2，玩家 1 HP | 唯一 reservation `Committed`，charge 0 |
| exact delivery replay | `AlreadyCommitted`；生命、revision、广播数不变 | charge 仍为 0，不二次消费 |
| 第二个新 M01 近战 5 damage | 请求不含 mirror layer，实际提交当前 1 HP，玩家 0 HP | charge 仍为 0 |
| World teardown + authority restart | 不重放战斗、不改生命 | 完整 snapshot 与重启前严格相等 |

第一击还验证了镜层使用 `FShanmenDefenseOrder::LethalInterception`、`PreventLethal`、精确 `SourceInstanceId` 与 `bRequiresCommitOnTrigger`。第二击具有不同 ActivationId / ImpactId，并明确断言 defense request 中不存在已耗尽镜的来源实例。

## 4. 幂等与持久边界

首次攻击只形成一次生命提交、一次正伤害广播和一个已提交的 charge reservation。把首次已解析 Impact 再次送入 Coordinator 时，生命权威返回 `AlreadyCommitted`，当前生命仍为 1、authority revision 仍为 1、正伤害广播仍为 1，物品 charge 仍为 0。

第二击是新的 canonical action，不会被第一次的 Impact ledger 吞掉。它在资源准备阶段观察到 charge 已耗尽，因而不生成伪镜层；生命组件只提交目标尚存的 1 点生命，revision 增至 2，击败结果成立。

测试最后先销毁 Coordinator 所在 World，再关闭并从持久存储重建 GameInstance 物品权威。重启后的 `FShanmenItemAuthoritySnapshot` 与第二击后的 snapshot 全量相等，且护心镜仍为 `Deployed / Charges=0`，证明消费不是进程内临时状态。

## 5. Automation 覆盖

新增：

- `Shanmen.0_0_10.Product.CombatRunCoordinator.HeartMirrorProductLifecycle`

该测试同时覆盖：真实准备栏部署、活动 Run correlation、GameInstance/World 绑定、玩家生命 authority、M01 enemy product attack、致死拦截、资源提交、exact replay、耗尽后的新 Impact、战斗 World teardown 与物品 authority restart。

完整 `CombatRunCoordinator` 组由 17 增至 18；完整 `Shanmen.0_0_10` 由 P17.0 的 749 增至 750。Full suite 首末 Success 时间为 `2026.09.02 17:54:00.203 -> 18:21:39.278 UTC`，约 27m39.08s。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| HeartMirror exact retry | 1 | 0 | `804B9B3558EF643973939E440C5020A89DAD567D7908CAF827A99ED4A788D0AE` |
| CombatRunCoordinator full | 18 | 0 | `C8CFDBF28364876CBBE7E6FC40A409B229D5F277DC2535C185406243143E8D4F` |
| `Shanmen.0_0_10` full | 750 | 0 | `5010BFA277587D49C521D9810BADC7E78E5C0CA16EC19FA04EB8F9244F0FAC0E` |
| `demo_map.V3.Attributes` | 4 | 0 | `C7E004BE61BA0281A7F678A139C7EB8408E24EE6465037A3508CE965512AA9F0` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `C1AE12606E616633CCF006FD1B576B8089368F6A02D3946F0C2BF6E0C96FDB53` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `FE7A58D4A48A84D0950E510DF88448C2990F4798F862B30DC1C0525C0196E28C` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `C7EAE0F8DFCAB9933D3159ECC09188CEEF9887720577C5B50A601F79266B7532` |

全部最终日志具有正常 Automation 终止结果，Fail、Fatal、Unhandled 与 Ensure 计数均为 0。四个 mapped legacy 组共 116/0；focused 与 Coordinator 组是 full suite 的子集。

## 7. 首次失败与修正

首次 exact 测试真实得到 0 Success / 1 Fail，失败日志 `P17.1_HeartMirrorProductLifecycle.log` 已保留，SHA-256 为 `1998F2A896ED6277DC7C0BD5AD1E4406A8C9945CC35E899CB72754822C383223`。

根因是新测试 WorldContext 虽设置了 `OwningGameInstance`，却没有调用 `World->SetGameInstance(GameInstance)`。因此产品代码通过 `Player->GetGameInstance()` 得到空值，Coordinator 按既有 fail-closed 规则没有绑定资源 authority，攻击正确地走普通伤害并把玩家降至 0。该结果证明生产硬依赖生效，不是护心镜产品回归。

修正仅补齐测试 World 的标准 GameInstance 绑定；生产代码零修改。重编译后 exact retry 为 1/0，随后所有扩展测试均通过。

## 8. 门禁与构建

```text
REGRESSION_COVERAGE: PASS Changed=1 Rules=1 Required=16 Logs=5
SELF_TEST: PASS 273/273
GIT_DIFF_CHECK: PASS
```

regression map 未修改，SHA-256 为 `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 3 / 24.67s | `EBD44F494DCB5B590918D38FBDD5A5D42026C00CF06866B94BF3F7121BEE78D3` |
| Editor Development | Succeeded / up to date | 0 / 0.98s | `5FB50F9F76F16F7BFF022B6DE3DF7DEB28695DCBE14956B19F20D46278C66664` |

最终产物：

- `demo_map.exe`：356,148,224 bytes，SHA-256 `30DE72E07832D7754E2FBDB1E6274CE8CD30DE61D9C7C3D0DA0863FAA099315D`；
- `UnrealEditor-demo_map.dll`：14,837,760 bytes，SHA-256 `3D1571EF47A09F0DCFEE45A9FE14BBE1BBD85249F9241030C33045F6D4577665`。

## 9. 修改范围与 P/F 边界

源码只修改 `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`，并新增本 Report 与 Development Log，共计划提交 3 个文件。没有修改生产 C++、Content、地图、资源、配置、Windows、UE Engine 或用户设置。

本轮只执行 unattended、NullRHI 的 P 阶段 Automation、静态门禁与 Development builds。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package，也没有把这些未执行项目描述为成功。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料未修改、未暂存、未提交。raw build/test logs 只保存在本地 `Saved/Codex/P17.1`。

## 10. 下一阶段

P17.2 应验证已耗尽护心镜的终局结算边界：分别经过撤离与死亡 settlement，确认现有通用规则如何持久化或销毁该真实实例，并验证下一局不会凭空恢复 charge。该阶段只收口既有 settlement 语义；在规划明确修复、补充或一次性销毁规则前，不新增 recharge 机制或数值设定。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p17-1-heart-mirror-product-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-1-heart-mirror-product-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P17.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-1-heart-mirror-product-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P17.1.r0_log.md>
