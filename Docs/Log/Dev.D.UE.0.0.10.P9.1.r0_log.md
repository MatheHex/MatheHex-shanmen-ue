# Dev.D.UE.0.0.10.P9.1.r0 Development Log

## 目标

把 P9.0 `bRequiresCommitOnTrigger` 的开放边界实现成真实、可重放、revision-checked 的 SpiritShield capacity authority，同时保持生命周期、时间、能量、输入和产品对象在纯值 Runtime 之外。

## 设计决策

1. authority 从有效 activation receipt 创建，初始 revision 为 `0`、容量为 immutable Definition 的 MaximumCapacity；
2. caller 不直接向 P9.0 runtime 注入自称的容量，必须通过 authority 的 `TryProjectDefenseLayer()`；
3. commit command 同时绑定 projection、canonical Impact request/result 与触发层；
4. command 创建时重跑 resolver，不信任外部可写 result；
5. ledger 以 `ImpactId` 幂等，`CommandId` 区分 exact replay 与 conflict；
6. 耗尽停止投影但不自动 deactivate，避免 capacity authority 窃取 lifecycle；
7. repeated anonymous helper names 是模块既有设计，模块固定 non-unity 比全仓机械重命名更小、更稳定。

## 执行序列

1. 审查 `FShanmenVitalityAuthority` 的 revision/idempotency 模型与 P9.0 projection receipt。
2. 冻结 capacity command、receipt、result、authority 接口。
3. 实现 canonical resolver proof、source 唯一性、deterministic IDs、atomic commit 与 replay ledger。
4. 增加 6 个 focused tests。
5. 增加 `SpiritShieldCapacityAuthority` changed-path 规则；self-test 从 `102` 增至 `104`。
6. mapping JSON `71` 条规则解析通过；self-test `104/104`。
7. 首次 Editor build 原生退出 `6`：clean unity 单元暴露既有匿名 helper 冲突；P9.1 两个独立单元本身编译通过。
8. 在 `ShanmenCombatRuntime.Build.cs` 固定 `bUseUnity = false`；标准命令 recovery build 原生退出 `0`。
9. focused candidate `6/6`。
10. 收紧 receipt 算术：成功 commit 必须产生 float-bit 精确且严格下降的容量。
11. Editor final 原生退出 `0`。
12. 最终 Automation：Capacity `6/6`、SpiritShield `11/11`、CombatCore `9/9`、CombatRuntime `45/45`、full `348/348`。
13. implementation/staged changed-file gate、静态扫描、cached diff check 全部通过。
14. Game final 原生退出 `0`。
15. 生成同名 Report/Log，执行 exact-stage、commit、push 与远端 SHA 核验。

## 核心状态迁移

```text
ActivationReceipt
  -> CapacityAuthority(revision=0, available=max)
  -> Projection(revision=N, available=C)
  -> canonical Resolve(request) == supplied result
  -> Command(ImpactId, ResolutionId, ProjectionId, requested)
  -> Commit
       new Impact    => available -= requested; revision += 1; receipt stored
       exact replay  => original receipt; no state mutation
       conflict      => rejected; no state mutation
       stale/foreign => rejected; no state mutation
  -> available == 0 => no projection; lifecycle remains explicit
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `SpiritShieldCapacity-final.log` | 6 | 0 | yes | 0 | `3F737B9BAAA49EF15775376A29B775C1A5168D6F226879DD79B06469785A2449` |
| `SpiritShield-final.log` | 11 | 0 | yes | 0 | `3D1AEF32CBDBE0D2F07569C90BB2E758AA5FF029E5CD0EFAB5FA6CA19C00443C` |
| `CombatCore-final.log` | 9 | 0 | yes | 0 | `C539B2CC2B48F8B458E213EEEBF3D8C369025FBCCF51D0D03E68C02CDCC1F6AB` |
| `CombatRuntime-final.log` | 45 | 0 | yes | 0 | `77A7F4862F7C3962344997CAC69CF59CC1382F9005E2838A9B36AB5F24506DCB` |
| `Shanmen-0_0_10-final.log` | 348 | 0 | yes | 0 | `F1733B6D80E5F199B6BC18B8587510E7070DFB38C371F59ED79767D5247899B9` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=71
SELF_TEST: PASS 104/104
REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=4 Logs=5
git diff --cached --check: PASS
```

- required groups：Capacity、SpiritShield、CombatRuntime、CombatCore；
- mapping SHA-256：`04F907EB3AB21EC2EABF8522D90C202ED8E56D83EF5C56D224E4604D11F044AA`；
- self-test SHA-256：`958599D9E792CAE12189C6E0D14ABA253AA921FEED701D13C6612BCC645AEA35`；
- production boundary scan：`demo_map`、World/Actor、damage side effect、discovery、Timer/Tick、while 与 RNG 均为 `0`；
- code/scripts：`6 files / 1234 insertions`。

## 构建证据

| Build | Actions | Time | Exit | SHA-256 |
|---|---:|---:|---:|---|
| Editor initial | 6 | 39.54s | 6 | `54104442FF0FEE69FDADBA984E3ACB93B740F0776DB1BA62A18CE9D544433EA8` |
| Editor recovery | 4 | 10.57s | 0 | `2716F8E5B53A94DC4E5F31B3C6620C105F85DC3914D54D4BCFBFF9B3FEC61060` |
| Editor final | 4 | 4.44s | 0 | `C4939472CBECFAD9035E0170E4A3B1B3DCC5639AF87BA5D35086D3AE7D396E2B` |
| Game final | 5 | 30.29s | 0 | `CEDFE7BFDC519FE66FECA578F5E1DA9CA44BEB674D0A94D214D99096545CF5DE` |

## 真实异常与修复

首次失败不是 commit-memory、SDK 或新 authority 编译错误。日志显示：

- `ShanmenSpiritShieldCapacityAuthority.cpp` 编译动作完成；
- `ShanmenSpiritShieldCapacityAuthorityTests.cpp` 编译动作完成；
- `Module.ShanmenCombatRuntime.cpp` 因既有多个 `.cpp` 的 file-local 同名 helper 被 unity 合并而产生 C2084/C2264。

修复为模块规则 `bUseUnity = false`。这把原设计上互相独立的匿名命名空间恢复为独立翻译单元，并让 clean build 可重复；未改任何 gameplay 行为。首次失败日志及退出码完整保留。

## P/F 边界

仅执行 P 阶段纯值实现、无头测试、静态/门禁和 Editor/Game Development 构建。没有启动 UI、PIE、Standalone、产品 exe、真实输入、Smoke、Cook 或 Package。
