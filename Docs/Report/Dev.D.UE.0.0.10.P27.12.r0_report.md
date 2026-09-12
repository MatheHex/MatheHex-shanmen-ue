# Dev.D.UE.0.0.10.P27.12.r0 Report

## 1. 结论

P27.12 已把 P27.11 的“阵图知识访问 → 设备无关开始输入”组合器接入真实产品依赖，形成唯一的具体阵法开始入口 `Fdemo_mapShanmenFormationDiagramStartProductRoute`。

调用方不再提交可自由替换的生命周期回调，也不能自行填写 RunId 或 OwnerId。入口只接收现有 ItemAuthority、CombatRunCoordinator、FormationRunLifecycle、共享 SpiritEnergyController、稳定 InputEventId、活动阵图目录与个人知识读取函数；Run 与玩家身份只从 Combat Run 获取，最终提交固定调用现有生命周期的共享灵力签名。

本轮没有新增第二份阵法、物品、资源、知识、输入或 Run 权威。专项 3/3、完整 `Shanmen.0_0_10` 1,353/1,353、属性交叉组 4/4、Editor/Game 两目标构建、回归映射自测与最终覆盖门全部通过。

## 2. 阶段问题与范围

P27.11 已保证知识读取、阵图选择和空间采样只在输入前置门通过后发生，但调用方仍需提供 RunId、OwnerId 与任意生命周期函数。错误接线仍可能把一个 Run 的阵法请求交给另一 Run 的生命周期或共享灵力控制器，也可能绕开 P27.10 的唯一扣费入口。

本轮只封闭这条最后的产品接线：

- gameplay gate 必须早于任何产品依赖检查和个人知识读取；
- RunId 与 OwnerId 只取自 ready CombatRunCoordinator；
- FormationRunLifecycle 必须 active、valid 且绑定同一 Run；
- SpiritEnergyController 必须 active、valid，并绑定同一 Run 与同一玩家；
- 依赖通过后只调用 P27.11 组合器；
- 组合器的固定生命周期函数只调用 `Lifecycle.TrySubmit(Authority, Coordinator, SpiritEnergyController, Intent)`；
- 返回后再次核对所有绑定，变化或无效证据一律 `StateDesynchronized`；
- 不绑定物理按键、UI、PlayerController、地图、资产或正式阵图内容。

## 3. 固定路由顺序

`RouteStart()` 的顺序冻结为：

1. gameplay gate；
2. 一次产品依赖验证；
3. Combat Run ready 与 Run/Owner 捕获；
4. Formation 生命周期 active/valid/Run 一致；
5. 共享 SpiritEnergy active/valid/Run/Owner 一致；
6. P27.11 惰性组合器完成事件、目录、个人知识、选择与空间样本；
7. P27.4 InputAdapter 派生唯一 IntentId；
8. P27.10 RunLifecycle 通过共享 SpiritEnergyController 提交；
9. 返回后重新核对依赖绑定。

gameplay 被阻止时 `DependencyValidationCount=0`，即使传入的 Coordinator、Lifecycle、SpiritEnergy、目录、事件与空间样本全部无效，也不会检查依赖、读取个人知识或调用生命周期。产品依赖失败时，嵌套输入证明固定为 `InputCompletedBeforeAccess / LifecycleUnavailable`，知识访问、空间采样和生命周期调用均为 0。

## 4. 可审计结果契约

顶层结果保留状态、诊断、依赖验证次数、捕获的 Run/Owner/LifecycleRun/SpiritEnergyRun/SpiritEnergyOwner，以及完整 P27.11 Composition 证明。

- `GameplayBlocked`：先于依赖完成；
- `CoordinatorUnavailable`：Combat Run 未 ready；
- `LifecycleUnavailable` / `LifecycleRunMismatch`：生命周期不可用或属于另一 Run；
- `SpiritEnergyUnavailable` / `SpiritEnergyRunMismatch` / `SpiritEnergyOwnerMismatch`：共享灵力控制器不可用或绑定不一致；
- `Routed`：具体依赖已核对，P27.11 已产生一个合法接受或拒绝结果；
- `StateDesynchronized`：嵌套证据无效或路由期间依赖绑定发生变化。

`Routed` 不等于业务接受：未知阵图等合法拒绝仍保留为有效审计结果，只有嵌套 Composition 真正接受时顶层 `IsAccepted()` 才为真。

## 5. 共享灵力与幂等证明

真实集成使用现有 ItemAuthority、CombatRunCoordinator、FormationRunLifecycle 与共享 SpiritEnergyController：

- 首次已知阵图事件成功启动，阵图激活成本使灵力从 100 降至 90；
- 只创建一个共享外部灵力事务，只占用一个阵法激活序号；
- 完全相同 InputEventId、目录 revision、阵图与空间载荷重放，复用同一 SelectionId、IntentId 和资源 ReceiptId；
- 重放后灵力仍为 90、事务仍为 1、序号不增加；
- 相同 InputEventId 配不同 origin 时，既有 Controller 返回 `IntentIdConflict`，不再次扣费或推进序号；
- 最终仍由唯一 FormationRunLifecycle 完成产品清理与 Combat Run 释放。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationDiagramStartProductRoute` | 3 | 0 | `110BD1D952265983B24FA6FAAF96C7FED85DA2FA244AB2EB169012EB9213CBB7` |
| `Shanmen.0_0_10` | 1,353 | 0 | `77EC9EFB3C5B03BCF027DD00126EDE9467A079ED3032AC7BA11FF009AB0AA7BA` |
| `demo_map.V3.Attributes` | 4 | 0 | `7C402B85A00B399460958ED6F9276BD0544DD745A4BFDCF893F8153EDA5A5230` |

三份日志均有 UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、测试进程原生退出 0、0 Fail 与 0 Fatal/Unhandled/Ensure。专项耗时 6.107 秒，完整根组耗时 65 分 32.719 秒，属性组耗时 5.563 秒。

## 7. 改动驱动回归

新增 `FormationDiagramStartProductRoute` 映射规则，要求根组以及 P27.11 composition、访问、知识、选择、输入、生命周期、产品控制器、Combat Run、产品 Host、材料、物品、WorldGameplay、部署、行动资源、Divine Sense、Spirit Shield 与 CombatCore 证据。

- regression map JSON：PASS；
- 映射器正反自测：`476/476` PASS；
- 负向自测确认只有 P27.12 专项不能替代依赖层与共享资源证明；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=50 Logs=3`；
- `git diff --check`：PASS。

## 8. 构建与静态边界

- Editor：5 actions，`SUCCEEDED`，原生退出 0，14.81 秒；stdout SHA-256 `1A1F08DA546DB54C230203046DE43E4D80416309291EAE26871EDE972CECE21E`；
- `UnrealEditor-demo_map.dll`：19,629,568 bytes，SHA-256 `57C994EE10625DDBFB35C983758C76FAA33C8C7962001D8C82602868070FD3F5`；
- Game：4 actions，`SUCCEEDED`，原生退出 0，23.53 秒；stdout SHA-256 `2B4166B06091457C40B145890466AF578DCACD2380B20743127446F93D6585B4`；
- `demo_map.exe`：360,256,512 bytes，SHA-256 `8B7B49D23F37821FACFD21BF6A5EB0FCBB7CE7D6F8D25129C20F662FD479E167`；
- 两次 stderr 均为空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增生产入口共 405 行非空行（header 99 + cpp 306）。它没有 World/Actor/PlayerController/Enhanced Input、物理键位、随机 GUID/RNG、Profile/schema、Inventory 或伤害调用；只组合既有产品所有者。没有新增内容资产、地图或配置改动。

## 9. P/F 边界与下一阶段

P 阶段已经证明具体产品依赖的同 Run/同 Owner 绑定、gameplay-first 零读取、个人知识拒绝零副作用、共享灵力首次扣费、同事件重放、载荷冲突失败关闭、完整回归、覆盖门和双目标构建。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。因此仍不声明玩家已有正式按键或阵图 UI。下一阶段应在正式 InputAction、键位与目录/选择界面责任明确后，让现有输入所有者持有本路由的四个真实依赖；不得恢复自由生命周期回调或调用方注入 Run/Owner。

## 10. GitHub 交接

基线提交：`1b206cd875f40b71e8dd970ec56676129c0d27bf`（P27.11）。分支：`agent/0.0.10-p27-12-formation-start-product-route`。本阶段只提交 2 个新增生产文件、1 个既有集成测试文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.12` 与构建证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-12-formation-start-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-12-formation-start-product-route/Docs/Report/Dev.D.UE.0.0.10.P27.12.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-12-formation-start-product-route/Docs/Log/Dev.D.UE.0.0.10.P27.12.r0_log.md>
