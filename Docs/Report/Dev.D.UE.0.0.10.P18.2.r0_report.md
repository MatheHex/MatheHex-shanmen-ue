# Dev.D.UE.0.0.10.P18.2.r0 Report

## 1. 结论

P18.2 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P18.1 的剑气物理载体与世界结算桥接成一个最小产品 `SwordQiRunHost`。Host 负责创建并采用唯一载体、绑定接触与超距事件、持有 Action/execution 生命周期副本，并以明确的“首次阻挡接触终止”策略收敛命中、阻挡未解析、超距和中断。合法目标仍只经现有 CombatRunCoordinator 与唯一 vitality authority 扣血，没有建立第二套伤害或生命路径。

最终结果：

```text
Sword Qi Run Host exact:               3 Success / 0 Fail
Shanmen.0_0_10 full:                 761 Success / 0 Fail
Regression coverage:                  PASS (Changed=5 / Rules=1 / Required=6 / Logs=2)
Regression gate self-test:             PASS 277/277
Boundary scan:                         PASS (Files=3 / Matches=0)
Game + Editor Development:             PASS / native status 0
```

本轮没有接真实输入、剑招选择、表现资源、声音或关卡投放，也没有启动 Unreal Editor UI 或产品。因此这里证明的是产品 Host 的代码级与无头生命周期闭环，不宣称实际游戏手感或表现验收通过。

## 2. 终止策略与所有权

P18.1 刻意没有冻结“命中后穿透”或“命中后消散”。P18.2 对实际载体重新审计后确认：当前 `USphereComponent` 使用 blocking hit，`UProjectileMovementComponent` 在阻挡接触后会停止；若 Host 声称成功目标可继续穿透，将与真实物理状态不一致。

因此本阶段采用与当前载体一致的单一规则：

```text
首次 blocking contact
  -> 可解析且可提交：一次 canonical impact -> 完成 Action -> 消散
  -> 不可解析或不可提交：0 impact -> 完成 Action -> 消散
range expired             -> 0 impact -> 完成 Action -> 消散
explicit interrupt        -> 0 impact -> 中断 Action -> 消散
```

真正的穿透能力需要后续把载体改成 overlap/sweep 或显式续航物理模型，不能只在 Host 中假装移动仍在继续。

Host 唯一持有 Action runtime、Sword Qi execution、carrier delegate binding、最大距离与终止审计。CombatRunCoordinator 是有界非拥有引用；上层 Run owner 必须让它存活到 Host 终止或 reset。载体只负责物理与几何事件，不拥有伤害、目标合法性或 Action 阶段。

## 3. 创建、发布与距离权威

`SpawnStagedCarrier` 依次校验 World、来源 Actor、载体 class 与有限 origin，再以 `AlwaysSpawn` 创建载体。创建结果只有在载体保持 `Empty`、无碰撞、movement inactive 且组件完整时才成功；异常载体立即销毁。

`TrySpawnAndLaunch` 复用 P18.1 的 copy-on-write 流程：

1. 暂存 launch plan，载体保持惰性；
2. 原子发布 execution 与 physical flight；
3. Host 采用已发布 flight 并绑定 native contact/range delegates；
4. 对 Host 自建的 World carrier 设置 `MaximumRange / Speed` lifespan。

最大距离和生存时间都来自冻结的 `FShanmenSwordQiLaunchReceipt`，Host 没有第二份范围或速度配置。测试冻结 `MaximumRange=1400`、`Speed=900`，lifespan 精确为 `1400 / 900` 秒。

## 4. 接触、生命提交与幂等

合法接触继续复用 P18.1 的唯一交付链：

```text
native FHitResult
  -> WorldGameplay identity
  -> Sword Qi candidate / resolver
  -> CombatRunCoordinator
  -> existing target vitality authority
```

测试冻结 `BaseDamage=0.5`、`AttackPower=20`、系数 `0.01`，一次目标接触精确造成 `0.7` vitality，authority revision 只增加 1。提交成功后 Host 才采用含 ledger 的 execution candidate，然后同步关闭 emission、消散载体并把 Action 从 Active 推进到 Recovery/Completed。

终止发布会先解除 contact/range delegates，再取消 lifespan 并销毁 Host 自建的 transient Actor。终止后重放同一 contact 不会产生第二次生命提交。

## 5. 失败关闭与终止审计

Host 为各路径保留不可变 `Fdemo_mapShanmenSwordQiTerminalReceipt`：`Impact`、`BlockingMiss`、`RangeExpired` 或 `Interrupted`。Impact 终态必须恰有 1 个 accepted impact，其余终态必须为 0；完成路径必须带有效 Recovery/Completion receipts，中断路径必须带 Interrupted receipt。

其它围栏包括：

- Active Host 拒绝 reset 与二次启动；
- 不匹配的 Action/execution/source/Run/载体状态拒绝采用；
- 未注册阻挡对象不会转化为伪目标或伪伤害；
- range 与 interrupt 重复终止失败关闭；
- GameThread 上销毁仍活跃的 Host 时，析构函数显式中断并消散载体；
- 若出现“生命已提交但物理终止意外失败”的不可达竞态，保留已提交 execution ledger，避免后续回调绕过幂等记录。

## 6. Automation 证据

新增三条产品测试：

1. `SpawnLaunchAndOwnership`：无 World 拒绝、真实 GamePreview World 创建、原子采用、冻结 lifespan、active reset 围栏与显式中断；
2. `ImpactLifecycle`：native delegate 到唯一 vitality authority、精确 0.7 伤害、revision +1、首次阻挡完成与终止后重放惰性；
3. `MissRangeAndFailClosed`：未注册阻挡、显式超距、重复终止和析构失败关闭。

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi Run Host exact | 3 | 0 | `1294266A616144CC550072C4FE53904770C61269F26D44DF6D13D131033051D3` |
| `Shanmen.0_0_10` full | 761 | 0 | `7F33030D97FC64D8F18EFDF1D9A54825CCFDB1769350886B830C37A534BB4858` |

两份 Automation 日志均有原生 `TEST COMPLETE. EXIT CODE: 0`，Fail、Fatal、Unhandled 与 Ensure 计数均为 0。完整套件首末成功时间为 `2026.09.02 21:29:35.873 -> 22:00:08.890 UTC`，约 30m33.017s。P18.1 的 758 条加本轮 3 条，计数增量严格一致。

## 7. 改动驱动回归与静态门禁

新增 `SwordQiRunHost` 回归映射。只要 Host 或其测试发生改动，必须同时具备以下六组健康证据：

- `Shanmen.0_0_10.Product.SwordQiRunHost`；
- `Shanmen.0_0_10.Product.SwordQiWorldDelivery`；
- `Shanmen.0_0_10.Product.CombatRunCoordinator`；
- `Shanmen.0_0_10.WorldGameplay`；
- `Shanmen.0_0_10.CombatRuntime`；
- `Shanmen.0_0_10.CombatCore`。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=6 Logs=2
SELF_TEST: PASS 277/277
BOUNDARY_SCAN: PASS Files=3 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage log SHA：`AEBFEE99C425DB9562D74EDF73692B551BBC089B6116DD84F0EAA2ADF864B875`；
- self-test log SHA：`7BF1E601F011F44E696BB2F1B7CF09CD77CA11FEDFFCE4F88DE6723FC4A393C6`；
- regression map SHA：`02B88A5104E50AD9EEFDC593D17B6B434E785FF87EC04D641C05D9A4EABECBB7`；
- boundary log SHA：`60B7F34F72883655FF6103A874009A2AEFF8A4AEF6B17979AE4CA41F304443DF`。

静态扫描没有发现直接 `ApplyDamage`/`TakeDamage`、`UGameplayStatics`、旧 `demo_mapSkillProjectile`、随机流或物品 Reserve/Commit/Consume 越界调用。Actor 创建与 lifespan 由本 Host 明确拥有，因此不作为越界项。

## 8. 构建与产物

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded | 5 / 31.40s | `3CC09889E4E1287CC8006F6B6415EDA060DE70211462657A283042F0C1B382A6` |
| Game Development final | Succeeded | 4 / 25.24s | `4BD20F6F496ECE6FBD78FF05B125333C987E1B0DF3AC60D44D469AB6799BCBE7` |
| Editor Development final | Succeeded / up to date | 0 / 1.00s | `75D43894B53A152D8088142E2A8A24D023E63E35FF212A30D138ADE8C75D1D26` |

最终产物：

- `demo_map.exe`：356,271,616 bytes，SHA-256 `1383B6AE4B659982A0B5DF15427C69FEC8E96885DABC461C1A3697C66B229434`；
- `UnrealEditor-demo_map.dll`：14,925,824 bytes，SHA-256 `8CE2F29CA4644B40633743E4C1ED2E83FBC1A2FFD98FD45CB331049EEAFA36BC`。

首次 Editor build 一次通过，没有编译修复轮；Game 与最终 Editor build 同样为 native 0。

## 9. 修改范围与 P/F 边界

本轮提交 3 个新增源码文件、2 个回归门禁文件及本 Report/Development Log，共 7 个文件。没有修改 Content、地图、资源、配置、Windows、UE Engine、既有生命权威或用户设置。

只执行 unattended、NullRHI Automation、静态门禁和 Development builds。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package，也没有把这些未执行项目描述为成功。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料未修改、未暂存、未提交。raw build/test logs 只保存在本地 `Saved/Codex/P18.2`。

## 10. 下一阶段

P18.3 建议建立最小 Sword Qi 产品命令路由：从既有剑动作授权与 Run owner 的 Active commit 点创建一个 Host，把 Action 终止同步到 Host，并以稳定 launch request 暴露给未来输入/表现层。仍先不接最终视觉、声音和真实输入，避免在产品授权链尚未闭合前冻结表现资产。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-2-sword-qi-run-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-2-sword-qi-run-host/Docs/Report/Dev.D.UE.0.0.10.P18.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-2-sword-qi-run-host/Docs/Log/Dev.D.UE.0.0.10.P18.2.r0_log.md>
