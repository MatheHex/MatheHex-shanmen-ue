# Dev.D.UE.0.0.10.P24.5.r0 Development Log

## 1. 目标

- 从 P24.4 的可见剑气载体继续审计真实玩家发射边界；
- 阻止剑气在生成点被占用或源点到生成点被墙体切断时发布；
- 保留现有复制后发布、Projectile、RunHost、Session、伤害与 HUD 权威；
- 用真实碰撞 World 覆盖失败关闭和清路恢复；
- 按改动文件映射完成精确回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线：`750875df516e1803ce0c15addf900864fd0a1323`（P24.4）；
- 分支：`agent/0.0.10-p24-5-sword-qi-launch-clearance`；
- 开始时 tracked tree clean；
- 用户 103 个未跟踪文件保持原样；
- 原始规划将剑气定义为剑法延伸出的距离攻击；本轮不新增平衡、资源、穿透或伤害规则。

## 3. 缺口审计

现有 `Ademo_mapShanmenThrownWeaponProjectile` 已在暂存前执行发射体积 overlap 和源点到发射点 sweep；Sword Qi 的对应 Actor 只验证身份并直接设置 Transform。

因此真实产品链存在两个可重现的结构性缺口：

1. 发射点落在墙体内时仍可进入 InFlight；
2. 发射点本身清空，但玩家与发射点之间有薄墙时仍可跨墙生成。

决策是复用现有 Projectile 的碰撞球和 World 查询模式，不增加预览、Manager、Subsystem、第二 Actor 或通用包装层。

## 4. 生产实现

### 4.1 暂存错误

在现有剑气 Projectile 增加非反射枚举 `Edemo_mapShanmenSwordQiProjectileStageError`：`None`、`ContractRejected`、`LaunchPathBlocked`。

`TryStageLaunch()` 的可选输出保持旧调用兼容。精确 staged replay 返回 `None`；任何普通契约失败返回 `ContractRejected`；真实物理阻挡返回 `LaunchPathBlocked`。

### 4.2 World 与碰撞门禁

产品暂存要求 SourceActor 与 Projectile 位于同一 World。无 World 的纯合同测试仍允许执行，但真实 World 不可与 detached source 混用。

查询使用现有 `USphereComponent` 的 `GetScaledSphereRadius()`、object type 和 response channels，并忽略 SourceActor 与 Projectile：

- `OverlapBlockingTestByChannel()` 验证完整生成体积；
- `SweepTestByChannel()` 验证 SourceActor 位置到 Launch Origin 的完整走廊；
- 零长度走廊在 overlap 通过后直接成立。

查询发生在 LaunchReceipt、HitContext、Owner、Instigator、Collision、Movement、Transform 与 State 的任何写入之前。

### 4.3 错误传播

WorldAdapter 将内部 `LaunchPathBlocked` 映射为外层同名错误；其它暂存拒绝仍为 `ProjectileStageRejected`。现有 RunHost、Session 与 Controller 保留嵌套结果，无需新增路由。

## 5. 自动化 fixture

`FSwordQiCollisionWorldFixture` 创建：

- `EWorldType::GamePreview` 临时 World；
- scenes、physics scene 与 trace collision；
- 已登记到 CombatRunCoordinator 的玩家 Pawn；
- QueryOnly / WorldStatic / BlockAll 的 2 cm 薄障碍；
- AlwaysSpawn、NoCollision 的现有 Sword Qi Projectile。

新增测试 `Shanmen.0_0_10.Product.SwordQiWorldDelivery.LaunchCorridorGate` 顺序验证：

1. 障碍移到 Launch Origin，返回 `LaunchPathBlocked`；
2. 障碍移到 Source 与 Launch 中点，endpoint 清空但 sweep 仍拒绝；
3. 障碍移出走廊，原动作与载体成功暂存；
4. 暂存仍可安全取消回 Empty。

每次受阻后同时断言 live Execution、emission、receipt、context、collision、movement、energy blade 和 point light 均未被污染。

## 6. 首次构建与聚焦测试

首次 Editor 构建执行 62 actions，`Result: Succeeded`，native 0。随后：

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `P24.5_SwordQiWorldDelivery_initial.log` | 4 | 0 | 265,942 | `DE033DEC43625EED53CE4D0C60750882957C4D372C35E6DAB126AF9F91B90BD8` |
| `P24.5_SwordQi_initial.log` | 39 | 0 | 307,670 | `AC6FF99C54D7D7571F864818241CE3023E249406F8604FEBDFC07CAA89742ABA` |

两组均自然达到 queue-empty，原生退出码 0，无测试 Fail、Fatal、Unhandled 或 Ensure。

每份 UE 日志在项目测试开始前仍有 13 行引擎 `UnifiedErrorTest` / `LogAutomationTest: Error: Condition failed` 初始化诊断；其数量与 P24.4 基线完全一致，且不属于任何项目 Test Started/Completed 区间。未删除或改写这些原始行。

## 7. 改动文件回归

5 个改动路径只命中 `SwordQiWorldDelivery` 规则。所需组与结果：

| Group | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `SwordQiWorldDelivery` | 4 | 0 | 265,942 | `DE033DEC43625EED53CE4D0C60750882957C4D372C35E6DAB126AF9F91B90BD8` |
| `CombatRunCoordinator` | 18 | 0 | 287,280 | `3F38CF1A9A6D28DE151387DA734FF51DCB67639FC661234A44752DD075BEE224` |
| `WorldGameplay` | 10 | 0 | 270,352 | `E091AAC7EBD79A593CA2870737838214B584E7950B7245431D61ACDDF8298983` |
| `CombatRuntime` | 146 | 0 | 411,108 | `E06713F7E5AE89F57612569F43B3D6E61D5CDDEDED64B3849A78243DE8E8827E` |
| `CombatCore` | 9 | 0 | 269,081 | `0C16EC88BAAD86E032520F603560A4723C2CE6EA60D034F2D362654B3380620A` |

合计 187/0。没有以主题猜测或无关 broad suite 替代路径映射。

覆盖器：`PASS Changed=5 Rules=1 Required=5 Logs=5`，日志 1,444 bytes，SHA-256 `3E3B36FEC3A52F293C0DE5CAB21E5FCD24A6368C5E7669255C4CBCB782876EBD`。

覆盖器自检：442/442，日志 43,595 bytes，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 最终构建

| Target | Result | Native exit | Actions | Bytes | SHA-256 |
|---|---|---:|---:|---:|---|
| `demo_mapEditor Win64 Development` initial | Succeeded | 0 | 62 | 6,248 | `D3BEE15462973A383EC0EABEBC1D5A52EF6A17FF02BBFC3FDCEA2E41B611C08E` |
| `demo_mapEditor Win64 Development` final | Succeeded / up to date | 0 | 0 | 1,021 | `BE6DA44C06319AF37C98FD49B71967D1A0A8C25F30489137997C39E4224A6CF5` |
| `demo_map Win64 Development` final | Succeeded | 0 | 61 | 6,033 | `29E73F77F93F48F1E9960D61621A43E7C68C21F1BFBA2BAD648A0CC8153EF1AC` |

最终 `demo_map.exe` 为 359,705,600 bytes，SHA-256 `A6D4209A2D1D1F831A810DD45A88C49F7CC877CAD6325F2E554ECB205AA4D0CB`；`UnrealEditor-demo_map.dll` 为 18,971,648 bytes，SHA-256 `FF9BA29191E900924C02D1A521B98AD7D07A9A3ADE3AB1702846677B16799B47`。

## 9. 静态与 P/F 边界

- 非文档增量：5 files，`+309/-8`；
- `git diff --check`：native 0；
- 生产新增行对 Timer、RNG、`ApplyDamage`、`SpawnActor`、`Destroy`、Manager、Subsystem：0；
- 测试 fixture 中的 World/Actor 生成不进入产品代码；
- 无新 UCLASS/USTRUCT、Actor、输入、存档、Config 或 Content 资产；
- 最终项目相关进程：0。

P 阶段证明几何门禁、原子失败与恢复路径成立。真实关卡复杂碰撞、动态障碍、网络与玩家手感未做 F 阶段声明；未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. 提交与后续

精确提交：

- `Source/demo_map/demo_mapShanmenSwordQiProjectile.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiProjectile.h`；
- `Source/demo_map/demo_mapShanmenSwordQiWorldAdapter.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiWorldAdapter.h`；
- `Source/demo_map/demo_mapShanmenSwordQiWorldAdapterTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P24.5.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P24.5.r0_log.md`。

用户 103 个未跟踪文件保持未暂存；`Saved/Codex/P24.5` 原始证据保持本地忽略。下一阶段优先补现有剑气链上的明确玩家行为，或在获得授权后进行 P24.4/P24.5 的正式 F 阶段屏幕与手感验收，不复制现有运行时。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-5-sword-qi-launch-clearance>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-5-sword-qi-launch-clearance/Docs/Report/Dev.D.UE.0.0.10.P24.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-5-sword-qi-launch-clearance/Docs/Log/Dev.D.UE.0.0.10.P24.5.r0_log.md>
