// Som do jogo no Unreal: toca os sons de Data/Sons.csv pelas regras de Rules/ZRSound.h (variação sem repetir, tom, vozes,
// espera e alcance). Os arquivos de Audio/*.wav entram no projeto como /Game/Zombie/Sons/SW_<nome> pelo script
// Tools/Unreal/importar_sons.ps1 (só no PC, editor fechado); som que não foi importado fica mudo, com um aviso no log por
// arquivo. A atenuação de cada som sai do alcance da tabela, criada em código (sem asset).
//
// Uso: UZMBAudioSubsystem::Play(this, "explosao", Onde), Play2D(this, "doacao") e PlayLoop("carro_motor", Componente).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Rules/ZRSound.h"
#include <memory>
#include <string>
#include "ZMBAudioSubsystem.generated.h"

class UAudioComponent;
class USceneComponent;
class USoundAttenuation;
class USoundBase;

UCLASS()
class ZOMBIE_API UZMBAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	static UZMBAudioSubsystem* Get(const UObject* WorldContext);

	/** Toca o som Id em Where; VolumeScale multiplica o volume da tabela. Sem mundo, sem som ou sem arquivo: nada. */
	static void Play(const UObject* WorldContext, const char* Id, const FVector& Where, float VolumeScale = 1.0f);

	/** Som de interface, sem posição (doação, pauta, clipe). */
	static void Play2D(const UObject* WorldContext, const char* Id, float VolumeScale = 1.0f);

	/** Som em laço preso a Parent (motor do carro). Quem chama guarda o componente, ajusta o tom e o para (FadeOut). */
	UAudioComponent* PlayLoop(const char* Id, USceneComponent* Parent);

private:
	void PlayAt(const char* Id, const FVector& Where, bool b2D, float VolumeScale);
	int32 FindSound(const char* Id) const;
	USoundBase* LoadSound(const std::string& File);
	USoundAttenuation* AttenuationFor(int32 Index, float RangeCm);
	int32 CountVoices(int32 Index);

	/** Arquivos já carregados, pelo nome do asset (SW_<nome>). */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<USoundBase>> Loaded;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<USoundAttenuation>> Attenuations;

	/** Toques de cada som ainda tocando (para o limite de vozes). */
	TMap<int32, TArray<TWeakObjectPtr<UAudioComponent>>> Voices;
	/** Assets que não existem (avisa uma vez só). */
	TSet<FString> Missing;
	/** Ids pedidos pelo código que a tabela não tem (avisa uma vez só). */
	TSet<FString> UnknownIds;
	std::unique_ptr<ZR::FSoundPicker> Picker;
};
