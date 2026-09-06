# Dev.D.UE.0.0.10.P20.60.r0 Development Log

## 基线与目标

- base：18cd799469f8e898691e2c7c53bd2c3c5ad7e14a；
- branch：agent/0.0.10-p20-60-thrown-weapon-arc-preview-mainhud-renderer-adapter-contract；
- 目标：把既有 Arc preview presentation command stack 落到 MainHUD 的一个显式物理 renderer endpoint；
- 权限边界：adapter 只保存 physical surface cursor；不拥有 Run、product、delivery、recovery、storage 或 retry authority；
- 运行边界：本轮只做 headless automation 与编译，不启动 Editor UI、PIE、Standalone 或产品。

## 设计判断

1. P20.23–P20.59 已形成 command、delivery、consumer、surface lifecycle、ownership handoff 与 recovery 链，但没有真实 HUD surface。
2. MainHUD endpoint 必须实现既有 handoff surface interface，不能复制一套 command 或 acknowledgement 类型。
3. stable consumer definition 与 physical instance identity 是两件事：前者跨实例稳定，后者每个 HUD 实例唯一。
4. renderer 只需要 visible-or-empty cursor；hidden/tombstone 语义仍留在 delivery ledger，避免物理层成为第二权威。
5. command kind、previous cursor、permit/ticket snapshot 必须在 mutation 前精确匹配；拒绝不能修改 cursor。
6. DrawHUD 可以读取 cursor 并投影，但不能轮询上游、生成命令或推进 lifecycle。
7. product/composition owner 接线与 renderer endpoint 应拆阶段；否则无头契约失败和真实 UI 失败难以归因。

## 实现记录

新增：

- demo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter.h；
- demo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter.cpp。

核心行为：

- non-copyable adapter；
- `TryInitialize` 接受 caller-supplied physical `SurfaceInstanceId`；
- stable consumer definition 为 `Renderer.ArcPreview.MainHUD.r1`；
- `Show` / `Replace` / `Hide` 通过同一 bounded mutation path；
- visible state 原样保存，hidden 归一为空；
- wrong entrypoint 产生 typed rejection（可构造时），stale cursor/NoOp mutation 失败关闭；
- `ClearToEmpty` 只接受 exact cleanup permit；
- `RetireForHandoff` 只接受 exact AdoptExact ownership transition ticket；
- 所有成功路径先构造并验证 typed response，再提交 cursor mutation。

修改 MainHUD：

- HUD 直接持有 adapter；
- BeginPlay 生成新 physical identity 并初始化；
- DrawHUD 读取当前 cursor；
- segments 投影为 cyan lines；
- apex / planned landing point 投影为 amber / green cross markers；
- 投影失败局部跳过，不写 product state。

## 自动化测试

新增 5 项：

- InitializationAndPhysicalIdentity；
- DeliverySequence；
- DirectCommandFences；
- ExactCleanupPermit；
- ExactHandoffRetirement。

正式结果：

- focused：5/0，native 0，72.993 s，269,165 bytes，SHA-256 C405DF778070BBD00D65E37F60BD0D9B338088E45C9B4071871B210DCCC63101；
- full Shanmen.0_0_10：1203/0，native 0，4215.998 s，1,849,863 bytes，SHA-256 2363D677380218186B77D6B905975604CC44AB1F6C148224468371F1A91011A2；
- InputRestore：101/0，native 0，25.291 s，395,377 bytes，SHA-256 64F45B6BE4F8C4FE6E51C64B50FE6377AA32A8A0ADB79890F60FAC5090098C72；
- ItemUseAndArmor：46/0，native 0，17.164 s，309,560 bytes，SHA-256 6FF47A2A7E9A35FEAD3357385D9B1B02A29DF58E108076D0C76A062F8C63B6F2；
- V2RangedCompatibility：22/0，native 0，16.516 s，285,182 bytes，SHA-256 BD0E961A20A976C7636AB5FFE540F6E0EFC5CB2FB7386AA042F713CF55E8B5EA；
- formal invocation total：1377/0（包含 focused/full overlap）；
- terminal-success markers：5/5；
- selected automation failure / Fatal / Unhandled / Ensure：0 / 0 / 0 / 0；
- focused 首轮即 5/0，因此无 first-failure artifact。

启动噪声说明：UE 5.8 每个进程在 Engine 初始化前、引擎自身 UnifiedErrorTest 输出附近产生 13 条 generic `LogAutomationTest: Error: Condition failed`。它们不属于已选择的测试队列；所有 selected result、原生终止码与健康校验均通过，因此保留日志但不判定为本阶段失败。

## Changed-file Regression

- mapping JSON：231 rules / parse PASS；
- MainHUD trajectory rule 扩展为同时匹配 h/cpp，并要求新的 renderer focused group；
- 新增 exact MainHUD renderer adapter rule；
- 该规则要求 focused、owner handoff、ownership transition、lifecycle executor、recreation policy、consumer adapter、delivery session、presentation command、presentation、完整 Shanmen、InputRestore、ItemUseAndArmor 与 V2RangedCompatibility 等 30 组最终并集；
- renderer focused group 回接 7 条上游 surface/presentation 规则，修改上游时不能漏跑 endpoint；
- self-test：413/413；
- focused-only fixture：预期失败；
- final gate：PASS，Changed=7 / Rules=3 / Required=30 / Logs=5。

## 静态边界

- adapter：2 files / 335 physical lines；
- adapter 中 Tick/timer/async/save/file access/World/Actor/UObject/RNG：0；
- adapter 中 TODO/FIXME/HACK：0；
- authority/storage/ledger/scheduler/retry policy：0；
- `git diff --check`：PASS；
- 编译警告只有工作区既有 LF→CRLF 提示，没有 whitespace error。

## 构建与产物

- initial Editor：Succeeded / native 0 / 42.49 s；
- tests integration Editor：Succeeded / native 0 / 10.71 s；
- initial Game：Succeeded / native 0 / 50.74 s；
- final Editor：Succeeded / native 0 / 0 actions / 1.21 s；
- final Game：Succeeded / native 0 / 0 actions / 0.98 s；
- Editor DLL：18,284,032 bytes / SHA-256 032861DD317A7C765A2CEB3A8A3982E060EB2C0C706EE7552DE903FA1DAF6854；
- Game EXE：359,166,464 bytes / SHA-256 50A5D281B39D09775DBC906007E3AFA0B65F36E7626A4BC20FE562327D8C6168。

## P/F 边界

PASS：物理 HUD surface identity、稳定 consumer identity、Show/Replace/Hide endpoint、typed response、replay/NoOp separation、stale/wrong-entrypoint fences、exact cleanup、exact handoff retirement、Canvas projection implementation、mapped regressions、Editor/Game builds。

未声明：active Run composition owner 到 HUD adapter 的运行时 binding、真实 delivery/UI/input/viewport、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.61：把现有 active Run composition owner / delivery host 显式绑定到 HUD adapter，并在 HUD recreation/destruction 时使用既有 lifecycle 与 handoff authority 完成解除/迁移。禁止 DrawHUD/Tick polling、第二份 preview state 或绕过 command ledger；先完成 headless composition tests，真实 UI 运行另行授权。
