# Dev.D.UE.0.0.10.P22.2.r0 Development Log

## 1. 基线与目标

- base：`ef26ad4a2bf96b1178ad9a8892579ffe8250d11f`（P22.1 thrown-weapon collision profile）；
- branch：`agent/0.0.10-p22-2-thrown-weapon-terminal-feedback`；
- 目标：把既有投掷物 Run Host 的权威终态回执变成主 HUD 可读反馈；
- 边界：不新增伤害、生命值、动作、库存、目标、输入、飞行、计时或死亡权威，不启动 UI/PIE/产品 executable。

## 2. 审计与决策

P22.1 已让可见飞刀、碰撞轮廓和真实接触闭合，但玩家只能看到飞行前的轨迹/输入提示，无法从主 HUD 判断飞刀最终是命中、被阻挡、超程、落空或中断。

既有 `Fdemo_mapShanmenThrownWeaponRunHost` 已在 Carrier 消失后保留不可变 Terminal Receipt，内容包含 LaunchId、终态 Kind、World Delivery、生命值 Commit Result 及动作恢复/完成/中断回执。因此最小方案是读取该回执并投影；没有理由再监听 Projectile、重算 Damage 或创建消息计时器。

## 3. 实现

新增 `Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation`：

- 接收一个有效 Terminal Receipt，先清空复用输出；
- 映射 Impact、Blocking Miss、Range Expired、Flight Time Expired 与 Interrupted；
- Impact 只接受 `Delivered + Committed + valid vitality receipt`；
- Applied Damage 读取提交回执，Defeated 由 `AppliedDamage > 0 && VitalityAfter <= 0` 得到；
- 构造七种规范中文文本并在 `IsValid()` 中重新验证；
- `Matches()` 提供确定性重放比较；
- 非 Impact 必须保持 Damage=0 且 Defeated=false。

Product Lifecycle 新增只读 `GetTerminalReceipt()` 转发。主 HUD 每帧只读取现有生命周期并尝试投影，不推进任何状态。

## 4. 现有提示栈接入

提示栈新增 `TerminalFeedback` Kind 与 Impact、Defeat、NoDamage、Blocked、Expired、Interrupted Tone。终态行规范 rank 为 5，固定在已有五种提示之后。

新增五参数 `TryCompose`，旧重载转发默认无效终态。直线模式允许 `Trajectory + optional Terminal`；弧线模式保持既有组合规则并允许末尾终态。最大行数由 5 改为 6，布局测试同步覆盖标准 6 行、最小 `640 × 216` 紧凑 6 行和 7 行拒绝边界。

Canvas 继续是唯一渲染器，只为六种 Tone 增加颜色和统一 `0.82` scale。没有新增 Widget、Overlay、Panel、Tick 或 Timer。

## 5. 测试扩展

`ThrownWeaponRunHost` 的 4 个既有测试现在同时证明：

- Spawn Gate：Interrupted 回执投影为“飞刀 · 已中断”；
- Contact：普通 committed impact 为 `0.7`、重复投影一致、终态成为直线栈第二行；
- Contact：真实 GamePreview World 的致死命中显示“击破”；
- Contact：Base Damage 和 Technique Power 均为 0 时显示“未造成伤害”；
- Miss：blocking geometry 显示“命中阻挡”；
- Miss：range expiry 显示“超出射程”。

`ThrownWeaponRunCommand` 的 Arc 生命周期测试增加 Flight Time Expired “落空”断言。布局和提示栈原测试保留，并扩展至六行容量。

## 6. 首次失败与修正

| Evidence | Observable result | SHA-256 |
|---|---|---|
| `P22.2_focused_run_host_initial.log` | 1 success 后 access violation；无自然终止 | `BEC4360452ACCA7CFABBE0A235F0BA6B47E8F58085BC62D99BE665CA94329838` |
| `P22.2_focused_run_host_after_lethal_world_fix.log` | 3/1；致死 Carrier 无匹配 World | `D9998AC324F8C368E71B89E31C969709459F9D7F150AA6BC2ABCAD1488C1C1D9` |
| `P22.2_editor_build_world_carrier_fix.log` | C2039；World 参数接到错误夹具重载 | `1C70061D526275AE9E6CC7BDC949E89E8623BB4F772A2F5EDE4EFF621091078C` |
| `P22.2_mapped_shanmen_0_0_10.log` | 727/0 后人工停止；缺终止标记，不计 PASS | `AA405B50CC4AB501F5EE6A6F3B1EE4CA177B068DB7932515A256CD8B25BB8D29` |

崩溃根因是 transient 敌人的致死路径需要 World Timer。最终夹具创建 GamePreview World、真实敌人和来源 Pawn，并通过生产 `SpawnStagedCarrier` 在同一 World 生成投掷物，保留真实死亡路径。错误重载接线修正后重新构建。

第一份精确门禁自测日志只到 142 条且没有最终汇总，不作为通过。完整重跑暴露 Main HUD fixture 在移除冗余大组后缺少 `ThrownWeaponInputChoiceInteractionPort`；补上精确日志后最终 `439/439`。

## 7. 改动文件映射回归

12 个改动路径由 7 条规则推导出 27 个精确组：

| Group | Success | SHA-256 |
|---|---:|---|
| `demo_map.InputRestore` | 101 | `0006E1B36E7065CE4ACB5ECDC8D73BE20BA788B8C29CEA12A6B584AFE5AF316D` |
| `demo_map.ItemUseAndArmor` | 46 | `7483E6F2739754E0D3952FE4333FDBD8A3B1DF7EE799307665C9E34A1A2F75E1` |
| `demo_map.V2RangedCompatibility` | 22 | `11F90F7E873174B3D8ADD67AA8BC48E783E575BA8A6295AEF5E9448F8476A470` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | `89FE63DA4343C95167360F46E43A1FD1C492345A1C79C012C62F59F39FFE0765` |
| `Shanmen.0_0_10.Items` | 77 | `D0DF285AE9424D7FF871EE543FBA1DEB20B4B18117FF23BC1B79669DA7C98E57` |
| `Product.CombatRunCoordinator` | 18 | `2A0E8DF733D0F8ED7E9C8A6CF3A29C641025746F102F133C738152E32A44FF96` |
| `Product.ControlledWeaponThreatCue` | 1 | `496E545C1884E90F8DA44E414B39757DB7276C7150A5F1836AA1824BA8D77125` |
| `Product.ControlledWeaponThreatReadoutPresentation` | 5 | `C7524FF3A2BB23CEE0C3CA48F4D8E315AA83B79501C249EB517780B9632C7164` |
| `Product.ThrownWeaponArcEditingInputHintPresentation` | 6 | `D4F39D217CAB0715E068A3E701C10AF0DA3B21B88C6A92FCFF722F6F1EC12B6B` |
| `Product.ThrownWeaponArcEditingPhysicalInput` | 10 | `CFA92974D2D8A6194F6D14C0BB1DCA187628F2698B61726867DBFD570C1E9E01` |
| `Product.ThrownWeaponArcEditingPresentation` | 7 | `0F137FAF132EB6B7FE8D67EC17029E4ADF1BDD2C03A03CF74E01120C4D05B2B6` |
| `Product.ThrownWeaponArcPreLaunchGestureFeedbackPresentation` | 3 | `F7CC597F533D3D95272BAE7DE70DCB98000FA145ED68862411163642281169E3` |
| `Product.ThrownWeaponArcPreviewMainHUDRendererAdapter` | 5 | `0C533E8242F04AE622F39E5FE40AE84841A359786F5010BC0DC2900D7D57FFDF` |
| `Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding` | 5 | `5259C741649ECB7429B55BD9FC75452E1C16CA18110DBA453F2F030E55028835` |
| `Product.ThrownWeaponInputChoiceInteractionPort` | 7 | `E39E38B4656366D4702073635414B1BF483ECC5E9CFC19F401714C41C61BD1B0` |
| `Product.ThrownWeaponItemAdapter` | 5 | `7C26A5FE4EDEF723E6D53CC8938E92514CDFF620E6F0984452BC047A90CE1AFE` |
| `Product.ThrownWeaponMainHUDCombatHintLayoutPolicy` | 3 | `9C8232FE11E4E359EDF4EA5A19D2A4805720A2158807C4B5338F621DE5D29A06` |
| `Product.ThrownWeaponMainHUDCombatHintStackPresentation` | 3 | `FCCD434847032D672A537B33C9E4B869AD829B3C9AD540517F639B19C362EFC3` |
| `Product.ThrownWeaponProductController` | 7 | `07F74A7E499338962349EC2D0D15DD434ACC1BACB6B4F22848AA67C0F27E6079` |
| `Product.ThrownWeaponProductLifecycle` | 5 | `4EEFB211DD0EB2A4D75C9401945AA089F9A50E0095BF35C220F7F9A9DC162979` |
| `Product.ThrownWeaponProductSession` | 6 | `8C3A6A18DC15CF8E7ABDCDBAFC589F0F3C7DC31C1E8E6803931F2A0084AB5D48` |
| `Product.ThrownWeaponRunCommand` | 6 | `3726FB8BFE2A0ECE6C1D130F8A5167E5D1D322020C72101006272DB2A88A8461` |
| `Product.ThrownWeaponRunHost` | 4 | `24C89135AC656837EE01F02B517E1A37EC966BBCF334DD3DFFA77EE32E9D08F9` |
| `Product.ThrownWeaponTrajectoryPresentation` | 6 | `E1B2B23F256F9EA0D5ADDCBBB90438E2804E757C7FA766E45EC7D27D99159382` |
| `Product.ThrownWeaponTrajectoryTogglePhysicalInput` | 7 | `7E59A4CD93BE811B269A8EA5A31A0B4BE4081B0DEE812125A4D9B38FCCCF7C0C` |
| `Product.ThrownWeaponWorldDelivery` | 6 | `E90DDBDC618C76332501F7B371964B2FAD075EAFF341FA2619BD9F64E1B27690` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | `0BC6BE1AE2F1F11EBD8361614C644A06C3180ADC3EA95CACB5A8008FCFA84F1A` |

合计 `527/0`；每份日志唯一命令、自然终止、零 Fatal/Unhandled/Ensure。

## 8. 覆盖门与流程收紧

新增 Terminal Feedback 映射规则，要求 World Delivery、Run Host、HUD Stack、Input Restore 与 V2 Ranged 证据。对应正/负 fixture 已加入门禁自测。

同时从 Main HUD、Terminal、HUD Stack 和 Layout 四条纯呈现规则移除冗余 `Shanmen.0_0_10` 大组；明确列出的直接依赖组保持不变。完整自测证明这些精确组足以覆盖规则，避免再次启动 1,248 项的高延迟诊断。

最终覆盖门：`PASS Changed=12 Rules=7 Required=27 Logs=27`，日志 SHA-256 `280909942CC0DA373F5607D49EF8AFFC9D46E7D9C6B1DB049A445EBDEF0E2E70`。自测 `439/439`，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

## 9. 构建与静态检查

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.2_game_build_final.log` | 90 actions / PASS / native 0 | 9,585 | `22BF077EDEDC088DE3F06D80BF589457BBED842BB343491CDCE66FA956E3A330` |
| `P22.2_editor_build_final.log` | 0 actions / PASS / native 0 | 974 | `6B450DC28DA63038AD4E6339A189D3DD4DA7A30AA994E4A9CD86C5D4782BDD50` |

`demo_map.exe` 为 359,555,584 bytes，SHA-256 `A2EE2AA0FE9ED9CAC337DE1A707CC21A16BB533F601A893EFA95DCC4121D0892`。`UnrealEditor-demo_map.dll` 为 18,752,512 bytes，SHA-256 `2BE5DD08B4F12084303233449630DD805DCE544E01896D40F40E97A9C8BC84F2`。

实现/测试/门禁映射 diff 为 12 files / 703 insertions / 45 deletions。新增生产行 317；Timer、SetTimer、Tick、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0。`git diff --check` 为 0，最终相关运行进程为 0。

## 10. 提交边界与 GitHub

精确提交 12 个实现/测试/流程文件与本 Report/Log。103 个用户原有 untracked 文件不暂存；所有 raw evidence 留在 `Saved/Codex/P22.2` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-2-thrown-weapon-terminal-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-2-thrown-weapon-terminal-feedback/Docs/Report/Dev.D.UE.0.0.10.P22.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-2-thrown-weapon-terminal-feedback/Docs/Log/Dev.D.UE.0.0.10.P22.2.r0_log.md>
