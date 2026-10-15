// Lugar de saque na cidade (Data/Saque.csv): lixeira, mochila de folião, isopor de ambulante, viatura largada. O jogador
// segura E perto dele (AZMBCharacter) e leva o que as regras sortearem (Rules/ZRLoot.h). Revirado, escurece e fica
// vazio; com renova_dias, volta a render depois. Visual de protótipo: uma caixa da cor e do tamanho da tabela.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rules/ZRLoot.h"
#include "ZMBLootSpot.generated.h"

class UStaticMeshComponent;

UCLASS()
class ZOMBIE_API AZMBLootSpot : public AActor
{
	GENERATED_BODY()

public:
	AZMBLootSpot();

	virtual void Tick(float DeltaSeconds) override;

	/** O ator fica no chão, de frente (+X) para onde a planta mandou; a caixa sobe metade da altura. */
	void Setup(const ZR::FLootSpotType& InType);

	bool CanSearch(int32 Day) const;

	/** Revira: sorteia o que rende, marca o dia e escurece. */
	ZR::FLootResult Search(int32 Day);

	/** Distância no chão do ponto até a borda do lugar [cm] (0 em cima dele). */
	float DistanceTo(const FVector& Where) const;

	const ZR::FLootSpotType& GetType() const { return Type; }
	/** O nome na língua do jogo ("lixeira", "trash bin"). */
	FString GetDisplayName() const;

private:
	void Paint(bool bEmpty);
	int32 CurrentDay() const;

	UPROPERTY(VisibleAnywhere, Category="Loot")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category="Loot")
	TObjectPtr<UStaticMeshComponent> Body;

	ZR::FLootSpotType Type;
	ZR::FLootSpotState State;
};
