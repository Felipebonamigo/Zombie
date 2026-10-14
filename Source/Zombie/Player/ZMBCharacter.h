// O jogador: um streamer com o celular preso no peito, transmitindo o apocalipse. Terceira pessoa por padrão, de longe e
// do alto (pedido do Felipe), com mais duas distâncias (C) e a primeira pessoa no "modo celular" (V), com lente, grão e
// a moldura do app da live no HUD (o visual de "isso é real?" do Unrecord e do Bodycam).
//
// Controles (teclado e controle; lista completa em docs/controles.md):
//   WASD andar, mouse olhar, Shift correr, Espaço pular, botão esquerdo atirar, direito mirar, R recarregar, 1 2 3 armas,
//   F chute, G granada, H arremessa bujão (atire nele no ar!), Q mira lenta (o Dead Eye do Red Dead), E entra/sai do carro,
//   C distância da câmera, V primeira pessoa, B clipar o melhor lance, F1 ajuda. Teste: N chama a horda, T avança 3 horas.
//
// Tudo que é regra (dano no membro, hype, Diretor) mora nas regras puras; aqui é entrada, câmera, mira e efeitos.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Rules/ZRData.h"
#include "ZMBCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class AZMBCar;
struct FInputActionValue;

UCLASS()
class ZOMBIE_API AZMBCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZMBCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void UnPossessed() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Tremor de câmera (0 a 1 somando; explosões perto). */
	void AddTrauma(float Amount);

	/** Entrou (Car) ou saiu (nullptr) de um carro: some do mundo enquanto dirige. */
	void SetDriving(AZMBCar* Car);

	/** Caixa de suprimentos: pentes extras por arma, granadas, bujões e vida. */
	void Resupply(int32 AmmoPacks, int32 InGrenades, int32 InBombs, float HealthBonus);

	// --- leitura para o HUD ---
	bool IsDead() const { return bDead; }
	float GetHealth() const { return Health; }
	float GetMaxHealth() const;
	float GetStamina01() const;
	float GetFocus01() const;
	bool IsSlowMo() const { return bSlowMo; }
	bool IsAiming() const { return bAiming; }
	bool IsFirstPerson() const { return CameraMode == 3; }
	int32 GetCameraMode() const { return CameraMode; }
	FString GetWeaponName() const;
	int32 GetMagazine() const;
	int32 GetReserve() const;
	bool IsReloading() const { return ReloadLeft > 0.0f; }
	int32 GetGrenades() const { return Grenades; }
	int32 GetBombs() const { return Bombs; }
	/** Segundos desde o último tiro que acertou zumbi (marcador de acerto na mira). */
	float GetHitMarkerAge() const { return HitMarkerAge; }
	/** Abertura da mira, 0 a 1 (dispersão atual da arma). */
	float GetSpread01() const;
	float GetRespawnSeconds() const { return RespawnLeft; }
	bool IsDriving() const { return bDriving; }
	bool IsHelpVisible() const { return bShowHelp; }

protected:
	virtual void BeginPlay() override;

private:
	void CreateInput();
	UInputAction* NewAction(const TCHAR* Name, bool bAxis2D, bool bAxis1D = false);

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void JumpOn(const FInputActionValue& Value);
	void JumpOff(const FInputActionValue& Value);
	void SprintOn(const FInputActionValue& Value);
	void SprintOff(const FInputActionValue& Value);
	void FireOn(const FInputActionValue& Value);
	void FireOff(const FInputActionValue& Value);
	void AimOn(const FInputActionValue& Value);
	void AimOff(const FInputActionValue& Value);
	void ReloadKey(const FInputActionValue& Value);
	void KickKey(const FInputActionValue& Value);
	void GrenadeKey(const FInputActionValue& Value);
	void BombKey(const FInputActionValue& Value);
	void SlowMoKey(const FInputActionValue& Value);
	void InteractKey(const FInputActionValue& Value);
	void CameraKey(const FInputActionValue& Value);
	void FirstPersonKey(const FInputActionValue& Value);
	void ClipKey(const FInputActionValue& Value);
	void WeaponKey(const FInputActionValue& Value, int32 Slot);
	void NextWeaponKey(const FInputActionValue& Value);
	void HordeKey(const FInputActionValue& Value);
	void TimeKey(const FInputActionValue& Value);
	void HelpKey(const FInputActionValue& Value);

	void FireOnce();
	void HandleBulletHit(const FHitResult& Hit, const FVector& Direction, const ZR::FWeaponType& Weapon);
	void Throw(const char* ExplosiveId);
	void SetSlowMo(bool bOn);
	void UpdateCamera(float RealDelta);
	void UpdatePostProcess();
	void Animate(float DeltaSeconds);
	void Die();
	void Respawn();
	const ZR::FWeaponType* CurrentWeaponType() const;

	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** Cabeça, tronco, braço esquerdo, braço direito, perna esquerda, perna direita, arma, celular. */
	UPROPERTY(VisibleAnywhere, Category="Body")
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> Actions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> SlotActions;

	TArray<FTransform> PartRest;

	struct FWeaponState
	{
		int32 Magazine = 0;
		int32 Reserve = 0;
	};
	TArray<FWeaponState> WeaponStates;
	int32 CurrentWeapon = 0;

	float Health = 100.0f;
	float Stamina = 100.0f;
	float Focus = 100.0f;
	int32 Grenades = 0;
	int32 Bombs = 0;
	int32 CameraMode = 2;
	int32 LastThirdPersonMode = 2;
	bool bAiming = false;
	bool bFireHeld = false;
	bool bShotThisPress = false;
	bool bSprintHeld = false;
	bool bSlowMo = false;
	bool bDead = false;
	bool bDriving = false;
	bool bShowHelp = true;
	bool bCloseCallPending = false;
	bool bCloseCallSpent = false;
	FVector2D MoveInput = FVector2D::ZeroVector;
	float FireCooldown = 0.0f;
	float ReloadLeft = 0.0f;
	float KickCooldown = 0.0f;
	float SinceShot = 10.0f;
	float SinceDamage = 100.0f;
	float CloseCallTimer = 0.0f;
	float HitMarkerAge = 10.0f;
	float Trauma = 0.0f;
	float ShakeTime = 0.0f;
	float WalkPhase = 0.0f;
	float RespawnLeft = 0.0f;
	float SpreadKick = 0.0f;
};
