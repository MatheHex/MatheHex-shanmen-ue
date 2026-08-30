# Dev.D.UE.0.0.10.P9.3.r0 Development Log

## 目标

在 P9.0 SpiritShield lifecycle、P9.1 capacity authority 与 P9.2 deadline gate 之外建立唯一 typed action-resource transaction，使资源在 action Startup 时预留、在真实 commit point 消费、在 commit 前终止时释放，同时不伪造产品灵力数值或第二套动作状态机。

## 审计结论

1. 人工 0.0.10 设计基线明确存在“灵力/消耗”概念，但最终数值延后；
2. 当前产品源码没有 SpiritShield 调用点，也没有可直接复用的 typed 灵力写 authority；
3. 现有 action orchestrator 已提供不可变 Startup、commit 与 terminal receipts，应复用其事实源；
4. 资源 owner、channel、revision、余额与 reservation ledger 必须由一个单写者 authority 维护；
5. P9.3 只建立通用资源事务，不把临时属性或硬编码 shield cost 冒充产品完成度。

## 设计决策

1. 资源通道使用 native GameplayTags，先定义 `Shanmen.Resource` 与 `Shanmen.Resource.SpiritEnergy`；
2. cost、snapshot、reservation request/receipt、finalization request/receipt 均为不可变 proof；
3. Startup reservation 降低 available 但不降低 current；
4. exact Startup→Active transition 才 commit；pre-commit Cancelled/Interrupted 只 release；
5. reservation identity 绑定 activation、source 与 channel，command identity 额外绑定 cost 与 snapshot；
6. exact replay 返回原 receipt，语义冲突失败关闭；
7. 所有 mutation 在 authority 候选副本中完成，完整校验后才原子替换；
8. snapshot revision 与 float bit identity 防止 stale 或近似输入改写权威；
9. final numeric balance、恢复、持久化与产品投影不属于本阶段。

## 执行序列

1. 审查 action orchestrator、SpiritShield 三阶段 runtime、现有 Items/resource transaction 与产品属性 seam。
2. 确认产品层不存在可提交的 SpiritShield/typed spirit-energy owner。
3. 冻结 generic action-resource contract、状态不变量、重放与冲突语义。
4. 新增 authority header/cpp、deterministic IDs、typed tags 与 structured results。
5. 首轮代码复审发现 Reserve/Finalize 在最终 `IsValid()` 失败时可能留下已写状态；改为候选副本验证后原子替换。
6. 新增 7 个 focused tests，覆盖 reserve/commit/release、失败关闭、多 reservation 与 deterministic replay。
7. 新增 `ActionResourceAuthority` changed-path 规则，强制 ActionResource、ActionLifecycle 与 CombatRuntime 三组证据。
8. mapping 从 `72` 增至 `73`；self-test 从 `106` 增至 `108`，含一条 pass 与一条 fail-closed fixture。
9. Editor candidate 11 actions，原生退出 `0`。
10. Automation：ActionResource `7/7`、ActionLifecycle `1/1`、CombatRuntime `58/58`、full `361/361`。
11. implementation changed-file gate `Changed=7 / Rules=2 / Required=3 / Logs=4` 通过。
12. 静态边界扫描、`git diff --check` 与 cached diff check 通过。
13. Editor final up-to-date success；Game final 10 actions，原生退出 `0`。
14. 生成同名 Report/Log，执行 exact-stage、commit、push 与远端 SHA 核验。

## 状态迁移

```text
Action Idle -> Startup receipt
  + typed cost
  + exact authority snapshot
  -> Reserve
       current unchanged
       reserved += cost
       available -= cost

Startup -> Active commit receipt
  -> Commit
       current -= cost
       reserved -= cost
       available unchanged

Startup -> Cancelled / Interrupted receipt
  -> Release
       current unchanged
       reserved -= cost
       available += cost

exact command replay -> original receipt; no revision change
conflicting/stale/foreign proof -> rejected; no mutation
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `ActionResource-final.log` | 7 | 0 | yes | 0 | `05B0FE83D33FEA66EAA9112D86AAA718B02B850FD4566A11BE19D4DFDAC466C0` |
| `ActionLifecycle-final.log` | 1 | 0 | yes | 0 | `2615A0E13FFEE1DB453D9BE3456D391795EFDAC375ADEB09065CE9FE4A09DBA1` |
| `CombatRuntime-final.log` | 58 | 0 | yes | 0 | `9AA33C4935B78127B4971E3C7D8E2EB7B702E8B5363FD7A7FF8126FD449E9CA0` |
| `Shanmen-0_0_10-final.log` | 361 | 0 | yes | 0 | `857A2FA57DEB67BF8E9614E3CB64E12130E24B02D2881E8157608D528B30EC46` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=73
SELF_TEST: PASS 108/108
REGRESSION_COVERAGE (implementation): PASS Changed=7 Rules=2 Required=3 Logs=4
git diff --check: PASS
BOUNDARY_SCAN_MATCHES=0
```

- required groups：ActionResource、ActionLifecycle、CombatRuntime；
- mapping SHA-256：`9B476EC30AA3DCA790CF5EFF19C110A174738E0E92F5D2281B8D9F48EA8B74DF`；
- self-test SHA-256：`B1F4E4981C9218218E652EF3DEDD89D1C73BC7B547B767C74B09F1ABE99E8E1E`；
- code/scripts：`7 files / +1839 / -0`。

## 构建证据

| Build | Actions | Time | Exit | SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | 11 | 52.30s | 0 | `FD53A32A99E43E88C5442FA5A86F00783BB0AA102BD010766D2AC20339367FDF` |
| Editor final | 0 | 0.90s | 0 | `3D546769EBFFC153C2FD598F47FE2F0FAAE699F97D6644061AAC37AF8BCDC624` |
| Game final | 10 | 40.45s | 0 | `1CD0EE4F4EBCD5A3842E8F4E9E5F94C84E7616CF2384B1BD471322CB60A145A4` |

## 真实异常

没有源码、构建或目标测试失败。四次 Automation 启动均保留 UE 5.8 既有的非目标平台 SDK metadata 警告和测试发现前 13 条 `Condition failed` 诊断；Win64 SDK 有效，目标测试全部 Success，进程均原生退出 `0`。

## P/F 边界

仅执行 P 阶段纯值实现、无头测试、静态/门禁和 Editor/Game Development 构建。没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P9.4 应以单一产品 host 原子组合 action Startup、typed resource reservation、commit-point resource consumption 与 SpiritShield activation；失败路径按 receipt release，成功后继续接 P9.2 deadline 和 P9.1 capacity。正式 cost 与恢复规则必须由产品内容定义。
