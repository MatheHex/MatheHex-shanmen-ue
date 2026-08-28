# Dev.D.UE.0.0.10.P6.5.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.5.r0`；
- 基线提交：`ed38fc324fe8a52fa27913f6ec575de27cf09134`（P6.4）；
- 分支：`agent/0.0.10-p6-5-controlled-weapon-run-host`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.4 已把一份 P6.1 Prepared evidence、一把真实物理飞剑、一个 P6.2 Session 和 P6.3 World delivery 收进单 Controller，但无法表达规划基础中的多飞剑控制与区域攻击：调用方若自行维护 Controller 数组，会把 item 唯一性、Run 绑定、遍历顺序和 Run 终止分散到未冻结的 PlayerController / presentation 代码中。

既有 `Ademo_mapSkillProjectile` 是 fire-and-forget projectile，复用它会丢失 exact deployed item、持续命令、独立 contact window 与 Recall 语义。P6.5 因而新增纯组合 Run Host，不修改 legacy projectile，也不提前建立第二套 Actor 系统。

## 实现记录

### Exact item ownership

Host 使用 `TMap<FGuid, Fdemo_mapShanmenControlledWeaponProductController>` 保存每个 exact item 的 P6.4 Controller。首次 attach 复用 P6.4 全部 source / Actor / Session 校验，并冻结 `RunId`、`SourceEntityId` 与 source Actor；后续 attach 必须匹配相同 Coordinator 和注册身份。

Attach 在提交前拒绝重复 `ItemInstanceId`、`ActivationId`、weapon Actor 或 collision root。所有检查和新 Controller 启动成功后才把 candidate Host 写回，失败不留下半绑定对象。

### Deterministic multi-weapon routing

所有批量操作按 GUID digits 的稳定升序执行，不依赖 `TMap` 顺序、attach 顺序或 UObject 地址。Launch、Redirect、contact window、sweep / overlap、Recall、Interrupt 与 removal 均先按 item 找到唯一 Controller；不同飞剑不共享 command sequence、hit ordinal 或 impact ledger。

`TryAdvanceDirectedInOrder` 先检查 Host、DeltaSeconds 和每个 Directed Controller 的冻结 `MaximumStepSeconds`，再按稳定顺序调用 P6.4 swept movement。无效时间片在世界写入前整体失败。合法批次的真实世界移动不可回滚，因此 batch receipt 明确记录每项 attempted / advanced / blocking hit，而不伪造跨 Actor 事务。

### Contact and Run termination

Host 的 sweep / overlap 入口使用统一 `CoordinatorMatches` 核验 ready 状态、Run、player entity 以及 source Actor 在 Entity Registry 中的解析结果。通过后仍交给 P6.4 Controller -> P6.3 Adapter -> Coordinator，不复制 candidate、impact 或 vitality 逻辑。

`TryInterruptAll` 先复制 Host，在副本上按稳定顺序中断所有 active item；任一中断或 receipt 不合法则丢弃副本。全部成功后一次替换 Host。已 terminal item 保持原终态，可随后按 item 独立 retire；最后一项移除时 Host 清空 Run binding。

### Deliberately unfrozen content

本轮没有新增飞剑 Actor class、spawn、InputAction、键位、编队、数量上限、默认速度、转向模型、碰撞 cadence 或正式 item definition。测试中的两把/三把飞剑、`400 uu/s` 与 `0.5s` 都是显式注入的测试输入，不进入产品默认值。

## 自动化覆盖

- 反序 attach 后仍按 stable ItemInstanceId 顺序返回和移动；
- 两把 physical Actor 使用独立 Launch direction 和 movement receipt；
- oversized batch 在任一 Actor 移动前拒绝；
- 两把飞剑分别用 sweep / overlap 命中同一目标，并保留不同 item / impact identity；
- canonical vitality 变化等于两个 newly committed damage 之和；
- 一把飞剑的重复 callback 不改变另一把 Session；
- 一项 Recall + Completed 不结束其它 active item；
- host-wide interrupt 只处理中断时仍 active 的 item，并关闭其 contact window；
- item、Actor / collision root、activation 重复绑定均失败关闭；
- terminal item 独立 removal 与最后一项后的 empty Host reset。

## 首次验证

首次 Editor integration build：`5/5` actions，`Result: Succeeded`，native exit `0`，`11.35s`。

首次 focused 日志 `p65_run_host_first.log`：`3/3` Success、`0` Fail、queue empty、native exit `0`、SHA-256 `DA6B233876EC1DBE2C5AE60AF108EFAA6E1D8E67251AB64044382DF7EC01E054`。

首次编译前静态复审发现 sweep 路由使用空 Prepared 作为 Coordinator-only 哨兵。该临时实现未进入构建；新增独立 `CoordinatorMatches` 后，sweep / overlap 共用完整 Run/source Registry 验证，attach 的 `BindingMatches` 再附加 Prepared 校验。无测试或构建失败。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p65_run_host_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 3 | 0 | 0 | `89A63080A4E011A8DF01B73210FD35C49374076863221473E924B5BBC75D35E0` |
| `p65_controller_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponController` | 3 | 0 | 0 | `52AA89850B65663A19DC86FC0DCCC90290C3EF80256A439A4B3B73318DCFC005` |
| `p65_world_delivery_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `4F0628F11423CD83F852952BA638EA7C29AB8890E58F0EB61D8BE2F55F7D4B82` |
| `p65_session_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `A54DD52EC44368605DBC95A11DC7E215156032F6F0FE3806B597D066B1A450F3` |
| `p65_adapter_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `AF7BEFA22BCA9EE60725773AEA6268C8046096D6CADFB465659B17EC3FDC79FB` |
| `p65_coordinator_final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `753FE56F6A73F4A2CD72C5915BB4A1935C4F80EF02472D80FF35CAFB85C04660` |
| `p65_items_final.log` | `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `A45387B2ED7612CE88F313527217EEF9D6D4CC9EF311933C12D59A016D89E3ED` |
| `p65_world_final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `26E01BA553B74B71B8A0AEA52069155704DBB294B37FA2F2FF23668C647A6D38` |
| `p65_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `C294D1484C8C38766A81C1FDA3900EA82359FD0D3BFD0C4D87E0DBCE807934D4` |
| `p65_full_final.log` | `Shanmen.0_0_10` | 146 | 0 | 0 | `785D9E2AFE490E356620EB44C3E6981F5B14235CD272B50912B72BC32F5C615A` |

十条最终日志均存在唯一 RunTests command、至少一个 success、fail 0、queue empty，且 Fatal / unhandled / ensure 为 0。全量唯一计数 `146/146`。

## Changed-file gate

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=9 Logs=10
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=p65_full_final.log,p65_runtime_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Items Evidence=p65_full_final.log,p65_items_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.CombatRunCoordinator Evidence=p65_coordinator_final.log,p65_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponAdapter Evidence=p65_adapter_final.log,p65_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponController Evidence=p65_controller_final.log,p65_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponRunHost Evidence=p65_full_final.log,p65_run_host_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponSession Evidence=p65_full_final.log,p65_session_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery Evidence=p65_full_final.log,p65_world_delivery_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.WorldGameplay Evidence=p65_full_final.log,p65_world_final.log
```

Regression coverage self-test 新增 Run Host 的直接 pass / fail 路由检查，最终 `10/10 PASS`。

## 构建

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `5/5` actions；
- `Result: Succeeded`；
- native exit `0`；
- `11.35s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `4/4` actions；
- `Result: Succeeded`；
- native exit `0`；
- `22.66s`；
- 生成 `Binaries/Win64/demo_map.exe`，未启动。

## 静态与兼容性

- regression JSON parse：PASS；
- 新宿主生产文件中的 `ApplyDamage` / `TakeDamage` / `SpawnActor` / input / inventory / resource / RNG / `UWorld`：0；
- `git diff --check`：native exit `0`；
- 既有生产源码、模块契约、item definition、Profile schema 与存档格式未改；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

P6.6 应在正式产品 owner 的生命周期位置明确后，将 Host attach / unified interrupt 接到真实部署与 Run teardown。仍应由外层注入输入和内容，不应让 Host 生成飞剑、选择默认数量或复制 inventory / vitality authority。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-5-controlled-weapon-run-host/Docs/Report/Dev.D.UE.0.0.10.P6.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-5-controlled-weapon-run-host/Docs/Log/Dev.D.UE.0.0.10.P6.5.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-5-controlled-weapon-run-host>
