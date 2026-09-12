# Dev.D.UE.0.0.10.P27.7.r0 Report

## 1. 结论

P27.7 已在 P27.6 阵图知识快照之前建立单向、只读的知识权威适配器。产品调用方可以向既有或未来的
进度/存档权威发起一次 Owner 定向读取；适配器把返回值捕获为内容与目录绑定的不可变读取证据，并且只有
在 Owner、revision、目录身份、内容 stamp 和知识声明全部通过验证后，才生成 P27.6 可消费的知识快照。

当前持久 Profile schema 7 没有阵图知识域，本阶段因此没有伪造一份隐性存档真值，也没有擅自决定
“获得阵图物品是否等于永久习得阵图”。它只建立权威读取与选择契约之间的正式接缝，不修改存档 schema、
物品权威、掉落、配方、正式阵图、效果、数值、输入或 UI。

## 2. 阶段问题与范围

P27.6 已能验证一份调用方提供的知识快照，但尚未约束快照如何从权威侧读取。若每个调用方自行拼装
Owner、revision 和已知阵图列表，就会形成不可审计的第三套知识真值。

P27.7 只解决读取与投影边界：

- 定义权威返回的最小 transport capture；
- 将一次返回值捕获为确定性、内部自洽的不可变读取证据；
- 有效请求最多调用知识权威一次，无效前置条件不调用；
- 在投影前拒绝跨 Owner、陈旧目录、内容漂移、重复身份和目录外声明；
- 把成功结果直接组合到 P27.6 选择端口；
- 为新增路径加入改动驱动回归映射及正反自测。

## 3. 单次权威读取契约

`Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project()` 先验证活动目录与请求 Owner。任一前置条件
无效时返回显式状态，`AuthorityReadCount` 保持 0。前置条件成立后只调用一次注入的 `FReadAuthority`；
读取失败、返回空诊断或返回畸形数据都不会触发重试，也不会产生有效知识快照。

适配器是单向只读端口。它不保存回调、不写回权威、不修改 Profile、库存或解锁状态，也不拥有重试队列、
World、Actor、GameMode、输入、UI、目录加载或产品生命周期。

## 4. 不可变读取证据与确定性

`Fdemo_mapShanmenFormationKnowledgeAuthorityRead::TryCapture()` 验证 Owner、非负 revision、CatalogId、内容
stamp 和知识身份。已知阵图身份按名字规范排序；空集合合法，空身份和重复身份失败关闭。

`ReadId` 由 Owner、revision、CatalogId、内容版本/digest 和完整规范知识集合确定性派生。相同权威返回值
可重放为相同 `ReadId`，输入枚举顺序不会改变身份；Owner、revision、目录、内容或集合变化都会改变身份。
该证据证明字段内部一致性，不宣称具有密码学认证能力；外部权威归属仍由产品组合层负责。

## 5. Owner、目录与知识投影围栏

读取证据形成后，适配器依次验证请求 Owner、活动 CatalogId 与内容 stamp。跨玩家证据返回
`OwnerMismatch`；旧目录或内容漂移返回 `CatalogMismatch`。两者均保留一次有效读取证据用于诊断，但不
产生知识快照。

通过身份围栏后，适配器调用 P27.6 的
`Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture()`。目录外阵图声明在此返回
`KnowledgeRejected`；空知识集合可成功投影，但通过 P27.6 选择端口选择任意阵图时仍明确返回
`DiagramUnavailable`。成功投影绑定同一 Owner、revision、目录、内容和规范知识集合。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationKnowledgeAuthorityAdapter` | 4 | 0 | `C9A08FD21CF8B96DF0BD5F5D5D834AD81FC6AD0EEFA460D86604396A9F8A4DEF` |
| `Shanmen.0_0_10.Product.FormationDiagramSelection` | 3 | 0 | `60EBEDADD2D72359AB0F2C339D81CB4BECE6DDD30E20E2434C3B17080289BF98` |
| `Shanmen.0_0_10` | 1,340 | 0 | `735BC36818CB6020552DE19D3D8E3449D420D99A011237AC84FA110F1A2BC17A` |

新增四项测试覆盖确定性重放与单次读取、前置条件/权威不可用、跨 Owner/目录漂移、重复/目录外声明及空
知识集合。完整根组包含新增测试在内共 1,340 项全部成功。三份日志均包含队列完成和原生退出码 0，且为
0 Fail、0 Fatal/Unhandled/Ensure/Assertion。

## 7. 改动驱动回归与流程自测

`Scripts/ShanmenRegressionMap.json` 新增 `FormationKnowledgeAuthorityAdapter` 路径规则，要求完整
`Shanmen.0_0_10`、适配器焦点组和 P27.6 选择焦点组三层证据。只提供适配器焦点日志的负向 fixture 会
因缺少选择及完整证据而失败。

映射器正反自测 `469/469` PASS；最终覆盖门为：
`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=3 Logs=3`。JSON 解析和 `git diff --check` 均通过。

## 8. 构建与静态边界

| Target | Result / native exit | Final duration |
|---|---|---:|
| `demo_map` Win64 Development | Rebuilt 4 actions, Succeeded / 0 | 44.52s |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 2.07s |

最终 `demo_map.exe` 为 360,172,032 bytes，SHA-256
`42B036ECDD0A7DA94762E39498C79BCEEA908E4F38A9B74520595D809D6B4568`；
`UnrealEditor-demo_map.dll` 为 19,532,800 bytes，SHA-256
`B20157DC063FDB50B22C62B6E8DB92E7822178CF17EB505707819AFA7A76A884`。

生产文件 scoped scan 未发现 `FMath::Rand`、`FGuid::NewGuid`、`EKeys`、`InputAction`、`UWorld`、
`AActor`、`demo_mapGameMode` 或测试 fixture 身份。没有新增正式内容资产或修改地图。

## 9. P/F 边界与下一阶段

P 阶段证明了单次读取、零次前置失败读取、确定性读取身份、Owner/目录/内容围栏、知识声明验证、空集合
语义、P27.6 选择组合、完整 0.0.10 回归及改动驱动覆盖。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook
或 Package。因此不声明正式存档已经持久化阵图知识，也不声明玩家获得阵图物品后已经自动习得。下一阶段
若要写入持久知识，必须先明确知识获得/撤销/迁移规则，再由唯一进度权威扩展 schema 并通过本适配器读取；
不得让物品、UI 或阵法调用方各自维护解锁列表。

## 10. GitHub 交接

基线提交：`2a4aee915c3685548294372a55d1fd57c1eb56dc`（P27.6）。
分支：`agent/0.0.10-p27-7-formation-knowledge-authority-adapter`。本阶段只提交 3 个实现/测试文件、2 个
回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.7`
原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-7-formation-knowledge-authority-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-7-formation-knowledge-authority-adapter/Docs/Report/Dev.D.UE.0.0.10.P27.7.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-7-formation-knowledge-authority-adapter/Docs/Log/Dev.D.UE.0.0.10.P27.7.r0_log.md>
