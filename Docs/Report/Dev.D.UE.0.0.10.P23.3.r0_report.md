# Dev.D.UE.0.0.10.P23.3.r0 Report

## 1. 结论

P23.3 在 P 阶段边界内完成，结论为 **PASS**。

本轮把神识成功脉冲的顶部 HUD 从机械的目标计数升级为可直接用于探路的战术摘要：无目标时明确显示 `AREA CLEAR`；有目标时显示接触总数、遮挡数、最近目标距离与脉冲后的剩余 SpiritEnergy。既有目标菱形、屏外方向箭头、遮挡颜色和失败反馈保持不变。

```text
Post-build Divine Sense:               56 Success / 0 Fail
Changed-file mapped regression:       208 Success / 0 Fail (18 exact logs)
Changed-file regression coverage:      PASS (4 paths / 2 rules / 18 groups)
Regression gate self-test:              PASS 442/442
Game + Editor Development:              PASS (both native 0)
git diff --check:                        PASS
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明最终编译 DLL 上的摘要投影、神识产品链与主 HUD 共享兼容面，不声明人工视觉或手感验收已经完成。

## 2. 玩家侧变化

一次成功神识脉冲现在给出两行信息：

- 无接触：`DIVINE SENSE · AREA CLEAR`；
- 有接触：`DIVINE SENSE · 3 CONTACTS · 2 OCCLUDED`；
- 有接触的第二行：`NEAREST 14.3m · SPIRIT 70 / 100`；
- 无接触的第二行仍保留权威资源读数，例如 `SPIRIT 90 / 100`。

这使玩家无需逐个读取世界标记便可先判断区域是否安全、是否存在隔墙目标、最近威胁有多远，以及还可负担多少次神识脉冲。

## 3. 单一权威与数据来源

新增的 `Fdemo_mapShanmenDivineSenseHUDTacticalSummary` 是既有 HUD 表现文件中的纯值投影。它只接收从当前有效 `FShanmenDivineSenseScanReceipt` 推导出的接触数、遮挡数、最近距离，以及既有 Product Controller 的 SpiritEnergy snapshot。

摘要不扫描 Actor、不重新做遮挡判定、不扣资源，也不改变 Receipt、输入、World 或过期时间。最近目标直接使用 P19 已保证按距离排序的首个 Reveal；遮挡数只累计每个 Reveal 已冻结的 `WasOccluded()`。因此 HUD 没有建立第二份扫描或资源权威。

## 4. 失败关闭边界

纯投影拒绝以下不可能组合，并在失败时清空复用输出：

- 负接触数或负遮挡数；
- 遮挡数大于接触总数；
- 非有限或负距离；
- `AREA CLEAR` 同时携带非零最近距离；
- 非有限、负值、超过最大值或无正最大值的 SpiritEnergy snapshot。

相同冻结输入产生完全相同的文本和值。单个接触使用 `CONTACT`，多个接触使用 `CONTACTS`，避免把语法变化交给 HUD 临时判断。

## 5. HUD 接线

既有 `DrawDivineSenseReveals` 仍先绘制世界/边缘目标标记，再处理 P23.2 的失败提示。只有成功 Reveal 有效且没有更高优先级失败提示时，HUD 才从同一 Receipt 生成两行战术摘要。

成功面板从 380×36 扩展为 520×54，并保持在既有顶部安全区内；第一行显示扫描结论，第二行显示最近距离与灵力。没有新增 Widget、资产、Timer、SetTimer、自定义 Tick、Actor、Subsystem 或持久字段。

## 6. 首次失败与修复

首次聚焦组进入实际测试并得到 7 Success / 1 Fail。唯一失败位于新测试夹具：输入 `14.25` 后固定期待一位小数为 `14.3`，落在二进制浮点中点舍入歧义上。保留失败日志后，将夹具改为非中点 `14.26`；产品格式仍保持一位小数，最终显示 `14.3m`。

随后静态复核发现空扫描仍容忍非零最近距离。提交前补上严格关联门和回归断言，重新编译并重跑全部最终证据。最终聚焦组为 8/0，完整神识树为 56/0。

首次失败日志：263,456 bytes，SHA-256 `8C9A998D19CB8A17763A458D9BE3B34D0D0894544661294C52D6123B78619887`。

## 7. 自动化与改动映射

最终 `Shanmen.0_0_10.Product.DivineSenseHUDPresentation` 为 8/0；新增 3 项分别覆盖多接触摘要、区域安全摘要和无效输入门。最终完整 `Shanmen.0_0_10.Product.DivineSense` 为 56/0，日志 332,359 bytes，SHA-256 `7A497E5B0DB7FE00BA97961467544B366DE0FE3EA72FCF1C561099EDFE24DB51`。

改动文件映射命中 `DivineSenseHUDPresentation` 与 `MainHUDTrajectoryPresentation` 两条规则。18 份精确日志合计 4,845,474 bytes、208 Success / 0 Fail，覆盖神识 HUD/物理输入/运行时、主 HUD 受控武器提示、投掷弧线编辑/预览/输入/提示布局/轨迹，以及 `demo_map.InputRestore` 与 `demo_map.V2RangedCompatibility`。

覆盖结果为 `PASS Changed=4 Rules=2 Required=18 Logs=18`，日志 SHA-256 `A72713BCCD75F191FD3614DA3A95BF9804E3192E77E765D1D7F959CAFD404ECF`。门禁自测为 442/442，日志 SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建与静态边界

最终构建均使用 UE 5.8 Development、`-WaitMutex`、`-NoHotReloadFromIDE` 与最多 2 个并行动作：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P23.3_GameBuild_final2.log` | PASS / 4 actions / native 0 | 2,203 | `DE47A44BA7D263CDAE6B07AF41A31181E15E8DFD030F3DC52641F1A872D72DBC` |
| `P23.3_EditorBuild_final2.log` | PASS / up-to-date / native 0 | 974 | `F76DBC2D7671D5F5F8295E6A1C1AC1F7996733B11F5CD0E9E439C6F8E98D5757` |

最终 `demo_map.exe` 为 359,635,456 bytes，SHA-256 `80B4FC1E186056C54F86F0960E222CF9085319BFAD81EFADEB622A5ECDD5892F`；`UnrealEditor-demo_map.dll` 为 18,890,240 bytes，SHA-256 `346A6CE2410A86F25862B5C4C14F3E85C1D432807D053A62B1BEF46FA7F3A51C`。

非文档增量为 4 个文件 `+248/-8`。新增生产代码中的 Timer/SetTimer、自定义 Tick、Sleep、Random/Rand/RNG、ApplyDamage、SpawnActor 和 DestroyActor 均为 0；回归映射 JSON 可解析，`git diff --check` 为 0。

## 9. P/F 边界

PASS：空扫描明确表达区域安全；接触摘要显示总数、遮挡数和最近距离；资源读数来自既有权威；不可能组合失败关闭；失败提示继续覆盖成功摘要；最终神识树 56/0；映射回归 208/0；覆盖门、自测和双构建通过。

未声明：英文文案已本地化、面板已完成所有分辨率/UI 缩放/颜色无障碍验收、重叠目标标记已自动避让、敌人类型或威胁等级已进入神识回执、多人客户端 HUD 或人工游玩已经验收。本轮没有改变神识半径、3 秒揭示期、10 点成本、10 次容量、遮挡政策或扫描权威。

## 10. 提交边界与 GitHub

本阶段只提交 3 个生产文件、1 个测试文件、本 Report 与本 Development Log，共 6 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P23.3` 的首次失败与最终成功原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-3-divine-sense-tactical-summary>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-3-divine-sense-tactical-summary/Docs/Report/Dev.D.UE.0.0.10.P23.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-3-divine-sense-tactical-summary/Docs/Log/Dev.D.UE.0.0.10.P23.3.r0_log.md>
