# Dev.D.UE.0.0.9B.P2.0.r0 Report

## 结论

`READY_FOR_CODE_B_STASH_LOADOUT_UI`

P2.0.r0 已完成 Code B 玩家配置、仓库/基础储存、装备槽、空间道具内部容器的非运行时 vertical slice。P1 Repository 仍是唯一可变权威；Projection 只读；所有修改由单一 Application Service/Command Adapter 翻译为 P1 原子事务。23 个自动化测试全部通过，Editor/Game 构建和默认地图 Smoke 全部通过。

## 任务身份与边界

- 项目：`Dev.D.UE.0.0.9B`
- Task：`Dev.D.UE.0.0.9B.P2.0.r0`
- 引擎：`C:\Program Files\Epic Games\UE_5.8`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- P2 Prompt：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Prompt\Dev.D.UE.0.0.9B.P2.0.r0_prompt.md`
- 活动 Code B：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Source\demo_map\CodeB`
- 对照基线：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`
- 状态：`READY_FOR_CODE_B_STASH_LOADOUT_UI`

P2 仅完成配置、仓库、装备与 Projection/Application Service 的非运行时垂直切片。没有把 Code B 接入玩家运行时、UI、Profile、Run、SaveGame、Loot 或正式地图流程；Code A 继续提供当前默认地图运行时路径，没有 A/B 双写。

## 实现内容

### P1 Repository 的最小扩展

- `Source\demo_map\CodeB\demo_mapCodeBInventory.h/.cpp`
  - 为空间物品增加确定性 child container 关联。
  - 支持以固定 ID 创建容器并校验重复、循环和唯一所有权。
  - 支持 `INDEX_NONE` 的首个空槽自动放置。
  - 对无空位返回 `TargetFull`，继续保证失败不改变 snapshot/revision。
  - P1 原子 Move/Split/Equip/Unequip 继续由 Repository 执行。

### P2 fixture、Projection 与 Application Service

- `Source\demo_map\CodeB\demo_mapCodeBP2.h/.cpp`
  - 建立稳定的 `FCodeBP2PlayerLayout`、`FCodeBP2FixtureIds` 和确定性 fixture。
  - 建立只读 `FCodeBP2Projection`、容器视图和槽位视图。
  - 建立单一 `FCodeBP2ApplicationService`，统一把 P2 command 映射到 P1 Repository 事务。
  - 每次返回 P1 result、受影响 ItemId 与新的只读 Projection。
  - 空间道具内部容器非空时，加载中的空间道具移动显式返回 `LoadedSpatialItemMoveUnsupported`，不会绕过 P1 或破坏父子容器不变量。

- `Source\demo_map\CodeB\demo_mapCodeBP2Tests.cpp`
  - 增加 19 个确定性 P2 自动化测试，并覆盖 P1 回归。

## Fixture 规格

布局使用确定性 ContainerId/ItemId（由固定 `FGuid` 命名策略生成），包含：

| 区域 | 容量/槽位 | 作用 |
|---|---:|---|
| Warehouse/Stash | 30 | 仓库与基础装备来源 |
| Basic | 6 | 基础储存 |
| Weapon | 1 | 武器装备槽 |
| Armor | 1 | 护甲装备槽 |
| Spatial | 1 | 空间道具槽 |
| Accessory 0/1 | 1 + 1 | 两个饰品槽 |
| Spatial Internal | 4 | 空间道具子容器 |

Fixture 注册 Weapon.A/B、Armor.Robe、Accessory.A/B、Spatial.Pouch、Material.Dust、Consumable.Potion、Generic.InvalidEquip；同时设置数量上限，仓库内放入确定性武器、护甲、饰品、空间道具、材料、药剂和一个不兼容装备定义。空间道具与内部 4 格容器通过 Repository setup API 关联。

## Authority、Projection 与命令边界

P1 Repository 是唯一 mutable authority。P2 Projection 仅复制稳定值：Revision、LayoutId、容器 role/ID、槽位 ID/索引、ItemId、DefinitionId、quantity、level、quality、type；不暴露可变引用，不维护第二份物品真相，并在构建时校验重复 ItemId、容器槽位和空间子容器不变量。

P2 Application Service 是唯一 P2 变更入口，覆盖：

- stash ↔ basic Move；
- equip、unequip、replacement；
- Split、Merge；
- stash/basic ↔ spatial internal；
- occupied、full、type mismatch、source mismatch、stale revision；
- 空间道具内部已有物品时的 loaded-spatial move unsupported。

成功事务递增 Revision 并返回 affected IDs；失败事务保持原快照与 Revision。固定重放中所有命令均经过该入口。

## 自动化结果

自动化日志：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\Dev.D.UE.0.0.9B.P2.0.r0_automation.log`

- 总测试：`23`
- 成功：`23`
- 失败：`0`
- `LogAutomationController: Error:`：`0`
- P1 回归：4 项原有测试全部通过。
- P2 新增 19 项：
  - `FixtureIdentity`
  - `ProjectionStable`
  - `StashBasicRoundTrip`
  - `WeaponEquip`
  - `WeaponReplacement`
  - `OtherEquipment`
  - `IncompatibleEquipFailure`
  - `Split`
  - `Merge`
  - `SpatialInternalRoundTrip`
  - `LoadedSpatialMoveUnsupported`
  - `OccupiedTargetFailure`
  - `FullTargetFailure`
  - `SourceMismatch`
  - `StaleRevision`
  - `RevisionAndAffectedIds`
  - `ProjectionImmutable`
  - `CompleteConfigureReorganizeChain`
  - `Replay100`

日志中的 P1 随机回归：`CodeB random seed=20260805 attempts=1000 success=181 legal_failures=819`。

日志中的 P2 固定重放：`CodeB P2 replay seed=20260805 rounds=100 successes=2000 failures=300`。

## 构建与 Smoke

以下构建均以 UE5.8、Win64 Development 完成，退出码 `0`：

- `demo_mapEditor Win64 Development`
- `demo_map Win64 Development`
- 游戏二进制：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Binaries\Win64\demo_map.exe`

默认地图 Smoke 日志：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\Dev.D.UE.0.0.9B.P2.0.r0_smoke.log`

- 地图：`/Game/M01/Maps/L_M01_Expedition?Name=Player`
- 进程退出：存在 `LogExit: Exiting.`
- Fatal：`0`
- Automation errors：`0`
- 新阻断 Error：`0`

Automation 启动后 UE 对当前机器未安装的可选 LinuxArm64/VisionOS SDK 输出了已知平台验证诊断；Win64 目标构建与 Smoke 均成功，该诊断不阻断本任务。

## A/B 与基线审计

- Code A 保持正式运行时基线，P2 未接管运行时玩家/背包/UI/持久化。
- Code B 只位于 `Source\demo_map\CodeB`，P1/P2 通过同一 Repository 事务链工作。
- 对照基线 `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1\Source\demo_map\CodeB` 不存在。
- `demo_mapItemAuthority.cpp` 活动工程与 XFix1 SHA256 均为 `4FC5BEC5584AB4F7907EAE573181DBE61FF7991694FA413B90F6F1676573F4E4`。
- XFix1 源保持未修改；隔离副本继续保留。

## 交付与下一步

已更新：

- `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\PROJECT.md`
- `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\PROJECT_INFO_CARD.md`

下一单一任务建议：在明确 reviewed boundary 后，开始 Code B stash/loadout UI 的展示与交互接入；继续以 P1 Repository 为唯一写入权威，不自动扩展到正式运行时、Profile、Run 或 SaveGame。

非阻断发现：当前尚无正式 UI/runtime/Persistence 接入，这是本 P2 范围外事项；UE 可选平台 SDK 验证诊断不影响 Win64 构建与 Smoke。
