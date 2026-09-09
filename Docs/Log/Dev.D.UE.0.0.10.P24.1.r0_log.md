# Dev.D.UE.0.0.10.P24.1.r0 Development Log

## 1. 目标

- 审计 P24.0 物理剑气入口到现有 P18 产品链的最后一跳；
- 用真实局内装备、属性、Run、行动仲裁与投射物替代合成产品结果；
- 证明真实 HostBusy 冻结请求和 Retry 不重新采样；
- 发现生产缺陷时修最短责任边界，不新增 Manager 或第二权威；
- 按改动文件映射完成回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线提交：`dd0667942f27ac87370826ad9ecd67ecc9166a12`；
- 工作分支：`agent/0.0.10-p24-1-sword-qi-product-loop`；
- 基线阶段：P24.0 已完成可重映射 `B` 键与主 HUD 剑气反馈；
- 开始时 tracked tree clean，保留用户 103 个未跟踪文件；
- P18.0–P18.10 已存在纯运行时、WorldDelivery、RunHost、Session、Controller、输入、命令事件与 availability 路由。

## 3. 审计结论

生产链的所有主要节点已经存在，因此本轮不建立新的组合服务。P24.0 的 `IssueRetryAndLock` 只验证物理按键到 command router，并预期产品路由不可用；其繁忙结果由测试人工构造，没有证明正式装备、攻击力、行动仲裁或投射物。

本轮把同一用例升级为完整的真实权威链。审计同时发现 ProductController 新意图在回执前进入 `CapturedIntents`，与 GameMode 同步占用投影形成循环不变量缺陷。

## 4. 实现

### 4.1 产品事务顺序

`Fdemo_mapShanmenSwordQiProductController::TrySubmit` 对新意图改为：局部捕获 → 同步 `RouteCaptured` → 得到 `LastRoute` → 提交 `CapturedIntents` → 提交后自检。

授权期间 GameMode 现在看到的是完整的既有 Controller 状态，不会读到半完成新意图。Host、Session、命令身份、装备和攻击力的所有权保持不变。

### 4.2 控制器回归

`FSwordQiControllerFixture::Submit` 不再给行动仲裁直接传空快照，而是调用当前 Controller 的 `TryAppendOccupancy()`。这条回归会在授权闭包内部验证 Controller 可以安全投影自己的状态。

### 4.3 物理产品用例

测试夹具把显式创建的 PlayerController 注册到 transient preview World，以匹配 GameMode 的 `GetFirstPlayerController()` 权威查询。

`IssueRetryAndLock` 现在：

- 建立并绑定真实 ItemSubsystem、AttributeComponent 与 Health；
- 开始真实 Run，添加并装备 TrainingBlade；
- 验证 `Primary01=6` 时最终 AttackPower 为 8；
- 第一次 `B` 创建在飞 Sword Qi Projectile；
- 第二次 `B` 从真实 HostBusy 保存冻结请求；
- 退役首枚投射物后改变位置、方向和属性；
- 第三次 `B` Retry 同一命令、起点、方向和攻击力 8；
- 退役第二枚投射物；
- 验证 UI 锁不消费第三个事件身份；
- 顺序结束 Owner、Controller、Coordinator 和 Item Run。

## 5. 首次失败与根因收敛

| 证据 | 结果 | 发现 | SHA-256 |
|---|---:|---|---|
| `P24.1_SwordQiPhysicalInput_initial.log` | 4/1 | 首个完整产品用例失败，触发诊断展开 | `75B2E92EE43F23D42FE8267E73A1EA1F3CDCFED761D9DA7BB801FF12B54BF121` |
| `P24.1_SwordQiPhysicalInput_diagnostic.log` | 4/1 | World 未暴露显式 Controller，`world_player=0` | `5F5FEAFBE5C0E4439AFC0FABC9CF86EDA1160ACD249EE6D1FEDD2100746A450C` |
| `P24.1_SwordQiPhysicalInput_after_world_binding.log` | 4/1 | 装备与 AttackPower=8 已通过；仲裁拒绝 malformed Host occupancy | `14C3C1C978B977B09A7F7BBA217022543490A6C42ABB87F5FEEFFBE28C7F7085` |

World 注册修复了测试夹具边界；第二次失败定位到生产代码的半完成意图公开顺序。两类问题分别修复，未用放宽断言或伪造授权绕过。

## 6. 聚焦验证

| Group | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 | 0 | 268,837 | `619F7FF745B059ED15FA7651B432A069DA40E1A8DAC7E92B4B2400533327AD42` |
| `Shanmen.0_0_10.Product.SwordQiProductController` | 4 | 0 | 265,456 | `33E51799219DA8DA43839D81966D17B142ED0B3EAD07BD1BAC56E9B5F1C1E143` |

两组均有唯一 RunTests 命令、0 Fail、UE 5.8 成功终止标记和原生退出码 0。

## 7. 完整回归与覆盖

| Group | Success | Fail | Test window | Bytes | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10` | 1,277 | 0 | 约 72m04s | 1,947,232 | `B867E6858C2D2F79DC8AE6F60FDABB1974F4995FE48E75DF4881FACF879175DC` |
| `demo_map` | 1,330 | 0 | 约 37s | 1,655,487 | `62B33E0CFA5CFB1C48352497292ED3D84CD73C6C98AF26D8C803425B1572FE0F` |

完整合计 2,607/0。全树慢段来自既有持久化、恢复与多代轮换测试；运行期间保留原生时间跳变和网络探测超时警告，没有把它们改写为源码失败。

改动文件映射：`PASS Changed=3 Rules=2 Required=35 Logs=2`。覆盖日志 SHA-256 `C035CCA55D520C999E06E178B118119978AF5B7084FD0D96A2AB910D6D2465C4`。覆盖器自测 442/442，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建

| Target | Result | Native exit | Actions | SHA-256 |
|---|---|---:|---:|---|
| `demo_mapEditor Win64 Development` | Succeeded | 0 | 4 | `986A3929AA83C2A5F5E5FA92AEFEB85302A6ABDFF9E28027230157E8FA19922A` |
| `demo_map Win64 Development` | Succeeded | 0 | 5 | `D3328CC13BBBAE3DA2DC9D9D778A98DB722700B26F523E2161EFD9FDF946476B` |

最终二进制：

- `demo_map.exe`：359,678,464 bytes，SHA-256 `BADB820D2DA1B82762CAA6C9677BCB58744B1C6C72A609B622E4B3B5311B3580`；
- `UnrealEditor-demo_map.dll`：18,942,976 bytes，SHA-256 `AD13A4B8A920F13C9B5558E80FB5424AFE6C22FCCC1838A93FE0AE7CA63EB142`。

## 9. 静态与 P/F 边界

- 非文档增量：3 files、`+208/-66`；
- 生产：1 file、`+14/-3`；测试：2 files、`+194/-63`；
- `git diff --check`：PASS；
- 新增模块、资产、UCLASS/USTRUCT、Actor、Subsystem、输入动作或持久状态：0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P 阶段证明物理脉冲到真实装备/属性/Run/仲裁/投射物、真实 HostBusy、冻结 Retry、UI 锁和清理。F 阶段保留真实设备输入、投射物渲染与碰撞、敌人 Impact、动画音效、HUD 布局和手感验收。

## 10. 提交与后续

精确提交 5 个文件：

- `Source/demo_map/demo_mapShanmenSwordQiProductController.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiProductControllerTests.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiPhysicalInputTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P24.1.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P24.1.r0_log.md`。

用户 103 个未跟踪文件保持未暂存，原始验证日志保持本地忽略。P24.2 建议先核实现有 WorldDelivery 是否已有从物理产品发射到敌人 Impact 的等价证明；仅在缺失时补最短端到端用例。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-1-sword-qi-product-loop>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-1-sword-qi-product-loop/Docs/Report/Dev.D.UE.0.0.10.P24.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-1-sword-qi-product-loop/Docs/Log/Dev.D.UE.0.0.10.P24.1.r0_log.md>
