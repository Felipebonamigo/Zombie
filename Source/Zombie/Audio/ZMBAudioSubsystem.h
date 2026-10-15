// Som do jogo no Unreal: toca os sons de Data/Sons.csv pelas regras de Rules/ZRSound.h (variação sem repetir, tom, vozes,
// espera e alcance). Os arquivos de Audio/*.wav entram no projeto como /Game/Zombie/Sons/SW_<nome> pelo script
// Tools/Unreal/importar_sons.ps1 (só no PC, editor fechado); som que não foi importado fica mudo, com um aviso no log por
// arquivo. A atenuação de cada som sai do alcance da tabela, criada em código (sem asset), e o som que rouba voz
// (roubar = 1) ganha uma concorrência "corta o mais antigo", também em código.
// A distância conta do personagem (ou do carro), não da câmera: a câmera fica a ~10 m dele e os sons do próprio jogador
// (recarga, chute, mordida) sumiriam. A direção continua vindo da câmera.
//
// Uso: UZMBAudioSubsystem::Play(this, "explosao", Onde), Play2D(this, "doacao"), PlayAttached(this, "chiado", Componente)
// e PlayLoop("carro_motor", Componente).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Rules/ZRSound.h"
#include <memory>
#include <string>
#include "ZMBAudioSubsystem.generated.h"

class UAudioComponent;
class USceneComponent;
class APawn;
class USoundAttenuation;
class USoundBase;
class USoundConcurrency;

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

	/** Som solto preso a Parent (anda com ele e para se ele sumir): o chiado do pavio. Quem chama pode guardar e dar Stop(). */
	static UAudioComponent* PlayAttached(const UObject* WorldContext, const char* Id, USceneComponent* Parent, float VolumeScale = 1.0f);

	/**
	 * Som em laço (só os com laco = 1) preso a Parent: o motor do carro. Quem chama guarda o componente, ajusta o tom e o
	 * volume (multiplicando o VolumeMultiplier de saída, que é o da tabela) e o para com bAutoDestroy e FadeOut.
	 */
	UAudioComponent* PlayLoop(const char* Id, USceneComponent* Parent);

private:
	UAudioComponent* PlayAt(const char* Id, const FVector& Where, bool b2D, float VolumeScale, USceneComponent* AttachTo = nullptr);
	int32 FindSound(const char* Id) const;
	USoundBase* LoadSound(const std::string& File);
	USoundAttenuation* AttenuationFor(int32 Index, float RangeCm);
	USoundConcurrency* ConcurrencyFor(int32 Index, int32 MaxVoices);
	int32 CountVoices(int32 Index);
	/** Onde o jogador "ouve" a distância: o pawn do jogador 0 (personagem ou carro); prende a atenuação do ouvinte nele. */
	bool ListenerLocation(UWorld* World, FVector& Out);
	/** Recarga das tabelas (zmb.dados.recarregar) muda as linhas: os caches por linha recomeçam. */
	void SyncWithData();

	/** Arquivos já carregados, pelo nome do asset (SW_<nome>). */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<USoundBase>> Loaded;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<USoundAttenuation>> Attenuations;

	/** Concorrência dos sons que roubam voz (roubar = 1), por linha da tabela. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<USoundConcurrency>> Concurrencies;

	/** Toques de cada som ainda tocando (para o limite de vozes). */
	TMap<int32, TArray<TWeakObjectPtr<UAudioComponent>>> Voices;
	/** Assets que não existem (avisa uma vez só). */
	TSet<FString> Missing;
	/** Ids pedidos pelo código que a tabela não tem (avisa uma vez só). */
	TSet<FString> UnknownIds;
	std::unique_ptr<ZR::FSoundPicker> Picker;
	/** O pawn em que a atenuação do ouvinte está presa (troca ao entrar e sair do carro). */
	TWeakObjectPtr<APawn> ListenerPawn;
	/** GetReloadCount das tabelas quando os caches foram feitos. */
	int32 SeenReload = -1;
};
