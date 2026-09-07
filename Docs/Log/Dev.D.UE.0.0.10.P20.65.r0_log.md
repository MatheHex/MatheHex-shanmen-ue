# Dev.D.UE.0.0.10.P20.65.r0 Development Log

## 基线与目标

- base：213a94dac0208918a9ccdb2298aa8212cd39c3db；
- branch：agent/0.0.10-p20-65-mainhud-combat-hint-stack；
- 目标：把 MainHUD 的 thrown-weapon gesture、Arc input、target、apex 与 trajectory 提示组合为一个确定、只读、renderer-neutral 的提示栈；
- 边界：不复制 choice、context、preview、input、Run、item、launch、World 或 UI 权威，不启动产品界面。

## 调查

P20.64 后，五条信息分别在 `DrawHUD` 中以 Y=-202、-180、-158、-136、-112 绘制；每条又独立选择颜色与比例。信息来源正确，但布局关系只存在于过程式调用顺序里，新增提示时容易重叠或颠倒。

既有四个 presentation 已足够组成栈：trajectory 提供当前模式，Arc editing 提供 target/apex，Arc input hint 提供当前绑定，gesture 提供 armed/target-required/ready-to-confirm。无需读取底层可变状态或引入 HUD 成员。

## 组合模型

新增：

- `Edemo_mapShanmenThrownWeaponMainHUDCombatHintStackMode`：Straight / BallisticArc；
- `Edemo_mapShanmenThrownWeaponMainHUDCombatHintKind`：trajectory / apex / target / input / gesture；
- `Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone`：模式、数值、控制、缺目标、可确认语义；
- immutable line：kind + tone + text；
- immutable stack：mode + Arc target presence + bottom-to-top lines。

规范顺序为 trajectory(0)、apex(1)、target(2)、input(3)、gesture(4)。`IsValid` 要求 rank 严格递增，因此重复或倒序在结构上无效；`Matches` 支持确定性重放核验。

## 一致性与降级

1. Straight 与任何 Arc 来源同时出现即拒绝；
2. apex/target 来源于同一个 Arc presentation，始终成对加入；
3. input hint 使用自身规范键名从当前 Arc presentation 重新投影，文本不一致即拒绝；
4. target-required 只匹配 targetless Arc，ready-to-confirm 只匹配 targeted Arc；
5. orphan input 或 gesture 因缺少 Arc 来源而拒绝；
6. trajectory-only Arc 与 Arc-values-only 是允许的有界降级；
7. 所有失败先把 `OutStack` 还原为 canonical empty。

## MainHUD 接线

`DrawHUD` 仍读取一次 interaction read，分别尝试原四类投影，然后只调用一次 `TryCompose`。成功后按数组顺序绘制：

- BottomAnchor = 112.0；
- LineSpacing = 22.5；
- 颜色和 scale 由 tone 的两个局部纯映射函数统一解析。

完整五行时 gesture 仍落在 -202 px；其余行只发生 0–1.5 px 的规范化。HUD 类未新增字段，也不持久化 stack。

## 自动化实现

新增 3 项 `Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintStackPresentation` 测试：

- `DeterministicOrder`：Straight 单行、完整 targetless Arc 五行、tone、顺序与 replay；
- `PartialSources`：trajectory-only Arc 和 Arc-values-only；
- `SourceFences`：stale limit hint、cross-mode、target/gesture mismatch、orphan sources 与失败清空。

首次 Editor build：6 actions / 15.49 s / Succeeded。首次焦点运行 3/0，无首败修复轮；正式焦点另行重跑并记录下表。

## 回归映射

`ShanmenRegressionMap.json`：

- MainHUD 规则新增本阶段焦点组；
- 新组合器规则要求四个来源投影、0.0.10 全量、InputRestore 与 V2RangedCompatibility。

`Test-ShanmenRegressionCoverageSelfTest.ps1`：

- 新增健康焦点 fixture；
- 新增组合器正例；
- 新增“单独焦点不能替代来源和兼容证据”反例；
- MainHUD 正例补入组合器日志。

映射 JSON 解析成功；self-test 421/421，41,770 bytes，SHA-256 B381528EE1DAD4F09DE630AB9286CDE0B50568ACA0FFFA0E08626B79FB7409F5。

## 正式自动化

| Log | Success/Fail | Bytes | SHA-256 |
|---|---:|---:|---|
| P20.65.r0_hint_stack.log | 3/0 | 257,682 | C479A37AE5E9CDD288EFC9E50C62397D09EC8F747109804CB4C59E2C2FE0A50F |
| P20.65.r0_input_restore.log | 101/0 | 388,037 | 288E13E5060B1B83D9C7807C2DB1EECA78CD05BE560023ADC97D3117278A9748 |
| P20.65.r0_ranged.log | 22/0 | 278,227 | 070921DA1DF3C04A1E8B4F75F22486B5FC910A4602899E7EAAD3FBAFEE464CE3 |
| P20.65.r0_full.log | 1219/0 | 1,823,668 | 1E09774CE4D4D050877471C53B12A1B61EB007C2FA9737321DCB9708866D944C |

合计 1345/0。四份日志均有 native terminal-success marker，Fatal/Unhandled/Ensure=0。全量运行跨 heartbeat 保持同一个 UnrealEditor-Cmd 实例，从 07:21 至 08:24 自然清空，没有重启或拼接结果。

changed-file gate：`REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=13 Logs=4`；2,238 bytes；SHA-256 422AF4A99D93C5E0FD26EC8BF7C5C85AA5C203A29F9D641700A76FF17F571CB3。

## 静态与差异审计

- 新生产组合器：2 files / 428 lines；
- World/Actor/RNG/damage/item/profile authority hits：0；
- begin/end/cancel/update/clear/route/commit/reserve/reduce/apply mutation hits：0；
- static log：118 bytes / SHA-256 CFFDE3E69089F54984F427439390D8B98EB334292BDD95E52CBD4C2C2535AE6B；
- source/test/mapping：6 files / +841 / -50；
- `git diff --check`：PASS，仅 LF→CRLF 提示；
- 103 份用户原有 untracked 文件保持未暂存；
- raw logs 不进入 Git。

## 最终构建与产物

- Game：5 actions / 28.24 s / Succeeded / native 0；log 2,343 bytes / SHA-256 34056D05AB6B72C347EEF2227BB69C2AD75B0A2306CA7A5D061300552E62B468；
- Editor：0 actions / 0.91 s / Succeeded / native 0；log 1,021 bytes / SHA-256 6DC0EDD11614ECCBFADD53BAF29295F6B25E8931EC053058196E48DB0CCB7CF1；
- `demo_map.exe`：359,285,248 bytes / SHA-256 FD0A8BE0D53CBF42F5236BC91271C02717E26CAE256F24F16856AB81CBE0FCF1；
- `UnrealEditor-demo_map.dll`：18,421,248 bytes / SHA-256 C288D5518CE021326CA65F1F485DA5CD133337013DB9DC177CF25067613D7724。

## P/F 边界

PASS：纯组合、确定顺序、来源一致性、有界降级、失败清空、HUD 单循环绘制、3 项焦点、旧输入、旧远程武器、全量、映射门禁、Game/Editor build。

未声明：真实 viewport、DPI、遮挡、颜色对比、真实输入、Editor UI、PIE、Standalone、产品 executable、截图、Smoke、Cook、Package。

## 下一阶段

P20.66 建议增加纯 viewport-safe hint layout policy：根据可用 canvas 高度计算锚点、间距和行预算，在小视口中确定性压缩或失败关闭；继续复用当前 stack，不拥有 UI 状态。真实视觉验收单独授权。
