# Dev.D.UE.0.0.10.P23.3.r0 Development Log

## 1. 目标

- 将 P23.0 的成功神识面板从目标计数升级为可用于探路的战术摘要；
- 明确表达区域安全、接触总数、遮挡数、最近距离与剩余灵力；
- 只读取既有 P19/P23 回执和资源投影，不新增玩法权威或包装链；
- 按改动文件映射完成回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线提交：`ff0f8aa63f160327aa9c8295c0bbd5a1a5dd662e`；
- 工作分支：`agent/0.0.10-p23-3-divine-sense-tactical-summary`；
- 基线阶段：P23.2 神识失败反馈；
- 开始时 tracked tree clean，保留用户 103 个 untracked 文件。

## 3. 实现

### 3.1 纯战术摘要

在既有 `demo_mapShanmenDivineSenseHUDPresentation.h/.cpp` 中增加 `Fdemo_mapShanmenDivineSenseHUDTacticalSummary`。输入为已冻结的接触数、遮挡数、最近距离和 SpiritEnergy；输出为两行只读文本以及可测试的原始值。

有接触时第一行按单复数显示 `CONTACT/CONTACTS` 和遮挡数，第二行显示最近距离与资源。无接触时第一行显示 `AREA CLEAR`，第二行仍显示资源。

### 3.2 结构门

`TryProject` 和 `IsValid` 同时约束计数关系、有限距离、空扫描与最近距离一致性、资源上下界。失败总是先清空 `OutSummary`，避免调用方复用陈旧文字。

### 3.3 主 HUD

`demo_mapHUD.cpp` 从当前成功 Receipt 统计 `WasOccluded()`，使用已排序首项取得最近距离，并从 GameMode 既有只读接口取得当前/最大 SpiritEnergy。成功面板改为 520×54 两行；P23.2 失败 banner 仍具有优先级。

## 4. 测试增量

`demo_mapShanmenDivineSenseHUDPresentationTests.cpp` 新增 3 项：

- `TacticalContacts`：3 接触、2 遮挡、14.26m、70/100 灵力，验证确定性重放与最终文案；
- `TacticalAreaClear`：0 接触明确投影为 `AREA CLEAR` 且保留 90/100 灵力；
- `TacticalSummaryFences`：拒绝非法计数、非有限距离、空扫描非零最近距离与非法资源，并证明复用输出被清空。

## 5. 首次失败与有界修复

首次聚焦日志 `P23.3_DivineSenseHUDPresentation_focused.log` 为 7/1，SHA-256 `8C9A998D19CB8A17763A458D9BE3B34D0D0894544661294C52D6123B78619887`。失败断言位于测试夹具的 `14.25 → 14.3` 中点舍入预期，不是编译、World 或产品链失败。

修复只把夹具改为非中点 14.26；随后补充空扫描必须携带零最近距离的结构门，并在最终源码上重新运行全部证据。没有放宽断言或覆盖器。

## 6. 最终自动化

### 6.1 聚焦与完整神识

| Group | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.DivineSenseHUDPresentation` | 8 | 0 | 262,908 | `F0B182B66725985D77D8F8CBEA704E7523C20DD5DF36A70E8F6D04159540A36B` |
| `Shanmen.0_0_10.Product.DivineSense` post-build | 56 | 0 | 332,359 | `7A497E5B0DB7FE00BA97961467544B366DE0FE3EA72FCF1C561099EDFE24DB51` |

### 6.2 改动文件映射

| Required group | Success |
|---|---:|
| `demo_map.InputRestore` | 101 |
| `demo_map.V2RangedCompatibility` | 22 |
| `Shanmen.0_0_10.CombatRuntime.DivineSense` | 4 |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatCue` | 1 |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation` | 5 |
| `Shanmen.0_0_10.Product.DivineSenseHUDPresentation` | 8 |
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

合计 208/0；18 份日志共 4,845,474 bytes。覆盖器结果：`PASS Changed=4 Rules=2 Required=18 Logs=18`，SHA-256 `A72713BCCD75F191FD3614DA3A95BF9804E3192E77E765D1D7F959CAFD404ECF`。覆盖器自测 442/442，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 7. 构建

| Target | Result | Native exit | Actions | SHA-256 |
|---|---|---:|---:|---|
| `demo_map Win64 Development` | Succeeded | 0 | 4 | `DE47A44BA7D263CDAE6B07AF41A31181E15E8DFD030F3DC52641F1A872D72DBC` |
| `demo_mapEditor Win64 Development` | Succeeded / up-to-date | 0 | 0 | `F76DBC2D7671D5F5F8295E6A1C1AC1F7996733B11F5CD0E9E439C6F8E98D5757` |

最终二进制：

- `demo_map.exe`：359,635,456 bytes，SHA-256 `80B4FC1E186056C54F86F0960E222CF9085319BFAD81EFADEB622A5ECDD5892F`；
- `UnrealEditor-demo_map.dll`：18,890,240 bytes，SHA-256 `346A6CE2410A86F25862B5C4C14F3E85C1D432807D053A62B1BEF46FA7F3A51C`。

## 8. 静态边界

- 生产文件：3；测试文件：1；净增量 `+248/-8`；
- 新增行中的 Timer/SetTimer、自定义 Tick、Sleep、Random/Rand/RNG、ApplyDamage、SpawnActor、DestroyActor：0；
- 新增模块、UCLASS/USTRUCT、Actor、Subsystem、资产、存档字段、扫描权威、资源权威、输入入口：0；
- `ShanmenRegressionMap.json` 可解析；
- `git diff --check`：PASS。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 9. P/F 说明

P 阶段证明：冻结数据可确定性生成战术摘要；空扫描、接触、遮挡、最近距离和资源边界有自动化保护；主 HUD 共用链未回归；最终 DLL 和 Game 二进制均可构建。

F 阶段保留：人工检查 520×54 面板在实际分辨率与 UI 缩放下的可读性；本地化与颜色无障碍；多个目标标记重叠时的视觉避让；真实输入、PIE、Standalone、截图与 Smoke。

## 10. 后续建议

P23.4 优先处理神识多目标标记重叠：在不改变 Receipt 顺序和目标权威的前提下，为屏幕邻近标记建立确定性最小避让或聚合提示。若审计发现现有场景不会产生可读性问题，则结束 P23，转向下一项尚未接入玩家主循环的 0.0.10 战斗能力。

GitHub：

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-3-divine-sense-tactical-summary>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-3-divine-sense-tactical-summary/Docs/Report/Dev.D.UE.0.0.10.P23.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-3-divine-sense-tactical-summary/Docs/Log/Dev.D.UE.0.0.10.P23.3.r0_log.md>
