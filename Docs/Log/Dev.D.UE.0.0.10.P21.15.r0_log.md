# Dev.D.UE.0.0.10.P21.15.r0 Development Log

## 1. 基线与目标

- base：`10772c8c53f6072780da73960502ed738cd5e6c9`（P21.14 flying-sword impact cue）；
- branch：`agent/0.0.10-p21-15-flying-sword-travel-facing`；
- 目标：让飞剑已有网格跟随同一 Activation 内相邻 canonical flight read model 的真实世界位移方向；
- 边界：只旋转无碰撞 `Visual` 子组件，不旋转 Actor/根碰撞，不新增速度、目标、计时、伤害、Actor 或表现权威，不启动 UI/PIE/产品 executable。

## 2. 审计与决策

P21.14 的飞剑 Actor 已有长 `X` 轴 Cube 网格、根 `UBoxComponent` 碰撞和按阶段发布的 canonical flight read model，但表现网格从不读取位移方向，因此出剑与返航时仍保持生成朝向。

没有新增“朝向状态”。采用最小派生规则：在 Actor 接受新 read model 前保存上一份，只有两份模型有效且属于同一 Activation 时，才从世界位置差计算方向。无前序事实时保持现状，避免猜测。

## 3. 实现

`demo_mapShanmenControlledWeaponActor` 新增 `RefreshTravelFacing()`：

- 验证 `Visual`、上一模型和当前模型；
- 对比精确 ActivationId；
- 计算 `current location - previous location`；
- 零位移直接返回；
- 非零位移只调用 `Visual->SetWorldRotation(TravelDelta.Rotation())`。

`TryPresentFlightReadModel()` 在替换成员前复制上一份模型，并在既有 `RefreshPresentation()` 前更新网格方向。新增只读 `GetPresentationForwardDirection()` 供真实 Actor 测试观察，不暴露写入口。

没有调用 `SetActorRotation()`，根碰撞 transform 保持原权威。颜色与点光源仍走 P21.14 已有表现解析。

## 4. 生命周期与边界

第一份 read model、新 Activation、无效模型和零位移都不会制造方向。旧 Activation 的末位置不能与新 Activation 的首位置拼接；停用时既有清理仍会清空 flight read model。

这使朝向寿命自然附着于现有 Activation，而不增加 Timer、Tick、缓存管理器或目标追踪。表现只消费 read model，不反写 controller、RunHost 或 world delivery。

## 5. 测试开发

扩展 `ControlledWeaponRunHost.BlockingContactTerminal` 的真实产品链测试：

- 把敌人放到世界 `+Y`，并以 `FVector::RightVector` 发射；
- 保存发射后 Actor/根碰撞旋转；
- 从同一 Activation 的 Directed 与 Returning 快照计算规范化位移；
- 断言位移为非默认 `+Y`；
- 断言网格 forward 与该位移一致；
- 断言 Actor/根碰撞旋转保持不变。

既有真实阻挡接触、生命值提交、返航、P21.14 impact cue 与下一激活清理断言全部保留。该方向断言不能由默认 Cube `+X` 姿态恒真通过。

## 6. 执行与诊断

Editor 初始编译首次通过，12 actions / 35.10s / native 0。Controlled-weapon 专项首次通过，`62 Success / 0 Fail`。

第一次全量运行在 `736 Success / 0 Fail` 时人工终止并保留，因为日志持续出现 EOS SDK 与 `google.com/generate_204` 健康检查，尝试以 UE 5.8 已提供的 `-NoEOS` 参数隔离无关在线服务。诊断日志为 1,230,561 bytes，SHA-256 `E80AA6A58EFE48B852A28ED5DD64BF02630668BBDF115498518DF90A492E8693`，不作为放行证据。

加入 `-NoEOS` 后从头重跑完整套件，最终为 `1247 Success / 0 Fail`、native 0。该运行没有 `LogEOSSDK`，但仍记录 274 次 `generate_204`，且总耗时未优于历史基线；因此结论是健康探测不由 EOS 单独拥有，`-NoEOS` 不能作为已验证提速方案。最终放行只依赖完整重跑，不依赖诊断运行。

## 7. 自动化与门禁

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P21.15_focused_controlled_weapon_initial.log` | 62/0 | 333,027 | `D20325CF1270E50E4B9821FA2241BA3D93068FEB51BDEDC0ACD6F563E2CABD50` |
| `P21.15_full_0_0_10.log` | 1247/0 | 1,902,750 | `842B13B21F98D78CE8809D596F54420789FDEE72AE25DA53FE4DB2D5D0FCD868` |
| `P21.15_regression_gate_final.log` | PASS 3/3/17/1 | 2,033 | `56B447358F6E8B9A7495948A539D7ED8498968217B51D808C1C850ABBEFD28F4` |
| `P21.15_regression_gate_selftest.log` | PASS 437/437 | 43,073 | `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0` |

覆盖门读取三个实际改动路径，由完整 `Shanmen.0_0_10` 日志覆盖映射得到的 17 个 required groups；没有用专项组代替跨模块证据。

## 8. 构建与静态检查

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P21.15_editor_build_initial.log` | 12 actions / PASS | 2,803 | `54021BA129637766E19CDBA57007CA1E710A5825ECF7A0B5127079A463CB511C` |
| `P21.15_game_build_final.log` | 11 actions / PASS | 2,786 | `F99236CC492D33DA35039D1C5022F4E05CE741A775E02AA7BF60E0B3C3F58C8A` |
| `P21.15_editor_build_final.log` | 0 actions / PASS | 1,021 | `0D76CEA193CDAF3525646CF7B84C69554DBF724536D755FA2629CF67F2FDCDFF` |

`git diff --check` native 0。实现/测试为 3 files、`+54 / -3`；新增生产行无 Timer、SetTimer、RNG、ApplyDamage、SpawnActor 或 Destroy。最终 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程数均为 0。

## 9. 提交边界

精确提交以下五个文件：

- `Source/demo_map/demo_mapShanmenControlledWeaponActor.h`；
- `Source/demo_map/demo_mapShanmenControlledWeaponActor.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P21.15.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P21.15.r0_log.md`。

103 个用户原有 untracked 文件不暂存；raw evidence 保留于 `Saved/Codex/P21.15` 且不进入 Git。未修改 Content、地图、Engine、Windows、存档 schema、物品权威、Impact resolver、target vitality 或 flight controller 数学。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-15-flying-sword-travel-facing>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-15-flying-sword-travel-facing/Docs/Report/Dev.D.UE.0.0.10.P21.15.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-15-flying-sword-travel-facing/Docs/Log/Dev.D.UE.0.0.10.P21.15.r0_log.md>
