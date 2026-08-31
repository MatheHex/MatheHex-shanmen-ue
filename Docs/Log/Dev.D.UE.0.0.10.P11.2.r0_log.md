# Dev.D.UE.0.0.10.P11.2.r0 Development Log

## 目标

在 P11.0 普通武器格挡窗口与 P11.1 精准格挡时序之上建立独立方向／夹角资格策略：消费上层显式提供的格挡朝向与“防御者指向威胁”向量，只在正面资格达标时透传 P11.1 已选的唯一 layer；不从 HitNormal 猜方向，不查询 World/Actor/input，也不硬编码产品角度。

## 审计结论

1. P11.0 已拥有 canonical weapon-guard action、Active-only window 与普通 Guard layer；
2. P11.1 已在同一 window 上互斥选择 Perfect 或 Ordinary layer；
3. P11.2 不应重新计算 timing，也不能暴露第二个可消费 layer；
4. `FShanmenHitCandidate::HitNormal` 只记录显式接触法线，现有契约没有冻结符号方向；
5. 用 HitNormal 猜“攻击来自前方”会把 World adapter 语义泄漏进纯 Runtime；
6. 方向来源应由上层明确提供，Runtime 只做有限性、非零、规范化和点积；
7. 人工规划只冻结武器可用于格挡与精准格挡收益，没有冻结扇形角度或默认阈值；
8. 因此 policy 应接收内容提供的 `MinimumFacingDot`，而不是内置度数；
9. OutsideArc 需要可审计 receipt，但不能携带防御 layer；
10. Recovery/Interrupted 关闭继续由 P11.0 action lifecycle 唯一负责；
11. candidate target 必须是 guard action source entity，防止把另一实体的接触用于本次格挡；
12. 所有 policy/sample/evaluation 身份都应由冻结输入确定性派生。

## 设计决策

1. 新增 `FShanmenWeaponGuardArcPolicy`，绑定 exact P11.0 window、ArcRuleId 与 inclusive dot threshold；
2. 阈值只接受有限 `[-1,1]`；
3. 新增 `FShanmenWeaponGuardThreatSample`，绑定完整 hit candidate 与两个显式三维向量；
4. `DirectionToThreat` 的符号固定为 defender-to-threat；
5. 两个向量在 capture 边界规范为单位向量并消除负零；
6. Runtime 不读取 transform、不做 trace、不读取输入，也不投影到 XY；
7. evaluator stateless，只消费 P11.1 valid receipt；
8. exact window identity 同时校验 actual window、arc policy 与 timing policy；
9. candidate target 必须等于 guard action source；
10. `dot >= threshold` 为 Qualified，精确边界包含在内；
11. Qualified 原样复制 timing selected layer；
12. OutsideArc 返回 valid evaluation、status/dot/identity 与默认无效 layer；
13. public `HasLayer()` 只有 Qualified 且 layer 有效时才为 true；
14. PolicyId、SampleId 与 EvaluationId 使用 IEEE double bits 和 canonical GUID parts；
15. 不修改 CombatCore、P11.0/P11.1 或任何资源权威。

## 执行序列

1. 从 P11.1 commit `bbeb022408fa8a974e7fd08ba34bcb65b8523168` 创建 `agent/0.0.10-p11-2-weapon-guard-arc`。
2. 审查人工 0.0.10 战斗规划、P11.0/P11.1、HitCandidate 与既有方向规范化实现。
3. 冻结显式向量、三维点积、inclusive threshold 与 no-layer OutsideArc 边界。
4. 新增 immutable policy、threat sample、evaluation receipt 与 stateless evaluator。
5. 实现 full candidate/direction identity 与 exact window/timing binding。
6. 新增六个 focused Automation tests。
7. 新增 `WeaponGuardArc` changed-file rule 与正／反 self-test fixtures。
8. mapping JSON 更新为 91 rules，self-test 更新为 142/142。
9. `git diff --check` 与边界扫描通过。
10. Editor Development：6 actions，29.43 秒，exit 0。
11. 首轮七组测试全部 Success，但命令内 `;Quit` 导致 regression gate 把组名解析为 `<group>;Quit`，门禁失败。
12. 不修改 gate；改用 canonical RunTests 命令和 `-TestExit`，完整重跑七组。
13. 最终 WeaponGuardArc 6/6、WeaponPerfectGuard 6/6、WeaponGuard 12/12、ActionLifecycle 1/1、CombatCore 9/9、CombatRuntime 105/105、全量 473/473。
14. changed-file gate：Changed=5、Rules=2、Required=6、Logs=7，PASS。
15. boundary scan、mapping、自检与 `git diff --cached --check` 通过。
16. Game Development：5 actions，28.49 秒，exit 0。
17. 精确暂存 5 个实现／门禁文件；加入同名 Report/Log 后扩展为 exact 7-file stage。
18. 复核长期未跟踪资料未进入暂存区，commit 并 push。

## 状态与数据流

```text
P11.0 exact Active WeaponGuardWindow
  + P11.1 exactly-one selected timing layer
  + content ArcRuleId / MinimumFacingDot
  + explicit hit candidate
  + explicit GuardFacing
  + explicit defender-to-threat direction
  -> canonical 3D unit vectors
  -> dot(GuardFacing, DirectionToThreat)
      dot >= threshold -> Qualified
                          -> pass P11.1 selected layer unchanged
      dot < threshold  -> OutsideArc
                          -> auditable receipt, no layer

Recovery | Interrupted | foreign window | foreign target
  -> fail closed
```

## Automation 证据

| Log | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `P11.2-WeaponGuardArc-final.log` | 6 | 0 | 1 | `A1CBABA476E1D1BCFAC1C9E1C92A869CBEE91BF484832BDB589210FB5140019D` |
| `P11.2-WeaponPerfectGuard-final.log` | 6 | 0 | 1 | `994FF9794D1338E28B5BFBF778E03427679FDC546070A6EDCC7AAC3F9493E672` |
| `P11.2-WeaponGuard-final.log` | 12 | 0 | 1 | `AD9E84B18E143F2F3EBC9519E1192FAD070ED39A54EECA981B6D2B470AE29F16` |
| `P11.2-ActionLifecycle-final.log` | 1 | 0 | 1 | `4C7179BB8FF0D4B24D5F685C303504FC72ED225A06F44C94CA8FAB840E94031A` |
| `P11.2-CombatCore-final.log` | 9 | 0 | 1 | `ED485D221FC6D89970BAF6364B1F820AF8AF66AE966EB4A211DA679B0D0153DD` |
| `P11.2-CombatRuntime-final.log` | 105 | 0 | 1 | `C39AB6169204E057A2270245E10477D352F09AA23DB89E10406F83C3B94E2D5F` |
| `P11.2-Shanmen-0_0_10-final.log` | 473 | 0 | 1 | `17B3F738C19DF74617202D9BDEED4D6F7C0DE611FBBC00851AD4E1799C57D49C` |

总计：612 Success / 0 Fail。七份最终日志均有唯一 RunTests、terminal queue-empty 与原生 exit 0。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=91
SELF_TEST: PASS 142/142
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=6 Logs=7
git diff --cached --check: PASS
BOUNDARY_SCAN: PASS hits=0
```

- required groups：WeaponGuardArc、WeaponPerfectGuard、WeaponGuard、ActionLifecycle、CombatCore、CombatRuntime；
- mapping SHA-256：`A15A1B1FFF780AF254143A111EBB24507F9BA8676CA42B7CD9795683460F8B6D`；
- self-test SHA-256：`E71A81D0A58ED3B083B8B5F177DD728438C0B04C0478462672E5C4FADCAD6B00`；
- implementation/mapping：`5 files / +1109 / -0`。

## 构建证据

| Build | Actions | Time | Exit | Log SHA-256 |
|---|---:|---:|---:|---|
| Editor Development | 6 | 29.43s | 0 | `42763954497C857523C50B71C6FCA26A5C58F14BFCFF7F180AD69EA87F0A2676` |
| Game Development | 5 | 28.49s | 0 | `B4976FB2F58A3554D6AA264DEB599CB8D6C7593C9F9BDE211D654EE2F0C8CBB3` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：1489408 bytes / `FA3CC218B92F4A843FE4E5F9358EF8F63802374055559504CCA62F5CDC76221D`；
- `demo_map.exe`：354086912 bytes / `128652480B69F81DFB2916780C87644E5CCD674CAB798AB8EEA178C9E1A4E016`。

## 真实异常

没有真实源码、UHT、Automation、Editor 或 Game 构建失败。

首轮证据门禁失败原文要点为：`REGRESSION_COVERAGE: missing required groups: Shanmen.0_0_10.CombatCore, Shanmen.0_0_10.CombatRuntime, Shanmen.0_0_10.CombatRuntime.ActionLifecycle, Shanmen.0_0_10.CombatRuntime.WeaponGuard, Shanmen.0_0_10.CombatRuntime.WeaponGuardArc, Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard`。根因是日志唯一 `Cmd:` 行包含 `;Quit` 后缀，parser 正确地把它视为组名的一部分。测试结果仍为 612 Success / 0 Fail。修正命令格式并完整重跑后门禁通过；没有修改 parser 或放宽映射。

UE 5.8 的 UnifiedErrorTest 初始化噪声与非 Win64 SDK validation 信息不属于 selected tests。Win64 目标有效，selected Fail 为 0，terminal marker 与原生退出码正常。

## P/F 边界

只执行 P 阶段纯值实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P11.3 优先建立唯一产品方向采样 adapter，把上层实体／Actor 的明确朝向与 threat direction 喂入本阶段纯策略；不得从 HitNormal 猜符号，不得复制 action lifecycle。耐久接入若先行，必须使用 ShanmenItems durable transaction，而不是直接改物品字段。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-2-weapon-guard-arc/Docs/Report/Dev.D.UE.0.0.10.P11.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-2-weapon-guard-arc/Docs/Log/Dev.D.UE.0.0.10.P11.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-2-weapon-guard-arc>
