# Dev.D.UE.0.0.10.P28.5.r0 Development Log

## 1. 入场

`P28_5_VERIFIED`。2026-09-17 16:50 heartbeat开始，17:49/18:22续查同一验证，19:17复核完成结果与交付。入场HEAD `ab703477136baccf9b972bbe31e96dfe3002ecf8`，分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。完整读取 P 阶段共享基线、P28.4 Report/Log 和冻结索引；入场无仍运行的 UE/构建。入场索引为空，只有用户总体报告任务的两份 OverallReadiness 跟踪修改，103个既有未跟踪用户文件。

确认 P28.4 锁定的1617个输入和103个用户文件哈希全部一致，复制该生成式清单为 `Saved/Automation/P28.5/baseline.json` 并更新 HEAD；清单 utc 为原始采集时间，非本阶段开始时间。当前阶段基线时间以上述 heartbeat 为准。

保护的 OverallReadiness Report SHA：`3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`；Log SHA：`A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。不修改或提交它们。

## 2. 审计与新增测试

读取 Manager 技术回滚、旧 ActivatePreparedProfileWorld、DeactivateProfileWorld、终局和 EndPlay；读取 GameMode 去激活、ReleaseCombatProductRun/TryFinishCombatRunRetirement 及 Flow.CancelActiveRunForActivationFailure。

确认两个分开的边界：P28.4 已解决 Flow 拒绝时仍清理的问题；但 Manager 直接清理路径仍在 GameMode 前删除部分投影，在 GameMode 后无条件 TeardownWorld。GameMode 的局部拒绝保留被外层破坏。更高层成功回滚后如何确认/重试尚需后续可达性审计，不在本轮冒称闭合。

新增 `Shanmen.0_0_10.Product.ControlledWeaponWorldLifecycle.ManagerDeactivationRetention`：真实持久 Run、发射飞剑、非零时钟、实际 AuthGameMode、两单位 World 材料、敌人；两次拒绝、恢复后一次清理和空操作。复用隔离夹具及真实引擎销毁拒绝，不手工设置退役结果。友元绑定不代表正式 M01 初始化，不调用 BeginPlay 或输入恢复。

## 3. 证据进度（Saved 原件仅本地）

| 证据 | 本地原件 | 实测结果 / SHA-256 |
|---|---|---|
| Red Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0.RedProof/BuildEditor/20260917T165441399Z-6934712e/stdout.log` | 51 actions / 238.05s / SUCCEEDED/native0；SHA `EAEB17F11E58394B9FBCACCD0A8197AD5646B75FC01A7F3619B1410392E5B137` |
| RedProof | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0/RedProof/20260917T165857571Z-9294d461/UnrealEditor.log` | 0/1，队列1/native0，崩溃指标0；SHA `C609B528E0F090BDE3F446DF4F6614EFC09C127C2F6DAEC3173E7845F64A3DAA` |
| 映射检查器自检 | `Saved/Automation/P28.5/regression-selftest.log` | 本轮执行517/517/native0；SHA `96B77D69044EEA0655A097D38D0629A20A894FF21CE4352562003F17ABCE6A5F` |
| 首次 Fixed Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0.Fixed/BuildEditor/20260917T170017637Z-f22dd872/stdout.log` | 37 actions / 145.25s / native0；SHA `3FAE59E9D7542F9CA5675DFB88E7DDDA2EE10C612B2FDAB5C8CB315F8E089C2F` |
| 首次修复后专项失败 | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0/WorldLifecycleFocused/20260917T170306644Z-ef3961d5/UnrealEditor.log` | 4/1，队列5/native0，崩溃指标0；SHA `93D7529FCFE2E98AF0CD1527EA81F7BE6038BACB27C40088040F7D582C1108A9` |
| 拆分断言 Diagnostic Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0.Diagnostic/BuildEditor/20260917T170431756Z-7d453980/stdout.log` | native0；SHA `B3E7AC54B43E326D5523C0CFD22431D1012F7787FD8B0B010F8D2A124D67BAD0` |
| 诊断单项失败 | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0/DiagnosticFocused/20260917T170507985Z-5d14413e/UnrealEditor.log` | 0/1，队列1/native0，崩溃指标0；SHA `F2133403CA73510D29DB0813FFC40EC772B20F2303C58E7029273710522D481E` |
| 新增测试误调 private 编译失败 | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0.Final/BuildEditor/20260917T170919779Z-28e14efe/stdout.log` | C2248 / OtherCompilationError / native6；SHA `007591A39BCD99F2E5F5FFCEC003719424E8F1CBB03D13037A41C0319C7849E2` |
| 最终 Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0.Final/BuildEditor/20260917T170942666Z-f15397d5/stdout.log` | 4 actions / 8.91s / SUCCEEDED/native0；SHA `24C9E9FB7CC87471E757E1ED1A45C43CC24805886C630D0ABA6979C1C3EA2CF1` |
| 最终 WorldLifecycle 专项 | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0/WorldLifecycleFocused/20260917T171002157Z-2dd94eeb/UnrealEditor.log` | 5/0，队列5/native0，崩溃指标0；SHA `B912D597FEB185F3AE99A490398EF30F5CDA24DE7AC13FEEA30470CF9753586D` |
| 最终 ProductFlow 专项 | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0/ProductFlowFocused/20260917T171022676Z-3fcdb831/UnrealEditor.log` | 5/0，队列5/native0，崩溃指标0；SHA `1D9A036DD0EE99FEDC9ED9B90123DC1144289B8C4DFF0ADC14C0881A0EA401D9` |
| Game | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0.Validation/BuildGame/20260917T171043610Z-afed937c/stdout.log` | 50 actions / 217.26s / SUCCEEDED/native0；SHA `CBE77F94D73F56AA6DB475D4BF97542975B6D440E98119E2A824EB652920307D` |
| 完整旧根 | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0/LegacyFullRoot/20260917T171421592Z-ffa4da39/UnrealEditor.log` | 1330/0，队列1330/native0，崩溃指标0；SHA `BD427CD9C007B692BCE23FDEF7832D008B34FE56CA997973F6F1FCDBAFB3FF83` |
| 完整新根 | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0/ShanmenFullRoot/20260917T171517335Z-4b0d3406/UnrealEditor.log` | 1428/0，队列1428/native0，崩溃指标0；SHA `57D9302D513EE5A813937CA03A329C5E6C431B6492BDE867BA74F5828F68D194` |
| 最终八路径映射 | `Saved/Automation/P28.5/regression-coverage.log` | PASS Changed=8 / Rules=3 / Required=84 / Logs=2，完整87行；SHA `3FEAB23FC45184C06C4B13D54CB350DDAD8C5342F090C909249FC4ED314A3B7F` |

Red Editor 已于16:58:39UTC结束，之后单项验证出现三条失败：两次 Manager 投影保留失败、一次恢复清理复合断言失败；GameMode原Run/owner/非零时钟、持久权威不变断言未失败。前两条是真实外层保留缺口；第三条包含多个条件，后续诊断证明存在测试契约误解，不把它归为第三个生产缺陷。只看native0不能将RedProof判为通过。日志中的启动Error/Warning不宣称为零。

最小生产修改：GameMode原void去激活函数返回Release是否被接受，原拒绝路径false、成功路径true；Manager将此调用移到所有自身清理之前，拒绝立即返回。保留Manager原void接口，未改Flow回滚、上层终局确认、EndPlay或UI调用；这些剩余语义继续列在有限审计项，不宣称已解决。既有GameMode测试增加bool返回值断言。

首次 Fixed Editor 17:02:43UTC成功结束。随后 WorldLifecycle 专项4/1，运行器按规则停止，未进入 ProductFlow/Game/全根。拆开恢复复合断言并重新编译，17:05单项0/1，唯一失败为“Recovered deactivation removes the World item”：FindInstance不为null。一次性销毁计数、Manager状态、World绑定、GameMode状态均通过。

读取 ItemSubsystem.TeardownWorld/NotifyWorldActorEndPlay、ItemAuthority.DestroyWorld 与既有 Destroyed/CompactDestroyedRun 测试后，确认现有契约保留 Destroyed 墓碑，而不是删记录。最终新增测试改为明确验证非零原数量、Destroyed归属、空Owner/Container/Slot、World列表排除和权威不变量。拒绝分支额外检查归属仍为World。未改ItemAuthority/ItemSubsystem。期间误用private ValidateWorldBindings导致C2248/native6；移除私有调用，沿用已验证的公开GetWorldActorCount和公开Authority.ValidateInvariants，不增加friend或改生产访问级别。

最终 Editor 17:09:52UTC前成功结束。17:10重新捕获1617个最终输入和103个原用户文件哈希到validation-inputs.json；之前锁定的是诊断前版本，明确不与最终版本混同。固定产品源码后，exec62858串行推进finish-validation.ps1，runner PID51628（外层45068）。两组最终专项均5/0；Game 17:14:21.1309719UTC完成，完整旧根从17:14:21.5942055至17:15:17.0475416UTC完成1330/0。Game仅编译产物，没有启动产品exe。

17:15:17UTC同一运行器启动完整新根，UE PID53144，目录 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.5.r0/ShanmenFullRoot/20260917T171517335Z-4b0d3406`。初始检查部分178/0，无完整队列，不把部分计数或活动日志当完整证据。后续heartbeat续查此实例；无须重跑已完成Editor/Game、专项或旧根。

17:49 heartbeat续查：exec62858及原runner51628、UE53144仍在；同一日志已到部分910/0，最后记录17:48:59UTC，Fatal/Ensure/Unhandled指标0，尚无最终队列或run-state。1617个锁定输入、103个原用户文件及两份OverallReadiness哈希均未变；index为空，105个未跟踪路径仍为103个原用户文件和本阶段两草稿。不启动重复验证，不改产品源码、不提交未完成阶段，保持当前运行等待下一轮。

18:22 heartbeat续查：仍为exec62858、runner51628及UE53144，同一新根日志部分1174/0，仍有18:22:58UTC的World清理记录，崩溃指标0，无最终队列或run-state。1617个输入、103个原用户文件及两份OverallReadiness哈希再次全部一致；index为空，未跟踪105项不变。只记录进行中结果，未重复启动、修改锁定源码或提前提交。

以上进行中记录的推进顺序为 完整新根完成 → 改动映射与文档检查 → 精确提交/推送。忽略目录中的validate.ps1联合检查计数/队列/native/crash；finish-validation.ps1顺序推进并在每段核对锁定输入，任一步失败即停止；capture-inputs.ps1仅生成输入哈希，不作为产品提交。没有因完整新根耗时而重复启动或拼接部分计数。

17:13核对最终1617个输入与103个原用户文件全部哈希未变，两份OverallReadiness保持第1节哈希；阶段草稿3个相对链接有效，git diff --check通过，index为空。此为续接检查，不是最终提交门。冻结索引保持P28.4的已交付入口，本阶段验证完成后再更新；未提交或推送未完成阶段。

## 4. 最终复核与精确交付（19:17 heartbeat）

完整新根run-state记录开始 `2026-09-17T17:15:17.3383372Z`、结束 `2026-09-17T18:45:05.8155179Z`，SUCCEEDED/native0，约89分48秒。原日志1428个Success、0个Fail和精确1428队列结束；Fatal/Ensure/Unhandled为0。exec62858外层也正常结束，最后输入检查通过。原runner PID51628已被系统RuntimeBroker复用，不能以PID数字仍存在认定测试在运行；使用进程名/命令行及原run-state判定，没有操作该系统进程。

- 逐项复核两组专项、完整双根、双构建状态与SHA；保留RedProof、错误测试断言和C2248编译失败原件，不改写成成功。阶段已有13份原件哈希再次一致，随后补入新根和映射哈希。517自检复用本阶段实际结果，脚本/映射字节不变。
- 八路径映射实际PASS：3条规则、84个去重必跑组，全部由本阶段完整新旧根覆盖；只使用2份健康完整日志。87行完整检查输出保存本地，不使用截断输出作完整审计清单。
- 1617个锁定产品/脚本输入、103个原用户文件及两份OverallReadiness原字节哈希保持不变。index入场仍为空，105个未跟踪路径中的两项为本阶段Report/Log，不混入原用户文件。
- 冻结索引更新P28.5局部释放拒绝传播和最近完整验证入口，FZ-1/2/3继续开放。不把Manager直接端口专项当全部上层回滚/终局或正式M01激活证明。
- 三份阶段Markdown的54个相对链接目标全部存在，git diff --check通过；最终15份证据原件SHA均与第3节一致，103个未跟踪用户文件路径集合保持一致。
- 仅精确交付五源码、冻结索引、本Report、本Log共8个文件；不提交OverallReadiness、Saved原件或其余用户文件。精确暂存范围检查后普通提交/推送，不强推。

构建/完整回归及交付所用五源码原字节SHA-256如下；Git依仓库配置规范化换行：

| 文件 | SHA-256 |
|---|---|
| `Source/demo_map/demo_mapGameMode.cpp` | `8802B9F84144DE4386E8F00DD6EA7257E644A2CB7F3A80F086632AB7526FF2CF` |
| `Source/demo_map/demo_mapGameMode.h` | `F99FACF0C6034166F83607FA587D064F76FC20944B7432E1669849EC8EB748BA` |
| `Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp` | `619A93C4C6D2F397707E49D7AF38A38A871EB90F1D7685DA4C2E8BADFFE1C4FC` |
| `Source/demo_map/demo_mapV3ProgressionManager.cpp` | `14350C0C60D293ADA02A64915B613B3B13EB5F696376E087B13A66BE56DE3C1E` |
| `Source/demo_map/demo_mapV3ProgressionManager.h` | `6394BFE7EC9005573E5B3BB1DB3767C2ACD58292924A69808C3E34BE332D1DBC` |

下一轮仅沿既有FZ-2核对成功技术回滚后如何确认/重试World释放，以及一般激活失败、终局和EndPlay的可达性；先取证再最小修复。FZ-1剩余入口和FZ-3最终冻结仍待完成，不进入游戏性开发。

- [Report](../Report/Dev.D.UE.0.0.10.P28.5.r0_report.md)
