# Dev.D.UE.0.0.10.P7.3.r0 Report

## 1. 结论

P7.3 已完成一次性直线暗器的 product RunHost、真实 World 生成、native contact 绑定与统一终态，结论为 **PASS**。

本轮在 P7.2 的 durable launch / World delivery 上建立唯一产品宿主：宿主持有 action、execution、item commit evidence 与 transient projectile；真实生成先得到碰撞关闭的 Empty carrier，exact Quantity commit 成功后才采用 flight；命中、阻挡未命中、距离到期和显式中止均通过既有 P7.2 终止能力收口，随后把 action 完成或中断。未增加第二套伤害、库存或 projectile authority。

输入、视觉资产、发射动画、弧线、追踪、转向和召回仍未接入。

## 2. Product Host 与生成边界

新增 `Fdemo_mapShanmenThrownWeaponRunHost`：

- `SpawnStagedCarrier` 只在有效 World、同 World source Actor、合法 class 与有限 origin 下生成一个 carrier；
- Actor 生成后仍为 `Empty`，collision `NoCollision`、movement inactive；
- `TrySpawnAndLaunchPrepared` 组合生成与 P7.2 stage/durable commit，失败时销毁未采用 carrier；
- `TryLaunchPreparedCarrier` 支持 deferred/pre-spawn 产品路径；
- `TryAdoptPublishedFlight` 只采用已经由 exact durable evidence 发布的 flight，不重复发送 inventory IO，可用于恢复与回放；
- host 为不可复制、不可移动对象，native delegate 不会因容器复制悬空；
- GameThread 上析构 active host 会失败关闭为 `Interrupted`，不会留下无人接收 contact 的飞行 Actor；
- coordinator 是明确标注的非拥有引用，run owner 必须在 host terminal/reset 前保持其存活。

生成适配器在真实无头 `GamePreview` World 中已有自动化证据，不只测试 null/fake 路径。

## 3. Contact 与终态

host 同时绑定 projectile 的 contact 与 range-expired native delegate：

1. 注册目标 contact 继续调用 P7.2 `ResolveProjectileContact`；
2. vitality 提交成功后，execution/Actor 进入 `Spent`；
3. host 将 action `Active -> Recovery -> Completed`；
4. terminal publication 解除两个 delegate、清理 lifespan 并按所有权销毁 World Actor；
5. 重复 callback 因 delegate 已解除且 flight 已 terminal，不能再次提交伤害。

未注册或不可交付的 blocking contact 失败关闭，并转成 `BlockingMiss`：不伪造 impact、不写 vitality，但物理一次性飞行仍结束。range expiry 使用 `MaximumDistance / frozen LaunchSpeed` 设置 lifespan，Actor 的 native expiry 回调与显式 `TryExpireRange` 都走同一个 no-impact terminal。显式 `TryInterrupt` 先 copy-on-write 校验 action interrupt 与 execution finish，再原子发布 `Interrupted`。

terminal receipt 保留 LaunchId、delivery diagnostic 以及 completed/interrupted action transition receipts，即使 transient Actor 已销毁仍可审计。

## 4. 自动化证据

最终 `-Unattended -NullRHI` 自动化全部通过；每份日志只有一个实际 RunTests、queue-empty、Fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.ThrownWeaponRunHost` | 3 | 0 | `3830F50CBF81F62A10A2FB8B29CB1711327456EFD09B85E2C73E23B2DBC64DDB` |
| `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery` | 3 | 0 | `D46082C935AB0C8607DDD3BE6D22B7B989934285BAE8893CA8370BE1251D9A5E` |
| `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter` | 4 | 0 | `F53FB699D5AECEBFD59793B8C2073DB2EF3072198DFE3C0C1660DB2281941A51` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | `DF02B67DDD3A67FABF7473FF61D2B6CD4E8E1C9CDEE64040B3F956C9E7C9E49A` |
| `Shanmen.0_0_10.Items` | 72 | 0 | `2E4163129C5D2A513CB096EACED31673633BBECF4B054130FEFABD179AD87EA5` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | `CA192DFB212D4A24E20810E9D907E506E3868272D90109D5112C4490C8B3E6F9` |
| `Shanmen.0_0_10.CombatRuntime` | 30 | 0 | `DE8D112C50099AC1C61BC7A0FB8D09FBA6E3937E44F327592887718C22C6B7DA` |
| `Shanmen.0_0_10` | 188 | 0 | `23B981CDA717336BDB95AD15358C1BE13078E3A0A39EFEC134A3417CB9AA5EFA` |

完整 suite 从 P7.2 的 185 增至 188。三项新增产品测试分别覆盖：真实/失败关闭 spawn 与 interruption、contact delegate 到 vitality 的一次性生命周期、blocking miss 与 range expiry。

## 5. 改动—回归与静态门禁

- `REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=7 Logs=8`；
- mapping self-test：`18/18 PASS`，新增 RunHost 到七个 authority seam 的映射用例；
- `git diff --check`：native exit `0`；
- 新 host/Actor 无 `ApplyDamage`、`UGameplayStatics`、GameMode、timer authority、RNG、legacy projectile 或临时 debug marker；
- `SpawnActor` 只存在于 P7.3 明确拥有的生产生成适配器；
- 未修改 schema、Build.cs、GameplayTags、Content、GameMode、输入、Profile 或 CodeB。

## 6. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor integration：`8/8`，Succeeded，native exit `0`，`22.03s`；
- 最终 Editor：`5/5`，Succeeded，native exit `0`，`9.86s`；
- 最终 Game：`4/4`，Succeeded，native exit `0`，`14.18s`；
- Editor product DLL UTC：`2026-08-29T11:09:15Z`；
- Game executable UTC：`2026-08-29T11:10:46Z`。

本轮没有源码编译失败或自动化失败。首次 host 自动化即为 `3/3` 成功；后续只增加真实 World spawn 证据和 copy-on-write interruption 收紧，并重新运行最终 host/full suite。

## 7. 修改范围与兼容性

生产范围仅包括：

- 新 thrown weapon RunHost；
- projectile 新增无业务逻辑的 native range-expired seam；
- 三项 RunHost 产品测试；
- changed-file regression mapping 与 self-test；
- 本 Report 与同名 Development Log。

P7.0 execution、P7.1 item adapter、P7.2 World adapter/coordinator delivery、ShanmenItems schema、旧 projectile/skill、御器、敌人和输入代码均未改。完整 0.0.10 回归通过。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 和用户文档未纳入 stage。

## 8. P/F 边界与下一阶段

本轮只执行 P 阶段源码、静态检查、无头 Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

P7.4 建议建立 thrown weapon Run command router：从已选择 item/action request 依次取得 P7.1 prepared intent、调用本轮 spawn-and-launch，并返回统一 request/receipt；仍先不绑定具体键位或视觉表现。这样输入层最终只提交命令，不直接触碰 inventory、Actor 或 vitality。

## 9. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-3-thrown-weapon-run-host/Docs/Report/Dev.D.UE.0.0.10.P7.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-3-thrown-weapon-run-host/Docs/Log/Dev.D.UE.0.0.10.P7.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-3-thrown-weapon-run-host>
