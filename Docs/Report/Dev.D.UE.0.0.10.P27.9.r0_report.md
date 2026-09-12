# Dev.D.UE.0.0.10.P27.9.r0 Report

## 1. 结论

P27.9 已把规划中明确存在、但此前阵图类型无法表达的“能量要求”补进阵图不可变契约。每份可部署阵图现在
必须携带一条有效、正值、指向既有共享 `SpiritEnergy` 通道的行动资源消耗；缺失、零值或错误通道均在阵图
捕获阶段失败关闭。能量要求同时进入阵图目录、部署与产品一致性身份，避免不同能量条件的阵图被误认为同一
份内容。

本阶段没有创建第三套资源系统，也没有冻结正式阵图能耗数值。资源金额与规则身份仍由后续正式内容拥有；
本轮只建立表达、验证和身份传播契约，尚未执行 reserve、commit、deduct 或 release。

## 2. 阶段问题与范围

既有阵图定义已经表达 Action、Diagram、锚点结构和材料要求，但没有能量条件。若继续接入选择和输入，后续
只能在产品层临时附加能耗，导致同一 DiagramId 可以对应不同真实成本，而 CatalogId、DeploymentId 和产品
一致性检查仍把它们视为相同内容。

P27.9 只解决该结构缺口：

- 复用 `FShanmenActionResourceCostCapture` / `FShanmenActionResourceCost`；
- 固定阵图激活能量通道为现有共享 `Resource.SpiritEnergy`；
- 要求规则身份有效、金额有限且大于零；
- 将 CostId 纳入目录与部署确定性身份；
- 让既有产品权威和控制器逐层核对能量成本身份；
- 更新全部既有阵图测试 fixture，但不把 fixture 金额当成 shipping 平衡值。

## 3. 能量要求契约

`FShanmenFormationDiagramCapture` 新增 `ActivationEnergyCost`，捕获时交给既有行动资源值类型规范化。只有完整
成本有效且资源通道精确等于 `CanonicalActivationEnergyChannel()` 时，才生成不可变
`FShanmenFormationDiagramDefinition`。

规范通道由 `FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()` 提供，因此阵法不会自造资源标签或独立
余额。不可变定义仅暴露只读 `GetActivationEnergyCost()`；正式规则名称和数值未写入生产代码。

## 4. 确定性身份与传播

能量成本的确定性 CostId 进入阵图目录 canonical parts 与部署 canonical parts。由于规范输入发生变化，目录
身份命名空间升级为 `demo_map.Formation.DiagramCatalog.r2`，部署身份命名空间升级为
`Shanmen.Formation.Deployment.r2`，防止旧身份与新身份跨版本碰撞。

同一内容可重放为同一身份；只改变激活能量金额会同时改变 CatalogId 和 DeploymentId。产品权威及产品控制器
的 `DiagramsMatch` 也核对 CostId；控制器的命令、Intent、Session 和 Deployment 交叉约束改为完整阵图比较，
不再只比较 DiagramDefinitionId。

## 5. 失败关闭与兼容性

自动化覆盖缺失成本、非 `SpiritEnergy` 通道和非正金额三类拒绝，并验证拒绝后不生成有效阵图。所有既有阵图
fixture 均通过公开捕获入口补充测试专用规则与金额，从而继续验证原产品链，而不是绕过新契约直接构造私有
状态。

旧阵法的锚点、材料、选择、知识、产品会话、World 适配及影响消费者语义未改变。没有修改 Profile schema、
迁移、物品真值、地图或资产。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `B7CACBA02873BB763D9D1F98800003E933938462AFD99BBD1FCCA7509A7C680E` |
| `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `6474FF86AA70AAA9738146E8B8A797D93E6A8AB4EDDB51DF1D87DD81B0934E73` |
| `Shanmen.0_0_10.Product.FormationDiagramSelection` | 3 | 0 | `1C5478EA4E808FABBAFB755BC928F7DC187918A9A9B62A3CD5C4D00906FB3493` |
| `Shanmen.0_0_10.Product.FormationProductController` | 2 | 0 | `985DDDE11F5ABE1A551BC8B9467C2DA0265B3386A71BCAF06481837787C92E46` |
| `demo_map.V3.Attributes` | 4 | 0 | `710413480F9A9500116CD5950596DDFF4E0FC371B72C2F532E65E37E35957D00` |
| `Shanmen.0_0_10` | 1,344 | 0 | `E376582E7966D868BA1F89BD4BDE65D998A5CEB3DC001ED261F3C758E4CC0669` |

六个最终测试进程均原生退出 0，日志均有 UE 5.8 `TEST COMPLETE. EXIT CODE: 0` 终止证据，且为 0 Fail、
0 Fatal/Unhandled/Ensure/Assertion。完整根组在一个无头进程内串行完成全部 1,344 项。

## 7. 改动驱动回归

`FormationDeploymentCore` 映射现在要求完整 `Shanmen.0_0_10`、部署、共享行动资源及阵图选择四层证据；新增
负向 fixture 证明只跑部署焦点组不能替代能量、目录和完整回归。

首次覆盖门准确发现 `ProductController` 改动还需要既有 `demo_map.V3.Attributes`，补跑该组 4/4 后最终为：
`REGRESSION_COVERAGE: PASS Changed=17 Rules=13 Required=45 Logs=6`。映射器正反自测 `472/472` PASS，JSON
解析与 `git diff --check` 均通过。

## 8. 构建与静态边界

| Target | Result / native exit | Final evidence |
|---|---|---|
| `demo_map` Win64 Development | Rebuilt 95 actions, Succeeded / 0 | 81.32s |
| `demo_mapEditor` Win64 Development | Latest code rebuilt, final target up to date / 0 | final check 1.83s |

最终 `demo_map.exe` 为 360,196,608 bytes，SHA-256
`231C2EEC5F9DB37B06F4F0C99F3DCF1624039538986F3A87C80146D7A4A6324F`；
`UnrealEditor-demo_map.dll` 为 19,558,400 bytes，SHA-256
`D54FB7581A8D7311D246AE14ED35D0B3F42FCA38227C76812F8D910C810CB43D`。

新增生产 diff 的 scoped scan 未发现随机 GUID/RNG、物理输入/UI、Profile/schema、`UWorld`/`AActor` 或正式
能量数值字面量。没有新增正式内容资产或修改地图。

## 9. P/F 边界与下一阶段

P 阶段证明了能量要求可表达、共享通道约束、非法输入拒绝、确定性身份传播、旧产品链兼容、完整回归以及
改动驱动覆盖。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或
Package；也没有声称资源已真实扣减。下一阶段应把阵图 Cost 接入既有 Run 行动资源权威的
reserve/commit/release 生命周期，再把 P27.8 的安全阵图选择组合进既有阵法输入链；不得新增第三套资源余额
或用临时按键冻结产品语义。

## 10. GitHub 交接

基线提交：`df36d9dcf8119e2ea4946d7397fdb89442593846`（P27.8）。
分支：`agent/0.0.10-p27-9-formation-energy-requirement-contract`。本阶段精确提交 17 个实现/测试/流程文件、
本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.9` 原始证据不
进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-9-formation-energy-requirement-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-9-formation-energy-requirement-contract/Docs/Report/Dev.D.UE.0.0.10.P27.9.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-9-formation-energy-requirement-contract/Docs/Log/Dev.D.UE.0.0.10.P27.9.r0_log.md>
