# Dev.D.UE.0.0.10.P6.0.r0 Report

## 1. 结论

P6.0 在本轮 P 阶段边界内完成，结论为 **PASS**。

本轮依据人工战斗规划中已经明确的“御器以真实物品为载体、飞剑离手后仍可持续控制”方向，建立第一份受控飞剑纯契约：每次执行绑定唯一 `SourceItemInstanceId`，Launch / Redirect / Recall 使用严格递增序列和确定性回执，飞行接触以独立的 `ControlledObject` detector 进入现有 CombatCore Impact 管线。

本轮没有实现 Actor 移动、玩家输入、VFX、物品扣除、耐久消耗、生命写入或产品层装配；也没有抢先冻结修理、护心镜、正式抗性或其它仍在讨论的规则。

## 2. 物理物品边界

新增 `FShanmenControlledWeaponExecution`，创建时必须同时提供：

- 有效且冻结的 `FShanmenCombatActionSnapshot`；
- canonical action `Combat.Action.ControlledWeapon.FlyingSword01`；
- 有效的真实 `SourceItemInstanceId`；
- 冻结的受控武器内容定义与控制力快照。

缺少物品身份、把通用 Projectile 身份冒充受控飞剑、内容标签不完整或快照无效时均失败关闭。执行期间只保存和回传该 exact item identity，不复制物品权威，也不直接修改库存或耐久。

产品 Adapter 在后续阶段负责证明该实例确实已部署到当前 Run；本纯契约只消费已经验证的稳定身份。

## 3. 控制状态机与确定性回放

受控武器状态固定为：

```text
Orbiting --Launch--> Directed --Redirect--> Directed --Recall--> Recalled
```

每条命令回执包含 `CommandId`、`ActivationId`、`SourceItemInstanceId`、sequence、kind、前后状态和归一化方向。

- 只有 Active action 可以发出命令；
- sequence 必须从 0 严格递增，跳号失败；
- 同一 sequence 的完全相同请求返回原回执，不再次推进状态；
- 同一 sequence 的冲突 kind 或 direction 失败关闭；
- Recall 后本 activation 不可重新 Launch；
- emission 尚未关闭时拒绝 Recall，避免控制终止与接触窗口竞态。

`CommandId` 由 activation、物品身份、sequence、kind 和归一化方向的 IEEE bits 规范派生；独立重放可得到完全相同的命令身份。

## 4. ControlledObject 接触与 Impact

受控飞剑不复用 fire-and-forget Projectile 语义。只有 Directed 状态可开启 `EShanmenHitDetectorKind::ControlledObject` emission。

每个合法候选通过现有 `FShanmenDetectorEmissionSession`、`FShanmenCombatIdFactory`、`FShanmenDefenseResolver` 与 `FShanmenImpactLedger` 生成可审计回执：

- 同一 emission 内同一目标只接受一次；
- 不同目标得到不同 ImpactId；
- 后续 emission 的 ordinal 稳定递增，同一目标可在新的采样窗口再次命中；
- interruption/termination 可显式关闭遗留接触窗口；
- Request/Result 保留 exact physical source item identity。

测试内容使用作者输入公式 `10 + 40 * 0.25 = 20`，仅证明冻结输入和纯函数结算链路，不声明最终平衡数值。

## 5. 自动化覆盖

新增 4 条无头测试：

1. `PhysicalItemBoundary`：物品身份、canonical action 与失败关闭；
2. `CommandSequenceAndReplay`：action phase、严格序列、冲突重放、emission/recall 竞态和终态；
3. `ControlledObjectImpacts`：detector 区分、Impact 管线、ordinal、去重和终止清理；
4. `DeterministicControlReplay`：跨独立执行的 CommandId 与完整回执重现。

最终证据：

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p60_controlled_weapon_focused.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 4 | 0 | 0 | `585D9672FE41BF14A02DF984E21850D3E41437EC46CA112C3D064B2D74B2B896` |
| `p60_combat_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `71E4EE4DD16B8C0166107AD71A3B2550F956DD0C2FB2381608AD8FAF959E3C17` |
| `p60_shanmen_full_final.log` | `Shanmen.0_0_10` | 129 | 0 | 0 | `67F39653A1ED51F7A82A87ABF92E796526BCD8C93A24DB0FA8BB4698CE008DD3` |

最终唯一用例计数为 `129 Success / 0 Fail`；前两组均包含于最终父组，不重复计数。三次队列均正常清空，原生退出码均为 `0`。

最终父组日志在 Automation 启动、测试队列建立前保留 UE 既有的 13 行 `LogAutomationTest: Error: Condition failed` 启动噪声；本轮目标队列随后为 129/129 Success，且无 Fatal。该噪声未删除、未伪装成测试失败或源码失败。

## 6. Changed-file 回归门禁

本轮改动的 3 个生产/测试文件全部位于 `Source/ShanmenCombatRuntime`，按 changed-file 映射要求必须覆盖 `Shanmen.0_0_10.CombatRuntime`。

门禁结果：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=p60_shanmen_full_final.log
REGRESSION_COVERAGE: Log=p60_shanmen_full_final.log Group=Shanmen.0_0_10 Success=129 SHA256=67F39653A1ED51F7A82A87ABF92E796526BCD8C93A24DB0FA8BB4698CE008DD3
```

最终父组是所需 CombatRuntime 组的严格超集，因此门禁通过。

## 7. 构建与静态检查

Editor Development：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `8/8` actions；
- `Result: Succeeded`；
- 原生退出码 `0`；
- 总执行时间 `38.08s`。

Game Development：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `5/5` actions；
- `Result: Succeeded`；
- 原生退出码 `0`；
- 总执行时间 `27.24s`；
- 生成 `Binaries/Win64/demo_map.exe`，未启动。

静态结果：

- `git diff --check`：退出码 `0`；
- 新文件中无 `demo_map` / `ShanmenItems` include；
- 无 `UWorld`、`AActor`、`ApplyDamage`、`UGameplayStatics`；
- 无 `FMath::Rand` 或 `FRandomStream`；
- 首轮聚焦测试、父组测试及两目标构建均一次成功，没有需掩盖的首次失败。

## 8. 修改范围与兼容性

新增：

- `Source/ShanmenCombatRuntime/Public/ShanmenControlledWeaponExecution.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenControlledWeaponExecution.cpp`；
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenControlledWeaponExecutionTests.cpp`；
- 本 Report 与 Development Log。

生产/测试源码共约 1,155 行新增，没有修改既有生产文件。

兼容性：

- 复用既有 Action、Detector、Impact、Defense 与 Ledger 契约；
- 不改变 Profile schema、ShanmenItems authority document、DefinitionId 或存档格式；
- 不为受控飞剑建立第二套物品真值；
- 通用 Projectile 行为不变；
- 长期未跟踪的历史 Prompt、Report、PDF 与用户文档均不纳入提交。

## 9. P/F 边界

本 Report 只包含 P 阶段源码、静态审查、无头 `-NullRHI` Automation、Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 10. 后续建议

P6.1 可建立单向产品 Adapter：先从当前 active Run 的 ShanmenItems 权威验证 exact flying-sword ItemInstanceId，再由产品层驱动 Actor/移动/输入/VFX，把真实几何接触转换为本契约的 ControlledObject candidate。

Adapter 不应直接修改物品、生命或库存，也不应把受控飞剑降级为 generic Projectile detector。耐久/修理/回收成本应等待规则冻结后接入既有资源事务，而不是在移动层临时扣值。

## 11. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-0-controlled-flying-sword-contract/Docs/Report/Dev.D.UE.0.0.10.P6.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-0-controlled-flying-sword-contract/Docs/Log/Dev.D.UE.0.0.10.P6.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-0-controlled-flying-sword-contract>
