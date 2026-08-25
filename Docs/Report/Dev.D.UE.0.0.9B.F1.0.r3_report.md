# Dev.D.UE.0.0.9B.F1.0.r3 验收报告

## 整体结论

`F1_NEEDS_FIX`。Windows desktop-control host、File Explorer、Unreal Editor、Standalone 与 PIE 的正式入口均已恢复并可真实控制；本轮不是宿主阻塞。0.0.9B 在 Standalone 与 PIE 中均能加载 `/Game/M01/Maps/L_M01_Expedition`，但正式产品启动页立即显示 `The retained Profile adapter returned an invalid OwnerId.`。同轮日志进一步确认 Schema 6 Profile 迁移未提交，随后任务初始化失败（`Targets=0 Exit=0 Enemy=0 Health=1`）。M01 部署与仓库入口不能进入业务流程，因此一次性综合生产闭环未达到可接受状态，必须分流为产品修复。

## 执行身份与边界

- Prompt：`Dev.D.UE.0.0.9B.F1.0.r3_prompt.md`；SHA-256：`DC1325F1C9FB34E319447ED72FA70A7957C852E59D92E3A40F96F87964FC199B`。
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`。
- 验收时 Git 分支：`agent/f1-0-r1-baseline`；验收前 HEAD：`16ac6daedbfc81ec11e4580799151b0060449613`。
- 验收收口时间：`2026-08-25T03:13:53.0202734Z`。
- 本轮只使用真实 File Explorer、Unreal Editor、Standalone/PIE 窗口、正式地图与产品 UI；未使用 Console、debug spawn、fixture、脚本直调、UI injection、存档编辑、手工 receipt、自动化测试、Cook 或 Package。
- 未修改或修复产品源码、资产、配置、schema、构建设置或测试数据；未 reset、checkout 或 clean。`git diff --check` 原生退出码为 `0`。

## 实际覆盖与结果矩阵

| 验收面 | 正式操作 | 实际结果 | 状态 |
|---|---|---|---|
| Windows host 与项目入口 | 枚举并唯一绑定 File Explorer；从 `.uproject` 的“打开方式”选择 Unreal Engine；加载 `L_M01_Expedition` | 截图、聚焦、鼠标与键盘输入均可用；Unreal Editor 正常启动并加载地图 | PASS |
| Standalone | 从 Unreal 运行模式“独立进程游戏”启动真实产品窗口 | 地图加载成功；启动页显示 retained Profile adapter 返回无效 `OwnerId`；M01 部署与仓库入口不能进入后续业务流程 | NEEDS_FIX |
| PIE | 从“新建编辑器窗口（PIE）”启动真实 PIE 产品窗口 | 复现相同无效 `OwnerId`；日志记录 Profile 迁移未提交与任务初始化失败 | NEEDS_FIX |
| M01 部署 / BasicCache | 在两运行面尝试正式“部署 M01 荒山灵矿遗迹”入口 | 入口保持在启动页，未形成可用 Owner/Run；不能安全进入 `M01.CodeBNormalContainer.BasicCache.01` | BLOCKED_BY_PRODUCT |
| 容器、物品、图谱与 WorldDrop | 依赖成功建立 Owner/Run 后执行 | 首个产品缺陷前置未通过，未运行 | NOT RUN |
| 生成奖励、receipt、重启/重绑与重复门 | 依赖 accepted source 与 durable receipt | 未形成 accepted source 或 durable receipt，未运行 | NOT RUN |
| reconciliation、Terminal 与局外仓库 | 依赖有效 Run 与正式终局 | 未建立有效 Run，未运行 | NOT RUN |

## 首个异常与复现证据

首个产品异常发生在正式地图已经加载之后、任何 M01 部署或仓库业务操作之前：

1. Standalone 产品启动页显示：`The retained Profile adapter returned an invalid OwnerId.`。
2. Standalone 日志 `Saved/Logs/demo_map_2.log`：
   - 第 1581 行：`PROFILE_NORMAL_STARTUP: production initialization status=4 diagnostic=Schema 6 migration did not commit: Existing primary is invalid or does not match the caller generation.`
   - 第 1582 行：正式入口验证为 `Valid=1`、`RuntimeM01=1`、`RuntimeReady=1`。
   - 第 1585、1596 行：`T7: mission initialization failed. Targets=0 Exit=0 Enemy=0 Health=1`。
   - SHA-256：`B4FBDA4A282E83518DE689AC65830299D2AF660FAD0FEC323320483A36DE6DB4`。
3. PIE 日志 `Saved/Logs/demo_map.log`：
   - 第 2100 行：`UEDPIE_0_L_M01_Expedition` 正式进入 play。
   - 第 2111 行：相同 Schema 6 迁移未提交诊断。
   - 第 2112 行：正式入口验证仍为 `Valid=1`、`RuntimeM01=1`、`RuntimeReady=1`。
   - 第 2115、2126 行：相同任务初始化失败。
   - SHA-256：`958F75096BCFEA54263498C55E1FE20F03DAD2E22897B4D5150039A8AEA85536`。

复现率为 `2/2`：独立进程 Standalone 与新建编辑器窗口 PIE 均复现。由于入口与地图验证通过而 Profile/Owner 初始化失败，缺陷归属产品运行态，不归属 Windows 控制宿主、项目路径或地图入口。

## 持久化影响与验收前后变化

- 验收前既有 `Saved/SaveGames/Shanmen/Profile_Default.json` 与 `.bak` 的 UTC 修改时间均为 `2026-08-13T23:45:25Z`；验收后时间未变化，SHA-256 分别为 `B0A1D74E8D5B086D0D2D9388516039A0B41735C154B77C4E5A539E09C40D11C9` 与 `333DDA700349F4C19BDD6758F66D5AFE356887212B248B6B18EA9C4DF16A762D`。
- 验收运行产生/更新了 `Profile_Default.json.tmp`，长度 `8831`，UTC 修改时间 `2026-08-25T03:10:13Z`，SHA-256 `AAD0EB581DAF3B595F99FA0520B010C5E92F782EBC0CB1337B0FC256E6311F8E`。该临时文件与“migration did not commit”同时出现，属于本轮实际观察到的持久化副作用；未删除、编辑或替换它。
- 未建立有效 OwnerId、RunId、accepted source、durable receipt、物品、WorldDrop 或 terminal settlement；因此没有可验证的正常业务持久化结果，也无法证明重复门、pity/budget 或局外仓库一致性。
- 产品代码、资产、配置、schema 与构建设置在验收前后均未改变。运行时仅新增/更新 Unreal 日志、临时 Profile 文件、编辑器运行工件及本 Report。
- Standalone、PIE 与 Unreal Editor 均已正常结束；没有继续运行产品或自动进入下一任务。

## 未测项、影响与修复分流

BasicCache 安全投射、Normal/BodyContainer、装备/快捷栏、拆分合并、关闭重开、普通/完整图谱、WorldDrop 投放/拾回/拒绝、Chest/Corpse/Boss/FullMap 奖励、manifest/预算/pity/affix/provenance、durable receipt 重放、Run rebind、重复授予门、自然 reconciliation、Terminal 与局外仓库均未测。原因不是验收时间不足，而是正式产品在建立 Owner/Profile/Run 的第一关键门即失败；继续探索会失去真实生产语义并违反 F 阶段安全边界。

建议后续正式修复任务聚焦：Profile Schema 6 migration 的 caller generation/primary 有效性判定、retained Profile adapter 的 OwnerId 产生与重绑，以及任务初始化对有效 targets/exit/enemy 的依赖。修复后必须重新执行新的 F Prompt；本 r3 不在原地修改或重试。

## GitHub 交付

- 本 Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/f1-0-r1-baseline/Docs/Report/Dev.D.UE.0.0.9B.F1.0.r3_report.md>
- 按当前用户授权，本轮 GitHub 推送只包含本 Report；未夹带工作区其他累积修改、Prompt、原始日志、截图或存档。关键原始日志的文件名、行号、时间与 SHA-256 已完整写入本 Report，原始文件保留在本机工程 `Saved` 目录待审核。

F1_NEEDS_FIX
