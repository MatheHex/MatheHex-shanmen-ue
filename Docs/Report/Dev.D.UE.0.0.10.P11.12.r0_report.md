# Dev.D.UE.0.0.10.P11.12.r0 Report

## 1. 结论

P11.12 已完成并通过 P 阶段门禁。

本阶段把 P11.10 的唯一活动 WeaponGuard Session 接入既有 M01 敌对 Impact 权威链。六类真实敌对 contact 都在资源防御准备之后、生命快照与纯函数 resolver 之前，借用一次 Session/Host 事务合成 defense；GameMode 与输入层不产生伤害结果，也不直接修改生命。

最终结果：

- Session 新增一次 Impact 防御合成事务，拒绝不改变 active Host，成功只提交一次单调时间观察；
- GameMode 只捕获当前 fixed-timeline sample，并把临时 context 交给 CombatRunCoordinator；
- BasicMelee、MeleeDash、RangedProjectile、HeavySector、BossShape 与 BossVolley 六类敌对攻击全部复用同一私有 Impact 路径；
- 正面 perfect window 产生 canonical perfect-guard receipt，最终伤害为 0；
- 普通窗口追加 25% guard layer，4 点原始伤害得到 1 点减免、3 点最终伤害；
- 背后 contact 仍被 Host 审计并推进单调 tick，但不追加 guard layer；
- timeline mismatch 与 non-monotonic observation 均在 vitality delivery 前 fail closed；
- 新增 3 个真实 transient-world focused tests；
- 0.0.10 全量达到 531/531；
- 7 个 legacy 回归组达到 146/146；
- 8 份健康 Automation 日志记录 680 次 Success、0 次 Fail；按 test identity 去重为 677 项；
- regression mapping 达到 101 rules，自检 162/162；
- changed-file gate：`PASS Changed=9 Rules=4 Required=42 Logs=7`；
- `git diff --check`、JSON 解析与新增行边界扫描通过；
- Editor Development 与 Game Development 单并发构建均成功，原生退出码均为 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 Session 事务

`Fdemo_mapShanmenWeaponGuardProductSession::TryComposeImpactDefense` 使用 copy-validate-commit：

```text
validate Session + active Host + exact timeline sample
  -> copy active ProductRoute
  -> copied Host.TryComposeDefense(...)
  -> validate Host receipt and advanced copied route
  -> commit copied route exactly once
  -> post-commit invariant check, otherwise restore previous route
```

失败结果区分 invalid Session、无 active Host、非法 timeline sample、timeline mismatch、Host rejection 与 state desynchronization。失败前后的 HostId、last observed tick 与 Session ownership 保持不变。

成功 receipt 同时保留 HostId、source item instance、timeline、observed tick 与 Host defense composition。`ComposedQualified` 必须携带 guard layer；`ComposedOutsideArc` 必须不携带 guard layer。

### 2.2 唯一敌对 Impact 路径

CombatRunCoordinator 新增临时 `Fdemo_mapM01EnemyAttackWeaponGuardContext`。它只借用 Session 指针和一个 opaque timeline sample，不拥有 clock、Host、Actor 或 World 生命周期。

执行顺序固定为：

```text
build action/candidate/impact identity
  -> capture base defense
  -> prepare item/resource defense
  -> compose active WeaponGuard defense
  -> capture post-preparation vitality snapshot
  -> canonical FShanmenDefenseResolver::Resolve
  -> existing idempotent vitality delivery
```

WeaponGuard 合成失败时，Coordinator 取消已准备的 resource reservation、interrupt 当前 action，并在生命提交前返回 `WeaponGuardDefensePreparationFailed`。如果 resource cancellation 自身失败，则保留更严重的 `ResourceDefensePreparationFailed`。

### 2.3 GameMode 边界

GameMode 的新增职责仅为捕获 fixed timeline：

- 空 Session：不启用 context，保持旧调用语义；
- 非空有效 Session：传递当前 timeline ID 与整数 tick；
- 非空但无有效 timeline：传递 enabled-invalid context，使 Coordinator fail closed；
- GameMode 不调用 resolver、不创建 defense outcome、不执行 vitality commit。

六个公开 M01 wrapper 都传入同一 context，并继续委托 Coordinator 的一个私有 `ExecuteM01EnemyAttack`。

## 3. 完整性

新增结果证明包含：

- 是否实际检查了 WeaponGuard；
- Session defense status/error；
- Host/item/timeline/tick identity；
- Host world observation 与 composition；
- 最终 canonical Impact request/result；
- 既有 idempotent vitality delivery receipt。

`Fdemo_mapM01EnemyAttackExecutionResult::IsExecuted` 在 guard 被检查时额外要求 Session defense receipt 成功；未启用 context 的既有 caller 继续保持兼容。

测试覆盖：

1. `PerfectFront`：正面、perfect window、0 最终伤害、零伤害 receipt 仍 canonical commit；
2. `OrdinaryAndOutsideArc`：普通窗口 25% 减免，随后背后 contact 不追加 layer 且 Host 连续推进；
3. `FailureAtomic`：错误 timeline 与倒退 tick 均不扣血、不提交 Impact、不改变 Session，合法重试仍可成功。

## 4. 兼容性与权威边界

- CombatRunCoordinator 继续拥有 Run、activation、candidate、Impact 与 vitality delivery；
- ItemAuthority/resource adapter 继续拥有装备与资源防御准备；
- Product Session 继续是唯一 active WeaponGuard Host 所有者；
- Product Host/DefenseCoordinator 继续拥有 timing、arc 与 guard layer composition；
- fixed timeline 继续由 Combat Run/GameMode 持有；
- GameMode 与 PlayerController 不成为第二套 damage authority；
- 所有新增公开参数均为默认 `nullptr`，不启用 guard 时旧调用行为不变；
- 新增代码未调用 `ApplyDamage`、`TakeDamage`、RNG、物理输入键查询或 frame/wall clock；
- GameMode 仍有 16 处历史 `ApplyDamage` 文本，均为本阶段前既有行；新增行命中为 0；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与其它资料未修改、未暂存、未提交。

## 5. 修改范围

实现与门禁共 9 个文件、897 行新增、29 行删除（不含本 Report/Log 与原生日志）：

- `Source/demo_map/demo_mapShanmenWeaponGuardProductSession.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductSession.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinator.h`
- `Source/demo_map/demo_mapCombatRunCoordinator.cpp`
- `Source/demo_map/demo_mapGameMode.h`
- `Source/demo_map/demo_mapGameMode.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardImpactRouteTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

## 6. 测试覆盖

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardImpactRoute` | 3 | 0 | 1 | `1FF1A5BAA22BA727C13B7CF1FA4581ED8F714F7B0348F8D107B76A8F83C9D48B` |
| `Shanmen.0_0_10` | 531 | 0 | 1 | `53B944D918E1620FEC600874942414BF27EC93B39B019483B74F9D5B6A0BA014` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 1 | `B5793AADC8E9671FBAB2C281EE6E25D8FEF5F8AA0CE8382B03D90572F6E18580` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `2F9367ECD02072C56C9B22689DBE67B61FF0E938BE3F2AB6A4F2BB5DB95FA28C` |
| `demo_map.V3.Attributes` | 4 | 0 | 1 | `01A2317A1ED9C1B4D57DE39A071C4DD5DE1969392F60207C9B20016480A78614` |
| `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `0DCA820D378FDA4C558690AA54193967E1632938F535765A5EB00623B9572994` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `E4A6B3F101CF23B96EF40C7D9A43F41A1F5DE08FADD1241BDC581A990DDED920` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | `4841BD7F0353F98AB4AE1B789AC0EF029CDA5FF59DE84E778E759A057661114C` |

focused 3 项包含于 full 531 项。日志原始 Success 合计 680，按 test identity 去重为 677。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=101
SELF_TEST: PASS 162/162
REGRESSION_COVERAGE: PASS Changed=9 Rules=4 Required=42 Logs=7
git diff --check: PASS (native exit 0)
ADDED_FORBIDDEN_SCAN: PASS (401 added production lines, 0 hits)
GAMEMODE_DAMAGE_AUTHORITY_SCAN: PASS (0 resolver/vitality-commit hits)
```

- mapping SHA-256：`14B0C3FBFFCCC1363A3593E9FD653728CB1D9CD57387EC26745F4DBEB9FFCF10`；
- self-test SHA-256：`7901766722BF5E2D892F16BDF5F2E40A061E6B6DE170AF439CA516EE6ED84CB3`；
- final self-test log SHA-256：`EF56EFB774B09384DE0CD412781B185611AAA075E3825AB4F75E9BA71F74FE65`；
- coverage evidence SHA-256：`B9D6AEA13B6E956A87F9AE39145BBC02FF08379DC0EAC7F8DA3821C5A22051CD`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / time | Exit |
|---|---|---|---:|
| Editor Development（实现构建） | Succeeded | 76 actions | 0 |
| Editor Development（rollback fence 增量） | Succeeded | 4 actions | 0 |
| Editor Development（最终） | Succeeded | up to date / 0.93s | 0 |
| Game Development（完整） | Succeeded | 75 actions / 214.77s | 0 |
| Game Development（最终） | Succeeded | up to date / 0.91s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：12,871,168 bytes，SHA-256 `074966B4089C15D81A6990C890912A03634FED41E232B5BF93E8DA51EE0EE272`；
- `demo_map.exe`：354,382,848 bytes，SHA-256 `13FD2230B18488DDD0ACC62A5D1F3D87A4128F244FFC4191251788D55E30EBB9`；
- build evidence SHA-256：`E4940E1AF8C443FD603BB68D3983CFE5F085E89234C6307A5C834C8B69BE53E7`。

## 9. 真实异常

1. regression self-test 首轮原生退出 1：既有 CombatRunCoordinator positive fixture 未包含本轮新增 rule 所要求的 legacy enemy/item evidence；补齐 fixture 后最终 162/162；
2. changed-file gate 首次通过 child `pwsh -File` 调用时数组被展开成位置参数，原生退出 1，未形成覆盖结论；改为同一 PowerShell host 传递 typed arrays 后 gate 通过；
3. Automation 启动阶段仍打印 UE 自带 UnifiedError 测试的 3 条示例 Error 与 13 条 `Condition failed`，均发生在本项目测试发现前；全部目标仍为 Success、0 Fail、terminal exit 0；
4. 本阶段没有产品源码编译失败、Automation 失败、Fatal、Unhandled Exception、Ensure 或内存环境错误。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段实现、代码审查、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或 F 阶段产品回归。

下一阶段建议审计剩余历史 hostile-damage 入口，把 Shipping 路径与 automation/demo-only `ApplyDamage` 明确分类；只迁移仍可到达的产品路径，并继续复用本阶段唯一 Session -> canonical Impact 事务，不扩建第二套伤害系统。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-12-weapon-guard-impact-route>
