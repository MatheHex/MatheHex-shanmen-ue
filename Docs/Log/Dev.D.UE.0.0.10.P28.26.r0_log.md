# Dev.D.UE.0.0.10.P28.26.r0 Development Log

状态：`P28_26_PASS`。完整验证链、改动覆盖及证据核验完成；发布基线由包含本文的Git提交追溯。不是整体冻结或F验收。

## 1. 基线与保护

2026-09-21T13:08:15.017Z heartbeat；分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition；HEAD `064e087155d16a2036becc04cf167ce6930305b7`。入口完整读取P28.25 Report/Log、P阶段基线及Git状态，13:08:27.7547433UTC Published核验1617输入、103原用户文件、两份保护文档、13原件SHA、122链接、7健康执行、2760独立完整根、暂存0通过。未重复执行上一阶段，也没有运行中的UE-Cmd。

保护文档OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`、Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。原103文件精确路径/SHA沿用P28.25输入锁；不编辑、不暂存。

## 2. 可达性与测试先行

读取Manager.RequestDropPlayerItem及其普通DropPlayerItemToWorld路由、UI现有逻辑调用方、空间包回收/投影生成/RemoveWorldBinding/ValidateWorldBindings。仅阅读既有输入调用关系，不接入物理输入。已释放成员没有剩余World绑定，领域仍归库存，正常带修订DropIntent可再次丢弃；旧空间包的一个原成员拒绝清理不等于所有历史成员永久归它管理。

只扩展既有Shanmen.0_0_10.Items.RunLifecycle.PreparedWorldPickupIdentity，使用实际瞬态World、原三对象包、已有引擎SimulatedProxy销毁拒绝条件、原成员身份及非零权威修订。普通独立丢弃使用实际DropPlayerItemToWorld端口，无新生产故障端口或注册测试。

13:10:47.5581647UTC首次Red锁1617输入/103原用户文件，仅测试源码变化。exec15255的Editor构建于13:11:03.1355378UTC以原生6结束，外层1；新增测试引用Fdemo_mapEntityLoadoutRules但未包含其头文件，C2653/C2065。未进入Red测试。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Red/BuildEditor/20260921T131048098Z-43539c64/stdout.log`，SHA `224FF2B7409EB2BF33FC265C6988E2ABAFC2F64329DF5B889F0C50382FDA5F80`。

搜索真实定义后加入demo_mapEntityLoadoutPresenter.h，不改变断言。原锁留存，13:13:48.5237839UTC重新捕获仅测试Red锁，独立RedRecovery1任务exec5859执行。Editor于13:14:00.4689559UTC正常完成，原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.RedRecovery1/BuildEditor/20260921T131349091Z-88b7b2a5/stdout.log`，SHA `8E4DC4BFF931DAAB445C5BD44564A0F78FF66A04998F5B218D01D64956C48408`。

有效Red于13:14:51.4740934UTC正常退出：0成功/1失败、精确队列1、原生0、崩溃指标0，外层0仅表示预期Red执行完成。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.RedRecovery1/RedProof/20260921T131400973Z-be9ca25d/UnrealEditor.log`，SHA `66CEE64919AB20D0D7E038F5EBC6F84B7C117764A8903D9CE90BDDCF9AF00AFC`。

最终一条Expected失败为“Released bundle member can acquire an independent new World projection”；此前通用Condition日志不重复计数。失败后恢复原role、清理并退出，所以后续新旧投影隔离、完成收尾和普通拾取没有独立Red。源码定位旧ValidateWorldBindings将原成员列表等同于当前待清理归属；旧释放循环和最后成员收尾也有同类过宽遍历，不能只放宽不变量检查。

## 3. 最小修复及锁定输入

旧包范围/释放/不变量遍历均筛选FindSpatialBundleId等于原BundleId；RemoveWorldBinding的剩余成员与映射收尾同样只看精确当前归属。正常World对象继续参与权威WorldIds及唯一绑定验证。未新增API、schema、数量权威、恢复字段、故障注入端口或测试注册。生产cpp13新增/3删除；测试77新增（含头文件），未修改生产头文件。

测试保持首次失败断言，新增后续两次拒绝、原包恰好一次完成、旧回执失效、新对象不被旧回执接管/释放、独立普通拾取一次；检查新对象World数量1、同一绑定、权威修订、原携入材料3、原Run与持久快照。既有部分生成回滚、健康回滚及持久终局变体继续执行。范围门没有独立远距离反例，不能扩大实测覆盖声明。

13:15:32.7757702UTC Final锁完成：1617产品/验证输入与103原用户文件，仅ItemSubsystem.cpp和PreparationAdapterTests.cpp相对上阶段变化。exec29243运行独立Final链：Editor→拾取专项1→WorldLifecycle专项7→ProductFlow专项5→Game→旧根1330→新根1430；各阶段间检查锁。该链最终正常0结束，不用历史日志替代、不重复启动UE-Cmd。

## 4. 本阶段已完成证据

Final Editor于13:15:42.9237995UTC完成，9.32秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/BuildEditor/20260921T131533284Z-ca52fbed/stdout.log`，SHA `8BE243696CC582B4487B38440695503F7BF16B02769949D2E4CE6BED7B05A6EF`。

Final拾取专项于13:16:03.7677738UTC完成，1成功/0失败、精确队列1、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/PickupFocused/20260921T131543404Z-bf3548da/UnrealEditor.log`，SHA `A990AD9E9DBB0572A4BA7593723C8819096B68480D434009F2E4EF2397643F8E`。

Final WorldLifecycle专项于13:16:24.1610910UTC完成，7成功/0失败、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/WorldLifecycleFocused/20260921T131603813Z-986849c7/UnrealEditor.log`，SHA `A9386F9F24D747CD89B14896D782DED632344731B1407C11BB4C103B322E2797`。

Final ProductFlow专项于13:16:44.5713515UTC完成，5成功/0失败、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/ProductFlowFocused/20260921T131624241Z-23171dce/UnrealEditor.log`，SHA `348C546E75F9DD0AA0C6FB8D0C8884F78FA2B63CC1913783D165D1390EF6021F`。

本阶段exec6922实际重跑回归映射自检，外层0、537/537；原件 `Saved/Automation/P28.26/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。相同SHA来自确定性输出，不是复用上一阶段文件。

Final Game于13:17:14.8364728UTC完成，4动作/29.65秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/BuildGame/20260921T131644961Z-e015dc29/stdout.log`，SHA `EACAD293F2439332CCF127F31286BB3D4844ABFB1B9EBE36632159585D7138A8`。没有启动生成的产品可执行文件。

Final完整旧根于13:18:30.6573583UTC完成，1330成功/0失败、精确队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/LegacyFullRoot/20260921T131715213Z-f5b5d741/UnrealEditor.log`，SHA `306CE55AFDABD29E890EB351F093EA5CE46BC4048D72C7429335C1FB590A140F`。

Final新根于13:18:30UTC由同一链启动，PID32632。13:19/13:54/14:27UTC依次观测410/912/1170成功且均无结束队列；这些仅作为续接状态，未声称通过、未重复启动或拼接计数。

Final完整新根于14:48:50.0391969UTC正常结束，1430成功/0失败、精确队列1430、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/ShanmenFullRoot/20260921T131830947Z-45d20b29/UnrealEditor.log`，SHA `8A0640A07E8785FE2C7F3A287B97A45E396718E2350742C5B89820C7EC68A19B`。15:21UTC heartbeat核对原始状态与完整日志，exec29243返回外层0及输入锁通过，当前没有UE-Cmd遗留进程。未因其耗时长于上一轮而重跑或提前终止。

本轮五路径覆盖门实际PASS Changed=5 Rules=1 Required=3 Logs=2；三个必跑组Shanmen.0_0_10.Items、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar均由本阶段两套完整根覆盖。原件 `Saved/Automation/P28.26/regression-coverage.log`，SHA `AD27EF380C0D1EBD27EC423E02E3E7A8903746D3A9D8865B673B4859BBF3AA11`。两套完整根共2760个独立成功用例，专项13项不再相加。

## 5. 精确交接与后续边界

七个最终执行均健康，完整原件已登记，不重跑。有限索引仅补充本阶段边界，FZ-1/2不改为关闭；精确交接两源码、索引、Report与Log五路径。发布前/暂存后/提交后检查原件SHA、产品输入、用户文件、文档链接、路径集合、七个健康执行和2760个唯一完整根用例。

13:18:50.1101705UTC检查点通过1617输入、103原未跟踪用户文件、两份保护文档、当时8原件SHA及4相对链接，暂存0、差异空白检查通过。随后追加Game/旧根两份原件共10项，产品输入未改。

15:21:42.7183821UTC入口检查点再次通过1617输入、103原用户文件、两份保护文档、当时10原件SHA与4相对链接、暂存0。新根和覆盖原件登记后共12项，产品输入与用户文件未变；只完成文档与有限索引，不修改通过验证的源码。

15:24:35.9604422UTC发布前核验通过1617锁定输入、103原用户文件、两份保护文档、12份原件SHA、126个相对链接、七个健康执行及2760个独立完整根用例；暂存0、git diff --check通过。本阶段只交接五路径，不纳入原始本地日志或用户文件。

原件均仅保留本地，当前不声称日志原件已上传GitHub。本次是同Runtime旧空间包续清理与独立新投影隔离，不替代所有生成/释放、强制EndPlay、重入或跨进程恢复。FZ-1/2仍开放；不暂停自动化、不越过P/F边界。

- [Report](../Report/Dev.D.UE.0.0.10.P28.26.r0_report.md)
