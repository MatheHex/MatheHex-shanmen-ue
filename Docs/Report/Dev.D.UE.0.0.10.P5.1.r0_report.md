# Dev.D.UE.0.0.10.P5.1.r0 Report

## 1. 结论

P5.1 完成，结论为 **PASS**。

本轮关闭了背包界面“使用”按钮绕过 `ShanmenItems` 持久权威、直接修改 Runtime stack 的旁路。准备阶段带入 ActiveRun、但未绑定快捷栏的消耗品，现在与 P5.0 快捷栏消费共享同一个预检和 Runtime 原子提交内核，并严格执行 durable receipt 先于局内 item/health/cooldown 投影。

最终 Editor Development、七组 changed-file Automation、回归覆盖门禁、Game Development 与静态检查全部通过。Report/Log 已按 0.0.10 简洁 Git 工件流程生成；没有 CSEMI 信封。

## 2. 缺陷与产品路由

原 `Udemo_mapInventoryWidget::HandleUse` 直接调用：

```cpp
Manager->GetItemSubsystem()->UseInventoryItem(...)
```

该调用只修改 transient Runtime。在 Shanmen lifecycle 下，它可绕过 P5.0 已建立的 append-only durable consumption receipt，使背包点击与键盘快捷栏拥有不同真值。

修正后产品路由为：

```text
InventoryWidget
  -> V3ProgressionManager::RequestUseInventoryItem
  -> ProfilePreparationFlow::UseActiveRunInventoryItem
  -> ShanmenRunLifecycleAdapter::UsePreparedRunInventoryItem
  -> ShanmenItems durable consume
  -> ItemSubsystem Runtime projection
```

存在 Shanmen lifecycle 时全链 fail-closed，不回退到 Runtime-only 或 Code B；旧流程没有 Shanmen lifecycle 时继续使用原 Runtime compatibility 路径。

## 3. 共用消费内核

`Udemo_mapItemSubsystem` 新增只读 `PreviewInventoryItemUse`，并把快捷栏与背包的公共逻辑收敛到：

- `PreviewItemUse`：Run identity、health、input、ownership、definition、stack、effect、full-health 与 cooldown；
- `CommitItemUse`：item quantity、health、cooldown、hotbar refresh 与 invariant 的原子提交/回滚。

背包入口不再临时篡改快捷栏来复用逻辑。未绑定物品使用前后快捷栏数组保持完全相同；若一个原本已绑定物品耗尽，既有 refresh 仍会清除失效 binding。

## 4. 持久身份与重试

Lifecycle adapter 只有一个共享 durable-first 实现，入口差异限于物品选择方式与请求命名空间：

- 快捷栏保留既有 `demo_map.Shanmen.RunItemUse.Request.r1`，不破坏 P5.0 重放身份；
- 背包使用新增 `demo_map.Shanmen.RunInventoryItemUse.Request.r1`。

背包 RequestId 由 owner、scope、ActiveRun、ItemInstanceId、消费前 stack 与 purpose 确定性派生。Runtime 注入失败时 durable receipt 保留，本地 stack/health/cooldown 回滚；相同未变化意图重试返回 `Replayed`，不会产生第二次持久扣减。

## 5. 自动化证据

新增 `Shanmen.0_0_10.Items.RunLifecycle.DurableInventoryUse`，验证：

1. 未绑定药品以完整 stack `3` materialize；
2. `AfterItemMutation` 失败注入后 durable command 为 `Persisted`，Runtime stack 仍为 `3`、health 仍为 `1`；
3. exact retry 为 `Replayed`，Runtime 最终 stack `2`、health `2`；
4. hotbar 在失败与重试后均不变；
5. authority snapshot 中只有一条 successful consume receipt；
6. restart rematerialization 恢复 stack `2` 且仍未绑定；
7. InventoryWidget 源码只调用产品 manager，不存在 Runtime-only use 字符串。

聚焦日志：`1 Success / 0 Fail`、queue empty、fatal/ensure `0`，SHA-256 `683D5382F89ABA19302E7D6F0EDD23114D5E26670371EF626CFCE93ADCFDACF3`。

## 6. Changed-file 回归

`Scripts/ShanmenRegressionMap.json` 新增两项约束：

- `demo_mapInventoryWidget.*` 纳入 ProductRunItemUse；
- ItemProductAdapters 除 `Shanmen.0_0_10.Items` 外，必须运行旧 `demo_map.ItemUseAndArmor` 与 `demo_map.P4.Hotbar`。

最终七份日志均 queue empty、`0 Fail`、fatal/unhandled/ensure `0`、进程退出码 `0`：

- `Shanmen.0_0_10`：`115/115`，SHA `43CD7890C70BC7F843588EC27C250C33656B46FB099A5411DEA8790D75CCD645`；
- `demo_map.Profile`：`211/211`，SHA `6E1DA6E53F3A0D77830FCCDAEB1235A9151B4F671B160FF035B6F1847C3B13C1`；
- `demo_map.ItemEconomySchema`：`23/23`，SHA `D2C792098BAF3021C67C27DFFBBF029C8AA8C5D44B6690509F86E9C9B6EAB9A5`；
- `demo_map.CodeB`：`60/60`，SHA `40F316AFAEC3CBD782E4BCDF5134E517884EF77E34F928F1E0D3EBE4215594CE`；
- `demo_map.V2RangedCompatibility`：`22/22`，SHA `3A95C1639B1670185A2D425EE5B2FF2E8D076101C82DB567F5B7EF7D583847FD`；
- `demo_map.ItemUseAndArmor`：`46/46`，SHA `782B53B4A26231A735D909302552531278CB66210E30A5A21FE0A1EA7CF2D08B`；
- `demo_map.P4.Hotbar`：`7/7`，SHA `F01A8CB04079E35F8C613FAE3F89B8D43F9B68B6E90E5A10D84104EB42220DDA`。

合计 `484 Success / 0 Fail`。门禁 self-test 为 `7/7 PASS`；真实检查为：

```text
REGRESSION_COVERAGE: PASS Changed=11 Rules=3 Required=8 Logs=7
```

## 7. 首次失败与恢复

1. 首次 Editor 完整编译在 test-only 断言使用了不存在的 `EShanmenItemDurableCommandStatus::Committed`，UBT 为 `OtherCompilationError`，原生退出码 `1`，总耗时 `201.91s`。实际状态名修正为 `Persisted` 后，Editor 增量 `4/4` 成功，退出码 `0`。
2. 第一次聚焦 Automation 调用使用 `-log=<absolute>`，进程退出码 `0` 但没有生成请求的测试证据文件；这不计为测试通过。改用 UE 的 `-AbsLog=<absolute>` 后形成有效日志并通过。
3. 增强 direct retry 自动化后，Editor 最终必要增量 `4/4` 成功，退出码 `0`；最终七组日志全部在该源码版本上重新生成。

没有把上述调用/测试代码错误描述为产品功能成功，也没有重复执行无关构建。

## 8. 构建与静态检查

Editor Development：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次：失败，退出码 `1`，test-only enum 名错误；
- 修正后：`4/4`，Succeeded，退出码 `0`，`6.25s`；
- 最终测试增强后：`4/4`，Succeeded，退出码 `0`，`12.68s`。

Game Development：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 完整：`51/51`，Succeeded，退出码 `0`，`175.61s`；
- 最终 test-only 增量：`3/3`，Succeeded，退出码 `0`，`20.44s`；
- 产物：`Binaries/Win64/demo_map.exe`，未启动。

静态检查：

- `git diff --check`：退出码 `0`；
- `ShanmenItems` 中 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage` 与运行时 RNG：`0` 匹配；
- InventoryWidget Runtime-only use 旁路：`0` 匹配；
- Source/Scripts：`11` 个文件，`543 insertions / 191 deletions`。

## 9. 修改范围

- `demo_mapItemSubsystem.*`：公共预检/提交内核与无绑定背包入口；
- `demo_mapShanmenRunLifecycleAdapter.*`：共享 durable-first 路由与背包请求身份；
- `demo_mapProfilePreparationFlow.*`、`demo_mapV3ProgressionManager.*`：产品 authority selection；
- `demo_mapInventoryWidget.cpp`：移除直接 Runtime use；
- `demo_mapShanmenPreparationAdapterTests.cpp`：失败、重放、重启与 source-wiring 证据；
- `Scripts/ShanmenRegressionMap.json`：Widget 映射和旧消费回归。

未纳入工作区长期未跟踪的旧 Prompt、Report、自动化交接文档或其它用户文件。

## 10. P/F 边界

本 Report 只包含 P 阶段源码开发、静态审查、无头 `-NullRHI` Automation、Editor Development 与 Game Development build。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。真实背包点击手感与端到端产品体验留给 F 阶段。

## 11. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-1-authority-inventory-use/Docs/Report/Dev.D.UE.0.0.10.P5.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-1-authority-inventory-use/Docs/Log/Dev.D.UE.0.0.10.P5.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-1-authority-inventory-use>
