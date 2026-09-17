# Dev.D.UE.0.0.10.P28.4.r0 Development Log

## 1. 当前状态与基线

`P28_4_VERIFIED`。2026-09-17 02:54 heartbeat 开始，后续 heartbeat 续接同一阶段；没有并发重复启动仍运行的验证。14:16 确认首次完整新根中断后仅补跑该根，14:52、15:28 仅检查仍运行的实例；16:12 复核其完整结果和阶段交付条件。

HEAD `8cab3d3611894b0d8b7fcb94e82df7ea20767f2e`，分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。入场跟踪/暂存区干净，103 个原用户文件未跟踪；完整读取 P 阶段基线、冻结索引、P28.3 Report/Log 及总体报告最新增补。总体文档交付保持，不混入本阶段。

`Saved/Automation/P28.4/baseline.json` 保存入场 1617 个输入和 103 个用户文件哈希；`validation-inputs.json` 在 Fixed Editor/专项期间锁定相同路径的当前字节，之后不改产品输入。忽略目录保存运行器与续接说明。

## 2. 调用链与证明边界

读取实际调用：RunStartCoordinator.ReturnToSectAfterTechnicalFailure → M01RuntimeAdapter.CancelAttempt → GameMode.Rollback0909BPreparedRun → Manager.RollbackPreparedProfileRunFor0909B。上层已在 false 时保留 attempt/correlation；Manager 自己却先清理再检查结果。

Flow 在物品服务 RecoveryRequired 时拒绝技术回滚，保留 Runtime 和原 Run；Manager.DeactivateProfileWorld 则清空拾取物/容器、调用 GameMode，随后 Items.TeardownWorld 删除 World 实例和绑定。新增测试复用真实 Flow/cutover 和隔离磁盘故障，而不是手动把 Flow phase 改成恢复状态。

测试友元仅绑定 Manager 现有拥有关系与非零标志，瞬态 World 提供真实物品和物理地面。没有正式 M01/BeginPlay，因此不声称验证全部激活链。健康变体与故障变体在同一用例内分别使用不同 GUID 隔离根；故障变体执行两次相同回滚调用。

RedProof 明确只有四条保持断言失败：Manager world/pending 两次，实际 World owner/binding/quantity 两次。原 Run/三颗丹/生命1和旧 Profile 字节保持断言未失败。修复仅把两条清理语句移动到 bAtSectReady 成功分支，沿用原 bool 和诊断，无新恢复层。

## 3. 原始证据（路径相对项目根，Saved 原件仅本地）

| 证据 | 路径 | 结果 / SHA-256 |
|---|---|---|
| 中断 Red Editor stdout | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0.RedProof/BuildEditor/20260917T025803437Z-50bc70d4/stdout.log` | 仅部分输出；SHA `8468814D309BEDF1A10CAAA0C8B7C68E174F26B91AF321670C56EDCDD4742E00` |
| Red Editor retry stdout | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0.RedProofRetry/BuildEditor/20260917T033827747Z-e78a8d51/stdout.log` | SUCCEEDED/native0；26 actions/239.45s；SHA `BCE8EF098A8F83B9B9BD3A4D6D4084324D424E840E264FB757F021C544998807` |
| RedProof | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0/RedProof/20260917T034234950Z-2bd96772/UnrealEditor.log` | 0/1，队列1/native0；SHA `4CEA69EC4C0E5740E86C42E1D3316408CAF7AF498D7C1F427A71F3D235C54F6C` |
| Fixed Editor stdout | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0.Fixed/BuildEditor/20260917T034402878Z-1e383a84/stdout.log` | SUCCEEDED/native0；4 actions/36.39s；SHA `0025E1CBE5049795E33C38BBC365651D0225B8CFBB0CB49CE2347BEFDFF11CA0` |
| ProductFlowFocused | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0/ProductFlowFocused/20260917T034446182Z-4c720c91/UnrealEditor.log` | 5/0，队列5/native0；SHA `503776A04211AA40704B89E2985D55BCC459C872FE741D818425C831BCE904DE` |
| Game stdout | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0.Validation/BuildGame/20260917T034556978Z-f30ee6ff/stdout.log` | SUCCEEDED/native0；25 actions/201.88s；SHA `CA0E7FF3BAA548483EC9C7A4E1F32829EB1DCCAF78ED19CF68DA3F3625D61D6C` |
| 映射检查器自检 | `Saved/Automation/P28.4/regression-selftest.log` | PASS 517/517；SHA `96B77D69044EEA0655A097D38D0629A20A894FF21CE4352562003F17ABCE6A5F` |
| LegacyFullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0/LegacyFullRoot/20260917T043328104Z-bba415b4/UnrealEditor.log` | 1330/0，队列1330/native0，崩溃指标0；SHA `4E70B98DD764DD3D40125C7A2CDACA7D31A019D6DCC0DE11A475682174742FDC` |
| 首次完整新根中断原件 | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0/ShanmenFullRoot/20260917T043524316Z-b81f231b/UnrealEditor.log` | 仅部分1173/0，无最终队列或退出码；SHA `4B3144720CB229F58CCB3A13760AD9948D1EB95FDA13DCACDED72700CA4BEA91` |
| 完整新根独立补跑 | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0/ShanmenFullRootResumed/20260917T141840350Z-424f019e/UnrealEditor.log` | 1427/0，队列1427/native0，崩溃指标0；SHA `D85BDC284BFB052D1FE772E288EE247446672E12B5D9C54A41A375B3DE7E6382` |
| 最终六路径映射 | `Saved/Automation/P28.4/regression-coverage.log` | PASS Changed=6 / Rules=3 / Required=6 / Logs=2；SHA `D7A9BD901D937CA99FD753379F00FBC847CDBC42243F3AB393A4544A45ACDA0E` |

首次外层启动退出 1073807364，stdout 止于 UHT 附近，stderr 空、无 run-state；本轮没有足够证据确定中断原因或原生构建退出码。确认无 UE/UBT 进程后，以显式 bundled pwsh -NoProfile 调用同一 Invoke-Shanmen.ps1 重试成功。未修改系统或构建入口。

RedProof、专项和两份完整根的 Fatal error/Ensure condition failed/Unhandled Exception 匹配均为0；不宣称所有启动 Error/Warning 为0。5项专项中包含新增用例，已经包含在完整新根中，不重复累计。

## 4. 验证续接与剩余项

Game 构建在 03:49:19 UTC 已完成，04:32 续接时没有 UE/UBT 残留；核对 1617 个输入及 103 个原用户文件哈希无变化后，以既有 validate.ps1 -Scope Roots 启动完整旧根，再串行运行完整新根。exec session 59641，runner PID 25224；旧根证据目录 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0/LegacyFullRoot/20260917T043328104Z-bba415b4`，初始 UE PID 37036。PID 只作续接定位，完成必须看原始日志与 run-state，不凭进程消失判成功。

源码三路径的映射并集为：Shanmen.0_0_10、Shanmen.0_0_10.Items.ProductFlow、demo_map.Profile、demo_map.ItemEconomySchema、demo_map.CodeB、demo_map.V2RangedCompatibility。完成前 1427 只作为完整性预期；最终以本阶段完整旧根和独立补跑新根的实测结果满足此范围，不以历史全绿日志替代改动验证。

04:35 同一运行器确认旧根 1330/0 后启动新根，UE PID 44172，证据目录 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0/ShanmenFullRoot/20260917T043524316Z-b81f231b`。05:08 与 05:42 分别读到部分902/0、1164/0且同一进程仍在推进，没有把中间结果判为通过。05:42 对活动日志计算哈希被共享锁拒绝，这不是测试失败；完成或关闭后才保存最终原件哈希。

14:16 恢复时该 UE 及运行器已不在，旧 exec session59641 返回 Unknown process id。原日志末尾05:46:55 UTC，部分1173/0、无完整队列、无 Fatal/Ensure/Unhandled Exception 特征，也无 run-state；stderr为空，stdout只有平台探测输出，没有原生最终码。系统最近启动时间仍为09-16，本轮不能把中断归因于重启、内存不足、源码崩溃或具体外层退出码。保留原件，不拼接其部分结果为完整通过。

再次核对1617个输入、103个用户文件以及两份总体 Report/Log 哈希无变化，确认无UE残留后，只在忽略目录的既有运行器增加 NewRoot 范围（原组名与预计数不变），于14:18:40 UTC 单独启动完整 Shanmen 新根。exec session1726、runner PID37972、UE PID12396；证据目录 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.4.r0/ShanmenFullRootResumed/20260917T141840350Z-424f019e`。不重跑已完成的旧根/构建，不修改产品、测试或正式 Scripts；当时尚无最终结果，因此没有提前提交。

14:52 与15:28 分别记录部分943/0、1176/0，同一实例持续推进，没有重复启动。最终 run-state 记录开始 `2026-09-17T14:18:40.3957267Z`、结束 `2026-09-17T15:39:59.2078236Z`，SUCCEEDED/native0，约81分19秒。完整原日志有1427个Success、0个Fail和精确1427队列结束；这是独立完整运行，不把中断日志或5项专项叠加进计数。

04:32 续接还完整读取最新 P28.3 Report、P/F 基线、冻结索引、P28.4 Report/Log 及 OverallReadiness Log 第21节。用户总体报告任务留下的两份未提交修改不纳入本阶段：OverallReadiness Report 原字节 SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log 为 `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。源码不变、index 仍为空；保持 105 个未跟踪路径中的 103 个用户文件及本阶段两草稿的身份区分。

## 5. 最终交付核验（2026-09-17 16:12 heartbeat）

- 重新读取分支、HEAD、工作区、P/F 基线、阶段 Report/Log、有限冻结索引及源代码差异。UE/运行器已完成，没有启动新验证或修改锁定源码。
- 逐项核对 RedProof、专项、完整新旧根的原日志/精确队列/native/crash，以及 Red Editor retry、Fixed Editor、Game 的 run-state/stdout；全部 SHA 与第3节相符。首次构建中断和首次新根中断原件同样保留，没有改写成成功。
- 六路径映射实际 PASS Changed=6 / Rules=3 / Required=6 / Logs=2；只使用最终完整的旧根和 Resumed 新根。自检517/517复用本阶段原件，检查器/映射输入未变，不冒称交付时又重跑一次自检。
- 1617个锁定产品/脚本输入和103个原用户文件原字节均不变；105个未跟踪路径中的另外两项为本阶段 Report/Log 草稿。两份 OverallReadiness 原字节保持第4节哈希，不纳入提交。
- 三份阶段 Markdown 的51个相对链接目标全部存在，`git diff --check`通过；103个原用户文件的路径集合也一致。未修改检查器或映射来缩小回归范围。
- 冻结索引只更新 P28.4 局部保持和最近验证入口。FZ-1/2/3 未整体关闭；不暂停、不新增玩法，不改总体 Report/Log。
- 精确交付六文件：`demo_mapProfilePreparationFlowTests.cpp`、`demo_mapV3ProgressionManager.cpp`、`demo_mapV3ProgressionManager.h`、冻结索引、本 Report、本 Log。普通提交/推送，不强推，不上传 Saved 原件。

交付源码原字节 SHA-256 与构建和完整根使用的锁定输入一致（Git 按仓库配置规范化换行）：

| 文件 | SHA-256 |
|---|---|
| `Source/demo_map/demo_mapProfilePreparationFlowTests.cpp` | `007F24451A1334256FAD50C0BC619FF10EF218BD2509C494BE9F1D49BDD87F04` |
| `Source/demo_map/demo_mapV3ProgressionManager.cpp` | `34B0A88C3844A77DFDA32A24CFC907C6C7EF506EECAF0C1DAF4E50F2A4298356` |
| `Source/demo_map/demo_mapV3ProgressionManager.h` | `2D586884DFC644E07D4A66C4781AB1B40777C438E2326123DBACFC9F91A41A37` |

下一轮沿既有 FZ-2 核对 Manager 成功回滚后的 World release 拒绝传播、激活失败与终局/EndPlay 调用条件，不把本次下层回滚拒绝保持当作全部 World 清理已经原子化。

- [Report](../Report/Dev.D.UE.0.0.10.P28.4.r0_report.md)
