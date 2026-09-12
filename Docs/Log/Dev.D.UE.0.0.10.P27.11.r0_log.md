# Dev.D.UE.0.0.10.P27.11.r0 Development Log

## 1. 目标

- 将 P27.8 阵图访问结果惰性接入 P27.4 设备无关开始输入；
- 保持输入前置门早于个人知识读取，访问/空间失败早于 Intent 与生命周期；
- 保持既有 InputAdapter 为唯一 Intent 派生与调用次序权威；
- 通过 P27.10 唯一 RunLifecycle 入口携带共享灵力控制器；
- 证明首次扣费与同事件重放幂等；
- 完成改动驱动回归、两目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`9498f9ed43a4bcc936a1842bec1fb4aa3f2e22b2`（P27.10）；
- 分支：`agent/0.0.10-p27-11-formation-selection-input-route`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不修改 Profile/schema、物品真值、阵图内容、按键、UI、PlayerController、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、Cook 或 Package。

## 3. 新增组合入口

新增 `Fdemo_mapShanmenFormationDiagramStartInputComposition`。它接收 gameplay/lifecycle gate、Run/Owner/InputEvent、活动目录、请求阵图、origin/forward、既有知识读取回调和既有生命周期回调，并把 `FormationDiagramAccessAdapter::Resolve()` 放入 `FormationInputAdapter::RouteStartInput()` 的惰性采样回调。

组合器不复写输入适配器的 preflight 或 Intent 逻辑，不保存回调，不拥有产品状态。访问、空间采样和生命周期调用次数均限制为 0 或 1。

## 4. 结果状态与协议校验

`Fdemo_mapShanmenFormationDiagramStartInputCompositionResult` 保留顶层状态、诊断、访问次数、空间样本次数，以及完整 Access/Input 嵌套证明。

- `InputCompletedBeforeAccess`：合法输入前置门完成，访问次数必须为 0；
- `DiagramAccessRejected`：访问一次并保留精确目录/知识/选择失败，空间与生命周期均为 0；
- `SpatialSampleRejected`：选择有效但空间捕获失败，不产生 Intent 或生命周期调用；
- `Delegated`：恰好一次访问、空间捕获和生命周期调用；
- `RouteProtocolRejected`：调用次数或访问/输入证据不一致时失败关闭。

成功委托逐项核对 SelectionId、CatalogId、KnowledgeSnapshotId、OwnerId、authority revision 与 DiagramId，防止调用方跨 revision 或跨 Owner 拼接。

## 5. 真实生命周期与共享灵力

集成 fixture 建立真实 ItemAuthority、CombatRun、FormationRunLifecycle 和共享 `SpiritEnergyController`。生命周期回调调用唯一签名：

`TrySubmit(Authority, Coordinator, SpiritEnergyController, Intent)`。

首次事件以阵图定义中的 10 点成本把余额从 100 降至 90，产生一个共享外部事务并只推进一次阵法激活序号。完全相同事件重放得到相同 SelectionId、IntentId 和资源 ReceiptId；余额仍为 90、事务数仍为 1、序号不再推进。结束仍通过唯一 Formation RunLifecycle 完成。

## 6. 测试结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationDiagramStartInputComposition` | 5 | 0 | `125FC42AF4A3B7C45017AB525ACF6AF42CF191CB4AB21BB4B6E9B15DC9784B86` |
| `Shanmen.0_0_10.Product.FormationDiagramAccessAdapter` | 4 | 0 | `B511F0F9351DB0A793F089F555D586063CF14FAA4AE497D64F4C330BD0CE97A0` |
| `Shanmen.0_0_10.Product.FormationKnowledgeAuthorityAdapter` | 4 | 0 | `DE4A3C8F59AD13E6A625F5AFB0AA8B5E2E03C3E5CE7B11C157FB8407F912F11A` |
| `Shanmen.0_0_10.Product.FormationDiagramSelection` | 3 | 0 | `53F52F411764C84237F6587C7A2F8BF35E5AAE52CE8EDF4B673AA0095349D8C6` |
| `Shanmen.0_0_10.Product.FormationInputAdapter` | 3 | 0 | `29F449BEE04D0BBE475E8CACBCD2F396391C22ED1A22C034A1FAAA4E560A5FEB` |
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 6 | 0 | `D6B2734C31B692D2A92469266448BE4809FFFBD8BEF3FABEA4F6B2DCA56C11A6` |
| `Shanmen.0_0_10.Product.FormationProductController` | 3 | 0 | `E8A6F9F22A8867F084BBEAFA5AE48B5E9788298FFF4ABD559765B2CEDAF945B2` |
| `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `FCF2AB6BE4004BA5B8D04D3EDDD46EB3F8D65A5D566C7D93E334E96A1105310A` |
| `Shanmen.0_0_10.Product.DivineSenseProductController` | 5 | 0 | `A7E3E87AFD1A6C0CA64ED3423F1F290D2C7F55F06D43C765176AA6EA4133FAB6` |
| `Shanmen.0_0_10.Product.SpiritShieldProductSession` | 9 | 0 | `EF7F3BFE75ABEBF901EB7471557873EB583B8E48E5C00BDFFE6EA310B43A5CC1` |
| `demo_map.V3.Attributes` | 4 | 0 | `15380860EED31E865E8287437C58C83D83338D39DA4619D7F61E6B7BF26A643A` |
| `Shanmen.0_0_10` | 1,350 | 0 | `052BC7CA79A79565C683F43511380EB0B8F73E2156751C76DE401DCAE60FC79D` |

所有最终日志均具备原生退出 0、UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组耗时 65 分 17.320 秒。

## 7. 回归与静态检查

- regression map JSON：PASS；
- 映射器正反自测：`474/474` PASS；
- 新增规则要求根组及访问、知识、选择、输入、生命周期、产品、共享资源和既有消费者证据；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=49 Logs=3`；
- 新增生产桥接 338 行，未发现 World/Actor/PlayerController/Enhanced Input、随机身份/RNG、Profile/schema、Inventory 或伤害依赖；
- `git diff --check`：PASS。

## 8. 构建证据

- Editor 6 actions，`SUCCEEDED` / native 0，12.053 秒；run-state SHA-256 `30EAF7E916E0E11EB3A2BA01497FABD03E960A325F140F3E17DDD424EE223DAF`；stdout SHA-256 `C1B6E5F8E4C57FE90D03B625F7497F5ABE696BFC4DCF9E9B1FC0A4D65C067627`；
- `UnrealEditor-demo_map.dll` 19,604,992 bytes，SHA-256 `4D744680D21163D9A8DD89D2628277974BF5B5EACABC097C2B2EFE3D4C2A2F6F`；
- Game 5 actions，`SUCCEEDED` / native 0，20.313 秒；run-state SHA-256 `CAF59C6F61C7F6279E1E010AA54CFE24DADCB7AB98292DE5EA696207D18DF633`；stdout SHA-256 `3774FAAB38D7665504A1374E013D3C01295A9D60C68395558D7F7B6BDBF5DDE2`；
- `demo_map.exe` 360,234,496 bytes，SHA-256 `BA8424A848458EDBEC13D3E153F63A8F82080B99BC410ECF865F36ACC512FF4C`；
- 两次 stderr 为空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

## 9. P/F 边界与下一步

P 阶段覆盖了访问到输入再到真实共享资源生命周期的组合证明。F 阶段未执行 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。下一阶段须先冻结正式输入 action/key 与 UI/目录供给责任，再把现有输入所有者接到该组合器；不得把测试 fixture 身份或内容升格为 shipping 配置。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationDiagramStartInputComposition.h`
2. `Source/demo_map/demo_mapShanmenFormationDiagramStartInputComposition.cpp`
3. `Source/demo_map/demo_mapShanmenFormationDiagramStartInputCompositionTests.cpp`
4. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
5. `Scripts/ShanmenRegressionMap.json`
6. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
7. `Docs/Report/Dev.D.UE.0.0.10.P27.11.r0_report.md`
8. `Docs/Log/Dev.D.UE.0.0.10.P27.11.r0_log.md`

`Saved/Codex/P27.11` 不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-11-formation-selection-input-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-11-formation-selection-input-route/Docs/Report/Dev.D.UE.0.0.10.P27.11.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-11-formation-selection-input-route/Docs/Log/Dev.D.UE.0.0.10.P27.11.r0_log.md>
