# Dev.D.UE.0.0.10.P23.2.r0 Report

## 1. 结论

P23.2 在 P 阶段边界内完成，结论为 **PASS**。

本轮修复神识按键失败时的“无反应”体验：底层原本已经能区分灵力不足、脉冲次数耗尽、重试、忙碌、未就绪与失败，但结果只进入日志。现在真实物理输入路径会把最近一次拒绝结果保留 2.25 秒，主 HUD 直接显示简短原因；成功脉冲仍只显示 P23.0/P23.1 的目标、距离、遮挡与边缘方向，不产生冲突提示。

```text
Full Shanmen regression:              1265 Success / 0 Fail
Post-build full Divine Sense:           53 Success / 0 Fail
Changed-file mapped regression:       1400 Success / 0 Fail (7 logs)
Changed-file regression coverage:     PASS (7 paths / 4 rules / 44 groups)
Regression gate self-test:             PASS 442/442
Game + Editor Development:             PASS (both native 0)
git diff --check:                       PASS
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明无头产品链、最终编译 DLL 的神识反馈、共享输入兼容面与构建边界，不声明视觉手感、本地化或人工游玩已验收。

## 2. 玩家侧变化

神识按键被拒绝时，屏幕顶部现在显示 2.25 秒反馈：

- 灵力不足：`DIVINE SENSE · NEED 10 SPIRIT · 0 AVAILABLE`；
- 本次运行脉冲容量耗尽：`DIVINE SENSE · PULSE LIMIT REACHED`；
- 可重试中断：显示当前重映射按键并提示重试；
- 产品忙碌：`DIVINE SENSE · STABILIZING`；
- UI 阻挡、运行未就绪或产品不可用：`DIVINE SENSE · UNAVAILABLE`；
- 绑定不一致、状态不同步或产品拒绝：红色失败提示。

可恢复或资源类原因使用金色警告；结构性失败使用红色错误。若先前一次成功揭示仍在 3 秒有效期内，随后按键失败只替换顶部状态面板，既有目标菱形、边缘箭头与距离仍继续显示。

## 3. 单一权威与原因投影

新增纯 `Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation`。它只读取现有 `Fdemo_mapShanmenDivineSenseLogicalInputResult` 的枚举状态与冻结 Availability，不解析 Diagnostic 文本，不重新查询或修改 SpiritEnergy，也不重新判定技能成败。

`ProductUnavailable` 会从冻结配置读取精确成本、从冻结资源快照读取可用灵力；成本大于可用量时生成灵力不足提示，否则再判断脉冲容量。相同输入得到相同原因、色调与文本；无效或成功结果失败关闭并清空复用输出。

本轮没有新增第二个 Divine Sense Controller、Route、Host、Actor、Subsystem、Gameplay 资源或存档字段。

## 4. 物理输入与 HUD 接线

`Ademo_mapPlayerController::RouteDivineSenseInput()` 的每个返回路径现在都调用同一个捕获函数，包括 UI surface 阻挡、缺少产品 GameMode 与 GameMode 返回的完整产品结果。接受结果清除旧拒绝提示；拒绝结果使用 World 游戏时间设置 2.25 秒截止点。

HUD 每帧只读 PlayerController 中的最近结果与截止点，再交给纯投影器。这里没有 Timer、SetTimer 或新增 Tick；暂停时游戏时间不前进，提示不会因真实墙钟漂移。HUD 仍从 GameMode 的成功 Receipt 读取目标揭示，两种状态没有双写。

## 5. 自动化证明

神识 HUD 表现组从 4 项增至 5 项。新增测试证明：结构化的 `AdapterInactive` 结果生成稳定、简短、warning 色调的 unavailable 文本；输入按键标签不会污染不需要按键的原因；无效证据会清空旧输出。

物理输入测试扩展真实 Controller→GameMode→Product 链：前 10 次规范脉冲全部接受并把 SpiritEnergy 从 100 消耗到 0；第 11 次返回 `ProductUnavailable`，HUD 原因严格为 `InsufficientSpirit`，文本包含 `NEED 10 SPIRIT · 0 AVAILABLE`。成功脉冲同时断言不会留下拒绝 banner。

最终 Editor DLL 上运行完整 `Shanmen.0_0_10.Product.DivineSense`，结果 53/0、原生终止 0、无 fatal/unhandled/ensure，日志 329,020 bytes，SHA-256 `9297412CAC7347411DCEB1E22C0D35AC061F542FB5D7793C5ED54B084BDB7018`。

## 6. 改动文件回归

本轮按改动文件而非主题选择回归。因为修改了中央 `demo_mapPlayerController.cpp/.h`，映射规则要求完整 `Shanmen.0_0_10`，没有用神识聚焦组替代。正式完整树自然结束为 1265/0；首末 Success 为 `2026-09-09 16:27:37.725` 至 `17:32:43.754`，日志 1,879,709 bytes，SHA-256 `C15513466E422671830F2373ABAE33F4B46946A547DC3D9A726305572E44EC84`。

最终 DLL 上另跑 6 个 legacy 映射组：FullSystemLoop 41/47、InputRestore 101、P5RuntimeInterface.06、P7Integration 9 与 V2RangedCompatibility 22，合计 135/0。连同完整 Shanmen 树，changed-file 映射证据合计 1400/0。

覆盖器结果为 `PASS Changed=7 Rules=4 Required=44 Logs=7`；`demo_map.InputRestore` 父组同时覆盖其要求的 `.32` 子组，完整 `Shanmen.0_0_10` 父组覆盖 37 个精确子要求。覆盖结果日志 6,677 bytes，SHA-256 `AD6CB1F91281D76C04E241C40B934E7F001D86A14949C2B6B4A79C1394EBD62D`；门禁自测 442/442，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 7. 环境失败与有界修复

第一次完整树命令漏带项目既有 Home Screen 禁用配置，在 737/0 时因持续 `generate_204` 探测而中止；日志保留为 `P23.2_full_interrupted_missing_cvarsini.log`，SHA-256 `9C44070BC313A96CD21E852E4CE00A79925EE2E5575D215E7887A65B3014516A`，不计通过。

第二次使用相对 `-cvarsini`，UE 从错误工作目录解析文件，启动产生 1 条 `Ensure`；测试虽自然完成 1265/0，但证据被门禁拒绝。日志 SHA-256 `9C19E19F7F3F77E045D12D20C4F8EA42DB563A275E6ADF2AD29B59EB6BCD1879`，不计通过。

修复使用 P22 已验证的进程级参数 `-ini:Engine:[ConsoleVariables]:HomeScreen.EnableHomeScreen=0`。先以 5/0 探针确认 `Fatal/Unhandled/Ensure=0` 且 `generate_204=0`，再从头运行正式完整树。最终日志 1265/0、原生终止 0、异常 0、外网探测 0；未修改 Engine、Windows 或项目持久配置。

一次覆盖汇总壳误读 PowerShell 脚本未设置的 `$LASTEXITCODE`，使外层返回 1；覆盖内容已 PASS、442 项自测也已全过。修正为读取 `$?` 后原样重跑，最终外层 native 0。没有放宽或修改覆盖规则。

## 8. 构建与静态边界

最终构建均使用 UE 5.8 Development、`-WaitMutex`、`-NoHotReloadFromIDE` 与最多 2 个并行动作：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P23.2_GameBuild_final.log` | PASS / 30 actions | 4,096 | `4DFB16E51F85FCF735E328BC7EB00CC674277250B9CB160C2FFB454882DE6113` |
| `P23.2_GameBuild_final_verify.log` | PASS / up to date / native 0 | 949 | `354C95A7B73398A4F4D6CEEDF8E2F4B53085A4F0676D4099ABD34DA588A9CF31` |
| `P23.2_EditorBuild_final.log` | PASS / 4 actions / native 0 | 2,332 | `9BA7D7AF8B5B2CEA4861E3BD4D11BE4A0B5DFED42BD53F8577D8E0110C996684` |

最终 `demo_map.exe` 为 359,625,728 bytes，SHA-256 `891901DDFFA60A3E1A6FE4144C77E7AA5F58B58F62ABABEBEC4242B2C384760B`；`UnrealEditor-demo_map.dll` 为 18,877,952 bytes，SHA-256 `58D5E82698D5B147CD531D46EDA8A9E6334DE6F565F4626B05A698603157904E`。

完整树后只修改了一个测试断言的描述文字；最终 Editor 构建明确只重编译该物理输入测试并重链接，生产对象未变化。最终 DLL 随后通过 Divine Sense 53/0 与全部 legacy 映射 135/0。

新增生产代码中的 Tick 调用、Timer/SetTimer、Sleep、随机数、ApplyDamage、SpawnActor、DestroyActor 均为 0；回归 JSON 可解析，`git diff --check` 为 0。

## 9. P/F 边界

PASS：失败脉冲短暂可见；灵力不足显示冻结成本与当前值；成功脉冲不残留错误；既有目标揭示与失败 banner 可共存；重映射键用于 retry 文本；完整树 1265/0；最终 DLL 神识 53/0；mapped 1400/0；覆盖门、自测和双构建通过。

未声明：提示文案已本地化、所有分辨率与 UI 缩放无裁切、颜色无障碍、手柄震动、多人客户端 HUD、人工游玩或真实视觉验收。本轮没有改变神识半径、3 秒揭示期、10 点成本、10 次容量、遮挡判定或扫描权威。

## 10. 提交边界与 GitHub

本阶段只提交 5 个生产文件、2 个测试文件、本 Report 与本 Development Log，共 9 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P23.2` 原始日志不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-2-divine-sense-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-2-divine-sense-feedback/Docs/Report/Dev.D.UE.0.0.10.P23.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-2-divine-sense-feedback/Docs/Log/Dev.D.UE.0.0.10.P23.2.r0_log.md>
