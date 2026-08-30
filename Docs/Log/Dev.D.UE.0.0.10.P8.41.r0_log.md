# Dev.D.UE.0.0.10.P8.41.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.41.r0`；
- 基线：`b00d5e240ea9911418bc924f6e95327e32ef6b1c`（P8.40）；
- 分支：`agent/0.0.10-p8-41-formation-consumer-lifecycle-owner`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

把 P8.39 activation 与 P8.40 exact deactivation 接入一个已经拥有 formation consumer runtime、receipt 与重放历史的产品生命周期 owner；不创建第二套 Session、ledger、registry、controller 或 pointer retention。

## Owner 审计

1. `FormationProductSession` 只拥有材料与部署，缺少 consumer runtime/receipt capability。
2. `FormationProductHost` 只拥有阵法形成、World delivery 与 influence ledger，也不是 consumer command owner。
3. `FormationInfluenceLifecycleCommandHost` 已拥有 consumer runtime、lifecycle router、source receipt/幂等历史、lease ordering 与 terminal drain guard。
4. 选择扩展现有 LifecycleCommandHost；拒绝把状态复制到 Session，拒绝新增 GameMode glue、扫描或轮询。

## 实现记录

### `TryActivateConsumerForRun`

新增 LifecycleCommandHost owner 入口，借用 CombatRun、已注册对象、ProductHost、exact delivery 与 AttributeComponent，内部直接调用 P8.39 `RunComposition::TryActivate`。Host 自身作为唯一 lifecycle capability 传入，不缓存返回 evidence。

### `TryDeactivateConsumerForRun`

新增对称 owner 入口，只接收 ProductHost 与 pointer-free activation evidence，内部直接调用 P8.40 `RunComposition::TryDeactivate`。清理不要求 World registry 仍然存活。

### 验证迁移

现有真实 fixture 的所有 direct composition 调用改为通过 CommandHost owner；测试叶名称改为 `CommandHostOwnedLifecycle`。原拒绝、forward alias、activation/replay、无效/foreign deactivation、exact deactivation/replay 与 CombatRun end 断言全部保留。

## 执行序列

1. 审查 Formation Session、ProductHost、LifecycleCommandHost 与成熟 Run lifecycle 模式。
2. 选择既有 LifecycleCommandHost 为唯一 owner，设计零新增状态的薄路由。
3. 增加两个 owner API，并迁移 composition fixture。
4. Editor candidate 单并发构建成功。
5. focused owner `1/1`、LifecycleCommandHost `5/5` 后顺序执行映射与全量 Automation。
6. 共 `390/390` success；映射、自检、changed-file gate、静态扫描与 diff check 通过。
7. Editor final 与 Game final 单并发构建成功。
8. 用最终 Editor 二进制重跑 focused `1/1` 与 full `337/337`，锁定最终测试名称证据。
9. 生成 Report/Log，执行 exact-stage、staged gate、commit 与 push。

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `ConsumerRunComposition-final.log` | 1 | 0 | yes | 0 | `B6BF3822B35137A17B4F30D1FDD3CB0BB0E9BD3A2274C321E3A41BFE2FCB2D6A` |
| `LifecycleCommandHost-final.log` | 5 | 0 | yes | 0 | `624C4698042E7B18AD8480955ED293E56176B717981B2A1724AA5CEC0866DC20` |
| `CombatRunCoordinator-final.log` | 17 | 0 | yes | 0 | `37FE24007FFCBE0EE2E993F53E2DB8B5CFD649442AD70BCCF618D940FA8E53B5` |
| `ConsumerWorldResolution-final.log` | 1 | 0 | yes | 0 | `5E9DA3DB568379AB3B06470C7FDEDF4B49DE30C543F967737CAA985C35AF62C9` |
| `ConsumerProductRuntime-final.log` | 4 | 0 | yes | 0 | `416388637AAAC5B959B933FC5E73B0F068C7C531CCD9FBCFCE533DB0EA3E6EA7` |
| `ConsumerProductBridge-final.log` | 5 | 0 | yes | 0 | `CE3EDFEB9944645FF89FBAAE85FC00442E90211D49B106E32757D2B393DA2261` |
| `FormationProductHost-final.log` | 6 | 0 | yes | 0 | `772AEAAEC41759E691AADB60E43023BB02D293F37791CE53B3814C86A0EC48B8` |
| `WorldGameplay-final.log` | 10 | 0 | yes | 0 | `9A7AAE571E0ADAD7AF9BEB8BE95DD8A44B043F87A169AAB34E959B1E1C4DD7B2` |
| `Attributes-final.log` | 4 | 0 | yes | 0 | `B75C2D553B62CD766F8630279E5210619E5D09221DB1D74FAA59501F9585045A` |
| `Shanmen-0_0_10-final.log` | 337 | 0 | yes | 0 | `751EFB66DFF043CC633A8750C39A6C4725B09BC4CE6E53B02E0CE5EC93A46669` |

最终证据合计 `390` 条 success、`0` fail。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=69
SELF_TEST: PASS 100/100
REGRESSION_COVERAGE (implementation): PASS Changed=3 Rules=2 Required=35 Logs=10
REGRESSION_COVERAGE (exact staged): PASS Changed=5 Rules=2 Required=35 Logs=10
EXACT_STAGE: PASS Files=5
git diff --check / git diff --cached --check: PASS
```

- mapping SHA-256：`F64264C9186448E10283D6B43856F6EB525C6BCD49591A6F5306396903E55448`；
- self-test SHA-256：`E2CBB790626C47C56E2F18194FF6EE9F62B992C854057844E3DFAE47EE81975C`；
- 新 diff 中 discovery、Tick/while、container authority 与 RNG 命中均为 `0`；
- 新数据成员与持久 raw engine-object pointer member 为 `0`。

## 构建证据

| Build | Actions | Time | Exit | SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | 9 | 48.16s | 0 | `0C09A52C6B393C1CBCD05FBCBBE2F499E3C1C9C041C01B306CB966AA29F49DC0` |
| Editor final | 4 | 6.76s | 0 | `B3CF8C6A162B92EC5183046DC0492A5C8B75C61C99AE744AF57E1F4A1C2B9FC2` |
| Game final | 8 | 35.69s | 0 | `77D647741B76B9FEFECC4D461E161E679DA6F4635A959AB9AA830B24E7C4450F` |

- `UnrealEditor-demo_map.dll`：`12145152` bytes，SHA-256 `69CF601BB87719DC8DF938823C9463CEC1B8B2019A32053DF9312DB786383603`；
- `demo_map.exe`：`353177600` bytes，SHA-256 `DD394185C67FDC2CF015F8CD9C750B6843930C52154AB93FB17520AB0A1F9CBB`。

## 真实异常

无源码、Automation、门禁或构建失败。focused 与 full 的最终重跑用于让证据与测试叶名称的最终文本完全一致，不属于错误重试。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
