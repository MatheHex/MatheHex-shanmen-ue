# Dev.D.UE.0.0.10.P10.2.r0 Report

## 1. 结论

P10.2 **PASS**。本阶段把 P10.1 的 action-bound movement request 接入产品侧显式位移策略：产品先冻结 policy id、请求距离、最低有效距离和 WorldStatic clearance，再与 request 形成确定性 plan；adapter 只做一次只读 capsule sweep preflight，不移动 Character，也不拥有持续时间、轨迹、资源、Timer 或 Tick。

同时移除了通用 `Fdemo_mapCombatDisplacement` 对旧敌人技能配置的反向依赖。clearance 现在是显式参数；所有旧 P6 调用点继续传入原 `WorldStaticSkin`，因此旧行为与 2 cm 安全距离保持不变。

最终验证为产品 adapter `5/5`、P10.1 movement `5/5`、EnemySkillFramework `44/44`、V2RangedCompatibility `22/22`、Profile `211/211`、CodeB `60/60`、0.0.10 全量 `395/395`。七份正式日志合计 `742` 条 Success、`0` Fail；changed-file gate、mapping self-test、边界扫描、`git diff --check`、Editor Development 和 Game Development 均通过。

## 2. 功能性

### 2.1 显式产品策略

`Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture` 要求：

- MovementPolicyId 非空；
- RequestedDistance、MinimumResolvedDistance、WorldStaticClearance 全部有限；
- RequestedDistance 与 MinimumResolvedDistance 均大于零；
- minimum 不大于 requested；
- clearance 不小于零。

代码中没有地面步默认距离或 clearance 默认值；调用者必须从产品权威显式提供。capture 成功后字段只读，避免后续蓝图或调用方修改冻结策略。

### 2.2 request-policy 绑定与确定性计划

`TryBuildPlan` 同时验证 P10.1 request、policy snapshot，以及 request intent 与 policy 的 MovementPolicyId 完全相等。PlanId 由 RequestId、policy id 和三个 float 的 IEEE bits 经 `FShanmenDeterministicId` 派生。

同一输入重放得到同一 PlanId；请求距离、action/request 或 policy identity 改变都会分离身份。`Plan::IsValid` 会重新派生并比对 PlanId，不能以任意 GUID 绕过绑定。

### 2.3 只读 WorldStatic preflight

`PreflightWorldStatic` 使用 P10.1 冻结的 planar direction 和 P10.2 冻结的距离/clearance 调用通用 capsule sweep：

- invalid plan 在访问产品 World 前失败；
- Character 或 World 不可用时返回明确状态；
- resolved distance 小于 minimum 时返回 `InsufficientResolvedDistance`；
- 满足 minimum 且不超过 frozen requested distance 时返回 `Ready`；
- 结果保留 PlanId 与完整 displacement receipt 供后续审计。

adapter 不调用 `MoveCharacterSwept`、`SafeMoveUpdatedComponent`、`LaunchCharacter`、`AddMovementInput` 或 `SetActorLocation`。本阶段只证明计划和碰撞预检，不把整段位移错误地实现为单帧传送。

### 2.4 通用位移 authority 解耦

`ClampPreflightDistance` 与 `PreflightWorldStatic` 新增显式 `WorldStaticClearance` 参数，并对非有限值和负 clearance 失败关闭。通用实现不再 include 或读取 `demo_mapEnemySkillTypes`。

两个 enemy runtime 调用、三个 V3 产品自动化调用及原 enemy framework 测试均显式传入 `Fdemo_mapEnemySkillPrototypeConfig::Get().WorldStaticSkin`。这是旧 P6 policy 留在旧调用方，而非通用 authority 反向读取业务配置。

## 3. 完整性与兼容性

- 复用 P10.1 immutable request，不复制输入或 action-window 生命周期；
- 复用已有 capsule sweep，不增加第二套碰撞算法；
- 旧 enemy skill 与 V3 调用仍使用原 2 cm skin，计算结果不变；
- V2 位移、EnemySkillFramework、Profile 与 CodeB 均按实际改动路径回归；
- 不访问或创建 SpiritEnergy float 账本；
- 不增加 duration、trajectory、airborne、invulnerability 或 recovery policy；
- 不修改 formation、controlled/thrown weapon、item authority 或 0.0.9B 存档结构；
- 没有 RNG、Timer、Tick 或产品移动 mutation。

## 4. 关键不变量

1. policy 数值必须由产品调用方显式注入；
2. request policy id 与 snapshot policy id 必须相同；
3. plan identity 同时绑定 request 与 exact policy bits；
4. invalid plan 不得触碰 World；
5. preflight 只读 WorldStatic，不执行移动；
6. resolved distance 必须达到 minimum 且不得超过 requested；
7. clearance 无效时失败关闭；
8. 通用 displacement authority 不依赖 enemy skill 配置；
9. 旧 P6 调用继续由旧 policy 决定 clearance；
10. SpiritEnergy 与实际时间轨迹仍属于后续阶段。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionMovementAdapter` 五个测试：

- `PolicyCapture`：有效 capture 及空 id、零 minimum、minimum 超限、负 clearance；
- `RequestPlanBinding`：匹配成功，policy 替换与 invalid request 失败；
- `DeterministicReplay`：等价重放、距离和 action 身份分离；
- `ExplicitClearance`：blocked/unblocked/clamp-to-zero/invalid clearance；
- `PreflightBoundary`：minimum 边界、超限、invalid plan 与 unavailable Character。

0.0.10 全量由 P10.1 的 `390` 增至 `395`。路径门禁要求并实际验证七组回归，而非只运行本轮主题测试。

## 6. 修改范围

生产代码：通用 displacement 的 h/cpp、enemy runtime、V3 progression manager，以及新增产品 adapter h/cpp。

测试与门禁：enemy framework tests、新 adapter tests、regression map 与 self-test。

实现为 `10` 个文件、`687` insertions、`22` deletions；加入本 Report 与同名 Log 后 exact stage 为 `12` 个文件。长期未跟踪的 0.0.9B Prompt/Report 和用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.2-SpiritEvasionMovementAdapter-final.log` | Product adapter | 5 | 0 | `FD821061D3F29427E01D5A36F0BF177AC840D9478FE485412E05366BE7EC3031` |
| `P10.2-SpiritEvasionMovement-final.log` | P10.1 movement | 5 | 0 | `C24A2B439B3305E37C1590C5995B7260D91C28DC5976315D99A65B2779DF8E78` |
| `P10.2-EnemySkillFramework-final.log` | Enemy skills | 44 | 0 | `8313BF4788A1F3D59C9B4E2651E2CEEAF63D4CBA0C58E45DC98CAB24833F238D` |
| `P10.2-V2RangedCompatibility-final.log` | V2 displacement | 22 | 0 | `CD1B6026D197B4DCAE2C75293E984AF77BBDC7AEE5160F812BE60358131A1144` |
| `P10.2-Profile-final.log` | Profile | 211 | 0 | `B702BE5CCD604C9837E2CAB1606BAE0E4E9F19D640D2DF6FBDBE8CA52E170FAA` |
| `P10.2-CodeB-final.log` | CodeB | 60 | 0 | `AF0E9E9B0746631E75A820680A840EEA8977F41CC563248A86217AB069835426` |
| `P10.2-Shanmen-0_0_10-final.log` | 0.0.10 full | 395 | 0 | `C97CC378907418088AACF03FE0D322694F67467BB82847311661D17A649A4A8D` |

```text
REGRESSION_MAP_JSON: PASS Rules=79
SELF_TEST: PASS 118/118
REGRESSION_COVERAGE: PASS Changed=10 Rules=4 Required=7 Logs=7
git diff --check: PASS
ADAPTER_FORBIDDEN_HITS: 0
GENERIC_ENEMY_SKILL_HITS: 0
```

mapping SHA-256：`F83DF6E2795EEADC428E5352D472A4486D5EC9B5542CD47E35E29B56A820A525`；self-test SHA-256：`C13BF776C75D227F4B275507098EFA0D41A9F5721C55AA8F92AE5770E37BD18F`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 13 / 62.73s | 0 | `0C318B93288231B877E83D571D89496B0EC6618DBB81BBE1E44AE0BD2FC26D50` |
| Editor final | Succeeded, up to date | 0 / 0.91s | 0 | `EFF1635A5B84337B3791AC92D25EA7C48EDE9DE772EDCDDFA2D133DB92BE1D54` |
| Game final | Succeeded | 10 / 46.41s | 0 | `21F47A42E2A49BC1E9A3C7B46CC7840140DF5F6F42E7DBFF3765A85D7F523068` |

`UnrealEditor-demo_map.dll`：`12172288` bytes / SHA-256 `6538BD1C4814B92E37353AF930D5A15EBD89001556BAC596280AACBB7A4D68F0`；`demo_map.exe`：`353655808` bytes / SHA-256 `58ADA70B0E4126EA1B25C3A1603DDB0349B94B567984D1FB4CFD7F55C64DD901`。

## 9. 真实异常

首次 self-test 被错误地交给 Windows PowerShell 5.1，旧解释器不能解析仓库脚本中的 PowerShell 7 管线续行；改用 `pwsh` 后原脚本 `118/118` 通过。这是调用解释器错误，不是脚本、源码或测试失败。

changed-file gate 首次正确拒绝了未映射的 `demo_mapEnemySkillRuntimeComponent.cpp`。已把该真实调用方加入 `SkillAndEnemyCombat` 路径规则，并扩展 self-test；最终 gate 以 `Changed=10 / Rules=4 / Required=7 / Logs=7` 通过。

没有 C3859、C1076、系统代码 1455、UBT 非零退出、Automation failure 或外层超时。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P10.3 可在 plan/preflight 之上设计持续时间与逐帧轨迹执行 authority，并以 receipt 记录真实位移；在接入 SpiritEnergy 前，仍需先确定真实 balance owner、revision、恢复和持久化契约。
