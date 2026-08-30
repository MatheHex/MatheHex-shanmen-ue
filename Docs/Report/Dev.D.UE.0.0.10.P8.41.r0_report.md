# Dev.D.UE.0.0.10.P8.41.r0 Report

## 1. 结论

P8.41 已把 P8.39/P8.40 的 formation consumer Run activate/deactivate composition 接入既有产品生命周期 owner：`Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost`。新入口复用 Host 已有的 consumer runtime、lifecycle router、source receipt 与幂等历史，不创建第二个 Session、ledger、registry、receipt cache 或轮询控制器。

本阶段结论为 **PASS**：owner composition `1/1`、LifecycleCommandHost `5/5`、CombatRunCoordinator `17/17`、World resolution `1/1`、Consumer ProductRuntime `4/4`、Consumer ProductBridge `5/5`、FormationProductHost `6/6`、WorldGameplay `10/10`、legacy Attributes `4/4`、`Shanmen.0_0_10` 全量 `337/337`；共 `390` 条 success、`0` fail。映射 JSON、自检 `100/100`、changed-file regression gate、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. Owner 审计与设计决策

### 2.1 没有把 consumer 生命周期塞回错误 owner

审查结果表明：

- `FormationProductSession` 只拥有材料与部署事务；
- `FormationProductHost` 只拥有阵法形成、World delivery 与 influence ledger；
- `FormationInfluenceLifecycleCommandHost` 已拥有 consumer runtime、lifecycle command router、source receipt/重放历史、lease 顺序栅栏与 terminal drain 检查。

因此 P8.41 扩展既有 LifecycleCommandHost，而不是让 Session 或 GameMode 再持有一份 consumer 状态。这使产品调用点与实际 authority 对齐。

### 2.2 显式 Run activate/deactivate owner API

LifecycleCommandHost 新增：

- `TryActivateConsumerForRun(...)`；
- `TryDeactivateConsumerForRun(...)`。

两者是薄路由：activation 借用现有 CombatRun、注册对象、ProductHost、delivery 与 AttributeComponent，立即调用 P8.39 composition；deactivation 只消费 ProductHost 与 P8.39 返回的 pointer-free activation evidence，立即调用 P8.40 exact removal。

Host 不缓存第二份 composition receipt，也不保留 CombatRun、UObject 或 AttributeComponent 指针。返回结果仍是调用方持有的 pointer-free 审计证据。

## 3. 功能性与幂等语义

专项真实 fixture 通过新的 Host owner 入口验证：

- inactive CombatRun 与无效 registry alias 在 delivery 前失败关闭；
- foreign LifecycleCommandHost 因缺少对应 source receipt 而拒绝，不污染原 Host runtime 或目标 AttributeComponent；
- 相同 activation command 重放保持 forward-only registry alias 与 exact delivery identity；
- 篡改 activation evidence 的 deactivation 在触及 runtime 前拒绝；
- 正确 Host 使用 activation receipt 精确移除原 delivery；
- 相同 deactivation 重放返回既有结果，不增加 completed transaction；
- consumer runtime 排空后 CombatRun 才允许结束并清理 alias。

P8.41 没有改变 P8.39/P8.40 的状态机或身份规则，只把它们暴露在真正持有 consumer authority 的既有 Host 上。

## 4. 完整性与兼容性

- 保留所有既有 LifecycleCommandHost、RunComposition、ProductRuntime 与 ProductBridge API；
- 新 owner API 没有新增数据成员、容器、后台任务或第二套权威；
- 不在 GameMode、Tick、World scan 或组件发现逻辑中隐式接线；
- deactivation 继续允许 registry forward teardown 后依赖冻结 evidence 精确清理；
- 原有 LifecycleCommandHost `5/5`、CombatRunCoordinator `17/17`、WorldGameplay `10/10` 与 legacy Attributes `4/4` 均通过；
- 最终测试名称改为 `CommandHostOwnedLifecycle`，focused 与全量日志均由最终 Editor 二进制重新生成。

本阶段建立的是产品 owner API 边界，不宣称已经完成 GameMode、UI、PIE 或真实玩家输入接线。

## 5. 修改范围

生产代码：

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.cpp`。

验证与文档：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- 本 Report；
- 同名 Development Log。

提交前源码净变更为 `3` 个文件、`73` insertions、`22` deletions。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 6. Automation 证据

| 日志 | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `ConsumerRunComposition-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerRunComposition` | 1 | 0 | `B6BF3822B35137A17B4F30D1FDD3CB0BB0E9BD3A2274C321E3A41BFE2FCB2D6A` |
| `LifecycleCommandHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost` | 5 | 0 | `624C4698042E7B18AD8480955ED293E56176B717981B2A1724AA5CEC0866DC20` |
| `CombatRunCoordinator-final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 17 | 0 | `37FE24007FFCBE0EE2E993F53E2DB8B5CFD649442AD70BCCF618D940FA8E53B5` |
| `ConsumerWorldResolution-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerWorldResolution` | 1 | 0 | `5E9DA3DB568379AB3B06470C7FDEDF4B49DE30C543F967737CAA985C35AF62C9` |
| `ConsumerProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProductRuntime` | 4 | 0 | `416388637AAAC5B959B933FC5E73B0F068C7C531CCD9FBCFCE533DB0EA3E6EA7` |
| `ConsumerProductBridge-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProductBridge` | 5 | 0 | `CE3EDFEB9944645FF89FBAAE85FC00442E90211D49B106E32757D2B393DA2261` |
| `FormationProductHost-final.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 6 | 0 | `772AEAAEC41759E691AADB60E43023BB02D293F37791CE53B3814C86A0EC48B8` |
| `WorldGameplay-final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | `9A7AAE571E0ADAD7AF9BEB8BE95DD8A44B043F87A169AAB34E959B1E1C4DD7B2` |
| `Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | `B75C2D553B62CD766F8630279E5210619E5D09221DB1D74FAA59501F9585045A` |
| `Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 337 | 0 | `751EFB66DFF043CC633A8750C39A6C4725B09BC4CE6E53B02E0CE5EC93A46669` |

十份日志均有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0`；进程原生退出码均为 `0`。

## 7. Changed-file gate 与静态边界

```text
REGRESSION_MAP_JSON: PASS Rules=69
SELF_TEST: PASS 100/100
REGRESSION_COVERAGE (implementation): PASS Changed=3 Rules=2 Required=35 Logs=10
REGRESSION_COVERAGE (exact staged): PASS Changed=5 Rules=2 Required=35 Logs=10
EXACT_STAGE: PASS Files=5
```

- mapping SHA-256：`F64264C9186448E10283D6B43856F6EB525C6BCD49591A6F5306396903E55448`；
- self-test SHA-256：`E2CBB790626C47C56E2F18194FF6EE9F62B992C854057844E3DFAE47EE81975C`；
- 新 owner diff：`FindComponent=0`、`TActorIterator=0`、`GetSubsystem=0`、Tick/while `0`、`TMap=0`、RNG `0`；
- 新数据成员/持久容器/raw engine-object pointer member：`0`；
- `git diff --check` / `git diff --cached --check`：PASS。

## 8. 构建证据

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | SHA-256 |
|---|---|---:|---|
| Editor candidate | Succeeded / 9 actions / 48.16s | 0 | `0C09A52C6B393C1CBCD05FBCBBE2F499E3C1C9C041C01B306CB966AA29F49DC0` |
| Editor final | Succeeded / 4 actions / 6.76s | 0 | `B3CF8C6A162B92EC5183046DC0492A5C8B75C61C99AE744AF57E1F4A1C2B9FC2` |
| Game final | Succeeded / 8 actions / 35.69s | 0 | `77D647741B76B9FEFECC4D461E161E679DA6F4635A959AB9AA830B24E7C4450F` |

- `UnrealEditor-demo_map.dll`：`12145152` bytes，SHA-256 `69CF601BB87719DC8DF938823C9463CEC1B8B2019A32053DF9312DB786383603`；
- `demo_map.exe`：`353177600` bytes，SHA-256 `DD394185C67FDC2CF015F8CD9C750B6843930C52154AB93FB17520AB0A1F9CBB`。

## 9. 真实异常

本阶段没有源码、Automation、门禁或构建失败。首次与最终构建均直接成功；最终 focused 与 full Automation 是在测试描述改为 owner 语义后主动重跑，不是失败重试。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议下一阶段先审计 formation 创建/部署结束时现有的真实 command source，只有当该调用点已经同时持有 CombatRun、ProductHost、LifecycleCommandHost 与目标 capability 时，才调用 P8.41 owner API；若 capability 不完整，应向上游显式传递，而不是增加 GameMode 扫描、全局查找或 Tick 轮询。
