# Dev.D.UE.0.0.10.P6.4.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.4.r0`；
- 基线提交：`b342bf84dc84f757590c8e3c9608e637a34c39cf`（P6.3）；
- 分支：`agent/0.0.10-p6-4-controlled-weapon-product-controller`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.3 已能把物理飞剑的 sweep / overlap 接触原子交给 canonical vitality，但产品仍需自行同时保存 Session、飞剑 Actor、当前 context、movement direction 与调用顺序。若直接在 PlayerController 中临时拼接，会在正式按键、飞剑 content 与生成规则尚未冻结时固化错误产品假设。

P6.4 因此新增无默认内容的产品 owner：外部系统必须提供 P6.1 Prepared evidence、已注册来源 Actor、真实飞剑 Actor / collision root 与一次 Session 的 movement capture。Controller 只拥有编排和物理 movement，不拥有物品生成、输入选择、命中频率或伤害写入。

## 实现记录

### Product binding

`TryStart` 验证 Coordinator、Run、Prepared、movement、Actor ownership 与 World Entity Registry。来源必须解析为当前 `PlayerEntityId`，Prepared action 的 `SourceEntityId` 必须与之完全一致；来源 Actor 不能复用为飞剑 Actor，碰撞根必须属于飞剑且为其实际根组件。

绑定成功后才启动 P6.2 Session。Controller 保存 weak Actor references、Run / source identity、冻结 movement capture 与唯一 Session，不复制 inventory、vitality 或 item authority。

### Directed movement

Launch / Redirect 分别封装 P6.2 对应命令，保留 exact replay 与单调 sequence。`TryAdvanceDirected` 只在 Directed 状态接受有限、正值且不超过 `MaximumStepSeconds` 的时间片，再以冻结 `DirectedSpeed` 对同一个 Actor 调用 sweep-enabled `SetActorLocation`。

Movement receipt 记录 ActivationId、SourceItemInstanceId、最新 accepted command sequence、方向、起点、请求终点、实际终点、DeltaSeconds、是否移动与 blocking hit。若参数无效或世界拒绝且没有 blocking hit，receipt 清空并失败关闭。

### Contact ownership

Controller 保存唯一 active context。只有 Directed 状态可开窗；重复开窗拒绝。Sweep / overlap 不在 Controller 中重写候选或伤害逻辑，而是携带该 context 调用 P6.3 adapter。只有 `IsDelivered()` 后 Session 副本才写回 Controller。

Recall 要求没有开放窗口，并仍调用 P6.2 的 Recall -> Recovery -> Completed 原子路径。Interrupt 关闭 Session emission 后同步清空 Controller context。

### Deliberately unfrozen content

本轮没有新增飞剑 Actor class、spawn、InputAction、键位、默认速度、转向速率、轨迹模式、碰撞 cadence 或正式 item definition。测试中的 `400 uu/s` 与 `0.5s` 只是显式注入的非零验证值，不进入产品默认状态。

### Regression map

新增 `ControlledWeaponProductController` 规则。新文件强制要求 Controller、P6.3 World delivery、P6.2 Session、P6.1 Adapter、Coordinator、Items、WorldGameplay、CombatRuntime 共 8 个组；完整父组作为额外证据，不代替映射检查。

## 自动化覆盖

- exact deployed item 与 physical Actor / collision root 绑定；
- source Registry identity mismatch 与 source/weapon alias 拒绝；
- Orbiting 禁止移动，Launch / Redirect 后按冻结速度移动；
- oversized DeltaSeconds 在 transform 写入前拒绝；
- movement receipt 的 item、activation、sequence、direction 与 actual transform；
- Controller-owned contact window 和稳定 ordinal；
- sweep / overlap canonical vitality delivery；
- duplicate callback 幂等；
- open-window Recall 竞争拒绝；
- completion / interrupt 后无窗口、无进一步 movement/control。

## 首次验证

首次 Editor integration build：`5/5` actions，`Result: Succeeded`，native exit `0`，`15.34s`。

首次 focused 日志 `p64_controlled_weapon_controller_first.log`：`3/3` Success、`0` Fail、queue empty、native exit `0`、SHA-256 `F13AF32BD27C5C7B94E33908826FD326CA33550F9250371EE195BC7087E5E364`。

随后静态复审发现产品 `TryLaunch` 额外要求 Orbiting，导致首次 Launch 已进入 Directed 后，同一 sequence / direction 的 exact replay 在到达 P6.2 ledger 前就被拒绝。删除该重复前置判断，让 P6.2 继续决定新命令合法性与 exact replay；`BindingAndMotion` 增加同一 CommandId、sequence 不前移的产品级断言。修正后重编译并重新生成全部最终日志。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p64_controller_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponController` | 3 | 0 | 0 | `03A5C5440A2D04DB296E17AD5BFC526E180E2776DC851BBEE2001694E81AA7B8` |
| `p64_world_delivery_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `CABC43241BE70DCFE97B34F5D4248EAF3E826F61CCE137540581546F70E42BFF` |
| `p64_session_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `4D94F6EC3C987B16ECAA9C1EE9436AB4B5F9F53376BFF2942440BAE62020F5CB` |
| `p64_adapter_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `184CA25917892C77ACE9C1003E974A025A1BEE823173D3C598409EB28157EDDE` |
| `p64_coordinator_final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `13D3261CE5B5395BBE32309E7E252C81C3BF87079043ACC235F5FB985FBBCBA5` |
| `p64_items_final.log` | `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `859FBCCD78227EE6748BACEEDBE0B24035810EB5AD5DB9E08E4CE70F90F64023` |
| `p64_world_final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `A7CDEB30526078E2EC6AFD8B1E08B32A5FC24CE54B29F8EED9ACD55CA188CF8D` |
| `p64_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `D63982A05B16AD25EE12C2CAE449653D12DD91F3847DB761688167FC3D79AE86` |
| `p64_full_final.log` | `Shanmen.0_0_10` | 143 | 0 | 0 | `29CDC11D0CEBFE2F050447CD91FC60C3170EB443B7CD688D42ECC8DE89759CA3` |

九条最终日志都存在唯一 RunTests command、至少一个 success、fail 0、queue empty，且 Fatal / unhandled / handled ensure 为 0。全量唯一计数 `143/143`。

## Changed-file gate

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=1 Required=8 Logs=9
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=p64_full_final.log,p64_runtime_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Items Evidence=p64_full_final.log,p64_items_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.CombatRunCoordinator Evidence=p64_coordinator_final.log,p64_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponAdapter Evidence=p64_adapter_final.log,p64_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponController Evidence=p64_controller_final.log,p64_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponSession Evidence=p64_full_final.log,p64_session_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery Evidence=p64_full_final.log,p64_world_delivery_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.WorldGameplay Evidence=p64_full_final.log,p64_world_final.log
```

Regression coverage self-test：`8/8 PASS`。

## 构建

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `5/5` actions；
- `Result: Succeeded`；
- native exit `0`；
- `8.87s`（重放修正后的最终增量构建）。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `4/4` actions；
- `Result: Succeeded`；
- native exit `0`；
- `14.93s`（重放修正后的最终增量构建）；
- 生成 `Binaries/Win64/demo_map.exe`，未启动。

## 静态与兼容性

- `git diff --cached --check`：native exit `0`；
- regression JSON parse：PASS；
- 新生产文件中的 `ApplyDamage` / `TakeDamage` / `SpawnActor` / input binding / direct inventory-resource mutation：0；
- 既有生产源码、模块契约、item definition、Profile schema 与存档格式未改；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

P6.5 可在正式输入语义与飞剑 content 冻结后，把既有 PlayerController / presentation spawn owner 接到本 Controller。该层只应提供 Prepared evidence、真实 Actor 与控制采样；不得重新实现 Session、World adapter、inventory 或 vitality。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-4-controlled-weapon-product-controller/Docs/Report/Dev.D.UE.0.0.10.P6.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-4-controlled-weapon-product-controller/Docs/Log/Dev.D.UE.0.0.10.P6.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-4-controlled-weapon-product-controller>
