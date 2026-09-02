# Dev.D.UE.0.0.10.P18.3.r0 Development Log

## 1. 目标与基线

- 基线提交：`b8f14021ef53f299b04d5d88307e82321b610177`（P18.2 Sword Qi Run Host）；
- 分支：`agent/0.0.10-p18-3-sword-qi-product-route`；
- 目标：把现有玩家动作授权、Run 身份、Sword Qi 命令与 P18.2 Host 组合为最小产品路由；
- 约束：仅做 P 阶段，不接真实输入、装备 UI、正式视觉/声音、关卡投放或第二套库存/伤害/生命权威。

## 2. 起始审计与设计决定

审计了 CombatRunCoordinator、Player Action Arbitration、Sword Qi Execution/World Delivery/Run Host 及既有 Run owner 模式。P18.3 选择新增独立 Sword Qi activation sequence，避免与暗器或其它动作共享可变身份；Product Session 复用现有仲裁、Action runtime 与 Host，不建立平行动作队列。

外部采样的 item id、attack power、origin 与 direction 均在 reservation 前验证。命令冻结后只包含设备无关值，输入层不能在 route 期间重算伤害、速度、范围或身份。

## 3. 生产源码

新增：

- `Source/demo_map/demo_mapShanmenSwordQiProductAuthority.h/.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiProductSession.h/.cpp`。

修改：

- CombatRunCoordinator：Sword Qi reservation、独立单调 sequence 与 Run reset；
- Player Action Arbitration：新增显式 `SwordQi` kind 与持久占用验证。

Product Authority 提供 canonical P18.3 config、Run-issued reservation 与 immutable launch command。Product Session 提供结构围栏、一次 action authorization、惰性载体创建、Startup/Active commit、Host adoption、exact replay、payload conflict、occupancy projection、interrupt/range terminal 与 retirement。

Session 不查询或修改库存；exact source item id 必须由后续现有装备权威 adapter 提供。

## 4. Automation 与 fixture

新增 `Source/demo_map/demo_mapShanmenSwordQiProductSessionTests.cpp`：

1. `CanonicalAuthority`；
2. `RouteReplayAndOccupancy`；
3. `ConflictAndPayloadReplay`；
4. `SpawnFailureAndRunReset`。

测试验证 canonical config 稳定、无效外部值不消耗 sequence、一次授权、载体先惰性后发布、持久 Sword Qi 占用、exact replay 无 Actor I/O、同 ID 不同 payload 冲突、gate/spawn failure 持久化及 Run reset。Player Action Arbitration 测试同步加入 Sword Qi non-preemptible claim。

## 5. 编译与测试过程

首次 Editor build 一次通过：116 actions、native 0、356.10s，日志 SHA `F58C1431BC5EA5B8366C0F18A2632C51BFA6B6581F88CB4D620253B3C028C9B1`。

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_sword_qi_product_session.log` | exact Product Session | 4/0 | `0BCD48357AA480B6AC0F4B0692694208373490F23A07B51FB125C8B88A0B9A49` |
| `automation_player_action_arbitration.log` | exact arbitration | 4/0 | `99890E05FEFF2B47DBC40561AB0501DAA269010DC6B4EB12CFDFCE2B4A815828` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 765/0 | `87446800837E0ABE1A2C6A49FC5BC53BD0C5F7F3A6C361AFB235F82698AC7D14` |
| `automation_enemy_skill_framework.log` | legacy | 44/0 | `6478BFBF997808098B1F573D5CF6D400D493060614AA162BCAD27BB5C5E7AF0A` |
| `automation_item_use_and_armor.log` | legacy | 46/0 | `2F149B1DF4286E133471E83BC4C447E6E508570B38A101759BAD37E2B0AF3CCF` |
| `automation_v2_ranged_compatibility.log` | legacy | 22/0 | `7439CE010A43F7768AC2034D6C3C3B8D1AE0E8060AEB1849D90C626AE58C0760` |
| `automation_v3.log` | legacy parent | 29/0 | `536AD127E89096B05F0F67E366DD966BB09DA0FE5B1A6E279DD98BF0AE700249` |

完整套件首末 Success 为 `22:30:41.072 -> 23:00:59.956 UTC`，约 30m18.884s；765 条相对 P18.2 精确增加 4。

## 6. 改动驱动回归与门禁过程

修改 `Scripts/ShanmenRegressionMap.json`，新增 `SwordQiProductSession` 映射。修改 self-test，加入完整证据通过和缺失依赖证据失败用例，计数由 277 增至 279。

第一次 coverage 检查发现四组 legacy 证据缺失并失败关闭。补跑后，`demo_map.V3.Attributes` 两次 exact 日志虽然进程为 0、4/4 Success，但快速关闭导致日志没有 terminal marker；门禁两次均正确拒绝。随后运行健康父组 `demo_map.V3`，覆盖 Attributes 并带完整终止证据，最终通过。

```text
REGRESSION_COVERAGE: PASS Changed=12 Rules=3 Required=21 Logs=7
SELF_TEST: PASS 279/279
BOUNDARY_SCAN: PASS Files=10 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage SHA：`00260411621944ADB81A8A74A4F0CCAB322E6FFA89B0C9439CBD503E762F1ACF`；
- self-test SHA：`357A522A520897382DC78E92BB6243D788B3A0B2E12F623337EDD8F650DB9786`；
- map SHA：`A1715B5596909DC3C1509FC440AF4D2C6F5FCA5FAC70200CB027A54E8FFDD12E`；
- boundary SHA：`DB0CA745A4B37C1A3F879D15D284FC23E20A0BDBBFF815AC7D83FE62F6CF1134`；
- diff-check SHA：`6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`。

## 7. 最终构建与产物

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

- Game Development：115 actions、native 0、298.69s、log SHA `FF6D01B2877EB1E6485B7775EC619240DFE9DE83DA0811703423366EEF22603C`；
- Editor Development：up to date、0 actions、native 0、1.01s、log SHA `F287061EF32B583C9036B4E55BBD940E8BC09ED493B42328812AF9EE761D784D`；
- `demo_map.exe`：356,320,256 bytes、SHA `F2A54784AF565925DFB4E7ADA321856FFCB0F91B1CB30ED995C0752FCB02D3E1`；
- `UnrealEditor-demo_map.dll`：14,980,096 bytes、SHA `76D707DDC5DBB305788C34DA1DA7CA5E18B3A014AE5B5366AC24F79D94BF0129`。

构建仅验证编译与链接，没有启动产品。

## 8. 边界与提交范围

本轮计划提交 14 个文件：5 个新增源码、5 个修改源码、2 个回归门禁文件、本 Report 与本 Development Log。未修改 Content、地图、资源、配置、Engine、Windows、既有库存或生命权威。

未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。长期未跟踪的 0.0.9B Prompt/Report、旧交接资料、PDF、handoff 与用户资料保持未暂存；`Saved/Codex/P18.3` raw logs 不入 Git。

## 9. 下一阶段建议

P18.4 建立 Run-scoped Sword Qi 产品控制器，用单向 adapter 从现有装备权威取得 exact equipped sword instance，组合 P18.3 Authority/Session 并统一 terminal retirement。仍保持真实输入与表现后置，先完成唯一产品入口及 owner 生命周期。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-3-sword-qi-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-3-sword-qi-product-route/Docs/Report/Dev.D.UE.0.0.10.P18.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-3-sword-qi-product-route/Docs/Log/Dev.D.UE.0.0.10.P18.3.r0_log.md>
