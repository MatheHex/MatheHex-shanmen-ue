# 山门 Demo2.0 本地运行

当前 Demo2.0 提供持久仓库与整备网格，以及不结算物品的石庭战斗练习。完整阶段目标是 [单关探索、随机搜集、撤离与死亡循环](../Prompt/Demo20.Expedition.goal.md)，仍在开发；当前整备增量不是完整探索 Demo。使用独立地图和占位美术，不沿用旧 Demo 材料。

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

## 仓库与整备

入口点击“仓库与整备”，或在整备入口按输入注册表中的背包键，默认 Tab。隔离档经现有唯一物品权威初始化，不读旧 Code A／Code B 库存。新档获得基础装备、少量丹药和一次性 1,000,000 测试灵石；重启不会重发初始物品或货币。

物品目录相对于运行时 `ProjectSavedDir` 为 `Demo20/<档名>/ShanmenItems/Authority`。启动器指定 `-UserDir=Saved/Demo20/User`，因此当前启动方式的默认物品档实际位于工程内 `Saved/Demo20/User/Saved/Demo20/ExpeditionProfile/ShanmenItems/Authority`。不要将它误认为工程根下的 `Saved/Demo20/ExpeditionProfile`，也不要搬移或删除已有档来“修复”路径。

- 点击选择物品；拖到合法格子移动，同类物品拖到一起合并，目标达到堆叠上限时余量留在原来源。
- 下方按钮支持旋转所选物品、拆分一半、仓库与普通背包间便捷转移、装备替换及返回入口。
- 普通背包容量来自已装备行囊：基础 6×4，扩容 8×5；护命匣提供 2×2 安全格。缩容放不下时拒绝并保留全部原状态，先整理内容再更换。
- 储物装备不能放进普通背包或安全格。安全格只允许丹药、材料与普通战利品；不能放入武器、护具或储物装备。
- 所有位置、数量和装备变化在保存确认后刷新。保存失败或状态不确定时不显示成功。

安全格死亡保留、装备战斗属性、治疗使用、基础补给、正式探索出发、随机掉落及终局恢复尚未接通。石庭练习不会损失整备物品，也不会发放战利品。物品目录与当前限制见 [整备物品目录](PreparationCatalog.md)。

独立验收档与分辨率可通过正常启动参数指定：

```powershell
./Scripts/Start-Demo20.ps1 -Action Play -ProfileName LocalAcceptance01 -Width 1920 -Height 1080 -TestMoney 1000000
```

档名仅允许字母、数字、下划线和连字符，最多 64 字符；目录始终位于运行时 `ProjectSavedDir/Demo20`。`TestMoney` 仅在该档首次创建时生效，已有档不会增加或重置货币。没有商店买卖系统。不要通过删除存档来模拟安全重试。

## 石庭练习与操作

点击“石庭练习（不结算）”，靠近石卫，将鼠标指向目标并出剑。赤色范围出现后约 0.8 秒发动攻击；移出范围、闪身或格挡。三座石卫全部击破后，返回入口青色归阵，按交互键撤出。死亡或主动结束后可以返回入口重新开始。这是历史战斗练习，不是探索 Goal 中开局可用的撤离流程。

| 默认输入 | 操作 |
| --- | --- |
| W/A/S/D | 移动 |
| 鼠标位置 | 指向出剑方向 |
| 鼠标左键 | 近距离出剑 |
| Space | 闪身，有恢复时间 |
| 按住鼠标右键 | 格挡，移动减速，不能同时出剑 |
| G | 在青色归阵处撤出 |
| Tab | 整备入口打开或关闭仓库与背包 |
| Esc | 整备背包内返回入口；练习中暂停或继续 |
| Alt+F4 | 关闭本地游戏窗口 |

实际键位读取既有输入注册表，若本机有自定义绑定，以游戏底部提示为准。窗口失焦时会暂停，需要点击继续。

## 当前范围与安全边界

- 玩家与石卫生命值、伤害、防御计算使用现有领域框架。
- 整备物品持久保存；石庭战斗仍是临时练习，关闭后战斗进度不保留。没有治疗、拾取、永久奖励或原局恢复产品接线。
- 不修改旧默认地图，不覆盖旧玩家存档。隔离配置与物品权威是不同用途；启动器指定的 UserDir 也会重定位运行时 ProjectSavedDir，物品实际路径见上文。
- 当前完成度与剩余事项见 [阶段状态](ExpeditionStatus.md)，不代表完整游戏完成。
- 没有执行 Cook 或 Package；旧自动开发任务仍保持暂停。

## 开发验证

历史战斗切片已观察入口、HUD、鼠标指向与空击提示、闪身、预警、死亡、重新开局及暂停页面。下面的历史石庭截图不证明新整备 UI 或完整探索流程；各增量新增的实际运行证据与待验收项见相应 Report。持续战斗和完整探索循环仍须真实键鼠验收，不以领域测试替代。

最新整备增量已实际操作移动、旋转、拆分、合并、装备替换、容量变化、缩容拒绝、便捷转移与重启读档。最终候选整备页在实际 1280×720 和 1920×1080 视口下核验；桌面截图受 Windows DPI 影响并非相同像素尺寸。证据与未验收项见 [持久整备 Report](../Report/Demo20.M1.Preparation.r0_report.md) 和 [Development Log](../Log/Demo20.M1.Preparation.r0_log.md)。

![最终候选整备页与安全格拒绝提示，实际视口 1920×1080](Evidence/M1.Preparation/secure-policy-1920x1080.png)

![新石庭与 HUD](Evidence/arena.png)

关闭游戏后可运行 `./Scripts/Test-Demo20.ps1`，覆盖 Demo20、CombatCore、BasicSword、VitalityAuthority、VitalityLedger 与 Items 六组。开发阶段使用 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.Development` 从实际改动文件推导全部映射组，包括适用的旧物品与快捷栏回归。脚本检查失败结果、结束记录、原生退出码、路径覆盖并输出 SHA-256。原始证据在 `Saved/FoundationRuns/<TaskId>` 下独立保留。
