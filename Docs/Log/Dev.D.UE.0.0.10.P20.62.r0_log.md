# Dev.D.UE.0.0.10.P20.62.r0 Development Log

## 基线与目标

- base：9c983bc4264261b997c0cbb5963ddea7be2a2caa；
- branch：agent/0.0.10-p20-62-thrown-weapon-runtime-arc-edit-authority；
- 目标：修正 P20.11 choice session 对运行期 Arc target/apex/clear 的错误冻结，同时保持 trajectory 的 Run 级冻结；
- 运行边界：只执行 headless automation 与原生构建，不启动 Editor UI、PIE、Standalone 或产品。

## 调查记录

1. P20.61 runtime binding 只在最终 Arc hotbar route 中调用 preview update，并在同一同步调用继续 launch，因此当前预览没有发射前可见窗口。
2. PlayerController 已存在 target/apex/clear 的完整 interaction → request coordinator → intent → controller → GameMode 路由。
3. GameMode 提交时使用真实 `CombatRunCoordinator.IsActive()` 与 `ThrownWeaponProductLifecycle.IsEmpty()`。
4. lifecycle 在整个 combat Run 中保持 active，因此真实输入组合为 active Run + non-empty lifecycle。
5. P20.11 session 对所有 `Reduced` 命令统一拒绝，导致三种运行期编辑路径全部不可用。
6. trajectory 是 lifecycle 启动时冻结的 Run config；target/apex 是每次 launch 前读取的 choice。发射会捕获不可变 action/command snapshot，后续编辑不修改 in-flight action。
7. 原始 0.0.10 规划明确要求中级暗器能力可在战斗中通过输入调整弧度并进行空间判断；因此该拒绝属于产品契约缺口。

## 实现记录

- 在 session cpp 内增加局部 `IsLiveArcAimEdit` 分类；
- 仅 `SetArcTargetIntent`、`AdjustArcApex`、`ClearArcTargetIntent` 可绕过 Run/lifecycle fence；
- reducer 仍先执行，Straight 下 Arc edit 保持 `ModeMismatch`；
- replay/no-op 在 fence 前继续安全确认；
- `SelectTrajectory` 仍依次返回 `CombatRunActive` 或 `ProductLifecycleNotEmpty`；
- 无新 enum、无 GameMode 特判、无第二套 state/session、无 UI/World/库存/伤害改动。

## 测试改动

- session fence 测试改为真实 `active Run + non-empty lifecycle` 组合，顺序覆盖 target → apex → clear；
- 同一测试分别证明 active Run 与 non-empty lifecycle 继续冻结 trajectory；
- 新增 controller `RuntimeArcEditing`，覆盖三种 live edit 与 trajectory 反例；
- 新增 intent `RuntimeArcEditing`，覆盖每次一次 state read / command capture / route / submit；
- 既有 mode mismatch、revision conflict、replay/no-op、all command kinds 继续通过。

## 编译与自动化

- first Editor：58 actions / native 0 / 108.10 s；
- focused `Product.ThrownWeaponInputChoice`：39/0 / native 0；
- full `Shanmen.0_0_10`：1210/0 / native 0；
- formal invocation total：1249/0；
- terminal-success markers：2/2；
- Fail/Fatal/Unhandled/Ensure：0/0/0/0；
- final Game：57 actions / native 0 / 38.92 s；
- final Editor：up to date / 0 actions / native 0 / 0.96 s。

正式日志：

- focused：303,339 bytes / SHA-256 B5F1F308A0F65F27F536242F2A3BD8D9E1684466016AEFAD016608749CFBCAC5；
- full：1,811,204 bytes / SHA-256 DD8E3037A84287B5D48E0827E507375B156B6453B1632960C15C4B24614B7CFF；
- evidence audit：404 bytes / SHA-256 DD3E423DE71167B3574B42323A1FA1642C5BDC59A8C142C74F07CE484FADC1E2。

## 调用修正

首次 full 调用漏带项目已有的 `-cvarsini=Saved/CodexAutomationConsoleVariables.ini`，World 测试间出现 Home Screen `generate_204` 网络超时。该批次在 676/0 时中止，无 terminal-success marker，不计正式结果。日志保留为 `full_interrupted_missing_cvarsini.log`：1,142,647 bytes / SHA-256 1309E508D77BC6FE9455F97A743F5A099D9E2E4F7E587A30E33A3F1873DE8970。

恢复 P20.61 已验证参数后，正式 full 中 `generate_204` 为 0，1210 项全部成功。没有修改引擎、Windows 或用户全局设置。

## Changed-file Regression

- mapping JSON：232 rules / parse PASS；
- self-test：415/415，41,084 bytes，SHA-256 0B449BE5D5475F0F1796CA019865BBBBB797E381086CE0D03100664DFF58011D；
- changed files：5；
- matched rules：3（session、controller adapter、intent adapter）；
- required groups：5；
- evidence logs：2；
- final gate：`REGRESSION_COVERAGE: PASS Changed=5 Rules=3 Required=5 Logs=2`；
- gate SHA-256：2A8C40C65DF4B227EE33421595FF88B3CF74ECE034923FC22959E815FA47799A。

## 静态边界

- production：2 files / 26 added lines；
- World/Actor/UObject/Tick/timer/async/inventory/damage/spawn/trace/sweep/RNG/TODO/FIXME/HACK：0；
- static log SHA-256：5DFE143D05EDE2AFBDC14D1EA21F940086C7B93BB9F61C73684F4B29DAE4391C；
- total code/test diff：173 additions / 17 deletions；
- `git diff --check -- . ':(exclude)Docs/Log/*.log'`：PASS；原始构建/自动化 `.log` 为保持证据字节与 SHA-256 不变而不做空白清洗；
- unrelated untracked files 未暂存、未修改。

## 构建与产物

- first Editor log：6,264 bytes / SHA-256 62AD0DD31A8FFE6CFC8B1506DAA931948E2B8B8EDAC9F1B127B16C55C34166DE；
- final Editor log：1,021 bytes / SHA-256 342CA488D8684E2C4596AFC1B9DD819F6B340D6014AAD15302FE2E109F2EB4BA；
- Game log：6,028 bytes / SHA-256 57B4E656BCBE33402057326A176A773FA518CA51E4AACD346C07A7F6164809A9；
- demo_map.exe：359,218,688 bytes / SHA-256 FAF2F095478DE6917033C6BFFE411320DEE57DEE13781E7D5D0CB0862B501C62；
- UnrealEditor-demo_map.dll：18,342,400 bytes / SHA-256 09802A7A3706E24DCF90B5F062F0F5EEA1753410C299B825A7B7C104176C34B8。

## P/F 边界

PASS：运行期 Arc target/apex/clear 权威变更、trajectory 双 fence、Straight mode mismatch、revision/replay/no-op、controller/intent 单次委托、映射回归、Editor/Game 构建。

未声明：发射前预览时机、编辑即时刷新、预览热栏槽、arm/confirm/cancel 手势、真实 UI/输入、PIE、Standalone、截图、Smoke、Cook、Package。

## 下一阶段

P20.63 应只解决 pre-launch preview context：在不复制 item/Run/choice/launch 权威的前提下，定义唯一已选 Arc hotbar slot 与明确的 arm/confirm/cancel 手势，使本轮已经可用的 target/apex/clear 编辑能刷新 P20.61 MainHUD runtime binding。真实 UI 验收仍需单独授权后执行。
