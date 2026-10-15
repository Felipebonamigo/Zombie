// Zumbi de protótipo: um ACharacter sem controlador (anda por bRunPhysicsWithNoController) e com o corpo montado de
// formas básicas, uma por membro. Cada membro é um componente que o tiro acerta; o corpo segue as regras puras de
// Rules/ZRBody.h (membro sai, cabeça estoura, perna arrancada vira rastejante, explosão forte despedaça). Morto, os
// pedaços caem com física e somem depois de Tuning.CorpseSeconds. Contrato: docs/regras-de-jogo.md, seção 4.
//
// Comportamento (à Left 4 Dead / Days Gone): sempre vai atrás do jogador; longe anda pelo campo de fluxo da cidade
// (AZMBHordeDirector::FlowDirection), perto vai direto; com barricada perto do jogador, segue o campo fino que contorna a
// base (direto quando nada atrapalha a reta); corre quando está perto, à noite ou na horda. Travado numa barricada
// (Base/ZMBBuildPiece.h), para e bate nela até quebrar (à Night of the Dead).
// Arte final: malha esquelética com ossos que se soltam (docs/plano-visual.md); este corpo de blocos é só o protótipo.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/ZMBTypes.h"
#include "Rules/ZRData.h"
#include "ZMBZombie.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UPrimitiveComponent;
class AZMBBuildPiece;

UCLASS()
class ZOMBIE_API AZMBZombie : public ACharacter
{
	GENERATED_BODY()

public:
	AZMBZombie();

	/** Antes de FinishSpawning (ou logo depois de nascer): tipo, nome de espectador (vazio = nenhum) e se veio na horda. */
	void InitZombie(const ZR::FZombieType& InType, const FString& InViewerName, bool bInHorde);

	virtual void Tick(float DeltaSeconds) override;

	/** Tiro ou chute num componente do corpo. Componente que não é membro conta como tronco. */
	ZR::FHitOutcome TakeLimbHit(UPrimitiveComponent* HitComponent, float Damage, const FVector& HitLocation, const FVector& Impulse,
		const FZMBHitContext& Context);

	/** Explosão: Damage já com a queda pela distância; ImpulseStrength empurra os pedaços para longe de Origin. */
	ZR::FHitOutcome TakeBlast(float Damage, const FVector& Origin, float ImpulseStrength, const FZMBHitContext& Context);

	/** Atropelado. */
	ZR::FHitOutcome TakeVehicleHit(float Damage, const FVector& Velocity, const FZMBHitContext& Context);

	/** Armadilha: bLegs acerta uma perna que ainda está presa (sempre a mesma até sair); senão, o tronco. */
	ZR::FHitOutcome TakeTrapHit(float Damage, bool bLegs, const FZMBHitContext& Context);

	/** Armadilha segurando: a velocidade cai para esta fração por um instante (duas armadilhas: vale a menor). */
	void ApplyTrapSlow(float Factor);

	bool IsDead() const { return Body.bDead; }
	bool IsNamed() const { return !ViewerName.IsEmpty(); }
	const FString& GetViewerName() const { return ViewerName; }
	const ZR::FZombieType& GetZombieType() const { return Type; }

protected:
	virtual void BeginPlay() override;

private:
	int32 LimbIndexOf(const UPrimitiveComponent* Component) const;
	void ApplyOutcome(const ZR::FHitOutcome& Outcome, const FVector& Origin, const FVector& Impulse, bool bFromBlast,
		const FZMBHitContext& Context);
	void SeverLimb(int32 Limb, const FVector& Impulse, bool bPop);
	void Die(const ZR::FHitOutcome& Outcome, const FZMBHitContext& Context);
	void ArmTank(const FZMBHitContext& Context);
	void ExplodeSelf();
	void BecomeCrawler();
	void UpdateLabel();
	void Animate(float DeltaSeconds, float Speed);
	void Scream();
	/** Barricada logo à frente na direção Heading (varredura curta pelos objetos WorldDynamic). */
	AZMBBuildPiece* FindBlockingBarricade(const FVector& Heading) const;

	/** Cabeça, tronco, braço esquerdo, braço direito, perna esquerda, perna direita (ordem de ZR::ELimb). */
	UPROPERTY(VisibleAnywhere, Category="Zombie")
	TArray<TObjectPtr<UStaticMeshComponent>> Limbs;

	/** Bujão nas costas (só no tipo "bujao"): tiro nele acende o pavio. */
	UPROPERTY(VisibleAnywhere, Category="Zombie")
	TObjectPtr<UStaticMeshComponent> Tank;

	/** Enfeite de Carnaval do folião (cocar) e capacete do brutamontes. */
	UPROPERTY(VisibleAnywhere, Category="Zombie")
	TObjectPtr<UStaticMeshComponent> Hat;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> Label;

	ZR::FZombieType Type;
	ZR::FBodyState Body;
	FString ViewerName;
	bool bHorde = false;
	bool bCrawler = false;
	bool bTankArmed = false;
	bool bExplodePending = false;
	/** Já explodiu (não agenda outra explosão quando Die roda dentro da própria explosão). */
	bool bSelfExploded = false;
	float ExplodeTimer = 0.0f;
	float AttackCooldown = 0.0f;
	float LungeTimer = 0.0f;
	float WalkPhase = 0.0f;
	float WobblePhase = 0.0f;
	/** Gritador: já gritou (grita uma vez) e quanto falta do grito (parado, braços para cima). */
	bool bScreamed = false;
	float ScreamLeft = 0.0f;
	FZMBHitContext LastHit;
	/** Barricada travando o caminho (vista quatro vezes por segundo) e se ele já está batendo nela. */
	TWeakObjectPtr<AZMBBuildPiece> BlockingPiece;
	float BlockCheckTimer = 0.0f;
	bool bBashing = false;
	float TrapSlowFactor = 1.0f;
	float TrapSlowLeft = 0.0f;
	TArray<FTransform> RestTransforms;
};
