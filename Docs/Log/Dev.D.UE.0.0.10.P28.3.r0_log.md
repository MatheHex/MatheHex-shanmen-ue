# Dev.D.UE.0.0.10.P28.3.r0 Development Log

## 1. 状态、基线和恢复

状态 `P28_3_VERIFIED`；2026-09-17 01:27 heartbeat 完成最终原件复查及阶段交付核对。保留原始失败，不表示整个框架最终冻结。

- 2026-09-16 22:14 heartbeat 开始审计，初始 HEAD `f4e34d277163eb3e6644df9a9fc8907ff05724e9`。准备新测试并完成 Red Editor 后切换到用户的总体报告请求，未继续产品修复。
- 23:00:29.040Z heartbeat 恢复，HEAD `175a651991b93aabaf2b9c19cd23bb9ab4ba9bef`，中间为总体 Report/Log 文档提交，保持该交付。Index 为空，已有两处本阶段测试修改，无运行中的 UE/UBT。
- 完整读取 P 基线、冻结索引、最新 P28.2 Report/Log；检查原 Red Editor run-state 已 SUCCEEDED/native 0，不重复编译原始测试。
- 1,617 个产品/脚本输入与 103 个既有未跟踪用户文件记录哈希；恢复时用户文件与原阶段基线一致。
- 2026-09-17 01:27:01.322Z 恢复交付时 HEAD 为 `a8bc3626eb3d8de0ca7a6e8b3720bbb304e3b0d8`；相对 P28.2 只有总体 Report/Log 的已提交变化，本阶段三个源码改动仍未暂存。没有运行中的 UE/UBT；不重复启动构建/测试。

## 2. 根因与修复边界

GameMode.DeactivateV3MissionContentForPreparation 不检查 ReleaseCombatProductRun 的 bool，后者即使保留 PendingCombatRunRetirement，调用方仍继续销毁任务对象和重置标志。

新增测试复用已有 PreparationAdapter/ControlledWeaponWorld 瞬态夹具，实际 seed/cutover/整备/启动、飞剑发射和非零 tick；以引擎 Actor 销毁拒绝制造具名失败。两次原样调用均未保持敌人/标志，恢复后销毁观察计数也不正确。RedProof 0/1，native 0 不等于测试成功。

生产改动是去激活入口拒绝即返回；没有新建 owner、权威、事务 ID 或持久恢复模型。代码审查发现 M01 激活失败分支原来依赖去激活清空 bV3MissionContentActive 才返回 false，因此补显式失败返回，避免保护对象后反而报告启动成功。该补充晚于 First Fixed Editor，Final Fixed Editor 后再跑专项。

专项验证失败两次保持、同 owner 恢复仅清理一次、空清理幂等、durable snapshot 和 Runtime RunId 均保持。未触及 Manager 更外层清理和强制 EndPlay；新增测试也不覆盖正式 M01 激活链，不扩张证明范围。

## 3. 当前原始验证

路径相对 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`，Saved 原件不上传。

| 组 | 原日志 | Success / Fail | SHA-256 |
|---|---|---|---|
| RedProof | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.3.r0/RedProof/20260916T230120434Z-c67c4f8b/UnrealEditor.log` | 0 / 1 | `E438E13C83C166C34A0126DB0C74A097CEC86025D97E07545F2F33C467A0EBCE` |
| WorldLifecycleFocused | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.3.r0/WorldLifecycleFocused/20260916T230342693Z-65c6f145/UnrealEditor.log` | 4 / 0 | `016A109CBC9E63D41ED53582A5A95A515B81E8E39C720E8BB3A79DE7111376A5` |
| LegacyFullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.3.r0/LegacyFullRoot/20260916T230746889Z-ac92d5ce/UnrealEditor.log` | 1330 / 0 | `8BB7255B51FC48108B96DCCE2719DFAF37D89F4A7E99886B13345D04374C9BBF` |
| ShanmenFullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.3.r0/ShanmenFullRoot/20260916T230842671Z-1c64f004/UnrealEditor.log` | 1426 / 0 | `C8E1C1FAEEF54A5D243FB9CBE297FD555AAC922DCC23FAD4083D163ECC8292A7` |

四组精确队列分别为 1、4、1330、1426；native 均 0，Fatal error / Ensure condition failed / Unhandled Exception 匹配均为 0，不宣称其他 Error/Warning 为零。原始失败包含两次任务保持断言和一次最终销毁计数断言失败；不是 UE 崩溃，也不将其改写为环境故障。专项 4 项包含在新根中，不重复计数。

完整旧根于 23:08 UTC 结束，随后串行进入新根；新根 run-state 记录开始 `2026-09-16T23:08:42.6742201Z`、结束 `2026-09-17T00:13:03.8829772Z`，SUCCEEDED/native 0，约 64 分 21 秒。最终 1426 个 Success 及精确队列结束均来自同一完整原日志，不拼接中间计数。期间没有重新启动或中断该根；原 session 83198 / runner 43016 已结束。Game 后及恢复交付时的 1,720 个锁定输入/用户路径均原字节不变。

## 4. 构建与锁定输入

| 构建 | 原目录 | 已知结果 |
|---|---|---|
| Red Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.3.r0.RedProof/BuildEditor/20260916T221805968Z-ed229f18` | 37 actions / 144.45s / native 0；stdout SHA `3809A6041FC4863002247DCAD4634BC6D2574C93C491FCEBA4EE8C5F74842327` |
| First Fixed Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.3.r0.Fixed/BuildEditor/20260916T230207661Z-6b400ebc` | 已完成，不作为最后源码版本的唯一证明 |
| Final Fixed Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.3.r0.FixedFinal/BuildEditor/20260916T230257305Z-ad08cedb` | 4 actions / 11.02s / SUCCEEDED/native 0 |
| Game | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.3.r0.Validation/BuildGame/20260916T230450204Z-e1c51c04` | 36 actions / 143.76s / SUCCEEDED/native 0 |

First Fixed Editor stdout SHA-256 为 `C23678EDF034F7A036C73449213675FEF73C4BA136F98D6ED50C17FE68E2A9C3`；Final Fixed Editor 为 `E44F9A1C8CA1A722EFB7106EBF5C1D7D0C0F023B3BFA25158E15AE5A34AC4D1F`；Game 为 `D6636C54D98E778750EC2414829592E1C669B55CE6ABE7E0D834723FDB9918F2`。Game 于 23:07:14.2620025Z 完成后才开始完整根，未并发构建和 UE 测试。

最终 Editor 和专项之后保存 `Saved/Automation/P28.3/validation-inputs.json`：1,617 个产品/脚本输入、103 个用户文件、HEAD 与原字节 SHA-256。Game 和完整根使用此锁定源码；恢复交付再次核对，输入和用户文件哈希差异均为 0，用户路径集合差异为 0。Ignored helper `Saved/Automation/P28.3/validate.ps1` 联合检查 Result/队列/native/崩溃指标，不提交为产品。

## 5. 有限后续项

- 三个源码命中 M01GameMode 与 ItemProductAdapters，要求 81 个去重组；精确六文件以两个完整根核验，PASS Changed=6 Rules=2 Required=81 Logs=2。检查器不是只看命令行声明，而是检查原日志结果与完成标记。
- 检查器自检 517/517，原件 `Saved/Automation/P28.3/regression-selftest.log`，SHA-256 `96B77D69044EEA0655A097D38D0629A20A894FF21CE4352562003F17ABCE6A5F`；检查器和映射未改动。
- Manager 的 DeactivateProfileWorld 在 GameMode 前清理拾取物/容器，之后 teardown Runtime；调用点包括技术回滚、激活失败、结算及 EndPlay。当前仅静态核对，没有声称全链失败保持已成立。
- GameMode 的两个旧可见 Smoke 调用点只被读取，不执行或修改；不凭其成功日志代表本轮 F 验收。
- FZ-1/2/3 尚未全部关闭，保持当前监控，不进入玩法或最终冻结。

已补全双构建/完整根原件、映射、自检哈希和输入保持核验；只精确暂存三个源码、冻结索引及本 Report/Log，不混入总体报告或用户文件。冻结索引只更新 P28.3 局部清理保持及剩余 Manager 边界，状态仍是 FREEZE_AUDIT_IN_PROGRESS。

- [Report](../Report/Dev.D.UE.0.0.10.P28.3.r0_report.md)

## 6. 最终交付核验（2026-09-17 01:27 heartbeat）

- 重新核对四组 UE 原日志、四次 Editor/Game 构建的 run-state 和 stdout SHA-256，均与上表匹配；完整根已结束，没有重新执行 UE 或启动 UI。
- 精确六路径映射再次 PASS：Changed=6 / Rules=2 / Required=81 / Logs=2，检查器原生 0。原件 `Saved/Automation/P28.3/regression-coverage.log`，SHA-256 `D458573EEBEAA8D9D5BF1226661AE35D36F6B7A96E7A6762622189074ECB744F`；自检 517/517 原件哈希与第 5 节一致。
- 三份阶段 Markdown 的 47 个本地相对链接目标存在，`git diff --check` 通过；没有改动回归映射/检查器来缩小必跑范围。
- 锁定 1,617 个产品/脚本输入及 103 个原用户文件的原字节无差异；原用户路径集合一致。额外以排序的“路径:原字节 SHA-256”清单计算阶段外 2,722 个跟踪/未跟踪文件的汇总哈希，入场与交付前均为 `4D2FECDBF36CB270F897523A59D8DF4AE5B81C96F81981D7D2B05A822538B53A`。不上传清单中的用户内容。
- 当前 HEAD 保持 `a8bc362`，原 index 为空。仅三个源码、冻结索引、本 Report/Log 共六文件进入阶段提交；不纳入总体报告、Saved 或个人文件。按普通非强制推送交付并核对远端提交一致。

交付源码的原字节 SHA-256（与构建及双根锁定输入一致；Git 按仓库配置规范化换行）：

| 文件 | SHA-256 |
|---|---|
| `Source/demo_map/demo_mapGameMode.cpp` | `AD32308693038BE05899AD4B75C54E033136A6E279824D4C2234D347CB1CCA0A` |
| `Source/demo_map/demo_mapGameMode.h` | `15450E134335F3586F31DACDDF2E4098ECE4D62BF51B3FA3E64E9D813BFD9A8C` |
| `Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp` | `406A37EC494572ACFF3B85E917CDF053964C184792F220096E47AD95AF64C891` |

下一轮先检查 Manager 外层清理/回滚的可达失败传播，沿现有 FZ-2 有限清单继续；不把本次完整根成功当作 FZ-1/2 已全部关闭，不进入 F 阶段或新增玩法。
