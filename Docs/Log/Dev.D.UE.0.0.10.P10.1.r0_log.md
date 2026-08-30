# Dev.D.UE.0.0.10.P10.1.r0 Development Log

## 目标

在 P10.0 action-bound active defense window 上增加方向移动的最小单向 seam，使产品输入可以形成确定性 movement-policy request，但不在纯运行时层决定移动距离、持续时间、碰撞、腾空、无敌帧或 SpiritEnergy 数值。

## 审计结论

1. P10.0 已提供 exact Startup-to-Active commit window，可继续作为唯一生命期权威；
2. 产品已有 `Fdemo_mapCombatDisplacement` swept movement，但其 collision skin 仍读取旧 P6 `Fdemo_mapEnemySkillPrototypeConfig`；
3. 直接让新主动闪避调用旧 displacement 会把 0.0.10 新契约重新耦合到敌人技能数值；
4. 旧 TwinStick Dash 使用 `LaunchCharacter`，不具备本阶段要求的 request/receipt/幂等边界；
5. 旧 DodgeChance 是随机被动属性，不是反应式主动闪避；
6. 产品层仍没有冻结的 SpiritEnergy writable balance、revision、恢复与持久化 seam；
7. 因此 P10.1 只冻结 action + opaque policy + planar direction -> active-window request。

## 设计决策

1. capture 只接受 canonical SpiritEvasion action；
2. MovementPolicyId 为产品所有的 opaque key，纯层不解释其参数；
3. 输入向量要求 finite 且 XY 非零；
4. capture 丢弃 Z，并把 XY 规范化为单位向量；
5. intent 保存完整 action snapshot，而不是松散 ActivationId；
6. intent identity 包含完整 action/content/source tags、policy 与 exact direction bits；
7. P10.0 window 暴露统一 `IsActiveFor` 查询；
8. 防御 projection 与移动 planner 共享同一窗口检查；
9. planner 只在 exact Active runtime 上创建 request；
10. request identity 绑定 IntentId 与 WindowReceiptId；
11. ledger 只处理 request replay，不拥有全局时间或产品状态；
12. 不包含 World、Actor、movement component、collision、resource 或 RNG 访问。

## 执行序列

1. 审查人工完善的 0.0.10 战斗规划、P10.0 window、旧 Dash 与 displacement authority。
2. 确认产品数值/资源边界尚未冻结，将本阶段收紧为纯 request contract。
3. 给 P10.0 window 增加 `IsActiveFor`，并让原 defense projection 复用。
4. 新增 mutable movement intent capture 与 immutable intent。
5. 实现 planar finite/zero/normalization 校验。
6. 实现完整 action equality 与 canonical source-tag serialization。
7. 实现 deterministic IntentId 与 RequestId。
8. 新增 Active-window planner 与 request ledger。
9. 新增五个 focused tests，覆盖 capture、active binding、phase、replay 与 idempotency。
10. 新增 changed-file mapping，强制 Movement、SpiritEvasion、ActionLifecycle 与 CombatRuntime 证据；修改 P10.0 文件同时触发 CombatCore。
11. 扩展 mapping self-test pass/fail fixtures。
12. JSON mapping `77` rules 与 self-test `116/116` 通过。
13. Editor candidate 8 actions，原生退出 `0`。
14. focused candidate `5/5`，无源码修正轮。
15. 串行执行六组正式 Automation，合计 `503` success、`0` fail，全量 `390/390`。
16. changed-file gate 首次因外层 PowerShell 数组传参方式失败，改为当前进程直接调用后通过。
17. changed-file gate 最终 `Changed=7 / Rules=3 / Required=5 / Logs=6`。
18. 精确边界扫描命中 `0`，`git diff --check` 通过。
19. 暂存区限定为 7 个实现/门禁文件，`+747/-5`。
20. Editor final up-to-date success；Game final 7 actions，原生退出 `0`。
21. 生成同名 Report/Log，执行 exact-stage gate 与暂存范围复核。
22. commit、push，并使用远端 commit SHA 形成不可变链接。

## 状态与数据流

```text
Product input sample
  -> capture(action, MovementPolicyId, candidate direction)
  -> validate canonical SpiritEvasion action + explicit policy
  -> finite XY, discard Z, normalize planar direction
  -> immutable MovementIntent + deterministic IntentId

P10.0 Window.IsActiveFor(exact ActionRuntime)
  false -> no movement request
  true  -> verify intent action == window action
        -> immutable MovementRequest + deterministic RequestId
        -> MovementLedger.TryAccept
           first delivery -> accepted
           exact replay   -> rejected

Future product adapter (not implemented here)
  -> resolve policy distance / trajectory / collision / resource
  -> execute through a decoupled swept movement authority
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `P10.1-SpiritEvasionMovement-final.log` | 5 | 0 | yes | 0 | `BB70229073298FBFF8217C14E6E10AC62C66C00CE4BA643B6CA52B146A1ED2D9` |
| `P10.1-SpiritEvasion-final.log` | 11 | 0 | yes | 0 | `B732D297425B5A259ADA0C6D099109628FE52CE48DD6AF2C497ACEC3DB74EE3A` |
| `P10.1-ActionLifecycle-final.log` | 1 | 0 | yes | 0 | `6C0F5E84951B60157340492CDDF425965DEB77CA4A280CAC222320220BC3F9E1` |
| `P10.1-CombatCore-final.log` | 9 | 0 | yes | 0 | `AFA44465E60F7C68DC00DEE49871B27A57097C94A122B1BD389457E65EF11FA0` |
| `P10.1-CombatRuntime-final.log` | 87 | 0 | yes | 0 | `8012A8BE4D934BB26F39833527312350856D629F7CEA05535CF2F419C3992F29` |
| `P10.1-Shanmen-0_0_10-final.log` | 390 | 0 | yes | 0 | `A3C9F9A8AC88CD8C0B5433C74EA6A6C01909863DE6C54B86E9E311BD60510153` |

命令形态：

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

六份日志均恰有一个 selected queue-empty、`0` selected fail、`0` fatal/unhandled/ensure，进程原生退出码均为 `0`。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=77
SELF_TEST: PASS 116/116
REGRESSION_COVERAGE (implementation): PASS Changed=7 Rules=3 Required=5 Logs=6
git diff --check: PASS
BOUNDARY_SCAN: PASS hits=0 (world/actor/damage/RNG/timer/tick-callback)
```

- required groups：SpiritEvasionMovement、SpiritEvasion、ActionLifecycle、CombatCore、CombatRuntime；
- mapping SHA-256：`EB3AE5705D07D035A349187B6D59AB97CD9E4A82B0B313985D55B578D6748CE4`；
- self-test SHA-256：`B5043A11E17658053C83479ABE549F5876EDCB109A288F43CC9B4E93D2453622`；
- 实现：`7 files / +747 / -5`。

## 构建证据

| Build | Actions | Time | Exit | Log SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | 8 | 44.52s | 0 | `D3A64B2D0BEA888F16B234614DFDF4061B36796140C3332F18D886F7F6D00055` |
| Editor final | 0 | 0.92s | 0 | `06E0A8922BA704EE654B600C190A4A21FF890524036D7027B48594FE959B278D` |
| Game final | 7 | 31.37s | 0 | `B946BC78A491417A13DE24374DF07B26152FAE02BD8C4C8009D3604338998246` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1299968` bytes / `054A9C499C19B0BCF11B7D729C26EA39DD84B07B293D88323AD6F83E85CEFDB9`；
- `demo_map.exe`：`353633280` bytes / `93F9CA6DBB28184F91B85E948A88FF7309F46C6AC1EFF01299354096CDAC8504`。

## 真实异常

首次 changed-file gate 包装命令使用 `pwsh -File` 再传两个 PowerShell 数组，数组元素被展开成多余位置参数，得到：

```text
Test-ShanmenRegressionCoverage.ps1: A positional parameter cannot be found that accepts argument 'Source/ShanmenCombatRuntime/Public/ShanmenSpiritEvasionMovement.h'.
```

脚本尚未进入 gate 计算；没有测试或源码失败。改为当前 PowerShell 进程直接 `& .\Scripts\Test-ShanmenRegressionCoverage.ps1 -ChangedPath $paths -AutomationLogPath $logs` 后，同一 7 个路径与 6 份日志通过。

UE 5.8 启动时仍输出 UnifiedError 基线 `Condition failed` 诊断，但所选测试随后全部逐项 Success，queue-empty 与原生退出码正常。

## P/F 边界

仅执行 P 阶段纯值实现、无头 Automation、静态/门禁和 Editor/Game Development 构建。没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P10.2 应先拆出通用 movement-policy/collision 配置，再让产品 adapter 单向消费 P10.1 request。不得直接复用旧 P6 enemy skill config 作为新闪避数值权威，也不得在没有真实 SpiritEnergy authority 时临时新增资源 float。
