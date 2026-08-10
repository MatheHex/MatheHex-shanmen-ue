# Dev.D.UE.0.0.9B.F0.0.r0 Report

## 1. 基线、运行面与只读边界

- 基线 commit：`804437f2cf97a0856932a0e7c70d3dd7504c8cf4`。
- 开始时 `git status --short`：空，工作区干净。
- 测试开始时间：`2026-08-10T10:14:43-04:00`。
- 地图：`/Game/M01/Maps/L_M01_Expedition`。
- Profile：沿用应用正常流程已有的本地 Profile，并非本轮新建；OwnerId 为 `B64B94F1-4B7F-6D3E-2F25-7796C570EFDD`。未读取、编辑或替换存档文件。
- PIE：已启动、经正式宗门 UI 部署进入 `UEDPIE_0_L_M01_Expedition`，RunId `BF1D139E-48E3-A7FB-97FE-C4AD47FE8968`。
- Standalone：已启动独立进程、经正式宗门 UI 部署进入 `L_M01_Expedition`，RunId `2C646F61-40F9-CD89-81CD-6CB75D307BDB`。
- 全部操作均为真实窗口、鼠标与键盘路径；未调用 Console、自动化测试、C++/Blueprint 函数、fixture、存档编辑或 UI 注入。
- 未修改生产源码、资产、配置、测试、数据表或存档。新增内容仅为本 Report 与一张 F0 运行证据截图。

## 2. 逐项测试表

| 用例 | 真实前置样本 | 操作 | 预期 | 实际 | 证据 | 结果 |
| --- | --- | --- | --- | --- | --- | --- |
| F0-B0 PIE 启动与部署 | 既有本地 Profile；正式宗门入口 | PIE 中点击“部署 M01”，进入真实 Run | M01、Pawn、输入与 Code B Run bridge 就绪 | 部署成功，进入 InRun | `Saved/Logs/demo_map.log:2280`，`DeploymentSucceeded` | PASS |
| F0-P49-PIE BasicCache 入口 | 同一 PIE Run；地图正式 P10 BasicCache anchor | 部署后观察正式世界与活动 Run inventory，并准备按 `[G]` 打开 BasicCache | `M01.CodeBNormalContainer.BasicCache.01` 生成且可通过正常交互打开 | 在部署成功前同帧，正式目标生成失败：`no safe projection`；世界中没有可进入的 BasicCache 正式入口，故无法取得 SpiritDust/IronShard source | `Saved/Logs/demo_map.log:2268`；`demo_map.log:2270` 显示其余 M01 content 正常 | FAIL |
| F0-B1 Standalone 启动与部署 | 同一 Owner 的正常 Standalone 会话 | 选择“独立进程游戏”，点击“部署 M01” | 独立运行面进入同一正式地图且 BasicCache 可用 | 部署成功，但相同 BasicCache 投影失败再次出现 | `Saved/Logs/demo_map_2.log:1819`；`demo_map_2.log:1807` | FAIL（复现） |
| F0-P49-Standalone simple 闭环 | Standalone 中应存在可打开/揭示的 BasicCache | 准备执行 normal Drag → GroundDrop → normal pickup | 完整 simple-stack 闭环 | 前置 P10 actor 不存在，无法合法取得 exact source；未伪造样本 | 截图 `Docs/Report/Evidence/Dev.D.UE.0.0.9B.F0.0.r0/02_standalone_inrun_missing_basiccache.png`；`demo_map_2.log:1807` | BLOCKED BY DEFECT |
| F0-P50 complete graph | 应由真实 P10/P18 流程物质化的 WindTalisman/BackpackLevel1 | 仅在存在真实样本时执行 | complete graph drop → close/reopen → pickup | BasicCache 正式入口未生成，无法通过允许路径取得任何 P18 parent；未 reroll、复制或编辑存档 | 两运行面同一 P10 错误 | NOT RUN |
| F0-P31 multi-record isolation | 同 session 中同时保留 P49 与 P50 record | 分别打开/关闭/拾回 | other record 身份保持 | P49/P50 均无法建立首个合法 record | 同上 | NOT RUN |
| F0 生命周期与保存/重开 | 至少一个 accepted WorldDrop record | 关闭/重开 UI 与合法恢复 | durable record/Actor 一致 | 无 accepted record，不能在不造假的前提下验证 | 同上 | NOT RUN |
| F0 拒绝路径 | 当前 exact source/record | 对错误目标或 stale record 操作 | 拒绝且零写入 | exact source/record 从未出现；为避免把无前置输入冒充拒绝验证，本项未执行 | 同上 | NOT RUN |

## 3. P49 两运行面与 exact-record 结论

PIE 与 Standalone 均完成了实际工程启动、正式宗门入口部署、M01 RuntimeReady 与 InRun 转换，但都在 `InitializeCodeBNormalContainerTarget` 的生产入口失败。错误发生在任何 P49 source、WorldDrop record、derived container 或 Actor 建立之前。因此两运行面均不能完成 P49 normal Drag → WorldDrop → normal pickup，亦无 exact-record cleanup 或 other-record isolation 可以合法声明为已验证。

没有观察到 P49 写入、record 清理、Actor 刷新、重复或丢失；这不是通过结论，而是因为对应产品入口没有生成。

## 4. P50 真实样本纪律

未取得真实 P18 canonical parent。尝试边界严格限于两个正常运行面各一次正式 M01 部署；在两者中 P10 BasicCache actor 都因同一 safe-projection 错误缺失。未编辑 profile/save/JSON/INI/DataTable，未强制随机种子，未调用 materializer，未添加 fixture，未复制 WindTalisman/BackpackLevel1，也未将普通地图掉落冒充 P18 样本。

因此以下 P50 项目均未测试：parent 定义与唯一 child closure、normal ground drop、spatial record/Actor、close/reopen、normal pickup、graph 保持、record cleanup、P47/P48/P30 附加分支。

## 5. 多 record、持久化、拒绝与 silent fallback

同一 OwnerId 在 PIE 与 Standalone 的正常启动/部署中保持一致，证明本轮使用的是同一应用 Profile 边界；但由于首个 BasicCache exact source 不存在，不能建立 P49/P50 record，也不能验证 P31 多 record、accepted-operation 保存/重开或 exact-record 拒绝。

未执行任何替代入口、自动目标、自动装备、child entry、Console、Actor 直调或存档修改；所以没有把 fallback 当作产品结果。无法对 duplicate Actor/record 或 BasicCache 回填给出通过结论。

## 6. 缺陷 F0-001

- 首次可观察异常：PIE 正式部署在 `DeploymentSucceeded` 前，同一 M01 初始化帧输出 `CODEB_P10_BASIC_CACHE: no safe projection for target=M01.CodeBNormalContainer.BasicCache.01 anchor=M01.Resource.TIER_1.Cluster.01`。
- 运行面与身份：PIE，OwnerId `B64B94F1-4B7F-6D3E-2F25-7796C570EFDD`，RunId `BF1D139E-48E3-A7FB-97FE-C4AD47FE8968`；随后在 Standalone、同一 OwnerId、新 RunId `2C646F61-40F9-CD89-81CD-6CB75D307BDB` 再现。
- 预期：在 anchor `M01.Resource.TIER_1.Cluster.01` 附近生成 identity 为 `M01.CodeBNormalContainer.BasicCache.01` 的正式 BasicCache actor，并允许 `[G]` 进入 P10/P18。
- 实际：`ResolveSafeWorldLocation` 未返回可用位置；生产目标投影被放弃。其余 M01 内容仍正常（149 reward sources、14 enemies），Run 仍进入 InRun。
- 持久化/记录/Actor：错误发生在 BasicCache actor 与任何 P49/P50 record 之前；未发生可归因于 P49/P50 的持久化、record cleanup、WorldDrop Actor 刷新、重复或丢失。
- 可复现性：在不修改文件的第二次真实 Standalone 操作中稳定复现。
- 影响范围：阻断 P10 BasicCache 正常交互，并连带阻断 P18、P49、P50 与依赖其 record 的 P31 F0 验证。
- 建议分流：后续 Fix 任务应只审计 BasicCache anchor 偏移、地图碰撞/可见性地面与 `ResolveSafeWorldLocation` 的生产投影条件；本 F0 不实施修复。

## 7. 证据清单与未测项

- PIE 日志：`Saved/Logs/demo_map.log:2268`（首次 safe-projection 失败）、`:2270`（其余 M01 content 正常）、`:2280`（部署成功与 Owner/Run 身份）。
- Standalone 日志：`Saved/Logs/demo_map_2.log:1807`（第二次同故障）、`:1809`（其余 M01 content 正常）、`:1819`（部署成功与 Owner/Run 身份）。
- Standalone 截图：`Docs/Report/Evidence/Dev.D.UE.0.0.9B.F0.0.r0/02_standalone_inrun_missing_basiccache.png`，SHA-256 `C52A8798310163022C3B263244EB1CA9F70463980A1C7494A916FE6AB62973B1`。截图显示正式 Standalone 已进入 M01 InRun，但当前交互焦点仍是安全出生区，未出现可打开的 `BASIC CACHE` 目标。
- 未测：P49 两运行面完整闭环、SpiritDust 与 IronShard 两种 source、P31 exact cleanup 与 other-record isolation、所有 P50 graph 项、第二种 P18 definition、P47/P48/P30、full/wrong target、stale record、保存/重开、terminal/recovery。原因均为 F0-001 在共同生产前置入口处阻断，继续尝试无法产生合法样本。

## 8. 最终状态

F0-001 是两个真实运行面均可复现的产品失败。按 F0 停止条件，不继续伪造样本、不扩大测试、不修改工程，也不自动开始 Fix、P51 或 F1。

NEEDS_FIX
