# Dev.D.UE.0.0.10.P27.11.r0 Report

## 1. 结论

P27.11 已把 P27.8 的阵图知识访问与 P27.4 的设备无关阵法开始输入组合成一个惰性、同步、失败关闭的产品入口。调用方提交活动目录、Owner、请求阵图、空间样本和稳定输入事件后，既有输入适配器先完成 gameplay/lifecycle/Run/Owner/event 前置门；只有这些条件全部成立才读取一次个人阵图知识、选择一次阵图并捕获一次空间样本，最后仍由既有 RunLifecycle 完成唯一产品提交。

本轮没有新增阵图目录、知识、选择、输入、序号、重试或资源权威，也没有恢复无能耗启动入口。真实集成证明沿 P27.10 的唯一生命周期签名传入共享 `SpiritEnergyController`：首次提交从 100 扣至 90，生成一个外部资源事务和一个阵法序号；完全相同事件重放复用选择、Intent 与资源收据，余额、事务数和序号均不再变化。

## 2. 阶段问题与范围

P27.8 已能从公开目录与玩家知识权威安全选出阵图，P27.10 已能以共享灵力原子启动阵法，但两者仍由调用方手工接线。若访问发生在输入前置门之前，会无必要读取个人知识；若选择、空间采样和生命周期分别拼装，则调用方可以混用 Owner、目录 revision、阵图或事件身份。

本轮只封闭这条接缝：

- 复用 `Fdemo_mapShanmenFormationInputAdapter::RouteStartInput()` 作为唯一输入排序与 Intent 派生入口；
- 在它的惰性采样回调中调用一次 `FormationDiagramAccessAdapter`；
- 只有访问成功才捕获 origin/forward，只有空间样本有效才触发生命周期；
- 交叉核对访问结果与输入样本中的 SelectionId、CatalogId、SnapshotId、Owner、authority revision 和 DiagramId；
- 用真实 RunLifecycle 集成证明共享灵力参数没有在组合层丢失；
- 扩展改动驱动回归映射与正反自测；
- 不绑定物理按键、UI、PlayerController、地图、正式阵图内容或存档 schema。

## 3. 惰性组合顺序

`Fdemo_mapShanmenFormationDiagramStartInputComposition::Route()` 不复制输入状态机，而是把阵图访问嵌入既有 `RouteStartInput()` 的单次 `SampleStart` 回调。固定顺序为：

1. gameplay gate；
2. lifecycle availability；
3. Run、Owner、InputEvent 身份；
4. 公开目录前置检查与至多一次知识权威读取；
5. 至多一次阵图选择；
6. 至多一次 origin/forward 捕获；
7. 既有 Intent 派生；
8. 至多一次调用方提供的 RunLifecycle 路由。

前置门拒绝时知识读取、空间采样和生命周期调用均为 0；目录未知、知识权威失败或未习得时空间采样与生命周期为 0；空间向量无效时不生成有效 Intent，也不进入生命周期。组合器无缓存、无循环、无内部重试。

## 4. 可审计结果契约

顶层结果区分 `InputCompletedBeforeAccess`、`DiagramAccessRejected`、`SpatialSampleRejected`、`Delegated` 与 `RouteProtocolRejected`。结果保留访问次数、空间捕获次数、完整访问证据和完整输入证据，而不是把失败压缩为布尔值。

`IsValid()` 对每个状态验证合法调用次数和嵌套结果形状。成功委托还要求访问选择与输入样本逐字段一致。若既有输入路由违反“采样至多一次”、绕过访问，或返回与所选阵图不一致的证据，组合器返回 `RouteProtocolRejected`，不会把结果视为可接受。

## 5. 身份、共享资源与幂等

组合器不派生新的随机身份。SelectionId 继续由目录、知识快照、Owner、revision 和阵图确定；IntentId 继续由 RunId 与 InputEventId 通过既有 InputAdapter 确定。相同输入和相同权威 revision 产生相同选择与 Intent，不同输入事件产生不同 Intent。

生命周期回调仍由产品所有者持有，因此组合器没有第二个 Run 或资源入口。真实集成回调调用 P27.10 唯一的 `FormationRunLifecycle::TrySubmit(Authority, Coordinator, SpiritEnergyController, Intent)`。测试证明首次启动消费阵图定义中的 10 点激活成本；精确重放复用同一个资源 ReceiptId，不重复扣费、不新增外部事务，也不再占用激活序号。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
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

五项新增测试覆盖输入前置门零知识读取、目录/知识失败阻断生命周期、空间样本失败、确定性委托，以及真实共享灵力扣费与幂等重放。最终日志均具备 UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、测试进程原生退出 0、0 Fail 和 0 Fatal/Unhandled/Ensure；本轮证据全部满足。完整根组耗时 65 分 17.320 秒。

## 7. 改动驱动回归

新增 `FormationDiagramStartInputComposition` 映射规则，要求完整根组以及访问、知识、选择、输入、生命周期、产品控制器、Combat Run、阵法产品/材料/部署、物品、WorldGameplay、共享行动资源、Divine Sense、Spirit Shield 与 CombatCore 证据。其负向自测确认只有组合器焦点日志不能替代依赖层证明。

- regression map JSON：PASS；
- 映射器正反自测：`474/474` PASS；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=49 Logs=3`；
- `git diff --check`：PASS。

## 8. 构建与静态边界

- Editor：6 actions，`SUCCEEDED`，原生退出 0，12.053 秒；stdout SHA-256 `C1B6E5F8E4C57FE90D03B625F7497F5ABE696BFC4DCF9E9B1FC0A4D65C067627`；
- `UnrealEditor-demo_map.dll`：19,604,992 bytes，SHA-256 `4D744680D21163D9A8DD89D2628277974BF5B5EACABC097C2B2EFE3D4C2A2F6F`；
- Game：5 actions，`SUCCEEDED`，原生退出 0，20.313 秒；stdout SHA-256 `3774FAAB38D7665504A1374E013D3C01295A9D60C68395558D7F7B6BDBF5DDE2`；
- `demo_map.exe`：360,234,496 bytes，SHA-256 `BA8424A848458EDBEC13D3E153F63A8F82080B99BC410ECF865F36ACC512FF4C`；
- 两次 stderr 均为空，SHA-256 为 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增生产桥接共 338 行。scoped scan 未发现 `UWorld`、`AActor`、PlayerController、Enhanced Input、物理 InputAction、随机 GUID/RNG、Profile/schema、Inventory 或伤害调用；没有新增正式内容、资产或地图改动。

## 9. P/F 边界与下一阶段

P 阶段证明了惰性前置门、单次知识访问/选择/空间采样、失败零生命周期副作用、访问与输入证据一致性、真实共享灵力消费、同事件幂等重放、完整 0.0.10 回归、改动驱动覆盖和两目标构建。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。因此不声明玩家已有正式按键或 UI 可选择/部署阵图。下一阶段应先冻结正式输入 action/key、目录供给者和玩家选择界面责任，再由现有 PlayerController/输入路由持有本组合器所需依赖；不得在组合器内建立第二份知识、选择、资源或重试状态。

## 10. GitHub 交接

基线提交：`9498f9ed43a4bcc936a1842bec1fb4aa3f2e22b2`（P27.10）。分支：`agent/0.0.10-p27-11-formation-selection-input-route`。本阶段只提交 3 个新增实现/测试文件、1 个既有集成测试文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.11` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-11-formation-selection-input-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-11-formation-selection-input-route/Docs/Report/Dev.D.UE.0.0.10.P27.11.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-11-formation-selection-input-route/Docs/Log/Dev.D.UE.0.0.10.P27.11.r0_log.md>
