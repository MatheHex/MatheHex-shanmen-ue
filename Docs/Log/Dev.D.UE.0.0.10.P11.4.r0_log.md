# Dev.D.UE.0.0.10.P11.4.r0 Development Log

## 目标

建立 P11.0–P11.3 与既有 `FShanmenDefenseSnapshot` 之间唯一的产品组合边界：正面威胁只追加精确 selected guard layer，背面威胁原样保留基础快照；不复制窗口、时序、夹角、World identity、Impact、资源或耐久权威。

## 基线

- branch：`agent/0.0.10-p11-4-weapon-guard-defense-coordinator`；
- base：`0a0f9224f5fa65967e3ce208b684b8546c323675`；
- P11.0：Active-only ordinary guard window/layer；
- P11.1：同一窗口上的 Perfect/Ordinary 互斥 timing projection；
- P11.2：显式 guard-facing 与 defender-to-threat arc evaluation；
- P11.3：live Actor、Run registry、稳定身份与 transform 的无状态 World adapter；
- base defense authority：现有 `demo_mapPlayerHealth` capture 与 resource adapter；
- Impact authority：现有 `FShanmenImpactResolver`；
- P 阶段，不启动产品。

## 审计结论

1. incoming enemy Impact 路径先从 `demo_mapPlayerHealth` 捕获基础 defense snapshot；
2. 资源 adapter 可在随后把 resource-backed layer 规范化；
3. `FShanmenImpactRequest` 在上述步骤完成后才构造并交给纯 resolver；
4. P11.3 尚未进入该路径，因此 P11.4 应先冻结组合契约，不同时修改大型 Run coordinator；
5. P11.0 是窗口生命周期唯一权威；
6. P11.1 是 Perfect/Ordinary 时序互斥唯一权威；
7. P11.2 是 arc 数学与 selected layer 唯一权威；
8. P11.3 是 Actor/World/entity/transform 证据唯一权威；
9. P11.4 只能按值组合 snapshot，不能求伤害、写生命值或控制资源；
10. append-only 必须保留基础 layer 数组顺序与 tags；
11. layer ID 冲突必须在进入 resolver 前 fail closed；
12. receipt 必须同时绑定 timing、World、base snapshot 与 final snapshot。

## 实现

新增：

- `Edemo_mapShanmenWeaponGuardDefenseStatus`：8 个精确阶段状态；
- `Fdemo_mapShanmenWeaponGuardDefenseResult`：UObject-free 组合结果；
- `Fdemo_mapShanmenWeaponGuardDefenseCoordinator::Compose`：唯一同步入口；
- snapshot digest namespace：`demo_map.Sword.WeaponGuard.DefenseSnapshotEvidence.r1`；
- composition receipt namespace：`demo_map.Sword.WeaponGuard.DefenseCoordinator.r1`。

执行顺序：

1. 验证 window/runtime/policies/observation/candidate；
2. 验证基础 defense snapshot；
3. 调用 P11.1 timing evaluator；
4. 调用 P11.3 World adapter；
5. `Qualified` 时拒绝重复 layer ID，然后追加 exact selected layer；
6. `OutsideArc` 时拒绝任何意外 layer 并保持 snapshot 不变；
7. 验证最终 snapshot；
8. 对所有 target/layer tags 做 lexical canonicalization；
9. 对 layer order、operation、flags、IDs 与 float bits 做精确编码；
10. 生成 base/final snapshot digests 与 composition receipt；
11. 通过 `IsSuccess` 复核 append/unchanged invariants；
12. 返回只含值与 immutable evidence 的结果。

## 测试

新增 5 个 focused tests：

- `PerfectImpact`
- `OrdinaryAndOutside`
- `Conflict`
- `AuthorityFailure`
- `ReplayLifecycle`

最终证据：

| Log | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `P11.4-WeaponGuardDefenseCoordinator-final.log` | 5 | 0 | 1 | `F68DE604FA1D140126E90A1F2B25DE98FD7367C38EDB5E08E61A95D422CBB1F3` |
| `P11.4-WeaponGuardWorldAdapter-final.log` | 5 | 0 | 1 | `A63260C6A1445030CDB28FEDABFE19014E50D01189601F531FA938BC49484C24` |
| `P11.4-WeaponGuardArc-final.log` | 6 | 0 | 1 | `B70CD85F664B58AE2D802EEDE9AB602AEDB7738370B5AD2ED1516CA297060960` |
| `P11.4-WeaponPerfectGuard-final.log` | 6 | 0 | 1 | `2CAE4178621128D044A35D41C81781E6C68D451C9F748820DC3026DBD737F4C6` |
| `P11.4-WeaponGuard-final.log` | 12 | 0 | 1 | `310B4A79054A5590B916AA3A17C967BF3D98A3BB72375EC43B3CC166D27DDAFC` |
| `P11.4-ActionLifecycle-final.log` | 1 | 0 | 1 | `26839636CA02D0D668CB796BCF723FB4F7053DD3A9BBFB174373E491E940BEF7` |
| `P11.4-CombatCore-final.log` | 9 | 0 | 1 | `A98D03AC6FF9316FD756553D7309380F6D36AFC63596ABDAD2589E7657B75EA0` |
| `P11.4-WorldGameplay-final.log` | 10 | 0 | 1 | `DE916835552894E463E281FBDD957789FBBF0740149BD01F848805C4E73DE6A4` |
| `P11.4-Shanmen-0_0_10-final.log` | 483 | 0 | 1 | `E5FC481C9CC13C24F5654D8BD6A3F24BAADDEDE6F2FC9F5956650D897BF56EA2` |

总计 537 Success / 0 Fail；包含 focused 与全量的重复覆盖。

## 回归映射

新增 `WeaponGuardDefenseCoordinator` rule，要求：

- `Shanmen.0_0_10`
- `Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator`
- `Shanmen.0_0_10.Product.WeaponGuardWorldAdapter`
- `Shanmen.0_0_10.CombatRuntime.WeaponGuardArc`
- `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard`
- `Shanmen.0_0_10.CombatRuntime.WeaponGuard`
- `Shanmen.0_0_10.CombatRuntime.ActionLifecycle`
- `Shanmen.0_0_10.CombatCore`
- `Shanmen.0_0_10.WorldGameplay`

结果：

```text
REGRESSION_MAP_JSON: PASS Rules=93
SELF_TEST: PASS 146/146
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=9 Logs=9
git diff --check: PASS
BOUNDARY_SCAN: PASS production cpp hits=0
```

## 构建

| Build | Result | Actions / Time | Exit | SHA-256 |
|---|---|---|---:|---|
| Editor first | Failed | 5 scheduled / 21.00s | 6 | `0E2057BA7AF1A266DEBE31A5445BB2F6C16A50B5AB6CC40204271454DAE114AF` |
| Editor final | Succeeded | 4 / 6.30s | 0 | `EA581B998231854F7C929669446096709BAED4ED4CC912030627D659C6B33590` |
| Game final | Succeeded | 4 / 23.62s | 0 | `A55E3ED1AC8CAC236F174169455E15059447676DDABA80671EF3339E7025D424` |

## 真实异常

1. mapping self-test 首次由 Windows PowerShell 5.1 启动，既有 PowerShell 7 行首管道语法解析失败，退出 1。改用 bundled `pwsh` 后 146/146。
2. Editor 首次构建：测试夹具使用不存在的 `SourceEnemy()`，退出 6；删除无必要 tag 后成功。没有把该失败描述为环境或内存错误。
3. focused 首次运行：`TArray` 自引用追加触发容器别名断言，原生退出 3，失败日志 SHA-256 `6E4313971E3D93C0049C4370403363737086A3516937DFFD8CD8E50F6B34262A`。改为值拷贝后 5/5。
4. 没有产品协调逻辑断言失败、Windows commit memory/页面文件错误或 Win64 SDK 错误。

## 修改统计

```text
5 files changed, 1109 insertions(+)
```

加入 Report/Log 后 exact stage 为 7 个文件。长期未跟踪资料不进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-4-weapon-guard-defense-coordinator>
