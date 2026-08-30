# Dev.D.UE.0.0.10.P9.6.r0 Development Log

## 目标

把 P9.3—P9.5 已完成的 SpiritShield action/resource、capacity 与 deadline authority 收束成一个确定性 session aggregate，消除产品调用方手工组合顺序、activation/timeline 错配和半提交状态，同时避免在产品资源 owner 尚未冻结时制造临时 SpiritEnergy 账本。

## 审计结论

1. P9.5 已闭合 action/shield 的提交后 lifecycle，但调用方仍需手工建立 coordinator、capacity authority、deadline contract 与 deadline gate；
2. 手工组合允许资源提交成功后遗漏 capacity/deadline，或 deadline 只关闭 shield 而 action 保持 Active；
3. P9.1 capacity depletion 明确不拥有 action lifecycle，只停止后续防御投影；
4. 现有产品层没有可安全写入、带 revision 且已冻结恢复/持久化语义的 SpiritEnergy authority；
5. 直接新增产品 Host 会发明最终数值和第三套账本，违反“手感优先、数值后置”与单一权威边界；
6. 因此本阶段实现 authority aggregate，不实现产品余额或输入层。

## 设计决策

1. `FShanmenSpiritShieldSchedule` 在 Begin 前冻结 timeline/start/deadline 并派生确定性 id；
2. session 内部拥有 action coordinator、capacity authority、deadline contract 与 deadline gate；
3. 外部只取得 const view，不能绕过 session 改写子 authority；
4. Begin 绑定 reservation 与 schedule，支持 exact replay 和 schedule conflict；
5. Commit 在 session 与外部 resource authority 的候选副本上执行，全部成功后才发布；
6. capacity commit 复用 P9.1 command/revision/receipt，不重复实现容量账本；
7. closure 后保留 exact capacity receipt replay，但拒绝任何新 capacity write；
8. `ObserveDeadline` 把 gate proof 与 action closure 合成一个 typed result；
9. deadline 未到、timeline 不匹配或 action closure 失败都不泄漏候选状态；
10. `Close(DurationElapsed)` 固定拒绝，时间到期必须由 gate 证明；
11. pre-commit Abort 与 post-commit Close 继续使用 P9.5 的不同资源语义；
12. 不加入 World、Actor、Timer、Tick callback、产品输入、资源恢复或存档规则。

## 执行序列

1. 审查 P9.3 resource authority、P9.4 action composition、P9.5 closure 与 P9.1/P9.2 capacity/deadline 契约。
2. 确认产品层缺少可复用 SpiritEnergy writable authority，限定本阶段边界。
3. 新增 immutable schedule 与 deadline closure typed result。
4. 实现 `FShanmenSpiritShieldSession::Begin/Commit/Abort/Close/Reset`。
5. 实现防御投影、capacity commit 与 atomic deadline observation。
6. 为所有多 authority 写入使用 candidate-copy 原子提交。
7. 增加 session 全结构 `IsValid` 交叉校验。
8. 新增八个 focused tests，覆盖 replay、conflict、rollback 与 determinism。
9. 新增 changed-file mapping：session 修改必须覆盖九个 authority/resolver groups。
10. 扩展 mapping self-test 的 pass/fail fixtures。
11. JSON mapping `75` rules 与 self-test `112/112` 通过。
12. Editor candidate 6 actions，原生退出 `0`。
13. 执行十组 Automation，合计 `537` success、`0` fail，全量 `379/379`。
14. changed-file gate `Changed=5 / Rules=2 / Required=9 / Logs=10` 通过。
15. `git diff --check` 与精确生产边界扫描通过。
16. Editor final up-to-date success；Game final 5 actions，原生退出 `0`。
17. 生成同名 Report/Log，执行 exact-stage gate 与暂存范围复核。
18. commit、push，并用远端 commit SHA 形成不可变链接。

## Aggregate 状态流

```text
Begin(action, definition, cost, schedule, resource)
  -> reserve resource + bind schedule
  -> exact replay returns original reservation proof

Commit(resource)
  candidate resource commit
  + shield/action activation
  + capacity authority
  + deadline contract/gate
  -> publish all or publish none

CommitCapacity(command)
  -> exact revision/receipt semantics
  -> zero remaining capacity stops projection only

ObserveDeadline(observation)
  not due -> rejected, unchanged
  due -> deadline proof + shield deactivation + action closure atomically
  exact replay -> original deadline and closure receipts

Close(Explicit|Interrupted|OwnerEnded) -> P9.5 closure
Close(DurationElapsed) -> rejected; must use ObserveDeadline
Abort(reason) -> pre-commit release
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `P9.6-SpiritShieldSession-final.log` | 8 | 0 | yes | 0 | `5E0D9928769CCA7511C495A15AC7B803A5A978F5523B6DDB35CE50F4EF3610F1` |
| `P9.6-SpiritShieldAction-final.log` | 10 | 0 | yes | 0 | `3BE242C4FDD0C346CBEA2B78304EE0AB05CCFCD38035E5898C6191DB00B6E6FC` |
| `P9.6-ActionResource-final.log` | 7 | 0 | yes | 0 | `3025C88CA0B2F6C5E401E67A1EAAD9379B48BAD1E5ABCFAF722E12EE5C3BDEDC` |
| `P9.6-ActionLifecycle-final.log` | 1 | 0 | yes | 0 | `BCCF1BFCB2877107EB1B97779D1D0CB786F90D78625826B750A904FC687E4982` |
| `P9.6-SpiritShield-final.log` | 35 | 0 | yes | 0 | `26E403C524B316873578696CE4799F3042FEC5B3AAC72A23A1104C89698120C3` |
| `P9.6-SpiritShieldCapacity-final.log` | 6 | 0 | yes | 0 | `89306F9F95C7CA3FBCC37B9F20664B46DDD253BDEAFBB5DB14DD802B2BE02F31` |
| `P9.6-SpiritShieldDeadline-final.log` | 6 | 0 | yes | 0 | `0679A65F5D35CC195436B26F666D5387B4A73021610A2EF3A207C6C437726427` |
| `P9.6-CombatCore-final.log` | 9 | 0 | yes | 0 | `1E9620CC714AB3013229696F7FADF246F223FD20400489C0E0D470ABEC2A9BE6` |
| `P9.6-CombatRuntime-final.log` | 76 | 0 | yes | 0 | `AC0FBCC59DB379E8CC0B8D01C4C06914BF846711E22948296B7F51C32A1C28AB` |
| `P9.6-Shanmen-0_0_10-final.log` | 379 | 0 | yes | 0 | `A9E65196549EDEA483475203AF2232F6C0715C3E6586EA7B0A43971EAF26BEAF` |

每份日志均恰有一个目标 `Automation RunTests` 命令、一个 queue-empty、`0` selected fail 和 `0` fatal/unhandled/ensure；十个进程原生退出码均为 `0`。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=75
SELF_TEST: PASS 112/112
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=2 Required=9 Logs=10
REGRESSION_COVERAGE (exact stage): PASS Changed=7 Rules=2 Required=9 Logs=10
git diff --check: PASS
BOUNDARY_SCAN: PASS hits=0 (world/actor/damage/RNG/timer/tick-callback)
```

- required groups：SpiritShieldSession、SpiritShieldAction、ActionResource、ActionLifecycle、SpiritShield、SpiritShieldCapacity、SpiritShieldDeadline、CombatCore、CombatRuntime；
- mapping SHA-256：`2DC746B76708DEEC4CD4F0655BA3289D6F5EDDF1AC308C56A50A80D327D4FE60`；
- self-test SHA-256：`E2002969DAEB7DF7B7D2861DF08087EADF12989191B29BCFC5D792E3650CB6C5`；
- 实现：`5 files / +1323 / -0`。

## 构建证据

| Build | Actions | Time | Exit | Log SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | 6 | 20.92s | 0 | `6FB7EC590862BC4F97A7AF055B4E9647369F4BDEDA4BD1B62C6403A0DB45011C` |
| Editor final | 0 | 0.92s | 0 | `656BBD3BA93559B6C1646E17CF8A7CE17B4C0F47C192C092E91D0C1FD8C65041` |
| Game final | 5 | 28.59s | 0 | `5D50FBB9F800F42178804E75582C8F2F8496A7516E4299F41A6FE49EEB8C3153` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1179136` bytes / `07BC22344B93A6579E1DEA9DF8D4C75B9C3BE7D105E1A0852E7C4BB0EF319781`；
- `demo_map.exe`：`353550848` bytes / `CEF1C13C0FA101B7060982D1149F8E9961BC6DC090AA8F12B1B72D24C21510A2`。

## 真实异常

没有源码、目标测试或构建失败。首次边界扫描用裸 `Tick` 字符串，误报 `StartTick/DeadlineTick` 数据字段；改为引擎 Tick callback、Timer、World/Actor、damage 与 RNG 语义后命中 `0`。没有改变产品代码来迎合扫描。

Automation 启动日志包含 UE 5.8 自带 UnifiedError 基线诊断；选中测试结果、queue-empty 与进程退出均正常。未发生 C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

## P/F 边界

仅执行 P 阶段纯值实现、无头 Automation、静态/门禁和 Editor/Game Development 构建。没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P9.7 应先冻结产品 SpiritEnergy authority 的 owner、revision、恢复与持久化语义，再把本 session 接入 product host；若这些数值政策尚未确定，则不创建临时 float 或第二套账本，改做另一条已明确的 0.0.10 战斗切片。
