# Dev.D.UE.0.0.10.P27.8.r0 Report

## 1. 结论

P27.8 已把 P27.7 的单次知识权威投影与 P27.6 的阵图选择收束为一个只读、单调用入口。产品调用方现在
只需提交活动阵图目录、Owner 和请求阵图；访问适配器先完成公开目录前置检查，再从一次权威读取产生一份
固定 revision 的知识快照，并且只用该快照完成选择。成功结果把目录、Owner、知识 revision、知识快照与
最终阵图选择逐字段绑定，避免调用方跨目录或跨 revision 手工拼装证据。

本阶段不修改 Profile schema，也不推断“拾取阵图物品即永久学习”、知识撤销、迁移、正式阵图内容、
按键或 UI。它只封闭既有两个契约之间的产品访问接缝。

## 2. 阶段问题与范围

P27.7 已建立进度/存档权威到知识快照的单向读取，但调用方仍需分别调用投影和选择。若目录、Owner、请求
阵图或知识快照在两次调用间由调用方自行替换，P27.6/P27.7 各自正确也无法证明最终选择来自同一次权威
revision。

P27.8 只解决该组合边界：

- 提供一个 `Resolve()` 入口，固定目录检查、一次知识投影和一次选择的顺序；
- 无效目录、无效 Owner、空请求及目录外请求在读取个人知识前失败关闭；
- 保留投影和选择的完整嵌套结果，不把精确失败压缩成布尔值；
- 用调用次数及逐字段交叉校验证明成功结果来自同一目录、Owner 和知识 revision；
- 为新增路径加入改动驱动回归映射和正反自测。

## 3. 有界访问流程

`Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve()` 的顺序固定为：活动目录有效性 → Owner 有效性 →
请求身份有效性 → 阵图是否存在于公开目录 → P27.7 知识投影 → P27.6 阵图选择。

前四项失败时 `KnowledgeProjectionCount` 与 `SelectionInvocationCount` 均为 0。请求阵图存在时，适配器只
调用一次知识投影；投影失败时不调用选择。投影成功后只调用一次选择，不重读权威、不重试，也不缓存回调。
因此一次 `Resolve()` 使用一份不可变知识 revision 完成判断，而不是宣称外部存档事务被锁定。

## 4. 失败证据与隐私前置门

顶层状态区分 `CatalogInvalid`、`OwnerInvalid`、`RequestInvalid`、`DiagramUnknown`、
`KnowledgeProjectionRejected`、`SelectionRejected` 与 `Selected`。目录中不存在的请求无需读取个人知识；
目录已知但该 Owner 未学习的阵图则在一次合法投影后明确返回 P27.6 的 `DiagramUnavailable`。

投影失败保留 P27.7 的精确状态和诊断，包括权威不可用、畸形读取、跨 Owner、目录/内容漂移及非法知识
声明。适配器不伪造替代快照，也不会在错误后回退到本地列表。

## 5. 成功结果交叉约束

`Fdemo_mapShanmenFormationDiagramAccessResult::IsValid()` 对成功结果验证：活动 CatalogId/内容 stamp、请求
Owner、投影知识的 CatalogId/Owner、选择引用的知识 SnapshotId/revision、选择内容 stamp 以及最终阵图
身份必须全部一致。完全相同的权威读取可重放为相同 ReadId、SnapshotId 和 SelectionId。

拒绝结果同样有结构约束：前置拒绝不得携带嵌套证据；投影拒绝必须恰有一次投影、零次选择并保留相同诊断；
选择拒绝必须恰有一次投影和一次选择，且精确为 `DiagramUnavailable`。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationDiagramAccessAdapter` | 4 | 0 | `BCC6170F509FCF368B62825209F7C7E6B19078E7419CF7AB965C816B95EE59DE` |
| `Shanmen.0_0_10.Product.FormationKnowledgeAuthorityAdapter` | 4 | 0 | `E5EF8BD286F61CA018C5C340CBAC0D08C6877011177F4AD4D6623D3F5839CD5A` |
| `Shanmen.0_0_10.Product.FormationDiagramSelection` | 3 | 0 | `6BD9F621CCE15ADD4EA0C621F4FA7F910D486838188846D58C3FC3F11EDC9B53` |
| `Shanmen.0_0_10` | 1,344 | 0 | `64AAEF6D40FB10FDE9401315133AE4252238BBF5844FD4CFE77F87E878BC726C` |

新增四项测试覆盖确定性单次读取/选择、零读取前置拒绝、投影失败阻断选择，以及目录已知但 Owner 未学习
的阵图。完整根组包含新增测试在内共 1,344 项全部成功。四份日志均有队列完成证据，且为 0 Fail、
0 Fatal/Unhandled/Ensure/Assertion；各测试进程原生退出码均为 0。

## 7. 改动驱动回归与流程自测

`Scripts/ShanmenRegressionMap.json` 新增 `FormationDiagramAccessAdapter` 路径规则，要求完整
`Shanmen.0_0_10`、P27.8 访问适配器、P27.7 知识权威适配器及 P27.6 阵图选择四层证据。正向 fixture
验证联合覆盖；只提供 P27.8 焦点日志的负向 fixture 会因缺少依赖层和完整证据而失败。

映射器正反自测 `471/471` PASS；最终覆盖门为：
`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=4`。JSON 解析和 `git diff --check` 均通过。

## 8. 构建与静态边界

| Target | Result / native exit | Final duration |
|---|---|---:|
| `demo_map` Win64 Development | Rebuilt 4 actions, Succeeded / 0 | 46.76s |
| `demo_mapEditor` Win64 Development | Rebuilt 5 actions, Succeeded / 0 | 25.32s |

最终 `demo_map.exe` 为 360,189,440 bytes，SHA-256
`EB5E623EB8236DD82679FD7CD5277DEE608D6685B906EED402B19402082A3933`；
`UnrealEditor-demo_map.dll` 为 19,554,816 bytes，SHA-256
`B89ED9D26AD607D737E7CFD33DF6E24429684075C0097CDBBE851645F0630657`。

生产文件 scoped scan 未发现随机 GUID/RNG、物理输入、World、Actor、PlayerController、Profile、物品子系统
或伤害调用依赖。没有新增正式内容资产或修改地图。

## 9. P/F 边界与下一阶段

P 阶段证明了公开目录前置门、零次无效读取、单次权威投影、单次选择、精确失败传递、同 revision 身份绑定、
确定性重放、完整 0.0.10 回归以及改动驱动覆盖。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或
Package。因此不声明 shipping Profile 已持久化阵图知识，也不声明玩家可通过正式 UI/按键选择阵图。若下一
阶段接入持久知识，仍须先固定获得、撤销与 N−1 迁移语义；若接物理输入，须先固定正式 action/key/UI 规则。

## 10. GitHub 交接

基线提交：`973cd95ff9ebf9cfb6d736ad1102ddd68ca09b4f`（P27.7）。
分支：`agent/0.0.10-p27-8-formation-diagram-access-adapter`。本阶段只提交 3 个实现/测试文件、2 个回归映射
文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.8` 原始
证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-8-formation-diagram-access-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-8-formation-diagram-access-adapter/Docs/Report/Dev.D.UE.0.0.10.P27.8.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-8-formation-diagram-access-adapter/Docs/Log/Dev.D.UE.0.0.10.P27.8.r0_log.md>
