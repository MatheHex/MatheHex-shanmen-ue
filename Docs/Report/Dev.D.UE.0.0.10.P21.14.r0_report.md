# Dev.D.UE.0.0.10.P21.14.r0 Report

## 1. 结论

P21.14 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P21.13 已有的权威命中回执继续投影到飞剑自身材质：飞剑只有在同一 Activation 的真实生命值提交已经完成、且飞行状态进入 `Returning` 时，才显示命中结果色。下一次激活或产品停用会沿既有生命周期清除反馈。

```text
Controlled-weapon focused:                    62 Success / 0 Fail
Shanmen.0_0_10 full:                        1247 Success / 0 Fail
Changed-file regression coverage:             PASS (3 files / 3 rules / 17 groups)
Regression gate self-test:                    PASS 437/437
Game + Editor Development:                    PASS / native status 0
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。因此本轮证明代码路径、状态优先级、生命周期、回归兼容和编译闭合，不宣称实际画面观感已经人工验收。

## 2. 玩家可见结果

已有飞剑网格继续复用同一个动态材质，不新增 Actor、HUD 面板或材质状态权威。返航阶段按权威提交结果选择颜色：

- 正伤害命中：亮金黄色；
- 正伤害且目标生命值归零：红橙色；
- 已提交但实际伤害为零：浅色中性提示；
- 没有当前命中回执：继续使用原有阶段色或近身威胁色。

命中回执在 `Directed` 阶段先到达时不会提前变色；同一激活进入 `Returning` 后才生效，避免把接触检测瞬间误表示为已完成结果。

## 3. 单一事实链

本轮没有重新计算伤害，也没有根据碰撞计数猜测结果。材质只读取 P21.13 已保存到当前飞剑 Actor 的 canonical vitality commit 快照：

```text
RunHost canonical delivery
  -> current ControlledWeaponActor committed-impact snapshot
  -> current flight read model phase fence
  -> existing dynamic material Color / BaseColor
```

`PresentedImpactAppliedDamage` 与 `PresentedImpactTargetVitalityAfter` 仍来自目标生命值权威提交回执。材质表现不写目标状态，不生成第二个 Impact，也不改变 ledger、resolver 或 RunHost。

## 4. 状态优先级与生命周期

`GetResolvedPresentationColor()` 的顺序现在为：

1. 同一激活的 committed impact 且飞剑处于 `Returning`；
2. 既有近身威胁提示；
3. 既有 Directed、Returning、Redeployed 或 Orbiting 阶段色；
4. 无有效 read model 时的 idle 色。

这样可防止阻挡接触后残留的 threat 状态盖住真正的命中结果；没有 committed impact 时，原有 threat-over-phase 行为完全保留。威胁点光源仍只由 threat 状态控制，命中反馈只改变既有材质颜色。

反馈不使用 Timer。它的有效期严格等于当前 Activation 的返航生命周期；新的 Activation read model 或 `DeactivateProductCollision()` 会使用既有清理点清空快照并刷新材质。

## 5. 测试覆盖

真实阻挡接触测试继续使用实际 `Ademo_mapShanmenControlledWeaponActor`、实际 enemy vitality authority 和实际 RunHost delivery，并新增断言：

- committed impact 在旧 `Directed` read model 下不能激活颜色；
- 精确回执重放仍幂等；
- 捕获并发布同一激活的 `Returning` read model 后，命中提示激活；
- 本次真实正伤害命中解析为精确亮金黄色；
- 下一 Activation 清除旧反馈并离开命中颜色。

专项组保持 62 项，因为本轮扩展现有端到端用例而不是增加重复 fixture。结果为 `62 Success / 0 Fail`。

## 6. 自动化证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Controlled-weapon focused | 62 | 0 | 332,544 | `19325B6B7195C4E5F16B9E940C9933AA3A846A8FFC8739ACAC746473EA3CA619` |
| `Shanmen.0_0_10` full | 1247 | 0 | 1,907,220 | `8291ADF0520FBBA41054083024B2BC3837C655898C80249ED71F93AD18526E5F` |

两份自动化日志各只有一个实际 RunTests 命令和一个成功终止标记，`Result={Fail}`、Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。全量从 `2026.09.08-17.11.04:455` 运行至 `2026.09.08-18.25.02:221`，自然完成，原生退出码为 0。

本轮产品自动化与构建均首次通过，没有产品失败日志。日志里的预期负向测试会输出 `Condition failed` 测试事件，但最终结果均为 `Result={Success}`，未将其误计为产品失败。

## 7. 改动文件回归门

覆盖门以三个实际改动文件计算要求，而非按主题手选：

- changed files：3；
- matched mapping rules：3；
- required groups：17；
- evidence logs：1 个完整 `Shanmen.0_0_10` 日志；
- result：`PASS Changed=3 Rules=3 Required=17 Logs=1`。

门禁日志为 2,033 bytes，SHA-256 `31B5B16BACCE2A405D20C89342CF1B46EEA08B180F471CD328F2970AEE9AFED5`。门禁自测为 `PASS 437/437`，43,073 bytes，SHA-256 `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0`。

## 8. 构建、产物与静态边界

| Target | Result | Actions / Time | Bytes | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded / native 0 | 12 / 21.88s | 2,882 | `711984CF904703AC1E4DE229820E46A24059B06F48D15A0DEDA941939DF23A61` |
| Game Development final | Succeeded / native 0 | 11 / 40.65s | 2,786 | `72B6BBD8760D40B19CA452D28DE8B0ADEA51CD5A91ACDA525C68AC8B76A3F487` |
| Editor Development final | Succeeded / native 0 | 0 / 1.17s | 1,021 | `338CB16CA1E8509DF390AE6A2B5F6328763FC056494A5616C7685446C75BB7F5` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,533,056 bytes，SHA-256 `1318639ACE809ADABC67FB5D455387480150480BB2FAD5273C77923026ADA1CB`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,724,352 bytes，SHA-256 `B0FCB4F9FED4CD294E3BC809A07F3EBB0E56CC5C634001D0B04DB5386762D973`。

实现/测试 diff 为 3 files、`+45 / -9`。新增生产行中 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor`、`Destroy` 命中 0；`git diff --check` 原生退出码 0，仅有工作树 LF→CRLF 提示；验证后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

## 9. P/F 边界

PASS：真实 committed impact 只在同一激活的返航阶段控制已有材质；普通命中、击破与零伤害有互斥颜色分支；命中优先于残留 threat 材质色；没有命中时原有阶段/威胁表现不变；下一激活和停用清除旧反馈；专项、全量、覆盖门、自测及双目标编译全部通过。

未声明：材质颜色在真实显示器、地图光照、后处理或玩家视角中的最终可读性；击破和零伤害颜色的真实产品场景画面；动画、粒子、音效、震屏、停留时长或玩家手感。若要证明这些内容，需要另行授权实际 UI/PIE 验收。

## 10. 提交边界与 GitHub

本阶段只提交飞剑 Actor 头/实现、现有真实阻挡接触测试、本 Report 与本 Development Log，共 5 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.14` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-14-flying-sword-impact-cue>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-14-flying-sword-impact-cue/Docs/Report/Dev.D.UE.0.0.10.P21.14.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-14-flying-sword-impact-cue/Docs/Log/Dev.D.UE.0.0.10.P21.14.r0_log.md>
