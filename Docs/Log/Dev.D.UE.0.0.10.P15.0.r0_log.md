# Dev.D.UE.0.0.10.P15.0.r0 Development Log

## 1. 目标

修复 Schema 4—6 profile 迁移时非零 `TownLevel` 被无条件清零的持久状态缺陷；补齐 N−1/旧 Schema 非零迁移 contract，消除恒真基准与失效 Schema token fixture，并让改动文件自动推导对应必跑测试组。

## 2. 实现

- `PromoteLegacyProfile` 仅对没有 `TownLevel` 字段的 Schema 1—3 补默认值 `0`；
- Schema 4—6 保留源存档中已校验的非零 `TownLevel`；
- Schema 4 contract 使用 `TownLevel=4`、`PersistentSpiritStones=404`；
- 新增 Schema 5/6 非零迁移 contract；
- Schema 6 Items migration 使用 `TownLevel=5`、`PersistentSpiritStones=606`，继续验证一次 generation、ordered stash 与精确备份；
- Schema 5 spatial-ring fixture 从当前 Schema 动态降级，三个 JSON 变换都要求精确命中一次，并验证非零等级；
- regression map 新增 P1R6 spatial-ring exact mapping；
- regression self-test 新增精确组正例和无关 broad evidence 反例；
- 没有 Schema bump、JSON 格式变化、新 repository/subsystem 或第二套存档 authority。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| ItemEconomySchema | 24 | 0 | 0 | `16B8D09FAEB7A8B68C15DD76B4CAB914571187D1A3C875F70B4AE9BA30D82D1E` |
| P1R6.SpatialRing | 2 | 0 | 0 | `777AD87E2F4D6A3ED5F175C12CFC1CB21F278B240076C4A4AC5F71603BC327FF` |
| Schema6ToCurrent | 1 | 0 | 0 | `738655C97FD9398771702CC13B50F31075A6B5199B0434C6EB6D5726C8C4FE2D` |
| Profile | 211 | 0 | 0 | `142BA2B7E3BA9C0468A81D9DB9370012D2B3B39AD62093D7B909BB4A30BB5593` |
| Items | 72 | 0 | 0 | `487FADC2C7A9EE819BF3E90E84613B93098FDCCAA8D6770E47F1F20D9F32C5BE` |
| ItemUseAndArmor | 46 | 0 | 0 | `BE39ADAB0854FEA0D0E86583769A8B93864CE758283F7C40A278B072F9F43B91` |
| P4.Hotbar | 7 | 0 | 0 | `765618C0A4F48DBF6D99EF6D1A0076E875F8E2628CCCD83D787AEDC6549E57D4` |
| CodeB | 60 | 0 | 0 | `E33DD809C7B94CB699CA9F384A70C4C61346FE47FA5990ED73E8A662729CBE4D` |
| V3.WorldInteraction | 4 | 0 | 0 | `97A5F4EB31ED0DC2C6DB08D2CAB60BF07CD9C5423412497145CC08477CC8D53C` |
| full 0.0.10 | 708 | 0 | 0 | `556491C890E1790BABA9DDBB7F5DC46C7582428DE275695B83B2EF9BF63C53DC` |

聚焦 `427` Success；全量 `708/708`；十份日志合计 `1,135` Success、0 Fail，组间有预期重叠。全部 terminal complete、native exit `0`。全量约 `27m47.69s`；Fatal/Unhandled/Ensure/Controller Error 均为 `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=4 Required=8 Logs=10
SELF_TEST: PASS 242/242
MIGRATION_BOUNDARY_SCAN: PASS ResetSites=1
JSON_PARSE: PASS Rules=141
git diff --check: PASS (native exit 0)
```

首次 coverage 检查正确拒绝未映射的 `demo_mapP1R6SpatialRingTests.cpp`；增加 exact mapping 与正反自检后通过。没有用 broad full suite 替代 legacy spatial-ring group。

## 5. 构建

- Editor initial：7 actions / 23.96s / exit `0` / log SHA `4D45F9857B087F38D9FABC6C13AED41FFD90399D257E876D532486D53A41A7E6`；
- Game final：6 actions / 22.14s / exit `0` / log SHA `9AAF89709A2315D107732C92E71D9797E3F6A012A63EFF80303CBB4B4A588EA4`；
- Editor final：up to date / 0 actions / 0.94s / exit `0` / log SHA `608BD07AD33BE497A8BC2BA0407CCCC64568C84C7AA53BB32E75344B203A1DF1`；
- Editor DLL：14,281,216 bytes / SHA `39FEA44E953626048A4A9278C59E3D52425F9B72CC3EA314FEE4CDBC47DAD9BF`；
- Game EXE：355,706,880 bytes / SHA `48D1E735CFD71B5930EF0449402484EA3E3050DA57795E049F175FC5BA8A2F6D`。

## 6. 范围与边界

Report/Log 前 6 个生产、测试、流程文件净变更 `+113/-21`。长期未跟踪用户文件未修改或提交；raw logs 仅本地保存。本轮仅 P 阶段，未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-0-profile-migration-regression-hardening>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-0-profile-migration-regression-hardening/Docs/Report/Dev.D.UE.0.0.10.P15.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-0-profile-migration-regression-hardening/Docs/Log/Dev.D.UE.0.0.10.P15.0.r0_log.md>
