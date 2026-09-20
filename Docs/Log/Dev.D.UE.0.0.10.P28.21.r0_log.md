# Dev.D.UE.0.0.10.P28.21.r0 Development Log

状态：`P28_21_PASS`。已复现普通容器目标重新初始化丢失拒绝owner，最小修复后的锁定双构建、专项、完整新旧根和实际覆盖通过。下文保留运行中检查点以追溯过程，最终结果见第5节；不是整体冻结或F验收。

## 1. 入场与保护

2026-09-20T16:59:52.964Z heartbeat。分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `1578b0058db90dc7d7c9dac570845807ec8eeada`。读取最新P28.20 Report/Log、Git状态及P/F共享基线；17:00:06.1555616UTC复核上一阶段1617输入、103原用户文件精确集合/哈希、两份保护文档、11项原件SHA、102个相对链接、暂存0通过，无UE-Cmd运行。没有重跑已发布P28.20。

保护哈希：OverallReadiness Report `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。103原未跟踪文件路径和哈希沿用上阶段锁，不编辑或暂存用户文件。

## 2. 可达性审计与边界

读取Manager/GameMode正常激活、直接清理、EndPlay及容器/空间包释放相关调用。只选择现有InitializeWorldContent→InitializeCodeBNormalContainerTarget的旧批次释放：两条登记不再全部有效时进入清理；旧循环忽略Destroy结果并Reset原创建数组和登记数组。正常World中的对象失效可使缓存检查失效，实际对象销毁拒绝则要求原owner保持；不能将World扫描回来的对象一概视为无运行时owner。

其余容器物化失败回滚、空间包部分释放、非M01再激活及强制EndPlay仍需单独核对；本轮不凭静态可疑代码扩充通用恢复系统。M01普通容器初始化失败只记录投影错误，代码明确不允许它回滚Code A Run，此界限保持。

## 3. 实际Red

只扩展已有ManagerDeactivationRetention并加入CodeBNormalContainerActor头文件，新增64行，注册数与友元不变。同一瞬态World持有两个不同静态目标ID和原Manager引用；实际销毁第二个后，第一个设ROLE_SimulatedProxy拒绝两次。验证原创建owner、登记、原Run与非零durable快照；恢复role后应只释放一次，再用缺少anchor的错误确认没有新物化，最后验证两个World提供对象的健康查找/重放且不进入Manager创建数组。

测试仅允许精确的missing static M01 anchor日志，Occurrences=0按本机UE源码表示至少一次而非忽略所有错误；恢复阶段一定走该拒绝。预期日志不屏蔽任何断言。最初编辑块定位到邻接测试，编译前即复核并移回目标方法；无源码构建失败被隐藏。

17:04:26.3032769UTC Red锁1617输入/103用户文件，仅允许既有测试文件变化。Red Editor于17:04:48.0068716UTC完成，4 actions/20.47秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.21.r0.RedBuild/BuildEditor/20260920T170427145Z-b1240ae6/stdout.log`，SHA `DEF8710C80F09F570CCFD3DE61AD159778F903A595868EE82BBCDE7B99AB0E5F`。

RedProof于17:05:28.9521679UTC完成，0成功/1失败、精确队列1、原生0、崩溃指标0；最终3条Expected为两次原owner/登记保持、一次原批次恰好一次释放。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.21.r0/RedProof/20260920T170448520Z-53873af3/UnrealEditor.log`，SHA `ABE4266C0BA3EA88D627D3FB13F23DE0B196DDDC2A1EFC679A68D04217B8C426`。exec36685退出0说明预期反例执行完毕，不将该测试说成通过。实际Red初始化仍因缺anchor返回false，失败是owner丢失，不声称曾错误返回初始化成功。

## 4. 最小修复与最终验证

只改原初始化旧批次清理：快照遍历、保留拒绝弱引用，任一拒绝返回false；全部完成后才清登记并进入现存World目标发现。没有新公共API/schema/恢复记录/故障端口、没有新注册测试或地图/内容变更。

17:05:57.9669887UTC Final锁1617输入/103用户文件于Saved/Automation/P28.21/validation-inputs.json，只有测试和Manager cpp较P28.20产品输入变化。原exec3299按Editor→WorldLifecycle7→ProductFlow5→Game→Legacy1330→Shanmen1430串行执行，各段前后核验输入。独立映射自检exec54165；最终实际覆盖仅使用本阶段健康完整根，不用静态映射代替日志。

最终Editor于17:06:18.6630826UTC完成，4 actions/19.61秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.21.r0/BuildEditor/20260920T170558743Z-80fb857a/stdout.log`，SHA `8C607C1C1FF14418A497814FB4CFE72DB3467AE84D19BE5542BE26BDA9F659B1`。

WorldLifecycle专项于17:06:39.7833209UTC完成，7成功/0失败、队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.21.r0/WorldLifecycleFocused/20260920T170619184Z-45393469/UnrealEditor.log`，SHA `C0AB189E7B163E913C1D83602D2B85E50E8896C7E0010FC27BCD8957A3A60944`。

ProductFlow专项于17:07:00.2588823UTC完成，5成功/0失败、队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.21.r0/ProductFlowFocused/20260920T170639869Z-d42bab11/UnrealEditor.log`，SHA `24077D264F0844BB034377145C0A78F92610C1100E4DC0774BA1AF786FA2A7E1`。

Game于17:07:41.3040137UTC完成，4 actions/40.33秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.21.r0/BuildGame/20260920T170700712Z-f5a2b32a/stdout.log`，SHA `9573C0C4CAD1B32C5BF5F1266A4B217E4C7D1A8EB2385370752E8242D66AB2E0`。

映射自检exec54165原生退出0，529/529；原件 `Saved/Automation/P28.21/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。输出与前阶段内容相同不表示复用，确实执行本阶段自检；它不是实际改动覆盖报告。

Legacy完整根于17:08:57.1552826UTC完成，1330成功/0失败、队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.21.r0/LegacyFullRoot/20260920T170741672Z-597ccecb/UnrealEditor.log`，SHA `A01D01B2C80FF783C833580340231E7BCB75CF531EF7CC55F22F8D2F49B53E39`。

17:13:41.9170636UTC检查新根原目录Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.21.r0/ShanmenFullRoot/20260920T170857451Z-2d0b6f71，UE-Cmd42668仍在运行，863成功/0失败；没有队列结束、run-state或原生退出，不计算运行中原件的最终SHA，不冒充1430通过。原exec3299保持运行，下一次heartbeat承接同一进程；不重新构建或重复启动。17:09:22.7166929UTC输入/用户保护复核通过，暂存0、原用户103加本轮Report/Log共105未跟踪文件。

## 5. 最终结果与精确交接

2026-09-20T17:48:23.732Z heartbeat继续同一验证，不另开阶段。17:48:41.0714194UTC新根已1093成功/0失败；17:49:20.0091402UTC复核原UE-Cmd42668仍运行、日志继续更新、崩溃指标0，没有最终run-state。1617输入、103原用户精确集合/哈希、两份保护文档和8项已完成原件SHA再次通过；暂存0、未跟踪105、有限索引未变、diff check通过。等待原进程完成，不重启、不更改锁定源码、不提前提交。

17:15:39.2035642UTC交接检查：HEAD未变，1617锁定产品输入、103原用户文件精确路径/哈希、两份保护文档、8项已完成原件SHA及4个文档链接通过；未跟踪105、暂存0，有限索引仍为P28.20，diff check通过。此检查不替代仍在运行的新根及其后实际覆盖检查。

2026-09-20T18:50:53.594Z heartbeat读取分支/状态、本阶段Report/Log、续接记录及P/F基线，承接原exec3299，不重复启动测试。新根于18:18:48.6605929UTC完成，1430成功/0失败、队列1430、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.21.r0/ShanmenFullRoot/20260920T170857451Z-2d0b6f71/UnrealEditor.log`，SHA `B423B1A5DD12471599C170AFB60F3C93AB5D029E3D1F0B380CFBC432DBEE7610`。外层exec3299也退出0，末尾输入核验通过；最终两根独立路径合计2760，专项7/5不重复计入。新根耗时约69分51秒，日志有HTTP连通性检查超时警告，但测试、队列与原生退出均正常；没有通过屏蔽错误或重启改变结果。

最终实际覆盖使用本阶段两份完整根日志，5路径/2规则/7必跑组/2日志通过；原件 `Saved/Automation/P28.21/regression-coverage.log`，SHA `708D1AFAB557503395ACB39F4094B378478D16C8B308ED35B384BB9A6FA5C06D`。必跑组为demo_map.CodeB、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、demo_map.Profile、demo_map.V2RangedCompatibility、Shanmen.0_0_10、Shanmen.0_0_10.Items。两份完整根及本轮529项映射自检均为实际运行，未借用上阶段产品日志。

最终精确范围：Source/demo_map/demo_mapV3ProgressionManager.cpp（10新增/4删除）、Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp（64新增）、有限冻结索引、本Report及本Log，共五路径。18:54:09.5586214UTC发布前独立检查通过：1617产品输入、103原用户精确集合/哈希、两份保护文档、10项原件SHA、106个相对链接、6项最终健康执行、2760个完整根独立用例、实际覆盖及diff check；暂存0，105未跟踪中仅两份为本轮新文档。仅精确暂存上述五路径，提交并推送当前分支，不强推。提交标识由Git历史记录，不在提交自身内伪造自引用哈希。

索引补充P28.21局部证据但FZ-1/2保持开放；其他容器生成失败/物品释放/空间包/强制EndPlay及全入口路由审计不能因此视为关闭。无Editor UI、PIE、Standalone、正式地图或玩法/UI开发，不暂停自动化或进入F。Saved原始日志保留本地，仓库Log仅给出可核验路径和哈希。

- [Report](../Report/Dev.D.UE.0.0.10.P28.21.r0_report.md)
