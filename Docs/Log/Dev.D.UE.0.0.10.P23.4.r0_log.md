# Dev.D.UE.0.0.10.P23.4.r0 Development Log

## 1. 目标

- 关闭 P23.3 留下的神识多目标标记重叠缺口；
- 保持最近目标精确、后续目标确定性避让，并保留屏边箭头语义；
- 不改变回执顺序、扫描、遮挡、距离、资源或过期权威；
- 按改动文件映射完成回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线提交：`2d2ad056fa04428c7aefee10797b6aa6f8632a05`；
- 工作分支：`agent/0.0.10-p23-4-divine-sense-marker-deconfliction`；
- 基线阶段：P23.3 神识战术摘要；
- 开始时 tracked tree clean，保留用户 103 个 untracked 文件；
- canonical 单次 Reveal 上限：8。

## 3. 实现

### 3.1 纯值避让

在既有 `Fdemo_mapShanmenDivineSenseHUDMarkerPlan` 增加 `TryDeconflict`。它先复制有效基础计划，再清空输出，因此支持输入和输出引用同一对象。无冲突时原样返回；有冲突时只修改 `ScreenPosition`。

占位冲突按现有 marker 加距离文字的保守足迹判断：水平小于 136px 且垂直小于 32px 才视为冲突。普通投影按上、右、下、左、四个对角的固定优先级搜索，最多 8 圈；水平与垂直步长分别等于足迹尺寸。

### 3.2 屏边语义

`ScreenEdge` 计划不进入二维网格。顶部/底部沿水平方向搜索，左侧/右侧沿垂直方向搜索；角点稳定选择水平边。计划的 `Placement`、`CanvasSize` 和 `EdgeDirection` 全部保留，因此箭头继续指向原始方位。

### 3.3 主 HUD 接线

`DrawDivineSenseReveals` 在当前绘制调用中建立局部 `OccupiedMarkerPositions`，预留当前 Reveal 数量，按权威回执顺序先 `TryPlan` 再 `TryDeconflict`。只有有效最终计划才加入占位并绘制。没有新增成员字段、Timer、Actor、Widget 或跨帧状态。

## 4. 测试增量

HUD Presentation 从 8 项增加到 12 项。新增：

- `ProjectedDeconfliction`：第一项原位，重叠后项固定避让且重放一致；
- `DeconflictionCapacity`：8 个同点输入全部满足足迹间隔，最近项仍精确；
- `EdgeDeconfliction`：屏边项只沿边移动，箭头方向完全保留；
- `DeconflictionFences`：最小视口饱和、非有限占用、输出清空及输入/输出别名。

## 5. 开发中复核

第一版仅以 32px 圆心距离分开菱形，编译和当时的 12 项测试均通过，没有自动化失败。提交前静态复核发现同一水平线上的 marker 虽已分开，距离文字仍可能互相覆盖，因此没有采用该证据放行。

实现随即收紧为 136×32 的文字足迹，并把屏边步长按边缘方向区分；随后重新编译、重跑聚焦组、全部 18 份映射日志、覆盖门、完整神识树和双构建。最终证据只引用收紧后的源码与日志。

## 6. 最终自动化

| Group | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.DivineSenseHUDPresentation` | 12 | 0 | 267,111 | `9F70394992D0AA397D65E76A0F5FEC7365C9746409E296234BDB619A39C37E21` |
| `Shanmen.0_0_10.Product.DivineSense` | 60 | 0 | 336,398 | `149B75D15F1DF8496FC4C7AEF011BC68FA95D0CDF9F9B0D7863A28E36FA13469` |

两份日志均含唯一 RunTests 命令、逐项 Success、自然终止标记、0 Fail 和原生退出码 0。

## 7. 改动文件映射

| Required group | Success |
|---|---:|
| `demo_map.InputRestore` | 101 |
| `demo_map.V2RangedCompatibility` | 22 |
| `Shanmen.0_0_10.CombatRuntime.DivineSense` | 4 |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatCue` | 1 |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation` | 5 |
| `Shanmen.0_0_10.Product.DivineSenseHUDPresentation` | 12 |
| `Shanmen.0_0_10.Product.DivineSensePhysicalInput` | 5 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation` | 6 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput` | 10 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation` | 7 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchGestureFeedbackPresentation` | 3 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRendererAdapter` | 5 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding` | 5 |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort` | 7 |
| `Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintLayoutPolicy` | 3 |
| `Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintStackPresentation` | 3 |
| `Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation` | 6 |
| `Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput` | 7 |

合计 212/0；18 份最终日志共 4,849,010 bytes。覆盖器结果为 `PASS Changed=4 Rules=2 Required=18 Logs=18`，SHA-256 `5A20FDB499E219B3666D3416FBEF1F5D77B3FD732332711098BC12C2709F3B25`。覆盖器自测 442/442，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建

| Target | Result | Native exit | Actions | SHA-256 |
|---|---|---:|---:|---|
| `demo_map Win64 Development` | Succeeded | 0 | 6 | `B147582761CD6896A51445E3CF99F4456EE3A8DCA1A434D26B4C404ACE995B0E` |
| `demo_mapEditor Win64 Development` | Succeeded / up-to-date | 0 | 0 | `D123BF9C052536D7390010EA4945A7206EBE79FFB51145F2A11FF4FD896A45A5` |

最终二进制：

- `demo_map.exe`：359,649,280 bytes，SHA-256 `1E5A5F7A66D90AD33D339965065D4935C732B7FAC54B462D1FA63B323FE46B85`；
- `UnrealEditor-demo_map.dll`：18,906,112 bytes，SHA-256 `C3A893C2B25C58CE54B1D17889C4D6FF3DFA8B647D938D24EC70E846BC602B1F`。

## 9. 静态与 P/F 边界

- 非文档源码：4 files、`+318/-1`；生产 `+134/-1`，测试 `+184/-0`；
- 新增生产行中的 Timer/SetTimer、自定义 Tick/Sleep/Random/Rand/RNG/ApplyDamage/SpawnActor/DestroyActor：0；
- 新增资产、模块、UCLASS/USTRUCT、Actor、Subsystem、存档字段、扫描/资源/输入权威：0；
- `ShanmenRegressionMap.json` 可解析；`git diff --check`：PASS。

P 阶段证明纯布局、容量、顺序、边缘、别名和失败关闭契约，以及最终 DLL、Game 二进制与共享 HUD 回归。F 阶段保留真实字体度量、本地化、UI 缩放、特殊宽高比、镜头密集场景、颜色无障碍和人工手感验收。

## 10. 后续建议与 GitHub

P23 神识玩家闭环到此结束。P24.0 建议审计 P18.10 的剑气 availability command router，并把它接到既有可重映射玩家输入与主 HUD 反馈；只复用正式剑动作授权和现有 Sword Qi 产品链，不新增包装层或复制 damage、Run、资源、投射物权威。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-4-divine-sense-marker-deconfliction>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-4-divine-sense-marker-deconfliction/Docs/Report/Dev.D.UE.0.0.10.P23.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-4-divine-sense-marker-deconfliction/Docs/Log/Dev.D.UE.0.0.10.P23.4.r0_log.md>
