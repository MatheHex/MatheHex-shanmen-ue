# Dev.D.UE.0.0.10.P27.12.r0 Development Log

## 1. 目标

- 将 P27.11 组合器绑定到真实 ItemAuthority、CombatRun、FormationRunLifecycle 与共享 SpiritEnergyController；
- 删除调用方注入 RunId、OwnerId 或自由生命周期回调的产品级绕行面；
- 保持 gameplay gate 早于依赖检查和个人知识读取；
- 证明依赖拒绝零访问、未知阵图零生命周期、首次扣费、精确重放与冲突失败关闭；
- 完成改动驱动回归、两目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`1b206cd875f40b71e8dd970ec56676129c0d27bf`（P27.11）；
- 分支：`agent/0.0.10-p27-12-formation-start-product-route`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不修改 Profile/schema、物品真值、阵图内容、按键、UI、PlayerController、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、Cook 或 Package。

## 3. 新增具体产品入口

新增：

- `Source/demo_map/demo_mapShanmenFormationDiagramStartProductRoute.h`；
- `Source/demo_map/demo_mapShanmenFormationDiagramStartProductRoute.cpp`。

`RouteStart()` 接收四个现有真实产品依赖，不接收 RunId、OwnerId 或生命周期函数。内部生命周期函数固定调用：

`Lifecycle.TrySubmit(Authority, Coordinator, SpiritEnergyController, Intent)`。

所有身份来自 Coordinator，并在调用 P27.11 前后核对。入口无持久状态、缓存、重试、序号或随机身份。

## 4. 顺序与失败关闭

实现先处理 gameplay gate；被阻止时以空身份调用现有输入 preflight，依赖验证次数为 0，个人知识与生命周期调用为 0。

允许 gameplay 后只进行一次依赖验证，依次检查 ready Coordinator、active/valid Lifecycle、Lifecycle Run、active/valid SpiritEnergy、SpiritEnergy Run 与玩家 Owner。任一失败都让 P27.11 以 lifecycle unavailable 结束，保留有效嵌套证明且不读取个人知识。

依赖有效时 P27.11 才能读取个人知识、选择阵图、捕获空间样本并派生 Intent。完成后重新检查所有绑定；依赖变化或嵌套证据无效时返回 `StateDesynchronized`。

## 5. 测试实现

在现有 Formation ProductHost 集成 fixture 上增加三个专项：

1. `GameplayAndDependencyFences`：用无效依赖证明 gameplay-first，并覆盖缺 Coordinator、缺 Lifecycle、跨 Run Lifecycle、缺共享灵力与跨 Run 共享灵力；全部保持 0 知识读取、0 序号消费、0 共享事务；
2. `PersonalKnowledgeFence`：真实依赖全部有效，但个人未习得阵图；只读一次知识，生命周期、序号与灵力均不变化；
3. `SharedEnergyReplayAndConflict`：首次 100→90；精确重放复用 Selection/Intent/Receipt；相同事件改变 origin 返回 `IntentIdConflict`，余额、事务与序号不再变化。

每个真实 fixture 最终通过唯一 FormationRunLifecycle 清理。

## 6. 自动化结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationDiagramStartProductRoute` | 3 | 0 | `110BD1D952265983B24FA6FAAF96C7FED85DA2FA244AB2EB169012EB9213CBB7` |
| `Shanmen.0_0_10` | 1,353 | 0 | `77EC9EFB3C5B03BCF027DD00126EDE9467A079ED3032AC7BA11FF009AB0AA7BA` |
| `demo_map.V3.Attributes` | 4 | 0 | `7C402B85A00B399460958ED6F9276BD0544DD745A4BFDCF893F8153EDA5A5230` |

所有最终日志均具备原生退出 0、UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组从 15:39:20.678 UTC 运行至 16:44:53.397 UTC，共 65 分 32.719 秒。

## 7. 回归映射

- `Scripts/ShanmenRegressionMap.json` 新增 `FormationDiagramStartProductRoute` 规则；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1` 新增专项 fixture、正向映射和负向“专项不可替代依赖证明”测试；
- JSON 解析：PASS；
- 正反自测：`476/476` PASS；
- 覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=50 Logs=3`；
- `git diff --check`：PASS。

## 8. 构建证据

Editor：

- 5 actions；`Result: Succeeded`；原生退出 0；14.81 秒；
- run-state SHA-256：`FE92C962B23707A01A7E588C5464FC94455903419699EE38D06E7084D01EFD99`；
- stdout SHA-256：`1A1F08DA546DB54C230203046DE43E4D80416309291EAE26871EDE972CECE21E`；
- `UnrealEditor-demo_map.dll`：19,629,568 bytes；SHA-256 `57C994EE10625DDBFB35C983758C76FAA33C8C7962001D8C82602868070FD3F5`。

Game：

- 4 actions；`Result: Succeeded`；原生退出 0；23.53 秒；
- run-state SHA-256：`53E1FD55CFA964A58F319730CBB61DE2BDD4EB0C82FDA1C3DC038CF15F05D3EC`；
- stdout SHA-256：`2B4166B06091457C40B145890466AF578DCACD2380B20743127446F93D6585B4`；
- `demo_map.exe`：360,256,512 bytes；SHA-256 `8B7B49D23F37821FACFD21BF6A5EB0FCBB7CE7D6F8D25129C20F662FD479E167`。

两次 stderr 均为 0 bytes，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

## 9. 静态边界与 P/F

- 新增生产入口：99 行非空 header + 306 行非空 cpp；
- 无 World/Actor/UI/PlayerController/Enhanced Input、随机 GUID/RNG、Profile/schema、Inventory、伤害或第二资源权威；
- 不新增正式阵图内容、InputAction、键位、地图、资产或配置；
- P 阶段完成原生编译、无头自动化与证据校验；
- F 阶段未启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationDiagramStartProductRoute.h`
2. `Source/demo_map/demo_mapShanmenFormationDiagramStartProductRoute.cpp`
3. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.12.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.12.r0_log.md`

`Saved/Codex/P27.12` 与 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.12.r0` 不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-12-formation-start-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-12-formation-start-product-route/Docs/Report/Dev.D.UE.0.0.10.P27.12.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-12-formation-start-product-route/Docs/Log/Dev.D.UE.0.0.10.P27.12.r0_log.md>
