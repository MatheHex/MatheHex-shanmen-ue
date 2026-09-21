# Dev.D.UE.0.0.10.P28.31.r0 Report

## 1. 结论与范围

`P28_31_DOMAIN_CONTRACT_COMPLETE`。2026-09-21 UTC，从 `8b00869fec92f1c78029406faa8c33a8b65067e3` 继续 P28.30 第1项：新增 ShanmenItems 的有界生成来源领域契约和测试。只实现候选计算/结构验证/精确重放判断，**没有持久接收命令，没有接通产品奖励生成，不是 FZ-1/2 关闭或最终冻结**。

新增三个源码文件：[接口](../../Source/ShanmenItems/Public/ShanmenItemGeneratedSource.h)、[实现](../../Source/ShanmenItems/Private/ShanmenItemGeneratedSource.cpp)、[测试](../../Source/ShanmenItems/Private/Tests/ShanmenItemGeneratedSourceTests.cpp)。不修改 Repository、AuthorityService、snapshot/schema、旧写入隔离、产品调用或内容资产；不新增第二份可变库存、奖励账本或旁路存档。

## 2. 信息分区与契约

| 输入/输出 | 本轮行为 | 不代表什么 |
|---|---|---|
| GeneratedSourcePlan | 保留 Owner/Run/稳定来源、来源 manifest、投影/分布/预算/事件策略身份、seed、预算值、保底前后值、有序定义/数量/section/slot/奖励元数据和子容器规格 | 不执行生成、目录查找、价格计算或玩法策略 |
| GeneratedSourceView | 只借用所属 Run 的状态、来源内容身份、接收序号和当前保底值；不修改输入 | 不是第二个 Run owner，也不从调用者的任意输入获得持久权威 |
| GeneratedSourceReceipt | 私有载荷，只有 const 访问器；包含稳定 SourceId、ContainerId、有序 ItemIds、完整计划与接收序号；可重新校验派生身份 | 存在或 IsValid 不证明保存成功，没有 Blueprint 可写入口 |
| Evaluate 结果 | 明确区分 Rejected / CandidatePrepared / ExactReplay；拒绝返回空回执 | CandidatePrepared 不是 durable accepted，不可直接据此物化 World |

身份以 `Owner + Run + SourceRole` 为自然键，复用现有长度前缀确定性算法，并将来源、容器、物品序号放进不同 v1 命名空间。FName 来源大小写归一；seed 或计划变化不会产生一个新来源键来逃避冲突。完整载荷逐字段比较，包含全部 RewardMetadata 和有序 affix，而不是依赖旧 PlannedStack 的不完整相等运算或只比较 seed。

新候选要求活动 Run、匹配 Owner/Run/来源 manifest、精确的序号和非零夹具证明的保底 compare-and-swap。既有来源必须是合法且匹配的回执，不能把损坏/错误来源当作不存在。相同完整输入可在后续序号、甚至该 Run 已终局后返回原回执；终局后不允许新增来源。序号上溢失败关闭，不递增外部 cursor、不发新随机身份。

领域层只检查解析后数据的结构与一致性：堆叠数量、合法奖励元数据、来源归属、section/slot 唯一、同定义不能漂移、子容器规格成对、列表/字符串边界。不内置产品的保底 0..3 规则；测试使用 7→8。未变化保底来源必须携带当前值原样通过，不能用默认0重置。预算/价值是已解析事实，本轮不另行实现奖励守恒公式或推断哪些数值包含 affix/rare 奖励。

## 3. 有界性和未闭合项

单来源最多1024条、slot/子容器容量上界4096、每定义最多64个标签、内容摘要最多1024字符；既有奖励元数据验证仍约束 affix 列表。这是技术输入上界，不是玩法掉落数量/数值设计。验证临时表按条目数线性增长，同定义检查保存借用指针而非反复复制完整定义；候选与返回重放回执仍是完整值拷贝。

这只限制一次调用的工作量，**不解决整个 Run 的来源历史增长、全快照保存峰值或实际 RAM/VRAM**。本轮没有新序列化格式，所以不报虚构的存档字节数或内存百分比。接同一持久文档时再记录来源数、条目数、序列化字节及复制成本；未决/重放历史不能直接删除。UI 不应取得这份完整来源账本。

调用前提明确写入接口：View 和 Existing 必须来自同一把锁下的唯一权威快照；Existing=null 必须是按 Owner/Run/SourceRole 查明不存在，不是查询失败。纯函数本身不提供并发互斥、目录授权、跨账本 GUID 冲突检查或持久事务，不能把这项测试通过解释成这些能力已实现。

下一增量接现有 Repository/AuthorityService/AuthorityDocument：同一候选原子保存来源与保底序号；显式处理当前 schema2 和仍支持的 schema1、旧摘要先验证；写前失败/结果不确定/重启重放保留同一身份。然后切换 Manager 查询/接收及容器物化，最后闭合未拿取/已获得/已消耗/遗失和 Extraction/Death/Abandon。沿用 P28.30 的终局警示：不能先把来源物品插入图，再直接绕过 AcquiredItemMismatch 重复导入检查。

## 4. 验证与交接

首次 Editor/Game 实际构建0/0后，新专项在测试夹具向 TArray 追加其自身元素引用时触发保护断言：原生3，只有1条 Success、无完成队列；不是测试通过，也不是产品缺陷 Red。修正为先复制局部值，再追加，未修改生产契约；首次日志/初始输入锁保留。

最终重新构建 Editor/Game，均实际编译/链接且原生0。新专项5/0，整个 Items 90/0，旧系统 demo_map 全根1330/0；均精确队列、原生0、无 Fail/Fatal/Ensure/Unhandled/Assertion。暂存检查另发现两处EOF空行；修正后Final2再次完整执行相同构建与回归，使最终证据对应提交文件。**独立成功用例1420，专项5项是 Items 子集，重复复跑也不重复相加。** 一个专项用例内部覆盖53种完整载荷变化；不是新增53个注册用例。测试夹具数量3、序号11、保底7→8，避免0==0恒真基准。

三源码路径触发 Items 必跑组；六个交接路径检查 `Changed=6 Rules=1 Required=1 Logs=1` 通过；检查器自检现跑537/537。原1617产品/验证输入均未变，新增3源码后固定1620输入；103个原未跟踪文件、两份 OverallReadiness 用户修改保持。原件与 SHA 见 [Development Log](../Log/Dev.D.UE.0.0.10.P28.31.r0_log.md)。

本轮未重跑 Shanmen 全根，也不把 P28.29 的1430/0冒充当前新增源码后的完整根验收；最终冻结仍需全根与双构建。只精确提交本轮三源码、Report、Log、[冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md) 共6文件。按 [P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)，没有启动 Editor UI/PIE/Standalone/产品程序或 Smoke/Cook/Package，没有输入、地图、UI、敌人或玩法改动。FZ-1/2仍开放，自动化继续底层闭合。
