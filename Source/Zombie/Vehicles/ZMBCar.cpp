#include "Vehicles/ZMBCar.h"
#include "Zombie.h"
#include "Combat/ZMBTypes.h"
#include "Game/ZMBGameData.h"
#include "Player/ZMBCharacter.h"
#include "Stream/ZMBStreamSubsystem.h"
#include "Zombies/ZMBZombie.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float WheelRadiusCm = 35.0f;
	constexpr float WheelBaseCm = 290.0f;
	constexpr float KmhPerCmS = 0.036f;
}

AZMBCar::AZMBCar()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// a caixa sem escala é o corpo físico; o visual pendura nela (assim as medidas dos filhos ficam em cm de verdade)
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->InitBoxExtent(FVector(220.0f, 92.0f, 38.0f));
	Box->SetCollisionProfileName(TEXT("PhysicsActor"));
	Box->SetCollisionObjectType(ECC_Vehicle);
	Box->SetCollisionResponseToChannel(ECC_ZMBZombie, ECR_Overlap);
	Box->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Box->SetGenerateOverlapEvents(true);
	Box->SetSimulatePhysics(true);
	Box->SetLinearDamping(0.05f);
	Box->SetAngularDamping(0.8f);
	SetRootComponent(Box);

	Shell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shell"));
	Shell->SetupAttachment(Box);
	Shell->SetStaticMesh(CubeMesh.Object);
	Shell->SetRelativeScale3D(FVector(4.4f, 1.84f, 0.72f));
	Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Cabin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabin"));
	Cabin->SetupAttachment(Box);
	Cabin->SetStaticMesh(CubeMesh.Object);
	Cabin->SetRelativeLocation(FVector(-30.0f, 0.0f, 66.0f));
	Cabin->SetRelativeScale3D(FVector(2.3f, 1.7f, 0.6f));
	Cabin->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WheelMounts = {FVector(140.0f, -82.0f, -10.0f), FVector(140.0f, 82.0f, -10.0f), FVector(-150.0f, -82.0f, -10.0f), FVector(-150.0f, 82.0f, -10.0f)};
	for (int32 i = 0; i < WheelMounts.Num(); ++i)
	{
		UStaticMeshComponent* Wheel = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Wheel%d"), i));
		Wheel->SetupAttachment(Box);
		Wheel->SetStaticMesh(CylinderMesh.Object);
		Wheel->SetRelativeLocation(WheelMounts[i] - FVector(0.0f, 0.0f, 40.0f));
		Wheel->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
		Wheel->SetRelativeScale3D(FVector(WheelRadiusCm * 2.0f / 100.0f, WheelRadiusCm * 2.0f / 100.0f, 0.24f));
		Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wheels.Add(Wheel);
	}

	for (int32 Side = 0; Side < 2; ++Side)
	{
		USpotLightComponent* Light = CreateDefaultSubobject<USpotLightComponent>(Side == 0 ? TEXT("HeadlightL") : TEXT("HeadlightR"));
		Light->SetupAttachment(Box);
		Light->SetRelativeLocation(FVector(225.0f, Side == 0 ? -65.0f : 65.0f, 5.0f));
		Light->SetRelativeRotation(FRotator(-4.0f, 0.0f, 0.0f));
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(4000.0f);
		Light->SetAttenuationRadius(4500.0f);
		Light->SetInnerConeAngle(16.0f);
		Light->SetOuterConeAngle(34.0f);
		Light->SetLightColor(FLinearColor(1.0f, 0.93f, 0.82f));
		Light->SetCastShadows(false);
		Headlights.Add(Light);
	}

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(Box);
	CameraArm->SetRelativeRotation(FRotator(-14.0f, 0.0f, 0.0f));
	CameraArm->TargetArmLength = 800.0f;
	CameraArm->SocketOffset = FVector(0.0f, 0.0f, 160.0f);
	CameraArm->bUsePawnControlRotation = false;
	CameraArm->bInheritPitch = false;
	CameraArm->bInheritRoll = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 8.0f;
	CameraArm->bEnableCameraRotationLag = true;
	CameraArm->CameraRotationLagSpeed = 6.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	Camera->SetFieldOfView(85.0f);
}

void AZMBCar::BeginPlay()
{
	Super::BeginPlay();
	if (UZMBGameData* Data = UZMBGameData::Get(this))
	{
		Box->SetMassOverrideInKg(NAME_None, Data->Tuning().Car.MassKg, true);
		const FLinearColor Paint = FLinearColor(0.85f, 0.45f, 0.02f);
		Shell->SetMaterial(0, Data->ColorMaterial(Paint, 0.25f, 0.3f));
		Cabin->SetMaterial(0, Data->ColorMaterial(FLinearColor(0.015f, 0.02f, 0.025f), 0.05f));
		for (UStaticMeshComponent* Wheel : Wheels)
		{
			Wheel->SetMaterial(0, Data->ColorMaterial(FLinearColor(0.012f, 0.012f, 0.012f), 0.9f));
		}
	}
	Box->SetCenterOfMass(FVector(0.0f, 0.0f, -35.0f));
	Box->OnComponentBeginOverlap.AddDynamic(this, &AZMBCar::HandleOverlap);
}

float AZMBCar::GetSpeedKmh() const
{
	return FVector::DotProduct(Box->GetPhysicsLinearVelocity(), GetActorForwardVector()) * KmhPerCmS;
}

void AZMBCar::Enter(AZMBCharacter* InDriver)
{
	AController* DriverController = InDriver ? InDriver->GetController() : nullptr;
	if (!DriverController || Driver.IsValid())
	{
		return;
	}
	Driver = InDriver;
	InDriver->SetDriving(this);
	DriverController->Possess(this);
}

void AZMBCar::ExitCar()
{
	AController* DriverController = GetController();
	AZMBCharacter* Character = Driver.Get();
	if (!DriverController || !Character)
	{
		return;
	}
	Driver.Reset();
	ThrottleInput = 0.0f;
	SteerInput = 0.0f;
	bHandbrake = false;
	const FVector Door = GetActorLocation() - GetActorRightVector() * 230.0f + FVector(0.0f, 0.0f, 80.0f);
	Character->SetDriving(nullptr);
	Character->TeleportTo(Door, FRotator(0.0f, GetActorRotation().Yaw, 0.0f));
	DriverController->Possess(Character);
}

void AZMBCar::CreateInput()
{
	if (Mapping)
	{
		return;
	}
	ThrottleAction = NewObject<UInputAction>(this, TEXT("ZMB_CarThrottle"));
	ThrottleAction->ValueType = EInputActionValueType::Axis1D;
	SteerAction = NewObject<UInputAction>(this, TEXT("ZMB_CarSteer"));
	SteerAction->ValueType = EInputActionValueType::Axis1D;
	HandbrakeAction = NewObject<UInputAction>(this, TEXT("ZMB_CarHandbrake"));
	HandbrakeAction->ValueType = EInputActionValueType::Boolean;
	ExitAction = NewObject<UInputAction>(this, TEXT("ZMB_CarExit"));
	ExitAction->ValueType = EInputActionValueType::Boolean;
	ResetAction = NewObject<UInputAction>(this, TEXT("ZMB_CarReset"));
	ResetAction->ValueType = EInputActionValueType::Boolean;

	Mapping = NewObject<UInputMappingContext>(this, TEXT("ZMB_CarMapping"));
	Mapping->MapKey(ThrottleAction, EKeys::W);
	Mapping->MapKey(ThrottleAction, EKeys::Gamepad_RightTriggerAxis);
	FEnhancedActionKeyMapping& Reverse = Mapping->MapKey(ThrottleAction, EKeys::S);
	Reverse.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
	FEnhancedActionKeyMapping& ReversePad = Mapping->MapKey(ThrottleAction, EKeys::Gamepad_LeftTriggerAxis);
	ReversePad.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
	Mapping->MapKey(SteerAction, EKeys::D);
	Mapping->MapKey(SteerAction, EKeys::Gamepad_LeftX);
	FEnhancedActionKeyMapping& Left = Mapping->MapKey(SteerAction, EKeys::A);
	Left.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
	Mapping->MapKey(HandbrakeAction, EKeys::SpaceBar);
	Mapping->MapKey(HandbrakeAction, EKeys::Gamepad_FaceButton_Bottom);
	Mapping->MapKey(ExitAction, EKeys::E);
	Mapping->MapKey(ExitAction, EKeys::Gamepad_FaceButton_Top);
	Mapping->MapKey(ResetAction, EKeys::R);
	Mapping->MapKey(ResetAction, EKeys::Gamepad_FaceButton_Left);
}

void AZMBCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateInput();
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(Mapping, 1);
		}
	}
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AZMBCar::Throttle);
		Input->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AZMBCar::Throttle);
		Input->BindAction(SteerAction, ETriggerEvent::Triggered, this, &AZMBCar::Steer);
		Input->BindAction(SteerAction, ETriggerEvent::Completed, this, &AZMBCar::Steer);
		Input->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &AZMBCar::HandbrakeOn);
		Input->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AZMBCar::HandbrakeOff);
		Input->BindAction(ExitAction, ETriggerEvent::Started, this, &AZMBCar::ExitKey);
		Input->BindAction(ResetAction, ETriggerEvent::Started, this, &AZMBCar::ResetKey);
	}
}

void AZMBCar::UnPossessed()
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(Mapping);
		}
	}
	ThrottleInput = 0.0f;
	SteerInput = 0.0f;
	bHandbrake = false;
	Super::UnPossessed();
}

void AZMBCar::Throttle(const FInputActionValue& Value)
{
	ThrottleInput = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);
}

void AZMBCar::Steer(const FInputActionValue& Value)
{
	SteerInput = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);
}

void AZMBCar::HandbrakeOn(const FInputActionValue& Value)
{
	bHandbrake = true;
}

void AZMBCar::HandbrakeOff(const FInputActionValue& Value)
{
	bHandbrake = false;
}

void AZMBCar::ExitKey(const FInputActionValue& Value)
{
	ExitCar();
}

void AZMBCar::ResetKey(const FInputActionValue& Value)
{
	// capotado ou preso: desvira no lugar
	const FVector Where = GetActorLocation() + FVector(0.0f, 0.0f, 150.0f);
	SetActorLocationAndRotation(Where, FRotator(0.0f, GetActorRotation().Yaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	Box->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Box->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

void AZMBCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || DeltaSeconds <= 0.0f)
	{
		return;
	}
	const ZR::FCarConfig& Car = Data->Tuning().Car;
	const FTransform Frame = Box->GetComponentTransform();
	const FVector Up = Frame.GetUnitAxis(EAxis::Z);
	const FVector Forward = Frame.GetUnitAxis(EAxis::X);
	const FVector Right = Frame.GetUnitAxis(EAxis::Y);
	const float Mass = Box->GetMass();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZMBCarWheel), false, this);
	const FCollisionObjectQueryParams Ground(ECC_WorldStatic);
	const float RayLength = Car.SuspensionRestCm + WheelRadiusCm;
	int32 Grounded = 0;
	for (int32 i = 0; i < WheelMounts.Num(); ++i)
	{
		const FVector Mount = Frame.TransformPosition(WheelMounts[i]);
		FHitResult Hit;
		float Reach = RayLength;
		if (GetWorld()->LineTraceSingleByObjectType(Hit, Mount, Mount - Up * RayLength, Ground, Params))
		{
			++Grounded;
			Reach = Hit.Distance;
			const float Compression = RayLength - Hit.Distance;
			const float UpSpeed = FVector::DotProduct(Box->GetPhysicsLinearVelocityAtPoint(Mount), Up);
			const float Force = FMath::Max(0.0f, Compression * Car.SpringPerCm - UpSpeed * Car.DamperPerCmS);
			Box->AddForceAtLocation(Up * Force, Mount);
		}
		if (Wheels.IsValidIndex(i))
		{
			const bool bFront = i < 2;
			const float SteerYaw = bFront ? SteerInput * Car.SteerDeg : 0.0f;
			Wheels[i]->SetRelativeLocation(WheelMounts[i] - FVector(0.0f, 0.0f, Reach - WheelRadiusCm));
			Wheels[i]->SetRelativeRotation(FRotator(-WheelSpin, SteerYaw, 90.0f));
		}
	}

	const FVector Velocity = Box->GetPhysicsLinearVelocity();
	const float ForwardSpeed = FVector::DotProduct(Velocity, Forward);
	const float LateralSpeed = FVector::DotProduct(Velocity, Right);
	WheelSpin = FMath::Fmod(WheelSpin + FMath::RadiansToDegrees(ForwardSpeed / WheelRadiusCm) * DeltaSeconds, 360.0f);
	if (Grounded < 2)
	{
		return;
	}

	const float MaxSpeed = Car.MaxSpeedKmh / KmhPerCmS;
	float Drive = 0.0f;
	if (ThrottleInput > 0.0f)
	{
		Drive = ForwardSpeed < MaxSpeed ? ThrottleInput * Car.EngineForce : 0.0f;
	}
	else if (ThrottleInput < 0.0f)
	{
		// andando para a frente o S freia; parado, dá ré (mais lenta)
		Drive = ForwardSpeed > 60.0f ? ThrottleInput * Car.BrakeForce : (ForwardSpeed > -MaxSpeed * 0.35f ? ThrottleInput * Car.EngineForce * 0.6f : 0.0f);
	}
	else
	{
		Drive = -ForwardSpeed * Mass * 0.35f;
	}
	if (bHandbrake)
	{
		Drive -= FMath::Sign(ForwardSpeed) * FMath::Min(FMath::Abs(ForwardSpeed) * Mass * 2.0f, Car.BrakeForce * 0.5f);
	}
	Box->AddForce(Forward * Drive);
	const float Grip = Car.SideGrip * (bHandbrake ? 0.22f : 1.0f);
	Box->AddForce(-Right * LateralSpeed * Grip * Mass);

	// esterço: modelo de bicicleta (giro = v / entre-eixos x tan(ângulo)), com menos ângulo em alta
	const float SpeedFactor = FMath::Clamp(FMath::Abs(ForwardSpeed) / MaxSpeed, 0.0f, 1.0f);
	const float SteerAngle = FMath::DegreesToRadians(Car.SteerDeg * SteerInput * FMath::Lerp(1.0f, 0.35f, SpeedFactor));
	const float TargetYawRate = FMath::RadiansToDegrees(ForwardSpeed / WheelBaseCm * FMath::Tan(SteerAngle)) * (bHandbrake ? 1.4f : 1.0f);
	FVector AngularVelocity = Box->GetPhysicsAngularVelocityInDegrees();
	const float CurrentYawRate = FVector::DotProduct(AngularVelocity, Up);
	const float NewYawRate = FMath::FInterpTo(CurrentYawRate, TargetYawRate, DeltaSeconds, 8.0f);
	AngularVelocity += Up * (NewYawRate - CurrentYawRate);
	Box->SetPhysicsAngularVelocityInDegrees(AngularVelocity);
}

void AZMBCar::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AZMBZombie* Zombie = Cast<AZMBZombie>(OtherActor);
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Zombie || Zombie->IsDead() || !Data)
	{
		return;
	}
	const ZR::FCarConfig& Car = Data->Tuning().Car;
	const FVector Velocity = Box->GetPhysicsLinearVelocity();
	const float SpeedKmh = Velocity.Size() * KmhPerCmS;
	if (SpeedKmh < Car.RunOverMinKmh)
	{
		return;
	}
	FZMBHitContext Context;
	Context.Instigator = GetController();
	Context.Cause = EZMBKillCause::Vehicle;
	Zombie->TakeVehicleHit(SpeedKmh * Car.RunOverDamagePerKmh, Velocity, Context);
	// cada atropelo rouba um pouco de velocidade
	Box->SetPhysicsLinearVelocity(Velocity * 0.96f);
}
