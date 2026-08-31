# Dev.D.UE.0.0.10.P11.1.r0 Development Log

## 目标

在 P11.0 普通武器格挡窗口之上建立独立的精准格挡时序策略：消费调用方显式提供的单调时间样本，在同一 Active action lifecycle 内互斥选择 PerfectGuard 或普通 Guard；不自建 Tick/Timer，不硬编码产品手感数值，不提前接入输入、方向或耐久。

## 审计结论

1. P11.0 已拥有 canonical weapon-guard action、exact Startup-to-Active commit、Active-only window 与普通 Guard projection；
2. CombatCore 已有 `PreventAll`、`PerfectGuard=200`、`Guard=300`、`DefensePerfectGuard` 与 `PerfectGuarded` outcome；
3. P9 SpiritShield deadline gate 已证明 caller-owned timeline ID + opaque integer tick 可以避免 Runtime 自建时钟；
4. SpiritShield 的 observation 类型带有 shield 语义，不应直接泄漏进武器格挡 API；
5. P11.1 只需要一个小型 guard-specific immutable observation，不需要第二套 stateful gate；
6. 分类可以是纯函数：`[start,end)` perfect，`end+` ordinary，pre-start invalid；
7. action runtime 仍负责 Recovery/Interrupted 关闭，不需要 timing evaluator 持有状态；
8. perfect layer 可以继承 P11.0 filters 与 source weapon，但不能在无 reservation 时声明资源 commit；
9. 最终 receipt 必须只公开一个 selected layer，避免消费者叠加 ordinary 与 perfect；
10. 产品时长、秒数、帧率、方向、输入和耐久均未冻结，应保持外置。

## 设计决策

1. `FShanmenWeaponPerfectGuardPolicy` 绑定 exact P11.0 window receipt；
2. policy 冻结 TimelineId、ActiveStartTick、PerfectEndTick 与 PerfectRuleId；
3. end 必须严格大于 start，不接受空区间；
4. tick 是调用方不透明整数，Runtime 不换算秒或帧；
5. observation identity 由 timeline + tick 确定性派生；
6. evaluator stateless，不持有上一 observation，也不声称拥有单调序列；
7. pre-start、timeline mismatch、window mismatch 均失败关闭；
8. perfect band 投影 PreventAll/PerfectGuard/DefensePerfectGuard；
9. ordinary band 原样选取 P11.0 projection；
10. perfect 与 ordinary tags 严格互斥；
11. perfect layer 继承 ordinary 的六组 tag filters 与 weapon source；
12. `bRequiresCommitOnTrigger=false`，直到 durable authority 提供真实 reservation；
13. public receipt 只暴露 selected layer；
14. policy、observation、layer 与 receipt 全部确定性派生；
15. 新模块不依赖 demo_map、World、Actor、ApplyDamage、RNG、Timer 或 Tick callback。

## 执行序列

1. 从 P11.0 commit `37be5c3cefa70155e9eb0f7fb438b36f3ab14d51` 创建分支 `agent/0.0.10-p11-1-perfect-guard-timing`。
2. 审查 P11.0 WeaponGuard、ActionOrchestrator、CombatCore defense order/tag/resolver 与 SpiritShield deadline gate。
3. 冻结 half-open interval 与 caller-owned timeline 边界。
4. 新增 immutable policy、timeline observation、timing projection receipt 与 stateless evaluator。
5. 实现 exact window/timeline/start binding 与 deterministic identity。
6. 实现 perfect layer，并继承 P11.0 filters/source identity。
7. ordinary 边界直接返回 P11.0 layer，避免重新编码 fraction 或 rule。
8. 新增六个 focused Automation tests。
9. 新增 `WeaponPerfectGuardTiming` changed-file rule 与正/反 self-test fixtures。
10. mapping JSON 首轮 `90` rules，self-test 首轮 `140/140`。
11. 首版 Editor Development 成功，focused 与五组回归全部成功。
12. API 自审发现 ordinary provenance getter 不必公开，移除 getter并调整测试旁路对比 P11.0 layer。
13. 按最终源码重新执行 Editor Development：6 actions / 22.53s / exit 0。
14. 最终 WeaponPerfectGuard 6/6、WeaponGuard 6/6、ActionLifecycle 1/1、CombatCore 9/9、CombatRuntime 99/99、全量 467/467。
15. changed-file gate：Changed=5、Rules=2、Required=5、Logs=6，PASS。
16. boundary scan、mapping、自检与 `git diff --cached --check` 通过。
17. 最终 Game Development：5 actions / 28.75s / exit 0。
18. 精确暂存 5 个实现/门禁文件；加入同名 Report/Log 后扩展为 exact 7-file stage。
19. 复核长期未跟踪文件未进入暂存区，commit 并 push。

## 状态与数据流

```text
P11.0 exact Active WeaponGuardWindow
  + caller-owned TimelineId
  + ActiveStartTick / PerfectEndTick / PerfectRuleId
  -> immutable PerfectGuardPolicy

explicit TimelineObservation(timeline, tick)
  -> Window still Active?
  -> exact Window/Timeline binding?
  -> tick >= ActiveStartTick?
  -> obtain P11.0 ordinary projection as private provenance
  -> classify exactly one band
      [start,end) -> PreventAll / PerfectGuard / DefensePerfectGuard
      [end,+inf)  -> original ReduceFraction / Guard / DefenseGuard
  -> public receipt exposes one selected GetLayer()

Recovery | Interrupted
  -> P11.0 window invalid for runtime
  -> timing projection rejected
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `P11.1-WeaponPerfectGuard-final.log` | 6 | 0 | yes | 0 | `B7BD207CFF48D2692C1FE0B26D2FED4E89C3638815B0370F9F96011A334BCC99` |
| `P11.1-WeaponGuard-final.log` | 6 | 0 | yes | 0 | `C3A805B8BB706D07834ECBCE91D79A33624A1E23686F9D0771EA83877337CD17` |
| `P11.1-ActionLifecycle-final.log` | 1 | 0 | yes | 0 | `DC7613811431FC2E679FE9DE4CE650724267AE218934CCE9B35E8F81E76E81B0` |
| `P11.1-CombatCore-final.log` | 9 | 0 | yes | 0 | `AF764B4E30D4FF80DC206D7D5DE52BB34D2A382AC23DF421F4952BBFE2976C1E` |
| `P11.1-CombatRuntime-final.log` | 99 | 0 | yes | 0 | `E293207951DF9E34AADF30BE95653B114DF50427D50D927E83BB5DC4DC8FA8AF` |
| `P11.1-Shanmen-0_0_10-final.log` | 467 | 0 | yes | 0 | `2E282C74EE1104A094347D6BE1F70883922B449C2E179A48F7ADE1DB4D3E004A` |

总计：588 Success / 0 Fail。六份日志均有唯一 RunTests、terminal queue-empty 与原生 exit 0。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=90
SELF_TEST: PASS 140/140
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=5 Logs=6
git diff --cached --check: PASS
BOUNDARY_SCAN: PASS hits=0
```

- required groups：WeaponPerfectGuard、WeaponGuard、ActionLifecycle、CombatCore、CombatRuntime；
- mapping SHA-256：`7AB7A538E145E576B661E93AC44969344FC94AA5F98C496ECDF03FF545CC44A1`；
- self-test SHA-256：`7BC6C4873E00BA9995E85B5F4EC526EEDD87B6BED72FEA3B757C204070172100`；
- implementation/mapping：`5 files / +993 / -0`。

## 构建证据

| Build | Actions | Time | Exit | Log SHA-256 |
|---|---:|---:|---:|---|
| Editor Development | 6 | 22.53s | 0 | `31293F0894E51C9687ED86259F40F547BA415654AC22C5FA5C94D0045635E11A` |
| Game Development | 5 | 28.75s | 0 | `E8C841E99850CD0EA9F71AD9FBE00D473318AD531B81FEC3D71E34F147ECCFAC` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：1424896 bytes / `23373EB70ED7FC61BCAA16A3C9FA664C3F2C206D1610C8E40D2D7859E3F4F166`；
- `demo_map.exe`：354039296 bytes / `745249F251C8D215E6567934142BB65A1D23FBEC84E6BD2AD1385026909705B0`。

## 真实异常

没有真实源码、UHT、Automation、门禁、Editor 或 Game 构建失败。API getter 收紧是成功验证后的主动审查修正；最终证据全部来自收紧后的源码。

UE 5.8 的 UnifiedErrorTest 初始化噪声与非 Win64 SDK validation 信息不属于 selected tests。Win64 目标有效，selected Fail 为 0，terminal marker 与原生退出码正常。

## P/F 边界

只执行 P 阶段纯值实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

后续优先把格挡方向/夹角做成同样的外部采样纯政策；若接武器耐久，必须通过现有 durable item authority 的真实 prepare/commit/cancel，不能让 timing evaluator 直接改库存或耐久。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-1-perfect-guard-timing/Docs/Report/Dev.D.UE.0.0.10.P11.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-1-perfect-guard-timing/Docs/Log/Dev.D.UE.0.0.10.P11.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-1-perfect-guard-timing>
