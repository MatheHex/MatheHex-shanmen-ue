# Dev.D.UE.0.0.10.P6.23.r0 Report

## 1. 结论

P6.23 已完成环绕飞剑的防御准备证据层，结论为 **PASS**。

一把 exact item 只有在 canonical Action 仍处于 Active、受控武器仍处于 `Orbiting`、真实武器 Actor 位于冻结轨道方程的当前解时，才能生成自校验 readiness receipt。Host 支持调用方显式选择 item 子集并按 GUID 稳定顺序原子采集；移动、发射、Action 终止或 Source 锚点变化都会让旧证据失败关闭。

本轮没有把准备证据解释为真实格挡。既有 `FShanmenDefenseSnapshot` 仍是 Impact 防御真值；P6.23 不创建 `FShanmenDefenseLayer`，不决定覆盖角度、触发窗口、减伤数值、耐久消耗或参与飞剑策略。

## 2. Runtime 状态证据

- 新增不可变 `FShanmenControlledWeaponDefenseReadinessReceipt`；
- readiness identity 由 Run、Owner、Activation、Source Entity、exact item、Action Definition、Content stamp、命令序号检查点与 `Orbiting` 状态确定性派生；
- 只有匹配同一 `FShanmenActionOrchestrator` 的 Active Action 可以捕获；Startup、Directed、Recalled 与 terminal Action 均拒绝；
- `IsOrbitDefenseReadinessCurrent()` 重新核对完整 Action、当前状态与命令序号；Launch 后旧 receipt 立即失效；
- candidate-only Orbit threat emission 不改变命令或姿态，因此不会误消耗 readiness。

## 3. 产品物理位姿证据

- Product Controller 在 Runtime receipt 外冻结当前中心、轨道平面、参考轴、半径、相位与真实武器位置；
- receipt 重算轨道位置，要求 `WeaponLocation == OrbitEquation(Center, Basis, Radius, Phase)`；
- SnapshotId 对所有几何分量使用位级 canonical identity，并统一正负零；
- 刚启动但尚未完成首次物理放置的逻辑 Orbiting item 不能伪装为已准备；
- Source Actor 移动而飞剑尚未跟随时，旧证据和新捕获都拒绝；下一次显式 Orbit step 放置完成后才能重新捕获；
- Orbit 位姿推进会改变 SnapshotId，但在没有控制命令时保留同一 Runtime readiness identity。

## 4. Host 子集、顺序与原子性

- `TryCaptureOrbitDefenseReadinessInOrder()` 只处理调用方明确传入的 exact item 子集；Host 不推断“全部飞剑都参与防御”；
- 输入顺序不影响输出：条目按 item GUID 严格递增；
- 空集合、重复 item、未知 item、Directed item 或物理位姿未就绪均失败关闭；
- 任一条目失败时清空整个输出，不留下部分 readiness；
- `IsOrbitDefenseReadinessCurrent()` 逐项重新验证 Host Run/Source、Action、命令检查点、Orbit 状态、Source 锚点与真实世界位姿；
- 额外绑定的 item 不影响调用方已明确选择的合法子集。

## 5. 自动化证据

最终无头 `-NullRHI` 自动化全部通过：

| Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 9 | 0 | 0 | `0.141s` | `0EF112314FFBD7D24110056C31A4C62966338C8A52BA9906472B7058AB590968` |
| `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 10 | 0 | 0 | `0.176s` | `D9AB96D5418F47723CEB35545DE5CD0164471CC01C200F9298FF3CFA04B5282C` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 37 | 0 | 0 | `0.641s` | `A98F15645DDF7E29DC71114097CBFC926077304361CDA3139D3440BA88347DC6` |
| `Shanmen.0_0_10` | 171 | 0 | 0 | `8.414s` | `5D9A0E11946EFC68A43D5000319F83CEFC7509FBC83A0E6493E382BD49540E83` |

新增两项聚焦证明：

- Runtime `OrbitDefenseReadiness`：确定性重放、threat sample 共存、Launch 与 terminal fence；
- RunHost `OrbitDefenseReadiness`：首次放置要求、逆序输入规范化、重复/未知/混合状态拒绝、移动与 Source 锚点陈旧、显式剩余 item 子集。

## 6. 改动—回归与静态门禁

- `REGRESSION_COVERAGE: PASS Changed=10 Rules=4 Required=12 Logs=4`；
- mapping self-test：`16/16 PASS`；
- `git diff --check`：native exit `0`；
- 新增行对 DefenseLayer/DefenseSnapshot/DefenseResolver、ApplyDamage/TakeDamage、生命提交、物品 Reserve/Consume、Tick、World query、SpawnActor 与 RNG 的边界扫描命中 `0`；
- Source 合计 `+688/-0`，其中生产 `+488/-0`、自动化 `+200/-0`；
- 未修改 Build.cs、GameplayTags、schema、存档、item definition、输入或资产。

## 7. 构建与失败记录

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor integration：`45/45` actions，Succeeded，native exit `0`，`133.26s`；
- identity hardening 首次增量编译：native exit `1`，`OtherCompilationError`，`C2664`；原因是把 `FShanmenContentStamp::Version` 的 `FName` 直接放入 `TArray<FString>`；
- 修正为稳定 `ToString()` 后最终 Editor：`5/5` actions，Succeeded，native exit `0`，`5.39s`；
- 最终 Game：`42/42` actions，Succeeded，native exit `0`，`134.91s`；
- Editor Runtime DLL UTC：`2026-08-29T08:34:40.5946035Z`；
- Editor demo_map DLL UTC：`2026-08-29T08:34:41.6982996Z`；
- Game executable UTC：`2026-08-29T08:39:34.0322475Z`。

该失败是新增源码类型错误，不是内存、SDK 或环境错误；修正后 Editor/Game 均成功。Game executable 只构建，未启动。

## 8. 修改范围与兼容性

- `Source/ShanmenCombatRuntime/Public/Private/ShanmenControlledWeaponExecution*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponSession*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponProductController*`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost*`；
- 本 Report 与同名 Development Log。

P6.0–P6.22 的 command、movement、candidate、threat policy/presence、sample Router、Impact 与 lifecycle API 均保留。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未纳入本阶段。

## 9. P/F 边界与后续

本轮只执行 P 阶段源码、静态检查、无头 Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

下一阶段若把 readiness 转换为真实防御层，产品策略必须先明确：参与 item 子集、空间覆盖、触发时机、可防攻击类型、层顺序和资源成本。P6.23 receipt 只提供这些策略所需的可靠前置证据，不能自行授权减伤。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-23-orbit-defense-readiness/Docs/Report/Dev.D.UE.0.0.10.P6.23.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-23-orbit-defense-readiness/Docs/Log/Dev.D.UE.0.0.10.P6.23.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-23-orbit-defense-readiness>
