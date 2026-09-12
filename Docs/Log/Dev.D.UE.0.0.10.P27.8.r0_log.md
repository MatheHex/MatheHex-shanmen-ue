# Dev.D.UE.0.0.10.P27.8.r0 Development Log

## 1. 目标

- 将活动阵图目录、一次知识权威投影与阵图选择收束为一个产品访问入口；
- 防止调用方跨目录、跨 Owner 或跨知识 revision 手工拼装选择证据；
- 在读取个人知识前拒绝无效及目录外请求，并保留各层精确失败；
- 不虚构持久知识、物品学习、正式按键、UI 或内容规则；
- 完成完整回归、改动驱动覆盖、Report/Log 提交及 GitHub 推送。

## 2. 基线与范围

- 基线：`973cd95ff9ebf9cfb6d736ad1102ddd68ca09b4f`（P27.7）；
- 分支：`agent/0.0.10-p27-8-formation-diagram-access-adapter`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 当前 Profile schema 7 仍无阵图知识域，本阶段不修改 schema 或迁移；
- 不新增正式阵图、材料、掉落、配方、效果、数值、按键、UI、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. 访问结果契约

新增 `Fdemo_mapShanmenFormationDiagramAccessResult`，记录活动 CatalogId/内容 stamp、请求 Owner/阵图、知识
投影调用次数、选择调用次数以及两层完整嵌套结果。状态明确区分前置拒绝、目录外请求、知识投影拒绝、选择
拒绝和成功选择。

`IsValid()` 对成功证据逐字段核对目录、内容、Owner、知识 SnapshotId/revision 及阵图身份；对失败结果
约束允许的调用次数和嵌套证据。投影或选择诊断原样保留，不降格成模糊布尔失败。

## 4. 有界组合流程

`Resolve()` 先验证活动目录、Owner、请求身份和请求是否属于公开目录。这些前置条件失败时，知识权威读取与
选择次数均为 0。通过后恰好调用一次 P27.7 `Project()`；失败即终止。成功后恰好调用一次 P27.6
`Select()`，全程不重读、不重试、不缓存回调。

目录中已知但 Owner 未学习的阵图保留合法知识投影，并精确返回 P27.6 `DiagramUnavailable`；目录外阵图
在读取个人知识前返回 `DiagramUnknown`。该流程只在单次调用中固定一份不可变 revision，不宣称锁定外部
存档事务。

## 5. 责任边界

适配器不拥有目录加载、Profile、schema、解锁写入、物品、掉落、重试、World、Actor、GameMode、输入、
UI 或部署生命周期。阵图知识的获得/撤销/迁移规则以及正式阵图内容仍待产品冻结，本阶段没有以 fixture 或
临时列表替代 shipping 真值。

## 6. 测试结果

新增四项 `FormationDiagramAccessAdapter` 自动化测试：

- `DeterministicSingleReadSelection`；
- `PreflightDoesNotReadAuthority`；
- `KnowledgeFailureStopsSelection`；
- `AuthoredButUnavailable`。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationDiagramAccessAdapter` | 4 | 0 | `BCC6170F509FCF368B62825209F7C7E6B19078E7419CF7AB965C816B95EE59DE` |
| `Shanmen.0_0_10.Product.FormationKnowledgeAuthorityAdapter` | 4 | 0 | `E5EF8BD286F61CA018C5C340CBAC0D08C6877011177F4AD4D6623D3F5839CD5A` |
| `Shanmen.0_0_10.Product.FormationDiagramSelection` | 3 | 0 | `6BD9F621CCE15ADD4EA0C621F4FA7F910D486838188846D58C3FC3F11EDC9B53` |
| `Shanmen.0_0_10` | 1,344 | 0 | `64AAEF6D40FB10FDE9401315133AE4252238BBF5844FD4CFE77F87E878BC726C` |

四个最终测试进程原生退出 0；日志为 0 Fail、0 Fatal/Unhandled/Ensure/Assertion，并包含各自队列完成
证据。完整根组在一个进程内串行执行。

## 7. 回归映射自测

新增 `FormationDiagramAccessAdapter` 路径规则，要求根组、P27.8 访问焦点、P27.7 知识焦点和 P27.6 选择
焦点。正向 fixture 验证四层联合证据；负向 fixture 证明访问焦点不能替代依赖层和完整回归。

- regression map JSON：PASS；
- 映射器正反自测：`471/471` PASS；
- 覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=4`；
- `git diff --check`：PASS。

## 8. 构建与二进制

| Target | Result | Duration |
|---|---|---:|
| `demo_map` Win64 Development | Rebuilt 4 actions, Succeeded / native 0 | 46.76s |
| `demo_mapEditor` Win64 Development | Rebuilt 5 actions, Succeeded / native 0 | 25.32s |

- `demo_map.exe`：360,189,440 bytes；SHA-256
  `EB5E623EB8236DD82679FD7CD5277DEE608D6685B906EED402B19402082A3933`；
- `UnrealEditor-demo_map.dll`：19,554,816 bytes；SHA-256
  `B89ED9D26AD607D737E7CFD33DF6E24429684075C0097CDBBE851645F0630657`。

## 9. 静态检查与下一步

生产文件 scoped scan 未出现随机 GUID/RNG、物理输入、World、Actor、PlayerController、Profile、物品子系统
或伤害调用依赖。未修改存档、物品、内容资产或地图。

下一阶段不应直接把本访问入口接到临时按键或临时存档字段。持久知识方向需先冻结获得、撤销、revision 与
N−1 迁移语义；物理输入方向需先冻结正式 action/key/UI 规则，然后才能接入现有输入注册表。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationDiagramAccessAdapter.h`
2. `Source/demo_map/demo_mapShanmenFormationDiagramAccessAdapter.cpp`
3. `Source/demo_map/demo_mapShanmenFormationDiagramAccessAdapterTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.8.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.8.r0_log.md`

`Saved/Codex/P27.8` 原始证据不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-8-formation-diagram-access-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-8-formation-diagram-access-adapter/Docs/Report/Dev.D.UE.0.0.10.P27.8.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-8-formation-diagram-access-adapter/Docs/Log/Dev.D.UE.0.0.10.P27.8.r0_log.md>
