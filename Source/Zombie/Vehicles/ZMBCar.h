// Carro de protótipo para dirigir (GTA: chega perto e aperta E). Física própria e simples, sem malha esquelética nem
// Chaos Vehicles: uma caixa com física, suspensão por quatro raios (mola e amortecedor), tração no chão, aderência lateral
// (freio de mão solta a traseira) e esterço pelo modelo de bicicleta com menos ângulo em alta. Atropelar zumbi a mais de
// Car.RunOverMinKmh despedaça (ZR::ApplyBlast com o dano pela velocidade). Números em Data/Ajustes.csv (carro.*).
// O carro final usa o Chaos Vehicles com a experiência do Detonauto (docs/tecnico.md).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ZMBCar.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class USpotLightComponent;
class UInputAction;
class UInputMappingContext;
class AZMBCharacter;
struct FInputActionValue;

UCLASS()
class ZOMBIE_API AZMBCar : public APawn
{
	GENERATED_BODY()

public:
	AZMBCar();

	/** O jogador entra (o controlador passa para o carro). */
	void Enter(AZMBCharacter* Driver);
	/** O jogador sai pela porta do motorista. */
	void ExitCar();

	float GetSpeedKmh() const;

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void UnPossessed() override;

protected:
	virtual void BeginPlay() override;

private:
	void CreateInput();
	void Throttle(const FInputActionValue& Value);
	void Steer(const FInputActionValue& Value);
	void HandbrakeOn(const FInputActionValue& Value);
	void HandbrakeOff(const FInputActionValue& Value);
	void ExitKey(const FInputActionValue& Value);
	void ResetKey(const FInputActionValue& Value);

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category="Car")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(VisibleAnywhere, Category="Car")
	TObjectPtr<UStaticMeshComponent> Shell;

	UPROPERTY(VisibleAnywhere, Category="Car")
	TObjectPtr<UStaticMeshComponent> Cabin;

	UPROPERTY(VisibleAnywhere, Category="Car")
	TArray<TObjectPtr<UStaticMeshComponent>> Wheels;

	UPROPERTY(VisibleAnywhere, Category="Car")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category="Car")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category="Car")
	TArray<TObjectPtr<USpotLightComponent>> Headlights;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ThrottleAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SteerAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> HandbrakeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ExitAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ResetAction;

	TWeakObjectPtr<AZMBCharacter> Driver;
	TArray<FVector> WheelMounts;
	float ThrottleInput = 0.0f;
	float SteerInput = 0.0f;
	bool bHandbrake = false;
	float WheelSpin = 0.0f;
};
