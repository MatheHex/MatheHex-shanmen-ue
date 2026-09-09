# Dev.D.UE.0.0.10.P23.4.r0 Report

## 1. 结论

P23.4 在 P 阶段边界内完成，结论为 **PASS**。

本轮关闭了神识多目标可读性缺口：同一次脉冲的 Reveal 仍按权威回执中的最近到最远顺序处理，最近目标保持原始投影像素；后续重叠目标按固定 HUD 文本足迹寻找确定性最小避让位置。屏外目标只沿原安全边缘移动，箭头方向不变。没有改变扫描、排序、距离、遮挡、资源或过期权威。

```text
Divine Sense HUD presentation:          12 Success / 0 Fail
Post-build Divine Sense tree:           60 Success / 0 Fail
Changed-file mapped regression:        212 Success / 0 Fail (18 exact logs)
Changed-file regression coverage:       PASS (4 paths / 2 rules / 18 groups)
Regression gate self-test:               PASS 442/442
Game + Editor Development:               PASS (both native 0)
git diff --check:                         PASS
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明最终编译 DLL 上的纯布局算法、神识产品链和主 HUD 相邻功能，不声明人工视觉或手感验收已经完成。

## 2. 玩家侧变化

- 多个敌人投影到同一区域时，最近目标的菱形和距离文字保持原位；
- 后续目标不再直接覆盖前项，而是在 136px 水平、32px 垂直的可读足迹上寻找最近槽位；
- 屏边目标始终留在原安全边缘，方向箭头仍指向原目标方位；
- 当前产品上限 8 个 Reveal 已有同点满容量自动化证明；
- 极小视口或饱和边缘无法容纳后续项时，该项失败关闭，顶部战术摘要仍显示权威总接触数。

## 3. 单一权威与顺序

`Fdemo_mapShanmenDivineSenseHUDMarkerPlan::TryDeconflict` 扩展既有纯屏幕计划，而不是建立新 Manager、Subsystem 或第二套 Reveal 状态。输入只有一个已经有效的基础计划和此前已占用的屏幕位置；输出仍是同一计划类型。

主 HUD 在一次 `DrawDivineSenseReveals` 调用内创建局部占位数组，并按 `Receipt.GetReveals()` 既有顺序逐项添加。数组不跨帧、不持久化，最多对应当前 canonical 8 个结果。算法不读取 Actor、不重新投影 World、不改变 Reveal，也不重排或聚合回执。

## 4. 确定性避让

基础位置无冲突时直接原样返回，因此第一项保持精确。发生冲突时，普通世界标记按固定方向优先级和最多 8 圈的二维足迹网格搜索；屏边标记只沿边缘切线负向、正向依次搜索。所有候选都必须位于既有 HUD 安全区内。

相同基础计划与相同占用序列产生完全相同的槽位。输出可与输入原地复用；方法先冻结基础计划，再清空输出，避免参数别名导致计划丢失。

## 5. 结构门与降级

下列输入会失败并清空输出：

- 无效基础计划；
- 非有限占用坐标；
- 不在同一 Canvas 安全区内的占用坐标；
- 有界 8 圈内不存在满足足迹和边缘约束的候选。

降级只是不绘制无法安全放置的后续 marker。权威 Receipt、战术摘要、SpiritEnergy、Reveal 有效期和其它已成功放置的标记不受影响。

## 6. 自动化覆盖

`Shanmen.0_0_10.Product.DivineSenseHUDPresentation` 最终为 12/0。新增 4 项覆盖：

- 世界投影重叠时最近项保持精确、后续项固定向上避让并可重放；
- 8 个同点目标全部得到互不覆盖且可重放的槽位；
- 屏边重叠只沿边缘移动并保留箭头方向；
- 最小视口饱和、非有限占用与输出复用/别名均失败关闭或正确保持。

聚焦日志为 267,111 bytes，SHA-256 `9F70394992D0AA397D65E76A0F5FEC7365C9746409E296234BDB619A39C37E21`。最终完整 `Shanmen.0_0_10.Product.DivineSense` 为 60/0，日志 336,398 bytes，SHA-256 `149B75D15F1DF8496FC4C7AEF011BC68FA95D0CDF9F9B0D7863A28E36FA13469`。

## 7. 改动文件回归

4 个实际改动路径命中 `DivineSenseHUDPresentation` 与 `MainHUDTrajectoryPresentation` 两条映射规则。18 份精确日志合计 4,849,010 bytes、212 Success / 0 Fail，覆盖：

- `demo_map.InputRestore` 101；`demo_map.V2RangedCompatibility` 22；
- 神识运行时 4、HUD 12、物理输入 5；
- 受控飞剑威胁提示/读数 1+5；
- 投掷弧线编辑、预览、输入选择、主 HUD 布局、轨迹与切换共 62。

覆盖结果为 `PASS Changed=4 Rules=2 Required=18 Logs=18`；覆盖日志 5,872 bytes，SHA-256 `5A20FDB499E219B3666D3416FBEF1F5D77B3FD732332711098BC12C2709F3B25`。门禁自测为 442/442，日志 43,595 bytes，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建与静态边界

最终构建均使用 UE 5.8 Development、`-WaitMutex`、`-NoHotReloadFromIDE` 与最多 2 个并行动作：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P23.4_GameBuild_final2.log` | PASS / 6 actions / native 0 | 2,310 | `B147582761CD6896A51445E3CF99F4456EE3A8DCA1A434D26B4C404ACE995B0E` |
| `P23.4_EditorBuild_final2.log` | PASS / up-to-date / native 0 | 974 | `D123BF9C052536D7390010EA4945A7206EBE79FFB51145F2A11FF4FD896A45A5` |

最终 `demo_map.exe` 为 359,649,280 bytes，SHA-256 `1E5A5F7A66D90AD33D339965065D4935C732B7FAC54B462D1FA63B323FE46B85`；`UnrealEditor-demo_map.dll` 为 18,906,112 bytes，SHA-256 `C3A893C2B25C58CE54B1D17889C4D6FF3DFA8B647D938D24EC70E846BC602B1F`。

非文档增量为 4 个文件 `+318/-1`，其中生产代码 `+134/-1`、测试 `+184/-0`。新增生产行中的 Timer/SetTimer、自定义 Tick、Sleep、Random/Rand/RNG、ApplyDamage、SpawnActor 和 DestroyActor 均为 0；无新增资产、模块、UCLASS/USTRUCT、Actor、Subsystem 或持久格式；回归映射 JSON 可解析，`git diff --check` 为 0。

## 9. P/F 边界

PASS：最近目标保持精确；后续目标按文字足迹确定性避让；8 个同点 Reveal 可重放；屏边语义不变；无槽位时失败关闭；最终神识 60/0；映射回归 212/0；覆盖门、自测及双构建通过。

未声明：实际字体度量、本地化后长文本、任意 UI 缩放、超宽/竖屏、颜色无障碍或真实战斗镜头已经人工验收。当前 136×32 足迹按现有英文小字体文案保守冻结；若文案或字体变化，应以真实度量替代常量，而不是建立第二套布局状态。本轮没有改变神识半径、3 秒揭示期、10 点成本、脉冲容量、遮挡政策或扫描权威。

## 10. 提交边界与 GitHub

基线提交为 `2d2ad056fa04428c7aefee10797b6aa6f8632a05`。本阶段只提交 3 个生产文件、1 个测试文件、本 Report 与本 Development Log，共 6 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P23.4` 原始证据不进入 Git。

P23 的玩家神识闭环到此结束，不继续增加无消费者包装层。下一阶段建议 P24.0 审计并接入 P18.10 已完成但尚未进入玩家主循环的剑气入口：优先复用现有剑动作授权、可重映射输入和主 HUD 反馈，不复制伤害、Run、资源或投射物权威。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-4-divine-sense-marker-deconfliction>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-4-divine-sense-marker-deconfliction/Docs/Report/Dev.D.UE.0.0.10.P23.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-4-divine-sense-marker-deconfliction/Docs/Log/Dev.D.UE.0.0.10.P23.4.r0_log.md>
