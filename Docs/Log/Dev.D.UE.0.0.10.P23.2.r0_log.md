# Dev.D.UE.0.0.10.P23.2.r0 Development Log

## 1. 基线与目标

- base：`20fbd0e4f4ecaed6d0d2f9b7d1583f6bbd2f475d`（P23.1 Divine Sense edge guidance）；
- branch：`agent/0.0.10-p23-2-divine-sense-feedback`；
- 目标：把现有神识产品链已经产生的拒绝原因变成玩家可读的短暂 HUD 反馈；
- 边界：不改扫描、生命、遮挡、成本、容量、Receipt、地图或资产，不新增 wrapper 链，不启动 UI/PIE/产品 executable。

## 2. 缺口定位

`Ademo_mapPlayerController::UseDivineSense()` 已能拿到完整 `Fdemo_mapShanmenDivineSenseLogicalInputResult`，状态可区分 ProductUnavailable、RetryRequired、Busy、AdapterInactive、BindingMismatch 与 StateDesynchronized，但该值只写 UE_LOG；shipping 构建没有可供 HUD 读取的最近结果。

因此正常成功有目标揭示，失败却表现为按键无反应。尤其第 11 次规范脉冲同时具备冻结的 10 点成本与 0 点可用 SpiritEnergy，却没有任何玩家侧表达。

## 3. 纯反馈投影

在既有 Divine Sense HUD Presentation 文件中新增两个轻量枚举与一个纯值投影器。输入是 typed logical result 和当前按键显示名，输出是 reason、warning/error tone 与一行文本。

ProductUnavailable 优先比较冻结成本与可用灵力，再检查 intent/route capacity；RetryRequired 才读取重映射键。Busy、Unavailable 与结构性 Failed 使用固定短文案。成功或无效结果返回 false，并先清空输出，防止复用对象泄漏旧提示。

投影器不持有 World、Timer、输入、资源或 Receipt，不解析 Diagnostic 字符串。

## 4. Controller 状态

将 `LastDivineSenseInputResult` 从 non-shipping 自动化字段提升为生产只读状态，并增加 2.25 秒游戏时间截止点。`RouteDivineSenseInput()` 的 UI blocked、missing GameMode 与真实 GameMode 返回三条路径统一捕获。

成功结果把截止点设回无效值，保证成功揭示不与旧错误竞争；拒绝结果只有在有效 World 上才激活。状态没有 UPROPERTY、复制、序列化或存档职责。

## 5. HUD 接线

`DrawDivineSenseReveals()` 不再要求“必须存在活跃成功揭示”才进入。它分别计算 success reveal 与 rejected feedback：

- reveal 活跃时继续绘制 P23.1 的屏内/边缘目标；
- feedback 活跃时绘制顶部 500×36 面板；
- 两者同时存在时保留目标 marker，并让失败原因替换成功状态面板；
- 两者都不存在时立即返回。

warning 使用金色，error 使用红色。HUD 不拥有结果、不修改资源、不重新路由输入。

## 6. 测试增量

HUD Presentation 新增 `UnavailableFeedback`，验证稳定投影、warning tone、按键无关性与无效输入清旧值。

Physical Input 的缺 GameMode 场景增加真实 HUD feedback 断言；GameMode lifecycle 改由 PlayerController 路由，连续执行 10 次成功脉冲，再验证第 11 次为 ProductUnavailable、InsufficientSpirit、`NEED 10 SPIRIT · 0 AVAILABLE`。teardown 的规范 pulse count 相应由 1 扩展到 10。

初始聚焦 HUD 5/0、Physical Input 5/0；初始完整 Divine Sense 53/0。最终 Editor DLL 再跑完整 Divine Sense，53/0，日志 SHA-256 `9297412CAC7347411DCEB1E22C0D35AC061F542FB5D7793C5ED54B084BDB7018`。

## 7. 改动映射回归

中央 PlayerController 规则要求完整 `Shanmen.0_0_10`，所以本轮没有像 P23.1 那样在 721/0 慢段主动终止。最终正式完整树从头到尾自然完成 1265/0，原生终止 0，且 `Fatal/Unhandled/Ensure/generate_204` 全为 0。

6 个 legacy 日志合计 135/0：FullSystemLoop.41 1、FullSystemLoop.47 1、InputRestore 101、P5RuntimeInterface.06 1、P7Integration 9、V2RangedCompatibility 22。覆盖器以 7 份日志满足 44 个要求组，结果 `PASS Changed=7 Rules=4 Required=44 Logs=7`；门禁自测 442/442。

## 8. 环境修复轨迹

第一次 full 漏带 Home Screen 配置，在 737/0 时中止并保留，含 69 条 `generate_204` 记录。第二次误用相对 `-cvarsini`，完整 1265/0 但启动含 1 条文件不存在 `Ensure`，故拒绝为正式证据。

读取 P20/P22 的既有工程记录后，恢复已验证的进程级 `-ini:Engine:[ConsoleVariables]:HomeScreen.EnableHomeScreen=0`。5/0 探针先证明外网探测与异常均为 0，再启动最终 full。该修复没有写项目配置、Engine 或 Windows。

覆盖汇总第一次把 PowerShell 脚本当作 native executable 读取 `$LASTEXITCODE`，该变量为空导致外层错误返回 1；内部覆盖与 442/442 已成功。改用 `$?` 重跑后外层 0。规则和测试证据未修改。

## 9. 构建与静态边界

- Game Development：30 actions / 79.04s / Result Succeeded；随后 up-to-date 原生退出 0；
- Editor Development：4 actions / 14.10s / 原生退出 0；
- Game：359,625,728 bytes / SHA-256 `891901DDFFA60A3E1A6FE4144C77E7AA5F58B58F62ABABEBEC4242B2C384760B`；
- Editor DLL：18,877,952 bytes / SHA-256 `58D5E82698D5B147CD531D46EDA8A9E6334DE6F565F4626B05A698603157904E`。

最终 Editor 构建只重编译一个文字描述改动的 Physical Input test 并重链接；生产对象没有变化。最终 DLL 随后通过 Divine Sense 53/0 与 legacy 135/0。新增生产增量没有 Tick 调用、Timer/SetTimer、Sleep、随机数、ApplyDamage、SpawnActor 或 DestroyActor；`git diff --check` 为 0。

## 10. 提交边界与 GitHub

只提交 7 个代码/测试文件、本 Report 与本 Log。103 个既有 untracked 文件保持未暂存；所有原始验证日志继续由 Git ignore。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-2-divine-sense-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-2-divine-sense-feedback/Docs/Report/Dev.D.UE.0.0.10.P23.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-2-divine-sense-feedback/Docs/Log/Dev.D.UE.0.0.10.P23.2.r0_log.md>
