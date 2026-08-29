# Dev.D.UE.0.0.10.P6.13.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.13.r0`；
- 基线提交：`aa09a34b672f3657e9dfc052e60a92476092b8a0`（P6.12）；
- 分支：`agent/0.0.10-p6-13-orbit-threat-world-evidence`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.11 已冻结 Orbit 近身威胁的 canonical geometry receipt，P6.12 已冻结 `RequiredTargetTags + bRejectSelf` 的确定性 policy receipt，但产品测试仍需要手工构造 `TargetLiving` evidence。下一最小闭环因此是明确 World 证据来源，而不是提前增加效果或数值。

Directed 路径已经把 `Idemo_mapCombatVitalityHost` 作为 canonical Living 目标证据，并核对其 EntityId。P6.13 复用该来源，让完成 receipt 与调用方同一采样窗口收集的 Actor batch 显式 join。

## 架构判断

### 不增加 Registry 反向查询

`FShanmenWorldEntityRegistry` 明确允许 Actor、Component 等多个 UObject 别名映射同一 EntityId。增加 `EntityId -> UObject` 单值反查会在结构上产生歧义；增加 multimap 后再猜“哪个对象是 tag authority”仍会引入第二套角色规则。因此 Registry 保持不变。

### 不持久缓存 Actor 或候选

World evidence capture 只在完成 emission 后接收显式 Actor batch。内部用 EntityId 临时 join，返回值只含值类型 evidence；函数返回后不保留 Actor、Component、TWeakObjectPtr 或第二份 candidate cache。

### 几何与策略继续分层

World 层不因为对象缺少 Living 接口而删除 geometry candidate。已注册但没有 vitality host 的对象得到空标签，P6.12 再产生 `RejectedMissingRequiredTags`。只有身份矛盾、缺失、重复、越界或未注册时 capture 本身失败关闭。

## 捕获契约

输入为 ready Coordinator、有效完成 receipt 和 Actor batch。处理顺序：

1. 校验 receipt Run 与 Coordinator Run；
2. 从 canonical candidates 建立允许的 EntityId 集合；
3. 逐 Actor 通过既有 Registry 做 object-to-entity 解析；
4. 拒绝 null、unregistered、outside-emission 和 duplicate；
5. 要求 Actor 数量与 candidate 数量完全相等；
6. 按 candidate canonical 顺序输出 evidence；
7. vitality host 的绑定 ID 必须相同，匹配时加入 `TargetLiving`；
8. Controller 把 evidence 交给 P6.12 原有策略，Run Host 继续按 exact item 路由。

## 自动化覆盖

### World Adapter 直属

- invalid / unfinished emission；
- null Actor；
- unregistered Actor；
- registered actor outside receipt；
- missing target；
- duplicate target identity；
- real Enemy vitality host -> Living evidence；
- capture 前后 vitality 相同、impact ledger 为 0。

### Controller / Run Host

- Controller 的 missing / duplicate / Player 替换 Enemy 三种失败；
- exact Enemy Actor 自动形成 accepted target policy receipt；
- Host exact item capture + evaluate；
- unknown item 不可借用 receipt；
- 完成 policy 后 Launch 与 Directed 路径继续工作。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p613_product.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 30 | 0 | 0 | `770631C299EFADB5AB6289D18B5E1ABEDBABE40ACA69990CC6CA9753C9D5DC3D` |
| `p613_full.log` | `Shanmen.0_0_10` | 161 | 0 | 0 | `7FD287F0DC9D774DEEFF5EE6783F8519294D4B608D5B9F1729F86160E296B7F5` |

最终日志位于 `Saved/Automation/P613/`。Product 实际执行约 `6.09s`，全量约 `15.07s`。每份日志有一个实际 RunTests 命令、一个 queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与 native exit `0`。

两份日志在测试发现前各包含 13 条既存 `LogAutomationTest: Error: Condition failed` 诊断噪声；目标测试随后均以 Success 完成，本阶段未改变该发现期噪声。

## Changed-file gate

最终 Source 状态：

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=11 Logs=2
```

完整 `Shanmen.0_0_10` 覆盖 World Adapter、Controller 与 Run Host 映射出的 11 个必跑组；Product focused 日志提供直接路径证据。Regression coverage self-test：`14/14 PASS`。

加入 Report / Log 后，Docs 属于 mapping 的 ignored paths，不增加产品测试要求；最终 staged gate 为：

```text
REGRESSION_COVERAGE: PASS Changed=11 Rules=3 Required=11 Logs=2
```

## 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- Editor 首次：`36/36` actions，`Result: Succeeded`，native exit `0`，`124.15s`；
- 契约复审补入直属测试后的 Editor 增量：`4/4`，`Result: Succeeded`，native exit `0`，`6.27s`；
- Game：`31/31`，`Result: Succeeded`，native exit `0`，`105.65s`；
- 没有首次失败、源码重试、环境错误、内存错误或外层超时；
- Game executable 未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- Source 新增 `456` 行、删除 `18` 行；生产新增 `267` 行；
- 新增 Source 行的 ApplyDamage / TakeDamage / impact delivery / world query / Spawn / RNG / inventory Commit 扫描命中 `0`；
- `Reserve` 字面命中仅为 `TSet/TMap/TArray::Reserve` 容量预留，不是物品 Reserve；
- Registry、GameplayTags、Profile schema、存档、Item authority、input、资产和 Build.cs 均未修改；
- 最新测试源码 UTC `2026-08-29T03:10:06.5954204Z`；
- Editor DLL UTC `2026-08-29T03:10:27.4292367Z`；
- Game executable UTC `2026-08-29T03:13:36.2821028Z`；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

下一阶段仍不应直接制造伤害。先冻结 accepted threat receipt 的第一种效果语义及其 cadence / per-target cooldown / authority owner；若新增阵营或状态标签，提供显式 tag-authority 接口并扩展本 capture，而不是从 legacy targeting 猜测。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-13-orbit-threat-world-evidence/Docs/Report/Dev.D.UE.0.0.10.P6.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-13-orbit-threat-world-evidence/Docs/Log/Dev.D.UE.0.0.10.P6.13.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-13-orbit-threat-world-evidence>
