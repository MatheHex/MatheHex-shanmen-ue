# Dev.D.UE.0.0.10.P28.32.r0 Report

## 1. 结论与本轮范围

`P28_32_LOSSLESS_PLAN_CODEC_COMPLETE`。2026-09-21 UTC，从 `0b73632b64110e7304a4990379c0c8ec36673741` 继续来源持久接入的前置工作：完成完整生成来源计划的无损 JSON 对象读写。**不是持久接收命令，不是库存/来源原子提交，不是 FZ-1/2 关闭或最终冻结。**

核对本机 UE 5.8 `JsonUtilities/Private/JsonObjectConverter.cpp` 第134行，默认整数导出经 `GetSignedIntPropertyValue` 进入 `FJsonValueNumber`；不能将该路径直接用于完整 uint64 seed 及超过 2^53 的精确 int64 事实。因此把 P28.31 所列的持久接入拆出这一具体前置缺口，先固定无损载荷格式，再接同一 AuthorityDocument。没有全局替换旧序列化器，也不声称已修复既有 schema2 奖励整数的潜在精度问题。

新增 [Codec接口](../../Source/ShanmenItems/Public/ShanmenItemGeneratedSourceCodec.h)、[实现](../../Source/ShanmenItems/Private/ShanmenItemGeneratedSourceCodec.cpp)，扩展 [既有来源测试](../../Source/ShanmenItems/Private/Tests/ShanmenItemGeneratedSourceTests.cpp)。现有领域计划、Repository、AuthorityService、AuthoritySnapshot、schema1/2、产品调用和内容资产均不改。没有旁路奖励文件或第二个可变账本。

## 2. 信息分区与格式规则

| 范围 | 本轮保证 | 明确不提供 |
|---|---|---|
| 嵌入载荷 FormatVersion=1 | 完整 Owner/Run/来源内容、策略身份、seed、预算、序号、保底前后值、有序条目/affix、定义/标签、奖励与子容器规格往返 | 该版本不是 AuthorityDocument 的 schema 版本 |
| 所有64位数值 | 十进制字符串；uint64 到最大值，非负 int64 到最大值；逐位检查溢出，拒绝正负号、前导零、小数、科学计数、空白和尾随字符 | 不把大整数放入 JSON number，也不宽松转零 |
| 严格解码 | 每层字段集合、JSON类型、整数范围、枚举范围和数组边界检查；最后复用 Plan.IsValid；失败清空输出并给出诊断 | 不授权定义目录、不修改 Run cursor、不证明保存成功 |
| 身份与顺序 | FName 按不区分大小写的语义规范化，标签集合排序；条目和 affix 顺序保留；往返后派生回执与身份不变 | 不重新生成物品、不改 seed、不分配随机 GUID |

读写共享局部字段布局，避免只改写入而漏改读出。解码先验证 FName 长度和内嵌 NUL，再构造名称；标签必须已注册且不重复，未知标签失败关闭而非静默丢弃。可选空元数据也必须携带完整字段集合，不能把缺字段当默认成功。规则仅用于新来源载荷，不改变历史文档字节或摘要算法。

## 3. 验证与内存边界

新增4个注册测试：完整无损往返、严格拒绝、集合边界、必需字段矩阵。原有53种载荷变更测试同时验证：合法变更往返保留变更字段，非法变更禁止编码。新测试包含 `UINT64_MAX`、`INT64_MAX`、2^53+1、序号最大合法前值、非零数量/保底、中文/转义摘要、有序多词缀；43种显式损坏变体，以及80个实际字段分别删除/置null形成的160种检查。上述内部检查数不冒充注册测试数。

1024条普通堆叠计划经真实 JSON 文本往返，实测 UTF-8 **701135字节**。这是该夹具的序列化体积，不是最坏载荷、进程峰值或运行期 RAM/VRAM。编码直接读 const 计划，不先复制整份计划；解码构造独立候选，全部成功后移动到输出。DOM、文本、候选和调用方已有计划仍可能同时驻留。

集合检查发生在领域数组分配前，限制单来源1024条、单定义64标签、奖励16词缀；不能据此宣称原始 JSON 解析器内存有界：本接口接收已解析对象，不负责原始文本的重复键识别、解析前字节上限或整 Run 历史保留量。上述外层限制须随同文档接入处理，不能另建缓存权威或随意删除幂等历史；UI 不应拿完整来源账本当展示模型。

Editor/Game 首次实际编译链接均原生0（5/4 actions）；来源专项9/0、Items94/0、旧系统 demo_map 全根1330/0，原生0、精确唯一计数/队列均吻合，无 Result=Fail/Fatal/Ensure/Unhandled/Assertion。**独立成功1424，专项9项已包含在Items中。** 改动6路径映射强制 Items 组，检查器自检537/537。启动日志仍有与前阶段相同的13条 `LogAutomationTest: Error: Condition failed`，不将这类既存启动输出描述成“整份日志零Error”；新测试注册结果全部成功。

本轮没有构建/测试失败或重试覆盖；原始日志及状态 SHA 见 [Development Log](../Log/Dev.D.UE.0.0.10.P28.32.r0_log.md)。未重跑整个 Shanmen 根，P28.29 的旧完整根只作历史，不冒充当前新源码的最终验收。固定1622产品/验证输入：前阶段1620中仅既有来源测试改动，新增2源码；其余1619输入、103个原未跟踪文件及两份 OverallReadiness 用户修改保持。

## 4. 剩余冻结阻塞与交接

下一实际增量仍是既有 Repository/AuthorityService/AuthorityDocument 的同一权威链：锁下查重并绑定真实活动 Run、来源内容与目录授权，原子提交完整来源与序号/保底；显式验证 schema1/2 旧摘要后迁移，不能因增加字段或更换整数编码而让旧摘要失效。还需写前失败回滚、结果不确定时重开核对及重启后精确重放的证据。本轮仅内存编码/解码测试，不把它称为跨进程恢复。

之后才切换 Manager 查询/接收和容器物化，并闭合未拿取/已获得/已消耗/遗失与 Extraction/Death/Abandon。继续保留 P28.30 的防线：不能先把来源物品插入图，再绕过终局 AcquiredItemMismatch 重复导入校验。这些仍属于 FZ-1 冻结阻塞，不下放成可延期 F 债务。

精确提交三源码、Report、Log、[冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md) 共6路径。按 [P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)，没有物理输入、地图/内容、玩法数值、敌人行为或UI表现改动，没有启动 Editor UI/PIE/Standalone/产品程序或 Smoke/Cook/Package。FZ-1/2仍开放，自动化继续底层闭合，不暂停或宣布完成。
