# Dev.D.UE.0.0.10.P28.23.r0 Report

## 1. 状态与范围

`P28_23_PASS`，仅指本阶段契约与完整验证通过，不是整体冻结或F验收。基线P28.22 / `fb14a536f9f0581eede1cb47d8ab7b4115ee4872`，2026-09-20。本轮闭合普通单件World拾取中的对象释放拒绝回滚；不接空间包多对象恢复或新增玩法。实际Red后首次修复的Editor/Game、三个专项、完整新旧根及实际改动覆盖均通过。

## 2. 入口与反例

WorldItem.RequestInteract直接进入ItemSubsystem.PickupWorldItem。非空间包路径先提交Runtime拾取、标记本Run来源、删除World绑定，最后调用Destroy却忽略返回值。销毁被拒绝时曾返回成功，原地面对象仍存续但不再有权威绑定，下一次拾取无法恢复。

既有PreparedWorldPickupIdentity测试使用真实瞬态World、Controller/Pawn、地板及SpiritDust对象；通过引擎既有role规则让原Actor拒绝销毁两次。测试没有新增生产故障端口或启动真实玩家流程。实际Red为0成功/1失败、队列1、原生0、崩溃指标0，七条最终Expected断言失败；原件见[Development Log](../Log/Dev.D.UE.0.0.10.P28.23.r0_log.md)。恢复role后的旧实现仍无法重拾原对象，因此Red在该处提前返回，后续合并变体不能冒充已独立取得Red。

## 3. 最小修复及验证意图

普通拾取在Destroy返回false时恢复既有Authority快照及原Actor绑定，返回InvalidWorldBinding。成功释放仍先移除绑定，避免EndPlay将已转入背包的身份重复退休。回滚后的错误结果使用已复制的DefinitionId，不再读取可能被RestoreState失效的旧实例指针。

Authority既有内存快照只含实例、背包、装备与临时仓库，遗漏AuthorityRevision；现在CaptureState/RestoreState同时记录和恢复修订号，使拒绝后数值与版本一致。不新增持久schema、公共操作接口、恢复记录或第二权威。该共用快照影响其他既有回滚调用方，因此完整新旧根用于核查，不能只用新拾取专项代替。

既有测试扩展61行：普通拾取两次拒绝后同对象重试；非零堆叠合并两次拒绝后恢复原目标数量、同对象成功合并且Actor只释放一次、已退休投影重放不重复合并；保留原携入身份与新获来源隔离及持久结算检查。测试注册总数不变。生产合计12行新增/3行删除。

## 4. 验证状态

21:36:41.6141988UTC锁定1617产品输入及103原用户文件；执行链依次为Editor构建、拾取专项1、WorldLifecycle专项7、ProductFlow专项5、Game构建、完整旧根1330、完整新根1430。首次失败和最终执行分目录保留，不覆盖、不拼接成功计数。

| 验证 | 当前事实 |
|---|---|
| Final Editor | 84 actions / 260.77秒，21:41:03.2600673UTC完成，原生0 |
| Final拾取专项 | 1成功/0失败，队列1、原生0、崩溃指标0；包含拒绝、恢复、合并和重放 |
| Final WorldLifecycle / ProductFlow | 7/7、5/5成功，队列7/5、原生0、崩溃指标0 |
| Final Game | 83 actions / 262.45秒，21:46:27.9937793UTC完成，原生0 |
| Final完整旧根 | 1330成功/0失败，队列1330、原生0、崩溃指标0，21:47:40.33851UTC完成 |
| Final完整新根 | 1430成功/0失败，队列1430、原生0、崩溃指标0，22:58:58.7763874UTC完成 |
| 改动驱动实际覆盖 | 7路径、1规则、3必跑组、2份本阶段健康完整日志，核验通过 |

两根共2760个独立成功用例，13项专项是完整根子集，不重复累计。原exec49915正常退出0，执行链末尾锁定输入核验通过，新根耗时约71分18秒；没有用运行中计数代替最终结果。11项原件SHA及过程检查点见Development Log；本轮没有使用P28.22产品日志冒充当前验证。

映射自检529/529通过、退出0；实际改动覆盖由本阶段两份健康完整日志通过检查器核验。四个源码路径命中既有ItemProductAdapters规则，要求demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、Shanmen.0_0_10.Items三组；七个阶段路径均已分类，不按主题裁剪或借用历史日志。

## 5. 边界与交接

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。没有正式地图/内容资产修改，没有物理输入、UI/玩法、Editor UI、PIE、Standalone、产品exe、Smoke/Cook/Package。只证明无头瞬态World中的底层端口，不替代玩家交互体验或空间包多对象原子释放。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)补充本阶段证据，FZ-1/2仍开放。精确交接ItemAuthority cpp/h、ItemSubsystem cpp、既有PreparationAdapter测试、索引、本Report/Log七路径。103原未跟踪用户文件及两份OverallReadiness修改保持不变；Saved原始日志只在本地，仓库Log提供路径和SHA，不宣称原件已上传。

后续仍先核对空间包多对象释放、其他物品/容器入口、强制EndPlay及全部权威路由的真实可达条件；不因本轮单对象回滚通过而宣布整体原子恢复、最终冻结或进入F。
