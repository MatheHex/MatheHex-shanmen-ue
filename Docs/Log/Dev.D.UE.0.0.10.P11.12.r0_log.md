# Dev.D.UE.0.0.10.P11.12.r0 Development Log

## 目标

把唯一 active WeaponGuard Product Session 接入真实 M01 敌对 Impact 路径：在既有资源防御之后、生命快照与 resolver 之前合成 guard layer，并保持 GameMode/input 无伤害权威。

## 基线

- branch：`agent/0.0.10-p11-12-weapon-guard-impact-route`；
- base：`82b0f665207f8872c2ef6ff7b0ea334596b3ab64`；
- P11.11：实际右键 press/release 与 30 Hz Run-owned timeline；
- P11.10：唯一 active Session 与 ordered release/interrupt；
- P11.6：WeaponGuard Product Host 与 canonical defense composition；
- 既有 CombatRunCoordinator：M01 六类 hostile contact、resource defense、resolver 与 idempotent vitality delivery；
- P 阶段，不启动产品。

## 审计结论

1. M01 的六类敌对攻击最终都进入一个私有 `ExecuteM01EnemyAttack`；
2. 该路径已经拥有 action/candidate/impact identity、resource preparation、vitality snapshot、pure resolver 与 delivery；
3. GameMode 只是 wrapper，不能成为第二个 resolver 或 vitality writer；
4. P11.11 fixed timeline 已与 Run 同生共灭，可在 contact 时提供唯一 observed tick；
5. P11.10 Session 是唯一 active Host owner，必须由它执行 copy-validate-commit；
6. guard composition 必须在 resource defense 之后，才能保留完整已有 defense stack；
7. vitality snapshot 必须在所有 preparation 后捕获，避免资源恢复造成 stale CAS；
8. context 只借用一次 Session，不保存 clock/World/Actor 生命周期；
9. 非空但失配的 Session/timeline 必须 fail closed，不能静默绕过 guard；
10. 没有 active Session 时必须保持旧 caller 兼容。

## 实现

### Session

新增：

- `Edemo_mapShanmenWeaponGuardSessionDefenseStatus`；
- `Edemo_mapShanmenWeaponGuardSessionDefenseError`；
- `Fdemo_mapShanmenWeaponGuardSessionDefenseResult`；
- `TryComposeImpactDefense`。

Session 复制 active ProductRoute，在副本 Host 上完成 world observation 与 defense composition；完整 receipt 与副本状态有效后才提交。提交后另有 invariant fence，失败则恢复 previous route。

### CombatRunCoordinator

新增 ephemeral `Fdemo_mapM01EnemyAttackWeaponGuardContext`，并给六个敌对攻击入口增加默认 `nullptr` 参数。唯一私有执行路径按下列顺序处理：

```text
base defense
resource defense preparation
WeaponGuard Session transaction
vitality snapshot
canonical resolver
idempotent delivery
```

合成失败会取消已准备资源并 interrupt action；不进入生命提交。

### GameMode

新增 `CaptureM01EnemyAttackWeaponGuardContext`：

- empty Session -> disabled context；
- active Session + valid timeline -> current sample；
- non-empty invalid state -> enabled-invalid context，交给 Coordinator 拒绝。

GameMode 六个 wrapper 只捕获并传递 context。

## 测试

新增真实 transient `GamePreview` World fixture，使用真实 Player pawn、health component、StandardSkirmisher、entity registry、TrainingBlade 与 active Session。

新增 3 项：

- `PerfectFront`；
- `OrdinaryAndOutsideArc`；
- `FailureAtomic`。

最终健康证据：

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `automation_weapon_guard_impact_route_first.log` | `Shanmen.0_0_10.Product.WeaponGuardImpactRoute` | 3 | 0 | `1FF1A5BAA22BA727C13B7CF1FA4581ED8F714F7B0348F8D107B76A8F83C9D48B` |
| `automation_shanmen_full_final.log` | `Shanmen.0_0_10` | 531 | 0 | `53B944D918E1620FEC600874942414BF27EC93B39B019483B74F9D5B6A0BA014` |
| `automation_enemy_skill_final.log` | `demo_map.EnemySkillFramework` | 44 | 0 | `B5793AADC8E9671FBAB2C281EE6E25D8FEF5F8AA0CE8382B03D90572F6E18580` |
| `automation_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | 22 | 0 | `2F9367ECD02072C56C9B22689DBE67B61FF0E938BE3F2AB6A4F2BB5DB95FA28C` |
| `automation_v3_attributes_final.log` | `demo_map.V3.Attributes` | 4 | 0 | `01A2317A1ED9C1B4D57DE39A071C4DD5DE1969392F60207C9B20016480A78614` |
| `automation_item_economy_schema_final.log` | `demo_map.ItemEconomySchema` | 23 | 0 | `0DCA820D378FDA4C558690AA54193967E1632938F535765A5EB00623B9572994` |
| `automation_item_use_armor_final.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | `E4A6B3F101CF23B96EF40C7D9A43F41A1F5DE08FADD1241BDC581A990DDED920` |
| `automation_p4_hotbar_final.log` | `demo_map.P4.Hotbar` | 7 | 0 | `4841BD7F0353F98AB4AE1B789AC0EF029CDA5FF59DE84E778E759A057661114C` |

原始合计 680 Success / 0 Fail；去重后 677 项。

## 回归映射

新增 `WeaponGuardImpactRoute` rule，并扩充 Session、CombatRunCoordinator 与 GameMode 的依赖证据。

```text
REGRESSION_MAP_JSON: PASS Rules=101
SELF_TEST: PASS 162/162
REGRESSION_COVERAGE: PASS Changed=9 Rules=4 Required=42 Logs=7
git diff --check: PASS (native exit 0)
ADDED_FORBIDDEN_SCAN: PASS
GAMEMODE_DAMAGE_AUTHORITY_SCAN: PASS
```

- mapping：`14B0C3FBFFCCC1363A3593E9FD653728CB1D9CD57387EC26745F4DBEB9FFCF10`；
- self-test script：`7901766722BF5E2D892F16BDF5F2E40A061E6B6DE170AF439CA516EE6ED84CB3`；
- final self-test log：`EF56EFB774B09384DE0CD412781B185611AAA075E3825AB4F75E9BA71F74FE65`；
- final coverage log：`B9D6AEA13B6E956A87F9AE39145BBC02FF08379DC0EAC7F8DA3821C5A22051CD`。

## 构建

| Build | Result | Actions / time | Exit |
|---|---|---|---:|
| Editor implementation | Succeeded | 76 actions | 0 |
| Editor rollback-fence incremental | Succeeded | 4 actions | 0 |
| Editor final | Succeeded | up to date / 0.93s | 0 |
| Game compile | Succeeded | 75 actions / 214.77s | 0 |
| Game final | Succeeded | up to date / 0.91s | 0 |

build evidence SHA-256：`E4940E1AF8C443FD603BB68D3983CFE5F085E89234C6307A5C834C8B69BE53E7`。

## 真实异常

1. mapping self-test 首轮因既有 positive fixture 缺少新增 legacy evidence 而退出 1；补齐后 162/162；
2. coverage gate 首次 child-process 调用因 PowerShell array flattening 退出 1；同 host typed-array 调用后通过；
3. UE 内建 UnifiedError startup self-test 在项目测试发现前打印 3 条示例 Error 与 13 条 `Condition failed`；项目测试全部 Success；
4. 无源码编译失败、Automation 失败、Fatal、Unhandled、Ensure 或环境内存错误。

## 修改统计

实现与门禁（不含 Report/Log）：

```text
9 files changed, 897 insertions(+), 29 deletions(-)
```

长期未跟踪资料未进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-12-weapon-guard-impact-route>
