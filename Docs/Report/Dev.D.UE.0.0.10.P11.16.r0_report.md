# Dev.D.UE.0.0.10.P11.16.r0 Report

## 1. 结论

P11.16 已完成并通过 P 阶段门禁。

本阶段把 P11.15 的固定布尔占用快照替换为注册式 typed claim 投影。WeaponGuard、ThrownWeapon 与 SpiritEvasion 仍分别由既有 Product Session、Lifecycle 与 Component 持有真实状态；GameMode 只在一次动作路由边界读取这些 Host，并把当前占用注册为带类型、稳定身份和抢占等级的只读 claim。没有新增全局动作状态机、可变注册表、Tick、计时器或第二份产品真值。

当前契约为：

- 每个非终态产品以 `OwningAction + OwnerId + Preemption` 注册一个 claim；
- WeaponGuard 必须声明 `ExactOwner`，且仍是唯一可抢占产品；
- ThrownWeapon 与 SpiritEvasion 必须声明 `None`，仲裁层不得隐式终止；
- claim 缺少身份、类型与抢占等级不匹配、或同一产品重复注册时，snapshot 在 identity 分配前 fail closed；
- 多个不同产品同时占用属于结构有效但策略冲突的状态，继续签发可重放拒绝 receipt；
- 单一阻塞者拒绝会在 receipt 中保留 `OccupyingAction` 与 `OccupyingOwnerId`；
- P11.15 的 exact WeaponGuard terminal proof 保持不变。

最终结果：

- focused `PlayerActionArbitration`：`4/4`；
- 0.0.10 全量：`541/541`；
- 四组按改动路径推导的旧回归：`116/116`；
- 六份日志原始合计 `661 Success / 0 Fail`，按 test identity 去重为 `657`；
- changed-file gate：`Changed=8 / Rules=6 / Required=41 / Logs=6`；
- regression gate self-test：`166/166`；
- `git diff --check`、静态边界审查、Editor/Game Development 构建全部通过。

## 2. 功能性

### 2.1 typed claim 契约

新增 `Fdemo_mapShanmenPlayerActionClaim`：

- `OwningAction` 表示实际占用动作；
- `OwnerId` 表示可审计的稳定拥有者身份；
- `Preemption` 明确是 `None` 或 `ExactOwner`。

`TryCreate` 与 `IsValid` 把现阶段规则固化到结构边界：只有持续占用通道的 ThrownWeapon、SpiritEvasion、WeaponGuard 可以形成 claim；BasicSword 是瞬时请求，不能伪装为持久占用；WeaponGuard 只能使用 `ExactOwner`，其余两类只能使用 `None`。未来新增产品必须显式扩展该契约，不能靠增加一个默认为真的 bool 获得抢占能力。

### 2.2 注册式只读投影

`Fdemo_mapShanmenPlayerActionOccupancySnapshot` 现在私有持有 claim 数组，只公开 `TryRegisterClaim`、`Invalidate`、计数和 sole-claim 查询：

- default snapshot 表示空闲通道；
- 无效 claim 会污染完整 projection，使仲裁在生成 CommandId 前拒绝；
- 相同 `OwningAction` 重复注册会使 projection 无效；
- 多个不同且有效的 claim 被保留为可识别的多 owner 冲突；
- snapshot 不拥有、推进或终止任何 Product Host。

GameMode 每次捕获时即时注册：

- WeaponGuard 使用现有 active HostId；
- ThrownWeapon 使用冻结 Action Snapshot 的 ActivationId；
- SpiritEvasion 使用现有 Product HostId。

Thrown lifecycle 仅增加一个只读 `GetOccupancyOwnerId` 透传；没有复制 action、Run 或 projectile 状态。

### 2.3 阻塞者身份回执

arbitration receipt 用 `OccupyingAction` 与 `OccupyingOwnerId` 替代 Guard 专用占用字段：

- Granted receipt 必须没有阻塞者；
- Thrown/Spirit 冲突 receipt 必须携带准确类型与身份；
- MultipleActiveProducts receipt 不伪造一个“代表性 owner”；
- AlreadyActive 与 WeaponGuardPreemptionRequired 必须携带 WeaponGuard 类型和 exact owner；
- AlreadyActive 只允许 Guard 请求，preemption-required 不允许 Guard 请求。

Gate 的真实终止动作仍是 WeaponGuard 专用：只有 retired HostId 与 `OccupyingOwnerId` 完全相等，且 Session 已清空，后续动作才获授权。

## 3. 完整性

新增或扩展测试覆盖：

1. 三类 persistent claim 的合法构造；
2. Guard 不能被注册为 non-preemptible；
3. Thrown 不能被注册为 preemptible；
4. 缺少 OwnerId 的 claim 使整个 projection fail closed；
5. 同一产品重复注册被拒绝且 projection 保持 invalid；
6. 多个不同产品形成 replayable `MultipleActiveProducts`，不伪造 sole owner；
7. Thrown 与 Spirit 的全部四类新请求均拒绝，并携带准确 blocker identity；
8. Guard + Guard 仍为 AlreadyActive；Guard + 其余动作仍要求 exact owner preemption；
9. malformed occupancy 不消耗 sequence，结构有效冲突继续消耗并签发 identity；
10. Thrown physical input 与 Spirit product route 的 typed conflict 入口继续原子拒绝；
11. 0.0.10 全量 541 项与四组改动路径旧回归全部通过。

## 4. 兼容性与权威边界

- Product Session/Lifecycle/Component 继续是唯一生命周期权威；
- CombatRunCoordinator 继续只拥有 arbitration sequence 与 CommandId，不保存 claims；
- GameMode 的 snapshot 是调用栈上的一次性投影，不是持久 registry；
- PlayerController 没有新增互斥状态或产品 bool；
- exact Guard termination 仍走唯一 typed `PlayerActionPreempted` route；
- Thrown 与 Spirit 没有获得可抢占能力；
- CombatCore、Impact、damage、vitality、inventory、schema 与 item authority 均未修改；
- arbitration 文件仍无 `UWorld`、`AActor`、GameMode、RNG 或 wall-clock 依赖；
- 新增源码没有 `ApplyDamage`、`TakeDamage`、`FGuid::NewGuid`、RNG、frame counter 或 wall clock；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

生产代码与测试共 8 个文件，`397` 行新增、`78` 行删除（不含本 Report/Log）：

- `Source/demo_map/demo_mapShanmenPlayerActionArbitration.h`
- `Source/demo_map/demo_mapShanmenPlayerActionArbitration.cpp`
- `Source/demo_map/demo_mapShanmenPlayerActionArbitrationTests.cpp`
- `Source/demo_map/demo_mapGameMode.cpp`
- `Source/demo_map/demo_mapShanmenThrownWeaponProductSession.h`
- `Source/demo_map/demo_mapShanmenThrownWeaponProductLifecycle.h`
- `Source/demo_map/demo_mapShanmenThrownWeaponInputAdapterTests.cpp`
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductRouteTests.cpp`

没有修改 Regression Map：P11.15 已建立的 103 条映射规则完整覆盖本轮 8 个改动路径；预检准确要求四组旧回归，证明映射没有被绕过。

## 6. 测试覆盖

| Group | Success | Fail | Queue | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.PlayerActionArbitration` | 4 | 0 | 1 | `01530259219193FE572EFA0BC4DE9E1E8A62117B9CA310B9FAD6434522F1056D` |
| `Shanmen.0_0_10` | 541 | 0 | 1 | `BE900CDCE87AB2F4EDE4206632CA674DD684CA7BAC1CA2CE5844F4468A192789` |
| `demo_map.V3.Attributes` | 4 | 0 | 1 | `6697CA25ADDCCEB3E54C4002CE18B2DDA9E61FAF7C927F5DC2B334EEFBD6C372` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 1 | `A06331B7D22C27892A518CAA2F11849E7D54F539C6A2C2FEE22CA9342CFF2747` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `90326AD1ABFBB01663EA4CD50E383AD6786DF50BDA0C07FFC169F32B2CE0E09B` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `E41189979EC9A601D664CE1AD876A52DD739A75CE90AD1845DC3638EF7F2278A` |

六份最终日志均只有一个 canonical `Automation RunTests <group>` 命令、一个 native queue-empty marker、Fail 0，且无 Fatal、Unhandled Exception 或 Ensure。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=103
SELF_TEST: PASS 166/166
REGRESSION_COVERAGE: PASS Changed=8 Rules=6 Required=41 Logs=6
STATIC_REVIEW: PASS AddedSourceLines=405 ForbiddenHits=0
PRODUCTION_CLAIM_REGISTRATION_CALLS=3 (GameMode only)
ARBITRATION_BOUNDARY_HITS=0
git diff --check: PASS (native exit 0)
```

最终 changed-file gate 日志 SHA-256：`3A2CF02289EA566A316173B319351A88E4EC9261A0C97C4BE3689D843D9B8C72`。Self-test 日志 SHA-256：`15F7374CDBCDF3970528D2BBE94056B9F6B37342044CB23412F0F9ED8C275B01`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor | Succeeded | 79 / 228.40s | 0 | `35F8D51A54534F7E3D973FA07EBFAFDB748CAD7E2E47D799C5721FEC0E15401B` |
| Game | Succeeded | 78 / 210.03s | 0 | `316551FB3CE58D35C822E37FD30A862B68FD1C8754BA43B3280A763AA2C930CE` |

最终产物：

- `UnrealEditor-demo_map.dll`：12,933,120 bytes，SHA-256 `7E8334422B7A393A633E9CA9A7443C9C5C1C49846930FD01797D326F552A496B`；
- `demo_map.exe`：354,435,584 bytes，SHA-256 `58932B560F4377A471DB1E8FD067904E80FF6DF751FDA5B3CC2DCFE511BBBB87`。

## 9. 异常与修复

没有源码编译失败、Automation 失败、Fatal、Unhandled Exception、Ensure、C3859、C1076 或系统 1455。

首次 changed-file 预检只输入 focused 与 full 两份日志，门禁按预期返回退出 1，并准确列出缺少：`demo_map.EnemySkillFramework`、`demo_map.ItemUseAndArmor`、`demo_map.V2RangedCompatibility`、`demo_map.V3.Attributes`。没有放宽映射或伪造覆盖；四组完整运行后，正式门禁以退出 0 通过。

UE SDK 检查仍报告本机未安装的非 Win64 平台 metadata；同一输出明确 Win64 SDK `10.0.22621.0` VALID，不属于本阶段源码或目标平台失败。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、实现、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P11.16 已消除每增加产品就在 snapshot 增加固定 bool 的扩展债。下一阶段应在出现第一个真实的长持续主动术法或受控武器启动入口时，直接注册其 typed claim，并先定义它的终态证明与抢占等级；在没有真实第五个产品前，不建立抽象的可变全局 provider registry。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-16-player-action-claims>
