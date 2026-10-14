// Explosivo do mundo: bujão de gás, barril de combustível, granada arremessada e o tanque dos carros abandonados.
// Tipo e números em Data/Explosivos.csv. Atingido, perde vida; sem vida, chia com uma luz piscando durante o pavio e
// explode (ZMBExplosion::Detonate). Explosão de vizinho acende o pavio com a cadeia + 1: é daí que sai a reação em cadeia.
// O carro abandonado não some: vira carcaça queimada, ainda com física.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rules/ZRData.h"
#include "ZMBExplosive.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class AController;

UCLASS()
class ZOMBIE_API AZMBExplosive : public AActor
{
	GENERATED_BODY()

public:
	AZMBExplosive();

	/** Chame logo depois de nascer: escolhe a forma, a cor e a massa pelo tipo. */
	void Setup(const ZR::FExplosiveType& InType);

	/** Dano de tiro ou de explosão. ChainDepth é a profundidade que esta explosão terá se o dano acender o pavio. */
	void TakeExplosiveDamage(float Damage, AController* DamageInstigator, int32 ChainDepth);

	/** Acende o pavio agora (granada arremessada). */
	void Arm(AController* DamageInstigator);

	/** Joga com física (arremesso do jogador). */
	void Throw(const FVector& Velocity);

	bool IsWreck() const { return bWreck; }

	/** Carro (inteiro ou carcaça) que dá para desmanchar em sucata: sem o pavio aceso. */
	bool CanDismantle() const { return bIsCar && FuseLeft < 0.0f; }

	virtual void Tick(float DeltaSeconds) override;

private:
	void Ignite(int32 ChainDepth);
	void Explode();

	UPROPERTY(VisibleAnywhere, Category="Explosive")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category="Explosive")
	TObjectPtr<UStaticMeshComponent> Detail;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> FuseLight;

	ZR::FExplosiveType Type;
	float Health = 1.0f;
	float FuseLeft = -1.0f;
	float FuseTime = 0.0f;
	int32 PendingChainDepth = 0;
	TWeakObjectPtr<AController> LastInstigator;
	bool bExploded = false;
	bool bWreck = false;
	bool bIsCar = false;
};
