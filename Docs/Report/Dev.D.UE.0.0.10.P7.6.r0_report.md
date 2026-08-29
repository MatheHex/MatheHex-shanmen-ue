# Dev.D.UE.0.0.10.P7.6.r0 Report

## 1. 结论

P7.6 已完成 active-Run 一次性直线暗器的 product session owner，结论为 **PASS**。

本轮把 P7.5 product controller 接到两个既有产品真值：exact item 只从冻结的 `Fdemo_mapShanmenRunCorrelation::HotbarItemInstanceIds` 解析，攻击数值只通过 `Fdemo_mapPlayerCombat::CaptureAttackPower` 从当前 source Actor 捕获。调用方只提交设备无关的热栏槽位和轨迹，不得传入 item identity、库存可用量、action sequence 或攻击力。

输入键位、UI、动画、视觉资产、弧线、追踪、转向和召回仍未接入。

## 2. Session 输入与绑定契约

新增 `Fdemo_mapShanmenThrownWeaponHotbarIntent`：

- 保存稳定 `SelectionId`、1-based 热栏槽位、origin、canonical aim direction 与 maximum distance；
- capture 拒绝无效 GUID、槽位 `1..9` 之外的值、非有限向量、零方向与非正 range；
- `Matches` 比较完整 payload，同一 SelectionId 不能更换槽位或轨迹；
- 不含 `EKeys`、`UInputAction`、item GUID、攻击力或库存命令。

新增 `Fdemo_mapShanmenThrownWeaponSessionConfig`，冻结 P7.0 definition capture 与产品 source tags。`TryBegin` 绑定 exact Run correlation、source Actor 和 config；同一绑定可幂等重入，活动期间禁止替换 Run、Actor 或 content。

## 3. 热栏与战斗属性真值

Session 复制完整 Run correlation，后续只按槽位读取其九格 `HotbarItemInstanceIds`。空槽在 product capture 和 action sequence reservation 之前拒绝；不存在第二套热栏，也不读取 legacy `Udemo_mapItemSubsystem`。

首次合法 SelectionId 解析 exact item 后调用 `Fdemo_mapPlayerCombat::CaptureAttackPower(SourceActor)`，并把结果写入 P7.5 immutable ProductCapture。之后即使角色属性变化，exact retry 仍使用首次捕获值。测试证明攻击力从 `6` 改为 `11` 后重放仍为 `6`，HostBusy 时以 `8` 首次捕获的 selection 在属性改为 `13` 后仍以 `8` 执行。

## 4. 提交、重放与冲突

`Fdemo_mapShanmenThrownWeaponProductSession` 统一拥有 P7.3 Host、P7.4 Router 与 P7.5 Controller：

1. 验证 active session、Run、source Actor 与 canonical hotbar intent；
2. exact SelectionId replay/conflict 在任何新槽位解析和属性采样前处理；
3. 新 selection 从冻结热栏解析 exact item，并只捕获一次当前 AttackPower；
4. 调用 P7.5 controller，由既有 item authority、Run coordinator 与 router 唯一拥有 sequence、Quantity 与 World launch；
5. 保存 hotbar intent、exact product selection、ProductCapture 和最终 product result 的可审计映射。

同 SelectionId 不同槽位或轨迹返回 `SelectionIdConflict`，不消耗 sequence、不读取新属性，也不触发库存或 Actor 副作用。HostBusy selection 保留原 command/sequence；Host 终止后重试不会分配新 identity。

## 5. Host 生命周期与恢复门

新的或 transient selection 可在 Host 已终止时显式 reset 后继续；已有 durable terminal record 的 exact replay 不清除 Host 审计，也不重新发射。

`TryRecoverCancellation` 只接受原 hotbar intent，委托 P7.5/P7.4 的 selection-only cancellation recovery，永不 launch。`TryEnd` 拒绝 in-flight Host 和任意 `RecoveryRequired` record；恢复完成后才 reset Host/Controller/Router 并清空 Run 绑定。

恢复测试先占用 Host、冻结第二个 selection、预准备其 Quantity，再在 null-class 取消路径注入 `WriteTemp` 失败。该 selection 保持 sequence `2` 和空 Host，session end 被阻止；取消恢复后才允许结束。

## 6. 自动化证据

最终 `-Unattended -NullRHI` 自动化全部通过。canonical 日志均只有一个实际 RunTests、一个 queue-empty、Fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `ThrownWeaponProductSession` / `P7.6_Targeted.log` | 4 | 0 | `5902A582853237B1A2268C94F84BD8B5C3CF03C4462F498ACDF57AC9DCBBAAC0` |
| `Shanmen.0_0_10` / `P7.6_Full.log` | 200 | 0 | `B76A227158AFCACB4E8D931ADD080CF4A1C79DAFC4E651AEB05B5038C5751034` |
| `demo_map.ItemUseAndArmor` / `P7.6_ItemUseAndArmor.log` | 46 | 0 | `C2F0C0485D0575291A6C24E8B559C8086BBF482D81B0E9156239A5F0D2C91C98` |

完整 suite 从 P7.5 的 `196` 增至 `200`。四项新增测试覆盖 contract/binding、热栏解析与属性冻结、exact replay/conflict、HostBusy retry/terminal rollover，以及 cancellation recovery/end gate。

首次定向测试为 `3 Success / 1 Fail`，日志 `P7.6_Targeted_FirstFailure.log` 的 SHA-256 为 `E4076D3199F2E7499F9E775D2B4CEAC7AB1A418E7B635BB277B676F1EBF454A8`。根因是测试在首次 prepare 前注入 `WriteTemp`，故障被准备事务提前消费，未形成预期 recovery record。测试改为沿用 P7.4/P7.5 的“先预准备、后让取消持久化失败”场景；产品实现无回退或放宽。

## 7. 改动—回归与静态门禁

- 新增 `ThrownWeaponProductSession` 路径映射，要求 Session、P7.5 controller、P7.4 command、P7.3 Host、World delivery、P7.1 item adapter、Run coordinator、Items、WorldGameplay、CombatRuntime 及 legacy `ItemUseAndArmor` 共十一组；
- `REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=3`；
- mapping self-test：`24/24 PASS`；反向测试证明缺少 legacy combat snapshot evidence 时必须失败关闭；
- boundary scan：无 `EKeys`、`UInputAction`、`ApplyDamage`、`demo_mapItemSubsystem`、RNG、直接 durable Run consumption 或 `CurrentVitality` 写入；
- 工作区与最终 staged `git diff --check`：native exit `0`。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor integration：Failed，`OtherCompilationError`，native exit `6`，`10.04s`；原因是 Session 恢复调用漏传 P7.5 Router 参数；
- 修正后 Editor：Succeeded，native exit `0`，`5.52s`；
- 最终测试修正后 Editor：Succeeded，native exit `0`，`5.74s`；
- Game Development：Succeeded，native exit `0`，`16.76s`；
- Editor DLL UTC：`2026-08-29T12:39:59Z`；Game executable UTC：`2026-08-29T12:42:18Z`。

首次失败构建保存在 `P7.6_EditorBuild_FirstAttempt.log`，SHA-256 为 `DDFFBB649D29A407D703B6801EF66DAD87C503F99EE5B58419BD8BBCB4EEC67D`，未描述为环境或内存故障。

## 9. 修改范围与 P/F 边界

生产改动只新增一个 device-independent product session；测试新增四项；流程改动只扩展 changed-file regression map 及其正反 self-test。未修改 schema、Build.cs、GameplayTags 配置、Content、输入、UI、GameMode、Profile、CodeB 或既有 authority 实现。长期未跟踪的 0.0.9B 与用户文件保持未跟踪且未 stage。

本轮只执行 P 阶段源码、静态检查、无头 Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一阶段与 GitHub

P7.7 建议建立 source-Actor 生命周期适配器：在产品 Run 开始/结束时创建和关闭本轮 session，把已有产品热栏选择事件转换为 device-independent intent；仍不接具体键位、Widget、动画或视觉表现。若产品层尚无稳定选择事件，应先冻结事件 owner 与 SelectionId 生命周期，不在 Session 内新增 Tick/polling。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-6-thrown-weapon-product-session/Docs/Report/Dev.D.UE.0.0.10.P7.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-6-thrown-weapon-product-session/Docs/Log/Dev.D.UE.0.0.10.P7.6.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-6-thrown-weapon-product-session>
