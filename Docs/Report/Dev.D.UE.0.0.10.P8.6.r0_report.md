# Dev.D.UE.0.0.10.P8.6.r0 Report

## 1. 结论

P8.6 已建立 Formation 的窄 World coverage sampler，结论为 **PASS**。

新增 `Fdemo_mapShanmenFormationWorldCoverageSampler`：调用方显式提供一个 World、P8.5 immutable area、既有 `FShanmenWorldEntityRegistry` 与有限 Actor 子集；sampler 同步读取 stable EntityId 和当前有限坐标，canonical 化为 P8.5 query，再返回不含 UObject pointer 的 coverage evidence。

最终 focused `4/4`、0.0.10 full `235/235`、changed-file gate、45/45 映射自测、Editor Development 与 Game Development 均通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. World-to-pure 边界

Sampler 是一次性、无状态同步桥：

- 不扫描 World，也不隐式发现 Actor；
- 不持有 World／Actor／Registry pointer；
- 不 Tick、不建 timer、不订阅 overlap；
- 不写 Registry、不施加 damage、Buff、effect 或库存变更；
- 返回值只保存 stable EntityId、值类型坐标及 P8.5 immutable receipts。

调用方仍拥有采样 cadence、Actor 子集和 area 与 World 的业务配对。P8.6 只验证 live Actor 确实属于传入 World；P8.5 area 本身没有 World identity，因此本层不伪造无法证明的 area-to-World 绑定。

## 3. 身份、World 与失败关闭

一次成功 sample 必须同时满足：

- area 自验证通过；
- World live；
- Registry 已开启并与 area 属于同一 Run；
- source subset 非空；
- 每个 Actor live、属于同一 World，并存在 exact actor-wide Registry binding；
- 每个 stable EntityId 在本批中唯一；
- 每个 Actor location 的 XYZ 都是有限值。

失败状态区分 area、World、Registry、Run、空 source、Actor 生命周期、跨 World、未注册、重复 entity、非法坐标与 P8.5 拒绝。任一输入失败时不发布部分 coverage。

## 4. 确定性与不可变输出

Actor 输入顺序不会进入 receipt identity。Sampler 先按 stable EntityId canonical 排序，再调用 P8.5；同一批 identity 与 exact location 重放得到同一 coverage receipt。Actor 移动后重新采样会产生新的 query location 与 receipt identity，但 area identity 保持不变。

`Fdemo_mapShanmenFormationWorldCoverageResult::IsSuccess()` 会核对 canonical query 顺序、membership 数量、entity identity 与 exact location，防止返回值被改写后继续冒充成功 evidence。

## 5. 自动化证据

最终日志均为 one command、one queue-empty、Fail `0`、fatal／unhandled／ensure `0`，进程原生退出码 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationWorldCoverage` / `FormationWorldCoverage.log` | 4 | 0 | `BDF0BAE0B8181F70FBED20B7AC1EE46E5E4FEDFAD4334445E2C4B380C0F4FD89` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 235 | 0 | `7CD8C924B925684B5F1B80962801CF1B3A4CF88794263D6398D120A1A1EBAD1C` |

四项 focused 测试覆盖：

1. shuffled Actor subset、canonical entity order、Inside／Boundary／Outside 与 Registry 不变；
2. live location 变化会改变 coverage identity，未变化重放保持 identity；
3. inactive／cross-Run Registry、空 source、null／unregistered Actor 与 duplicate stable entity；
4. invalid area、null World、cross-World Actor 与 destroyed Actor。

完整 suite 从 P8.5 的 `231` 增至 `235`，此前测试全部继续通过。

## 6. 首次失败与修正

首次 focused 运行的进程退出码为 `0`，但 Automation 结果为 `2 Success / 2 Fail`；首次日志已保留为 `FormationWorldCoverage-first.log`，SHA-256：`98F3F62681166045CF0B6BCAE7383596E9B80EA1C9C12F36D97A1CF4C554F1CC`。

根因是测试夹具直接 spawn 裸 `AActor`。该类没有 root component，spawn transform 与后续 `SetActorLocation` 无法形成可读 live location，三个几何样本都退化为原点并被正确判为 Boundary。修正只发生在测试夹具：给测试 Actor 增加最小 transient `USceneComponent` transform root，并在构造时验证实际位置。之后源码重编译成功，focused `4/4`、full `235/235`。

该失败不是 production sampler 算法、内存、页面文件或环境故障；Report 同时记录进程退出码与测试 case 结果，不把 exit `0` 误写成测试通过。

## 7. 改动—回归与静态门禁

新增 `FormationWorldCoverage` mapping rule，要求六组证据：WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment。

- regression map JSON：PASS，`42` rules；
- mapping self-test：`45/45 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=6 Logs=2`；
- map SHA-256：`90678CB6309F4CD42A22924E803A270597879EE543625451DE5FAB9F6B67D6EA`；
- self-test SHA-256：`C7ECB6929254910A1F1CC4EC981FBCFF15D59BF48DC2F7656D7AD6B535391EF4`；
- production implementation 无 Actor／World ownership field，无 UPROPERTY／TObjectPtr／TWeakObjectPtr，无 discovery、Tick／timer、RNG、damage／effect、input／UI 或 item subsystem 调用；
- `git diff --check` 与最终 staged `git diff --cached --check`：PASS。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Native exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source rebuild after fixture correction | Succeeded | 0 | 6.65s | 后续 final check 覆盖 |
| Editor final check | Succeeded / up to date | 0 | 0.92s | `A0DEEA350D86123775D1B866EDF040E17130221473044FC4B2A6211492927A60` |
| Game final | Succeeded | 0 | 19.37s | `8CE7A012620FE11167B15F307B8F9AAAE1BF99340C7E19BADD38489AD24D1300` |

- Editor module：`11085312` bytes，UTC `2026-08-29T17:57:55.7152016Z`；
- Game executable：`352295424` bytes，UTC `2026-08-29T18:00:29.6848910Z`。

## 9. 修改范围、兼容性与 P/F 边界

新增：

- `demo_mapShanmenFormationWorldCoverageSampler.h/.cpp`；
- `demo_mapShanmenFormationWorldCoverageSamplerTests.cpp`。

更新 regression map、自测、本 Report 与同名 Development Log。没有修改 P8.0—P8.5、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链。长期未跟踪用户与 0.0.9B 工件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一步与 GitHub

P8.7 建议建立纯 coverage transition reducer：显式比较同一 area／entity 的前后 immutable membership receipts，产出 Enter／Stay／Leave facts；cadence、效果策略和 Actor 生命周期仍留在外层，不在 reducer 内引入 Tick 或 Gameplay effect。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-6-formation-world-coverage/Docs/Report/Dev.D.UE.0.0.10.P8.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-6-formation-world-coverage/Docs/Log/Dev.D.UE.0.0.10.P8.6.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-6-formation-world-coverage>
