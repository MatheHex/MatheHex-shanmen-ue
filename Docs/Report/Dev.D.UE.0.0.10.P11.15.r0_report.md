# Dev.D.UE.0.0.10.P11.15.r0 Report

## 1. 结论

P11.15 已完成并通过 P 阶段门禁。

本阶段为 BasicSword、ThrownWeapon、SpiritEvasion 与 WeaponGuard 建立一条统一、确定性、可审计的玩家动作互斥通道。现有 Product Host 继续拥有各自生命周期状态；CombatRunCoordinator 只签发 Run-scoped typed arbitration receipt；GameMode 只读取现有 Host 占用并执行唯一允许的 WeaponGuard 精确抢占，没有在 PlayerController 或 GameMode 中建立第二套动作状态机。

最终策略：

- 空闲动作通道允许四类动作进入；
- active WeaponGuard 收到再次 Guard 时返回 `AlreadyActive`；
- active WeaponGuard 收到 BasicSword、ThrownWeapon 或 SpiritEvasion 时，必须先以 exact HostId 完成 typed `PlayerActionPreempted`；
- ThrownWeapon `InFlight` 或 SpiritEvasion 非终态时，拒绝所有新动作；
- 同时观察到多个 active product 时 fail closed；
- 只有 WeaponGuard 可被抢占，其余产品绝不被仲裁层隐式终止；
- 每次结构有效的决策都由 Combat Run 单调序列派生确定性 CommandId；策略冲突也保留可重放 identity。

最终结果：

- 0.0.10 全量 `540/540`；
- 六组映射旧回归 `146/146`；
- 六组 focused `41/41`；
- 13 份最终日志原始合计 `727 Success / 0 Fail`，按 test identity 去重为 `686`；
- changed-file gate、166 项 gate self-test、静态审查、Editor/Game Development 构建全部通过。

## 2. 功能性

### 2.1 确定性仲裁契约

新增 `Fdemo_mapShanmenPlayerActionArbitrationPolicy`、占用快照、arbitration receipt 与 gate result。纯策略只读取：

- 请求动作类型；
- active WeaponGuard 及 exact HostId；
- ThrownWeapon 是否 `InFlight`；
- SpiritEvasion 是否非终态。

CommandId 使用现有 `FShanmenCombatIdFactory::MakeActivationId`，canonical definition 为 `Action.Player.ExclusiveLane.Arbitration.r1`。相同 RunId、PlayerEntityId、sequence 与 action 输入产生相同 identity；没有随机 GUID、wall clock、frame counter 或 RNG。

结构错误在 identity 生成前拒绝且不消耗序列。通道占用冲突属于结构有效的产品决策，会获得 deterministic identity 并消耗一次序列，因此拒绝审计也可回放。Run teardown 与 Reset 都把 arbitration sequence 恢复为 1。

### 2.2 exact WeaponGuard 抢占

GameMode 从唯一 `WeaponGuardProductSession` 读取 active HostId。只有 policy 返回 `WeaponGuardPreemptionRequired` 时，才发送 typed `PlayerActionPreempted` 终止原因。

后续动作只有在以下条件全部成立时才被授权：

1. terminal result 为 `Interrupted`；
2. terminal HostId 等于 arbitration receipt 中要求退休的 HostId；
3. WeaponGuard Session 已为空；
4. gate receipt 自身结构有效。

HostId 不匹配、终止拒绝或 Session 未清空都会返回 valid fail-closed gate，不会伪造授权。再次 Guard 不终止现有 Host，而是形成 `AlreadyActive` receipt。

### 2.3 四条产品入口

- **BasicSword**：先验证装备 weapon identity 与 offense snapshot，再通过 gate，最后执行既有 CombatRunCoordinator sweep；ActionGate 被保留在 execution result。
- **ThrownWeapon**：直接 hotbar intent 与物理输入 adapter 均接入 gate。物理输入先完成 slot/source/aim/SelectionId/intent 校验，再仲裁；拒绝不会推进 selection ordinal、修改 authority 或启动 Host。
- **SpiritEvasion**：ProductStart capture 后、command dispatch 前仲裁；拒绝不会向 Component 发送启动命令。
- **WeaponGuard**：item authority 可用后、Session `TryStart` 前仲裁；active Guard 的重复 start 保持幂等，冲突返回 typed `ActionConflict`。

### 2.4 fail-closed 占用投影

GameMode 不复制 product state，只生成一次只读 snapshot：

- Guard 来自 `WeaponGuardProductSession`；
- Thrown 来自 `ThrownWeaponProductLifecycle` 的 `InFlight` 状态；
- Spirit 来自现有 Component 的 `CanStart()`。

Guard/Thrown lifecycle 若自身结构无效，snapshot 会被构造成明确的 invalid shape，使仲裁拒绝，而不是把损坏状态误报为空闲。多个现有 owner 同样拒绝所有请求。

## 3. 完整性

测试覆盖：

1. 四类动作在空通道全部 Granted；
2. Guard + Guard 为 AlreadyActive，Guard + 其余三类动作要求 exact preemption；
3. Thrown InFlight 与 Spirit nonterminal 分别拒绝全部四类新动作；
4. 多 owner 状态 fail closed，且策略冲突 receipt 保留 deterministic identity；
5. 相同 canonical 输入重放相同 CommandId；
6. malformed occupancy 不消耗 sequence，policy conflict 消耗 sequence，Run release 重置 sequence；
7. 错误 HostId、typed preemption failure 与 exact HostId 成功路径；
8. 默认 arbitration receipt / gate 是明确的无证据哨兵，不能被误判为有效拒绝；
9. Spirit route ActionConflict 不 dispatch；
10. Thrown input ActionConflict 不推进 ordinal、不修改 authority、不创建 Host；
11. WeaponGuard 新 typed preemption reason 的 terminal receipt 与既有七类终止回归；
12. 既有 CombatCore、items、attributes、enemy、ranged、hotbar 与完整 0.0.10 回归全部通过。

## 4. 兼容性与权威边界

- CombatRunCoordinator 只拥有 arbitration command identity 和单调 sequence，不拥有四类 Product Host；
- GameMode 只负责读取现有 Host、调用唯一产品 route 与核验 exact preemption proof；
- PlayerController 未增加动作状态、互斥 bool 或产品生命周期写入口；
- WeaponGuard Session、Thrown lifecycle 与 Spirit Component 继续分别拥有其真实状态；
- ItemSubsystem/ItemAuthority 继续拥有装备与 hotbar 真值；
- CombatCore、Impact、vitality、伤害公式与防御层顺序未修改；
- 既有直接 ProductSession/Coordinator 单元测试仍可在没有 GameMode gate 的隔离层运行；可选 gate 以无效默认哨兵表示，不把默认拒绝伪装成已参与仲裁；
- 新增行未调用 `ApplyDamage`、`TakeDamage`、`FGuid::NewGuid`、RNG、frame counter 或 wall clock；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与其它用户资料未修改、未暂存、未提交。

## 5. 修改范围

实现、测试与 regression mapping 共 19 个文件，1,201 行新增、12 行删除（含 3 个新文件，不含本 Report/Log 与原始证据日志）：

- `Source/demo_map/demo_mapShanmenPlayerActionArbitration.h`
- `Source/demo_map/demo_mapShanmenPlayerActionArbitration.cpp`
- `Source/demo_map/demo_mapShanmenPlayerActionArbitrationTests.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinator.h`
- `Source/demo_map/demo_mapCombatRunCoordinator.cpp`
- `Source/demo_map/demo_mapGameMode.h`
- `Source/demo_map/demo_mapGameMode.cpp`
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductRoute.h`
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductRoute.cpp`
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductRouteTests.cpp`
- `Source/demo_map/demo_mapShanmenThrownWeaponInputAdapter.h`
- `Source/demo_map/demo_mapShanmenThrownWeaponInputAdapter.cpp`
- `Source/demo_map/demo_mapShanmenThrownWeaponInputAdapterTests.cpp`
- `Source/demo_map/demo_mapShanmenThrownWeaponProductSession.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductSession.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductSession.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductSessionTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

Regression map 从 102 条扩展为 103 条；新 `PlayerActionArbitration` rule 强制 focused policy、CombatRunCoordinator 与 broad 0.0.10 三层证据。Self-test 从 164 扩展为 166，分别证明完整证据通过与只有 focused 证据时 fail closed。

## 6. 测试覆盖

| Group | Success | Fail | Queue | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.PlayerActionArbitration` | 3 | 0 | 1 | `BE0E785F9988CFF3A577741D4F1DB29B3C42E044CA8C002D001894C46A1FFE5A` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 17 | 0 | 1 | `5C2D0D633EAA9EC0CBC7C005C46E125F66ECCE31CA8E4AEAAEB4368842D07456` |
| `Shanmen.0_0_10.Product.SpiritEvasionProductRoute` | 7 | 0 | 1 | `E9E463407160639BC6D8916296B33C162845225E25ABB6C49A4243810E0BC751` |
| `Shanmen.0_0_10.Product.ThrownWeaponInputAdapter` | 4 | 0 | 1 | `FD8C06B5F6C03DC74C8A44BE0EED66FCFE49E3DCF7AA941320CABB6C485758FA` |
| `Shanmen.0_0_10.Product.WeaponGuardProductSession` | 7 | 0 | 1 | `D8CC26C7D8EAD24DA5AF582C9E2B1A50D7047CAE448DDDDF2B18C99CCE9665E0` |
| `Shanmen.0_0_10.Product.WeaponGuardImpactRoute` | 3 | 0 | 1 | `E92FF420759E75A39CA6C875B4E6C807F37AC6B3B7798611D2AE617509F8AF8D` |
| `Shanmen.0_0_10` | 540 | 0 | 1 | `B64B08577C7F3785457CBFC893574C0AF4CFDA16D38F4B389A7FD53D4F2CACCB` |
| `demo_map.V3.Attributes` | 4 | 0 | 1 | `11EB930F8D26639E5F905656DFF809A3E5B62DF5263600E970890ABB600979BB` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 1 | `DF8BC1228998BC61C156544244D33933A76C45E1EF70968D00495D7E46D79506` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `6A1C95D0E6CB09327C4A43816CFF3C210EF3AA4444BE51263023B2A112DC4BA2` |
| `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `541D2399D5267CF967F3C8C230961293F851AB881D39E32D14EB815702BF4AF8` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `3A4376D781DDBCDDF61A4F94E1D66C1D5F5FFF95C292BFA07F693306F1EAC576` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | `0329EEBA17BE465DD798A2E18212F5DBFFE85DF912CE347EC3CB315DC6763899` |

十三份最终日志均只有一个 canonical `Automation RunTests <group>` 命令、一个原生 queue-empty / TEST COMPLETE marker、Fail 0，且无 Fatal、Unhandled Exception 或 Ensure。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=103
SELF_TEST: PASS 166/166
REGRESSION_COVERAGE: PASS Changed=19 Rules=7 Required=43 Logs=13
ADDED_AUTHORITY_SCAN: PASS AddedSourceLines=492 ForbiddenHits=0
PLAYER_ACTION_GATE_ROUTE_CALLERS: 5 product entry routes
DIRECT_ARBITRATION_POLICY_PRODUCTION_CALLERS: 1 (CombatRunCoordinator only)
git diff --check: PASS (native exit 0)
```

最终 changed-file gate 日志 SHA-256：`C4FB0C4DBAC87A0D0FC88D12B99B2A41DF5BC1F09D13D618D5B9690B7C9359FC`。Self-test 日志 SHA-256：`15F7374CDBCDF3970528D2BBE94056B9F6B37342044CB23412F0F9ED8C275B01`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / time | Exit | Final log SHA-256 |
|---|---|---|---:|---|
| Editor implementation after compatibility fix | Succeeded | 79 / 197.38s | 0 | 过程输出已核对 |
| Game implementation | Succeeded | 78 / 210.97s | 0 | 过程输出已核对 |
| Editor final | Succeeded | 4 / 9.77s | 0 | `BB79F5C3E613044179283C8775E09DAF406F7CD84E9355940BB34C5E345968C8` |
| Game final | Succeeded | 3 / 11.31s | 0 | `4B28E96DD4DE53BADCC2BEA08C56AE682B9B0E13249ECB04BBDDB0B6CC794C35` |

最终产物：

- `UnrealEditor-demo_map.dll`：12,926,464 bytes，SHA-256 `C4372D36C5488598EEC320B9DFEF337E1B9E7E0BD4555541FF8BC9D7CDA691D0`；
- `demo_map.exe`：354,429,440 bytes，SHA-256 `D148A98BDDA6CFD1382D4E264ACBD3A98B9BA848A9E16817317EE9BC12BB8B4A`。

## 9. 真实异常与修复

### 9.1 首次完整回归失败

首次 `Shanmen.0_0_10` 得到 530 Success / 10 Fail，UE TEST COMPLETE 为 `-1`，进程原生退出码 `255`。全部失败集中在既有 WeaponGuard ProductSession / ImpactRoute fixture。

根因不是 Guard 产品逻辑，而是新可选 `ActionGate` 的默认 arbitration receipt 使用 `CoordinatorNotReady`，该默认值按 receipt 契约会被视为“有效拒绝”，从而让没有接入 GameMode gate 的既有隔离层结果被误判为 invalid。修复为：默认 receipt 使用 `Error=None` 表示无证据哨兵，所有真实拒绝仍由 Coordinator/Policy 显式设置 typed error；同时新增两条直接断言锁定 default receipt / gate 必须 invalid。

失败日志保留为 `Dev.D.UE.0.0.10.P11.15.r0_automation_shanmen_full_first_failed.log`，SHA-256 `451EBFB66BDD31E0D36689B440A126B6ED9620EA1211258D8E5B095313824DCA`。修复后 Guard Session 7/7、ImpactRoute 3/3、全量 540/540。

### 9.2 首次 evidence gate 失败

第一轮 Automation 命令沿用了历史 `;Quit` 后缀。测试 case 与进程本身均成功，但最短的 Attributes 组在 TestExit 写出 terminal marker 前被 Quit 收口；changed-file gate 因 `terminal completion marker missing` 正确返回失败（shell exit 1）。

没有放宽 parser 或 gate。最终 13 组全部移除 `;Quit`，只由 `-TestExit="Automation Test Queue Empty"` 收口并完整重跑，最终 gate 通过。

### 9.3 环境信息

UE SDK 检查继续报告未安装的非 Win64 平台 metadata；同一输出明确 Win64 SDK `10.0.22621.0` VALID。没有 C3859、C1076、1455、源码编译失败、Fatal、Unhandled Exception 或 Ensure。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、实现、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P11.15 已关闭四类现有玩家动作的 Host 并发入口。下一阶段建议把同一 gate contract 扩展为面向后续主动术法／控制飞剑产品的注册式占用投影，避免每新增产品都在 GameMode 增加新的 bool；扩展前应先定义可抢占等级与 exact terminal proof，继续禁止第二套全局状态机。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-15-player-action-arbitration>
