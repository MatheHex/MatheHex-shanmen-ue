# Dev.D.UE.0.0.10.P20.28.r0 Log

## 阶段

- 任务：P20.28 thrown-weapon Ballistic Arc remapped input HUD hint；
- 基线：`36d8f4194c31aa9553b7d87b2846b68b77fda122`；
- 分支：`agent/0.0.10-p20-28-thrown-weapon-arc-input-hud-hint`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实设备输入/截图/Smoke/Cook/Package。

## 实现记录

1. 新增不可变 `Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation`，输入仅为一份既有 Arc presentation 与四个当前键显示名。
2. 键名统一 trim；任一为空或忽略大小写后重复时失败关闭并清空复用输出。
3. 提示文本显示 target、apex increase/decrease、clear 四个动作；apex 上下界附加 `(LIMIT)`，无 target 的 clear 附加 `(NO TARGET)`。
4. HUD 复用唯一 `ReadThrownWeaponInputChoiceInteraction()` 结果，不重复读取或缓存 choice state。
5. HUD 从统一 `Fdemo_mapInputBindingSettings` 分别读取四个 P20.27 action 的当前键名，不硬编码默认键。
6. 新提示位于既有 target/apex/trajectory 三行上方；未新增 Widget、timer、retry、loop 或 UI/input authority。
7. 新增 6 项纯值自动化，完整 0.0.10 总数 `979→985`。
8. changed-file map 新增 hint 规则并扩展 UnifiedInput/MainHUD 要求；正反 self-test `349→351`。
9. 103 个历史无关未跟踪文件保持未暂存、未修改、未删除。

## 有界证据修复

第一次门禁检查拒绝 `interaction_port` 日志：其中 7 项断言均成功，但缺 UE 原生终止标记，因此不能作为健康证据。未修改源码，仅串行重跑该组一次；最终日志为 `7/0`、唯一终止标记、零 fatal/unhandled/ensure、native exit `0`，随后门禁通过。

## 最终自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_input_hint_presentation_final.log` | `6/0` | `83774F2E50C4ECD08AA4BD05A189F3FD5B3ACFB07AB511AE5D548A8EC8D7BF95` |
| `arc_editing_presentation_final.log` | `7/0` | `4045D92DDCC26212525E98CEC2ECED08CA6BDBF10A58D9763E8E935380C6EDF6` |
| `interaction_port_final.log` | `7/0` | `8B8BA9006A24D1C4B77CFFB87CE75E7CEE579BB10776609AFF0ABB3FD92C3BCF` |
| `trajectory_presentation_final.log` | `6/0` | `DF05248FAFABAC76A007BAFD37F78BA897B7B602517CC54437EDFFB40B612EFC` |
| `trajectory_toggle_physical_input_final.log` | `7/0` | `3A96908711C79B6E7DA56238001AC5493A2C27F048C47780803F02680A99D929` |
| `arc_editing_physical_input_final.log` | `10/0` | `F36DF79DC80A130F1A87B504E63A29E8637377AC88C2EACC127DB6DACB06C91B` |
| `legacy_input_restore_final.log` | `101/0` | `E5C51629821BCA41D8E5A160D52722C42C390733B85D0E7757AF46EBE3BA0FB0` |
| `legacy_v2_ranged_final.log` | `22/0` | `CD42CC7E5D4F6F92709DE112E76181FFB7554980F7D62DF61CA5A12FACB67B07` |
| `full_0_0_10_final.log` | `985/0` | `AD2EE0E42A3D704714835C8E624326A0CE1970EBAC85E8763AD8BA983D363654` |

最终证据合计 `1151/0`。9 份日志均有唯一 RunTests 命令、精确成功数、零失败、唯一原生终止标记与零 fatal/unhandled/ensure；日志审计 SHA-256 `048E7BA6F6AB2C4765B0FBDF63FB11B0FA0D24B88CB6F6163BCB61D7CB5A9787`。

## 流程与静态证据

- regression self-test：`351/351`；SHA-256 `760DBBD335284E50A9BBCB1A0DB62DBCD38D098AB0F6A0775578501B820D4B49`；
- static audit：`26/26`；SHA-256 `AA6199D4BC10CB569024BC894AE333FB52FB9E5402830C53FE1F21B6F924FBB8`；
- changed-file gate：`PASS Changed=6 Rules=2 Required=9 Logs=9`；SHA-256 `DF9F18F70939F3B6F5BEF1B5B09DB05ADDC28413D26698842FF59A03EFC1540C`；
- `git diff --check`：PASS；SHA-256 `73CD9640FFF26DC1B19EECB07087B0095046B76052E4C6F3D54A6039858DE852`。

## 构建与产物

- first Editor：6 actions / native 0 / 24.03 秒；SHA-256 `C9CD71364328DF4824D2C748277DAF54121D377223E87EFF4091E9E8BF78333F`；
- final Editor：up to date / native 0 / 0.96 秒；SHA-256 `342CA488D8684E2C4596AFC1B9DD819F6B340D6014AAD15302FE2E109F2EB4BA`；
- final Game：5 actions / native 0 / 26.37 秒；SHA-256 `F2839CA36F736BF27D1A60451DC744A22B1A75098573012C190349D4A2349C74`；
- Editor artifact：16,456,704 bytes，SHA-256 `B7B94D721A135A4C7156FD9845E7E8590B545B576A5D24A12802597902AE1411`；
- Game artifact：357,686,272 bytes，SHA-256 `54E219A09F85C439E7FCE28CC27DC491D4C67A675CCF46637CE9689AB083ABC9`。

## P/F

PASS：纯值 Arc input hint、当前 remap 键名、capability 注记、无效/歧义输入失败关闭、HUD 单读接线，以及完整新旧输入回归。

未验证：实际 HUD 可见布局、真实鼠标/键盘与 OS 焦点、cursor hit、Editor UI/PIE/Standalone、轨迹/落点、真实投掷/碰撞/命中、库存、伤害、Smoke、Cook 或 Package。

## 下一步

P20.29：增加纯值 Ballistic Arc preview sampler，从既有有效 Arc plan 确定性生成固定数量 world-space 轨迹点与落点描述；不查询 World、不 trace、不创建 Actor/Widget/渲染状态。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-28-thrown-weapon-arc-input-hud-hint>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-28-thrown-weapon-arc-input-hud-hint/Docs/Report/Dev.D.UE.0.0.10.P20.28.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-28-thrown-weapon-arc-input-hud-hint/Docs/Log/Dev.D.UE.0.0.10.P20.28.r0_log.md>
