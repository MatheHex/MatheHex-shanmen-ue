# Dev.D.UE.0.0.10.P21.10.r0 Report

## 1. 结论

P21.10 在 P 阶段边界内完成，结论为 **PASS**。

本轮为 P21.9 的实体飞剑闭环增加了只读 flight-phase/presentation cue。表现层现在可区分 `Orbiting`、`Directed`、`Returning` 与一帧 `Redeployed`，并在同一个 Actor 材质和同一个 HUD readout 上显示阶段；阶段仍由既有 Controller/Host/World lifecycle 的 gameplay 权威状态派生，表现层不拥有移动、碰撞、伤害、库存、状态迁移或时间。

```text
Controlled-weapon focused:                 58 Success / 0 Fail
Shanmen.0_0_10 full:                     1243 Success / 0 Fail
Required 0.0.9B compatibility groups:     224 Success / 0 Fail
Regression coverage:                       PASS (Changed=15 / Rules=8 / Required=84 / Logs=7)
Regression gate self-test:                 PASS 437/437
Game + Editor Development:                 PASS / native status 0
```

本轮未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是四阶段只读投影、真实 World Actor/HUD 消费边界、确定性身份、自动化与原生构建；不宣称玩家镜头下的视觉品质、文案观感、输入手感或音画反馈已验收。

## 2. 四阶段只读模型

新增 `Fdemo_mapShanmenControlledWeaponFlightReadModel`，只携带：

- Combat Run、item instance 与 activation 的确定性身份；
- `Orbiting`、`Directed`、`Returning` 或 `Redeployed` 阶段；
- 当前武器位置与返回锚点。

模型拒绝无效 GUID、非法阶段、非有限向量，以及“不在返回锚点却声称 Redeployed”的快照。它没有 mutation API，也不持有 Actor、计时器、碰撞、库存或 Combat authority。

Controller 从现有权威状态捕获模型：Orbiting 与 Directed 直接映射既有状态；Completed 且处于返航闭环映射 Returning；只有本帧通过原子换代并已位于锚点时才允许映射 Redeployed。下一帧同一新 activation 自动呈现 Orbiting，因此没有为 Redeployed 新建持久状态。

## 3. 唯一发布点与表现消费

GameMode 在本帧既有返回、换代、定向移动、环绕和威胁采样完成后，从 canonical lifecycle Actor 与 Run Host controller 捕获一次模型，再发布给同一个 Actor。Actor、controller 或 transform 不一致时失败关闭并输出精确错误；生产捕获和发布调用点各严格为一处。

Actor 只缓存已验证的 read model：

- Orbiting 使用既有待命蓝；
- Directed 使用出击橙；
- Returning 使用返航紫；
- Redeployed 使用归位浅绿；
- 近身威胁仍使用既有警戒绿覆盖阶段色；威胁清空后恢复当前阶段色。

HUD 复用既有飞剑 readout，不建立第二面板。即使近身目标为 0，也显示 `环绕待命`、`御剑出击`、`返航` 或 `已归位`；投影失败时继续复用既有屏内 fallback，并追加 `屏外`。Actor Tick 仍关闭，HUD 与材质均不能反向改变 gameplay。

## 4. 产品级自动化覆盖

新增 Controller `FlightReadModel` 测试，覆盖确定性 Orbiting、非锚点 Redeployed 拒绝并清空输出、锚点上一帧 Redeployed、下一次观察回到 Orbiting、Launch 后 Directed、Completed 后 Returning、终态伪造 redeploy 拒绝，以及身份/锚点失败边界。

canonical World lifecycle 测试扩展为 Returning → 原子换代 Redeployed → 下一观察 Orbiting → 第二次 Launch Directed，验证始终复用同一 Actor/item，新 activation identity 可观察。威胁 cue 集成测试验证威胁色覆盖与清空后阶段色恢复；readout 测试覆盖四种文案、零目标、屏外 fallback、非法阶段和负计数拒绝。

全量测试由 P21.9 的 1242 项精确增加到 1243 项；controlled-weapon focused 由 57 项增加到 58 项。

## 5. 自动化证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Controlled-weapon focused | 58 | 0 | 327,105 | `F03FA53A7C3D04A4942D65B1906A9D3BF6582DDDC37950330D1698EBAC83CF17` |
| `Shanmen.0_0_10` full | 1243 | 0 | 1,898,790 | `369C3CFFD60302F4ABB20AFEB2E144D7A418EF869FDC063D34B2DF8712597760` |
| `demo_map.V3.Attributes` | 4 | 0 | 264,361 | `B3FCD98580883C4D06243BEC85BAEC383E0AA612914196A5E1D941A9B57433C6` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 304,855 | `335E2F119DCB4909B5FEAB7E0EDB5D715DCF7717BED121DF0E64842C215BB54A` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 285,290 | `E6036AFFF4A254F50164F6AB103E9AAD897A037872C15A446ACB0C7F65A36A8B` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 310,380 | `21FD495021E89EDAE18A95123C9F6C115B13737102F4FB63E16F8A03D6F9CF48` |
| `demo_map.P4.Hotbar` | 7 | 0 | 267,479 | `7C0651BEB4CA61499D26F0AA036653541A6816A88EBD0EF11FDCE9BE3C39C531` |
| `demo_map.InputRestore` | 101 | 0 | 395,101 | `A1F48EFF3D0A32830E3ED5A9686D0E139FE9223B55DD14306AED60189ECF5154` |

full 从 `2026-09-08 09:25:59.159` 至 `10:37:17.274` 自然清空 1243 项并收到 native exit 0。六组旧版兼容合计 224/0。全部八份自动化日志各有且仅有一个实际 RunTests 命令、一个成功终止标记；Fail、Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

## 6. 覆盖门、静态审计与构建

首轮 changed-file gate 真实失败，精确指出缺少 `demo_map.InputRestore`；失败日志被保留，随后补跑该组 101/0。第二轮 gate 通过：`Changed=15 / Rules=8 / Required=84 / Logs=7`。

- 首轮 gate failure：345 bytes，SHA-256 `B1666433283C14CD233E84623DBD32617D7FCF678EF86402C618620403CFEEF3`；
- 最终 gate：10,410 bytes，SHA-256 `26ADFE651729F87113678303F74E37C0FFA911A24EBB32B58859B768FB46952D`；
- regression self-test：`PASS 437/437`，43,073 bytes，SHA-256 `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0`；
- implementation/test/mapping diff：15 files，`+631 / -27`；
- 新增生产行中的 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor`、`Destroy` 命中 0；
- 生产 `TryCaptureFlightReadModel` 与 `TryPresentFlightReadModel` 调用点各 1，均在 GameMode 的 canonical frame owner；
- `git diff --check` 通过，仅有工作树 LF→CRLF 提示；验证结束后无 UnrealEditor、UnrealEditor-Cmd 或产品进程残留。

| Target | Result | Actions / Time | Bytes | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded / native 0 | 65 / 49.93s | 6,884 | `78EEB66F5E28C0F550CCEBE71A34FC1B8BABEA341A934A28A44D7F9027D164A0` |
| Game Development final | Succeeded / native 0 | 64 / 52.80s | 6,678 | `2315E7B7A59637ADF1DE28D73E04C74E01A5911E99136BF2213BEF38F6F3BB8D` |
| Editor Development final | Succeeded / native 0 | 0 / 0.97s | 1,030 | `BFF4C9306D8F9CC6A8F2023BC88E84D8CF7F82E8FF6484C806545B88347A6C0E` |

最终产物：

- `demo_map.exe`：359,493,120 bytes，SHA-256 `F83D9EA4036E489D1445F4FE1215C7DFECF70818557043BABB6119C8380E9E0D`；
- `UnrealEditor-demo_map.dll`：18,672,640 bytes，SHA-256 `5D3D22FC60821567CFFA09869B423F1F14BC572AC2AF9DEA2447503F9FD90E4F`。

## 7. P/F 边界

PASS：四个 flight phase 均从既有权威状态只读派生；Redeployed 只持续一帧且必须位于返回锚点；Run/item/activation/transform 失败关闭；同一 Actor 材质和同一 HUD readout 可观察阶段；威胁覆盖清除后恢复阶段色；没有第二时钟、第二状态机、第二 Actor、第二伤害或库存路径；focused、full、兼容、覆盖门与双目标构建通过。

未声明：玩家镜头下颜色辨识度、HUD 排版与文案品质、返回/换代动画连续性、真实键鼠或手柄交互、PIE/Standalone/打包体验。当前阶段色是程序化材质参数，不包含新增美术资产、音效或特效。

## 8. 后续建议与 GitHub

下一轮应优先增加可直接影响玩法的受控飞剑编排，而不是继续堆叠交接或 wrapper：在现有单 Actor 闭环上建立明确的再次出击决策/输入语义，并继续复用唯一 Controller、Host、fixed timeline 与 read model。若需要玩家镜头下的视觉验收，则必须在单独授权的受控 PIE 轮次执行。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-10-flying-sword-phase-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-10-flying-sword-phase-presentation/Docs/Report/Dev.D.UE.0.0.10.P21.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-10-flying-sword-phase-presentation/Docs/Log/Dev.D.UE.0.0.10.P21.10.r0_log.md>
