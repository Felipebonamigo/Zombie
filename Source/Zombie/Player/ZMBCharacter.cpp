#include "Player/ZMBCharacter.h"
#include "Zombie.h"
#include "Combat/ZMBExplosive.h"
#include "Combat/ZMBFx.h"
#include "Combat/ZMBTypes.h"
#include "Game/ZMBGameData.h"
#include "Game/ZMBGameMode.h"
#include "Game/ZMBWorldBuilder.h"
#include "Stream/ZMBStreamSubsystem.h"
#include "Vehicles/ZMBCar.h"
#include "Zombies/ZMBHordeDirector.h"
#include "Zombies/ZMBZombie.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DamageEvents.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ZombiePlayer"

namespace
{
	constexpr int32 GunPart = 6;
	constexpr int32 PhonePart = 7;
	constexpr float EyeHeight = 72.0f;
}

AZMBCharacter::AZMBCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(34.0f, 88.0f);
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
	Move->JumpZVelocity = 520.0f;
	Move->AirControl = 0.3f;
	Move->MaxWalkSpeed = 400.0f;
	Move->BrakingDecelerationWalking = 1800.0f;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Capsule);
	CameraBoom->TargetArmLength = 900.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.0f;
	CameraBoom->ProbeSize = 14.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	struct FPartSpec
	{
		const TCHAR* Name;
		UStaticMesh* Mesh;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
	};
	// corpo de protótipo (escala 1, cápsula de 1,76 m): braço direito à frente segurando a arma, celular preso no peito
	const FPartSpec Specs[] = {
		{TEXT("Head"), SphereMesh.Object, FVector(2.0f, 0.0f, 71.0f), FRotator::ZeroRotator, FVector(0.27f)},
		{TEXT("Torso"), CubeMesh.Object, FVector(0.0f, 0.0f, 25.0f), FRotator::ZeroRotator, FVector(0.25f, 0.44f, 0.6f)},
		{TEXT("ArmL"), CylinderMesh.Object, FVector(0.0f, -29.0f, 26.0f), FRotator::ZeroRotator, FVector(0.11f, 0.11f, 0.6f)},
		{TEXT("ArmR"), CylinderMesh.Object, FVector(22.0f, 27.0f, 42.0f), FRotator(-80.0f, 0.0f, 0.0f), FVector(0.11f, 0.11f, 0.6f)},
		{TEXT("LegL"), CylinderMesh.Object, FVector(0.0f, -11.0f, -46.0f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 0.84f)},
		{TEXT("LegR"), CylinderMesh.Object, FVector(0.0f, 11.0f, -46.0f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 0.84f)},
		{TEXT("Gun"), CubeMesh.Object, FVector(56.0f, 27.0f, 48.0f), FRotator::ZeroRotator, FVector(0.34f, 0.06f, 0.13f)},
		{TEXT("Phone"), CubeMesh.Object, FVector(14.0f, 0.0f, 40.0f), FRotator::ZeroRotator, FVector(0.02f, 0.08f, 0.15f)},
	};
	for (const FPartSpec& Spec : Specs)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Spec.Name);
		Part->SetupAttachment(Capsule);
		Part->SetStaticMesh(Spec.Mesh);
		Part->SetRelativeLocation(Spec.Location);
		Part->SetRelativeRotation(Spec.Rotation);
		Part->SetRelativeScale3D(Spec.Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastHiddenShadow(true);
		Parts.Add(Part);
	}
}

void AZMBCharacter::BeginPlay()
{
	Super::BeginPlay();
	UZMBGameData* Data = UZMBGameData::Get(this);
	if (Data)
	{
		const ZR::FTuning& Tuning = Data->Tuning();
		Health = Tuning.Player.Health;
		Stamina = Tuning.Player.Stamina;
		Focus = Tuning.Player.FocusMax;
		Grenades = Tuning.Player.StartGrenades;
		Bombs = Tuning.Player.StartBombs;
		CameraMode = FMath::Clamp(Tuning.Camera.StartMode, 0, 3);
		LastThirdPersonMode = CameraMode == 3 ? 2 : CameraMode;
		for (const ZR::FWeaponType& Weapon : Data->Weapons())
		{
			FWeaponState State;
			State.Magazine = Weapon.Magazine;
			State.Reserve = Weapon.StartReserve;
			WeaponStates.Add(State);
		}
		const FLinearColor Skin(0.50f, 0.34f, 0.24f);
		const FLinearColor Jacket(0.04f, 0.07f, 0.11f);
		const FLinearColor Jeans(0.03f, 0.05f, 0.09f);
		const FLinearColor Black(0.01f, 0.01f, 0.012f);
		const FLinearColor Colors[] = {Skin, Jacket, Jacket, Jacket, Jeans, Jeans, Black, Black};
		for (int32 i = 0; i < Parts.Num() && i < static_cast<int32>(UE_ARRAY_COUNT(Colors)); ++i)
		{
			Parts[i]->SetMaterial(0, Data->ColorMaterial(Colors[i], i == GunPart ? 0.3f : 0.75f, i == GunPart ? 0.8f : 0.0f));
		}
	}
	for (UStaticMeshComponent* Part : Parts)
	{
		PartRest.Add(Part->GetRelativeTransform());
	}
	if (AZMBGameMode* GameMode = AZMBGameMode::Get(this))
	{
		bShowHelp = !GameMode->IsAutomatedRun();
	}
	int32 StartCamera = -1;
	if (FParse::Value(FCommandLine::Get(), TEXT("ZMBCam="), StartCamera) && StartCamera >= 0 && StartCamera <= 3)
	{
		CameraMode = StartCamera;
		LastThirdPersonMode = StartCamera == 3 ? 2 : StartCamera;
	}
}

float AZMBCharacter::GetMaxHealth() const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	return Data ? Data->Tuning().Player.Health : 100.0f;
}

float AZMBCharacter::GetStamina01() const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	return Data ? Stamina / FMath::Max(1.0f, Data->Tuning().Player.Stamina) : 1.0f;
}

float AZMBCharacter::GetFocus01() const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	return Data ? Focus / FMath::Max(1.0f, Data->Tuning().Player.FocusMax) : 1.0f;
}

const ZR::FWeaponType* AZMBCharacter::CurrentWeaponType() const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || CurrentWeapon < 0 || CurrentWeapon >= static_cast<int32>(Data->Weapons().size()))
	{
		return nullptr;
	}
	return &Data->Weapons()[static_cast<size_t>(CurrentWeapon)];
}

FString AZMBCharacter::GetWeaponName() const
{
	const ZR::FWeaponType* Weapon = CurrentWeaponType();
	return Weapon ? UZMBGameData::ToFString(Weapon->NamePt) : FString();
}

int32 AZMBCharacter::GetMagazine() const
{
	return WeaponStates.IsValidIndex(CurrentWeapon) ? WeaponStates[CurrentWeapon].Magazine : 0;
}

int32 AZMBCharacter::GetReserve() const
{
	return WeaponStates.IsValidIndex(CurrentWeapon) ? WeaponStates[CurrentWeapon].Reserve : 0;
}

float AZMBCharacter::GetSpread01() const
{
	const ZR::FWeaponType* Weapon = CurrentWeaponType();
	const float Base = Weapon ? Weapon->SpreadDeg : 1.0f;
	return FMath::Clamp((Base * (bAiming ? 0.5f : 1.0f) + SpreadKick) / 8.0f, 0.0f, 1.0f);
}

UInputAction* AZMBCharacter::NewAction(const TCHAR* Name, bool bAxis2D, bool bAxis1D)
{
	UInputAction* Action = NewObject<UInputAction>(this, Name);
	Action->ValueType = bAxis2D ? EInputActionValueType::Axis2D : (bAxis1D ? EInputActionValueType::Axis1D : EInputActionValueType::Boolean);
	Actions.Add(Action);
	return Action;
}

void AZMBCharacter::CreateInput()
{
	if (Mapping)
	{
		return;
	}
	Mapping = NewObject<UInputMappingContext>(this, TEXT("ZMB_PlayerMapping"));
	UInputAction* MoveAction = NewAction(TEXT("ZMB_Move"), true);
	UInputAction* LookAction = NewAction(TEXT("ZMB_Look"), true);
	// WASD num eixo 2D: W = +Y (troca X por Y), S = -Y, A = -X, D = +X
	Mapping->MapKey(MoveAction, EKeys::D);
	FEnhancedActionKeyMapping& Left = Mapping->MapKey(MoveAction, EKeys::A);
	Left.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
	FEnhancedActionKeyMapping& Forward = Mapping->MapKey(MoveAction, EKeys::W);
	Forward.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Mapping));
	FEnhancedActionKeyMapping& Back = Mapping->MapKey(MoveAction, EKeys::S);
	Back.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Mapping));
	Back.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
	Mapping->MapKey(MoveAction, EKeys::Gamepad_Left2D);
	Mapping->MapKey(LookAction, EKeys::Mouse2D);
	Mapping->MapKey(LookAction, EKeys::Gamepad_Right2D);

	struct FButton
	{
		const TCHAR* Name;
		FKey Key;
		FKey Pad;
	};
	const FButton Buttons[] = {
		{TEXT("ZMB_Jump"), EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom},
		{TEXT("ZMB_Sprint"), EKeys::LeftShift, EKeys::Gamepad_LeftThumbstick},
		{TEXT("ZMB_Fire"), EKeys::LeftMouseButton, EKeys::Gamepad_RightTrigger},
		{TEXT("ZMB_Aim"), EKeys::RightMouseButton, EKeys::Gamepad_LeftTrigger},
		{TEXT("ZMB_Reload"), EKeys::R, EKeys::Gamepad_FaceButton_Left},
		{TEXT("ZMB_Kick"), EKeys::F, EKeys::Gamepad_FaceButton_Right},
		{TEXT("ZMB_Grenade"), EKeys::G, EKeys::Gamepad_RightShoulder},
		{TEXT("ZMB_Bomb"), EKeys::H, EKeys::Gamepad_DPad_Left},
		{TEXT("ZMB_SlowMo"), EKeys::Q, EKeys::Gamepad_LeftShoulder},
		{TEXT("ZMB_Interact"), EKeys::E, EKeys::Gamepad_FaceButton_Top},
		{TEXT("ZMB_Camera"), EKeys::C, EKeys::Gamepad_DPad_Up},
		{TEXT("ZMB_FirstPerson"), EKeys::V, EKeys::Gamepad_DPad_Down},
		{TEXT("ZMB_Clip"), EKeys::B, EKeys::Gamepad_Special_Left},
		{TEXT("ZMB_NextWeapon"), EKeys::MouseScrollDown, EKeys::Gamepad_DPad_Right},
		{TEXT("ZMB_Horde"), EKeys::N, EKeys::Invalid},
		{TEXT("ZMB_Time"), EKeys::T, EKeys::Invalid},
		{TEXT("ZMB_Help"), EKeys::F1, EKeys::Gamepad_Special_Right},
	};
	for (const FButton& Button : Buttons)
	{
		UInputAction* Action = NewAction(Button.Name, false);
		Mapping->MapKey(Action, Button.Key);
		if (Button.Pad.IsValid())
		{
			Mapping->MapKey(Action, Button.Pad);
		}
	}

	const FKey SlotKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
	for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(SlotKeys)); ++i)
	{
		UInputAction* Action = NewObject<UInputAction>(this, *FString::Printf(TEXT("ZMB_Slot%d"), i + 1));
		Action->ValueType = EInputActionValueType::Boolean;
		Mapping->MapKey(Action, SlotKeys[i]);
		SlotActions.Add(Action);
	}
}

void AZMBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateInput();
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(Mapping, 0);
		}
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input || Actions.Num() < 19)
	{
		UE_LOG(LogZombie, Error, TEXT("AZMBCharacter: precisa do Enhanced Input Component (Config/DefaultInput.ini)."));
		return;
	}
	// a ordem é a de CreateInput: Move, Look, depois os botões na ordem da tabela
	Input->BindAction(Actions[0], ETriggerEvent::Triggered, this, &AZMBCharacter::Move);
	Input->BindAction(Actions[0], ETriggerEvent::Completed, this, &AZMBCharacter::Move);
	Input->BindAction(Actions[1], ETriggerEvent::Triggered, this, &AZMBCharacter::Look);
	Input->BindAction(Actions[2], ETriggerEvent::Started, this, &AZMBCharacter::JumpOn);
	Input->BindAction(Actions[2], ETriggerEvent::Completed, this, &AZMBCharacter::JumpOff);
	Input->BindAction(Actions[3], ETriggerEvent::Started, this, &AZMBCharacter::SprintOn);
	Input->BindAction(Actions[3], ETriggerEvent::Completed, this, &AZMBCharacter::SprintOff);
	Input->BindAction(Actions[4], ETriggerEvent::Started, this, &AZMBCharacter::FireOn);
	Input->BindAction(Actions[4], ETriggerEvent::Completed, this, &AZMBCharacter::FireOff);
	Input->BindAction(Actions[5], ETriggerEvent::Started, this, &AZMBCharacter::AimOn);
	Input->BindAction(Actions[5], ETriggerEvent::Completed, this, &AZMBCharacter::AimOff);
	Input->BindAction(Actions[6], ETriggerEvent::Started, this, &AZMBCharacter::ReloadKey);
	Input->BindAction(Actions[7], ETriggerEvent::Started, this, &AZMBCharacter::KickKey);
	Input->BindAction(Actions[8], ETriggerEvent::Started, this, &AZMBCharacter::GrenadeKey);
	Input->BindAction(Actions[9], ETriggerEvent::Started, this, &AZMBCharacter::BombKey);
	Input->BindAction(Actions[10], ETriggerEvent::Started, this, &AZMBCharacter::SlowMoKey);
	Input->BindAction(Actions[11], ETriggerEvent::Started, this, &AZMBCharacter::InteractKey);
	Input->BindAction(Actions[12], ETriggerEvent::Started, this, &AZMBCharacter::CameraKey);
	Input->BindAction(Actions[13], ETriggerEvent::Started, this, &AZMBCharacter::FirstPersonKey);
	Input->BindAction(Actions[14], ETriggerEvent::Started, this, &AZMBCharacter::ClipKey);
	Input->BindAction(Actions[15], ETriggerEvent::Started, this, &AZMBCharacter::NextWeaponKey);
	Input->BindAction(Actions[16], ETriggerEvent::Started, this, &AZMBCharacter::HordeKey);
	Input->BindAction(Actions[17], ETriggerEvent::Started, this, &AZMBCharacter::TimeKey);
	Input->BindAction(Actions[18], ETriggerEvent::Started, this, &AZMBCharacter::HelpKey);
	for (int32 i = 0; i < SlotActions.Num(); ++i)
	{
		Input->BindAction(SlotActions[i], ETriggerEvent::Started, this, &AZMBCharacter::WeaponKey, i + 1);
	}
}

void AZMBCharacter::UnPossessed()
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(Mapping);
		}
	}
	bFireHeld = false;
	bAiming = false;
	bSprintHeld = false;
	Super::UnPossessed();
}

void AZMBCharacter::Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
	if (bDead || bDriving)
	{
		return;
	}
	const FRotator Yaw(0.0f, GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), MoveInput.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), MoveInput.X);
}

void AZMBCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	// com as escalas antigas ligadas (DefaultInput.ini) o pitch vem invertido: mouse para cima = valor negativo aqui
	AddControllerPitchInput(-Axis.Y);
}

void AZMBCharacter::JumpOn(const FInputActionValue& Value)
{
	if (!bDead)
	{
		Jump();
	}
}

void AZMBCharacter::JumpOff(const FInputActionValue& Value)
{
	StopJumping();
}

void AZMBCharacter::SprintOn(const FInputActionValue& Value)
{
	bSprintHeld = true;
}

void AZMBCharacter::SprintOff(const FInputActionValue& Value)
{
	bSprintHeld = false;
}

void AZMBCharacter::FireOn(const FInputActionValue& Value)
{
	bFireHeld = true;
	bShotThisPress = false;
}

void AZMBCharacter::FireOff(const FInputActionValue& Value)
{
	bFireHeld = false;
}

void AZMBCharacter::AimOn(const FInputActionValue& Value)
{
	bAiming = true;
}

void AZMBCharacter::AimOff(const FInputActionValue& Value)
{
	bAiming = false;
}

void AZMBCharacter::ReloadKey(const FInputActionValue& Value)
{
	const ZR::FWeaponType* Weapon = CurrentWeaponType();
	if (bDead || !Weapon || ReloadLeft > 0.0f || !WeaponStates.IsValidIndex(CurrentWeapon))
	{
		return;
	}
	const FWeaponState& State = WeaponStates[CurrentWeapon];
	if (State.Magazine < Weapon->Magazine && State.Reserve > 0)
	{
		ReloadLeft = Weapon->ReloadSeconds;
	}
}

void AZMBCharacter::WeaponKey(const FInputActionValue& Value, int32 Slot)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || bDead)
	{
		return;
	}
	const std::vector<ZR::FWeaponType>& Weapons = Data->Weapons();
	for (size_t i = 0; i < Weapons.size(); ++i)
	{
		if (Weapons[i].Slot == Slot && static_cast<int32>(i) != CurrentWeapon)
		{
			CurrentWeapon = static_cast<int32>(i);
			ReloadLeft = 0.0f;
			FireCooldown = 0.25f;
			return;
		}
	}
}

void AZMBCharacter::NextWeaponKey(const FInputActionValue& Value)
{
	if (WeaponStates.Num() > 0 && !bDead)
	{
		CurrentWeapon = (CurrentWeapon + 1) % WeaponStates.Num();
		ReloadLeft = 0.0f;
		FireCooldown = 0.25f;
	}
}

void AZMBCharacter::KickKey(const FInputActionValue& Value)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || bDead || KickCooldown > 0.0f)
	{
		return;
	}
	KickCooldown = 0.6f;
	const ZR::FPlayerConfig& Player = Data->Tuning().Player;
	const FVector Forward = GetActorForwardVector();
	const FVector Center = GetActorLocation() + Forward * (Player.KickRangeCm * 0.6f);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_ZMBZombie);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZMBKick), false, this);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Player.KickRangeCm * 0.6f), Params);
	FZMBHitContext Context;
	Context.Instigator = GetController();
	Context.Cause = EZMBKillCause::Kick;
	Context.bSlowMo = bSlowMo;
	TSet<AActor*> Done;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (AZMBZombie* Zombie = Cast<AZMBZombie>(Actor))
		{
			if (Done.Contains(Zombie))
			{
				continue;
			}
			Done.Add(Zombie);
			Zombie->TakeLimbHit(nullptr, Player.KickDamage, Zombie->GetActorLocation(), Forward * Player.KickImpulse, Context);
			if (!Zombie->IsDead())
			{
				Zombie->LaunchCharacter(Forward * Player.KickImpulse + FVector(0.0f, 0.0f, 320.0f), true, true);
			}
		}
		else if (UPrimitiveComponent* Component = Overlap.GetComponent())
		{
			if (Component->IsSimulatingPhysics())
			{
				Component->AddImpulse(Forward * Player.KickImpulse + FVector(0.0f, 0.0f, 250.0f), NAME_None, true);
			}
		}
	}
	AddTrauma(0.15f);
}

void AZMBCharacter::Throw(const char* ExplosiveId)
{
	UZMBGameData* Data = UZMBGameData::Get(this);
	const ZR::FExplosiveType* Type = Data ? Data->FindExplosive(ExplosiveId) : nullptr;
	if (!Type)
	{
		return;
	}
	const FVector Aim = Camera->GetForwardVector();
	const FVector Hand = GetActorLocation() + GetActorForwardVector() * 70.0f + FVector(0.0f, 0.0f, 60.0f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AZMBExplosive* Thrown = GetWorld()->SpawnActor<AZMBExplosive>(AZMBExplosive::StaticClass(), Hand, Aim.Rotation(), Params);
	if (!Thrown)
	{
		return;
	}
	Thrown->Setup(*Type);
	Thrown->Throw(Aim * Data->Tuning().Player.ThrowSpeed + FVector(0.0f, 0.0f, 320.0f) + GetVelocity());
	if (Type->Id == "granada")
	{
		Thrown->Arm(GetController());
	}
}

void AZMBCharacter::GrenadeKey(const FInputActionValue& Value)
{
	if (!bDead && Grenades > 0)
	{
		--Grenades;
		Throw("granada");
	}
}

void AZMBCharacter::BombKey(const FInputActionValue& Value)
{
	if (!bDead && Bombs > 0)
	{
		--Bombs;
		Throw("bujao");
	}
}

void AZMBCharacter::SlowMoKey(const FInputActionValue& Value)
{
	if (bDead)
	{
		return;
	}
	SetSlowMo(!bSlowMo && Focus > 10.0f);
}

void AZMBCharacter::SetSlowMo(bool bOn)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	bSlowMo = bOn;
	UGameplayStatics::SetGlobalTimeDilation(this, bOn && Data ? Data->Tuning().Player.SlowMoTimeDilation : 1.0f);
	if (bOn)
	{
		if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
		{
			Stream->React("mira_lenta");
		}
	}
}

void AZMBCharacter::InteractKey(const FInputActionValue& Value)
{
	if (bDead || bDriving)
	{
		return;
	}
	AZMBCar* Nearest = nullptr;
	float Best = 450.0f;
	for (TActorIterator<AZMBCar> It(GetWorld()); It; ++It)
	{
		const float Distance = FVector::Dist(It->GetActorLocation(), GetActorLocation());
		if (Distance < Best)
		{
			Best = Distance;
			Nearest = *It;
		}
	}
	if (Nearest)
	{
		Nearest->Enter(this);
	}
}

void AZMBCharacter::SetDriving(AZMBCar* Car)
{
	bDriving = Car != nullptr;
	bFireHeld = false;
	bAiming = false;
	if (Car)
	{
		SetSlowMo(false);
		GetCharacterMovement()->DisableMovement();
		SetActorEnableCollision(false);
		SetActorHiddenInGame(true);
		AttachToActor(Car, FAttachmentTransformRules::KeepWorldTransform);
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AZMBCharacter::CameraKey(const FInputActionValue& Value)
{
	CameraMode = CameraMode == 3 ? LastThirdPersonMode : (CameraMode + 1) % 3;
	LastThirdPersonMode = CameraMode;
}

void AZMBCharacter::FirstPersonKey(const FInputActionValue& Value)
{
	CameraMode = CameraMode == 3 ? LastThirdPersonMode : 3;
}

void AZMBCharacter::ClipKey(const FInputActionValue& Value)
{
	if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
	{
		Stream->Clip();
	}
}

void AZMBCharacter::HordeKey(const FInputActionValue& Value)
{
	if (AZMBHordeDirector* Director = AZMBHordeDirector::Find(this))
	{
		Director->StartHorde(40.0f);
		if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
		{
			Stream->Announce(LOCTEXT("DebugHorde", "HORDA (TECLA N)"));
		}
	}
}

void AZMBCharacter::TimeKey(const FInputActionValue& Value)
{
	if (AZMBGameMode* GameMode = AZMBGameMode::Get(this))
	{
		if (AZMBWorldBuilder* Builder = GameMode->GetBuilder())
		{
			Builder->SkipHours(3.0f);
		}
	}
}

void AZMBCharacter::HelpKey(const FInputActionValue& Value)
{
	bShowHelp = !bShowHelp;
}

void AZMBCharacter::Resupply(int32 AmmoPacks, int32 InGrenades, int32 InBombs, float HealthBonus)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (Data)
	{
		const std::vector<ZR::FWeaponType>& Weapons = Data->Weapons();
		for (int32 i = 0; i < WeaponStates.Num() && i < static_cast<int32>(Weapons.size()); ++i)
		{
			WeaponStates[i].Reserve += Weapons[static_cast<size_t>(i)].Magazine * AmmoPacks;
		}
	}
	Grenades += InGrenades;
	Bombs += InBombs;
	Health = FMath::Min(GetMaxHealth(), Health + HealthBonus);
	if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
	{
		Stream->Announce(LOCTEXT("Resupply", "SUPRIMENTOS: MUNIÇÃO, GRANADAS E BUJÕES"));
	}
}

void AZMBCharacter::AddTrauma(float Amount)
{
	Trauma = FMath::Clamp(Trauma + Amount, 0.0f, 1.0f);
}

float AZMBCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bDead || bDriving || Applied <= 0.0f)
	{
		return 0.0f;
	}
	Health = FMath::Max(0.0f, Health - Applied);
	SinceDamage = 0.0f;
	AddTrauma(0.2f + Applied / 60.0f);
	if (AZMBHordeDirector* Director = AZMBHordeDirector::Find(this))
	{
		Director->ReportDamageTaken(Applied);
	}
	if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
	{
		Stream->ReportPlayerDamage(Applied, Health);
	}
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (Data && Health > 0.0f && Health <= Data->Tuning().Player.CloseCallHealth && !bCloseCallPending && !bCloseCallSpent)
	{
		bCloseCallPending = true;
		CloseCallTimer = 0.0f;
	}
	if (Health <= 0.0f)
	{
		Die();
	}
	return Applied;
}

void AZMBCharacter::Die()
{
	bDead = true;
	bCloseCallPending = false;
	SetSlowMo(false);
	RespawnLeft = 6.0f;
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_ZMBZombie, ECR_Ignore);
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Part->SetCollisionProfileName(TEXT("PhysicsActor"));
		Part->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Part->SetCollisionResponseToChannel(ECC_ZMBZombie, ECR_Ignore);
		Part->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Part->SetSimulatePhysics(true);
		Part->AddImpulse(FMath::VRand() * 150.0f + FVector(0.0f, 0.0f, 120.0f), NAME_None, true);
	}
	if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
	{
		Stream->OnPlayerDied();
	}
}

void AZMBCharacter::Respawn()
{
	AZMBGameMode* GameMode = AZMBGameMode::Get(this);
	AZMBWorldBuilder* Builder = GameMode ? GameMode->GetBuilder() : nullptr;
	if (Builder)
	{
		TeleportTo(Builder->GetStartLocation(), Builder->GetStartRotation());
	}
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		UStaticMeshComponent* Part = Parts[i];
		Part->SetSimulatePhysics(false);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		if (PartRest.IsValidIndex(i))
		{
			Part->SetRelativeTransform(PartRest[i]);
		}
	}
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_ZMBZombie, ECR_Block);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	bDead = false;
	bCloseCallSpent = false;
	Health = GetMaxHealth();
	if (const UZMBGameData* Data = UZMBGameData::Get(this))
	{
		Focus = Data->Tuning().Player.FocusMax;
		Grenades = FMath::Max(Grenades, Data->Tuning().Player.StartGrenades);
		Bombs = FMath::Max(Bombs, Data->Tuning().Player.StartBombs);
	}
	if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
	{
		Stream->OnPlayerRespawned();
	}
}

void AZMBCharacter::FireOnce()
{
	const ZR::FWeaponType* Weapon = CurrentWeaponType();
	if (!Weapon || !WeaponStates.IsValidIndex(CurrentWeapon))
	{
		return;
	}
	FWeaponState& State = WeaponStates[CurrentWeapon];
	if (State.Magazine <= 0)
	{
		ReloadKey(FInputActionValue());
		return;
	}
	--State.Magazine;
	FireCooldown = 1.0f / FMath::Max(Weapon->ShotsPerSecond, 0.1f);
	SinceShot = 0.0f;

	// o tiro sai do centro da tela (a mira), começando na altura do personagem para não acertar nada entre ele e a câmera
	const FVector ViewLocation = Camera->GetComponentLocation();
	const FVector ViewDirection = Camera->GetForwardVector();
	const float Along = FVector::DotProduct(GetActorLocation() - ViewLocation, ViewDirection);
	const FVector TraceStart = ViewLocation + ViewDirection * FMath::Max(Along, 0.0f);
	const FVector Muzzle = Parts.IsValidIndex(GunPart) ? Parts[GunPart]->GetComponentLocation() + GetActorForwardVector() * 20.0f : GetActorLocation();
	const float SpreadDeg = Weapon->SpreadDeg * (bAiming ? 0.5f : 1.0f) + SpreadKick;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZMBShot), false, this);
	for (int32 Pellet = 0; Pellet < Weapon->Pellets; ++Pellet)
	{
		const FVector Direction = FMath::VRandCone(ViewDirection, FMath::DegreesToRadians(SpreadDeg));
		const FVector End = TraceStart + Direction * Weapon->RangeM * 100.0f;
		FHitResult Hit;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, End, ECC_Visibility, Params);
		AZMBFx::Tracer(GetWorld(), Muzzle, bHit ? Hit.ImpactPoint : End);
		if (bHit)
		{
			HandleBulletHit(Hit, Direction, *Weapon);
		}
	}
	AZMBFx::MuzzleFlash(GetWorld(), Muzzle, GetActorRotation());
	SpreadKick = FMath::Min(SpreadKick + Weapon->SpreadDeg * 0.6f, 6.0f);
	AddControllerPitchInput(-0.12f * FMath::Sqrt(static_cast<float>(Weapon->Pellets)));
	AddTrauma(0.04f * Weapon->Pellets);
	if (AZMBHordeDirector* Director = AZMBHordeDirector::Find(this))
	{
		Director->ReportNoise(Weapon->Noise);
	}
}

void AZMBCharacter::HandleBulletHit(const FHitResult& Hit, const FVector& Direction, const ZR::FWeaponType& Weapon)
{
	AActor* Actor = Hit.GetActor();
	UPrimitiveComponent* Component = Hit.GetComponent();
	if (AZMBZombie* Zombie = Cast<AZMBZombie>(Actor))
	{
		FZMBHitContext Context;
		Context.Instigator = GetController();
		Context.Cause = EZMBKillCause::Bullet;
		Context.bSlowMo = bSlowMo;
		Context.DistanceCm = FVector::Dist(GetActorLocation(), Hit.ImpactPoint);
		const ZR::FHitOutcome Outcome = Zombie->TakeLimbHit(Component, Weapon.Damage, Hit.ImpactPoint, Direction * Weapon.Impulse, Context);
		HitMarkerAge = 0.0f;
		if (Outcome.bHeadshotKill)
		{
			if (const UZMBGameData* Data = UZMBGameData::Get(this))
			{
				Focus = FMath::Min(Data->Tuning().Player.FocusMax, Focus + Data->Tuning().Player.FocusPerHeadshot);
			}
		}
		return;
	}
	if (AZMBExplosive* Explosive = Cast<AZMBExplosive>(Actor))
	{
		Explosive->TakeExplosiveDamage(Weapon.Damage, GetController(), 0);
		HitMarkerAge = 0.0f;
	}
	if (Component && Component->IsSimulatingPhysics())
	{
		Component->AddImpulseAtLocation(Direction * Weapon.Impulse * Component->GetMass(), Hit.ImpactPoint);
	}
	AZMBFx::ImpactDust(GetWorld(), Hit.ImpactPoint, Hit.ImpactNormal);
}

void AZMBCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data)
	{
		return;
	}
	const ZR::FPlayerConfig& Player = Data->Tuning().Player;
	// relógios que andam em tempo REAL mesmo na mira lenta (tiro, recarga e foco, como no Dead Eye)
	const float Dilation = FMath::Max(UGameplayStatics::GetGlobalTimeDilation(this), 0.01f);
	const float RealDelta = DeltaSeconds / Dilation;

	UpdateCamera(RealDelta);
	if (bDriving)
	{
		return;
	}
	if (bDead)
	{
		RespawnLeft -= RealDelta;
		if (RespawnLeft <= 0.0f)
		{
			Respawn();
		}
		return;
	}

	FireCooldown -= RealDelta;
	KickCooldown -= RealDelta;
	SinceShot += RealDelta;
	SinceDamage += DeltaSeconds;
	HitMarkerAge += RealDelta;
	SpreadKick = FMath::Max(0.0f, SpreadKick - RealDelta * 6.0f);

	if (ReloadLeft > 0.0f && WeaponStates.IsValidIndex(CurrentWeapon))
	{
		ReloadLeft -= RealDelta;
		if (ReloadLeft <= 0.0f)
		{
			ReloadLeft = 0.0f;
			FWeaponState& State = WeaponStates[CurrentWeapon];
			const ZR::FWeaponType* Weapon = CurrentWeaponType();
			const int32 Wanted = Weapon ? Weapon->Magazine - State.Magazine : 0;
			const int32 Moved = FMath::Clamp(Wanted, 0, State.Reserve);
			State.Magazine += Moved;
			State.Reserve -= Moved;
		}
	}
	const ZR::FWeaponType* Weapon = CurrentWeaponType();
	if (bFireHeld && Weapon && FireCooldown <= 0.0f && ReloadLeft <= 0.0f && (Weapon->bAutomatic || !bShotThisPress))
	{
		bShotThisPress = true;
		FireOnce();
	}

	// mira lenta gasta foco em tempo real; fora dela o foco volta devagar
	if (bSlowMo)
	{
		Focus -= Player.FocusDrainPerSecond * RealDelta;
		if (Focus <= 0.0f)
		{
			Focus = 0.0f;
			SetSlowMo(false);
		}
	}
	else
	{
		Focus = FMath::Min(Player.FocusMax, Focus + Player.FocusRegenPerSecond * DeltaSeconds);
	}

	// correr gasta fôlego; mirando anda devagar
	const bool bMoving = !MoveInput.IsNearlyZero() && GetVelocity().Size2D() > 50.0f;
	const bool bSprinting = bSprintHeld && bMoving && !bAiming && Stamina > 0.0f;
	if (bSprinting)
	{
		Stamina = FMath::Max(0.0f, Stamina - Player.StaminaDrainPerSecond * DeltaSeconds);
	}
	else
	{
		Stamina = FMath::Min(Player.Stamina, Stamina + Player.StaminaRegenPerSecond * DeltaSeconds);
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = bSprinting ? Player.SprintSpeed : (bAiming ? Player.WalkSpeed * 0.6f : Player.WalkSpeed);
	// mirando, atirando ou em primeira pessoa o corpo olha para onde a câmera olha
	const bool bFaceAim = bAiming || SinceShot < 0.8f || CameraMode == 3;
	bUseControllerRotationYaw = bFaceAim;
	Move->bOrientRotationToMovement = !bFaceAim;

	// vida volta sozinha devagar, até um teto (o resto vem das caixas)
	if (SinceDamage > Player.RegenDelaySeconds && Health < Player.RegenCap)
	{
		Health = FMath::Min(Player.RegenCap, Health + Player.RegenPerSecond * DeltaSeconds);
	}
	if (bCloseCallPending)
	{
		CloseCallTimer += DeltaSeconds;
		if (CloseCallTimer > 2.5f)
		{
			bCloseCallPending = false;
			bCloseCallSpent = true;
			if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
			{
				Stream->ReportHype(ZR::EHypeEvent::CloseCall, 1, GetActorLocation());
			}
		}
	}
	if (bCloseCallSpent && Health > Player.CloseCallHealth * 2.0f)
	{
		bCloseCallSpent = false;
	}
	Animate(DeltaSeconds);
}

void AZMBCharacter::UpdateCamera(float RealDelta)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data)
	{
		return;
	}
	const ZR::FCameraConfig& Config = Data->Tuning().Camera;
	float Length = Config.FarDistanceCm;
	float Height = Config.FarHeightCm;
	float Side = Config.FarSideCm;
	float Pitch = Config.FarPitchDeg;
	float Fov = Config.Fov;
	float Eye = 0.0f;
	switch (CameraMode)
	{
	case 0: Length = Config.NearDistanceCm; Height = Config.NearHeightCm; Side = Config.NearSideCm; Pitch = Config.NearPitchDeg; break;
	case 1: Length = Config.MidDistanceCm; Height = Config.MidHeightCm; Side = Config.MidSideCm; Pitch = Config.MidPitchDeg; break;
	case 3: Length = 0.0f; Height = 0.0f; Side = 0.0f; Pitch = 0.0f; Fov = Config.FirstPersonFov; Eye = EyeHeight; break;
	default: break;
	}
	if (bAiming)
	{
		if (CameraMode == 3)
		{
			Fov = Config.AimFov;
		}
		else
		{
			Length = Config.AimDistanceCm;
			Height = Config.NearHeightCm;
			Side = Config.AimSideCm;
			Pitch = 0.0f;
			Fov = Config.AimFov;
		}
	}
	const float Speed = 9.0f;
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, Length, RealDelta, Speed);
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, FVector(0.0f, Side, Height), RealDelta, Speed);
	CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, FVector(0.0f, 0.0f, Eye), RealDelta, Speed);
	CameraBoom->bDoCollisionTest = CameraMode != 3;
	CameraBoom->CameraLagSpeed = CameraMode == 3 ? 40.0f : Config.LagSpeed;
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, Fov, RealDelta, Speed));

	// tremor: ruído suave, mais forte quanto maior o trauma (ao quadrado)
	Trauma = FMath::Max(0.0f, Trauma - 1.4f * RealDelta);
	ShakeTime += RealDelta;
	const float Shake = Trauma * Trauma;
	const FVector Offset(0.0f, FMath::PerlinNoise1D(ShakeTime * 23.0f) * 14.0f * Shake, FMath::PerlinNoise1D(ShakeTime * 29.0f + 7.0f) * 11.0f * Shake);
	const float Roll = FMath::PerlinNoise1D(ShakeTime * 17.0f + 3.0f) * 3.5f * Shake;
	Camera->SetRelativeLocation(Offset);
	const float CurrentPitch = Camera->GetRelativeRotation().Pitch;
	Camera->SetRelativeRotation(FRotator(FMath::FInterpTo(CurrentPitch, Pitch, RealDelta, Speed), 0.0f, Roll));

	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		// primeira pessoa: o corpo some da câmera mas a sombra fica
		Parts[i]->SetOwnerNoSee(CameraMode == 3 && i != GunPart);
	}
	UpdatePostProcess();
}

void AZMBCharacter::UpdatePostProcess()
{
	FPostProcessSettings& Post = Camera->PostProcessSettings;
	const bool bPhone = CameraMode == 3;
	// modo celular: lente barata (aberração e grão fortes, vinheta); mira lenta: o mundo perde cor
	Post.bOverride_SceneFringeIntensity = true;
	Post.SceneFringeIntensity = bPhone ? 1.6f : 0.2f;
	Post.bOverride_FilmGrainIntensity = true;
	Post.FilmGrainIntensity = bPhone ? 0.35f : 0.12f;
	Post.bOverride_VignetteIntensity = true;
	Post.VignetteIntensity = bSlowMo ? 0.9f : (bPhone ? 0.6f : 0.35f);
	Post.bOverride_ColorSaturation = true;
	Post.ColorSaturation = bSlowMo ? FVector4(0.45, 0.45, 0.5, 1.0) : FVector4(0.9, 0.92, 0.95, 1.0);
	Post.bOverride_MotionBlurAmount = true;
	Post.MotionBlurAmount = bPhone ? 0.8f : 0.4f;
	Camera->PostProcessBlendWeight = 1.0f;
}

void AZMBCharacter::Animate(float DeltaSeconds)
{
	if (PartRest.Num() != Parts.Num())
	{
		return;
	}
	const float Speed = GetVelocity().Size2D();
	WalkPhase += DeltaSeconds * Speed / 40.0f;
	const float Swing = FMath::Sin(WalkPhase) * FMath::Clamp(Speed / 300.0f, 0.0f, 1.0f) * 30.0f;
	// pernas: índices 4 e 5; braço esquerdo (2) balança; o direito segura a arma
	Parts[4]->SetRelativeRotation(PartRest[4].Rotator() + FRotator(Swing, 0.0f, 0.0f));
	Parts[5]->SetRelativeRotation(PartRest[5].Rotator() + FRotator(-Swing, 0.0f, 0.0f));
	Parts[2]->SetRelativeRotation(PartRest[2].Rotator() + FRotator(-Swing * 0.7f, 0.0f, 0.0f));
}

#undef LOCTEXT_NAMESPACE
