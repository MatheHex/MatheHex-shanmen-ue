# Dev.D.UE.0.0.10.P5.1.r0 Development Log

## 目标与基线

- 基线分支：`agent/0.0.10-p5-0-authority-quick-use`；
- 基线提交：`45194b6125a406cfdd7bd1ad4b936979ff6ab36c`；
- 当前分支：`agent/0.0.10-p5-1-authority-inventory-use`；
- 目标：关闭 InventoryWidget 对 Runtime `UseInventoryItem` 的直接调用，使准备物品的背包使用与 P5.0 快捷栏使用共享同一 durable-first 权威链。

## 设计决定

### 一个 Runtime 内核

旧 `UseInventoryItem` 通过临时写入快捷栏再调用 `UseHotbarSlot`。它能复用代码，但引入了不可见的 hotbar mutation，并且产品 Widget 仍可绕过持久权威。

本轮将公共逻辑抽为 `PreviewItemUse` 与 `CommitItemUse`。快捷栏入口额外校验 1--9 slot 和 expected binding；背包入口直接以 ItemInstanceId 预检。两者共享 ownership、effect、health、cooldown、consume、rollback 与 invariants。

### 一个 durable 实现

Lifecycle adapter 以 `EPreparedRunItemUseRoute` 区分 Hotbar/Inventory，但 durable preflight、prepared Quantity line、snapshot、CAS、receipt 校验和 Runtime-after-authority 顺序只有一个实现。

既有 Hotbar 请求命名空间保持不变；Inventory 使用独立命名空间，避免改变已发布 P5.0 RequestId。相同 route、item 与 before-stack 形成相同 RequestId，可在 Runtime rollback 后精确重放。

### 产品 fail-closed

InventoryWidget 只调用 V3ProgressionManager。存在 Shanmen lifecycle 时，manager 只允许 materialized `RunActive`，并把失败原样返回 UI；不回退 Runtime-only 或 Code B。没有 Shanmen lifecycle 时保留旧 compatibility。

## 实现步骤

1. ItemSubsystem 新增 direct preview，并收敛 hotbar/inventory 公共事务。
2. 删除 direct inventory 临时 hotbar binding 技巧。
3. Lifecycle adapter 新增 prepared inventory use，共享 P5.0 durable-first 内核。
4. ProfilePreparationFlow 返回 UI 可消费的 `Fdemo_mapItemUseResult`，区分 preview reject、authority reject 与 Runtime projection reject。
5. V3ProgressionManager 新增 authority-selecting inventory API。
6. InventoryWidget 改走 manager，不再取得 subsystem 后直接 use。
7. 新增 direct inventory durable failure/replay/restart 测试和 Widget source-wiring 断言。
8. RegressionMap 把 Widget 纳入产品链，并把通用 ItemSubsystem 改动映射到两个旧消费套件。

## 聚焦自动化

命令：

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Items.RunLifecycle.DurableInventoryUse" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

最终：`1 Success / 0 Fail`，queue empty，fatal/unhandled/ensure `0`，进程退出码 `0`，SHA-256 `683D5382F89ABA19302E7D6F0EDD23114D5E26670371EF626CFCE93ADCFDACF3`。

验证序列：

- materialize stack `3`，hotbar 不含该 identity；
- authority durable consume `Persisted`；
- `AfterItemMutation` 使 Runtime rollback 为 stack `3` / health `1`；
- exact retry 的 authority status 为 `Replayed`；
- Runtime 最终 stack `2` / health `2`；
- hotbar 全程不变，durable consume receipt 数量严格为 `1`；
- restart 后 stack `2`，仍无 hotbar binding；
- Widget source 只有 manager route，无 direct subsystem use。

## 最终回归日志

- `Dev.D.UE.0.0.10.P5.1.r0_shanmen_final.log`：`115/115`，SHA `43CD7890C70BC7F843588EC27C250C33656B46FB099A5411DEA8790D75CCD645`；
- `Dev.D.UE.0.0.10.P5.1.r0_profile_final.log`：`211/211`，SHA `6E1DA6E53F3A0D77830FCCDAEB1235A9151B4F671B160FF035B6F1847C3B13C1`；
- `Dev.D.UE.0.0.10.P5.1.r0_itemeconomy_final.log`：`23/23`，SHA `D2C792098BAF3021C67C27DFFBBF029C8AA8C5D44B6690509F86E9C9B6EAB9A5`；
- `Dev.D.UE.0.0.10.P5.1.r0_codeb_final.log`：`60/60`，SHA `40F316AFAEC3CBD782E4BCDF5134E517884EF77E34F928F1E0D3EBE4215594CE`；
- `Dev.D.UE.0.0.10.P5.1.r0_v2ranged_final.log`：`22/22`，SHA `3A95C1639B1670185A2D425EE5B2FF2E8D076101C82DB567F5B7EF7D583847FD`；
- `Dev.D.UE.0.0.10.P5.1.r0_itemuse_final.log`：`46/46`，SHA `782B53B4A26231A735D909302552531278CB66210E30A5A21FE0A1EA7CF2D08B`；
- `Dev.D.UE.0.0.10.P5.1.r0_hotbar_final.log`：`7/7`，SHA `F01A8CB04079E35F8C613FAE3F89B8D43F9B68B6E90E5A10D84104EB42220DDA`。

总计 `484 Success / 0 Fail`；全部 queue empty、fatal/unhandled/ensure `0`、进程退出码 `0`。

## Changed-file gate

- self-test：`SELF_TEST: PASS 7/7`；
- final gate：`REGRESSION_COVERAGE: PASS Changed=11 Rules=3 Required=8 Logs=7`；
- required：`Shanmen.0_0_10`、`Shanmen.0_0_10.Items`、`demo_map.Profile`、`demo_map.ItemEconomySchema`、`demo_map.CodeB`、`demo_map.V2RangedCompatibility`、`demo_map.ItemUseAndArmor`、`demo_map.P4.Hotbar`。

## 首次失败与修复

1. Editor 首次完整构建计划 `54` actions；`demo_mapShanmenPreparationAdapterTests.cpp` 使用不存在的 `Committed` 枚举，UBT `OtherCompilationError`、原生退出码 `1`、`201.91s`。改为真实 `Persisted` 后增量成功。
2. 第一条聚焦 Automation 命令误用 `-log=<absolute>`，进程退出码 `0` 但目标日志不存在，因此没有当作测试证据。改用 `-AbsLog=<absolute>` 后得到有效 `1/1` 日志。
3. Direct test 初版只覆盖正常消费；审查后补入 Runtime 失败与 exact replay。最终 Editor、七组日志和 Game 均在增强后的源码上重新验证。

## 构建

Editor Development，单并发、NoUBA：

- 初次：失败，退出码 `1`，`201.91s`；
- enum 修正：`4/4`，退出码 `0`，`6.25s`；
- 最终 test-only 增量：`4/4`，退出码 `0`，`12.68s`。

Game Development，单并发、NoUBA：

- 完整：`51/51`，退出码 `0`，`175.61s`；
- 最终 test-only 增量：`3/3`，退出码 `0`，`20.44s`；
- `Binaries/Win64/demo_map.exe` 已生成，未启动。

## 静态与边界

- `git diff --check`：退出码 `0`；
- `ShanmenItems` 禁止依赖/RNG 扫描：`0`；
- InventoryWidget direct Runtime use 扫描：`0`；
- Source/Scripts 改动：`11` 文件，`543 insertions / 191 deletions`；
- 长期未跟踪的旧 Prompt、Report 与用户文件未暂存。

## 最终不变量

1. 背包与快捷栏只选择入口不同，消费事务不分叉。
2. Shanmen lifecycle 存在时，UI 不得直接修改 Runtime stack。
3. Durable receipt 必须先于 Runtime effect。
4. Runtime rollback 不删除 durable receipt。
5. 相同未变化意图只能重放，不得二次持久扣减。
6. Direct inventory use 不得临时创建 hotbar binding。
7. Restart materialization 必须减去全部 successful consumption receipts。
8. 改动通用 item 内核时必须同时跑新 Items 与旧 item/hotbar 回归。

## P/F 边界

只执行 P 阶段代码、静态检查、无头 `-NullRHI` Automation、Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-1-authority-inventory-use/Docs/Report/Dev.D.UE.0.0.10.P5.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-1-authority-inventory-use/Docs/Log/Dev.D.UE.0.0.10.P5.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-1-authority-inventory-use>
