# Dev.D.UE.0.0.10.P28.22.r0 Report

## 1. 状态与范围

`P28_22_PASS`，仅指本阶段契约与完整验证通过，不是整体冻结或F验收。基线P28.21 / `7523758c24124ae18344dfa6cda7fb6f581a6aaa`，2026-09-20。本轮闭合普通容器部分生成失败后的原owner保持与同World恢复。实际Red证明生成回滚丢失owner、重试将运行时对象误当World提供对象；首修恢复测试又暴露固定内部对象名冲突导致的原生退出3。两份失败及最初夹具错误原件均保留；最终专项、完整新旧根、双构建与改动覆盖均通过。

## 2. 入口与实际反例

InitializeWorldContent的M01分支调用InitializeCodeBNormalContainerTarget。P28.21只闭合入口旧批次清理；其内部安全落点失败及生成/身份失败分支仍无条件清空创建数组，忽略Destroy拒绝。本轮使用瞬态锚点和地板，让实际生成的第一容器拒绝销毁，并在其生成回调内关闭地板碰撞，使第二容器安全落点失败。

实际Red中第一项最初标记RuntimeAdapter；回滚后原owner丢失。恢复地板后再次初始化，第一项被误记为MapAuthored，第二项新建并错误返回成功。四条最终Expected断言失败，0成功/1失败、队列1、原生0。最初锚点偏移超出测试地板的执行只算夹具错误，不能作为这个产品反例。

首修使拒绝owner保留下来；恢复role后，旧对象真实释放，立即重建却因固定UObject名仍被占用而崩溃。该次专项仅完成3个其他测试，原生3、无队列结束，不能记为专项通过。

## 3. 最小修复与契约

只改既有初始化函数：提取局部ReleaseSpawnedTargets，共用于入口和两个失败回滚分支；真实拒绝项保留原弱引用，有未释放owner不继续World查找或物化，全部完成才清登记。生成/身份失败分支先登记已生成对象，再走同一释放逻辑；该分支未单独注入失败，不能冒充专项覆盖。

容器内部命名使用UE既有Requested模式：冲突时生成唯一UObject名，不改变MapTargetIdentity。源码核对本机UE5.8的World.cpp默认NameMode为Required_Fatal，LevelActor.cpp发现占名后按该模式Fatal；Requested则调用引擎现有唯一命名。没有改引擎、强制GC、引入新GUID/业务ID，或降低身份校验。

扩展既有ManagerDeactivationRetention：第二项失败、两次释放拒绝、同World立即恢复、两个稳定目标ID、恢复对象名不同于已退休对象、原对象恰好释放一次、健康重放不替换owner，以及原Run和非零持久快照不变。无新注册测试、友元、公共API、schema或恢复记录。当前生产20行新增/24行删除，测试62行新增。

## 4. 验证状态

| 验证 | 当前证据 |
|---|---|
| 正确夹具Red | 0成功/1失败、队列1、原生0、崩溃指标0；4条最终Expected失败 |
| 首修专项 | 原生3、3成功/0失败但无队列结束，固定内部对象名Fatal；明确失败 |
| Recovery1 Editor/Game | 各4 actions，20.57/35.98秒，均SUCCEEDED/原生0 |
| Recovery1 WorldLifecycle/ProductFlow | 7/7、5/5成功，队列7/5、原生0、崩溃指标0 |
| Recovery1 完整旧根 | 1330成功/0失败、队列1330、原生0、崩溃指标0 |
| Recovery1 完整新根 | 1430成功/0失败、队列1430、原生0、崩溃指标0；20:52:14.1109461UTC完成 |
| 映射自检 | 529/529通过、退出0；不替代产品测试或实际覆盖 |
| 改动驱动实际覆盖 | 5路径、2规则、7必跑组、2份本阶段健康完整日志，核验通过 |

19:38:51.5702029UTC锁定1617产品输入和103原用户文件。exec38983按Editor→专项7/5→Game→旧根1330→新根1430串行完成，外层退出0，各段前后输入检查通过；运行期间未改源码或重复启动。14项原件与SHA及过程检查点见[Development Log](../Log/Dev.D.UE.0.0.10.P28.22.r0_log.md)。两根共2760个独立成功用例，专项属于完整根子集，不重复计数。首修失败专项与最后完整日志不拼接。

两个源码改动路径要求覆盖demo_map.CodeB、ItemUseAndArmor、P4.Hotbar、Profile、V2RangedCompatibility以及Shanmen.0_0_10和Items组；这些组均由本轮健康完整日志通过映射核验，不借用P28.21日志，不以主题选测代替。

## 5. 边界与交接

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。本轮只证明私有底层初始化端口中的实际生成/回滚/恢复，不替代完整M01激活、正式地图、玩家互动、Code B持久交易或所有其他容器失败分支。没有Editor UI、PIE、Standalone、产品exe、玩法/UI、Smoke/Cook/Package。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)补充本阶段证据，FZ-1/2仍开放。只精确交接Manager cpp、既有测试、索引、本Report/Log五路径；103原未跟踪文件与两份OverallReadiness修改不动。Saved原始日志只保留本地，仓库Log提供路径与SHA，不宣称原件已上传。

下一步仍核对其他容器/物品/空间包释放、强制EndPlay及全入口权威路由的真实可达条件；不因本轮通过而扩充通用恢复系统、进入实际玩法或宣布最终架构冻结。
