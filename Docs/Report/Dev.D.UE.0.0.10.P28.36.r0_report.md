# Dev.D.UE.0.0.10.P28.36.r0 Report

## 1. 结论与范围

本轮补充生成来源的同锁只读查询：从同一物品权威一次取得来源是否存在、活动/终局状态、当前顺序、保底、内容身份、权威修订以及指定来源的原始完整计划。没有生成、写库存、重新规划或切换产品入口，不改变持久 schema 4。

结论：`P28_36_SOURCE_READ_BOUNDARY_READY`。这是来源读取契约的阶段通过，不是 FZ-1/2 关闭、整体冻结或产品生成入口已可用。

P28.35 已有 fresh 候选转换，但旧 `TryGetGeneratedSource` 的 false 同时代表未找到和服务不可用，也没有携带下一次规划所需的当前顺序/保底。全量快照可以读取这些事实，却要求消费者拷贝并重新解释整个库存/历史。本轮提供直接、同锁、显式结果的查询，不把上述 API 缺口描述为已经发生过的玩家掉落故障。

## 2. 查询契约与信息边界

入口：[AuthorityService::ReadGeneratedSource](../../Source/ShanmenItems/Public/ShanmenItemAuthorityService.h)，值类型：[GeneratedSourceReadResult](../../Source/ShanmenItems/Public/ShanmenItemGeneratedSource.h)。Repository 从既有已验证状态推导；Service 在原 Mutex 内同时检查 Ready/Storage Owner 并读取，不建立另一份可变状态或顺序缓存。

| 返回状态 | 含义 | 调用方边界 |
|---|---|---|
| Unavailable | 服务未打开/待恢复、Owner 不匹配、无效请求或未知 Run；所有事实字段为空/default | 不是已确认不存在，不能据此重新生成或退回旧 writer |
| Absent | 在已知、正确归属的 Run 中，指定来源确实没有已接受记录 | 还须检查 RunState 为 Active；终局的 Absent 也不能接受新来源 |
| Accepted | 存在该来源的完整原回执/计划，随同返回当前 Run 顺序/保底 | 恢复使用原计划，不重新调用生成器；旧回执顺序可早于当前 Run 顺序 |

第一次来源接受前，ItemContent 已来自物品权威，SourceContent 则明确为空、尚未绑定；不借用调用方传入的 manifest 冒充持久事实。首次接受后读取实际持久来源内容。该查询不承担首次来源内容授权，新定义目录接纳也仍未解决。

返回值只是当前服务已安装、已提交状态的一次值拷贝，不是预留、提交许可或跨进程最新磁盘保证。读取后另一次接收可能推进顺序，后续写命令仍必须核对原有顺序/保底和磁盘代次；本轮不允许读取锁跨越产品生成器执行。公开 Repository 结果本身不证明持久化，产品只能通过 Ready Service 发布事实。

## 3. 验证结果

| 检查 | 本轮结果 | 范围说明 |
|---|---|---|
| Editor / Game 构建 | 239 / 236 actions，均原生0、Succeeded | 实际 Development Win64 编译，不是 up-to-date；未运行游戏程序 |
| ReadFocused | 2 Success / 0 Fail，正常结束队列2、原生0 | 新增读取专项，包含于下行 Items |
| ItemsRegression / Recovery1 | 108 Success / 0 Fail，正常结束队列108、原生0 | 完整物品组，覆盖所有本轮源码路径 |
| LegacyFullRoot / Recovery1 | 1330 Success / 0 Fail，正常结束队列1330、原生0 | 完整旧 demo_map 根 |
| 改动驱动回归 | Changed=9、Rules=1、Required=1、Logs=2 | 原 Items 映射已覆盖6个改动源码；文档分类不要求额外产品组 |
| 映射自检 / 边界检查 | 549/549；核心边界通过 | 未修改映射或自检脚本 |

独立成功用例共1438（108+1330），专项2项不重复计数。一个 Initial 输入锁包含1628个路径，验证与重跑期间源码/配置/内容/脚本未变。没有重跑完整 Shanmen 根；P28.34.r1 的2778完整双根仅为历史证据，不能据此声称当前完整双根通过。

首轮 Items 日志有108项成功、0项失败，原生进程退出0，但缺少队列结束和退出尾段；外层证据检查退出1，未计为完整通过。保留原件后，以同一输入/构建重跑到独立 Recovery1 目录得到上表完整结果。缺尾段原因未确定，不宣称源码错误或已修复的日志故障。四份测试日志各有13条既存启动 `Condition failed` Error，无注册 Fail/Fatal/Ensure/Unhandled Exception/Assertion failed；不把原日志清洗成“零错误”。完整路径、SHA和原生记录见 [Development Log](../Log/Dev.D.UE.0.0.10.P28.36.r0_log.md)。

## 4. 测试覆盖的具体边界

新增两个注册用例，保留既有用例并扩展其并发读取断言：

- `Read.LifecycleAndRestart`：真实预备 Run、初始 Absent；两份来源推进顺序 0→1→2、保底 0→7→8；旧回执与当前顺序同时存在；过期读取用于另一来源提交被拒绝；读取与拒绝不改变完整文档或原始文件字节；重启后值保持；Abandon 后重启仍区分已接受历史和终局缺失来源。
- `Read.UnavailableAndRecovery`：Closed、错误/无效 Owner、无效/未知 Run、空来源；原非零记录不会泄露为旧输出；WriteTemp/AtomicReplace 回滚后确认 Ready+Absent；提交后回读故障经重开证明后才返回 Accepted；实际外部写入竞争造成 RecoveryRequired 时，已有和不存在来源都返回 Unavailable；重新打开后只保留胜出来源与正确顺序/保底。
- 既有 `DurableConcurrentAcceptance`：八个同来源并发提交各自随后读取，仍只接受一次、七次重放，读回同一完整计划和顺序1/保底7。不是不同来源高并发吞吐或跨进程压力测试。

夹具在隔离 Saved 测试目录使用原持久服务；保底7/8是领域契约测试值，不改变产品0..3政策；没有新故障注入口。首次 API 不存在，属于新增读取契约及证据，不制造旧实现 RedProof。

## 5. 内存、复杂度与模块边界

查询扫描现有请求历史寻找 Run 开始/终局，扫描来源记录寻找本 Run 最大已接受顺序；不拷贝整个 AuthorityDocument、库存图或所有来源计划，不排序、不新增长期索引。返回最多一份所查询来源的计划及其确定性身份。

静态复杂度仍随请求/来源历史长度增长，已接受回执重建也要验证条目；不能把“只返回一份计划”写成常数时间或零分配。没有本轮 RAM 峰值、锁占用时长或吞吐测量，历史容量及全量持久化成本仍是既有债务。

ShanmenCore/CombatCore/Items 仍无 demo_map include、World 类型、ApplyDamage、直接 RNG 或 Engine 模块依赖；读方法没有存取盘/生成器调用。新方法不修改原来源接收、终局、schema、旧 TryGet 行为或 Code A/B 权威归属。

## 6. 未闭合项与交接

FZ-1 下一步仍是合法新定义的同权威目录接纳、首次来源内容授权，以及将产品 Subsystem/Manager 的查询和接受接入既有权威。之后验证原计划恢复/物化失败续接、获物/使用/消耗/遗失和 Extraction/Death/Abandon 身份组合。保持 `AcquiredItemMismatch`、`ItemNotPrepared` 和旧写入隔离，不用本次只读接口绕过这些未闭合门。

FZ-2 实际生命周期与未覆盖生成/释放组合继续按有限清单核对；FZ-3 仅在 FZ-1/2 关闭、最终产品输入固定后执行完整双根与双构建。见 [冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)。不是整体冻结，不暂停为已完成。

本阶段只交付6个既有源码路径及 Report/Log/索引，共9路径；103个用户未跟踪文件与两份 OverallReadiness 修改保持。遵守 [P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：无物理输入、地图/资产、玩法/UI/敌人行为改动，无 Editor UI、PIE、Standalone、游戏程序、截图、Smoke、Cook 或 Package。原始日志本地保留，GitHub 发布报告及证据索引。
