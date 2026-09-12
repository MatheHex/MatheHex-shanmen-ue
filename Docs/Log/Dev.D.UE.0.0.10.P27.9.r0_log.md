# Dev.D.UE.0.0.10.P27.9.r0 Development Log

## 1. 目标

- 补齐阵图定义中缺失的激活能量要求；
- 复用既有共享行动资源模型，不创建阵法专用余额或事务系统；
- 让能量要求进入目录、部署及产品一致性身份；
- 对缺失、错误通道和非正金额失败关闭；
- 完成完整回归、改动驱动覆盖、Report/Log 提交与 GitHub 推送。

## 2. 基线与范围

- 基线：`df36d9dcf8119e2ea4946d7397fdb89442593846`（P27.8）；
- 分支：`agent/0.0.10-p27-9-formation-energy-requirement-contract`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不修改 Profile schema、迁移、物品真值、正式内容、按键、UI、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. 阵图能量类型

`FShanmenFormationDiagramCapture` 增加 `ActivationEnergyCost`，使用现有
`FShanmenActionResourceCostCapture`。捕获成功后冻结为只读 `FShanmenActionResourceCost`；规范通道直接引用
`FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()`。

捕获要求规则身份有效、通道精确匹配、金额有限且大于零。任一条件不满足时清空输出并返回 false。生产代码
没有写入正式金额或具体平衡规则；测试中的 10/11/12/13 等数值只用于证明身份漂移与拒绝边界。

## 4. 身份与产品传播

CostId 加入阵图目录和部署 canonical parts，身份命名空间分别升级到
`demo_map.Formation.DiagramCatalog.r2` 与 `Shanmen.Formation.Deployment.r2`。能量金额改变会产生不同 CostId、
CatalogId 和 DeploymentId。

产品权威及控制器的阵图比较加入 CostId。控制器对 Command、Intent、Session 与 Deployment 的后置完整性
检查改用完整 `DiagramsMatch`，防止同名阵图在锚点、材料或能量条件漂移后越过只比较 DiagramId 的旧门。

## 5. Fixture 迁移与测试扩展

全部既有阵图 fixture 经公开 Capture API 补充测试专用共享能量成本，覆盖访问适配器、选择、知识、材料、
产品 Host/Session、World 适配及影响消费者链。没有测试直接写入不可变私有状态。

核心测试新增：冻结值检查、缺失成本、错误通道、零金额拒绝，以及能量变化导致 DeploymentId 改变。目录测试
新增能量变化导致 CatalogId 改变，并验证缺少能量的目录失败关闭。

## 6. 自动化结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `B7CACBA02873BB763D9D1F98800003E933938462AFD99BBD1FCCA7509A7C680E` |
| `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `6474FF86AA70AAA9738146E8B8A797D93E6A8AB4EDDB51DF1D87DD81B0934E73` |
| `Shanmen.0_0_10.Product.FormationDiagramSelection` | 3 | 0 | `1C5478EA4E808FABBAFB755BC928F7DC187918A9A9B62A3CD5C4D00906FB3493` |
| `Shanmen.0_0_10.Product.FormationProductController` | 2 | 0 | `985DDDE11F5ABE1A551BC8B9467C2DA0265B3386A71BCAF06481837787C92E46` |
| `demo_map.V3.Attributes` | 4 | 0 | `710413480F9A9500116CD5950596DDFF4E0FC371B72C2F532E65E37E35957D00` |
| `Shanmen.0_0_10` | 1,344 | 0 | `E376582E7966D868BA1F89BD4BDE65D998A5CEB3DC001ED261F3C758E4CC0669` |

所有最终进程原生退出 0；日志为 0 Fail、0 Fatal/Unhandled/Ensure/Assertion，并包含 UE 5.8 原生完成证据。

## 7. 回归映射与覆盖门

`FormationDeploymentCore` 从单一部署组提升为完整根组 + 部署 + 行动资源 + 阵图选择联合证据，并增加只提供
部署日志的负向 fixture。

- regression map JSON：PASS；
- 映射器正反自测：`472/472` PASS；
- 首次门检：准确缺少 `demo_map.V3.Attributes`；
- 补跑后最终门检：`REGRESSION_COVERAGE: PASS Changed=17 Rules=13 Required=45 Logs=6`；
- `git diff --check`：PASS。

## 8. 构建与二进制

| Target | Result | Duration |
|---|---|---:|
| `demo_map` Win64 Development | Rebuilt 95 actions, Succeeded / native 0 | 81.32s |
| `demo_mapEditor` Win64 Development | Latest code rebuilt; final 0-action up-to-date check / native 0 | 1.83s final check |

- `demo_map.exe`：360,196,608 bytes；SHA-256
  `231C2EEC5F9DB37B06F4F0C99F3DCF1624039538986F3A87C80146D7A4A6324F`；
- `UnrealEditor-demo_map.dll`：19,558,400 bytes；SHA-256
  `D54FB7581A8D7311D246AE14ED35D0B3F42FCA38227C76812F8D910C810CB43D`。

## 9. 静态边界与下一步

新增生产 diff 的 scoped scan 对随机 GUID/RNG、物理输入/UI、Profile/schema、`UWorld`/`AActor` 和正式能量
数值字面量均为 0。没有新增正式内容资产或地图改动。

本阶段不扣除资源。下一阶段应由既有共享行动资源权威在阵法 Run 生命周期中完成 reserve、commit 与
release，再把 P27.8 的安全选择结果组合进既有阵法输入；不得新增第三套资源权威或临时物理输入。

## 10. 精确提交清单

1. `Scripts/ShanmenRegressionMap.json`
2. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
3. `Source/ShanmenCombatRuntime/Public/ShanmenFormationDeployment.h`
4. `Source/ShanmenCombatRuntime/Private/ShanmenFormationDeployment.cpp`
5. `Source/ShanmenCombatRuntime/Private/Tests/ShanmenFormationDeploymentTests.cpp`
6. `Source/demo_map/demo_mapShanmenFormationDiagramAccessAdapterTests.cpp`
7. `Source/demo_map/demo_mapShanmenFormationDiagramSelectionPort.cpp`
8. `Source/demo_map/demo_mapShanmenFormationDiagramSelectionPortTests.cpp`
9. `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductBridgeTests.cpp`
10. `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductRuntimeTests.cpp`
11. `Source/demo_map/demo_mapShanmenFormationKnowledgeAuthorityAdapterTests.cpp`
12. `Source/demo_map/demo_mapShanmenFormationMaterialAdapterTests.cpp`
13. `Source/demo_map/demo_mapShanmenFormationProductAuthority.cpp`
14. `Source/demo_map/demo_mapShanmenFormationProductController.cpp`
15. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
16. `Source/demo_map/demo_mapShanmenFormationProductSessionTests.cpp`
17. `Source/demo_map/demo_mapShanmenFormationWorldAdapterTests.cpp`
18. `Docs/Report/Dev.D.UE.0.0.10.P27.9.r0_report.md`
19. `Docs/Log/Dev.D.UE.0.0.10.P27.9.r0_log.md`

`Saved/Codex/P27.9` 原始证据不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-9-formation-energy-requirement-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-9-formation-energy-requirement-contract/Docs/Report/Dev.D.UE.0.0.10.P27.9.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-9-formation-energy-requirement-contract/Docs/Log/Dev.D.UE.0.0.10.P27.9.r0_log.md>
