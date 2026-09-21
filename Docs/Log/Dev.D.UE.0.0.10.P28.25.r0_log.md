# Dev.D.UE.0.0.10.P28.25.r0 Development Log

状态：`P28_25_PASS`，阶段产品验证及改动覆盖门完成；实际发布基线由包含本文的Git提交追溯。不是底层整体冻结或F验收。

## 1. 基线与保护

2026-09-21T02:49:00.659Z heartbeat，分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `c4f2d59846980ccd6b7115ae7cb906670aa44c64`。完整读取最新P28.24 Report/Log、P阶段基线并核对Git。02:49:13.0613447UTC Published复核通过1617输入、103原用户集合/哈希、两份保护文档、11原件SHA、118相对链接、7健康执行、2760独立完整根、暂存0。没有重复启动P28.24。

保护：OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。原103文件沿用P28.24锁的精确路径与SHA，不编辑或暂存。

## 2. 可达路径与首次Red

读取Manager确认空间包丢弃路由、Runtime.DiscardSpatialItemBundle、SpawnBoundWorldActor、NotifyWorldActorEndPlay、RemoveWorldBinding、TeardownWorld及领域Discard/Recover实现。直接Runtime端口由既有Manager调用，不需要接入物理输入。UE5.8本机LevelActor.cpp的bIsTearingDown分支明确拒绝SpawnActor；生成通知可发生在已生成前缀之后。

扩展既有PreparedWorldPickupIdentity，不增加注册数。瞬态夹具实际生成两个对象，第二次生成通知中设置World已有teardown标记及第二个对象SimulatedProxy角色，第三次Spawn拒绝后恢复标记；首项真实销毁、第二项真实拒绝。测试并未执行完整World teardown，更不能冒称正式地图或玩家流程。初始库存布局、非零修订、携入三单位材料和持久Run均实测保持，原拒绝投影绑定丢失。

02:51:54.2515119UTC仅测试输入Red锁完成；exec82653执行Editor→单项Red，外层0表示预期Red执行结束而非测试通过。

Red Editor于02:52:32.6962994UTC完成，4动作/37.55秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Red/BuildEditor/20260921T025154789Z-7c693677/stdout.log`，SHA `42FA854A3A33639D23068D3AAFF3959BD5991D2D234AEA7BD42565D706A370E2`。

Red于02:52:53.5376899UTC完成，0成功/1失败、队列1、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Red/RedProof/20260921T025233161Z-49cb1118/UnrealEditor.log`，SHA `F51A1A1A33840BF0360E9AA0F21EFE78FA1731B4D7D552F73DF7F75A01F7EB22`。一条最终Expected失败为“Partial spatial spawn rollback retains its original refused projection”。此前通用Condition日志不重复当作独立断言。失败后夹具恢复角色、清理原对象并退出；后续拒绝重试、竞争门和健康回滚变体没有独立Red。

## 3. 首次修复

回滚先RestoreState(Before)及同步修饰，避免EndPlay退休已恢复物品；复用既有PendingSpatialRecoveries接受记录、原BundleId和已生成成员绑定，成功释放前缀不重做，拒绝成员保留原World归属。失败返回保留WorldActorSpawnFailed/InvariantViolation，不再谎称World释放全部完成。回滚仅清本次bundle，不全量清其他包记录。无新API、schema、数量权威、恢复字段或故障端口；私有注释明确记录也服务于失败丢弃回滚。

最终候选相对P28.24为生产cpp36新增/9删除、头文件注释1换1，既有测试78新增；弱绑定修正仅替换清理调用并添加两行原因注释。完整新根/旧根注册数仍1430/1330。普通回收、终局组合仍保留于同一测试中。

## 4. 完整验证与保留的失败

02:54:14.2152944UTC Final锁1617输入与103原用户文件，仅三源码不同于P28.24。原exec73453首次修复链Editor成功，随后拾取专项失败并停止，外层1。未运行后续专项、Game及完整根，不能因UE原生0而声称测试通过。

首修Editor于02:57:16.0138205UTC完成，48动作/180.89秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Final/BuildEditor/20260921T025414750Z-1cbfe44e/stdout.log`，SHA `37971ED4366EA60CB06DF7DD46BF96B6EEAB2D87CB21D682E730506CE8665294`。

首修拾取专项0成功/1失败、精确队列1、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Final/PickupFocused/20260921T025716448Z-0b071df0/UnrealEditor.log`，SHA `AF401E5EB48370163BC7CA156C0461F75361256883C97D09105E53094D28A27B`。最终Expected仍为“Partial spatial spawn rollback retains its original refused projection”，后续变体仍未执行。

核对RemoveWorldBinding可知：Destroy成功后弱指针Get可能已经为空，但原ExpectedActor非空，不匹配即提前返回。只将本次生成回滚中已经接受释放的实例改为按已拥有的InstanceId清理，与P28.24回收循环一致，拒绝项仍continue保留，未改变测试标准。

02:59:53.9386060UTC重新捕获Final锁，旧锁保存为Final-inputs-20260921T025414215Z.json。新链使用独立Recovery1目录、原exec90813，Editor→拾取专项1→WorldLifecycle专项7→ProductFlow专项5→Game→旧根1330→新根1430；各阶段间检查输入锁。七个原始执行均成功。12:30UTC恢复时exec会话已不可查，故以持久化run-state及原日志核对实际结果，不将会话缺失当作产品中断或新回归证据；没有重新启动测试。

Recovery1 Editor于03:00:02.6939764UTC完成，7.92秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Recovery1/BuildEditor/20260921T025954474Z-9ad19333/stdout.log`，SHA `C31FFFDC197407FA4E67D483EF9180D9075BE60C0FF8D6DF4D75F899719C5049`。

Recovery1拾取专项于03:00:23.4691453UTC完成，1成功/0失败、精确队列1、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Recovery1/PickupFocused/20260921T030003095Z-7130c828/UnrealEditor.log`，SHA `5AB5046C1E227FDC1CD34CC16E6F468AA64C1C7FC3C74E0D9C71EBD56C207599`。保持原失败断言，拒绝重试/恢复恰好一次/健康回滚和后续P28.24变体均沿既有测试正常完成。

Recovery1 WorldLifecycle专项于03:00:43.9311739UTC完成，7成功/0失败、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Recovery1/WorldLifecycleFocused/20260921T030023572Z-34ef9dd8/UnrealEditor.log`，SHA `31C030545DD1881D1D49AF673E75832FDB4A613C80713025BD65D0F10A39850F`。

Recovery1 ProductFlow专项于03:01:04.3632593UTC完成，5成功/0失败、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Recovery1/ProductFlowFocused/20260921T030044011Z-b7960485/UnrealEditor.log`，SHA `89561A2E6C163BDF6AD3E96D72A9414DD5E0C1F1FFB4F6C8CD8A22D5D344BC69`。

映射脚本未修改，本轮重新运行自检exec79445，正常0、537/537；原件 `Saved/Automation/P28.25/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。与上一阶段SHA一致是确定性输出，不表示复用其原件。

Recovery1 Game于03:04:15.3101705UTC完成，47动作/190.24秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Recovery1/BuildGame/20260921T030104784Z-71bac731/stdout.log`，SHA `CFF2CE2EA3D77E0C9957429F1D93612EDBD2739DD9BA0128418F4B3EA20C3688`。只构建，没有启动生成的产品程序。

Recovery1完整旧根于03:05:26.1129708UTC完成，1330成功/0失败、精确队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Recovery1/LegacyFullRoot/20260921T030415683Z-82720e87/UnrealEditor.log`，SHA `D902E8A98F62EBAC33313C013768D51EC33C92D938DF53E45C8CDF7D32233CF3`。

Recovery1完整新根于04:16:08.8687294UTC完成，1430成功/0失败、精确队列1430、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.25.r0.Recovery1/ShanmenFullRoot/20260921T030526388Z-31d97201/UnrealEditor.log`，SHA `901E14F0AAB0431B8DE4C3B706FF0ED05C3E50CA3A3FF4C1970C90CAB71CBEE2`。该日志独立完整结束，不拼接此前heartbeat观察到的中间计数。

12:31UTC运行本阶段六路径映射检查，实际为PASS Changed=6 Rules=1 Required=3 Logs=2；ItemProductAdapters映射的Shanmen.0_0_10.Items、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar均有本阶段健康完整根证据。原件 `Saved/Automation/P28.25/regression-coverage.log`，SHA `8D1BBDA6DEA12B4C45CDA3F26832CE5670D0FDF6FFE73334A771F147B79E5261`。13项专项为完整根子集，不与2760独立用例重复相加。

## 5. 精确交接与后续边界

12:30:46.0598136UTC入口复核1617输入、103原用户文件集合/哈希、两份保护文档、当时11份已登记日志SHA及四个文档链接通过，暂存0。新根及覆盖证据登记后共有13份原件SHA；有限索引仅补本次已验证范围，不将FZ-1/2改成关闭。

本阶段只交接三源码、有限索引及本Report/Log共六路径；发布前/暂存后/提交后均核对精确路径、锁定输入、用户文件、原件SHA、文档链接、七个健康执行及2760个独立完整根用例。原始日志不上传GitHub，不作“已在仓库”声明。不变量失败分支未独立注入，完整World shutdown、全部释放/终局组合与跨进程恢复仍不在本次通过断言内。P/F边界保持，不暂停尚未闭合的整体审计。

12:33:31.2834964UTC发布前校验通过：1617输入、103原未跟踪文件、两份保护文档、13份原件SHA、122个相对链接、七个健康执行及2760个独立完整根用例；暂存0，git diff --check通过。仅文档完成登记，产品输入未变。

- [Report](../Report/Dev.D.UE.0.0.10.P28.25.r0_report.md)
