# Dev.D.UE.0.0.10.P28.24.r0 Report

## 1. 状态与范围

`P28_24_PASS`，本阶段底层契约验证完成，不是整体冻结或F验收。基线P28.23 / `81464d6780b833e7333c72a35a115464c542e1d2`，2026-09-21 UTC。本轮闭合既有空间包回收中逻辑成功与多对象World释放的分离；不添加玩法或新恢复平台。

## 2. 实际反例

WorldItem.RequestInteract→PickupWorldItem→RecoverSpatialItemBundle实际端口：整包逻辑归属先回到背包/装备，随后逐对象Destroy。但旧实现先删除空间包/World绑定并忽略Destroy返回值，即使三对象中的两个释放成功、一个真实拒绝，也报告整体成功并丢失原拒绝对象。后续重试不能恢复，新的丢弃可能继续重绑定同一组身份。

既有PreparedWorldPickupIdentity在真实瞬态World中扩展：先整备原持久背包、保持三单位携入材料，再形成背包加两个溢出物品的三对象包。引擎既有role规则让一个对象拒绝两次。Red为0成功/1失败、队列1、原生0、崩溃指标0，八条最终Expected失败；保留原始日志。新增终局变体是在Red之后追加，不能冒称它有独立Red。

## 3. 修复边界

逻辑回收成功后，仅在原Runtime内保存该BundleId的已接受操作回执。逐个释放World对象，拒绝项保留原绑定、包身份和可调用的续清理端口；成功前缀不重做，重试不再回收、装备或改变权威修订号。全部释放才返回原接受结果并清除待释放记录。失败诊断明确“逻辑物品已回收，投影仍待释放”，不声称多次Actor销毁是可回滚原子事务。

新的整包丢弃在旧回收投影未释放时失败关闭，避免同一身份被重新登记到另一包。正常回调和Teardown沿原World绑定释放；完整回收后的物品可在唯一权威中结算，待释放投影不再次退休已入包/已保全身份。

新增一个有界、同实例、非持久的待释放回执表和只读查询，不新增物品数量真值、持久schema或生产故障注入。CanInteract只接通既有待清理端口的资格查询，未接物理输入、改变提示文案或制作UI。它不证明跨进程World恢复、空间包生成失败回滚或所有终局组合。

## 4. 完成验证

00:18:24.2967041UTC锁定1617产品/验证输入及103原用户文件，原exec21699依次执行Editor→拾取专项1→WorldLifecycle专项7→ProductFlow专项5→Game→完整旧根→完整新根，全链完成、外层0，无重复启动。验证期间产品/脚本输入未改变。

| 验证 | 实际结果 |
|---|---|
| Editor构建 | 48动作、185.44秒、原生0；00:21:30.6728061UTC完成 |
| Game构建 | 47动作、188.66秒、原生0；00:25:41.8144517UTC完成 |
| 拾取 / WorldLifecycle / ProductFlow专项 | 分别1/1、7/7、5/5成功；精确队列、原生0、崩溃指标0 |
| 完整旧根demo_map | 1330成功、0失败；队列1330、原生0、崩溃指标0；00:27:07.6885627UTC完成 |
| 完整新根Shanmen.0_0_10 | 1430成功、0失败；队列1430、原生0、崩溃指标0；01:38:04.9990257UTC完成 |

两个完整根共2760个独立成功用例；13项专项是子集，不重复计入。新根耗时约71分钟，中间HTTP探测超时警告没有导致用例失败，不将过程中的无退出码状态误记为通过。原始结果、队列、进程状态及SHA记录在Development Log。

映射自检537/537、原生0（原529项加8项新规则正反检查）。新WorldItemProjection映射覆盖WorldItem cpp/h，要求旧WorldInteraction、M01Extraction及新Items；与ItemProductAdapters合并，实际检查通过 `Changed=9 Rules=2 Required=5 Logs=2`，使用本轮两份完整健康日志，不借P28.23证据代替。五必跑组为ItemUseAndArmor、P4.Hotbar、V3.WorldInteraction、M01Extraction和Shanmen.0_0_10.Items。

## 5. 保护、交接及未完成项

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。两份OverallReadiness用户修改和103原未跟踪文件保持不变，不纳入本阶段提交。交接范围仅四源码、映射及其自检、索引、本Report/Log共九路径；承载这九路径的Git提交提供阶段版本追溯，远端推送结果另行核验。Saved原始日志仅在本地，[Development Log](../Log/Dev.D.UE.0.0.10.P28.24.r0_log.md)记录11项原件路径和SHA，包括首次Red。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)补入本阶段有界证据。FZ-1/2仍开放，空间包生成回滚、其余物品/容器入口和强制EndPlay须继续按真实可达条件核对；本轮不证明全部入口、所有部分释放组合或跨进程World恢复。没有进入玩法、正式地图/资产、UI或F验收，没有启动Editor UI/PIE/Standalone/产品exe，也不做Smoke/Cook/Package。未达到整体最终冻结条件，不暂停底层闭合监控。
