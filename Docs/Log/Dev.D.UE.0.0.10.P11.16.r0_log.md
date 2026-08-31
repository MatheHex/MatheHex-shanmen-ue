# Dev.D.UE.0.0.10.P11.16.r0 Development Log

## 1. 目标

承接 P11.15 的四产品互斥通道，移除固定 `Guard bool + Thrown bool + Spirit bool` 占用结构。目标是在不创建第二套 Host 状态或全局可变 registry 的前提下，把占用改为可扩展 typed claim 投影，并继续锁定“只有 exact WeaponGuard 可抢占”。

## 2. 基线审计

三个持续占用产品都已有稳定身份：

- WeaponGuard Product Host 暴露 HostId；
- SpiritEvasion Product Host 暴露 HostId；
- ThrownWeapon Run Host 持有冻结 Action Snapshot，其 ActivationId 可作为 flight owner identity。

因此无需新生成 GUID、复制 Run identity 或添加中心状态管理器。P11.15 的 GameMode capture 是只读边界，适合改造成每次调用即时注册 claim。

## 3. 实现

新增 typed claim：`OwningAction + OwnerId + Preemption`。Snapshot 私有保存 claim 数组，公开安全注册和显式 invalidation；无身份、错误抢占等级和重复产品都会污染 projection 并在 identity 前拒绝。多个不同产品仍形成结构有效的 replayable policy conflict。

抢占等级规则在 `Claim::IsValid` 中关闭：WeaponGuard 必须 `ExactOwner`，Thrown/Spirit 必须 `None`。receipt 泛化为 `OccupyingAction + OccupyingOwnerId`，使 non-preemptible 冲突也能准确记录阻塞者；真实 gate 仍只允许 WeaponGuard exact owner terminal proof。

GameMode 从三个既有 Host 即时注册三个可能 claim。Thrown Session/Lifecycle 只增加冻结 ActivationId 的只读透传，没有新增生命周期字段。

## 4. 测试

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.PlayerActionArbitration` | 4 | 0 | `01530259219193FE572EFA0BC4DE9E1E8A62117B9CA310B9FAD6434522F1056D` |
| `Shanmen.0_0_10` | 541 | 0 | `BE900CDCE87AB2F4EDE4206632CA674DD684CA7BAC1CA2CE5844F4468A192789` |
| `demo_map.V3.Attributes` | 4 | 0 | `6697CA25ADDCCEB3E54C4002CE18B2DDA9E61FAF7C927F5DC2B334EEFBD6C372` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `A06331B7D22C27892A518CAA2F11849E7D54F539C6A2C2FEE22CA9342CFF2747` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `90326AD1ABFBB01663EA4CD50E383AD6786DF50BDA0C07FFC169F32B2CE0E09B` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `E41189979EC9A601D664CE1AD876A52DD739A75CE90AD1845DC3638EF7F2278A` |

原始 Success 合计 661、Fail 0；focused 4 项包含于 full 541，按 identity 去重为 657。

## 5. 门禁

```text
REGRESSION_MAP_JSON: PASS Rules=103
SELF_TEST: PASS 166/166
REGRESSION_COVERAGE: PASS Changed=8 Rules=6 Required=41 Logs=6
STATIC_REVIEW: PASS AddedSourceLines=405 ForbiddenHits=0
PRODUCTION_CLAIM_REGISTRATION_CALLS=3
ARBITRATION_BOUNDARY_HITS=0
git diff --check: PASS (native exit 0)
```

预检只给 focused/full 时，gate 退出 1 并列出四组缺失旧回归；补齐真实日志后正式通过，没有修改 mapping。

## 6. 构建

- Editor：79/79，228.40s，原生退出 0，日志 SHA-256 `35F8D51A54534F7E3D973FA07EBFAFDB748CAD7E2E47D799C5721FEC0E15401B`；
- Game：78/78，210.03s，原生退出 0，日志 SHA-256 `316551FB3CE58D35C822E37FD30A862B68FD1C8754BA43B3280A763AA2C930CE`。

无源码或测试失败。非 Win64 SDK metadata 告警未影响 Win64 VALID 目标。

## 7. 边界

本阶段未修改 Product Host authority、CombatCore、Impact、damage、vitality、inventory 或 schema。Coordinator 不保存 claim；GameMode 不持久化 claim；PlayerController 不拥有产品互斥状态。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
