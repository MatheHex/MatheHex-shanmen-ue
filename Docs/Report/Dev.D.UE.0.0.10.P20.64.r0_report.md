# Dev.D.UE.0.0.10.P20.64.r0 Report

## 1. 结论

P20.64 已把 P20.63 的唯一 BallisticArc 发射前选择上下文投影为 MainHUD 可见的手势反馈：玩家能看到当前已选热栏槽；尚无目标时会收到设置目标提示；已有目标且物理弧线确实与当前 choice 一致时，会收到编辑目标/弧顶并再次按同槽投掷的提示。

该反馈是纯只读投影。它直接读取唯一 pre-launch context、既有 interaction read 与 MainHUD 物理 surface cursor，不保存第二份选择、choice、preview、物品、发射或 UI 权威。旧 surface、跨 Run surface、空 key label、非 Arc choice、未 armed context 与不完整读取均失败关闭，因此文字不会提前承诺一次实际不可确认的投掷。

正式自动化 1388/0：手势反馈焦点 3/0、三组旧系统兼容 169/0、0.0.10 全量 1216/0。全量从 P20.63 的 1213 增至 1216，增量正好是本轮两项纯投影测试与一项真实生命周期组合测试。映射自检 419/419；changed-file regression 对 7 个源码/脚本文件推导出 25 个必跑组，全部有健康日志覆盖。Game 构建 6/6 actions，Editor 构建 up to date，均 native 0。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：89fe2ef1fad1ee42067a59f7a286adf8a8e53d00（P20.63）；
- 分支：agent/0.0.10-p20-64-thrown-weapon-prelaunch-gesture-feedback；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 玩家可见行为

反馈只在唯一 pre-launch context 已选择热栏槽时出现：

1. BallisticArc 已 armed、但尚无 target：显示 ARC PREVIEW: SLOT N [Key] ARMED | SET TARGET TO SHOW ARC；
2. BallisticArc 已 armed、已有 target，并且 MainHUD 的物理 preview cursor 与当前 choice、Run 完全一致：显示 ARC PREVIEW: SLOT N [Key] ARMED | EDIT TARGET/APEX | PRESS [Key] AGAIN TO THROW；
3. target 已改但 surface 仍显示旧 choice：不显示“可确认”提示；
4. targetless choice 却仍有旧物理 surface：不显示提示；
5. context 取消、Run 结束、读取失败或 key label 为空：不显示残留提示。

热栏键名由现有 input registry 和当前 input settings 读取，因此重新绑定后文字使用当前物理键名，不写死数字键。

## 4. 实现与权威边界

新增 Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation：

- 输入为唯一 Run context、revisionless choice read、物理 preview cursor 与键名；
- 输出只包含 mode、RunId、armed slot、context revision、规范化键名和完整显示文本；
- TargetRequired 要求 targetless Arc 与空物理 surface；
- ReadyToConfirm 要求 targeted Arc、可见 surface、相同 Run，并将 surface 内冻结 choice 重新投影后与当前 read model 精确匹配；
- 每次失败先把复用输出恢复为规范 invalid 值；
- 不调用 context、choice、surface、item、launch 或 World 的任何修改入口。

Ademo_mapHUD::DrawHUD 复用一次现有 choice read，并从 GameMode 读取 const pre-launch context、从既有 renderer adapter 读取 const physical cursor。投影成功后才绘制一行小字体提示：TargetRequired 使用橙色，ReadyToConfirm 使用绿色。HUD 类没有新增持久字段。

## 5. 失败关闭与一致性

以下情况均不会产生提示：

- context 未激活、未 armed、槽位不在 1–9、revision 非法；
- interaction read 未成功投影；
- choice 不是 BallisticArc，或缺少既有 trajectory/target 编辑能力；
- 当前输入绑定无法提供非空显示名；
- targetless choice 对应非空 surface；
- targeted choice 对应空、隐藏、非法或跨 Run surface；
- surface 冻结 choice 无法投影，或与当前 choice 不匹配。

这使“再次按同槽投掷”成为对当前物理事实的描述，而不是另一套可执行状态机。

## 6. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| P20.64.r0_gesture_feedback.log | Product.ThrownWeaponArcPreLaunchGestureFeedbackPresentation | 3/0 | 12E491862834E1F44F488F93331F50342E1E9F484EDBD6F7C9C4EEFB22D189EF |
| P20.64.r0_input_restore.log | demo_map.InputRestore | 101/0 | 05CE8FD297E28CB1EAEA70E19FDE8ADB212E223A04CC368046031AE66C0C0C11 |
| P20.64.r0_ranged.log | demo_map.V2RangedCompatibility | 22/0 | 9E44DE4D0DE8831B477159876EC8B22C84FF6E54B8253434553EE46DF8AD4BA1 |
| P20.64.r0_item_use.log | demo_map.ItemUseAndArmor | 46/0 | C7E764BAC2705A87A2EA3E934E14C9A66B719054E94A0C2E6B24BB32CFBCCAC6 |
| P20.64.r0_full.log | Shanmen.0_0_10 | 1216/0 | 1B9612C23DA3D37BB33A1B37AA91E7EB3EE5CE0C5A06368F56820F5E464D4AE2 |

五份正式自动化日志合计 1388 Success、0 Fail，均含 native terminal-success marker，Fatal/Unhandled/Ensure 为 0。

首次焦点运行保留为 P20.64.r0_gesture_feedback_first_failure.log：前两项成功后，组合测试使用向上 1 米的弧顶调整越过策略上界，preview 正确拒绝，测试中的 check 因错误夹具输入终止；同时相对 cvars 路径被 UE 从引擎目录解析并触发 ensure。修复为边界内向下调整并使用绝对 cvars 路径后，焦点与全量均通过。该首败日志 268,900 bytes，SHA-256 062097951F7630B37309138A639A06D06484137639E624073D7E07F744DB7B60。

## 7. 关键行为证据

自动化分别证明：

- targetless armed Arc 生成唯一 TargetRequired 提示；
- 键名会去除首尾空白，空键名失败关闭；
- inactive context、targeted choice + empty surface 均拒绝；
- 当前可见物理 cursor 生成 same-slot confirmation 提示；
- 投影不改变 context revision 或 surface presentation state identity；
- choice revision 更新而 surface 尚未刷新时拒绝旧 cursor；
- runtime binding 刷新物理 cursor 后提示恢复；
- clear + cancel 后不产生残留反馈；
- InputRestore、V2RangedCompatibility 与 ItemUseAndArmor 保持通过。

## 8. Changed-file Regression 与静态边界

- mapping self-test：419/419，41,548 bytes，SHA-256 7A5FB81D22DF1AC50F084EAF4BF58C4348DFC64C6E7C30732F92F8DEF9F5F286；
- changed-file gate：PASS Changed=7 Rules=3 Required=25 Logs=5；
- gate SHA-256：4EF71223386C54737A7BC711489CA24F039C97C49D498D9FA5266602D5DF57CA；
- 新纯生产投影：2 files / 205 lines；
- World/Actor/UObject、Tick/timer/async、inventory、damage、spawn、trace/sweep、RNG、TODO/FIXME/HACK：0；
- 生产投影中的 begin/end/cancel/update/clear/route/complete/commit/reserve/reduce 修改调用：0；
- 静态日志 SHA-256：735F23EC9C5262D7A0E33350EB70A0363658A8092B264D346779C919AAF05154；
- 主体源码/测试/映射改动：7 files / 595 additions / 0 deletions；
- git diff --check：PASS，仅有工作区既有 LF→CRLF 提示；
- 用户已有约百份未跟踪 Prompt、Report 与文档未暂存、未修改。

## 9. 构建与产物

- final Game：6 actions / Succeeded / native 0 / 21.69 s；构建日志 SHA-256 3D363C864E58A4EF529A642DA895C315010763AF407B1E8DA29BB385B60B8912；
- final Editor：up to date / 0 actions / Succeeded / native 0 / 0.90 s；构建日志 SHA-256 D6FC7F1EEF27DB58030B95C21644ED310901ABEFD7CFD1C2E62430CA7CD553AC。

产物：

- Binaries/Win64/demo_map.exe：359,264,768 bytes，SHA-256 7B7B5F62C63116BC94052B1C33849EBDF137DE60D959C1AE4D39C1C1E0B1C2CE；
- Binaries/Win64/UnrealEditor-demo_map.dll：18,395,648 bytes，SHA-256 85DC68A2D7C7A9C42103DD621EB861A6949729E57B15F113DEF7C1079E4A523B。

## 10. P/F 边界与下一步

PASS：armed slot 只读显示、target-required 提示、same-slot confirmation 提示、rebind-aware 键名、当前物理 cursor 精确匹配、旧/跨 Run/空 cursor 拒绝、失败输出清空、取消无残留、唯一 context/choice/surface/launch 权威不变、旧系统兼容、全量回归、Editor/Game 构建。

未声明：真实屏幕中的可读性、viewport/DPI/遮挡、颜色对比、真实键盘/鼠标手感、Unreal Editor UI、PIE、Standalone、截图、Smoke、Cook 或 Package。

建议 P20.65 建立纯 MainHUD combat-hint stack composition：把 gesture、Arc edit 与 trajectory 三类既有只读提示按模式和优先级组合成一个确定顺序，消除硬编码位置之间未来互相覆盖的风险；仍不创建 UI 状态或启动真实界面。真实视觉验收继续作为单独授权阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-64-thrown-weapon-prelaunch-gesture-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-64-thrown-weapon-prelaunch-gesture-feedback/Docs/Report/Dev.D.UE.0.0.10.P20.64.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-64-thrown-weapon-prelaunch-gesture-feedback/Docs/Log/Dev.D.UE.0.0.10.P20.64.r0_log.md>
