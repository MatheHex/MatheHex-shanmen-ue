# Dev.D.UE.0.0.9BFix3.P1 Report

## 1. 结果

`READY_FOR_F0_RERUN`

F0-001 已在现行生产物理投影链内作窄范围修复。最终 `git diff --check`、`demo_mapEditor Win64 Development` 与 `demo_map Win64 Development` 均通过，最终 native exit code 均为 `0`。本轮未启动产品或执行任何 F 阶段验证。

## 2. 修改与未修改范围

修改：

- `Source/demo_map/demo_mapV3ProgressionManager.cpp`
  - 新增唯一 BasicCache 的稳定 anchor-local 偏移常量 `(0,-1600,0)`。
  - `InitializeCodeBNormalContainerTarget` 改用该常量；目标 identity、anchor identity、安全投影 helper、候选过滤和 spawn/registration 分支均未改变。
- `Docs/Prompt/Dev.D.UE.0.0.9BFix3.P1_prompt.md`
  - 归档策划部真实下发文件，内容未改写。
- `Docs/Report/Dev.D.UE.0.0.9BFix3.P1_report.md`
  - 本报告。

未修改：M01 map asset、`demo_mapItemSubsystem` 通用投影器、NormalContainer Actor 类、Code B Repository/Store、P1/P5/P6/P9/P18/P31/P49/P50、receipt/history、P13/P15/P17、存档 schema、Code A Run/input/HUD/combat/enemy/reward/terminal authority。

## 3. F0-001 事实与根因

F0 已在真实 PIE 与 Standalone 中共同复现：

- target：`M01.CodeBNormalContainer.BasicCache.01`
- anchor：`M01.Resource.TIER_1.Cluster.01`
- 首次异常：`CODEB_P10_BASIC_CACHE: no safe projection ...`
- PIE：`Saved/Logs/demo_map.log:2268,2270,2280`
- Standalone：`Saved/Logs/demo_map_2.log:1807,1809,1819`

静态审计确认失败发生于 Actor spawn、P10 打开以及任何 P9 materialization/P18 roll/P31 record/P49/P50 transaction 之前。

根因是生产布局冲突，而非 durable 或物品事务错误：

1. `Fdemo_mapM01RewardDistribution` 在同一 Tier-1 anchor 周围声明 24 Wood + 24 Ore，共 48 个 SearchContainer；其 `GridOffset` 为 8×6、240uu 间距，局部范围为 X `[-840,840]`、Y `[-600,600]`。
2. P10 原始局部偏移 `(260,-220,0)` 位于该密集网格内部。
3. `ResolveSafeWorldLocation` 对 footprint 64uu 的 BasicCache 与已有 container footprint 66uu 加 28uu padding，要求至少 158uu 的二维间距，并只在原点及 82/128/178uu 三个稳定环上检查 25 个候选。
4. M01 先完成 48 个 Tier-1 等 reward container 投影，再初始化 BasicCache；因此上述候选继续落在既有 SearchContainer 网格覆盖区，全部被通用容器间距规则拒绝，形成 F0 的确定 `no safe projection`。

## 4. 修复前后活动调用链

修复前后共有链路均为：

1. 正常 Run 激活调用 `ActivateV3MissionContentForRun`，随后进入 `InitializeWorldContent`。
2. M01 先通过 `InitializeM01RewardContent` 解析静态 resource anchors、导航投影并生成既有 reward containers。
3. `InitializeCodeBNormalContainerTarget`：
   - 若当前 manager 已持有有效 exact target，直接复用；
   - 否则枚举 pre-authored `Ademo_mapCodeBNormalContainerActor`，只接受 exact identity；第二个 exact identity 立即结构化失败；
   - 若没有预置 Actor，解析 exact anchor，计算稳定 Desired，调用 `ResolveSafeWorldLocation`；
   - helper 依次执行稳定候选、Visibility ground trace、坡度、world-item/container separation、object overlap；通过后才 `AlwaysSpawn`；失败时零 Actor 创建；
   - spawn 后再次校验 exact identity，再记录唯一弱引用和 `bCodeBNormalContainerTargetSpawned=true`。
4. Actor 只暴露 prompt、interaction location、range/timer 与瞬时 intent。Code A focus/input 经 `RequestInteractFocused` 转交 `RequestCodeBNormalContainerInteract`；P10 从 static identity 派生 `SearchTargetId` 后才进入 Code B Store。
5. `DestroyRuntimeContainers`、terminal settlement、profile deactivation 和 manager EndPlay 关闭页面/动作，只销毁 manager-spawned Actor，清空 target 弱引用；Actor EndPlay 同时中断其精确 pending action，避免 stale adapter/timer。

唯一行为差异：Desired 从网格内部的 `(260,-220,0)` 改为同一 anchor 的 `(0,-1600,0)`。其余链路完全保持。

## 5. 生产投影方案与 duplicate 排除

选择既有 **anchor safe projection**，没有新增 pre-authored Actor 或 map fixture。理由：根因是动态生产偏移与既有 reward grid 冲突；同一 helper、ground/collision contract 与 spawn/teardown 已正确，只需把该唯一生产目标的声明位置移出密集网格。

pre-authored 路径仍是互斥优先分支：exact Actor 存在时立即返回，不执行动态 spawn；出现两个 exact Actor 时失败。只有 exact Actor 数为零时才进入 anchor projection，因此不会并行产生第二 Actor、prompt 或 registration。

## 6. Physical placement 与失败语义

- 稳定来源：exact anchor transform `M01.Resource.TIER_1.Cluster.01` + 固定 local offset `(0,-1600,0)`。
- 当前 map generation 声明的 anchor 为 `(-4300,2900,60)`，对应 Desired `(-4300,1300,60)`；该点位于开放的 M01 low-tier ground，已离开 Tier-1 8×6 reward grid。
- candidate 顺序：Desired 优先；如受阻，继续使用 `M01.CodeBNormalContainer.BasicCache.01` 的稳定 CRC phase 与固定 82/128/178uu 环，不使用随机数、帧序、Actor pointer 或 UI 状态。
- ground：Visibility channel 线段检测、`ImpactNormal.Z >= 0.65`，最终抬高 36uu。
- clearance：既有 world item/container 间距、reserved location、WorldStatic/WorldDynamic overlap 与组件阻挡响应继续有效。
- spawn：仍只在 helper 返回合法位置后执行；`AlwaysSpawn` 不绕过前置安全验证。
- 失败：仍输出结构化 `no safe projection`，且零 target/P9/item/receipt/record；不会重定向到玩家、其他资源、旧 Code A Loot 或无关坐标。

## 7. Identity、authority 与生命周期审查

- static target identity 仍为 `M01.CodeBNormalContainer.BasicCache.01`；anchor provenance 仍为 `M01.Resource.TIER_1.Cluster.01`。
- `SearchTargetId` 仍仅由 P10 static identity 的固定 CRC 字段推导。
- Actor 不写 P1/P5/P6/P9/P31，不创建 item、receipt、loot roll、graph 或 WorldDrop。
- P9 首次 materialization 仍只能发生在正常 P10 open/search 路径；本修复不预建内容。
- P18 r2 history/optional spatial utility、P31 WorldDrop、P49/P50 drop、P13/P15/P17 graph 与所有既有 revision/receipt 逻辑未改变。
- 重复初始化最多复用一个 manager-held exact Actor；pre-authored discovery 拒绝 duplicate；dynamic branch只在零 exact Actor 时执行。
- teardown 仅销毁 manager-spawned Actor，并清理 pending action/weak target；未增加 stale registration 或幽灵 prompt 分支。
- Code A Run、地图、输入、终局以及 M01 enemy/reward authority未改变；map diff 为零。

## 8. 静态检查与编译

- `git diff --check`：通过，native exit code `0`。
- Editor 命令：

  `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

  首次调用在源码编译前被已有 `demo_map` Editor Live Coding 会话拒绝，native exit code `1`；项目 Editor 随后通过正常 `CloseMainWindow` 退出，未强制终止。相同命令重跑成功，最终 native exit code `0`，`demo_mapV3ProgressionManager.cpp` 编译、Editor lib/dll 链接与 metadata 写入完成。

- Game 命令：

  `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

  成功，native exit code `0`；`demo_mapV3ProgressionManager.cpp` 编译、`demo_map.exe` 链接与 metadata 写入完成。

## 9. 明确未执行的 F 阶段项目

未执行：产品启动、Editor Play、PIE、Standalone、正常部署、BasicCache 真实可见性、`[G]`、P10 open/reveal/search、P49/P50 drop/pickup、P31 multi-record isolation、保存/重开、拒绝路径动态验证、真实鼠标键盘、截图、自动化、回归、Smoke、试玩、Cook、Package、最终验收。

上述项目只能由后续独立 F0 rerun 决定。本报告不把编译或静态几何审查伪装成运行通过。

## 10. 治理确认

- 未读取、引用或采用废止的 `0.2 final report.docx`、任何 0.2/V2/V3/旧规则。
- 未创建 fixture、假目标、第二 BasicCache、第二库存、debug spawn、随机 fallback、直接 P6 grant 或 UI injection。
- 未修改物品数量、掉落内容、地图玩法、敌人、奖励、经济、存档或 P10/P18/P49/P50 产品语义。
- 未自行开始 F0 重测、F1、Fix3.P2、常规 P、自动化或其他任务。

`READY_FOR_F0_RERUN`
