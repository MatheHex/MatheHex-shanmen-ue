# Dev.D.UE.0.0.10.P20.44.r0 Report

## 1. 结论

P20.44 已建立 renderer surface recreation/adoption 的纯策略契约。策略把 Host-authoritative cursor 与重建后 observed physical surface cursor 做确定性比较，明确区分 fresh empty、exact visible、empty-needs-rehydrate、residual visible、foreign visible、conflicting visible 与 consumer mismatch。

策略只对四种显式请求作出决定：Inspect、BindFresh、AdoptExact、ClearToEmpty。只有 fresh empty 可以取得 BindFresh permit，只有 exact visible 可以取得 AdoptExact permit，三类 visible conflict 只有在调用方明确请求 ClearToEmpty 时才取得 cleanup permit。可见权威对应空 surface 时强制进入 EmptyNeedsRehydrate，本阶段不签发任何伪装成 bind/adopt/clear 的 permit。

本阶段仍是 P 阶段无头契约：Policy 不持有、不调用也不修改真实 surface、Owner、Host、Adapter、widget 或 World；所有结果由纯值测试验证。

## 2. 基线、分支与改动范围

- 基线：`fb66833964ea3549651dcebc89b484e1c18fd3b0`（P20.43）；
- 分支：`agent/0.0.10-p20-44-thrown-weapon-arc-preview-surface-recreation-policy`；
- 新增 `demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.h/.cpp`；
- 扩展 `demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`；
- 扩展 changed-file regression map 与其自测；
- 新增本 Report 与同名 Log；
- 未修改 P20.43 Owner、P20.42 Adapter、P20.41 Host 或其 public contract；
- 未执行 surface mutation、pointer rebind、renderer rehydrate 或产品 mutation。

## 3. Recreation disposition matrix

Policy 接收 exact Run、expected consumer、Host-authoritative cursor、observed consumer、observed physical cursor 与 requested action，并生成一种 disposition：

1. `FreshEmpty`：权威物理 cursor 与 observed surface 均为空；
2. `ExactVisible`：两者为同一 Run 的完全相同 visible presentation state；
3. `EmptyNeedsRehydrate`：权威为 visible，但重建 surface 为空；
4. `ResidualVisible`：权威已 empty/hidden，但同 Run surface 仍 visible；
5. `ForeignVisible`：observed visible state 属于另一 Run；
6. `ConflictingVisible`：同 Run visible，但 state identity/content 与权威不一致；
7. `ConsumerMismatch`：surface consumer identity 与 expected scope 不同；
8. `InputRejected`：Run、consumer、authoritative cursor 或 physical cursor 输入不合法。

Host Hidden 是 audit tombstone，在 renderer 边界归一化为 Empty。observed physical surface 不允许报告 Hidden；这会直接成为 typed input rejection。

## 4. 显式 action authorization

授权矩阵固定为：

| Requested action | 唯一可授权 disposition | 行为含义 |
|---|---|---|
| BindFresh | FreshEmpty | 新 Owner/Adapter 可绑定一个空 surface |
| AdoptExact | ExactVisible | 可接管与权威完全一致的 visible surface |
| ClearToEmpty | ResidualVisible / ForeignVisible / ConflictingVisible | 仅授权后续显式清理当前 exact visible snapshot |
| Inspect | 所有可分类输入 | 只返回分类，不生成 permit |

任何 action/disposition 错配都返回有效的 typed rejection，不会降级为其它 action。consumer mismatch 允许 Inspect 观察，但永远不能签发 bind、adopt 或 cleanup permit。

## 5. Rehydrate fence

当 Host 权威为 visible、observed surface 为空时，renderer 必须重新建立可见内容。现有 delivery ledger 已经记录该 state 为 applied，不能把重建操作冒充新的普通 Show，也不能把空 surface 当成 exact adoption。

因此 `EmptyNeedsRehydrate` 在本阶段只提供状态事实：

- BindFresh：拒绝；
- AdoptExact：拒绝；
- ClearToEmpty：拒绝；
- Inspect：成功分类但无 permit。

后续必须由独立的 attested restore executor 定义一次性 rehydrate 与 receipt，再交给 Owner 做安全接管。

## 6. Deterministic exact permit

Authorized result 携带 immutable permit，绑定：

- deterministic PermitId；
- requested action 与 classified disposition；
- expected Run 与 consumer definition；
- normalized expected physical cursor；
- policy 评估时的 exact observed surface cursor。

`MatchesSnapshot` 要求 future executor 在执行前重新核对 Run、consumer、expected cursor 与当前 observed cursor。surface 在评估后发生任何变化，旧 permit 立即不再匹配，不能被复用到新状态。

Permit 不是 mutation receipt，也不证明 renderer 已执行任何动作；它只证明这一组冻结输入允许某一个精确后续操作。

## 7. Deterministic decision 与 fail-closed

每个 result 都有由 outcome、action、disposition、Run、双 consumer、authoritative cursor、normalized expected cursor 与 observed cursor共同派生的 DecisionId。同一请求稳定重放得到同一 DecisionId；更换 action 或任一状态快照都会改变 identity。

Result 保存完整输入、分类、outcome、diagnostic 与可选 permit，并在构造时自验证后缓存。invalid Run、缺失 consumer、wrong-Run authoritative state、Hidden physical cursor 与未知 action 都形成可审计 rejection；Policy 不抛弃输入，也不制造默认授权。

Policy 不引用 surface 接口实例，因此不存在 callback、重入、外部副作用或 rollback 路径。

## 8. 自动化覆盖

新增 6 项 focused automation：

1. `ClassificationMatrix`：覆盖所有正常分类与 Hidden-to-Empty normalization；
2. `BindingPermits`：fresh bind、exact adoption、deterministic replay 与 snapshot drift fence；
3. `ExplicitCleanupPermits`：residual、foreign、conflicting 三类显式 cleanup permit；
4. `RehydrateFence`：空新 surface 不能伪装成 bind/adopt/clear；
5. `InvalidInputs`：invalid Run/consumer/authority/physical cursor 与 consumer mismatch；
6. `ActionFence`：每个 action 只能映射到规定 disposition，Inspect/rejection 不泄漏 permit。

完整 `Shanmen.0_0_10` 从 P20.43 的 1089 增至 1095 项。

## 9. Changed-file 回归与验证证据

新增 `ThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy` exact-path mapping rule。5 个改动路径命中 2 条规则，求并集要求 29 个测试组；3 份正式日志全部满足映射。

- mapping self-test：381/381；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=29 / Logs=3；
- boundary scan：PASS，Policy 2 files / 658 lines；World/Actor/UObject/RNG/ApplyDamage 与 direct Show/Replace/Hide surface call 0；
- `git diff --check`：PASS，0 whitespace errors。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| surface_recreation_policy_final.log | Product.ThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy | 6/0 | 0064F12FB683147043691D730C458F0AA5B8336A7C72C056666B55272312018A |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 6F7B15123F4F13912BC362BF54735E93462C9D81941ABC92522CC41D347F1D4E |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1095/0 | 0422CDD841C5A4DE03A32FA1A8BFFB835EA4499B1C149309DB244916E5122AE8 |

正式日志累计 1147/0，包含 focused、legacy 与完整套件重叠；独立完整套件为 1095/0。完整套件在既有 SwordRhythm journal/checkpoint/manifest codec 慢区段出现非惩罚性 unresponsive 通知，进程持续使用 CPU、恢复逐项进展，最后由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束；没有拆组、外层 timeout 或人工中止替代正式结果。

构建：

- candidate Editor：5 actions / native 0 / 25.71 秒；
- final Editor：up to date / 0 actions / native 0 / 1.19 秒；
- final Game：4 actions / native 0 / 33.38 秒；
- Editor DLL：17219072 bytes，SHA-256 `49DB418DB1C7E6E715B78CEF8E4DB97D83FCF72EB6FC5ACC8A1509D5399B9062`；
- Game EXE：358337024 bytes，SHA-256 `B88465A336E1184AB49DFD2E2E016EE392B157BC786C5D1B9F07DDD442A0DFE5`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：authoritative/physical cursor classification、Hidden normalization、fresh bind policy、exact visible adoption policy、explicit cleanup policy、foreign/conflicting surface separation、rehydrate fence、consumer fence、deterministic decision/permit、snapshot drift fence 与 changed-file regression。

未验证：permit 的真实执行、surface cleanup/rehydrate mutation、Owner/Adapter pointer replacement、旧 surface retirement、renderer attestation 来源真实性、真实 MainHUD/widget/component/visible frame、外部副作用补偿、跨线程同步、进程崩溃持久性、真实输入、World、Editor UI/PIE/Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.45 建立 narrow attested surface lifecycle executor：消费 P20.44 exact permit，BindFresh/AdoptExact 只签发执行 receipt，ClearToEmpty 最多调用一次独立 lifecycle surface capability 并核对 before/after cursor；仍不修改 Owner pointer，rehydrate 继续单独设计。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-44-thrown-weapon-arc-preview-surface-recreation-policy>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-44-thrown-weapon-arc-preview-surface-recreation-policy/Docs/Report/Dev.D.UE.0.0.10.P20.44.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-44-thrown-weapon-arc-preview-surface-recreation-policy/Docs/Log/Dev.D.UE.0.0.10.P20.44.r0_log.md>
