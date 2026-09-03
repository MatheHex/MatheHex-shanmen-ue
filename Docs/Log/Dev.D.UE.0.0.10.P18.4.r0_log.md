# Dev.D.UE.0.0.10.P18.4.r0 Development Log

## 1. 目标与基线

- 基线提交：`7f767e421b307f28a690d5e38bd9b1457f01f158`（P18.3 Sword Qi product route）；
- 分支：`agent/0.0.10-p18-4-sword-qi-run-controller`；
- 目标：以现有装备与属性权威为唯一来源，建立 Run-scoped Sword Qi 产品控制器，并接入 GameMode Run owner；
- 约束：仅做 P 阶段，不接真实输入、按键资产、装备 UI、正式视觉/声音、关卡投放或第二套装备/库存/属性/伤害/生命权威。

## 2. 起始审计与设计决定

审计了 Runtime `Fdemo_mapItemAuthority` 的 WeaponSlot 真值、属性组件的 derived final value、CombatRunCoordinator、P18.3 Product Authority/Session、GameMode 的 Run 激活/释放顺序，以及 Weapon Guard 与 Thrown Weapon 的既有 adapter/controller 模式。

本轮做出四项约束性决定：

1. 调用方不能传 item id；adapter 必须读取 WeaponSlot 的 exact occupant；
2. `SwordQiSource` 是独立显式语义，不能从 `WeaponGuard` 或 category 猜测；
3. 装备与 final AttackPower 只在新 IntentId 首次出现时采样一次，之后重放用冻结命令；
4. GameMode 只组合现有 authority/session/host，不增加库存写事务或平行动作仲裁器。

## 3. 生产源码

新增：

- `demo_mapShanmenSwordQiItemAdapter.h/.cpp`；
- `demo_mapShanmenSwordQiProductController.h/.cpp`。

修改：

- Item gameplay semantic 增加末尾 `SwordQiSource`，保持既有枚举序值；
- canonical catalog 为六个武器增加该语义并更新 P18.4 content identity；
- `Ademo_mapGameMode` 增加设备无关 route、interrupt/range/retire 入口，将控制器纳入 occupancy、Run begin/rollback/end/orphan/empty-state；
- 三个 identity/migration adapter 测试更新 current/historical content evidence；
- regression map 与 self-test 增加新路径规则及正反 fixture。

Item authorization 绑定 authority revision、exact instance、definition、WeaponSlot、content version/digest。Controller 捕获 IntentId、RunId、origin 与 normalized direction，首次请求再冻结 exact item、final AttackPower 与 P18.3 command。

## 4. 产品生命周期与不变量

控制器只在 exact Run 上 active。新 intent 成功捕获后可出现三种稳定结果：正常 route、可重放的 product rejection，或 HostBusy 下保留冻结 command 等待重试。相同 IntentId 的 payload 不一致会拒绝；相同 payload 重放不重采样、不重复 reserve。

Controller 把 Session 的飞行状态投影到既有 Player Action Occupancy。Run end 时若仍 in-flight，先 interrupt，再复制 terminal receipt、reset Session、清空 captured intent map 与 Run identity。GameMode teardown 中，此步骤被放在 Meridian Shock durable recovery 之后，避免在持久事务恢复前拆除其它产品 authority。

## 5. Automation 与 fixture

新增 Item Adapter 测试：

1. `CanonicalCatalog`；
2. `ExactAuthorization`；
3. `EquipmentFences`；
4. `RevisionFence`。

新增 Product Controller 测试：

1. `RouteAndRunEnd`；
2. `FrozenReplay`；
3. `BusyRetry`；
4. `Fences`。

Controller fixture 使用 unattended GamePreview world 与 NullRHI，不启动 Editor UI 或产品。测试覆盖 exact item、final derived AttackPower、唯一 sequence、占用投影、Run teardown、装备/属性变化后的冻结重放、HostBusy retirement/retry、无装备/foreign Run/占用 gate/IntentId conflict 等围栏。

## 6. 首败、定位与修正

首次 Product Controller exact 运行保存为 `automation_sword_qi_product_controller.log`：0/4、terminal exit -1、进程状态 255、SHA `1D3A672095A5DF61CF47A4B36AA6A44306A2C78BD0B72E8C14A71A55A497735E`。

四个测试同时在 fixture 构造处失败。定位结果是 fixture 调用了 `SetBaseValue(AttackPower, ...)`，但 AttackPower 在现有 attribute graph 中是派生属性，authority 正确失败关闭。只修改测试输入为 `Primary01`：6 产生 final AttackPower 7，98 产生 99，12 产生 13。生产控制器、属性图与伤害公式保持不变。`build_editor_fix1.log` 重编测试文件 4 actions、5.75s 后，exact retry 为 4/0。

随后静态复查发现 GameMode 初稿在 durable Meridian Shock recovery 前结束 Sword Qi Controller，违反既有 teardown 注释契约。移动该清理块后，`build_editor_fix2.log` 重编 GameMode 4 actions、10.32s，一次成功。该调整发生在完整/旧回归与最终 Game build之前。

Regression self-test 第一次按设计发现新 adapter/controller 的 positive fixture 缺少 legacy evidence；fixture 加入 `$Legacy` 后最终 283/283。没有放宽生产 coverage 规则。

## 7. 最终测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_sword_qi_item_adapter.log` | exact Item Adapter | 4/0 | `42305051323BDC2324CCBC90A26AB4F740BA338975780875A844EDE47AED3FDA` |
| `automation_sword_qi_product_controller_retry.log` | exact Controller | 4/0 | `E6CD2956B92A581BE55EDCC720C75967121198E03BBE38B6DA67C0EA39A056E8` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 773/0 | `73D48AB6BC2F148A333C46C5E9913FD0E58547DC8497CE930DA0FEF10A2CB088` |
| `automation_item_economy_schema.log` | legacy | 24/0 | `0C37D5BEA68AC17EBD16BFAF364E671FB29E3505F5B96968C0861C348056ECE4` |
| `automation_profile.log` | legacy | 211/0 | `D38B3D3D0B4D4B93BB0FCBD0232726DBA6FC5B19D833694329B88FE23A879F0D` |
| `automation_code_b.log` | legacy | 60/0 | `7C176FE778ED3CA72B9FE893167F104AA66B933963D4AD8268CECD510E04A881` |
| `automation_item_use_and_armor.log` | legacy | 46/0 | `BB773C276AD823B6AC8A20BE6497449AD5254FA0577F8FA13062371D11A9EBC0` |
| `automation_p4_hotbar.log` | legacy | 7/0 | `64E7CEBF71FA4870BE091A678AD5AD3BE95347B33300654B7EF3C7E0BF4780B4` |
| `automation_v3.log` | legacy parent | 29/0 | `897664DA4D342C0154EC4BCB6843A303A8122F3B9CA397F39B0FE2573C1E8D28` |
| `automation_enemy_skill_framework.log` | legacy | 44/0 | `CA06C5BE3CB6DCD957276AED1EDB1FFC7321E4A9D7CB23C3C0337C7E2AC004C4` |
| `automation_v2_ranged_compatibility.log` | legacy | 22/0 | `C25D0519929476C95C519FF52AC39100DDDFFA48D0DFD4904C8D9ECE137A3BAD` |

完整套件为 773/0，比 P18.3 的 765 精确增加 8；首末 Success 为 `2026.09.02 23:54:54.110 -> 2026.09.03 00:26:01.252 UTC`。八组 legacy 合计 443/0。所有采用日志有 terminal marker/exit 0，失败首跑没有进入 coverage evidence。

## 8. 改动驱动回归与构建

```text
REGRESSION_COVERAGE: PASS Changed=16 Rules=7 Required=56 Logs=11
SELF_TEST: PASS 283/283
BOUNDARY_SCAN: PASS Files=6 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage SHA：`E96A1D05270BBEBEE99B0E96440958C4267C05C163E0D805867643F2FA8B972E`；
- self-test SHA：`6382D19F1DD45D0763A9672CD732DC44D74EC520285F5510BCB2EBCFFE61781B`；
- map SHA：`B5FC5E1809918D8FF3EF5E4F137D29DE451D676246240B8FB4AD9AAB89D18CEF`；
- boundary SHA：`D1598293097DB92895DA0482B555258EA4D2F5F7ECD6DBE78F98844BEF3B2DDF`。

构建证据：

- Editor initial：198 actions、native success、543.21s、SHA `721A4BB277650FA0574DC7D9DB1851E006E72C1FAEBEC1A03418DE5FAFC6AB68`；
- Game final：197 actions、native 0、568.65s、SHA `2EC57CF215B9ABE85B062F556B49B878E4D073122F39213FDADF5CD28DCC409D`；
- Editor final：up to date、0 actions、native 0、1.31s、SHA `51BFC236F14A929C23984E35CC6ABF65D25F6C521692A5C9D98B9A551F2A865E`；
- `demo_map.exe`：356,381,696 bytes、SHA `24BD29C1178DF1FFA4C66B6A039ED676249DF9A56DD01C5A7E34A30D46CD3F43`；
- `UnrealEditor-demo_map.dll`：15,054,336 bytes、SHA `39C60B0306F8B66495096FDC2134188F4031CAC3ECF76C909CD757C380B36C0A`。

## 9. 提交边界与下一阶段

本轮计划提交 18 个文件：6 个新增源码、10 个修改文件、本 Report 与本 Development Log。未修改 Content、地图、资源、配置、Engine、Windows、save schema 或既有权威写路径。长期未跟踪的 0.0.9B Prompt/Report、旧交接资料、PDF、handoff 与用户资料保持未暂存；`Saved/Codex/P18.4` raw logs 不入 Git。

未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P18.5 建议建立薄型 Sword Qi command/input adapter，只生成 device-independent intent 并调用唯一 GameMode route；输入层不能读取装备、属性或伤害配置。正式按键资产与表现仍留给明确授权的 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-4-sword-qi-run-controller>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-4-sword-qi-run-controller/Docs/Report/Dev.D.UE.0.0.10.P18.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-4-sword-qi-run-controller/Docs/Log/Dev.D.UE.0.0.10.P18.4.r0_log.md>
