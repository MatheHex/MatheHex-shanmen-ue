# Dev.D.UE.0.0.10.P20.64.r0 Development Log

## 基线与目标

- base：89fe2ef1fad1ee42067a59f7a286adf8a8e53d00；
- branch：agent/0.0.10-p20-64-thrown-weapon-prelaunch-gesture-feedback；
- 目标：将 P20.63 的唯一 BallisticArc 发射前选择投影为 MainHUD 可操作提示，使玩家明确知道已选槽、是否缺 target、以及何时可再次按同槽确认；
- 边界：不复制 context、choice、preview、item、launch、Run、World 或 UI 权威；不启动产品界面。

## 调查记录

1. P20.63 已建立 Run-scoped pre-launch context，但 MainHUD 只能显示 trajectory 模式、Arc target/apex 与编辑键，不知道哪个热栏槽被 armed，也不知道同槽二次按下何时才是真正可确认。
2. context 只保存 RunId、armed slot 与 revision，适合作为“玩家意图已 armed”的唯一来源。
3. interaction port 已提供 revisionless choice read；MainHUD renderer adapter 已提供实际物理 surface cursor。两者组合可验证“当前 choice 已真实显示”，不需要再保存一份提示状态。
4. input registry 已提供 HotbarActionId(slot)，可复用当前绑定，不应在提示中写死数字键。
5. MainHUD 现有底部提示位于 Y=-180、-158、-136、-112；新增反馈放在 Y=-202，与既有 Arc 信息形成连续 22 像素栈。

## 投影契约

新增 mode：

- TargetRequired：context 已 armed、choice 是 targetless BallisticArc、物理 surface 为空；
- ReadyToConfirm：context 已 armed、choice 是 targeted BallisticArc、物理 surface 可见、Run 匹配，且 surface 内冻结 choice 与当前 read model 精确一致；
- Invalid：所有其它状态。

输出只保存 mode、RunId、armed slot、context revision、规范化 key label 与由这些值确定生成的文本。TryProject 入口先清空 OutPresentation，任何失败都留下规范 invalid 值。

## 生产实现

新增：

- demo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation.h；
- demo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation.cpp。

投影要求：

- active、valid、armed 的唯一 context；
- 1–9 槽位与有效 revision；
- projected interaction read；
- BallisticArc trajectory；
- 既有 SelectStraightTrajectory 与 SetArcTargetIntent capabilities；
- 非空、首尾规范化的物理 key label；
- 与 target 状态一致的物理 surface；
- targeted 路径必须满足 surface Run 和 choice 全量匹配。

MainHUD 接入：

- 在现有 DemoController 分支中只做一次 choice read；
- 从 ActiveMode 读取 const pre-launch context；
- 从现有 input settings 读取 armed slot 的当前 key display name；
- 从现有 MainHUD renderer adapter 读取 const surface cursor；
- 投影成功才绘制；
- TargetRequired 为橙色，ReadyToConfirm 为绿色；
- HUD header 未增加状态字段。

## 自动化改动

新增纯投影测试：

- TargetRequired：targetless armed Arc、键名裁剪、slot/revision/text 精确值；
- SourceFences：inactive context、空 key label、targeted choice + empty surface、失败后复用输出清空。

新增真实生命周期组合测试：

- VisibleCurrentAndStaleFences；
- 使用真实 thrown lifecycle fixture、combat Run coordinator、MainHUD runtime binding 与 renderer adapter；
- 当前物理 preview 生成 ready 提示；
- 投影前后 context revision 与 surface state identity 不变；
- choice apex revision 更新但 surface 未更新时，旧 cursor 失败关闭；
- runtime binding 刷新后 ready 提示恢复；
- clear + cancel 后无残留提示。

映射更新：

- MainHUD 规则新增本焦点组；
- 新投影规则要求 context、MainHUD runtime binding、renderer adapter、interaction port、full、InputRestore 与 V2RangedCompatibility；
- self-test 增加一个应通过组合与一个缺依赖应失败组合；
- 新 interaction-port fixture 补入自检。

## 首次失败与修正

首次焦点运行：

- 前两项纯投影测试 Success；
- 组合测试在第二次 Binding.TryUpdate 处触发 assertion；
- 原因不是生产实现：夹具使用向上 1.0 米的 apex adjustment，初始 choice 已位于策略上界，reducer 正常产生新 state，但 preview policy 正确拒绝越界几何；
- 同一运行使用相对 cvars 路径，UE 从引擎工作目录解析，触发“文件不存在”的 handled ensure；
- native exit：3；
- 原始日志保存为 P20.64.r0_gesture_feedback_first_failure.log，268,900 bytes，SHA-256 062097951F7630B37309138A639A06D06484137639E624073D7E07F744DB7B60。

有界修正：

- apex adjustment 改为边界内的 -1.0 米，与既有 live-edit 测试使用同一合法方向；
- cvarsini 改为项目文件的绝对路径；
- 重新编译成功；
- 重跑焦点 3/0；
- 全量中的同三项再次 3/0。

## 编译与正式自动化

- initial Editor integration build：7 actions / Succeeded / native 0；
- post-fix Editor build：4 actions / Succeeded / native 0；
- gesture feedback focus：3/0 / native 0；
- InputRestore：101/0 / native 0；
- V2RangedCompatibility：22/0 / native 0；
- ItemUseAndArmor：46/0 / native 0；
- full Shanmen.0_0_10：1216/0 / native 0；
- formal automation total：1388/0；
- terminal-success markers：5/5；
- formal logs Fatal/Unhandled/Ensure：0/0/0。

正式日志：

| File | Bytes | SHA-256 |
|---|---:|---|
| P20.64.r0_gesture_feedback.log | 258,387 | 12E491862834E1F44F488F93331F50342E1E9F484EDBD6F7C9C4EEFB22D189EF |
| P20.64.r0_input_restore.log | 388,037 | 05CE8FD297E28CB1EAEA70E19FDE8ADB212E223A04CC368046031AE66C0C0C11 |
| P20.64.r0_ranged.log | 278,226 | 9E44DE4D0DE8831B477159876EC8B22C84FF6E54B8253434553EE46DF8AD4BA1 |
| P20.64.r0_item_use.log | 304,285 | C7E764BAC2705A87A2EA3E934E14C9A66B719054E94A0C2E6B24BB32CFBCCAC6 |
| P20.64.r0_full.log | 1,820,515 | 1B9612C23DA3D37BB33A1B37AA91E7EB3EE5CE0C5A06368F56820F5E464D4AE2 |

## Changed-file Regression

- mapping JSON parse：PASS；
- mapping self-test：419/419；
- self-test log：41,548 bytes，SHA-256 7A5FB81D22DF1AC50F084EAF4BF58C4348DFC64C6E7C30732F92F8DEF9F5F286；
- changed files：7；
- matched rules：3；
- required groups：25；
- evidence logs：5；
- gate：REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=25 Logs=5；
- gate log：3,574 bytes，SHA-256 4EF71223386C54737A7BC711489CA24F039C97C49D498D9FA5266602D5DF57CA。

## 静态与差异审计

- pure feedback production：2 files / 205 lines；
- UWorld/AActor/UObject/Tick/timer/async/inventory/damage/spawn/trace/sweep/RNG/TODO/FIXME/HACK：0；
- begin/end/cancel/update/clear/route/complete/commit/reserve/reduce mutation calls：0；
- static log：320 bytes，SHA-256 735F23EC9C5262D7A0E33350EB70A0363658A8092B264D346779C919AAF05154；
- source/test/mapping：7 files / 595 additions / 0 deletions；
- git diff --check：PASS，仅 LF→CRLF 工作区提示；
- 约百份用户已有 untracked Prompt/Report/文档保持未暂存、未修改；
- 自动化 raw logs 与构建 raw logs仅作为本地证据，不纳入 Git 提交。

## 最终构建与产物

- Game：6 actions / 21.69 s / Succeeded / native 0；log 2,484 bytes / SHA-256 3D363C864E58A4EF529A642DA895C315010763AF407B1E8DA29BB385B60B8912；
- Editor：up to date / 0 actions / 0.90 s / Succeeded / native 0；log 1,014 bytes / SHA-256 D6FC7F1EEF27DB58030B95C21644ED310901ABEFD7CFD1C2E62430CA7CD553AC；
- demo_map.exe：359,264,768 bytes / SHA-256 7B7B5F62C63116BC94052B1C33849EBDF137DE60D959C1AE4D39C1C1E0B1C2CE；
- UnrealEditor-demo_map.dll：18,395,648 bytes / SHA-256 85DC68A2D7C7A9C42103DD621EB861A6949729E57B15F113DEF7C1079E4A523B。

## P/F 边界

PASS：target-required、ready-to-confirm、armed slot、rebind-aware key label、physical-current fence、stale choice/surface fence、Run fence、失败输出清空、取消无残留、投影零 mutation、旧兼容、全量、Game/Editor build。

未声明：真实 MainHUD 可读性、viewport/DPI/遮挡、颜色对比、真实输入手感、Unreal Editor UI、PIE、Standalone、产品可执行文件、截图、Smoke、Cook、Package。

## 下一阶段

P20.65 建议建立纯 MainHUD combat-hint stack composition：将 gesture feedback、Arc edit hint、target/apex 与 trajectory mode 组合成确定顺序和模式化显示计划，替代 DrawHUD 中继续增长的独立硬编码位置；仍复用全部现有只读 presentation，不拥有 UI 状态。真实视觉验收单独授权。
