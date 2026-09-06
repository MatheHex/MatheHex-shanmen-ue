# Dev.D.UE.0.0.10.P20.60.r0 Report

## 1. 结论

P20.60 已完成 thrown-weapon Arc preview 到 MainHUD 的首个真实 renderer adapter 契约：`Ademo_mapHUD` 现在持有一个独占、不可复制的物理 surface adapter，显式初始化每个 HUD 实例的身份，并把 renderer-neutral preview segments、apex 与 planned landing point 投影到 Canvas。

适配器继续复用既有 presentation command、consumer、delivery、surface recreation 与 ownership handoff 协议，没有建立第二套显示状态、authority、ledger、scheduler、retry 或 storage。Show / Replace / Hide、cleanup 与 retirement 均返回既有 typed evidence；非法入口、陈旧 cursor、错误 permit/ticket 与身份旋转全部失败关闭且不修改 surface。

本阶段通过 headless contract automation 与 Editor/Game 编译证明代码契约成立；没有把 product/composition owner 绑定到这个 HUD endpoint，也没有启动真实 UI。因此不声明玩家已经能在运行中的游戏里看到轨迹。该最终产品接线留给 P20.61。

## 2. 基线、分支与改动范围

- 基线：18cd799469f8e898691e2c7c53bd2c3c5ad7e14a（P20.59）；
- 分支：agent/0.0.10-p20-60-thrown-weapon-arc-preview-mainhud-renderer-adapter-contract；
- 新增 MainHUD Arc preview renderer adapter 头文件与实现；
- `Ademo_mapHUD` 新增 adapter ownership、BeginPlay physical identity 初始化与 DrawHUD 投影绘制；
- ProductLifecycle automation 新增 5 项 renderer endpoint 契约测试；
- changed-file regression mapping 新增 adapter exact rule，并把该 focused group 回接到 7 条上游 surface/presentation 规则；
- mapping self-test 新增 MainHUD header/cpp、完整依赖与 focused-only rejection fixtures；
- 新增本 Report 与同名 Development Log；
- 未改动既有 presentation command、delivery、owner handoff、recovery 或 product authority 实现。

## 3. 物理 Surface 身份与 HUD 所有权

`Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter` 由 `Ademo_mapHUD` 直接持有，禁止复制。HUD 的 `BeginPlay` 为每个物理实例生成一个非零 `SurfaceInstanceId`，adapter 使用稳定 consumer definition `Renderer.ArcPreview.MainHUD.r1`。

初始化规则：

1. 缺少物理身份时失败关闭；
2. 相同身份的初始化重放幂等；
3. 已初始化实例拒绝静默更换身份；
4. dirty initial cursor 或 consumer state 不可初始化；
5. 任一失败均不产生部分可用状态。

adapter 只保存 physical identity、stable consumer identity 与 visible-or-empty surface cursor。Run ownership、delivery ordering、replay sealing 和 recovery 仍属于既有上游契约。

## 4. Show / Replace / Hide 契约

三个 renderer mutation endpoint 共同要求：

- command 自身有效且确实要求 render mutation；
- 调用的 endpoint 与 command kind 精确一致；
- command previous state 与当前物理 cursor 精确匹配；
- candidate state 只允许规范化为 visible state 或 empty state；
- typed response 能由既有 surface response contract 自验证。

Show 与 Replace 保存 renderer-neutral visible cursor；Hide 保存 empty cursor。合法命令产生 `Applied` evidence。错误 endpoint 可在证据可构造时产生 `Rejected` evidence；陈旧 cursor、NoOp 误入 mutation endpoint 或不可自验证输入直接返回 invalid response。任何拒绝均保持原 cursor 不变。

通过既有 consumer + delivery session 的顺序测试同时证明：Show 应用、精确 Show replay 不再次调用 surface、Replace 应用、NoOp 只推进 delivery ledger 而不改物理 cursor、Hide 归一为空。

## 5. Cleanup 与 Ownership Handoff

`ClearToEmpty` 只接受：

- 有效且已初始化的 renderer；
- action 为 `ClearToEmpty` 的 exact cleanup permit；
- 相同 consumer definition；
- permit observed cursor 与当前 visible cursor 精确一致。

绑定 permit 不能伪装成 cleanup 权威；成功清理返回 typed lifecycle response，cursor 置空，同一 permit 对空 surface 的重放失败关闭。

`RetireForHandoff` 只接受有效 `AdoptExact` transition ticket，要求 consumer identity 相同、新旧 physical surface identity 不同、ticket observed cursor 精确匹配旧 surface 的 visible cursor。成功后旧 surface 返回 typed retirement evidence 并置空；新 surface 保持已采纳的 visible cursor；旧 surface 的 retirement replay 不产生第二次修改。

## 6. MainHUD 绘制边界

`DrawHUD` 只读取 adapter 当前 cursor：

- 对每个 renderer-neutral segment 分别执行 world-to-screen projection，并绘制 cyan line；
- apex 绘制 amber cross marker；
- planned landing point 绘制 green cross marker；
- Canvas、PlayerController、projection 或 visible state 不可用时局部跳过，不写回 product state；
- 绘制过程不生成 command、不推进 delivery、不拥有 Run authority。

这一层是 renderer endpoint，不是 product route。P20.60 故意不从 Tick/DrawHUD 轮询上游状态，也不直接读取物品、动作或战斗权威。

## 7. 测试覆盖

新增 5 项 focused automation：

1. InitializationAndPhysicalIdentity；
2. DeliverySequence；
3. DirectCommandFences；
4. ExactCleanupPermit；
5. ExactHandoffRetirement。

覆盖了显式初始化、身份重放/旋转拒绝、完整 Show→replay→Replace→NoOp→Hide 顺序、错误入口、陈旧 cursor、cleanup permit 类型隔离、exact handoff retirement 与重放不变性。

| Log | Group | Success/Fail | Native exit | Seconds | Bytes | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| P20.60_Focused.log | Product...MainHUDRendererAdapter | 5/0 | 0 | 72.993 | 269,165 | C405DF778070BBD00D65E37F60BD0D9B338088E45C9B4071871B210DCCC63101 |
| P20.60_Full.log | Shanmen.0_0_10 | 1203/0 | 0 | 4215.998 | 1,849,863 | 2363D677380218186B77D6B905975604CC44AB1F6C148224468371F1A91011A2 |
| P20.60_InputRestore.log | demo_map.InputRestore | 101/0 | 0 | 25.291 | 395,377 | 64F45B6BE4F8C4FE6E51C64B50FE6377AA32A8A0ADB79890F60FAC5090098C72 |
| P20.60_ItemUseAndArmor.log | demo_map.ItemUseAndArmor | 46/0 | 0 | 17.164 | 309,560 | 6FF47A2A7E9A35FEAD3357385D9B1B02A29DF58E108076D0C76A062F8C63B6F2 |
| P20.60_V2RangedCompatibility.log | demo_map.V2RangedCompatibility | 22/0 | 0 | 16.516 | 285,182 | BD0E961A20A976C7636AB5FFE540F6E0EFC5CB2FB7386AA042F713CF55E8B5EA |

五次正式 invocation 累计 1377/0（focused 与 full 有意重叠）；每份日志均包含 `TEST COMPLETE. EXIT CODE: 0` 原生终止标记。selected automation failure、Fatal、Unhandled 与 `Ensure condition failed` 均为 0。

UE 5.8 在每次进程的 Engine 初始化前都会输出 13 条 generic `LogAutomationTest: Error: Condition failed`，紧邻引擎自身 `UnifiedErrorTest` 启动输出；它们不属于已选择的测试、发生在测试队列前，且没有形成 Ensure/Fatal、失败结果或非零退出码。本 Report 保留这一事实，不把启动噪声伪装成产品测试失败，也不忽略原生结果。

## 8. Changed-file Regression、静态边界与构建

- mapping JSON：231 rules / parse PASS；
- final changed-file gate：PASS，Changed=7 / Rules=3 / Required=30 / Logs=5；
- mapping self-test：413/413；
- focused-only gate fixture：按预期拒绝，不能用 adapter focused 证据替代 presentation/lifecycle/handoff/HUD compatibility；
- `git diff --check`：PASS，0 whitespace errors；
- 新 adapter：335 physical lines（h 80 / cpp 255）；
- adapter 中 Tick/timer/async/save/file access/World/Actor/UObject/RNG：0；
- adapter 中 TODO/FIXME/HACK：0；
- 首次 Editor compile：Succeeded / native 0 / 42.49 s；
- tests integration Editor compile：Succeeded / native 0 / 10.71 s；
- 首次 Game compile：Succeeded / native 0 / 50.74 s；
- final Editor：Succeeded / native 0 / 0 actions / 1.21 s / up to date；
- final Game：Succeeded / native 0 / 0 actions / 0.98 s / up to date；
- Editor DLL：18,284,032 bytes / SHA-256 032861DD317A7C765A2CEB3A8A3982E060EB2C0C706EE7552DE903FA1DAF6854；
- Game EXE：359,166,464 bytes / SHA-256 50A5D281B39D09775DBC906007E3AFA0B65F36E7626A4BC20FE562327D8C6168。

## 9. P/F 边界

PASS 范围：MainHUD-owned renderer adapter、explicit physical identity、stable consumer identity、visible-or-empty cursor、typed Show/Replace/Hide evidence、delivery replay/NoOp separation、stale/wrong-entrypoint fences、exact cleanup、exact handoff retirement、Canvas projection implementation、focused/full/legacy regressions、changed-file coverage、Editor/Game compilation。

未验证且不声明：product/composition owner 到 HUD adapter 的运行时绑定、真实 Run delivery、玩家可见轨迹、真实窗口尺寸/DPI/遮挡/多 viewport/split-screen 行为、真实输入、Unreal Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 10. 下一步与 GitHub

P20.60 已建立真实 MainHUD endpoint，但它仍是未接线的被动 surface。P20.61 应让现有 active Run composition owner / delivery host 在 HUD 创建与销毁边界显式绑定、解除并 handoff 这个 adapter；必须沿用既有 command ledger、surface lifecycle 与 owner authority，禁止在 DrawHUD/Tick 轮询或建立第二套 preview 状态。完成 headless composition tests 后，再由用户单独授权真实 UI 运行验收。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-60-thrown-weapon-arc-preview-mainhud-renderer-adapter-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-60-thrown-weapon-arc-preview-mainhud-renderer-adapter-contract/Docs/Report/Dev.D.UE.0.0.10.P20.60.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-60-thrown-weapon-arc-preview-mainhud-renderer-adapter-contract/Docs/Log/Dev.D.UE.0.0.10.P20.60.r0_log.md>
