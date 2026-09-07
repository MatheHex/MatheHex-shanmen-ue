# Dev.D.UE.0.0.10.P20.65.r0 Report

## 1. 结论

P20.65 已把 MainHUD 中五条分散绘制的投掷武器提示收束为一个纯只读 combat-hint stack。现有 trajectory、Arc apex、Arc target、Arc input 与 pre-launch gesture 投影仍各自拥有原有事实边界；新层只验证它们是否一致、复制显示文本并输出唯一的确定顺序，不创建第二套 choice、context、preview、input 或 UI 权威。

MainHUD 现在只遍历一份组合结果，并以一个底部锚点和一个行距绘制全部有效行。完整 Arc 栈从下到上固定为：trajectory mode → apex → target → edit input → pre-launch gesture。Straight 模式只显示 trajectory；某个可选投影不可用时保留其它仍然可靠的行；跨模式、旧 input hint 或 target/gesture 互相矛盾时失败关闭。

正式自动化 1345/0：提示栈 3/0、InputRestore 101/0、V2RangedCompatibility 22/0、0.0.10 全量 1219/0。全量较 P20.64 的 1216 增加 3 项，正好对应本轮新增测试。映射自检 421/421；changed-file regression 对 4 个生产改动路径推导出 13 个必跑组，全部有健康日志覆盖。Game 构建 5/5 actions，Editor 构建 up to date，均 native 0。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：213a94dac0208918a9ccdb2298aa8212cd39c3db（P20.64）；
- 分支：agent/0.0.10-p20-65-mainhud-combat-hint-stack；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 玩家可见行为

1. Straight 模式继续显示当前 trajectory 与切换键，不出现 Arc 专属信息；
2. BallisticArc 模式按固定层次显示 mode、apex、target、编辑按键和已 armed 手势；
3. target 未设置时，最上层提示要求设置 target；
4. target 与物理 preview 都是当前事实时，最上层提示允许再次按同槽投掷；
5. 输入绑定标签缺失只移除依赖该标签的行，不会抹掉其它可靠 Arc 数值；
6. 旧 Arc input hint、Straight/Arc 混搭或 target 状态冲突不会被绘制成貌似可执行的组合。

完整五行栈保持 P20.64 的大致视觉位置：底部锚点 112 px，统一行距 22.5 px，最上层 gesture 仍位于底部上方 202 px。颜色与字号按语义 tone 集中映射，DrawHUD 不再为每一种提示单独写位置、颜色和比例。

## 4. 组合契约

新增 `Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation`：

- 输入只接受四种既有不可变 presentation；
- 输出为 renderer-neutral line 数组，每行只有 kind、tone 与 display text；
- 顺序严格递增且同类不可重复；
- Straight 只能包含一个 cool trajectory line；
- Arc apex 与 target 必须成对出现；
- Arc input 必须能由当前 Arc presentation 和同一组键名重新投影出完全相同文本；
- gesture 必须与当前 Arc 是否已有 target 一致；
- `Matches` 对同一输入重放给出完全相同的 mode、标志、行数、顺序、tone 与文本；
- 任意失败都先清空复用输出。

组合器允许两种有界降级：只有 Arc trajectory 时仍显示 mode；toggle 标签不可用但 Arc 数值有效时仍显示 apex 与 target。孤立 input/gesture 不被接受，因为它们无法证明所描述的当前 Arc 来源。

## 5. MainHUD 接线

`Ademo_mapHUD::DrawHUD` 仍只读取一次当前 thrown-weapon interaction read，并继续使用：

- GameMode 的唯一 pre-launch context；
- renderer adapter 的唯一 physical surface cursor；
- input settings 的当前物理键名；
- 原 trajectory、Arc editing、Arc input hint 与 gesture 投影。

这些投影完成后统一交给 stack composer。HUD 不缓存 stack，不修改任何输入或产品状态，也没有新增成员字段。绘制循环只把 line tone 映射为既有颜色和字号。

## 6. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| P20.65.r0_hint_stack.log | Product.ThrownWeaponMainHUDCombatHintStackPresentation | 3/0 | C479A37AE5E9CDD288EFC9E50C62397D09EC8F747109804CB4C59E2C2FE0A50F |
| P20.65.r0_input_restore.log | demo_map.InputRestore | 101/0 | 288E13E5060B1B83D9C7807C2DB1EECA78CD05BE560023ADC97D3117278A9748 |
| P20.65.r0_ranged.log | demo_map.V2RangedCompatibility | 22/0 | 070921DA1DF3C04A1E8B4F75F22486B5FC910A4602899E7EAAD3FBAFEE464CE3 |
| P20.65.r0_full.log | Shanmen.0_0_10 | 1219/0 | 1E09774CE4D4D050877471C53B12A1B61EB007C2FA9737321DCB9708866D944C |

四份正式日志合计 1345 Success、0 Fail，均含 UE 5.8 native terminal-success marker；Fatal、Unhandled 与 Ensure 命中均为 0。

新增测试覆盖：

- Straight 与完整 targetless Arc 的确定顺序、tone 和重放一致性；
- trajectory-only 与 Arc-values-only 两种有界降级；
- 旧 apex-limit input hint、Straight/Arc 混搭、target/gesture 冲突与孤立来源失败关闭；
- 失败后复用输出恢复为空。

## 7. 改动—回归映射

新增 `ThrownWeaponMainHUDCombatHintStackPresentation` 规则，并把该焦点组加入 `MainHUDTrajectoryPresentation` 规则。映射要求组合器同时具备四个来源投影、0.0.10 全量、InputRestore 与 V2RangedCompatibility 证据。

- 映射自检：421/421；SHA-256 B381528EE1DAD4F09DE630AB9286CDE0B50568ACA0FFFA0E08626B79FB7409F5；
- changed-file gate：PASS，Changed=4 / Rules=2 / Required=13 / Logs=4；
- gate SHA-256：422AF4A99D93C5E0FD26EC8BF7C5C85AA5C203A29F9D641700A76FF17F571CB3。

## 8. 静态审计与构建

- 新生产组合器：2 files / 428 lines；
- World/Actor/RNG/damage/item/profile authority 命中：0；
- begin/end/cancel/update/clear/route/commit/reserve/reduce/apply mutation 调用命中：0；
- static log：118 bytes；SHA-256 CFFDE3E69089F54984F427439390D8B98EB334292BDD95E52CBD4C2C2535AE6B；
- source/test/mapping：6 files / 841 additions / 50 deletions；
- `git diff --check`：PASS，仅 Windows LF→CRLF 工作区提示；
- 103 份用户已有 untracked 文档保持未暂存、未修改；
- 原始自动化与构建日志继续只作本地证据，不进入提交。

最终构建：

- Game：5 actions / 28.24 s / Succeeded / native 0；log SHA-256 34056D05AB6B72C347EEF2227BB69C2AD75B0A2306CA7A5D061300552E62B468；
- Editor：up to date / 0 actions / 0.91 s / Succeeded / native 0；log SHA-256 6DC0EDD11614ECCBFADD53BAF29295F6B25E8931EC053058196E48DB0CCB7CF1；
- `demo_map.exe`：359,285,248 bytes / SHA-256 FD0A8BE0D53CBF42F5236BC91271C02717E26CAE256F24F16856AB81CBE0FCF1；
- `UnrealEditor-demo_map.dll`：18,421,248 bytes / SHA-256 C288D5518CE021326CA65F1F485DA5CD133337013DB9DC177CF25067613D7724。

## 9. P/F 边界

PASS：只读组合、确定顺序、同源 input 复核、target/gesture 一致性、有界降级、失败输出清空、统一 HUD 绘制、重放一致性、旧输入、旧远程武器、0.0.10 全量、回归映射、Game/Editor build。

未声明：真实 viewport 可读性、DPI 缩放、小分辨率裁切、与其它 HUD 面板遮挡、颜色对比、真实输入手感、Unreal Editor UI、PIE、Standalone、产品可执行文件、截图、Smoke、Cook、Package。

## 10. GitHub 与下一阶段

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-65-mainhud-combat-hint-stack>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-65-mainhud-combat-hint-stack/Docs/Report/Dev.D.UE.0.0.10.P20.65.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-65-mainhud-combat-hint-stack/Docs/Log/Dev.D.UE.0.0.10.P20.65.r0_log.md>

建议 P20.66 建立纯 viewport-safe hint layout policy：根据 canvas 高度和可用底部区域为 stack 计算锚点、行距与可见预算，并对小视口失败关闭或有序裁剪；仍不启动真实界面，真实视觉验收继续单独授权。
