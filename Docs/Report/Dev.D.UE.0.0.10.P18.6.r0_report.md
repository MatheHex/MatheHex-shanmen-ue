# Dev.D.UE.0.0.10.P18.6.r0 Report

## 1. 结论

P18.6 在 P 阶段边界内完成，结论为 **PASS**。

本轮在 P18.5 的 Sword Qi 输入适配器之前增加 Run-scoped 逻辑命令事件所有者。每次新命令由当前 Run 与单调序号派生稳定 `InputEventId`，再调用既有 `RouteSwordQiStartInput` 唯一入口；显式重放必须携带该 owner 已提交的不可变事件令牌。该层不拥有真实按键、空间真值、物品、属性、库存、Actor、投射物、伤害或生命权威。

```text
Sword Qi Command Event Owner exact:    4 Success / 0 Fail
Shanmen.0_0_10 full:                 781 Success / 0 Fail
Required legacy groups:              443 Success / 0 Fail
Regression coverage:                  PASS (Changed=7 / Rules=2 / Required=55 / Logs=10)
Regression gate self-test:             PASS 287/287
Boundary scan:                         PASS (Files=2 / Matches=0)
Game + Editor Development:             PASS / native status 0
```

本轮没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件或真实输入，也没有执行截图、Smoke、Cook 或 Package。因此证明的是逻辑命令身份、重放及现有产品链的无头代码闭环，不宣称物理按键、表现、手感或人工验收通过。

## 2. Run-scoped 逻辑事件身份

新增 `Fdemo_mapShanmenSwordQiCommandEventOwner`、不可变事件值 `Fdemo_mapShanmenSwordQiCommandEvent`、结果与 teardown summary。事件身份命名空间为 `demo_map.SwordQi.CommandEvent.r1`，输入是：

```text
active RunId + EventSequence -> deterministic InputEventId
```

事件值只公开 RunId、InputEventId 与 EventSequence getter；有效性会重新派生并核对身份。默认 owner 为 valid/empty，`TryBegin` 只能绑定一个有效 Run，active owner 不能换绑，`TryEnd` 必须匹配同一 Run。序号从 1 开始；达到保留上限时失败关闭，不允许整数回绕或身份复用。

## 3. 提交与重放纪律

一次新命令先生成候选事件，再委托 P18.5：

```text
Run owner valid
  -> derive candidate event
  -> P18.5 gameplay/route/identity/spatial gates
  -> invoke sole product route
  -> commit event and advance sequence
```

- gameplay blocked、route unavailable、Run/identity 无效或空间样本无效发生在产品调用前，不提交事件、不推进序号；
- 一旦 `bProductRouteInvoked` 为真，即使产品返回 HostBusy 或其它拒绝，也提交本事件并推进序号，避免潜在副作用后自动复用身份；
- 自动重试不存在；重放必须调用 `TryReplay` 并传回同一 active Run owner 已提交的事件值；
- 重放不推进序号，也不能伪造未来事件或跨 Run 事件；
- owner 只拥有事件身份。空间样本仍由调用方提供；P18.4 一旦捕获 intent 后负责冻结 payload 并拒绝同身份异轨迹冲突。

## 4. GameMode 生命周期集成

`Ademo_mapGameMode` 新增：

- `IssueSwordQiStartCommand`：分配新逻辑事件并调用现有输入适配入口；
- `ReplaySwordQiStartCommand`：只重放已提交事件；
- `GetSwordQiCommandEventOwner`：只读暴露生命周期证据。

Run activation 在 Sword Qi Product Controller 成功绑定后绑定 command owner；任一步失败沿既有 `ReleaseCombatProductRun` 回滚。teardown 先恢复可能未完成的经脉冲击 durable transaction，再释放 command owner，随后释放 Sword Qi Product Controller。正常、空态、孤儿态及 rollback 路径都清理 owner；RunBound/RunReleased 日志分别记录 owner active 状态与已提交事件数。

## 5. 聚焦测试

新增四条 Automation：

1. `LifecycleIdentity`：确定性身份、Run/sequence 隔离、幂等同 Run begin、错 Run teardown 拒绝与 summary；
2. `PreRouteFences`：门禁与无效空间样本不采取消耗身份，产品调用后即提交；
3. `AppliedReplay`：首次应用、显式重放、跨 Run/未来/无效事件拒绝及 teardown 计数；
4. `BusyRetry`：HostBusy 仍提交逻辑事件，上一发 terminal retirement 后以同一事件显式重试，复用 P18.4 冻结 command，不重新分配事件。

Fixture 使用 unattended GamePreview world、NullRHI，以及真实 ItemAuthority、AttributeComponent、CombatRunCoordinator、P18.4 Product Controller 与 P18.5 Input Adapter。测试没有绑定或模拟物理设备。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi Command Event Owner exact | 4 | 0 | `E6417A8B4EC960481E05F9DC6B329DA83358BE735587F6CA081DB0AB68C5F12A` |
| `Shanmen.0_0_10` full | 781 | 0 | `041DE0950137FD66BF08DAC3AEB45574E5076A7F896896CEA02AD674E953A3BD` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `481198595A1695312E5B59E255E5FF811F084CFE2715B5ECD4434F9FA1946AA7` |
| `demo_map.Profile` | 211 | 0 | `DF92122AA26720C00ADD2BCCA7FA270C55D217D4BF98C496B1B5378B813B9804` |
| `demo_map.CodeB` | 60 | 0 | `114987905ACD3B98B93A0C1F26C4EFA07C69C1975F3A33182E05518A3739F946` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `A0C8B75AB381828ED138B3A37DD7C47CEEC2F8F8C0A46F559DCDEE943830D674` |
| `demo_map.P4.Hotbar` | 7 | 0 | `F66E3F79DDF83D8199024FEE4A736C430D4C12805CD204EEC96370CC3E0819D0` |
| `demo_map.V3` | 29 | 0 | `2D379CC8553429618420F28A9FF65CFF66F81F8326B8E06C5557CA8CD81995A8` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `4BBCF9C3E7BD26558618B16618FBF46C73E5A94B013A435962A8C4BCE9E0E92F` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `9E935992A7C814736A1F6167E3C6FCAE3A40821E986680B4045219D6EE794BE7` |

完整套件首末 Success 为 `2026.09.03 02:38:41.974 -> 03:08:26.876 UTC`，约 29m44.902s；相对 P18.5 的 777 条精确增加 4。所有采用日志 Fail 为 0，并包含正常 terminal/exit 证据。

## 7. 改动驱动回归与静态门禁

Regression map 新增 `SwordQiCommandEventOwner` 路径规则，并把 GameMode 映射扩展到聚焦组。最终门禁：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=55 Logs=10
SELF_TEST: PASS 287/287
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage SHA：`C2A4B6116A9BF72D4DC42C7AD9D0069ECD88F5AFCFE3187B2624E71B6AA4CF6C`；
- self-test SHA：`41D356DF8529E4075B30222EF4AD4AE041E1511EF5E4778AFE89D5CB2BEE26B2`；
- regression map SHA：`10D562215DF2E8AA6DD4767EFCD56E146C8D72D586DDFC5E17BAC25E44C5FCD3`；
- boundary SHA：`0D37EA04FB41FF5D5784CA3F2E447A2B54605F170FD31EE541661C2681B4C572`；
- diff-check SHA：`2617504A024FB5606992573FB635EAFB19096E71F4F520ECB1430757C8F4111C`。

边界扫描确认生产 owner 不依赖 GameplayStatics/ApplyDamage/TakeDamage、旧 SkillProjectile、UWorld/AActor、RNG、库存写事务、物品/属性读取、物理输入绑定、声音或 Niagara。

## 8. 构建与产物

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded / native 0 | 27 / 151.91s | `55F89AF157D4DE8C7011B8EEC0D75A9EF725B53B10A6BE64FCC03D63500E4158` |
| Game Development final | Succeeded / native 0 | 26 / 132.51s | `40C081E812362533A9D2F37B52041E2E7CFEC0E7942AC67005CF6237095BD5B3` |
| Editor Development final | Succeeded / native 0 / up to date | 0 / 0.97s | `2C6C97BE7B04AC54520A9BC363196CA12634F3265CE4FE738425DF4D27125C51` |

最终产物：

- `demo_map.exe`：356,427,264 bytes，SHA-256 `345B6174A687AF476063F0109CB405164D83ADBA4FEC61D4170F959D991934A9`；
- `UnrealEditor-demo_map.dll`：15,109,632 bytes，SHA-256 `502EA2E798053803FF4776BFBEFC997C12078DA91E09F85C1274F1BF09415C38`。

## 9. 提交与 P/F 边界

基线提交为 `64ab4e252fe7684683bb694b856ad5f6a162f85c`。本轮提交边界为 3 个新增源码、4 个修改文件、本 Report 与本 Development Log，共 9 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；`Saved/Codex/P18.6` raw logs 不进入 Git。

未修改 Content、地图、资产、配置、Windows、UE Engine、存档 schema，亦未改变既有装备、库存、属性、伤害或生命权威。真实 Enhanced Input、键位、动画、声音、特效、手感与产品运行仍属于明确授权后的 F 阶段。

## 10. 下一阶段与 GitHub

P18.7 建议把一次逻辑命令的 origin/aim 样本与事件身份一起冻结为不可变 command request。这样即使产品在 P18.4 捕获前拒绝，后续显式重放也不能意外重新采样成另一轨迹；仍不绑定物理按键，也不复制 P18.4 的产品 command authority。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-6-sword-qi-command-owner>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-6-sword-qi-command-owner/Docs/Report/Dev.D.UE.0.0.10.P18.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-6-sword-qi-command-owner/Docs/Log/Dev.D.UE.0.0.10.P18.6.r0_log.md>
