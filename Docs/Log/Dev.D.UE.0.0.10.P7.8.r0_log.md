# Dev.D.UE.0.0.10.P7.8.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.8.r0`；
- 基线：`4924d918eb513d5e817e2463ae9bd564051d74f1`（P7.7）；
- 分支：`agent/0.0.10-p7-8-thrown-weapon-lifecycle`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

P7.7 已建立第一件真实暗器商品。P7.8 关闭下一处产品缺口：把 P7.6 product session 绑定到真实 GameMode combat Run 生命周期，并建立 device-independent hotbar intent 路由，不提前混入键位、Widget、动画或视觉内容。

## 设计证据

静态追踪确认：

1. `Fdemo_mapCombatRunCoordinator::TryBeginRun` 已在 ready Shanmen authority 下调用 `TryGetActiveRunCorrelation`；
2. correlation 由 prepared batch + lifecycle durable receipts 重建，已含 exact hotbar item identities；
3. P7.6 Session 已拥有 P7.3 Host、P7.4 Router、P7.5 Controller；
4. 现有 `RequestUseBoundQuickSlot` 只有 slot number，不含稳定 selection identity、origin 或 aim；
5. GameMode 已是 CombatRunCoordinator 与 controlled-weapon lifecycle 的唯一 Run owner。

因此没有新增 Run subsystem，也没有把设备输入塞入 Session。新生命周期在 begin 时再次只读重建 correlation，并通过 coordinator registry 核验 source Actor。

## 实现过程

### Product lifecycle

新增：

- `demo_mapShanmenThrownWeaponProductLifecycle.h`；
- `demo_mapShanmenThrownWeaponProductLifecycle.cpp`；
- `demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`。

Lifecycle 只保存 P7.6 Session 与 authority 弱引用。其 `IsValid` 要求 active session 与有效 authority 同时存在，empty session 与空 authority 同时存在。authority 生命周期消失时不允许静默丢弃捕获状态。

### 内容策略

`TryCaptureTrainingThrowingKnifeConfig` 先查询 P7.7 canonical definition 并检查 typed `ThrownWeapon` semantic，再冻结直线策略 `12 + 0.3 * TechniquePower`、速度 `900`、physical slash、living target、player source 与 self reject。

### GameMode

`TryActivateCombatRun` 在 coordinator/entity 成立后绑定 lifecycle。不存在 ready Shanmen authority 时保持旧流程；存在但 correlation/source 不匹配时整个 product Run 失败关闭并释放 coordinator。

新增 route/recovery/interrupt/range-expire 四个显式方法。`ReleaseCombatProductRun` 先处理暗器，再处理 controlled weapon，最后结束 coordinator。这样不会在 projectile 仍 in-flight 或 durable cancellation recovery 未解决时先销毁 Run identity。

## 新增测试

`Shanmen.0_0_10.Product.ThrownWeaponProductLifecycle` 包含：

1. `ContentAndBinding`：内容常量、durable correlation、registry source、begin replay 与 source takeover rejection；
2. `HotbarRouteReplayAndEnd`：真实飞刀、authority +2 revision、exact replay snapshot、SelectionId conflict、automatic interrupt-before-end；
3. `FailClosedBoundaries`：foreign coordinator 和 inactive lifecycle 均 mutation-free。

## 首次失败时间线

### Editor integration

首次单并发 Editor build：

```text
demo_mapGameMode.cpp(1069): C2039: IsReady is not a member of Udemo_mapShanmenItemAuthoritySubsystem
Result: Failed (OtherCompilationError)
Native exit: 1
Total: 111.63s
```

修复为现有枚举契约：

```cpp
Authority->GetLifecycleState()
    == Edemo_mapShanmenItemAuthorityLifecycleState::Ready
```

之后 Editor integration 成功。

### Targeted automation

首次 targeted：`1 Success / 2 Fail`，native exit `0`。失败 SHA-256：

```text
B48A5E9A5226E50B33076279179068428D4E6F1D83F7827DBD25FDA6AA05CF8F
```

产品 route 已执行，但新测试把 complete-stack preparation 后的 active-Run reservation 当成普通 Quantity `3 -> 2` 读取。该层的正确可审计证据是 durable authority revision 与 snapshot。测试改为：首次 launch `revision +2`，replay/foreign/inactive 的完整 snapshot 不变。未修改产品逻辑或放宽业务契约。

## 最终验证

Automation 命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P7.8_Targeted.log` | `Shanmen.0_0_10.Product.ThrownWeaponProductLifecycle` | 3 | 0 | 0 | `87AA534F7B2B00CD4D479813002F9FEFD52605CECD00F6C320C5DBDA1626FB81` |
| `P7.8_Full.log` | `Shanmen.0_0_10` | 204 | 0 | 0 | `775EBECF6665AA9E033F4001558E714F1DA137591D1C5E1C6DB061045C00BD38` |
| `P7.8_ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `DB23FAC76925A45ABA73AA36D5AA587A527F1220F5CAF23DC9A3888E9618697C` |

所有最终日志都有一个 RunTests、queue-empty、Fail `0`，fatal/unhandled/ensure marker 为 `0`。

## Changed-file regression gate

GameMode 映射由泛化 full suite 扩展为其实际拥有的 11 个产品 authority seams；新增 lifecycle 规则要求完整 thrown pipeline 与 legacy combat snapshot suite。

```text
REGRESSION_MAP_JSON: PASS
SELF_TEST: PASS 28/28
REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=13 Logs=3
```

反向 self-test 证明仅提供 `Shanmen.0_0_10` 而缺少 `demo_map.ItemUseAndArmor` 时，lifecycle 改动必须失败关闭。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | `OtherCompilationError` | 1 | 111.63s | 首次独立文件未保留 |
| Editor final | Succeeded | 0 | 5.86s | `DA9F34A745E5F7A3D068138F4D90C02E403901E4B7452B42A6D59139DAA49EF3` |
| Game final | Succeeded | 0 | 104.86s | `97FE173873901B10261DB898A7C823A3CAE7BC403A80CB742B2CF061E9BC2C54` |

Game executable：`351897088` bytes，UTC `2026-08-29T13:56:17Z`。

## 静态与边界

- regression JSON parse：PASS；
- boundary scan：无 Tick、timer、input binding、legacy item subsystem、ApplyDamage、FRand/RandRange；
- 工作区 `git diff --check`：exit `0`；
- 未修改 schema、Build.cs、GameplayTags、Content 或既有输入路径；
- 长期用户未跟踪文件未修改、未 stage。

## P/F 边界与下一步

本轮仅做 P 阶段代码、静态检查、无头 Automation、Editor/Game Development build。未启动 Editor UI、PIE、Standalone、产品 executable、Smoke、Cook 或 Package。

P7.9 应在 GameMode 外建立唯一 input adapter：从 frozen hotbar exact item semantic 分流，暗器才采样 trajectory 并生成稳定 SelectionId；普通 consumable 保留原路径。视觉、动画和 F 阶段真实输入验证继续后置。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-8-thrown-weapon-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P7.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-8-thrown-weapon-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P7.8.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-8-thrown-weapon-lifecycle>
