# 山门 Demo2.0 本地运行

新的 Demo 从“归尘试炼”石庭开始，使用当前框架，但不沿用旧 Demo 的地图、界面与材料。当前为首个战斗切片，使用基础几何体占位美术。

## 启动

先关闭正在运行的 Unreal Editor。双击项目目录中的 `Scripts/Start-Demo20.cmd`，即可使用本机 UE 5.8 启动不经打包的游戏窗口。启动器会明确加载 `/Game/Demo20/Maps/L_Demo20_StoneCourt`，不会落到旧默认 M01 地图。

也可在项目目录运行：

```powershell
./Scripts/Start-Demo20.ps1 -Action Play
```

需要编辑场景或使用 PIE 时：

```powershell
./Scripts/Start-Demo20.ps1 -Action Editor
```

Editor 打开新地图后按 Play。不要直接双击工程后运行旧默认地图来验收 Demo2.0。启动器默认请求 1280×720 窗口，Windows DPI 缩放可能使桌面截图像素尺寸不同。

此入口依赖本机 UE 5.8 和已构建的工程 DLL，不是可以单独分发的 exe。如果刚从 GitHub 获取源码，先运行：

```powershell
./Scripts/Invoke-Shanmen.ps1 -Action BuildBoth -TaskId Demo20.Local
```

新地图和独立调色材质 `Content/Demo20/Materials/M_Demo20_Color.uasset` 一同交付。`GenerateDemo20Map.py` 与 `GenerateDemo20Material.py` 仅供首次生成，拒绝覆盖已经存在的资产。石庭几何由新的 GameMode 在运行时构建。

## 玩法与操作

点击“进入试炼”，靠近石卫，将鼠标指向目标并出剑。赤色范围出现后约 0.8 秒发动攻击；移出范围、闪身或格挡。三座石卫全部击破后，返回入口青色归阵，按交互键撤出。死亡或主动结束后可以返回入口重新开始。

| 默认输入 | 操作 |
| --- | --- |
| W/A/S/D | 移动 |
| 鼠标位置 | 指向出剑方向 |
| 鼠标左键 | 近距离出剑 |
| Space | 闪身，有恢复时间 |
| 按住鼠标右键 | 格挡，移动减速，不能同时出剑 |
| G | 在青色归阵处撤出 |
| Esc | 暂停或继续 |
| Alt+F4 | 关闭本地游戏窗口 |

实际键位读取既有输入注册表，若本机有自定义绑定，以游戏底部提示为准。窗口失焦时会暂停，需要点击继续。

## 当前范围与安全边界

- 玩家与石卫生命值、伤害、防御计算使用现有领域框架。
- 每局是一次临时试炼；没有背包、治疗、拾取、永久奖励和存档恢复。关闭窗口后本次进度不会保留。
- 不修改项目的旧默认地图，不覆盖旧玩家存档。启动器使用 `Saved/Demo20/User` 隔离用户目录。
- UI 与玩法仍是首版，不代表完整游戏完成；验收范围和剩余事项见本阶段 Report。
- 没有执行 Cook 或 Package；旧自动开发任务仍保持暂停。

## 开发验证

本轮已在真实游戏窗口观察入口、HUD、鼠标指向与空击提示、闪身位移、攻击预警、受击死亡、返回后重新开局及暂停页面。持续 WASD 移动、持续按住格挡和完整击破三石卫后撤出，仍需人工实际游玩确认；不要将领域测试的通关断言当作真实键鼠通关证据。

![新石庭与 HUD](Evidence/arena.png)

关闭游戏后运行 `./Scripts/Test-Demo20.ps1`。该脚本分别运行 Demo20、CombatCore、BasicSword、VitalityAuthority 和 VitalityLedger 五个映射测试组，检查失败结果、结束记录和原生退出码，再运行路径覆盖检查；输出证据目录与 SHA-256。构建、生成和运行日志均在 `Saved/FoundationRuns/Demo20.S01*` 下独立保留。
