# Dev.D.UE.0.0.10.P15.0.r0 Report

## 1. 结论

P15.0 已完成并通过 P 阶段门禁。

本阶段修复了一处真实的旧存档迁移数据丢失：`PromoteLegacyProfile` 原先会对所有受支持的旧 Schema 无条件把 `TownLevel` 清零。`TownLevel` 在 Schema 4 起已经是持久字段，因此 Schema 4、5、6 中合法的非零城镇等级会在升级到当前 Schema 7 时静默丢失。

修复后，只有没有该字段的 Schema 1—3 会补默认值 `0`；Schema 4—6 保留源存档中的合法 `TownLevel`。本轮没有提高 Schema 版本、改变 JSON 格式、增加迁移入口或建立第二套存档权威。

同时修复了两类会掩盖缺陷的回归假阳性：旧 Schema 4/6 测试使用全新档案的零值作基准，无法识别“无条件清零”；旧 Schema 5 空间戒指测试把硬编码的 Schema 6 token 替换为 5，而当前源档已是 Schema 7，替换实际没有发生。现在 Schema 4、5、6 都使用非零等级，并要求版本 token 精确替换一次。

最终证据：9 份聚焦日志 `427` 个 Success、0 Fail；0.0.10 全量 `708/708`；10 份日志合计 `1,135` 个 Success、0 Fail（组间有预期重叠）。changed-file regression gate、242 项门禁自检、迁移边界扫描、JSON、`git diff --check`、Game 与 Editor Development 构建全部通过。

## 2. 缺陷与修复

### 2.1 生产缺陷

旧实现完成其它 Schema 补偿后，无条件执行：

```cpp
Promoted.TownLevel = 0;
```

这会把 Schema 4—6 已序列化且已通过 `0—5` 合法性校验的等级覆盖为零。修复把默认补值限制在 Schema 1—3：

```cpp
if (Legacy.SchemaVersion <= 3)
{
    Promoted.TownLevel = 0;
}
```

因此迁移语义现在与字段年代一致：

- Schema 1—3：源格式没有 `TownLevel`，迁移补 `0`；
- Schema 4—6：源格式已有 `TownLevel`，迁移保留源值；
- Schema 7：当前格式不进入 legacy promotion；
- 未来或不受支持 Schema：继续由既有 fail-closed 读取边界拒绝。

### 2.2 不变量

本轮没有放宽 `ValidateProfile` 的等级范围，没有改变原子提交、备份、`SaveGeneration`、库存顺序或资源余额规则。迁移后仍必须满足：

- `SchemaVersion == CurrentSchemaVersion`；
- `TownLevel` 精确等于 Schema 4—6 源值；
- `PersistentSpiritStones`、`PermanentStash` 等既有状态保持；
- 一次迁移只推进一次 `SaveGeneration`；
- 备份保留迁移前的精确旧 Schema 字节。

## 3. 回归强化

### 3.1 Schema 4

`demo_map.ItemEconomySchema.22.SchemaFourMigration` 不再用默认 `TownLevel=0` 自比。测试先保存 `PersistentSpiritStones=404`、`TownLevel=4`，降级为 Schema 4，再断言迁移后两个非零值及库存/商店状态全部保留。

### 3.2 Schema 5 与 6

新增 `demo_map.ItemEconomySchema.23.SchemaFiveAndSixTownMigration`，对 Schema 5、6 分别建立非零 `TownLevel` 和灵石余额，逐一验证升级到当前 Schema 后不丢失。

既有 `Shanmen.0_0_10.Items.Migration.Schema6ToCurrent` 改为显式保存 `TownLevel=5`、`PersistentSpiritStones=606`，并继续检查一次 generation 推进、ordered item identities 与精确备份字节。

### 3.3 Schema 5 空间戒指 fixture

空间戒指迁移测试现在从 `CurrentSchemaVersion` 动态构造源 token，并要求下列每次文本变换的返回计数严格等于 `1`：

- 当前 Schema → Schema 5；
- legacy accessory 字段写入戒指 identity；
- 当前 spatial-ring 字段移除。

测试同时保存并验证非零 `TownLevel=4`。如果未来 Schema 再变化、fixture 结构漂移或字段没有实际替换，测试会直接失败，而不会把当前档案误当成旧档案通过。

## 4. 回归覆盖门禁

首次按改动文件运行门禁时，它正确 fail closed：`demo_mapP1R6SpatialRingTests.cpp` 尚无映射，不能证明对应回归已经执行。

本轮为该文件增加精确规则：

```text
Source/demo_map/demo_mapP1R6SpatialRingTests.cpp
  -> demo_map.P1R6.SpatialRing
```

并增加两个流程自检：精确组证据必须通过；无关的 0.0.10 全量证据不能替代该 legacy migration 组。最终门禁结果：

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=4 Required=8 Logs=10
SELF_TEST: PASS 242/242
```

八个由改动路径推导出的必跑组全部有健康原生日志：`demo_map.Profile`、`demo_map.ItemEconomySchema`、`demo_map.P1R6.SpatialRing`、`Shanmen.0_0_10.Items`、`demo_map.ItemUseAndArmor`、`demo_map.P4.Hotbar`、`demo_map.CodeB`、`demo_map.V3.WorldInteraction`。

## 5. 修改范围

- `demo_mapProfileRepository.cpp`：把 `TownLevel=0` 默认补值限制为 Schema 1—3；
- `demo_mapItemEconomySchemaTests.cpp`：非零 Schema 4 基准，新增 Schema 5/6 非零迁移 contract；
- `demo_mapP1R6SpatialRingTests.cpp`：动态 Schema token、精确替换计数和非零等级保存；
- `demo_mapShanmenItemMigrationTests.cpp`：强化 Schema 6 非零状态、generation、库存和备份断言；
- `ShanmenRegressionMap.json`：增加 P1R6 spatial-ring migration 精确映射；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：增加正例与无关证据反例；
- Report/Log 之前 6 个生产、测试、流程文件净变更 `+113 / -21`。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `demo_map.ItemEconomySchema` | 24 | 0 | 0 | `16B8D09FAEB7A8B68C15DD76B4CAB914571187D1A3C875F70B4AE9BA30D82D1E` |
| `demo_map.P1R6.SpatialRing` | 2 | 0 | 0 | `777AD87E2F4D6A3ED5F175C12CFC1CB21F278B240076C4A4AC5F71603BC327FF` |
| `Shanmen.0_0_10.Items.Migration.Schema6ToCurrent` | 1 | 0 | 0 | `738655C97FD9398771702CC13B50F31075A6B5199B0434C6EB6D5726C8C4FE2D` |
| `demo_map.Profile` | 211 | 0 | 0 | `142BA2B7E3BA9C0468A81D9DB9370012D2B3B39AD62093D7B909BB4A30BB5593` |
| `Shanmen.0_0_10.Items` | 72 | 0 | 0 | `487FADC2C7A9EE819BF3E90E84613B93098FDCCAA8D6770E47F1F20D9F32C5BE` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `BE39ADAB0854FEA0D0E86583769A8B93864CE758283F7C40A278B072F9F43B91` |
| `demo_map.P4.Hotbar` | 7 | 0 | 0 | `765618C0A4F48DBF6D99EF6D1A0076E875F8E2628CCCD83D787AEDC6549E57D4` |
| `demo_map.CodeB` | 60 | 0 | 0 | `E33DD809C7B94CB699CA9F384A70C4C61346FE47FA5990ED73E8A662729CBE4D` |
| `demo_map.V3.WorldInteraction` | 4 | 0 | 0 | `97A5F4EB31ED0DC2C6DB08D2CAB60BF07CD9C5423412497145CC08477CC8D53C` |
| `Shanmen.0_0_10` | 708 | 0 | 0 | `556491C890E1790BABA9DDBB7F5DC46C7582428DE275695B83B2EF9BF63C53DC` |

九份聚焦日志合计 `427` 个 Success、0 Fail；全量为 `708/708`；十份日志合计 `1,135` 个 Success、0 Fail，组间有预期重叠。全部日志均有 terminal completion evidence，原生退出 `0`。

全量首末 Success 时间为 `12:38:33.764 -> 13:06:21.454`，约 `27m47.69s`。全量日志中 Fatal error、Unhandled Exception、Ensure condition failed、AutomationController Error 与 Result Fail 均为 `0`。

## 7. 静态门禁

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=4 Required=8 Logs=10
SELF_TEST: PASS 242/242
MIGRATION_BOUNDARY_SCAN: PASS ResetSites=1
JSON_PARSE: PASS Rules=141
git diff --check: PASS (native exit 0)
```

唯一 `Promoted.TownLevel = 0` 写点位于 `Legacy.SchemaVersion <= 3` 条件内。没有新增 Schema、序列化字段、repository、subsystem、Actor、World 查询、RNG、timer、damage 或存档写入口。

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 7 / 23.96s | 0 | `4D45F9857B087F38D9FABC6C13AED41FFD90399D257E876D532486D53A41A7E6` |
| Game final | Succeeded | 6 / 22.14s | 0 | `9AAF89709A2315D107732C92E71D9797E3F6A012A63EFF80303CBB4B4A588EA4` |
| Editor final | Succeeded, up to date | 0 / 0.94s | 0 | `608BD07AD33BE497A8BC2BA0407CCCC64568C84C7AA53BB32E75344B203A1DF1` |

最终产物：

- `UnrealEditor-demo_map.dll`：14,281,216 bytes，SHA-256 `39FEA44E953626048A4A9278C59E3D52425F9B72CC3EA314FEE4CDBC47DAD9BF`；
- `demo_map.exe`：355,706,880 bytes，SHA-256 `48D1E735CFD71B5930EF0449402484EA3E3050DA57795E049F175FC5BA8A2F6D`。

## 9. 异常与修复记录

本阶段没有源码编译失败、Automation 失败、Windows commit-memory/pagefile 故障或构建重试。Editor initial、全部选定 Automation、Game final 与 Editor final 均首次成功。

唯一有意的门禁失败发生在测试映射审计：首次检查发现 `demo_mapP1R6SpatialRingTests.cpp` 未映射，因此拒绝通过。增加精确映射与正反自检后，门禁以 8 个必跑组完整通过；没有用 broad 0.0.10 证据替代 legacy migration 组。

全量 Automation 期间 UE 后台 connectivity probe 对 `https://www.google.com/generate_204` 记录超时 warning，使一组重试/恢复 contract 执行较慢。该探测不属于产品测试路径；测试队列继续执行并以 `708/708`、terminal success 与 native exit `0` 完成。raw Automation/build 日志仅本地保留，Report 记录 SHA-256 摘要。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段存档迁移修复、fixture 与自动化门禁强化、NullRHI 无头 Automation、静态审查和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P15.0 已把用户提出的三项低成本缺口落到可执行证据：Schema 4—6 非零迁移、非恒真基准、改动文件到回归组的精确匹配。下一阶段可以继续检查其它 legacy fixture 是否仍以默认零值自比，优先处理能造成真实持久状态丢失的路径；不得仅为增加测试数量而复制无差异 contract。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-0-profile-migration-regression-hardening>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-0-profile-migration-regression-hardening/Docs/Report/Dev.D.UE.0.0.10.P15.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-0-profile-migration-regression-hardening/Docs/Log/Dev.D.UE.0.0.10.P15.0.r0_log.md>
