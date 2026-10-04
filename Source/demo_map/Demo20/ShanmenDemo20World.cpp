#include "ShanmenDemo20World.h"
#include "ShanmenDemo20Widget.h"
#include "ShanmenDemo20Catalog.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "UnrealClient.h"

namespace
{
	FKey Key(FName Id) { return Fdemo_mapInputBindingSettings::Get().GetKey(Id); }
	AShanmenDemo20GameMode* Mode(const UObject* Object)
	{
		return Object && Object->GetWorld() ? Cast<AShanmenDemo20GameMode>(Object->GetWorld()->GetAuthGameMode()) : nullptr;
	}
	void Tint(UStaticMeshComponent* Mesh, FLinearColor Color)
	{
		if (auto* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0)) Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

AShanmenDemo20Character::AShanmenDemo20Character()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 80.f);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 480.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2400.f;
	auto* Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Demo20CameraArm"));
	Arm->SetupAttachment(RootComponent);
	Arm->SetUsingAbsoluteRotation(true);
	Arm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	Arm->TargetArmLength = 1700.f;
	Arm->bDoCollisionTest = false;
	auto* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Demo20Camera"));
	Camera->SetupAttachment(Arm, USpringArmComponent::SocketName);
	Camera->FieldOfView = 58.f;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Palette(TEXT("/Game/Demo20/Materials/M_Demo20_Color.M_Demo20_Color"));
	Robe = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Robe"));
	Robe->SetupAttachment(RootComponent);
	Robe->SetStaticMesh(Cylinder.Object);
	Robe->SetMaterial(0, Palette.Object);
	Robe->SetRelativeScale3D(FVector(.52f, .52f, 1.25f));
	Robe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Blade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PracticeSword"));
	Blade->SetupAttachment(RootComponent);
	Blade->SetStaticMesh(Cube.Object);
	Blade->SetMaterial(0, Palette.Object);
	Blade->SetRelativeLocation(FVector(52.f, 24.f, 12.f));
	Blade->SetRelativeScale3D(FVector(1.1f, .06f, .06f));
	Blade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AShanmenDemo20Character::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const auto* Game = Mode(this);
	if (!Game) return;
	const auto& State = Game->GetSession();
	// Reuse the existing dynamic instance: do not allocate a material per frame.
	auto SetColor = [](UStaticMeshComponent* ShapeMesh, FLinearColor Color)
	{
		auto* Material = Cast<UMaterialInstanceDynamic>(ShapeMesh->GetMaterial(0));
		if (!Material) Material = ShapeMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (Material) Material->SetVectorParameterValue(TEXT("Color"), Color);
	};
	SetColor(Robe, State.IsEvading() ? FLinearColor(.65f, .95f, .90f) : State.IsGuarding() ? FLinearColor(.78f, .59f, .22f) : FLinearColor(.08f, .40f, .35f));
	SetColor(Blade, FLinearColor(.80f, .85f, .70f));
	Blade->SetRelativeRotation(FRotator(0.f, State.GetAttackCooldown() > .20f ? -50.f : 0.f, 0.f));
}

AShanmenDemo20Controller::AShanmenDemo20Controller()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AShanmenDemo20Controller::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(Key(Fdemo_mapInputActionIds::PrimaryAttack), IE_Pressed, this, &AShanmenDemo20Controller::Attack);
	InputComponent->BindKey(Key(Fdemo_mapInputActionIds::SpiritEvasion), IE_Pressed, this, &AShanmenDemo20Controller::Evade);
	InputComponent->BindKey(Key(Fdemo_mapInputActionIds::Interact), IE_Pressed, this, &AShanmenDemo20Controller::Interact);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AShanmenDemo20Controller::ToggleMenu);
	InputComponent->BindKey(Key(Fdemo_mapInputActionIds::Inventory), IE_Pressed, this, &AShanmenDemo20Controller::ToggleInventory);
}

void AShanmenDemo20Controller::ApplySurface(bool bGameplay)
{
	FlushPressedKeys();
	MoveDirection = FVector::ZeroVector;
	if (auto* Avatar = Cast<ACharacter>(GetPawn())) Avatar->GetCharacterMovement()->StopMovementImmediately();
	if (bGameplay)
	{
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		SetInputMode(InputMode);
	}
	else
	{
		// Buttons consume clicks; Escape remains available to dismiss pause.
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	bShowMouseCursor = true;
	SetIgnoreMoveInput(false);
	if (!bGameplay) if (auto* Game = Mode(this)) Game->SetGuard(false);
}

void AShanmenDemo20Controller::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);
	auto* Game = Mode(this);
	if (!Game || !GetPawn()) return;
	const auto* Viewport = GetWorld()->GetGameViewport();
	// UMG temporarily owns keyboard focus during button activation. That is not
	// application deactivation and must not immediately pause a newly started run.
	if (!Viewport || !Viewport->Viewport || !FSlateApplication::Get().IsActive())
	{
		Game->PauseForFocusLoss();
		MoveDirection = FVector::ZeroVector;
		return;
	}
	if (!Game->IsPlaying()) return;
	const float Forward = (IsInputKeyDown(Key(Fdemo_mapInputActionIds::MoveForward)) ? 1.f : 0.f)
		- (IsInputKeyDown(Key(Fdemo_mapInputActionIds::MoveBackward)) ? 1.f : 0.f);
	const float Right = (IsInputKeyDown(Key(Fdemo_mapInputActionIds::MoveRight)) ? 1.f : 0.f)
		- (IsInputKeyDown(Key(Fdemo_mapInputActionIds::MoveLeft)) ? 1.f : 0.f);
	MoveDirection = FVector(Forward, Right, 0.f).GetSafeNormal();
	Game->SetGuard(IsInputKeyDown(Key(Fdemo_mapInputActionIds::WeaponGuard)));
	if (auto* Avatar = Cast<ACharacter>(GetPawn()))
	{
		Avatar->GetCharacterMovement()->MaxWalkSpeed = Game->GetSession().IsGuarding() ? 190.f : 480.f;
		Avatar->AddMovementInput(MoveDirection);
	}
	FVector Origin, Direction;
	if (DeprojectMousePositionToWorld(Origin, Direction) && FMath::Abs(Direction.Z) > .001f)
	{
		const float T = (GetPawn()->GetActorLocation().Z - Origin.Z) / Direction.Z;
		if (T > 0.f)
		{
			FVector Aim = Origin + Direction * T - GetPawn()->GetActorLocation();
			Aim.Z = 0.f;
			if (!Aim.IsNearlyZero()) GetPawn()->SetActorRotation(Aim.Rotation());
		}
	}
}

void AShanmenDemo20Controller::Attack() { if (auto* Game = Mode(this)) Game->Attack(); }
void AShanmenDemo20Controller::Evade() { if (auto* Game = Mode(this)) Game->Evade(); }
void AShanmenDemo20Controller::Interact() { if (auto* Game = Mode(this)) Game->Interact(); }
void AShanmenDemo20Controller::ToggleMenu() { if (auto* Game = Mode(this)) Game->TogglePause(); }
void AShanmenDemo20Controller::ToggleInventory() { if (auto* Game = Mode(this)) Game->ToggleInventory(); }

AShanmenDemo20GameMode::AShanmenDemo20GameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AShanmenDemo20Character::StaticClass();
	PlayerControllerClass = AShanmenDemo20Controller::StaticClass();
	HUDClass = nullptr;
}

FVector AShanmenDemo20GameMode::SentinelLocation(int32 Index)
{
	const FVector Positions[3] = { FVector(40.f, -430.f, 65.f), FVector(480.f, 0.f, 65.f), FVector(40.f, 430.f, 65.f) };
	return Index >= 0 && Index < 3 ? Positions[Index] : FVector::ZeroVector;
}

AActor* AShanmenDemo20GameMode::AddShape(const FVector& Location, const FVector& Scale, const FLinearColor& Color, bool bCollision, bool bCylinder)
{
	AActor* Actor = GetWorld()->SpawnActor<AActor>(Location, FRotator::ZeroRotator);
	if (!Actor) return nullptr;
	auto* Mesh = NewObject<UStaticMeshComponent>(Actor);
	Actor->SetRootComponent(Mesh);
	Actor->AddInstanceComponent(Mesh);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, bCylinder ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder") : TEXT("/Engine/BasicShapes/Cube.Cube")));
	auto* Palette = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Demo20/Materials/M_Demo20_Color.M_Demo20_Color"));
	if (!Palette) { Actor->Destroy(); return nullptr; }
	Mesh->SetMaterial(0, Palette);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Mesh->SetCollisionObjectType(ECC_WorldStatic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->RegisterComponent();
	Actor->SetActorLocation(Location);
	Actor->SetActorScale3D(Scale);
	Tint(Mesh, Color);
	ArenaActors.Add(Actor);
	return Actor;
}

bool AShanmenDemo20GameMode::BuildArena()
{
	const FLinearColor Stone(.25f, .30f, .29f), Dark(.07f, .115f, .105f), Gold(.55f, .40f, .16f);
	if (!AddShape(FVector(0,0,-35), FVector(26,22,.7f), Stone, true)) return false;
	for (int32 Side : {-1, 1})
	{
		if (!AddShape(FVector(0, Side * 1090, 55), FVector(26,.4f,1.8f), Dark, true)
			|| !AddShape(FVector(Side * 1290,0,55), FVector(.4f,22,1.8f), Dark, true)) return false;
	}
	for (int32 X = -5; X <= 5; ++X) AddShape(FVector(X * 220,0,1), FVector(.018f,21,.015f), Dark, false);
	for (int32 Y = -4; Y <= 4; ++Y) AddShape(FVector(0,Y * 220,1), FVector(25,.018f,.015f), Dark, false);
	AddShape(FVector(120,0,2), FVector(14,14,.04f), Gold, false, true);
	AddShape(FVector(120,0,5), FVector(13.6f,13.6f,.04f), Stone, false, true);
	for (int32 Y : {-880, 880}) for (int32 X : {-950, -400, 400, 950})
	{
		AddShape(FVector(X,Y,130), FVector(.9f,.9f,2.6f), Dark, true, true);
		AddShape(FVector(X,Y,270), FVector(1.1f,1.1f,.15f), Gold, false);
	}
	ExitMarker = AddShape(ExitLocation() + FVector(0,0,3), FVector(3.4f,3.4f,.08f), FLinearColor(.12f,.55f,.43f), false, true);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Sentinels.Add(AddShape(SentinelLocation(Index), FVector(.9f,.9f,1.3f), FLinearColor(.46f,.24f,.13f), true, true));
		Warnings.Add(AddShape(SentinelLocation(Index) * FVector(1,1,0) + FVector(0,0,8), FVector(5.2f,5.2f,.04f), FLinearColor(.55f,.12f,.04f), false, true));
		if (!Sentinels.Last() || !Warnings.Last()) return false;
		Warnings.Last()->SetActorHiddenInGame(true);
	}
	auto* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1000), FRotator(-55,-25,0));
	auto* Sky = GetWorld()->SpawnActor<ASkyLight>();
	if (!Sun || !Sky || !ExitMarker) return false;
	Sun->GetLightComponent()->SetIntensity(5.f);
	Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f,.90f,.73f));
	Sky->GetLightComponent()->SetIntensity(1.2f);
	ArenaActors.Add(Sun);
	ArenaActors.Add(Sky);
	return true;
}

void AShanmenDemo20GameMode::BeginPlay()
{
	Super::BeginPlay();
	bWorldReady = BuildArena();
	int32 TestMoney = 1000000;
	FParse::Value(FCommandLine::Get(), TEXT("Demo20TestMoney="), TestMoney);
	FString ProfileName = TEXT("ExpeditionProfile");
	FParse::Value(FCommandLine::Get(), TEXT("Demo20ProfileName="), ProfileName);
	bool ValidProfileName = !ProfileName.IsEmpty() && ProfileName.Len() <= 64;
	for (TCHAR C : ProfileName) ValidProfileName &= (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z')
		|| (C >= '0' && C <= '9') || C == '_' || C == '-';
	if (auto* Authority = ValidProfileName ? GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>() : nullptr)
	{
		const auto Initial = FShanmenDemo20Catalog::Initial(TestMoney);
		const auto Bound = Authority->BindNativeProfile(Fdemo_mapProfileStorageContext::ForRoot(
			FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Demo20"), ProfileName)),
			FShanmenDemo20Catalog::OwnerId(), FShanmenDemo20Catalog::ProductId(), Initial);
		bProfileReady = Bound.IsReady();
		UE_LOG(LogTemp, Display, TEXT("DEMO20_PROFILE Ready=%d Native=%d Generation=%d Diagnostic=%s"),
			bProfileReady, !Bound.bLegacyInputsRead, Bound.AuthorityStart.DocumentGeneration, *Bound.Diagnostic);
	}
	auto* Player = Cast<AShanmenDemo20Controller>(GetWorld()->GetFirstPlayerController());
	if (!Player) { bWorldReady = false; return; }
	if (Player->GetPawn()) Player->GetPawn()->SetActorLocation(ExitLocation() + FVector(0,0,100));
	else bWorldReady = false;
	Screen = CreateWidget<UShanmenDemo20Widget>(Player, UShanmenDemo20Widget::StaticClass());
	if (!Screen) { bWorldReady = false; return; }
	Screen->InitializeForDemo(this);
	Screen->AddToViewport(20);
	Notice = !bProfileReady ? TEXT("隔离物品档未能打开，整备已禁用。未覆盖旧档，请查看运行日志。")
		: bWorldReady ? TEXT("隔离物品档已就绪，可打开仓库与整备。石庭仅为不结算的战斗练习。")
		: TEXT("新场景初始化失败，请查看运行日志。未进入试炼。");
	RefreshSurface();
	UE_LOG(LogTemp, Display, TEXT("DEMO20_READY World=%s Ready=%d Actors=%d"), *GetWorld()->GetMapName(), bWorldReady, ArenaActors.Num());
}

void AShanmenDemo20GameMode::RefreshSurface()
{
	if (Screen) Screen->Refresh();
	if (auto* Player = Cast<AShanmenDemo20Controller>(GetWorld()->GetFirstPlayerController())) Player->ApplySurface(IsPlaying());
	if (Screen) Screen->FocusActiveSurface();
}

void AShanmenDemo20GameMode::StartTrial()
{
	if (bInventoryOpen) ToggleInventory();
	if (!bWorldReady || !Session.Begin(FGuid::NewGuid())) return;
	bPaused = false;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Sentinels[Index]->SetActorHiddenInGame(false);
		Sentinels[Index]->SetActorEnableCollision(true);
		Warnings[Index]->SetActorHiddenInGame(true);
		SentinelClocks[Index] = 0.f;
	}
	if (auto* Player = GetWorld()->GetFirstPlayerController()) if (APawn* Pawn = Player->GetPawn()) Pawn->SetActorLocation(ExitLocation() + FVector(0,0,100));
	Notice = TEXT("击败三座守阵石卫，再返回青色归阵。赤光出现时闪避或格挡。");
	NoticeTime = 6.f;
	RefreshSurface();
	UE_LOG(LogTemp, Display, TEXT("DEMO20_BEGIN Run=%s"), *Session.GetRunId().ToString());
}

bool AShanmenDemo20GameMode::TryCaptureItems(FShanmenItemAuthoritySnapshot& Out) const
{
	const auto* Authority = GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	return bProfileReady && Authority && Authority->TryCaptureSnapshot(Out);
}
FShanmenItemDurableCommandResult AShanmenDemo20GameMode::EditItemGrid(const FShanmenItemGridRequest& Intent)
{
	if (Session.GetPhase() != EShanmenDemo20Phase::Preparation || !bInventoryOpen || !bProfileReady)
	{
		FShanmenItemDurableCommandResult Rejected; Rejected.Diagnostic = TEXT("Inventory edits require the ready preparation surface."); return Rejected;
	}
	auto* Authority = GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!Authority) return {};
	const auto Result = Authority->EditGridDurable(Intent);
	bProfileReady = Authority->GetLifecycleState() == Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
	UE_LOG(LogTemp, Display, TEXT("DEMO20_GRID Success=%d Status=%d Error=%d Generation=%d"),
		Result.IsCommandSuccess(), static_cast<int32>(Result.Status), static_cast<int32>(Result.Receipt.Error), Result.DocumentGeneration);
	return Result;
}
void AShanmenDemo20GameMode::ToggleInventory()
{
	if (bInventoryOpen) { bInventoryOpen = false; RefreshSurface(); return; }
	if (Session.GetPhase() != EShanmenDemo20Phase::Preparation || !bProfileReady)
	{
		Notice = TEXT("当前仅整备支持持久背包；局内探索接线尚未完成。未执行物品操作。"); NoticeTime = 4.f; return;
	}
	bInventoryOpen = true; if (Screen) Screen->RefreshInventory(); RefreshSurface();
}

FString AShanmenDemo20GameMode::ReplenishBasicEquipment()
{
	if (Session.GetPhase() != EShanmenDemo20Phase::Preparation || !bInventoryOpen || !bProfileReady)
		return TEXT("补给不可用：请先返回已就绪的整备界面。");
	auto* Authority = GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority || !Authority->TryCaptureSnapshot(Snapshot)) return TEXT("物品档尚未就绪，未发放补给。");
	const auto Request = FShanmenDemo20Catalog::BasicSupply(Snapshot);
	if (!Request.DeathRequestId.IsValid()) return TEXT("无需领取：仅正式探索死亡后提供有限补给；石庭练习不计入。");
	const auto Result = Authority->ReplenishBasicsDurable(Request);
	bProfileReady = Authority->GetLifecycleState() == Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
	UE_LOG(LogTemp, Display, TEXT("DEMO20_RESUPPLY Success=%d Status=%d Error=%d Generation=%d Lines=%d"),
		Result.IsCommandSuccess(), static_cast<int32>(Result.Status), static_cast<int32>(Result.Receipt.Error), Result.DocumentGeneration, Result.Receipt.Amount);
	if (!Result.IsDurable()) return bProfileReady ? TEXT("补给未保存，已回滚；可重试，未确认发放。") : TEXT("物品档正在恢复，补给未确认；请重新打开游戏。");
	if (Result.IsCommandSuccess()) return Result.bRepositoryMutated
		? TEXT("补给已保存：缺失的基础装备已穿戴，缺药时仓库提供 2 枚丹药。")
		: TEXT("本次死亡的补给已经领取，没有重复发放。");
	switch (Result.Receipt.Error)
	{
	case EShanmenItemTransactionError::BasicSupplyNotNeeded: return TEXT("无需补给：已有可用装备和丹药；请检查仓库，不会额外发放。");
	case EShanmenItemTransactionError::BasicSupplyAlreadyUsed: return TEXT("本次死亡的补给机会已使用，不可反复领取。");
	case EShanmenItemTransactionError::BasicSupplyNotEligible: return TEXT("补给条件不满足：需要最近一次正式死亡结算，且尚未开始准备下一局。");
	case EShanmenItemTransactionError::GridNoSpace: return TEXT("仓库空间不足或背包仍有未处理物品；整理后再领取，未发放任何补给。");
	case EShanmenItemTransactionError::StaleAuthorityRevision: return TEXT("物品状态已变化，请刷新后重试；未发放补给。");
	default: return TEXT("补给被物品权威拒绝，未执行变更；详情已写入日志。");
	}
}

void AShanmenDemo20GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (const auto* Viewport = GetWorld()->GetGameViewport(); Viewport && Viewport->Viewport)
	{
		const FIntPoint Pixels = Viewport->Viewport->GetSizeXY();
		if (Pixels != LastViewportPixels && Pixels.X > 0 && Pixels.Y > 0)
		{
			LastViewportPixels = Pixels;
			UE_LOG(LogTemp, Display, TEXT("DEMO20_VIEWPORT Pixels=%dx%d WidgetScale=%.3f"),
				Pixels.X, Pixels.Y, UWidgetLayoutLibrary::GetViewportScale(this));
		}
	}
	const auto Previous = Session.GetPhase();
	if (IsPlaying())
	{
		Session.Advance(DeltaSeconds);
		UpdateSentinels(DeltaSeconds);
		NoticeTime = FMath::Max(0.f, NoticeTime - DeltaSeconds);
		if (NoticeTime <= 0.f) Notice = Session.NumDefeated() == 3 ? TEXT("阵门已开启：返回青色归阵，按交互键撤出。") : TEXT("观察赤光蓄势，侧移或防御，再靠近出剑。");
	}
	if (Previous != Session.GetPhase()) RefreshSurface();
	if (Screen) Screen->Refresh();
}

void AShanmenDemo20GameMode::UpdateSentinels(float DeltaSeconds)
{
	const auto* Player = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!Pawn) return;
	for (int32 Index = 0; Index < 3 && IsPlaying(); ++Index)
	{
		if (Session.GetHealth(Index + 1) <= 0.f) continue;
		const float Distance = FVector::Dist2D(Pawn->GetActorLocation(), SentinelLocation(Index));
		float& Clock = SentinelClocks[Index];
		if (Clock < 0.f) Clock = FMath::Min(0.f, Clock + DeltaSeconds);
		else if (Clock > 0.f || Distance < 260.f)
		{
			Clock += DeltaSeconds;
			Warnings[Index]->SetActorHiddenInGame(false);
			if (Clock >= .8f)
			{
				if (Distance < 260.f && Session.ReceiveSentinelStrike(Index))
				{
					Notice = Session.IsEvading() ? TEXT("闪避成功") : Session.IsGuarding() ? TEXT("格挡 · 伤害减免") : TEXT("受到攻击，留意赤色蓄势范围。");
					NoticeTime = 1.5f;
				}
				Clock = -1.1f;
				Warnings[Index]->SetActorHiddenInGame(true);
			}
		}
	}
}

void AShanmenDemo20GameMode::Attack()
{
	if (!IsPlaying()) return;
	const auto* Player = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!Pawn) return;
	int32 Best = INDEX_NONE;
	float Nearest = 250.f;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FVector Offset = SentinelLocation(Index) - Pawn->GetActorLocation();
		const float Distance = Offset.Size2D();
		if (Session.GetHealth(Index + 1) > 0.f && Distance < Nearest
			&& FVector::DotProduct(Offset.GetSafeNormal2D(), Pawn->GetActorForwardVector()) > .25f)
		{
			Best = Index;
			Nearest = Distance;
		}
	}
	if (Best != INDEX_NONE && Session.StrikeSentinel(Best))
	{
		Notice = TEXT("剑击命中");
		NoticeTime = 1.f;
		if (Session.GetHealth(Best + 1) <= 0.f)
		{
			Sentinels[Best]->SetActorHiddenInGame(true);
			Sentinels[Best]->SetActorEnableCollision(false);
			Warnings[Best]->SetActorHiddenInGame(true);
			Notice = TEXT("守阵石卫已击破");
		}
		UE_LOG(LogTemp, Display, TEXT("DEMO20_HIT Target=%d Health=%.1f"), Best, Session.GetHealth(Best + 1));
	}
	else if (Session.GetAttackCooldown() <= 0.f)
	{
		Notice = TEXT("未命中：靠近石卫，并将鼠标指向目标。格挡时不能出剑。");
		NoticeTime = 1.5f;
	}
}

void AShanmenDemo20GameMode::Evade()
{
	if (!IsPlaying()) return;
	auto* Player = Cast<AShanmenDemo20Controller>(GetWorld()->GetFirstPlayerController());
	auto* Pawn = Player ? Cast<ACharacter>(Player->GetPawn()) : nullptr;
	if (Pawn && Session.TryEvade())
	{
		const FVector Direction = Player->GetMoveDirection().IsNearlyZero() ? Pawn->GetActorForwardVector() : Player->GetMoveDirection();
		Pawn->LaunchCharacter(Direction * 1150.f + FVector(0,0,35), true, true);
		Notice = TEXT("闪身");
		NoticeTime = .6f;
	}
}

void AShanmenDemo20GameMode::Interact()
{
	if (!IsPlaying()) return;
	const auto* Player = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!Pawn || FVector::Dist2D(Pawn->GetActorLocation(), ExitLocation()) > 210.f)
	{
		Notice = TEXT("请靠近入口处的青色归阵。");
		NoticeTime = 2.f;
		return;
	}
	if (!Session.TryExtract())
	{
		Notice = TEXT("归阵尚未开启：先击破全部三座石卫。");
		NoticeTime = 2.f;
		return;
	}
	RefreshSurface();
	UE_LOG(LogTemp, Display, TEXT("DEMO20_EXTRACT Run=%s Time=%.2f"), *Session.GetRunId().ToString(), Session.GetElapsed());
}

void AShanmenDemo20GameMode::SetGuard(bool bHeld) { Session.SetGuarding(bHeld && IsPlaying()); }
void AShanmenDemo20GameMode::TogglePause()
{
	if (bInventoryOpen) { ToggleInventory(); return; }
	if (Session.GetPhase() != EShanmenDemo20Phase::Active) return;
	bPaused = !bPaused;
	Session.SetGuarding(false);
	RefreshSurface();
}
void AShanmenDemo20GameMode::PauseForFocusLoss()
{
	if (IsPlaying())
	{
		bPaused = true;
		Session.SetGuarding(false);
		Notice = TEXT("窗口失去焦点，试炼已暂停。点击继续后恢复。");
		RefreshSurface();
	}
}
void AShanmenDemo20GameMode::LeaveTrial()
{
	Session.Abandon();
	bPaused = false;
	RefreshSurface();
}
void AShanmenDemo20GameMode::ReturnToPreparation()
{
	if (!Session.ReturnToPreparation()) return;
	bPaused = false;
	Notice = TEXT("可再次开始独立试炼。未发放奖励，未修改持久存档。");
	RefreshSurface();
}
void AShanmenDemo20GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Screen) Screen->RemoveFromParent();
	Screen = nullptr;
	Session.Abandon();
	UE_LOG(LogTemp, Display, TEXT("DEMO20_END Run=%s Impacts=%d"), *Session.GetRunId().ToString(), Session.GetImpactCount());
	Super::EndPlay(EndPlayReason);
}
