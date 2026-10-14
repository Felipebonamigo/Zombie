// Caixa de suprimentos da "entrega por drone": cai do céu com um sinalizador aceso (doação dos espectadores ou voto do
// chat em !caixa). Encostou, pegou: munição, granadas, bujões e um pouco de vida.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZMBPickup.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UPointLightComponent;

UCLASS()
class ZOMBIE_API AZMBPickup : public AActor
{
	GENERATED_BODY()

public:
	AZMBPickup();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category="Pickup")
	TObjectPtr<UStaticMeshComponent> Crate;

	UPROPERTY(VisibleAnywhere, Category="Pickup")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category="Pickup")
	TObjectPtr<UPointLightComponent> Flare;

	float Age = 0.0f;
	bool bTaken = false;
};
