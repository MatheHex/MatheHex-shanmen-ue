# Dev.D.UE.0.0.10.P28.0.r0 Report

## 1. 结论

完成冻结前当前文档入口、共享 P 基线和契约索引整理，修复两个根 Markdown 文件未被回归映射分类的问题。不修改产品源码、内容或存档；不宣布整体底层冻结。

当前入口不再把 0.0.9B P23、旧附件信封和旧 P/F 门禁当作 0.0.10 当前流程。旧正文保留并明确标记历史，既有未跟踪 CSEMI 等用户文件不修改。

## 2. 基线与已证实问题

- 日期：2026-09-16 UTC。
- 入场 HEAD：`d62343b3c4ba48d3ba9dc5c97f9cc7569e39ca81`；最近产品实现 P27.31 / `0b6af9c`。
- 续交接基线：`8c47c6641e17ecd701241fb466b6d6e41e369320`。期间只独立提交总体 Report/Log；本阶段产品输入、映射与自检脚本保持，复核原结果后交接，不重复运行构建。
- 分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- `PROJECT.md` / `PROJECT_INFO_CARD.md` 仍以旧 P23 为当前任务，并要求附件/标记交接；旧 I 门禁也与当前无头验证授权冲突。
- ItemAuthority ADR 仍将已修复的 schema 范围写为当前缺陷；Transactions ADR 将所有拒绝一概表述为持久 ledger 写入，与只读冲突拒绝不符。
- 原检查器处理两个根 Markdown 路径时实际抛出 `unmapped changed paths ... (unclassified)`。这是流程分类缺口，不是 UE 产品测试失败。

## 3. 最小修改

1. 两个根入口新增当前说明与链接；旧正文明确限定为 0.0.9B 历史，不移动或删除历史文件。
2. 旧 I 门禁增加版本范围声明，不恢复旧附件流程或 I 前置关卡。
3. 新建 [P 阶段共享基线](../Process/P_STAGE_BASELINE_0_0_10.md)，收纳现有范围、改动驱动验证、原生状态区分、GitHub 交接和最终暂停条件；不增加新玩法授权。
4. 新建 [底层契约索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)，连接领域权威、产品入口、Run 生灭和已有证据，将未核验项与 F 债务分开。
5. ADR 修正 schema 历史状态、可记录拒绝与只读拒绝的区别，并明确历史裁剪仍未实现。
6. 映射只增加根目录 `PROJECT.md` / `PROJECT_INFO_CARD.md` 的完整文件名匹配；不忽略整个 PROJECT 前缀、Source 或任意 Markdown。
7. 新增两个正例及四个近似路径反例，既有 504 个自检保留，合计 510。

## 4. 索引的真实边界

索引给出已有实现与证据，不声称对所有历史调用点完成证明。下一步只剩三个明确审计项：

- FZ-1：产品整备、拾取、快捷/库存使用与终局的旧/新权威路由；说明 Code A Runtime 投影的约束。
- FZ-2：Run 激活早期失败与最终释放的可达性、owner 保持或安全回滚。
- FZ-3：前两项关闭后固定产品输入、执行最终完整根回归/双构建、发布冻结基线并暂停监控。

本轮深读了激活前置排斥、Condition 空状态初始化和剑法 Session/Presentation 候选创建。早期失败分支的 Reset 尚不构成正常入口可达缺陷的证明；未据此新增注入 API 或改产品代码。

## 5. 映射与脚本验证

- 原根文档映射失败已留档，精确错误及 SHA 在 Log。
- 映射自检：`SELF_TEST: PASS 510/510`。
- 最终十一文件范围：`REGRESSION_COVERAGE: PASS Changed=11 Rules=0 Required=0 Logs=0`。
- 0 个必跑 UE 组表示本轮无生产路径改动，不表示“零测试即可证明产品通过”。实际变动的映射由 510 个脚本自检验证。
- 新反例覆盖 `PROJECT.cpp`、`PROJECT_INFO_CARD.md.cpp`、`PROJECT_NOTES.md`、`Source/demo_map/PROJECT.md`，均仍被拒绝。

## 6. 双目标构建与既有回归

本轮通过统一入口串行执行新的 Editor/Game 构建：

| 目标 | 实际结果 | stdout 原字节 SHA-256 |
|---|---|---|
| Editor | Succeeded，原生 0；5 个链接/元数据 action，12.09 秒 | `46899DC8FF264B7C385BDB9CA03B1BA30C948C2D582016C8766883EF9D7F095B` |
| Game | Succeeded，原生 0；0 action，1.46 秒 | `BCDF7D0DD1B6BEC7EAC4F836832F447D4CA253A3BF3A828AB719071CD7B117AA` |

没有因重新链接就宣称新跑完整 UE 根。最近完整产品结果仍是 P27.31 的新根 1419/0、旧根 1330/0；本轮产品输入保持不变，引用时明确原阶段。最终冻结仍需独立完整验证。

## 7. 外层调用判定错误

初次 self-test 与 BuildBoth 外层命令都在脚本正常返回后错误检查调用者 `$LASTEXITCODE`，外层以异常退出 1。该变量不是这些 PowerShell 脚本的返回协议。

重新核验原 self-test 完成标记与两个原生 run-state，分别为 510/510 和 0/0。保留初次日志及外层错误说明，不把外层异常伪称成功，也不将其当作 UE 编译失败；没有重复构建。

## 8. P/F 与交接

仅文档、回归分类及其测试、双目标编译。未接物理输入、修改正式地图/内容资产、设计玩法/数值/敌人/UI，未启动 Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook 或 Package。

本阶段精确十一文件，Report/Development Log 随提交推 GitHub；原始 Saved 证据仅留本地并提供路径/哈希，103 个既有未跟踪用户文件保持。不暂停监控，不进入实际游戏性开发。

发布前核验：60 个文档本地链接目标存在，映射 JSON 可解析，差异检查通过；1,577 个产品输入和 103 个原用户文件的原字节保持，两个根入口的历史正文保持。具体续交接证据见 Log 第 8 节。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.0.r0_log.md)
- [上一实现阶段 P27.31](Dev.D.UE.0.0.10.P27.31.r0_report.md)
