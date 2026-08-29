# Dev.D.UE.0.0.10.P7.5.r0 Report

## 1. 结论

P7.5 已完成一次性直线暗器的 product controller/capture facade，结论为 **PASS**。

本轮把未来输入层压缩为一个不含键位语义的 `SelectionIntent`：输入只选择 active Run、exact item、origin、canonical aim 与 range。产品边界读取 ShanmenItems 权威中的物品定义与标签，冻结 combat definition/offense，再由 `Fdemo_mapCombatRunCoordinator` 分配 Run-owned deterministic action sequence，最终提交 P7.4 router。一个 `SelectionId` 永久映射到一个 action/command；exact retry 复用原 sequence，payload conflict 不消耗新 sequence，也不触发库存、World 或 Actor 副作用。

输入绑定、热栏 UI、角色属性实时采样策略、动画、视觉资产、弧线、追踪、转向与召回仍未接入。

## 2. 产品选择与冻结契约

新增 `Fdemo_mapShanmenThrownWeaponSelectionIntent`：

- 保存稳定 `SelectionId`、active `RunId`、exact `SourceItemInstanceId`、origin、canonical aim direction 与 maximum distance；
- capture 时拒绝无效 GUID、非有限向量、零方向和非正 range，并统一方向长度；
- `Matches` 比较完整选择 payload，同一 `SelectionId` 不能替换 item、方向或其它执行语义；
- 不含 `EKeys`、`UInputAction`、World、Actor、库存事务或产品数值。

新增 `Fdemo_mapShanmenThrownWeaponProductCapture`，独立冻结 thrown-weapon definition、technique-power offense 与产品 source tags。controller 再从 item authority snapshot 读取 exact item 的 owner/scope、definition、Quantity capability 与 `Item.Weapon.Thrown` 标签；调用方不能伪造物品类型标签。

## 3. Run-owned action identity

`Fdemo_mapCombatRunCoordinator` 新增 `TryReservePlayerThrownWeaponAction` 与 `Fdemo_mapPlayerThrownWeaponActionReservation`：

- sequence 从 `1` 开始，严格由当前 combat Run 单调分配；
- `ActivationId` 由 `RunId + PlayerEntityId + canonical thrown action definition + sequence` 确定性派生；
- reservation 自校验上述派生关系，并绑定 exact source item；
- Run end/reset 将 sequence 恢复为 `1`；无 ready Run、无效 item 或序列耗尽均失败关闭。

controller 只在选择、Run correlation、item authority 与产品 capture 全部有效后请求 sequence。已捕获 selection 的 transient retry 直接使用原 sequence，不会再次调用 coordinator；冲突 selection 和未纳入 prepared Run inventory 的 item 在 reservation 前失败。

## 4. 提交、重放与恢复

`Fdemo_mapShanmenThrownWeaponProductController::TrySubmit` 固定执行：

1. 验证 ready coordinator、selection/product、active Run correlation 与 controller Run ownership；
2. exact replay/conflict 优先于新权威读取；
3. 在 Game Thread 捕获 ready item authority snapshot，核对生命周期 revision、owner、scope、exact definition/capability/tag；
4. 向 coordinator 预留 action sequence，并构造不可变 action snapshot；
5. 合并产品 source tags、`Source.Player` 与 authority item tags；
6. 捕获 P7.4 command，保存 `SelectionId -> sequence/action/command`，再交给 router。

HostBusy 等 transient rejection 不丢失 selection：重试时复用同一 command 和 sequence。`TryRecoverCancellation` 只接收原 selection，委托 P7.4 的 cancellation recovery，永不重启 launch。

controller 的 `IsValid` 同时校验 Run、selection、product、command、exact item、方向/range、source tags、deterministic ActivationId 及 sequence/ActivationId 唯一性。

## 5. Quantity 权威修正与首次失败

首次定向自动化发现 1 项成功、3 项失败；测试进程原生退出码虽为 `0`，但 ControllerResults 明确失败，因此该轮按失败保留：

- 日志：`P7.5_Targeted_FirstFailure.log`；
- Success/Fail：`1/3`；
- SHA-256：`01AAA1709DFDF0607083807D107F60535DD6286293DD1068FCB4D618CCE6E0B2`。

根因是 controller 错误要求 authority snapshot 中 loose `Item.Quantity > 0`。active Run 开始后，Quantity 真值已进入 committed reservation，loose item 可以合法为 `0`；可用量与 pending-intent 排除属于 P7.1 的唯一权威。修复后 controller 只校验 exact item 的 owner/scope、definition、capability/tag，把 availability 判定完整委托给 P7.1。未引入第二套数量计算。

## 6. 自动化证据

最终 `-Unattended -NullRHI` 自动化全部通过；两份 canonical 日志均只有一个实际 RunTests、一个 queue-empty、Fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.ThrownWeaponProductController` | 4 | 0 | `52DFF287C6ACC76BCC31E59034DC240592A225DF03BF753555D8C58A382AE393` |
| `Shanmen.0_0_10` | 196 | 0 | `D4919EEDC2CB788898610A77ADC11E1C0856BF21B9B494C04316E5F960790724` |

完整 suite 从 P7.4 的 192 增至 196。四项新增测试覆盖 capture contract、真实 submit/exact replay/payload conflict、HostBusy transient retry 的 sequence 稳定性，以及 selection-only cancellation recovery。额外断言证明未 prepared exact item 在 action reservation 前拒绝。

## 7. 改动—回归与静态门禁

- 新增 `ThrownWeaponProductController` 精确路径映射，要求 product controller、P7.4 command、RunHost、World delivery、P7.1 item adapter、coordinator、Items、WorldGameplay 与 CombatRuntime 九组；
- `REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=9 Logs=2`；
- mapping self-test：`22/22 PASS`，正向证明 full suite 覆盖全部 seam，反向证明 coordinator-only 日志不能伪装 product coverage；
- 工作区 `git diff --check`：native exit `0`；
- 新 controller 无 `EKeys`、`UInputAction`、`ApplyDamage`、legacy `demo_mapItemSubsystem`、RNG、直接 vitality commit 或 `ConsumePreparedRunItemDurable`；
- effectful authority 仍只经 P7.1/P7.4/P7.3 既有边界；
- 未修改 schema、Build.cs、GameplayTags 配置、Content、GameMode、Profile 或 CodeB。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor integration：`43/43`，Succeeded，native exit `0`，`136.97s`；
- 首次 Game integration：`42/42`，Succeeded，native exit `0`，`129.25s`；
- 最终 source-tag invariant 后 Editor 增量：`4/4`，Succeeded，native exit `0`，`11.53s`；
- 最终 Game 增量：`3/3`，Succeeded，native exit `0`，`11.95s`；
- Editor product DLL UTC：`2026-08-29T12:16:43Z`；
- Game executable UTC：`2026-08-29T12:18:07Z`。

构建没有源码失败。首次自动化失败已按真实 ControllerResults 记录，修复后重新执行定向与完整 suite。

## 9. 修改范围与 P/F 边界

本轮生产范围为一个 product controller/capture facade 与 coordinator 的 thrown-action identity reservation；测试新增四项 product contract；流程范围只增加该路径的回归映射与正反 self-test。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文件保持未跟踪且未 stage。

只执行 P 阶段源码、静态检查、无头 Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

P7.6 建议建立 active-Run thrown-weapon selection source/session owner：从产品热栏/装备选择状态取得 exact item，并从角色战斗状态冻结 ProductCapture 后调用本轮 controller。仍保持设备无关，不在 P 阶段接键位、UI 或视觉表现。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-5-thrown-weapon-product-controller/Docs/Report/Dev.D.UE.0.0.10.P7.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-5-thrown-weapon-product-controller/Docs/Log/Dev.D.UE.0.0.10.P7.5.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-5-thrown-weapon-product-controller>
