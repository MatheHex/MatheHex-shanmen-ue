# Dev.D.UE.0.0.10.P20.61.r0 Report

## 1. 结论

P20.61 已把 P20.60 的 MainHUD Arc preview renderer endpoint 接入真实运行时生命周期：`Ademo_mapGameMode` 现在独占一个 runtime binding，`Ademo_mapHUD` 在 BeginPlay/EndPlay 事件上显式 attach/detach，active Run 在激活与释放边界显式 begin/end。整个接线不依赖 Tick、DrawHUD 轮询、timer、async 或第二套 preview 状态。

runtime binding 复用既有 composition owner、delivery、surface ownership transition、owner handoff 与 recovery 协议。一个永久 fallback surface 保证 HUD 尚未创建、销毁或重建时，owner 的非拥有型 surface 引用始终指向有效对象；可见 preview 跨 HUD 重建时只把同一 authoritative cursor 复原到新物理 surface，再执行 exact `AdoptExact` handoff，不生成第二条逻辑 presentation command。

本阶段通过 headless automation、changed-file regression gate 与 Editor/Game 原生构建证明生命周期和协议接线成立。没有启动 Unreal Editor UI、PIE、Standalone 或产品，也没有执行真实输入和截图，因此不声明玩家可见轨迹已完成真实运行验收。

## 2. 基线、分支与改动范围

- 基线：917e7501d8c4bb1c0f47e649e530716477d31337（P20.60）；
- 分支：agent/0.0.10-p20-61-thrown-weapon-arc-preview-mainhud-runtime-binding；
- 新增 GameMode-owned MainHUD runtime binding 头文件与实现；
- `Ademo_mapGameMode` 接入初始化、Run begin/end、HUD attach/detach 与 preview update；
- `Ademo_mapHUD` 在 BeginPlay/EndPlay 显式注册和解除物理 renderer；
- P20.60 renderer adapter 新增 exact visible rehydrate 与未采纳 cursor rollback；
- ProductLifecycle automation 新增 5 项 runtime lifecycle/recreation 测试；
- changed-file regression mapping 新增 runtime exact rule，并回接 GameMode、HUD 与 renderer adapter 规则；
- mapping self-test 新增 pass 与 focused-only rejection fixtures；
- 新增本 Report 与同名 Development Log；
- 生产代码、测试与流程映射合计 11 个改动路径；本阶段主体 diff 为 1,269 additions / 1 deletion（不含 Report/Log）。

## 3. Runtime Binding 与永久 Fallback

`Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding` 由 GameMode 直接持有且不可复制。初始化时创建一个永久有效的 fallback renderer，并分别记录：

1. 当前已 attach 的 HUD surface（可为空）；
2. active owner 当前绑定的 surface（Run 外为空）；
3. 既有 composition owner 与 handoff/recovery authority；
4. 有界 operation guard，拒绝重入。

Run 开始时优先绑定已 attach HUD；HUD 尚不存在时绑定 fallback。HUD 后到、替换或销毁只触发显式 surface handoff，不修改 RunId、consumer definition、owner cursor 或 product authority。陈旧 HUD 的晚到 detach 通知按身份判定为幂等 no-op，不会把新 HUD 错误解除。

## 4. HUD 与 Combat Run 生命周期接线

`Ademo_mapHUD::BeginPlay` 先初始化自身 renderer physical identity，再向当前 `Ademo_mapGameMode` attach。`EndPlay` 在调用父类之前 detach，确保旧 HUD 对象销毁前 owner 已转移到永久 fallback 或新 HUD。

`Ademo_mapGameMode::TryActivateCombatRun` 在既有 combat products 全部成功绑定后启动 preview runtime；失败时沿既有 `ReleaseCombatProductRun` 路径回滚。stale-state fence 也纳入 runtime active 状态，禁止第二个 Run 静默覆盖。

`ReleaseCombatProductRun` 在 thrown lifecycle 与 coordinator 释放前先结束 preview。若当前状态可见，binding 从 owner 的 immutable choice snapshot 派生一个仅用于本地显示清理的 Arc-target-clear command，使既有 owner/delivery/surface 路径产生 Hide；随后结束 owner Run。调用者持有的 authoritative input choice 不被改写。

## 5. 可见 HUD 重建与 Exact Handoff

新 HUD 在 active visible preview 中创建时没有历史 physical cursor。P20.61 为 renderer adapter 增加两个严格入口：

- `TryRehydrateVisibleForHandoff`：只接受相同 Run、稳定 consumer 与一个有效 visible authoritative state，且目标 surface 必须为空或已持有完全相同 cursor；
- `TryDiscardRehydratedVisibleForHandoff`：只回滚尚未被采纳、且与预期完全匹配的 visible cursor。

rehydrate 是物理 renderer reconstruction，不是逻辑 presentation command，因此不产生 command、delivery receipt 或第二次 product update。完成复原后，既有 surface ownership transition 生成 `AdoptExact` ticket，既有 owner handoff 负责旧 surface retirement 与 owner commit；handoff 失败时使用既有 recovery checkpoint，未进入 recovery 的失败则精确回滚新 surface cursor。

hidden/empty 状态不需要重建 visible cursor，直接使用既有 `BindFresh`。这保持 physical cursor、host cursor 与 owner identity 的单一一致性。

## 6. 输入 Basis 与 Preview/Launch 一致性

`RouteThrownWeaponArcChoiceHotbarInput` 现在用一个局部 frozen basis 包装调用者的 `SampleBasis`：

1. 每次 route 最多采样一次；
2. active preview binding 先用该 exact basis 更新 composition owner；
3. 实际 thrown-weapon launch 继续使用同一冻结值；
4. preview update 失败只记录诊断，不阻断既有 launch authority。

因此 preview 与实际命令不可能因同一次输入中重复采样位置/方向而发生漂移。runtime binding 只编排 presentation，不拥有物品、伤害、launch、库存或战斗结算权威。

## 7. 测试覆盖

新增 5 项 focused automation：

1. InitializationAndDirectHUDRunBinding；
2. FallbackAndFreshLateAttach；
3. VisibleHUDRecreationAndDetach；
4. ExactUpdateReplay；
5. LocalClearAndLifecycleFences。

覆盖 direct HUD/fallback Run begin、late attach、visible HUD replacement、旧 HUD detach no-op、exact cursor adoption、一次 NoOp 后的 ledger replay、local Hide teardown、错误 RunId、重入与生命周期 fence。

| Log | Group | Success/Fail | Native exit | Seconds | Bytes | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| P20.61_Focused.log | Product...MainHUDRuntimeBinding | 5/0 | 0 | 112.818 | 269,875 | CD23FCDD414DDD53A3DBA99886952B28834E9D742BA3ACF829B71841E849CC68 |
| P20.61_Full.log | Shanmen.0_0_10 | 1208/0 | 0 | 4694.209 | 1,808,554 | 36B92B61F7128097426FADB79067E6A368048A15CA23BB10B16138745C7DD5F1 |
| P20.61_EnemySkill.log | demo_map.EnemySkillFramework | 44/0 | 0 | 38.067 | 300,067 | 841BD264194FAA664D9B02277BA3ECE40769F2E3EE9CBE5B9151FAE2804C55DB |
| P20.61_InputRestore.log | demo_map.InputRestore | 101/0 | 0 | 18.458 | 388,230 | 4152B788EF562D5085D3EF2CBFAA1EF9657FC2FBE7273669E468AB84E7FE7E06 |
| P20.61_ItemUseAndArmor.log | demo_map.ItemUseAndArmor | 46/0 | 0 | 16.321 | 303,792 | AD894FEEB6C7C74082E0BF0C84935E2F23F1D8941B2D4466BF09A0DEB4E9470A |
| P20.61_V2RangedCompatibility.log | demo_map.V2RangedCompatibility | 22/0 | 0 | 15.812 | 279,447 | B252861B7C7934C1AF215B8C31DB2B51862438AE397FA28BB24C94A40840288E |
| P20.61_V3Attributes.log | demo_map.V3.Attributes | 4/0 | 0 | 14.731 | 258,290 | 5F71E1197AC49A36073F0B36B35F8D56D9B17EB501B404CDEC61AD829AAF2386 |

七次正式 invocation 累计 1430/0（focused 与 full 有意重叠）；7/7 均包含 `TEST COMPLETE. EXIT CODE: 0`。selected automation failure、Fatal、Unhandled 与 `Ensure condition failed` 均为 0。

首轮 focused 为 4/1，失败项仅是测试错误地把第一次相同输入期望为 ledger replay；既有协议实际先生成一次不触碰 surface 的 NoOp，下一次 exact application 才命中 replay。只修正测试顺序与断言，产品代码未为测试改变。首失败日志保留为 `Dev.D.UE.0.0.10.P20.61.r0_focused-backup-2026.09.06-23.50.31.log`：119.343 s / 270,379 bytes / SHA-256 DE7F3FB228DC7B93438E1C09E087D911A7B520FC8ACEE21706162DF91D58244F。

UE 5.8 每次进程仍在 selected queue 前输出 13 条引擎自身 generic `LogAutomationTest: Error: Condition failed`；它们没有形成 controller failure、Ensure/Fatal 或非零退出码。正式结果按 selected test record 与原生终止标记判定。

## 8. Changed-file Regression、静态边界与构建

- mapping JSON：232 rules / parse PASS；
- mapping self-test：415/415；
- final changed-file gate：PASS，Changed=11 / Rules=5 / Required=75 / Logs=7；
- runtime focused-only fixture：按预期拒绝，不能替代 lifecycle/handoff/legacy evidence；
- `git diff --check`：PASS，0 whitespace errors；
- runtime binding：622 physical lines（h 125 / cpp 497）；
- runtime binding 中 Tick/DrawHUD/timer/async/save/file access/GetWorld/AActor/UObject/RNG：0；
- runtime binding 中 TODO/FIXME/HACK：0；
- final Editor：Succeeded / native 0 / 0 actions / 1.62 s / up to date；
- final Game：Succeeded / native 0 / 33 actions / 97.64 s；
- Editor DLL：18,330,112 bytes / SHA-256 32114BC0A3AC85CE54FC46B0359712FA7241C644058A0010FE819D7E3269855A；
- Game EXE：359,207,424 bytes / SHA-256 5C20B206B0841C8EA118E29E7AC511A848D3ACB66FDB702C28E317C79EF168A8。

最终 automation 使用 `-NoEOS`，并通过项目 `Saved` 下的临时 startup cvars 文件在 MainFrame 初始化前关闭 Home Screen 联网探测。该文件被忽略且不提交，没有修改 Windows、引擎安装或用户全局设置。三次环境定位期间中止的非正式 full 日志保留在本地，不计入正式测试证据。

## 9. P/F 边界

PASS 范围：GameMode-owned runtime binding、永久 fallback、HUD BeginPlay/EndPlay attach/detach、Run begin/end、single-sampled basis、preview/launch basis 一致、visible rehydrate、exact AdoptExact/BindFresh handoff、旧 HUD retirement、stale detach no-op、local Hide teardown、NoOp/replay separation、focused/full/legacy regressions、changed-file coverage、Editor/Game compilation。

未验证且不声明：真实玩家可见轨迹、实际窗口尺寸/DPI/遮挡、多 viewport/split-screen、本机输入设备、Unreal Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。preview failure 当前按非阻断诊断处理；真实 UI 对颜色、线宽、时序与可读性的验收仍需单独产品运行阶段。

## 10. 下一步与 GitHub

P20.61 已关闭 P20.60 留下的运行时接线缺口。下一阶段建议作为 P20.62 进行受控真实 UI 验收：启动一个授权的最小产品运行，验证 Arc 模式进入/编辑/确认/取消、HUD 重建后可见性、preview 与实际落点一致，并把失败严格归因到 renderer、binding 或 source basis。该阶段不得以截图代替运行证据，也不得在发现视觉问题时建立第二套状态。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-61-thrown-weapon-arc-preview-mainhud-runtime-binding>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-61-thrown-weapon-arc-preview-mainhud-runtime-binding/Docs/Report/Dev.D.UE.0.0.10.P20.61.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-61-thrown-weapon-arc-preview-mainhud-runtime-binding/Docs/Log/Dev.D.UE.0.0.10.P20.61.r0_log.md>
