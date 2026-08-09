# I 阶段项目基础自查门禁

## 目的

每一个 `I` 阶段都必须检查整个项目的基础调用，而不是只检查该阶段的测试脚本。此门禁是静态／结构审计，不启动产品，不替代 `0.0.9B.F` 的真实功能测试。

## 固定执行时点

1. `I` 阶段开始：确认基线、唯一入口、持久化和阶段边界仍然有效。
2. `I` 阶段中途：仅当阶段修改了启动、构建、进程、输入、存档、交接或配置基础时重跑。
3. `I` 阶段结束：生成本阶段最终 `foundation-audit.json` 与 `foundation-audit.md`，并将状态写入同名 Report。

标准命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\Invoke-Shanmen.ps1 `
  -Action Audit `
  -StageId Dev.D.UE.0.0.9B.Ix.y.rz
```

需要把未提交改动也作为结束阻断时，直接运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\Invoke-FoundationAudit.ps1 `
  -StageId Dev.D.UE.0.0.9B.Ix.y.rz `
  -RequireClean
```

## BLOCKER 检查

- 有效 Git 基线与本轮基础快照。
- `demo_map.uproject` 和 UE 安装可由统一解析器定位；不得在新脚本复制固定安装路径。
- 工作区与项目 `Latest Demo` 都委托 `Scripts\Invoke-Shanmen.ps1`；不得执行未 Cook 的 `Binaries\Win64\demo_map.exe`。
- 所有测试源仍受 `WITH_DEV_AUTOMATION_TESTS` 保护。
- Profile 生产根目录、temp、读回、backup、replace 和测试 `UserDir` 隔离边界没有退化。
- GameOnly、GameAndUI、UIOnly 和鼠标状态仍由统一输入上下文恢复链管理。
- Prompt／Report 使用持久化交接账本；没有真实附件可见确认时禁止记录 `REPORT_SENT`。
- 交接闭环必须到达 `NEXT_PROMPT_HASHED`；上传或页面中断进入 `BLOCKED` 后，使用 `ResumeBlocked` 返回失败前状态继续，禁止跳过步骤重开任务。
- 配置中不得出现重复关键项、模板项目名或固定服务 token。

任一 BLOCKER 失败：本 `I` 阶段停止交接，不运行后续 Prompt，不把 Report 标记为完成。

## WARNING 技术债

WARNING 不得被隐藏，必须进入 `I` Report，但可在不改变当前功能边界时延期：

- 历史脚本仍含固定 UE 路径或旧 TaskId。
- 历史／专项脚本仍绕过统一 PID、退出码与证据封装直接调用 `Start-Process`。
- 自动化源仍与 Runtime 模块共存并导致全模块 `bUseUnity=false`。
- 单个运行时源文件超过 4,000 行。

连续两个 `I` 阶段未下降的同一 WARNING，下一 `I` 阶段必须给出拆分任务或策划豁免。

## 阶段职责

- `P`：功能开发、代码审查、Prompt 明确要求的单次最小编译；不进行实际产品测试。
- `I`：边界接入和项目基础自查；不把静态自查伪装成功能验收。
- `F`：一次规范 Package 后执行内部 Automation／Gauntlet、真实 Windows 前台输入、可见证据、存档隔离、候选包发布和 Latest Demo 切换。

## Windows 与 Unreal 调用分层

- `Scripts\Invoke-Shanmen.ps1`：唯一项目入口，负责 UE 解析、证据目录、PID、参数、退出码和日志。
- `ResolveLatest`：I 阶段只解析 Latest Demo 的真实启动目标与参数，不启动 UE；不存在打包产物时明确报告 `EDITOR_GAME_FALLBACK`。
- Unreal Automation Driver：F 阶段的引擎内输入／Slate 语义验证。
- Gauntlet：F 阶段的打包产品会话、超时、崩溃和最多三次恢复。
- Windows UIA／WGC／SendInput 工具：F 阶段的真实外部窗口验收；必须使用 PID/HWND 和已解锁前台桌面。
- Prompt／Report 浏览器交接：由浏览器控制完成，`Update-HandoffLedger.ps1` 只在页面真实状态已经确认后推进状态。

## 已登记的结构迁移

1. 建立 `demo_mapAutomation` Developer/Test 模块或测试插件，把 53 个自动化源移出 Runtime 模块并恢复 Runtime Unity 编译。
2. 将 `demo_mapV3ProgressionManager` 按 Profile、Run Lifecycle、World Interaction、UI Host 和 Automation Adapter 拆分。
3. 将 `demo_mapCodeBOutOfRaidProfile` 按 Store、Migration、P5/P6、Run-local containers 和 terminal receipts 拆分。
4. 旧 F0/F1 与地图脚本逐个改为调用统一模块；迁移完成后移入 `Scripts\Legacy` 或删除。

这些拆分必须在独立 `I` Prompt 中执行，先保持行为不变，再由 `F` 做真实验证。
