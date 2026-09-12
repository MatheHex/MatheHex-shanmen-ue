# Dev.D.UE.0.0.10.P27.6.r0 Report

## 1. 结论

P27.6 已在 P27.4 的输入边界前加入正式的阵图选择端口。产品调用方不再把任意
`FShanmenFormationDiagramDefinition` 直接送入开始输入，而必须先用当前内容目录和当前玩家的阵图知识
快照取得一份不可变、所有者绑定的选择证据。输入适配器会在生成 Intent、调用生命周期之前验证该证据
属于当前玩家，从结构上阻止未知阵图、未习得阵图、陈旧目录和跨玩家选择进入部署链。

本阶段没有写死正式阵图、锚点、材料、配方、效果、数值、输入键或 UI。目录内容和玩家习得状态仍由
未来的调用方权威提供；测试中的身份和材料仅存在于自动化 fixture。

## 2. 阶段问题与范围

P27.5 已把阵法产品接入唯一 Combat Run，但 P27.4 输入样本仍直接接收完整阵图定义。任意调用方因此可以
绕开“玩家是否拥有该阵图知识”和“定义是否来自当前内容版本”两条产品约束。

P27.6 只解决选择证据边界：

- 建立调用方供给、内容身份绑定的只读阵图目录；
- 建立玩家所有者、权威 revision 与已知阵图集合绑定的知识快照；
- 仅允许从同一目录中选择该玩家已知的阵图；
- 把选择证据接入开始输入，并在采样后、Intent 生成前校验所有者；
- 更新改动驱动回归映射及其正反自测。

## 3. 阵图目录契约

`Fdemo_mapShanmenFormationDiagramCatalog::TryCapture()` 接收内容 stamp 和一组既有阵图 capture。目录要求
内容 stamp 有效、至少包含一个阵图、每个阵图可进入既有不可变定义，且阵图身份唯一。目录保留作者顺序，
并将内容版本、digest、完整阵图/锚点/材料几何和顺序共同派生为确定性 `CatalogId`。

完全相同的 capture 会重放为相同目录身份；内容、几何或作者顺序发生变化都会产生不同身份。空目录、
重复阵图或无效定义均失败关闭。端口只持有值快照，不负责加载资产、解锁进度、存档写入或热更新。

## 4. 玩家知识与选择证据

`Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot` 绑定 `CatalogId`、内容 stamp、玩家 `OwnerId`、权威
revision 和已知阵图身份集合。集合在捕获时按名字规范排序，因此调用方枚举顺序不影响 `SnapshotId`；
重复、空身份或目录外身份均被拒绝。空知识集合是合法状态，但不能选择任何阵图。

`Fdemo_mapShanmenFormationDiagramSelectionPort::Select()` 依次验证目录、知识快照、目录/内容一致性、请求
身份、目录成员和玩家已知状态。成功结果把目录、知识快照、所有者、revision、内容和精确阵图共同绑定到
确定性 `SelectionId`；失败结果不携带可用选择。

## 5. 输入边界所有者校验

`Fdemo_mapShanmenFormationStartInputSample` 现在只接收有效
`Fdemo_mapShanmenFormationDiagramSelection`，同时保留既有 origin/facing 归一化和有效性约束。

`RouteStartInput()` 新增当前 `OwnerId` 输入及 `OwnerUnavailable`、`SelectionOwnerMismatch` 两个显式状态。
门控顺序保持有界：gameplay、生命周期、Run、Owner、事件身份、一次采样、所有者匹配、Intent 捕获、一次
生命周期调用。外来所有者选择在 Intent 与产品副作用之前被拒绝，且结果保留精确诊断与调用次数证据。

## 6. 确定性、失败关闭与责任边界

目录、知识和选择身份均由规范字段确定性派生，不调用随机 GUID 或 RNG。相同输入可精确重放；内容、
权限 revision、玩家或阵图变化会改变对应身份。目录漂移不能继续使用旧知识快照。

选择端口是无状态值边界，不拥有目录加载、玩家进度、库存、World、Actor、GameMode、输入设备、UI、效果、
配方、重试队列或产品生命周期。后续接入必须由现有进度/存档权威捕获当前知识快照，并由调用方证明使用的
目录就是当前内容版本。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationDiagramSelection` | 3 | 0 | `7DFA47C66AC908E10C7F4119089892A7C0F0D18E8C237A74043C6114ABEE4C62` |
| `Shanmen.0_0_10.Product.FormationInputAdapter` | 3 | 0 | `F7A06DD785828C48D9CA5ED00D2151815C9887529C4884A4F45BB4B60E1A2EB9` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `DDC7EFF635DA84C0B9FC568E12D76780317CCE8D5304546F20783E9394A8446D` |
| `Shanmen.0_0_10` | 1,336 | 0 | `9A25F9FF2B27B77969D90F29AC05B9B93F712E6FC4FC3142F3D5B53F0391BE9C` |
| `demo_map.V3.Attributes` | 4 | 0 | `C95EA793D3A94CD76A314A3ED240283BEA02D348D1CBF99093797AD7A28A3262` |

五份日志均要求 0 Fail、0 Fatal/Unhandled/Ensure/Assertion、自动化队列完成证据与测试进程原生退出码 0。
根组通过项目侧启动配置关闭 UE HomeScreen 联网探测；没有修改 Windows、引擎安装或用户全局配置。

## 8. 改动驱动回归、构建与静态检查

阵图选择端口规则要求完整 `Shanmen.0_0_10` 根组及选择、输入、部署三条焦点证据；输入适配器和
ProductHost 变更继续触发现有产品/属性回归。最终覆盖门：
`REGRESSION_COVERAGE: PASS Changed=8 Rules=3 Required=41 Logs=5`。映射器正反自测 `467/467`
PASS，regression map JSON 解析通过。

| Target | Result / native exit | Final incremental duration |
|---|---|---:|
| `demo_map` Win64 Development | Rebuilt 6 actions, Succeeded / 0 | 23.63s |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 1.11s |

最终 `demo_map.exe` 为 360,147,456 bytes，SHA-256
`FDE41C994BF9669194E9B76FC12826B58B0FF333B41D9FF6EFBD0C24ECA7ACC2`；
`UnrealEditor-demo_map.dll` 为 19,500,544 bytes，SHA-256
`8D18AF6E3AD9E84346472989C4FFCF8A70CC8B07E80A3FB2F32DA7D256965326`。

`git diff --check` 通过；生产代码 scoped scan 未发现 `FMath::Rand`、`FGuid::NewGuid`、`EKeys`、
`InputAction`、`UWorld`、`AActor`、`demo_mapGameMode` 或测试阵图/材料身份。没有新增正式内容资产或修改地图。

## 9. P/F 边界与下一阶段

P 阶段证明了目录身份与漂移、知识集合规范化、未知/未习得阵图拒绝、目录外知识拒绝、所有者绑定、跨玩家
输入拒绝、确定性重放、完整 0.0.10 回归和改动驱动覆盖。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook
或 Package。因此不声明玩家已能在正式 UI 中选择阵图或通过物理按键部署。下一阶段应由现有进度/存档
权威提供生产目录与玩家知识快照，再把确认后的物理输入路由到本端口；不得把测试 fixture 变成 shipping
内容。

## 10. GitHub 交接

基线提交：`d44ac384c64e3aaf37aa2120788a32736eab7552`（P27.5）。
分支：`agent/0.0.10-p27-6-formation-diagram-selection`。本阶段只提交 8 个实现/测试/映射文件、本 Report
与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.6` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-6-formation-diagram-selection>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-6-formation-diagram-selection/Docs/Report/Dev.D.UE.0.0.10.P27.6.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-6-formation-diagram-selection/Docs/Log/Dev.D.UE.0.0.10.P27.6.r0_log.md>
