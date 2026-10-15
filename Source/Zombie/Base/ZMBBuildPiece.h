// Peça da base (à Night of the Dead): barricada ou armadilha que o jogador constrói com sucata no modo construção (X).
// Tipo e números em Data/Construcoes.csv; as regras (dano, conserto, desmonte, efeito da armadilha) moram em Rules/ZRBase.h.
//
//  - Barricada: caixa sólida que segura zumbi e jogador (a baixa dá para pular; zumbi não pula). O zumbi travado na frente
//    dela bate até quebrar (AZMBZombie); explosão também machuca. Para tiro ela é cobertura.
//  - Armadilha: o jogador e a bala passam; o zumbi que pisa leva dano e anda devagar (estacas nas pernas, arame no corpo).
//    O bujão armado acende um pavio curto quando um zumbi pisa, quando leva tiro ou na explosão vizinha (cadeia).
// Arte final: malhas de verdade (acervo-3d, skill modelo-3d); aqui é forma básica, como o resto do protótipo.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/ZMBGameData.h"
#include "Rules/ZRBase.h"
#include "ZMBBuildPiece.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AController;

UCLASS()
class ZOMBIE_API AZMBBuildPiece : public AActor
{
	GENERATED_BODY()

public:
	AZMBBuildPiece();

	/** Chame logo depois de nascer. O ator já nasce girado (Rot90 x 90 graus) e com o centro na metade da altura. */
	void Setup(const ZR::FBuildPieceType& InType, int32 InRot90);

	virtual void Tick(float DeltaSeconds) override;

	/** Golpe de zumbi vindo de From; o multiplicador da base (base.dano_zumbi_multiplicador) entra aqui. */
	void TakeZombieHit(float Damage, const FVector& From);
	/**
	 * Explosão com o dano já pela distância; o multiplicador da base entra aqui. ChainDepth é a profundidade que a explosão
	 * desta peça terá se ela for um bujão armado (como em AZMBExplosive::TakeExplosiveDamage).
	 */
	void TakeBlast(float Damage, AController* DamageInstigator, int32 ChainDepth);
	/** Tiro do jogador: só o bujão armado reage (acende). */
	void TakeBullet(AController* DamageInstigator);
	/** Carro batendo em ImpactPoint (dano já com o multiplicador da base). Verdadeiro se arrombou. */
	bool TakeVehicleHit(float Damage, const FVector& ImpactPoint);

	/** Conserta o que a sucata pagar (ZR::RepairPiece). Devolve a sucata gasta. */
	int32 Repair(int32 Scrap);
	/** Some do mundo e devolve a sucata (ZR::DismantleRefund). */
	int32 Dismantle();

	const ZR::FBuildPieceType& GetPieceType() const { return Type; }
	bool IsBarricade() const { return Type.Kind == ZR::EPieceKind::Barricade; }
	/** Quebrou, explodiu ou foi desmontada (some no fim do quadro). */
	bool IsGone() const { return bGone; }
	/** Estado de 0 a 1: o pior entre a vida e o desgaste (ZR::PieceCondition). */
	float GetHealth01() const;
	/** Sucata para consertar tudo agora (0 se inteira). */
	int32 FullRepairCost() const;
	/** Sucata que desmontar agora devolve. */
	int32 RefundNow() const;
	/** Pegada no plano da cidade: a posição menos a origem do mundo (AZMBWorldBuilder::GetOrigin). */
	ZR::FFootprint Footprint(const FVector& WorldOrigin) const;

private:
	struct FPartLook
	{
		FLinearColor Color;
		float Roughness = 0.8f;
		float Metallic = 0.0f;
		FRotator Rest;
	};

	UStaticMeshComponent* AddPart(EZMBShape Shape, const FVector& LocationCm, const FRotator& Rotation, const FVector& SizeCm,
		const FLinearColor& Color, float Roughness, float Metallic);
	void BuildLook();
	void UpdateLook();
	void TickTrap(float Dt);
	void Break();
	void Arm(AController* DamageInstigator, int32 ChainDepth);
	void Explode();

	/** Raiz: a colisão. Barricada bloqueia; armadilha só sente zumbi (sobreposição). */
	UPROPERTY(VisibleAnywhere, Category="Base")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	TArray<FPartLook> Looks;
	ZR::FBuildPieceType Type;
	ZR::FPieceState State;
	TWeakObjectPtr<AController> PendingInstigator;
	int32 PendingChainDepth = 0;
	int32 Rot90 = 0;
	int32 LastStage = 0;
	float TrapClock = 0.0f;
	/** Pavio do bujão armado; negativo = apagado. */
	float FuseLeft = -1.0f;
	bool bGone = false;
};
