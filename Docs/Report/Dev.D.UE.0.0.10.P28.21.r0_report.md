# Dev.D.UE.0.0.10.P28.21.r0 Report

## 1. 状态与范围

`P28_21_PASS`，仅指本阶段契约与验证完成，不是整体冻结或F验收。基线P28.20 / `1578b0058db90dc7d7c9dac570845807ec8eeada`，2026-09-20。本轮闭合Manager普通容器投影重新初始化入口的原运行时owner释放拒绝保持。实际Red为0成功/1失败（三条最终Expected断言）；最小修复后，Editor/Game双构建、专项7/5、完整新根1430/旧根1330及实际改动覆盖全部通过。

## 2. 入口与反例

InitializeWorldContent的M01分支在内容准备后调用InitializeCodeBNormalContainerTarget；其失败只报告投影不可用，不能改变Code A Run权威。私有初始化在两条已登记目标均有效时直接复用，否则先清旧Manager创建的对象再扫描World现有对象。

旧实现忽略Destroy拒绝并无条件清两个数组。部分登记失效时，仍存活的拒绝对象因此失去运行时owner，下一次初始化无法继续原释放；World查找还可能将其当成非Manager创建的现存目标。实际Red证明的是原owner丢失和后续无法释放，不冒充已复现所有World重登记组合。

扩展既有ManagerDeactivationRetention：同一瞬态World/实际AuthGameMode中，两个容器对象持有不同正式静态目标ID，一个真实销毁导致部分登记失效，另一个真实拒绝销毁两次；验证原owner/登记、原Run和非零持久快照保持。恢复后必须恰好释放一次；没有正式anchor仍应拒绝新物化，不能以清理完成冒充初始化成功。随后以两个World提供、未加入Manager创建数组的对象验证健康查找及重放，不转移其所有权。

## 3. 最小修复

只修改InitializeCodeBNormalContainerTarget开头的旧批次清理：按快照释放，仅保留实际拒绝项；有拒绝就返回false，不清登记或继续扫描/物化。全部释放后才清旧登记。没有修改物品权威、公开接口、schema、地图资产、目标配置或新建恢复记录；没有增加注册测试或友元。

本专项是直接私有底层端口验证，不包含完整M01激活、正式地图、投影生成失败回滚、玩家互动或Code B持久交易；这些不能由本次测试代替。

生产源码仅10行新增/4行删除，既有测试64行新增。没有新增测试注册；成功前缀不重复释放，拒绝期间原Run和非零持久快照保持不变，正常World提供对象仍不被接管为Manager创建对象。

## 4. 验证状态

| 验证 | 当前证据 |
|---|---|
| Red Editor | 4 actions / 20.47秒，SUCCEEDED/原生0 |
| RedProof | 0成功/1失败，队列1、原生0、崩溃指标0；3条最终Expected失败 |
| 最终Editor/Game | 各4 actions，19.61/40.33秒，均SUCCEEDED/原生0 |
| WorldLifecycle/ProductFlow | 7/7、5/5成功，队列7/5、原生0、崩溃指标0 |
| 完整旧根 | 1330成功/0失败，队列1330、原生0、崩溃指标0 |
| 完整新根 | 1430成功/0失败，队列1430、原生0、崩溃指标0；18:18:48.6605929UTC完成 |
| 映射自检/实际覆盖 | 自检529/529通过；实际5路径、2规则、7必跑组、2份本阶段健康完整日志通过 |

17:05:57.9669887UTC锁定1617产品输入和103原用户文件，最终exec3299按Editor→专项7/5→Game→旧根1330→新根1430串行完成并退出0。验证期间没有改产品输入、重复启动或重试完整根；本阶段两根共2760个独立成功用例。专项是完整根的子集，不将重复执行计入独立覆盖总数。10项原件路径与SHA见[Development Log](../Log/Dev.D.UE.0.0.10.P28.21.r0_log.md)，首次Red失败原件保留。

必跑组由两个改动源码路径推导：demo_map.CodeB、ItemUseAndArmor、P4.Hotbar、Profile、V2RangedCompatibility，以及Shanmen.0_0_10和其Items组。完整日志不是用上阶段计数替代；自检也不冒充产品测试。

## 5. 边界与交接

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。不启动Editor UI/PIE/Standalone/产品exe，不改正式地图/资产、物理输入、玩法/UI，不做Smoke/Cook/Package。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)记录本阶段证据，FZ-1/2仍开放。只精确交接Manager cpp、既有测试、索引、本Report/Log五路径；103原未跟踪文件与两份OverallReadiness修改不动，Saved原件仅本地保留，不宣称原始日志已上传GitHub。

后续仍先核对容器生成失败回滚、其他物品/空间包释放和强制EndPlay的真实入口条件，以及全入口权威路由；不因局部通过而新增玩法、通用恢复系统或宣布最终架构冻结。
