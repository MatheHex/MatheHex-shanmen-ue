# Dev.D.UE.0.0.10.P18.1.r0 Report

## 1. 结论

P18.1 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P18.0 的纯运行时剑气契约接到了最小 UE 世界交付层：剑气可被原子化地暂存并发布为独立物理载体，命中通过既有 WorldGameplay 身份、P18.0 Resolver 与 CombatRunCoordinator 唯一生命权威结算。相同目标回调不会重复扣血，超距/阻挡与动作终止均有显式收敛路径。

本轮没有接输入、实际剑动作 host、表现资源或自动 Spawn。成功命中后继续飞行，穿透/命中即消散策略仍由后续产品 host 决定，未在基础桥中提前冻结。

最终结果：

```text
Sword Qi world delivery exact:          3 Success / 0 Fail
Shanmen.0_0_10 full:                  758 Success / 0 Fail
Mapped legacy regression:             116 Success / 0 Fail
Regression coverage:                   PASS (Changed=9 / Rules=2 / Required=18 / Logs=6)
Regression gate self-test:              PASS 275/275
Game + Editor Development:              PASS / native status 0
```

## 2. 产品定位与复用边界

P18.1 采用现有“纯执行 -> 世界命中 -> 唯一生命权威”模式，但没有把剑气伪装成旧技能弹体或暗器：

- `Ademo_mapShanmenSwordQiProjectile` 是剑气专属载体，不复用 `Ademo_mapSkillProjectile`；
- `Fdemo_mapShanmenSwordQiWorldAdapter` 复用 `FShanmenWorldHitAdapter`、P18.0 `FShanmenSwordQiExecution` 与现有 CombatRunCoordinator；
- Coordinator 只增加 Sword Qi receipt 的类型安全入口，最终仍委托既有通用玩家 Impact 交付函数；
- 未创建第二套生命、伤害、幂等、物品或技能权威；
- 载体不拥有输入、目标判定、伤害公式、生命写入或表现。

## 3. 原子化发射与物理载体

世界桥以 copy-on-write 方式构建 `Fdemo_mapShanmenSwordQiLaunchPlan`。暂存阶段在 execution 副本上完成 launch 与 Projectile emission，同时让 Actor 保持无碰撞、无运动；任何身份或注册失败都不会留下活跃弹体，也不会修改 live execution。

发布前再次校验 Action、Run、Source Entity、Launch ID、Detector、ordinal、execution 状态和载体状态。通过后才同时发布 execution candidate 与载体飞行：

- 16 cm sphere，`QueryOnly`；
- authored speed 900，零重力；
- 不反弹、不追踪、不启用 Actor tick；
- 忽略精确来源 Actor；
- Stage 时仍为 `NoCollision` 且 movement inactive。

来源 Actor 必须能在当前 Run 的 EntityRegistry 中解析为 Coordinator 的玩家实体。仅 GUID 相同但对象未注册不足以发射。

## 4. 世界接触到唯一生命权威

命中处理顺序固定为：

```text
FHitResult
  -> FShanmenWorldHitAdapter::TryFromProjectile
  -> registered TargetEntityId
  -> Idemo_mapCombatVitalityHost snapshot
  -> P18.0 FShanmenSwordQiExecution resolver
  -> CombatRunCoordinator canonical vitality delivery
```

本轮 fixture 冻结 `BaseDamage=0.5`、`AttackPower=20`、系数 `0.01`，一次合法接触精确提交 0.7 vitality，authority revision 只增加 1。随后重放同一目标/同一 sample 时，P18.0 ledger 以同一 candidate identity 拒绝第二次结算，目标生命和 revision 均不再变化。

只有 Coordinator 成功提交生命后，live execution 才接收含新 ledger 状态的 candidate；世界身份、vitality snapshot、resolver 或最终提交任一步失败都保持原 execution 不变。

## 5. 生命周期与刻意保留的产品决策

载体状态为 `Empty -> Staged -> InFlight -> Dissipated`。`FinishFlight` 同步关闭 emission、dissipate execution 并停用碰撞/运动；`EndForActionTermination` 在 Action 已中断后执行同样的双端收敛。重复终止失败关闭。

载体提供 native contact 与 range-expired seam，但本轮没有建立实际产品 host 去绑定 delegate、创建 Actor 或选择命中终止策略。因此：

- 成功敌方接触不会自动消散，允许未来穿透策略；
- 世界阻挡或距离到期必须由后续 host 调用 `FinishFlight`；
- Action 中断必须由后续 host 调用 `EndForActionTermination`；
- 最终碰撞半径、视觉、音频、速度曲线与伤害平衡仍未冻结。

## 6. Automation 与回归证据

新增三条产品测试：

- `LaunchGateAndFlight`：未注册来源拒绝、暂存惰性、原子发布与物理参数；
- `ContactToVitalityAndReplay`：世界命中、精确 0.7 生命提交、revision 与幂等重放；
- `FailClosedRangeAndTermination`：未注册目标拒绝、显式超距终止、Action 中断与重复清理。

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi world delivery exact | 3 | 0 | `3DF5DE8F9E3B2423F993A3D7DF5CD4ADE52F2BCE27701D581F1158C4FDDE2324` |
| `Shanmen.0_0_10` full | 758 | 0 | `5FD51B421EEF61BC7BDF9A0616F07A9F3A3CC731B10C50ECCC75460BCC65C6C0` |
| `demo_map.V3.Attributes` | 4 | 0 | `380E07FB6EBB3755A32494A174F0F4D563F0851B8956D76487AD3B7198198D21` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `C3ED47BB27CE4B5423EAB71BEE835C43DFE1C61F029A82BCBA60C606E10059DA` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `FDC854A03B57417DEF216197B50D1251B25B93B7B7E778E8B60FE9ACC7880A7D` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `5A5D839B5B19556E27003CF2799FC42BEDA5AD9EF150DEC52FA92B42A08C69F7` |

全部 Automation 日志均有原生 `TEST COMPLETE. EXIT CODE: 0`，Fail、Fatal、Unhandled 与 Ensure 计数为 0。完整 0.0.10 首末 Success 时间为 `2026.09.02 20:28:40.727 -> 20:56:58.304 UTC`，约 28m17.577s。P18.0 的 755 条加本轮 3 条，计数增量严格一致。

## 7. 首次失败与修正

首次 Editor build 真实失败，原生退出码为 6，UBT 结果为 `OtherCompilationError`。失败只位于新增测试翻译单元：测试通过前置声明调用 `USphereComponent` API，产生 C2027，并连带产生 C2661；新增生产载体、Adapter 与 Coordinator 路径没有编译错误。

修正仅为在测试文件显式包含 `Components/SphereComponent.h`。随后 Editor retry 以 4/4 actions、native 0、7.63s 成功；再运行全部 Automation 与最终双构建。首次失败日志保留，SHA-256 为 `DDEE110606B8D6AB7259BB8CFD6523966D5393AC3AE0CC579282667923876517`；修正构建日志 SHA 为 `069FF96A2435FFA6BF5EDEC4CB70112892EEF6862260483B2B54D997B3401642`。

## 8. 门禁与最终构建

本轮把剑气世界交付路径加入改动文件回归映射，并为“完整证据可通过”和“只有聚焦测试必须失败”各增加一条自测：

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=18 Logs=6
SELF_TEST: PASS 275/275
BOUNDARY_SCAN: PASS Files=5 Matches=0
GIT_DIFF_CHECK: PASS
```

coverage log SHA 为 `B50DF28A4A8C4DD0B1C480120A4168271DF69D661288EC02559C0ED3343DA892`，self-test log SHA 为 `4F3870AF407EAE69CF9F4F93124E543C5B55B37587B4933787FAE32CD9296733`，更新后的 regression map SHA 为 `1E4E12A0418630F1FBDCA35D25A4CD60C7C34447D1116643BAFDAA1EC9DB94D3`。

边界扫描对直接 `ApplyDamage`/`TakeDamage`、`UGameplayStatics::ApplyDamage`、旧 `demo_mapSkillProjectile`、随机流、timer 与自建 `SpawnActor` 调用均为 0。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 102 / 303.26s | `D87E042BEFBB03CE95C7D420531FBA2739DFC77B582A8011ACEBDFD18FDFA984` |
| Editor Development | Succeeded / up to date | 0 / 1.00s | `FEEEFF8C93E98AE23732F76E70BD714F4024159ABEB95901D0B3B7868706057C` |

最终产物：

- `demo_map.exe`：356,239,872 bytes，SHA-256 `BE2F902E8206C7179850A4CA6E4E8CBD5887568AD1DA95377423ECD3E5121097`；
- `UnrealEditor-demo_map.dll`：14,891,520 bytes，SHA-256 `A7D70F0F73A6CE5E0150D814CFEF966FE56F19706CA14837196397CD84BF141A`。

## 9. 修改范围与 P/F 边界

计划提交 5 个新增源码文件、2 个 Coordinator 类型安全入口修改、2 个回归门禁文件与本 Report/Development Log，共 11 个文件。没有修改 Content、地图、资源、配置、Windows、UE Engine 或用户设置。

本轮只执行 unattended、NullRHI Automation、静态门禁和 Development builds。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package，也没有把这些未执行项目描述为成功。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料未修改、未暂存、未提交。raw build/test logs 只保存在本地 `Saved/Codex/P18.1`。

## 10. 下一阶段

P18.2 建议建立最小 Sword Qi product host：在正式剑动作 Active commit 点创建并绑定载体，把 contact/range/action-terminal delegate 路由到 P18.1 Adapter，并由一个明确策略决定“首个阻挡消散”或“合法目标穿透”。该 host 仍应复用当前生命与 Impact 权威，不接视觉资产前不宣称实际游戏手感通过。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-1-sword-qi-world-delivery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-1-sword-qi-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P18.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-1-sword-qi-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P18.1.r0_log.md>
