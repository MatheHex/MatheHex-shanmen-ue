# Dev.D.UE.0.0.10.P20.61.r0 Development Log

## 基线与目标

- base：917e7501d8c4bb1c0f47e649e530716477d31337；
- branch：agent/0.0.10-p20-61-thrown-weapon-arc-preview-mainhud-runtime-binding；
- 目标：把既有 active Run Arc-preview composition owner 显式绑定到 MainHUD renderer，并在 HUD 创建、替换与销毁时沿既有 handoff 协议迁移；
- 权限边界：runtime binding 只编排 presentation surface，不拥有物品、launch、伤害、库存、存档或战斗结算；
- 运行边界：本轮只做 headless automation 与原生构建，不启动 Editor UI、PIE、Standalone 或产品。

## 设计判断

1. P20.60 的 adapter 是真实物理 endpoint，但没有 Run owner 接线，不能靠 DrawHUD/Tick 轮询上游补齐。
2. composition owner 内部持有非 owning surface 指针，所以 HUD 生命周期之间必须存在一个寿命覆盖 GameMode 的 fallback surface。
3. HUD physical identity 与 stable consumer definition 必须继续分离；replacement 是物理实例变化，不是 consumer 变化。
4. visible HUD recreation 不能重发 Show/Replace，否则会产生第二条逻辑 command/receipt；应复原 exact physical cursor 后使用既有 `AdoptExact` ticket。
5. hidden/empty handoff 不需要复原 renderer state，沿用 `BindFresh`。
6. 新 HUD 必须先 attach 成为当前候选，旧 HUD 的晚到 detach 只能是 no-op，避免 replacement 顺序竞争。
7. 同一次 hotbar route 必须只采样一次 source basis，preview 与 launch 共享冻结值。
8. Run teardown 必须先把可见 preview 经既有 owner 路径清为空，再释放 thrown lifecycle/coordinator；本地清理不能改写 authoritative input choice。

## 实现记录

新增：

- demo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding.h；
- demo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding.cpp。

runtime binding 核心行为：

- non-copyable GameMode-owned object；
- permanent fallback renderer；
- explicit attach/detach/begin/update/end API；
- active owner 的 old/new surface handoff；
- visible state exact rehydrate + `AdoptExact`；
- empty state `BindFresh`；
- handoff recovery checkpoint 复用；
- operation reentry guard；
- stale detach 幂等 no-op；
- canonical segment count = 8；
- 不含 Tick、DrawHUD polling、timer 或 async。

renderer adapter 扩展：

- `TryRehydrateVisibleForHandoff` 只写入 exact valid visible cursor；
- 相同 cursor replay 幂等，冲突 cursor/Run/consumer 失败关闭；
- `TryDiscardRehydratedVisibleForHandoff` 只回滚 exact unadopted cursor；
- 两个入口不生成 presentation command 或 receipt。

GameMode/HUD 接线：

- GameMode BeginPlay 初始化永久 fallback；
- HUD BeginPlay 初始化 physical renderer 后 attach；
- HUD EndPlay 在对象销毁前 detach；
- combat Run activation 最后 begin preview runtime，失败走统一 rollback；
- combat Run release 最先清理并 end preview runtime；
- hotbar Arc route 用同一 frozen basis 更新 preview 和 launch；
- preview 更新拒绝记录诊断但不越权阻断 launch。

## 自动化测试

新增 5 项：

- InitializationAndDirectHUDRunBinding；
- FallbackAndFreshLateAttach；
- VisibleHUDRecreationAndDetach；
- ExactUpdateReplay；
- LocalClearAndLifecycleFences。

首轮 focused：4/1 / native -1。`ExactUpdateReplay` 的测试预期错误地跳过了既有 NoOp 阶段；修正为 Applied → NoOp（adapter called、surface not called）→ ApplicationReplayed（adapter not called），产品实现未修改。首失败日志：119.343 s / 270,379 bytes / SHA-256 DE7F3FB228DC7B93438E1C09E087D911A7B520FC8ACEE21706162DF91D58244F。

正式结果：

- focused：5/0，native 0，112.818 s，269,875 bytes，SHA-256 CD23FCDD414DDD53A3DBA99886952B28834E9D742BA3ACF829B71841E849CC68；
- full Shanmen.0_0_10：1208/0，native 0，4694.209 s，1,808,554 bytes，SHA-256 36B92B61F7128097426FADB79067E6A368048A15CA23BB10B16138745C7DD5F1；
- EnemySkillFramework：44/0，native 0，38.067 s，300,067 bytes，SHA-256 841BD264194FAA664D9B02277BA3ECE40769F2E3EE9CBE5B9151FAE2804C55DB；
- InputRestore：101/0，native 0，18.458 s，388,230 bytes，SHA-256 4152B788EF562D5085D3EF2CBFAA1EF9657FC2FBE7273669E468AB84E7FE7E06；
- ItemUseAndArmor：46/0，native 0，16.321 s，303,792 bytes，SHA-256 AD894FEEB6C7C74082E0BF0C84935E2F23F1D8941B2D4466BF09A0DEB4E9470A；
- V2RangedCompatibility：22/0，native 0，15.812 s，279,447 bytes，SHA-256 B252861B7C7934C1AF215B8C31DB2B51862438AE397FA28BB24C94A40840288E；
- V3.Attributes：4/0，native 0，14.731 s，258,290 bytes，SHA-256 5F71E1197AC49A36073F0B36B35F8D56D9B17EB501B404CDEC61AD829AAF2386；
- formal invocation total：1430/0（包含 focused/full overlap）；
- terminal-success markers：7/7；
- selected automation failure / Fatal / Unhandled / Ensure：0 / 0 / 0 / 0。

启动噪声说明：7 份正式日志各有 13 条 selected queue 前的 UE 5.8 generic `LogAutomationTest: Error: Condition failed`，没有 controller failure、Ensure/Fatal 或非零原生退出码。

环境定位：最初 full invocation 受到 Editor Home Screen `generate_204` 联网探测与 EOS 初始化干扰。最终使用 `-NoEOS`，并通过项目 `Saved/CodexAutomationConsoleVariables.ini` 的 `[Startup] HomeScreen.EnableHomeScreen=0` 在 MainFrame 模块注册前创建 deferred CVar。该临时文件不提交、不修改引擎或 Windows 全局设置；三份中止的定位日志仅本地保留，不计正式证据。

## Changed-file Regression

- mapping JSON：232 rules / parse PASS；
- 新增 `ThrownWeaponArcPreviewMainHUDRuntimeBinding` exact rule；
- GameMode、MainHUD 与 renderer adapter 规则均要求 runtime focused group；
- runtime rule要求 renderer、composition owner、surface transition、owner handoff、product lifecycle、combat coordinator、full、InputRestore、V2RangedCompatibility 与 ItemUseAndArmor；
- self-test：415/415，41,084 bytes，SHA-256 0B449BE5D5475F0F1796CA019865BBBBB797E381086CE0D03100664DFF58011D；
- focused-only fixture：预期失败；
- final gate：PASS，Changed=11 / Rules=5 / Required=75 / Logs=7；
- gate log：10,633 bytes，SHA-256 AC75AF4A717F5365FB96AC84E52E10C7D7BF13C3E2C1856CD3E6646BF94399AD。

## 静态边界

- runtime binding：2 files / 622 physical lines（h 125 / cpp 497）；
- runtime binding 中 Tick/DrawHUD/timer/async/save/file access/GetWorld/AActor/UObject/RNG：0；
- runtime binding 中 TODO/FIXME/HACK：0；
- 主体改动：1,269 additions / 1 deletion（不含 Report/Log）；
- `git diff --check`：PASS；
- 仅有工作区既有 LF→CRLF 提示，无 whitespace error。

## 构建与产物

- implementation Editor compile：Succeeded / native 0；
- focused test correction incremental Editor compile：Succeeded / native 0；
- final Editor：Succeeded / native 0 / 0 actions / 1.62 s；
- final Game：Succeeded / native 0 / 33 actions / 97.64 s；
- Editor build log：1,021 bytes / SHA-256 B650D01F9CA8C8F6556313992AD83159F9FE6EE59A4A129D20D1CD35EB3D6076；
- Game build log：4,166 bytes / SHA-256 29D3DE84D0D400BE93F024E547303E76C6F832300D02B8AA0CE9C6D5555FB04C；
- Editor DLL：18,330,112 bytes / SHA-256 32114BC0A3AC85CE54FC46B0359712FA7241C644058A0010FE819D7E3269855A；
- Game EXE：359,207,424 bytes / SHA-256 5C20B206B0841C8EA118E29E7AC511A848D3ACB66FDB702C28E317C79EF168A8。

## P/F 边界

PASS：GameMode runtime owner、permanent fallback、HUD attach/detach、Run begin/end、single-sampled basis、visible exact rehydrate/handoff、fresh empty binding、stale detach no-op、local Hide teardown、NoOp/replay separation、mapped regressions、Editor/Game builds。

未声明：真实 UI 可见性、输入设备、viewport/DPI/遮挡、多 viewport、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.62 建议进入单独授权的真实 UI 验收：验证 Arc 模式进入、编辑、确认、取消、HUD recreation 与实际落点一致性，并保留 renderer/binding/source-basis 三层故障归因。若继续保持 headless 边界，则先不声明 P20.62 完成。
