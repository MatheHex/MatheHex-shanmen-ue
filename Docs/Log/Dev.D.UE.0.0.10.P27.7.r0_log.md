# Dev.D.UE.0.0.10.P27.7.r0 Development Log

## 1. 目标

- 在 P27.6 知识快照前建立单向、只读、单次调用的权威适配边界；
- 把 Owner、revision、活动目录、内容 stamp 与已知阵图集合捕获为确定性读取证据；
- 在生成选择可消费知识前拒绝不可用、畸形、跨玩家、陈旧目录和非法声明；
- 不虚构当前 Profile 已拥有的知识字段或“阵图物品即永久习得”规则；
- 完成完整回归、改动驱动覆盖、Report/Log 提交及 GitHub 推送。

## 2. 基线与范围

- 基线：`2a4aee915c3685548294372a55d1fd57c1eb56dc`（P27.6）；
- 分支：`agent/0.0.10-p27-7-formation-knowledge-authority-adapter`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 当前持久 Profile schema 7 没有阵图知识域，本阶段不修改 schema 或迁移；
- 不新增正式阵图、材料、锚点、掉落、配方、效果、数值、按键、UI、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. 权威读取 transport 与证据

新增 `Fdemo_mapShanmenFormationKnowledgeAuthorityCapture`，作为既有/未来进度权威返回 Owner、revision、
CatalogId、内容 stamp 和已知阵图集合的最小 transport。该结构不是权威存储，也不在适配器中持久化。

新增不可变 `Fdemo_mapShanmenFormationKnowledgeAuthorityRead`。捕获时验证身份字段、规范排序知识集合并拒绝
空/重复身份；以全部规范字段派生 `ReadId`。完全重放身份稳定，知识枚举顺序不影响身份。

## 4. 有界投影流程

`Project()` 的顺序固定为：活动目录 → 请求 Owner → 一次权威读取 → 读取证据捕获 → Owner 匹配 →
CatalogId/内容匹配 → P27.6 知识快照捕获。目录或 Owner 无效时读取次数为 0；其它路径最多读取一次。

状态区分 `AuthorityUnavailable`、`AuthorityReadInvalid`、`OwnerMismatch`、`CatalogMismatch` 与
`KnowledgeRejected`。失败结果不携带有效知识；成功结果保持读取证据与知识快照字段完全一致。

## 5. 责任边界

适配器不拥有 Profile、schema、解锁写入、物品、掉落、目录加载、重试、World、Actor、GameMode、输入、
UI 或产品生命周期。阵图知识最终由哪个现有进度权威持久化，以及阵图物品何时转化为知识，仍是待明确的
产品规则；本阶段没有用临时列表或测试 fixture 替代它。

## 6. 测试结果

新增四项 `FormationKnowledgeAuthorityAdapter` 自动化测试：

- `DeterministicSingleReadProjection`；
- `PreflightAndAvailabilityFences`；
- `OwnerAndCatalogFences`；
- `ClaimAndEmptyKnowledgeFences`。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationKnowledgeAuthorityAdapter` | 4 | 0 | `C9A08FD21CF8B96DF0BD5F5D5D834AD81FC6AD0EEFA460D86604396A9F8A4DEF` |
| `Shanmen.0_0_10.Product.FormationDiagramSelection` | 3 | 0 | `60EBEDADD2D72359AB0F2C339D81CB4BECE6DDD30E20E2434C3B17080289BF98` |
| `Shanmen.0_0_10` | 1,340 | 0 | `735BC36818CB6020552DE19D3D8E3449D420D99A011237AC84FA110F1A2BC17A` |

所有最终测试进程原生退出 0，日志为 0 Fail、0 Fatal/Unhandled/Ensure/Assertion，并包含自动化队列完成
证据。完整根组在同一进程串行执行，避免 UE 自动化消息总线交叉污染。

## 7. 回归映射自测

新增路径规则 `FormationKnowledgeAuthorityAdapter`，要求根组、适配器焦点和选择焦点。正向 fixture 验证
三层日志联合覆盖；负向 fixture 证明适配器焦点不能替代选择和完整证据。

- regression map JSON：PASS；
- 映射器正反自测：`469/469` PASS；
- 覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=3 Logs=3`；
- `git diff --check`：PASS。

## 8. 构建与二进制

| Target | Result | Duration |
|---|---|---:|
| `demo_map` Win64 Development | Rebuilt 4 actions, Succeeded / native 0 | 44.52s |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / native 0 | 2.07s |

- `demo_map.exe`：360,172,032 bytes；SHA-256
  `42B036ECDD0A7DA94762E39498C79BCEEA908E4F38A9B74520595D809D6B4568`；
- `UnrealEditor-demo_map.dll`：19,532,800 bytes；SHA-256
  `B20157DC063FDB50B22C62B6E8DB92E7822178CF17EB505707819AFA7A76A884`。

## 9. 静态检查与下一步

生产文件 scoped scan 未出现随机 GUID/RNG、物理输入、World、Actor、GameMode 或测试身份。未修改存档、
物品、内容资产或地图。

下一阶段若继续持久知识，应先固定获得、撤销、revision 与 N−1 schema 迁移语义，再把唯一进度权威接到
本读取端口。未固定这些规则前，不把测试目录或本地临时数组提升为 shipping 真值。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationKnowledgeAuthorityAdapter.h`
2. `Source/demo_map/demo_mapShanmenFormationKnowledgeAuthorityAdapter.cpp`
3. `Source/demo_map/demo_mapShanmenFormationKnowledgeAuthorityAdapterTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.7.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.7.r0_log.md`

`Saved/Codex/P27.7` 原始证据不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-7-formation-knowledge-authority-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-7-formation-knowledge-authority-adapter/Docs/Report/Dev.D.UE.0.0.10.P27.7.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-7-formation-knowledge-authority-adapter/Docs/Log/Dev.D.UE.0.0.10.P27.7.r0_log.md>
