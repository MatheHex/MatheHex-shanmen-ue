# Dev.D.UE.0.0.10.P28.37.r0 Report

## 1. 结论与真实缺口

本轮补齐来源计划的新定义目录接纳：同一物品权威、同一候选快照、同一次保存内，接受完整来源计划及其中尚不存在的定义。已有定义只能完全一致地复用，不能通过奖励计划修改既有物品能力。

P28.35已构建产品候选，P28.36已提供明确三态读取；但迁移目录只带入原持有定义，原AcceptGeneratedSource要求计划里的定义已经全部存在。新增稀疏目录夹具用真实M01/P8计划复现拒绝，来源恢复/接收链因而不能仅靠切换Manager调用点闭合。不是本轮运行了玩家流程。

结论：`P28_37_SOURCE_CATALOG_ADMISSION_READY`。目录一致性接纳子项通过；不是首次来源授权、产品生成可用或整体冻结。

## 2. 实现、唯一权威与授权边界

唯一生产改动：[RepositoryGeneratedSource.cpp](../../Source/ShanmenItems/Private/ShanmenItemRepositoryGeneratedSource.cpp)，13行新增。通过原有Owner/Run/内容/重放/终局检查后，在局部Candidate中逐项处理计划定义：已有则完全相等才复用，缺失则加入；随后仍走来源/请求回执、全状态验证与原AuthorityService持久化/恢复。

- 计划内部同ID不一致、无效定义、已有目录定义冲突均拒绝；先暂加新定义、后遇到错误也不污染原状态。
- 相同新定义出现在多个槽位，只加入一次。没有额外目录登记命令、第二份目录/数量权威或先保存目录后保存来源的中间状态。
- 未改来源ID、指纹、schema4、codec或旧文档加载校验；已保存来源若缺目录/定义漂移仍拒绝，不在加载时悄悄修补。
- 只扩充定义目录，不把来源条目插入Items/Containers，不表示玩家已拿取，不绕过AcquiredItemMismatch或ItemNotPrepared。

该规则沿用既有撤离导入的“缺失定义加入、已有定义不可漂移”语义。这里证明的是领域一致性，**不是产品来源授权**：通用领域请求仍由可信宿主负责构造；当前Subsystem/Manager没有接入，不能把任意外部输入直接传给此端口。首次manifest授权、canonical内容身份与合法产品入口仍是FZ-1必须闭合的门。

## 3. 可重复证据与测试边界

新增3个注册用例，扩展原并发用例：

| 用例 | 本轮覆盖 |
|---|---|
| Catalog.DurableAdmissionAndRestart | 四条来源条目含两个新定义及重复定义槽；目录2→4，只推进一次修订/保存代次；原物品/容器/预留/迁移证据保持；重启、无写重放、后一来源复用、终局重开；删除/篡改目录后加载拒绝 |
| Catalog.AtomicFailureAndGuards | 已有非零来源/保底基准；8种非法请求保持全文档与磁盘字节；4个写前失败回滚；替换后回读失败只在重开证明目录与来源一并成功后返回成功 |
| Catalog.CanonicalProductAdmission | 一个真实携入训练飞剑定义的稀疏目录，经Reserve/StartPreparedRun取得实际Run；接收一个M01和一个P8原规划器候选，目录扩展但物品图不增加；此测试使用内存Repository，持久证明由上两项承担 |
| 既有DurableConcurrentAcceptance | 八个线程提交同一含新定义来源：一次持久接受、七次重放，只增加两个定义；随后读取完整同一原计划 |

修复前同一3项专项为0 Success/3 Fail，正常结束队列3、原生退出0；共10条最终Expected断言失败，均围绕尚未接纳的新定义。测试不更改而只增加上述13行后，专项3 Success/0 Fail。不得将旧实现的“拒绝”夸大成已经发生库存丢失，也不以进程退出0掩盖实际失败。

## 4. 构建与回归

| 本轮检查 | 结果 | 证据边界 |
|---|---|---|
| Editor / Game | 4 / 5 actions，原生0 / 0 | 实际Development Win64编译 |
| CatalogFocused | 3 Success / 0 Fail，队列3、原生0 | 同一测试由修复前0/3转为通过，包含于Items |
| ItemsRegression / Recovery1 | 111 Success / 0 Fail，队列111、原生0 | 完整物品组，含新增3项与并发去重 |
| 两武器内容 / Recovery1 | 各1 Success / 0 Fail，队列各1、原生0 | 遵守产品候选测试文件的共享映射 |
| LegacyFullRoot / Recovery1 | 1330 Success / 0 Fail，队列1330、原生0 | 完整旧demo_map根 |
| 改动映射 / 自检 | Changed=6、Rules=3、Required=7、Logs=4；549/549 | 不以主题或仅专项裁剪回归 |

独立成功1443=111+1+1+1330，专项3项不重复计数；1628输入和103用户文件保持。没有当前完整Shanmen根结果，P28.34.r1完整双根只作为历史。

Initial物品组原生0但仅110项结果、无结束队列，未计完整通过；保留原件后，确认本机UE源码支持逐行日志刷新，以同一构建/输入加入FORCELOGFLUSH重跑得到完整结果。不声称已经查明缺尾段根因。七份测试日志各有13条既存启动Condition failed Error；成功证据无注册Fail/Fatal/Ensure/Unhandled Exception/Assertion failed，Red保留其3项失败。25条原件路径/SHA及异常过程见 [Development Log](../Log/Dev.D.UE.0.0.10.P28.37.r0_log.md)。

## 5. 内存与容量

复用原有整份Candidate复制，新增遍历计划条目的目录查找/缺失插入；只永久存入不同的新定义，不额外复制一套目录或缓存。目录因此会随实际接纳内容增长；现有来源4096、条目总数65536及文档64MiB上限保持，不能把文件上限解释为RAM上限。

专项首个来源4条目、目录2→4，完整序列化文档9619字节；这是该夹具测量，不是最大容量、工作集峰值或目标硬件性能。全快照复制/序列化、历史增长和未来UI查询成本仍需最终输入与F阶段实测，不裁剪重放历史换取表面内存下降。

## 6. 未闭合项与交接

本轮关闭“来源计划缺失定义不能在原事务中接纳”的子项；仍需首次来源内容授权、Subsystem/Manager查询及接受路由、原计划物化失败续接、获物/消费/遗失与Extraction/Death/Abandon组合。FZ-2剩余生命周期入口审计继续，FZ-3最终完整根与双构建等FZ-1/2关闭后执行，见 [冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)。不是整体冻结，不暂停为完成。

本轮仅3个Source（1生产、2测试）及Report/Log/索引，共6路径。原103个未跟踪用户文件和两份OverallReadiness修改保持且不暂存。遵守 [P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：没有物理输入、地图/正式内容、玩法/UI改动，没有Editor UI、PIE、Standalone、游戏程序、截图、Smoke、Cook、Package。
