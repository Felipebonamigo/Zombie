// Efeitos visuais passageiros feitos só com formas básicas e luz: bola de fogo, clarão, fumaça, rastro de tiro, faísca
// de cano, pedaços (entulho e sangue) e manchas no chão. Provisório até os sistemas de Niagara do plano visual
// (docs/plano-visual.md): servem para sentir o jogo, não para julgar a aparência.
//
// Orçamento: pedaços com física contam em LivePhysicsPieces(); acima de Tuning.MaxGibs novos pedaços não nascem.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZMBFx.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;

UCLASS()
class ZOMBIE_API AZMBFx : public AActor
{
	GENERATED_BODY()

public:
	AZMBFx();

	virtual void Tick(float DeltaSeconds) override;

	static void Explosion(UWorld* World, const FVector& Origin, float RadiusCm);
	static void MuzzleFlash(UWorld* World, const FVector& Location, const FRotator& Direction);
	static void Tracer(UWorld* World, const FVector& Start, const FVector& End);
	static void Blood(UWorld* World, const FVector& Location, const FVector& Direction, int32 Chunks);
	static void Splat(UWorld* World, const FVector& Location, float RadiusCm, const FLinearColor& Color);
	static void Debris(UWorld* World, const FVector& Origin, int32 Count, const FLinearColor& Color, float Speed);
	static void ImpactDust(UWorld* World, const FVector& Location, const FVector& Normal);

	static int32 LivePhysicsPieces() { return PhysicsPieces; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	static AZMBFx* SpawnFx(UWorld* World, const FVector& Location, const FRotator& Rotation, float Lifetime);
	static bool CanSpawnPiece(UWorld* World);

	void SetShape(int32 ShapeIndex, UMaterialInterface* Material, const FVector& InStartScale, const FVector& InEndScale);
	void AddFlash(const FLinearColor& Color, float Intensity, float Radius);
	void MakePhysicsPiece(const FVector& Velocity);

	UPROPERTY(VisibleAnywhere, Category="Fx")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> Light;

	float Age = 0.0f;
	float Lifetime = 1.0f;
	FVector StartScale = FVector::OneVector;
	FVector EndScale = FVector::OneVector;
	/** Fração da vida em que a escala chega ao fim; depois disso encolhe até sumir (bola de fogo e fumaça). */
	float GrowFraction = 1.0f;
	bool bShrinkAtEnd = false;
	float LightStart = 0.0f;
	FVector Drift = FVector::ZeroVector;
	bool bPhysicsPiece = false;

	static int32 PhysicsPieces;
};
