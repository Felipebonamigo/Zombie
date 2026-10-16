// Monta Porto Brasa (protótipo) em código a partir da planta das regras (ZR::GenerateCity): chão, ruas com faixa, calçadas,
// prédios com janelas (algumas acesas à noite), caixas d'água, praças com árvores, postes, praia e mar, os bujões, barris
// e carros largados na rua e os carros que dá para dirigir. Também o céu: sol e lua que giram com a hora (ZR::HourAt),
// atmosfera, luz do céu capturada em tempo real, neblina volumétrica e o pós-processo "cinema do fim do mundo".
//
// Tudo em formas básicas da engine e componentes instanciados (um desenho por cor): roda num mapa vazio, sem .umap, e
// serve para jogar e sentir a escala. A cidade final vem de assets (City Sample, PCG) num mapa com World Partition
// (docs/plano-visual.md).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rules/ZRCityPlan.h"
#include "Rules/ZRDayNight.h"
#include "ZMBWorldBuilder.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;

UCLASS()
class ZOMBIE_API AZMBWorldBuilder : public AActor
{
	GENERATED_BODY()

public:
	AZMBWorldBuilder();

	/** Gera a planta e monta tudo. Chamado pelo AZMBGameMode logo depois de nascer (antes de o jogador entrar). */
	void Build(std::uint64_t Seed);

	virtual void Tick(float DeltaSeconds) override;

	const ZR::FCityPlan& GetPlan() const { return Plan; }
	FVector CellCenter(int32 X, int32 Y) const;
	bool WorldToCell(const FVector& Location, int32& OutX, int32& OutY) const;
	float GroundHeight() const { return 0.0f; }
	/** Canto da célula (0, 0) no mundo e o lado de uma célula [cm]: o plano da cidade é o mundo menos a origem. */
	FVector GetOrigin() const { return Origin; }
	float GetCellSize() const { return CellSize; }
	FVector GetStartLocation() const { return StartLocation; }
	FRotator GetStartRotation() const { return StartRotation; }

	float GetHour() const;
	bool IsNightNow() const;
	int32 GetDayIndex() const;
	/** Avança o relógio (tecla de teste T, opção -ZMBHour). */
	void SkipHours(float Hours);
	void SetHour(float Hour);
	/** Para o relógio (cena de aparência: a mesma luz em todas as capturas). */
	void SetFrozen(bool bInFrozen) { bFrozen = bInFrozen; }

private:
	UInstancedStaticMeshComponent* NewInstances(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material, bool bCollision);
	void BuildGround();
	void BuildBlocks();
	void BuildStreetFurniture();
	void SpawnProps(std::uint64_t Seed);
	/** Lixeiras, mochilas, isopores e viaturas para revirar (Rules/ZRLoot.h). */
	void SpawnLoot(std::uint64_t Seed);
	void UpdateSky();

	UPROPERTY(VisibleAnywhere, Category="Sky")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category="Sky")
	TObjectPtr<UDirectionalLightComponent> Moon;

	UPROPERTY(VisibleAnywhere, Category="Sky")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category="Sky")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category="Sky")
	TObjectPtr<UExponentialHeightFogComponent> Fog;

	UPROPERTY(VisibleAnywhere, Category="Sky")
	TObjectPtr<UPostProcessComponent> PostProcess;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> StreetLights;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LitWindows;

	ZR::FCityPlan Plan;
	ZR::FDayConfig Day;
	float CellSize = 2000.0f;
	FVector Origin = FVector::ZeroVector;
	FVector StartLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	std::uint64_t CitySeed = 1;
	double Clock = 0.0;
	bool bLightsOn = false;
	bool bFrozen = false;
};
