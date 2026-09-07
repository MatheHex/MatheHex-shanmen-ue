# Dev.D.UE.0.0.10.P20.62.r0 Report

## 1. 结论

P20.62 已修正投掷武器 Arc 输入选择的运行期权威边界。

此前 `Fdemo_mapShanmenThrownWeaponInputChoiceSession` 把 trajectory、Arc target 与 Arc apex 全部视为“只允许在 combat Run 之间修改”的同类状态；真实 combat Run 激活后，GameMode 同时传入 `bCombatRunActive=true` 与 `bProductLifecycleEmpty=false`，导致 P20.22–P20.27 已接入的目标点、弧高和清除物理输入全部被 session 拒绝。玩家虽然拥有按键路由，却无法在实际战斗中完成规划要求的弧线空间判断。

本轮保持 reducer 为唯一状态转换权威，只把 `SetArcTargetIntent`、`AdjustArcApex`、`ClearArcTargetIntent` 定义为 BallisticArc Run 中可继续变化的 live aim edit。`SelectTrajectory` 仍受 active Run 与非空 lifecycle 双重 fence，不能在一次 Run 中途切换 Straight/Arc。Arc edit 仍先经过 reducer，因此 Straight 模式继续以 `ModeMismatch` 失败关闭；revision、replay、no-op 与 deterministic state identity 契约均未绕过。

聚焦自动化 39/0，全量 `Shanmen.0_0_10` 1210/0；Editor 与 Game 构建均为 native 0。changed-file regression gate 对 5 个改动文件推导出 5 个必跑组并全部覆盖。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`9c983bc4264261b997c0cbb5963ddea7be2a2caa`（P20.61）；
- 分支：`agent/0.0.10-p20-62-thrown-weapon-runtime-arc-edit-authority`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 根因与权威判断

P20.9/P20.11 的原始 fence 用于冻结一次 combat Run 的 trajectory 配置，这一目标正确；但 P20.10 后来把 trajectory 与 Arc aim 放进同一 choice state，session 仍对所有 `Reduced` 命令统一套用旧 fence，形成了契约错位。

`Fdemo_mapShanmenThrownWeaponProductLifecycle::IsEmpty()` 在这里表示生命周期是否绑定整个 combat Run，不等同于“当前是否存在一个可被 aim edit 改写的飞行快照”。投掷发射路径会从 choice state 捕获一次不可变 command/action snapshot；之后的目标或弧高编辑只影响下一次捕获，不会改写已在飞行中的投掷物。

因此正确边界是：

- trajectory 是 Run 级配置，只能在 Run/lifecycle 空态修改；
- Arc target/apex 是 BallisticArc Run 内的实时瞄准选择，可以持续修改；
- reducer 继续负责模式校验、revision、规范化、no-op 与 replay；
- session 只决定一个已成功归约的变化是否受产品生命周期 fence 阻挡。

## 4. 实现

`demo_mapShanmenThrownWeaponInputChoiceSession.cpp` 新增局部、无状态的 `IsLiveArcAimEdit` 分类：仅识别 target、apex 与 clear 三种命令。session 的执行顺序保持不变：

1. 先调用唯一 reducer；
2. reducer 失败时透传精确 typed 原因；
3. exact replay 与 fresh no-op 直接安全确认；
4. 对真正 `Reduced` 的 trajectory 命令执行 Run/lifecycle fence；
5. 对真正 `Reduced` 的 Arc aim edit 发布 reducer 产生的新 state。

没有新增 enum、第二套 session、旁路状态或 GameMode 特判。header 注释同步明确 trajectory 与 live Arc aim 的不同寿命。

## 5. 自动化覆盖

session 的 `RunAndLifecycleFences` 现在证明：

- 初始 Straight→Arc trajectory 变化在 active Run 或非空 lifecycle 下仍被拒绝；
- Run 之前选择 BallisticArc 后，target、apex、clear 在 `active Run + non-empty lifecycle` 的真实组合下依次成功；
- 三次 live edit 各推进一次 revision；
- active Run 与 lifecycle 单独释放前都继续阻止 Arc→Straight；
- 双 fence 释放后 trajectory 才能改变。

新增两项跨层自动化：

- `ThrownWeaponInputChoiceControllerAdapter.RuntimeArcEditing`：证明 controller 对三种 live edit 各 resolve/submit 一次，同时 trajectory 仍被 session 拒绝；
- `ThrownWeaponInputChoiceIntentAdapter.RuntimeArcEditing`：证明 logical intent 每次只读取当前 state、捕获当前 revision 并委托一次，三种 live edit 均可在真实 Run fence 组合下生效。

既有 reducer 的 Straight `ModeMismatch`、revision conflict、replay/no-op 与所有 command-kind 测试继续由聚焦和全量组覆盖。

## 6. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `P20.62.r0_focused.log` | `Product.ThrownWeaponInputChoice` | 39/0 | `B5F1F308A0F65F27F536242F2A3BD8D9E1684466016AEFAD016608749CFBCAC5` |
| `P20.62.r0_full.log` | `Shanmen.0_0_10` | 1210/0 | `DD8E3037A84287B5D48E0827E507375B156B6453B1632960C15C4B24614B7CFF` |

两份正式日志合计 1249 个 Success、0 Fail、2/2 native terminal-success markers、0 Fatal/Unhandled/Ensure。聚焦日志 303,339 bytes；全量日志 1,811,204 bytes。全量从 P20.61 的 1208 增至 1210，增量正好是本轮新增的 controller 与 intent 两项测试。

## 7. 调用修正与保留证据

首次正式全量调用漏带 P20.61 已验证的 `-cvarsini=Saved/CodexAutomationConsoleVariables.ini`，导致 Editor Home Screen 对 `generate_204` 的联网探测在 World 测试之间反复超时。该批次在 676/0 时主动停止，未产生终止标记，绝不计作正式成功；原始日志以 `full_interrupted_missing_cvarsini.log` 保留，1,142,647 bytes，SHA-256 `1309E508D77BC6FE9455F97A743F5A099D9E2E4F7E587A30E33A3F1873DE8970`。

修正调用只恢复项目本地已有的自动化 CVar 文件，没有修改引擎、Windows、用户全局配置或产品代码。重跑正式全量的 `generate_204` 计数为 0，并获得完整 1210/0 与 native 0。

首次 Editor 编译即成功：58 actions / native 0。没有源码、断言或产品自动化失败。

## 8. Changed-file Regression 与静态边界

- mapping self-test：415/415，SHA-256 `0B449BE5D5475F0F1796CA019865BBBBB797E381086CE0D03100664DFF58011D`；
- changed-file gate：`PASS Changed=5 Rules=3 Required=5 Logs=2`；
- gate SHA-256：`2A8C40C65DF4B227EE33421595FF88B3CF74ECE034923FC22959E815FA47799A`；
- production 新增行静态扫描：2 files / 26 added lines / 0 matches；
- 扫描项：World/Actor/UObject、Tick/timer/async、库存、伤害、spawn、trace/sweep、RNG、TODO/FIXME/HACK；
- `git diff --check -- . ':(exclude)Docs/Log/*.log'`：PASS，仅有工作区既有 LF→CRLF 提示，无源码或 Markdown whitespace error；原始构建/自动化 `.log` 为保持证据字节与上述 SHA-256 不变而不做空白清洗。

主体改动为 5 个文件、173 additions / 17 deletions；其中生产契约只修改 session header/cpp，controller 与 intent 生产适配器未改。

## 9. 构建与产物

- first Editor：58 actions / Succeeded / native 0，SHA-256 `62AD0DD31A8FFE6CFC8B1506DAA931948E2B8B8EDAC9F1B127B16C55C34166DE`；
- final Editor：up to date / 0 actions / Succeeded / native 0，SHA-256 `342CA488D8684E2C4596AFC1B9DD819F6B340D6014AAD15302FE2E109F2EB4BA`；
- final Game：57 actions / Succeeded / native 0，SHA-256 `57B4E656BCBE33402057326A176A773FA518CA51E4AACD346C07A7F6164809A9`。

产物：

- `Binaries/Win64/demo_map.exe`：359,218,688 bytes，SHA-256 `FAF2F095478DE6917033C6BFFE411320DEE57DEE13781E7D5D0CB0862B501C62`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,342,400 bytes，SHA-256 `09802A7A3706E24DCF90B5F062F0F5EEA1753410C299B825A7B7C104176C34B8`。

## 10. P/F 边界与下一步

PASS：BallisticArc Run 中 target/apex/clear live edit、active Run + non-empty lifecycle 真实 fence 组合、trajectory 冻结、Straight mode mismatch、revision 单次推进、controller/intent 单次委托、replay/no-op、全量回归、Editor/Game 构建。

未声明：发射前可见预览、编辑后即时刷新、热栏预览槽选择、二段确认手势、真实 UI、真实输入、viewport/DPI/遮挡、PIE、Standalone、截图、Smoke、Cook 或 Package。

P20.61 的 preview 目前仍只在 Arc hotbar 最终确认路径中更新，target/apex 编辑虽然从本轮开始能真正改变权威 state，但还没有一个“发射前已选中的 Arc 热栏槽/预览上下文”可供编辑事件刷新。建议 P20.63 单独建立唯一的 pre-launch preview context，并明确 arm/confirm/cancel 的产品手势，再把三种已可用的 live edit 接到既有 MainHUD runtime binding；继续不复制 item、Run、choice 或 launch 权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-62-thrown-weapon-runtime-arc-edit-authority>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-62-thrown-weapon-runtime-arc-edit-authority/Docs/Report/Dev.D.UE.0.0.10.P20.62.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-62-thrown-weapon-runtime-arc-edit-authority/Docs/Log/Dev.D.UE.0.0.10.P20.62.r0_log.md>
