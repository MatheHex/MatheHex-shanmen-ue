# Dev.D.UE.0.0.10.P27.6.r0 Development Log

## 1. 目标

- 在阵法开始输入前建立正式的阵图目录、玩家知识和选择证据边界；
- 使开始输入只能携带当前玩家已知、且属于当前内容目录的阵图；
- 在 Intent/生命周期调用前拒绝无 Owner 或跨玩家选择；
- 保持选择端口无状态、确定性，并不拥有内容、进度、库存、World、输入或 UI；
- 完成完整回归、改动驱动覆盖、Report/Log 提交及 GitHub 推送。

## 2. 基线与范围

- 基线：`d44ac384c64e3aaf37aa2120788a32736eab7552`（P27.5）；
- 分支：`agent/0.0.10-p27-6-formation-diagram-selection`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不新增正式阵图、材料、锚点、配方、效果、数值、按键、Input Action、UI、地图或资产；
- 不改变内容加载、玩家进度、存档、物品、World 或 Combat Run 的权威归属；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. 阵图目录

新增 `Fdemo_mapShanmenFormationDiagramCatalogCapture` 与不可变目录。捕获时校验内容 stamp、非空列表、
每个既有阵图定义和唯一阵图身份。目录保留作者顺序，以内容 stamp 加完整有序阵图/锚点/材料几何派生
确定性 `CatalogId`。

测试证明完全重放身份稳定，内容、几何和作者顺序漂移都会改变身份；重复、无效或空目录失败关闭。

## 4. 玩家知识与选择

新增不可变知识快照，绑定目录、内容、玩家 `OwnerId`、权威 revision 和规范排序后的已知阵图集合。
重复、空身份和目录外知识被拒绝；空集合可作为“尚未习得任何阵图”的合法状态。

选择端口只允许从匹配目录中选择该玩家已知的阵图。成功选择绑定目录、知识快照、Owner、revision、内容与
精确阵图并派生 `SelectionId`；未知、未习得、空请求或陈旧目录返回显式失败状态且不携带有效选择。

## 5. 输入链改造

开始输入样本由直接持有阵图定义改为持有选择证据，`GetDiagram()` 仅从有效选择中暴露既有不可变定义。
`RouteStartInput()` 新增 `OwnerId`，并在一次采样后、Intent 派生前校验选择 Owner。

新增 `OwnerUnavailable` 与 `SelectionOwnerMismatch` 状态；失败结果必须保持零生命周期调用和无 Intent
证据。现有 ProductHost 测试通过纯测试目录/知识/选择 helper 更新，不向生产代码添加 fixture 身份。

## 6. 测试结果

新增三项 `FormationDiagramSelection` 自动化测试：

- `CatalogIdentityAndDrift`；
- `KnowledgeAndCatalogFences`；
- `InputEvidence`。

同时回归既有输入适配器、部署运行时和 ProductHost 属性入口。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationDiagramSelection` | 3 | 0 | `7DFA47C66AC908E10C7F4119089892A7C0F0D18E8C237A74043C6114ABEE4C62` |
| `Shanmen.0_0_10.Product.FormationInputAdapter` | 3 | 0 | `F7A06DD785828C48D9CA5ED00D2151815C9887529C4884A4F45BB4B60E1A2EB9` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `DDC7EFF635DA84C0B9FC568E12D76780317CCE8D5304546F20783E9394A8446D` |
| `Shanmen.0_0_10` | 1,336 | 0 | `9A25F9FF2B27B77969D90F29AC05B9B93F712E6FC4FC3142F3D5B53F0391BE9C` |
| `demo_map.V3.Attributes` | 4 | 0 | `C95EA793D3A94CD76A314A3ED240283BEA02D348D1CBF99093797AD7A28A3262` |

所有最终测试进程要求原生退出 0，且日志为 0 Fail、0 Fatal/Unhandled/Ensure/Assertion并包含自动化完成证据。

## 7. 回归执行环境修正

完整根组首次执行显示 UE 5.8 HomeScreen 在命令行测试期间反复探测
`https://www.google.com/generate_204`，为每个短测试引入额外超时。通过项目证据目录中的临时
`ConsoleVariables.ini` 在启动前设置只读 CVar `HomeScreen.EnableHomeScreen=0`，重新运行后探测数为 0。

该修正只作用于本次 `UnrealEditor-Cmd` 进程，不修改 Windows、引擎安装、项目正式配置或用户全局配置。
剩余长耗时来自既有复杂回放测试自身，故最终仍以单进程根组日志作为严格覆盖证据。

## 8. 改动驱动覆盖与构建

`Scripts/ShanmenRegressionMap.json` 新增 `FormationDiagramSelectionPort` 规则；其正向 fixture 要求根组、
选择、输入和部署日志，负向 fixture 证明只跑选择焦点不能替代下游证据。映射器正反自测
`467/467` PASS，JSON 解析通过。

最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=8 Rules=3 Required=41 Logs=5`。

| Target | Result | Duration |
|---|---|---:|
| `demo_map` Win64 Development | Rebuilt 6 actions, Succeeded / native 0 | 23.63s |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / native 0 | 1.11s |

二进制：

- `demo_map.exe`：360,147,456 bytes；SHA-256
  `FDE41C994BF9669194E9B76FC12826B58B0FF333B41D9FF6EFBD0C24ECA7ACC2`；
- `UnrealEditor-demo_map.dll`：19,500,544 bytes；SHA-256
  `8D18AF6E3AD9E84346472989C4FFCF8A70CC8B07E80A3FB2F32DA7D256965326`。

## 9. 静态边界与下一步

`git diff --check`、regression map JSON 解析均 PASS。生产代码 scoped scan 未出现 `FMath::Rand`、
`FGuid::NewGuid`、`EKeys`、`InputAction`、`UWorld`、`AActor`、`demo_mapGameMode` 或测试身份。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
下一阶段由现有进度/存档权威提供当前目录及玩家知识快照，再接入物理输入路由；目录加载、解锁写入与
正式内容 authoring 不属于本端口。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationDiagramSelectionPort.h`
2. `Source/demo_map/demo_mapShanmenFormationDiagramSelectionPort.cpp`
3. `Source/demo_map/demo_mapShanmenFormationDiagramSelectionPortTests.cpp`
4. `Source/demo_map/demo_mapShanmenFormationInputAdapter.h`
5. `Source/demo_map/demo_mapShanmenFormationInputAdapter.cpp`
6. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
7. `Scripts/ShanmenRegressionMap.json`
8. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
9. `Docs/Report/Dev.D.UE.0.0.10.P27.6.r0_report.md`
10. `Docs/Log/Dev.D.UE.0.0.10.P27.6.r0_log.md`

`Saved/Codex/P27.6` 原始证据不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-6-formation-diagram-selection>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-6-formation-diagram-selection/Docs/Report/Dev.D.UE.0.0.10.P27.6.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-6-formation-diagram-selection/Docs/Log/Dev.D.UE.0.0.10.P27.6.r0_log.md>
