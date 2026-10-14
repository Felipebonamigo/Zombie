// Diretor da horda no mundo: aplica o ritmo das regras (ZR::TickDirector) e decide ONDE e QUAL zumbi nasce: numa rua a
// SpawnMin..SpawnMax do jogador, de preferência fora da vista da câmera, com o tipo sorteado pelo peso do dia ou da noite.
// Mantém o campo de fluxo da cidade (ZR::FFlowField) apontando para o jogador, some com zumbi que ficou longe demais,
// começa a Noite da Horda e atende os pedidos do chat (zumbi com nome, horda, esquadrão do bujão, entrega por drone).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rules/ZRData.h"
#include "Rules/ZRDirector.h"
#include "Rules/ZRFlowField.h"
#include "ZMBHordeDirector.generated.h"

class AZMBWorldBuilder;
class AZMBZombie;

UCLASS()
class ZOMBIE_API AZMBHordeDirector : public AActor
{
	GENERATED_BODY()

public:
	AZMBHordeDirector();

	static AZMBHordeDirector* Find(const UObject* WorldContext);

	void Setup(AZMBWorldBuilder* InBuilder);

	virtual void Tick(float DeltaSeconds) override;

	/** Direção (no plano) para o zumbi seguir a rua até o jogador. Falso fora das ruas ou sem caminho. */
	bool FlowDirection(const FVector& From, FVector& OutDirection) const;

	void ReportDamageTaken(float Amount);
	void ReportExplosion(const FVector& Where, float Noise);
	void ReportNoise(float Noise);
	void ReportZombieDied(AZMBZombie* Zombie, bool bPlayerKill);

	/** Horda já (chat, Noite da Horda ou a tecla de teste N). */
	void StartHorde(float Seconds);
	/** Chat pediu !bujao: um grupo de zumbis-bujão nasce junto, perto o bastante para virar cadeia. */
	void SpawnBombSquad(int32 Count);
	/** Chat pediu !zumbi: nasce um zumbi com o nome do espectador. */
	bool SpawnNamedZombie(const FString& Viewer);
	/** Entrega por drone: uma caixa de suprimentos cai perto do jogador. */
	void DropSupplyCrate();

	int32 AliveCount() const;
	int32 NamedAliveCount() const;
	bool IsNight() const { return bNight; }
	bool IsHordeActive() const { return State.HordeSecondsLeft > 0.0f; }
	ZR::EDirectorPhase GetPhase() const { return State.Phase; }
	float GetIntensity() const { return State.Intensity; }

private:
	AZMBZombie* SpawnZombie(const ZR::FZombieType& Type, const FVector& Location, const FString& Viewer, bool bHordeSpawn);
	bool PickSpawnLocation(FVector& OutLocation, float MinCm, float MaxCm, bool bPreferHidden);
	const ZR::FZombieType* PickType(bool bNightNow) const;
	void UpdateFlow();
	void ConsumeChatActions();
	void Cleanup();

	UPROPERTY(Transient)
	TObjectPtr<AZMBWorldBuilder> Builder;

	TArray<TWeakObjectPtr<AZMBZombie>> Alive;
	ZR::FDirectorState State;
	ZR::FFlowField Flow;
	int32 FlowTargetX = -1;
	int32 FlowTargetY = -1;
	float FlowAge = 0.0f;

	float PendingDamage = 0.0f;
	int32 PendingKillsNear = 0;
	int32 PendingExplosions = 0;
	float PendingNoise = 0.0f;

	bool bNight = false;
	int32 NightsSeen = 0;
};
