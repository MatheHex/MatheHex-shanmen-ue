# Dev.D.UE.0.0.10.P11.17.r0 Report

## 1. 结论

P11.17 已完成并通过 P 阶段门禁。

本阶段关闭了玩家动作通道在 WeaponGuard 抢占后的 TOCTOU 缺口。旧路径只证明 exact Guard Host 已经按 typed `PlayerActionPreempted` 终止并从 Session 清空，随后立即授权原请求；现在必须在终止完成后重新投影 WeaponGuard、ThrownWeapon 与 SpiritEvasion 三个既有 Product Host，只有新投影结构有效且通道确实为空，原请求才获授权。

最终结果：

- focused `PlayerActionArbitration`：`4/4`；
- 0.0.10 全量：`541/541`；
- 四组按改动路径推导的旧回归：`116/116`；
- 六份 Automation 日志原始合计 `661 Success / 0 Fail`，按 test identity 去重为 `657`；
- changed-file gate：`Changed=4 / Rules=2 / Required=40 / Logs=6`；
- regression gate self-test：`166/166`；
- `git diff --check`、静态边界审查、Editor/Game Development 构建全部通过。

## 2. 功能性

### 2.1 抢占后重新投影

GameMode 的抢占顺序现在为：

1. 捕获首次 occupancy，并取得 Run-scoped deterministic arbitration receipt；
2. 只对 receipt 指定的 exact WeaponGuard Host 发送 typed terminal intent；
3. 核验 terminal status、HostId 与空 Guard Session；
4. 再次调用同一个 `CapturePlayerActionOccupancy()`，读取全部既有产品权威；
5. 只有第二次投影合法且 owner 数量为零，才返回授权。

第二次捕获不重新仲裁、不消耗新的 CommandSequence，也不建立持久状态；它只是对已签发决策执行结果的一次 postcondition 验证。

### 2.2 typed postcondition 证据

新增 `Edemo_mapShanmenPlayerActionPostPreemptionObservation`：

- `NotObserved`：不涉及 Guard 抢占；
- `Empty`：exact Guard 已退休，且重捕获后通道为空；
- `Occupied`：Guard 已退休，但终止过程中出现了另一个 owner；
- `Invalid`：Guard 已退休，但新的 occupancy projection 结构损坏。

后两种状态分别使用 `PostPreemptionLaneOccupied` 与 `PostPreemptionProjectionInvalid` fail closed。拒绝结果仍保留 `RetiredWeaponGuardHostId`，因此审计端能够准确区分“抢占失败”和“抢占成功但 postcondition 失败”。

### 2.3 未伪造第五种独占产品

本阶段审计了现有 ControlledWeapon、Formation 与 SpiritShield：

- 飞剑 Orbit 是攻击、威胁、防御和发射的共同准备姿态，应允许与玩家即时动作共存；
- 已部署阵法属于持续 World 影响，不应在完整存续期间锁死动作通道；
- 已启动 SpiritShield 属于持续短效防御，不自动等于玩家不能攻击或移动；
- 三者目前都没有一条已经落地且需要独占通道的第五种物理输入 start route。

因此没有为了“扩展性”增加虚构 claim、可变全局 registry 或第二套产品状态机。未来真实 start route 出现时，再按其准备动作与 terminal proof 接入。

## 3. 完整性

测试新增并锁定：

1. exact Guard 退休 + 空 post projection 才能授权；
2. 错误退休 HostId 继续 fail closed；
3. typed Guard 终止拒绝继续可审计；
4. exact Guard 已退休但出现 Thrown owner 时，保留退休证据并拒绝；
5. exact Guard 已退休但 post projection invalid 时，保留退休证据并拒绝；
6. success result 必须携带 `Empty` observation；
7. postcondition 拒绝的 error、observation 与 retired HostId 必须互相一致；
8. 全量 0.0.10 与四组改动路径旧回归全部通过。

## 4. 兼容性与权威边界

- WeaponGuard Session、Thrown lifecycle 与 Spirit Component 仍是唯一生命周期权威；
- GameMode 只进行两次调用栈内只读投影，不缓存 claims；
- CombatRunCoordinator 不保存 post projection，也不为重验证分配第二个 identity；
- PlayerController 未增加互斥 bool 或产品状态；
- existing Guard/Thrown/Spirit action kinds 与抢占等级未改变；
- ControlledWeapon、Formation 与 SpiritShield 没有被误分类为持续独占动作；
- CombatCore、Impact、damage、vitality、inventory、schema 与 item authority 未修改；
- 新增源码未调用 `ApplyDamage`、`TakeDamage`、`FGuid::NewGuid`、RNG、frame counter 或 wall clock；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

生产代码与测试共 4 个文件，`120` 行新增、`10` 行删除（不含本 Report/Log 与证据日志）：

- `Source/demo_map/demo_mapGameMode.cpp`
- `Source/demo_map/demo_mapShanmenPlayerActionArbitration.h`
- `Source/demo_map/demo_mapShanmenPlayerActionArbitration.cpp`
- `Source/demo_map/demo_mapShanmenPlayerActionArbitrationTests.cpp`

Regression mapping 无需修改；现有 `GameMode` 与 `PlayerActionArbitration` 两条规则已经从改动文件推导出 40 个 required groups。

## 6. 测试覆盖

| Group | Success | Fail | Queue | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.PlayerActionArbitration` | 4 | 0 | 1 | `DEBA349B4F22F1A1BFC0E11A56ABBA4B23474DF3131D4552879F5CC51272E61F` |
| `Shanmen.0_0_10` | 541 | 0 | 1 | `AF2CBF504BAFA84F7B667B2BCC8A3E672204256221DDBC6CFAB7F5C91E322FE3` |
| `demo_map.V3.Attributes` | 4 | 0 | 1 | `F6647A2BB681D592EEDE8944E63F83B350F0FC4B2AB00796C7F2D33FA095D5F1` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 1 | `AA1D3BF0FCE6F16A6BBE3907CB408519D354108FF58BFF0070C389A76683AC64` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `ED02D73ACCD529DE096B2012DAC79A3468BCB576E477901E5163BF2E5C3E36BE` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `8D2EA3BDB62B0C65F2AB3335F85A4B97FEF4AD6A192569B8AF94F2876D62207A` |

六份最终日志均包含一个 canonical `Automation RunTests <group>` 命令、一个原生 queue-empty / TEST COMPLETE marker、Fail 0，且无 Fatal、Unhandled Exception 或 Ensure。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=40 Logs=6
SELF_TEST: PASS 166/166
ADDED_SOURCE_SCAN: PASS AddedSourceLines=124 ForbiddenHits=0
git diff --check: PASS (native exit 0)
```

最终 changed-file gate 日志 SHA-256：`98E338B395B01ECAFF464943DA73988F0F80775316578233AB941FC07EAB45BF`。Self-test 日志 SHA-256：`15F7374CDBCDF3970528D2BBE94056B9F6B37342044CB23412F0F9ED8C275B01`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor first | Failed: test fixture identifier | 76 actions attempted / 222.06s | 1 | `95622D14B4E7DD8E5A01CB4B774E982D7C8ABA0801C6974EEA4B45E450235765` |
| Editor after fix | Succeeded | 4 / 5.40s | 0 | `577A25C431838CEFD21648BB10D0433A82145016D01477F353E7AD0274E1F176` |
| Game | Succeeded | 78 / 208.87s | 0 | `1FD20940371E3323B7A51440A54E4933AB00B6378F891E9D1CA803A1EFF96594` |

最终产物：

- `UnrealEditor-demo_map.dll`：12,935,168 bytes，SHA-256 `2E30C173192AB323166AFB69E50E2F5C6A768124CEC5869EFAB16F709ED06C00`；
- `demo_map.exe`：354,436,608 bytes，SHA-256 `AF204E2C04A3C8A49C6A4CA6EF1932E0E79126A249301F98DA9F33079F755719`。

## 9. 真实异常与修复

首次 Editor 构建原生退出码 `1`，UBT 最终为 `OtherCompilationError`。生产代码已经编译通过，失败位于新增测试：post-occupancy fixture 引用了不存在的 `ArbitrationThrownHost`，现有常量实际名为 `ArbitrationThrownOwner`。

修正该单一标识符后，Editor 增量构建 4/4、focused 4/4、全量 541/541、旧回归 116/116、Game 78/78 全部成功。该失败是测试源码错误，不是 Windows commit memory、页面文件、SDK 或工具链环境故障；没有出现 C3859、C1076 或系统代码 1455。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、实现、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P11 的四类玩家动作互斥与 exact Guard 抢占链现在同时具备 precondition、terminal proof 和 postcondition。下一阶段不应继续为不存在的第五输入预造动作注册层；应进入下一个真实产品纵切，并在其正式输入 start route 出现时判断“仅启动动作独占”还是“完整生命周期独占”，再复用现有 typed claim 契约。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-17-post-preemption-revalidation>
