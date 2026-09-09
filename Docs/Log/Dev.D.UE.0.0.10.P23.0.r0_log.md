# Dev.D.UE.0.0.10.P23.0.r0 Development Log

## 1. 基线与目标

- base：`7f0b34c1d321d3d84f09afd5ed74005bf1dfc025`（P22.12 hand-release origin）；
- branch：`agent/0.0.10-p23-0-divine-sense-reveal`；
- 目标：把既有 P19 神识纯产品链接入 M01 玩家输入、Combat Run、真实存活敌人和主 HUD；
- 边界：复用既有 Controller/Adapter/Receipt，不新建平行神识系统，不改伤害、敌人 AI、库存、存档或地图资产，不启动 UI/PIE/产品 executable。

## 2. 审计与切片选择

P19 已具备确定性 Scan、World Observation、Product Controller、Logical Input Adapter、SpiritEnergy 事务与 Receipt，但主游戏没有物理键、Run 所有者或 HUD 消费者。P23.0 因此选择最短可玩纵向切片：

```text
physical key -> existing logical/product chain -> live M01 subjects
             -> vitality + visibility evidence -> receipt -> HUD
```

没有继续堆叠新的抽象层；新增职责集中在现有 PlayerController、GameMode 和 HUD。

## 3. 输入与配置迁移

输入注册表新增 `DivineSense`：默认 `V`、Press-only、战斗分类。注册表总数从 30 变为 31，持久文本格式从 version 8 升为 version 9。

新增迁移测试构造两份真实 v8 配置：`V` 空闲时新动作取得 `V`；用户已把 Interact 改为 `V` 时保留该覆盖，并让神识使用空闲的 `G`。实时重映射到 `Z` 后重建绑定，旧 `V` 不再触发；结算输入锁仍拒绝 `Z`。

所有冻结注册表总数和持久版本断言同步更新，没有修改其它动作默认键。

## 4. GameMode 产品组合

`Ademo_mapGameMode` 新增并唯一拥有：

- `Fdemo_mapShanmenDivineSenseProductController`；
- `Fdemo_mapShanmenDivineSenseLogicalInputAdapter`；
- 最后一次成功 `FShanmenDivineSenseScanReceipt`；
- 3 秒 HUD reveal 到期时间。

Combat Run 启动时创建 `Resource.SpiritEnergy = 100/100` 的原型 Authority Snapshot，再通过既有 Product Route 启动 Controller 与 Adapter。若绑定失败，复用 `ReleaseCombatProductRun()` 回滚整个产品组合。释放时先结束 Adapter/Controller、记录脉冲数、清空 Receipt，再结束 Run Coordinator。

输入路由只接受正式 M01 地图、已激活 M01 敌人内容、ready Run、active Controller 与 active Adapter。若存在一个 pending retry，本次按既有重试合同执行；失败后的陈旧批次在一次重试后取消。

## 5. World 证据与 HUD

Live Evidence Provider 对每个已注册 M01 候选执行：Actor 有效性、Combat Vitality Host 绑定、Entity ID 一致、Vitality Snapshot 有效且大于零。通过者添加 `TargetLiving` tag，并使用 `ECC_Visibility` 从玩家到目标进行遮挡检测，Authority Revision 来自 Vitality Snapshot。

成功 Receipt 保存于 GameMode。HUD 将观察位置投影到屏幕：可见目标青色、遮挡目标金色并显示 `OCCLUDED`，同时显示距离和剩余 SpiritEnergy。到期判断读取现有世界时间，不增加 Timer 或第二个 Tick 状态。

## 6. 测试实现与修复轨迹

新增 `demo_mapShanmenDivineSensePhysicalInputTests.cpp`，共 5 项：RegistryDefault、VersionEightMigration、PressOnlyBinding、LiveRemapAndLock、GameModeLifecycle。

首次物理组 `4/1`，SHA-256 `087FF26E98865C4325C59DF25AF83671EBCB324315BD0DA83EC1E6B915A8BA9A`。诊断后确认是测试 World 未满足生产入口的 M01/Controller 发现条件。修复步骤：

1. 为临时 World 创建以 `L_M01_Expedition` 结尾的唯一包；
2. 在无头 World 中显式注册本地 Controller；
3. GC 前清除临时包观察指针，消除 `UObjectArray Index >= 0` 清理断言；
4. 保持生产失败关闭规则不放宽。

最终 GameMode 生命周期 `1/1`，SHA-256 `10EA35A24BFF6C0A2E812D233CA0E54138F2C61272337E4E6731ED0024A5D048`；完整神识树 `48/0`，SHA-256 `B0725D3A00DDAECC19D52B2C3EA81125B511A22BD517C6F7FE4765EDB8DB7638`。

## 7. 首次完整回归失败

`demo_map` 首轮为 `1329 Success / 1 Fail`，SHA-256 `10B989FC5FC9BEEDCDF9415E25A090FA20DBCC58651CA4BEC206CBE0EFF75B7C`。唯一失败 `RewardBossSource.48.PlanEquipmentNotBackpack` 在未改生产代码时可稳定聚焦复现，失败日志 SHA-256 `AD3DF598243C30EE7220799A9F689DAE1AC39E23FC2E03B56E22AB8AB700CD48`。

根因是旧测试 helper 将非背包装备硬编码为 Weapon/Armor/Accessory，遗漏已存在的 Spatial Ring 类别。只把 `SpatialRingCategory` 加入认可集合；未改奖励池或规划器。聚焦复测 `1/1`，SHA-256 `EE978CEB0537E4B6E4576A4E07C3740D6060A61628EC98D01FDCB6B2713D244D`；完整 `demo_map` 复测 `1330/0`。

## 8. 映射回归与构建

21 个当前代码/规则路径命中 10 条映射规则，要求 97 个测试组。两份健康宽树日志覆盖全部要求：

- `Shanmen.0_0_10`：1,260/0，SHA-256 `1967863BFFEE8782F4CAD80ADC1DF67E8C801C5A12A5069F5B0EBA8FFBD0DBEC`；
- `demo_map`：1,330/0，SHA-256 `1774624E3182DDC59137B61A23D5F444840C0D35BD048501F0E549AA1D1FE026`；
- 合计：2,590/0；
- 覆盖门：`PASS Changed=21 Rules=10 Required=97 Logs=2`；
- 门禁自测：`PASS 440/440`。

最终构建：

- Game：55 actions / native 0 / 119.54s；
- Editor：up-to-date / native 0 / 0.98s；
- `demo_map.exe`：359,607,808 bytes / SHA-256 `C2997AFC0EB2C7919E60B45CF41322905D4166789D82E2FC287BD4613F17B47E`；
- `UnrealEditor-demo_map.dll`：18,856,960 bytes / SHA-256 `1CA47E8B0050E99061ED21A90F466F85202E5995D80222B6FE6110E4E1342E56`。

## 9. 静态边界

非文档增量：生产 `+468/-7`，测试 `+572/-20`，回归脚本/映射 `+30/-1`。新增生产代码没有 Timer/SetTimer、自定义 Tick、Sleep、随机数、ApplyDamage、SpawnActor 或 DestroyActor。

`git diff --check` 为 0；无新增资产、模块依赖、Actor、Subsystem、存档字段或第二套神识权威。自动化只启动 `UnrealEditor-Cmd` 无头测试；未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 提交边界与 GitHub

精确提交 8 个生产文件、11 个测试文件、2 个回归规则文件、本 Report 与本 Development Log，共 23 个文件。103 个用户原有 untracked 文件不暂存；所有原始证据保存在 `Saved/Codex/P23.0` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-0-divine-sense-reveal>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-0-divine-sense-reveal/Docs/Report/Dev.D.UE.0.0.10.P23.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-0-divine-sense-reveal/Docs/Log/Dev.D.UE.0.0.10.P23.0.r0_log.md>
