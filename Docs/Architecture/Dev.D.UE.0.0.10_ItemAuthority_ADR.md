# 0.0.10 物品唯一权威 ADR

## 状态

`Accepted`，自 0.0.10 P1 起生效。

## 背景

0.0.9B 同时存在两组已投入使用的物品模型：

- Code A：`Udemo_mapItemSubsystem` 及其局内快捷使用、拾取和投影路径。
- Code B：`demo_map/CodeB` 的持久物品图、局外仓库、Run 容器、结算与审计桥。

现有桥接明确包含只读／审计语义，不能据此把 Code B 当成所有局内物品操作的既有唯一权威。若 0.0.10 的资源事务只适配 Code B，技能消耗会绕过仍持有局内真值的 Code A；若同时写入两者，则会形成第三条写入路径并延续双权威。

## 决策

1. 新模块 `ShanmenItems` 是 0.0.10 对局内与局外物品的唯一可变权威。
2. Code A 与 Code B 都只作为版本迁移来源和行为参考；0.0.10 运行时不得向任一旧模型双写。
3. P1 不建立“只对接 Code B 的在线 Adapter”。P1 先建立新权威的定义、实例、容器、事务、Repository 与迁移边界。
4. 资源消费统一执行：`Validate -> Reserve -> Commit Point -> Commit/Cancel -> Receipt`。
5. 技能、Actor、GAS、战斗 Resolver 和 UI 不得直接修改物品图；它们只能通过 `ShanmenItems` 命令端口提交带有 `RunId / OwnerId / RequestId / ItemInstanceId` 的请求。
6. `Reserve` 只冻结可消费资格，不提前扣除；命中、施法或防御触发到达明确 Commit Point 后才原子提交。取消、打断、失效和重复请求必须返回确定性 receipt。
7. 护心镜等由战斗层判定触发的物品效果，只在 Impact receipt 中返回精确 `SourceInstanceId` 与 `bRequiresCommit`；实际耐久／次数扣除由 `ShanmenItems` 完成。

## 迁移规则

- 0.0.9B 的活动 Run 不做中途无损热迁移；必须先以旧版本规则结算或放弃，再进入 0.0.10。
- 迁移器分别读取 Code A Profile 与 Code B 持久文档，输出规范化候选与来源 receipt，不直接写入在线 Repository。
- 两个来源一致时才能原子导入；身份、数量、容器闭包或 revision 冲突时失败关闭，禁止静默合并或按“较新一边”覆盖。
- 导入成功后写入单一 0.0.10 文档与迁移 receipt；之后旧模型只读保留，不能继续接收业务写入。

## 前置缺陷

`demo_mapProfileRepository.cpp` 中 `IsExactLegacySourceFor` 仍把 legacy schema 限制为 `1..5`，无法正确承接当前 Schema 6 到 7 的 N-1 迁移。该缺陷必须在首次 0.0.10 持久数据导入和端到端运行验证之前修复并补回归测试；它不应通过放宽迁移校验或绕过 provenance 来规避。

## 结果

- P1 的首要产物是 `ShanmenItems` 的新事务内核和可验证迁移入口，不是旧 Code B 在线适配层。
- Code A／Code B 不再被定义为 0.0.10 的并行运行时权威。
- 战斗、飞剑、暗器、阵材、灵器被动和局外仓库最终共享同一物品身份、事务与 receipt 语义。
