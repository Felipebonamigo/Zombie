#include "Audio/ZMBAudioSubsystem.h"
#include "Zombie.h"
#include "Game/ZMBGameData.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundWave.h"

UZMBAudioSubsystem* UZMBAudioSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UZMBAudioSubsystem>() : nullptr;
}

void UZMBAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Picker = std::make_unique<ZR::FSoundPicker>(FPlatformTime::Cycles64());
	// a mira lenta baixa o tom no dispositivo de áudio, que sobrevive ao mundo: um PIE parado com ela ligada não pode
	// começar o próximo grave
	UGameplayStatics::SetGlobalPitchModulation(&InWorld, 1.0f, 0.0f);
	// carrega tudo agora: carregar no meio do tiroteio daria um engasgo no primeiro tiro de cada arma
	if (const UZMBGameData* Data = UZMBGameData::Get(this))
	{
		int32 Count = 0;
		for (const ZR::FSoundDef& Def : Data->Sounds())
		{
			for (const std::string& File : Def.Files)
			{
				Count += LoadSound(File) ? 1 : 0;
			}
		}
		UE_LOG(LogZombie, Display, TEXT("Som: %d arquivos carregados de /Game/Zombie/Sons, %d faltando"), Count, Missing.Num());
		SeenReload = Data->GetReloadCount();
	}
}

void UZMBAudioSubsystem::SyncWithData()
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || Data->GetReloadCount() == SeenReload)
	{
		return;
	}
	SeenReload = Data->GetReloadCount();
	Attenuations.Empty();
	Concurrencies.Empty();
	Voices.Empty();
	UnknownIds.Empty();
	Picker = std::make_unique<ZR::FSoundPicker>(FPlatformTime::Cycles64());
}

bool UZMBAudioSubsystem::ListenerLocation(UWorld* World, FVector& Out)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(World, 0);
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn || !Pawn->GetRootComponent())
	{
		// sem pawn (destruído, trocando): solta, senão o motor mediria do componente nulo, a origem do mapa
		if (Controller && bListenerOverridden)
		{
			Controller->ClearAudioListenerAttenuationOverride();
		}
		bListenerOverridden = false;
		ListenerPawn = nullptr;
		const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(World, 0);
		if (!Camera)
		{
			return false;
		}
		Out = Camera->GetCameraLocation();
		return true;
	}
	// a atenuação do motor de áudio também mede daqui; a direção (esquerda e direita) segue a câmera
	if (ListenerPawn.Get() != Pawn)
	{
		Controller->SetAudioListenerAttenuationOverride(Pawn->GetRootComponent(), FVector::ZeroVector);
		ListenerPawn = Pawn;
		bListenerOverridden = true;
	}
	Out = Pawn->GetActorLocation();
	return true;
}

void UZMBAudioSubsystem::RefreshListener()
{
	if (UWorld* World = GetWorld())
	{
		FVector Listener = FVector::ZeroVector;
		ListenerLocation(World, Listener);
	}
}

void UZMBAudioSubsystem::Play(const UObject* WorldContext, const char* Id, const FVector& Where, float VolumeScale)
{
	if (UZMBAudioSubsystem* Audio = Get(WorldContext))
	{
		Audio->PlayAt(Id, Where, false, VolumeScale);
	}
}

void UZMBAudioSubsystem::Play2D(const UObject* WorldContext, const char* Id, float VolumeScale)
{
	if (UZMBAudioSubsystem* Audio = Get(WorldContext))
	{
		Audio->PlayAt(Id, FVector::ZeroVector, true, VolumeScale);
	}
}

int32 UZMBAudioSubsystem::FindSound(const char* Id) const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || !Id)
	{
		return INDEX_NONE;
	}
	const std::vector<ZR::FSoundDef>& Sounds = Data->Sounds();
	for (size_t i = 0; i < Sounds.size(); ++i)
	{
		if (Sounds[i].Id == Id)
		{
			return static_cast<int32>(i);
		}
	}
	return INDEX_NONE;
}

USoundBase* UZMBAudioSubsystem::LoadSound(const std::string& File)
{
	const FString Name = FString(TEXT("SW_")) + UZMBGameData::ToFString(File);
	if (const TObjectPtr<USoundBase>* Found = Loaded.Find(Name))
	{
		return Found->Get();
	}
	if (Missing.Contains(Name))
	{
		return nullptr;
	}
	const FString Path = FString::Printf(TEXT("/Game/Zombie/Sons/%s.%s"), *Name, *Name);
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Sound)
	{
		Missing.Add(Name);
		UE_LOG(LogZombie, Warning, TEXT("Som %s não existe no projeto: rode Tools/Unreal/importar_sons.ps1 (fica mudo)"), *Path);
		return nullptr;
	}
	// o cache de streaming do UE5 só traz o áudio no primeiro toque: adianta o primeiro pedaço (primeiro tiro sem atraso)
	UGameplayStatics::PrimeSound(Sound);
	Loaded.Add(Name, Sound);
	return Sound;
}

USoundAttenuation* UZMBAudioSubsystem::AttenuationFor(int32 Index, float RangeCm)
{
	if (const TObjectPtr<USoundAttenuation>* Found = Attenuations.Find(Index))
	{
		return Found->Get();
	}
	USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(this);
	FSoundAttenuationSettings& Settings = Attenuation->Attenuation;
	Settings.bAttenuate = true;
	Settings.bSpatialize = true;
	// volume cheio até 5% do alcance e queda natural até o alcance (o tiro de fuzil ainda se ouve a 150 m, o gemido a 30)
	Settings.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	Settings.AttenuationShape = EAttenuationShape::Sphere;
	Settings.AttenuationShapeExtents = FVector(RangeCm * 0.05f, 0.0f, 0.0f);
	Settings.FalloffDistance = RangeCm * 0.95f;
	// -30 dB no fim do alcance (o padrão de -60 deixa a meia distância em -30 dB, quase mudo); perto do que a física dá
	// para um som que vai de 5% a 100% do alcance (20 vezes mais longe: -26 dB)
	Settings.dBAttenuationAtMax = -30.0f;
	Attenuations.Add(Index, Attenuation);
	return Attenuation;
}

USoundConcurrency* UZMBAudioSubsystem::ConcurrencyFor(int32 Index, int32 MaxVoices)
{
	if (const TObjectPtr<USoundConcurrency>* Found = Concurrencies.Find(Index))
	{
		return Found->Get();
	}
	// a rajada passa de vozes_max: o toque novo entra e o mais antigo (já na cauda) sai
	USoundConcurrency* Concurrency = NewObject<USoundConcurrency>(this);
	Concurrency->Concurrency.MaxCount = FMath::Max(MaxVoices, 1);
	Concurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopOldest;
	Concurrencies.Add(Index, Concurrency);
	return Concurrency;
}

int32 UZMBAudioSubsystem::CountVoices(int32 Index)
{
	TArray<TWeakObjectPtr<UAudioComponent>>& List = Voices.FindOrAdd(Index);
	List.RemoveAll([](const TWeakObjectPtr<UAudioComponent>& Voice) { return !Voice.IsValid() || !Voice->IsPlaying(); });
	return List.Num();
}

UAudioComponent* UZMBAudioSubsystem::PlayAttached(const UObject* WorldContext, const char* Id, USceneComponent* Parent, float VolumeScale)
{
	UZMBAudioSubsystem* Audio = Get(WorldContext);
	return Audio && Parent ? Audio->PlayAt(Id, Parent->GetComponentLocation(), false, VolumeScale, Parent) : nullptr;
}

UAudioComponent* UZMBAudioSubsystem::PlayAt(const char* Id, const FVector& Where, bool b2D, float VolumeScale, USceneComponent* AttachTo)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	UWorld* World = GetWorld();
	if (!Data || !World || !Picker || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}
	SyncWithData();
	const int32 Index = FindSound(Id);
	if (Index == INDEX_NONE)
	{
		const FString Name = UTF8_TO_TCHAR(Id ? Id : "");
		if (!UnknownIds.Contains(Name))
		{
			UnknownIds.Add(Name);
			UE_LOG(LogZombie, Warning, TEXT("Som '%s' não está em Data/Sons.csv"), *Name);
		}
		return nullptr;
	}
	const ZR::FSoundDef& Def = Data->Sounds()[static_cast<size_t>(Index)];
	if (Def.bLoop)
	{
		return nullptr;
	}
	const bool bFlat = (b2D || Def.b2D) && !AttachTo;
	float Distance = 0.0f;
	FVector Listener = FVector::ZeroVector;
	if (!bFlat && ListenerLocation(World, Listener))
	{
		Distance = static_cast<float>(FVector::Dist(Listener, Where));
	}
	const ZR::FSoundPick Pick = Picker->Pick(Data->Sounds(), Index, CountVoices(Index), Distance, World->GetTimeSeconds());
	if (!Pick.bPlay)
	{
		return nullptr;
	}
	USoundBase* Sound = LoadSound(Def.Files[static_cast<size_t>(Pick.File)]);
	if (!Sound)
	{
		return nullptr;
	}
	const float Volume = Pick.Volume * VolumeScale;
	USoundConcurrency* Concurrency = Def.bStealOldest ? ConcurrencyFor(Index, Def.MaxVoices) : nullptr;
	UAudioComponent* Voice = nullptr;
	if (bFlat)
	{
		Voice = UGameplayStatics::SpawnSound2D(World, Sound, Volume, Pick.Pitch, 0.0f, Concurrency);
	}
	else if (AttachTo)
	{
		Voice = UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset,
			true, Volume, Pick.Pitch, 0.0f, AttenuationFor(Index, Def.RangeCm), Concurrency, true);
	}
	else
	{
		Voice = UGameplayStatics::SpawnSoundAtLocation(World, Sound, Where, FRotator::ZeroRotator, Volume, Pick.Pitch, 0.0f,
			AttenuationFor(Index, Def.RangeCm), Concurrency);
	}
	if (Voice)
	{
		Voices.FindOrAdd(Index).Add(Voice);
	}
	return Voice;
}

UAudioComponent* UZMBAudioSubsystem::PlayLoop(const char* Id, USceneComponent* Parent)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	SyncWithData();
	const int32 Index = FindSound(Id);
	if (!Data || !Parent || Index == INDEX_NONE)
	{
		return nullptr;
	}
	// o motor também mede a distância do carro: prende o ouvinte nele já ao entrar
	if (UWorld* World = GetWorld())
	{
		FVector Listener = FVector::ZeroVector;
		ListenerLocation(World, Listener);
	}
	const ZR::FSoundDef& Def = Data->Sounds()[static_cast<size_t>(Index)];
	if (!Def.bLoop)
	{
		// marcar bLooping num som solto deixaria em laço todos os toques dele (o asset é um só)
		UE_LOG(LogZombie, Warning, TEXT("PlayLoop('%s'): o som não tem laco = 1 em Data/Sons.csv"), UTF8_TO_TCHAR(Id));
		return nullptr;
	}
	USoundBase* Sound = Def.Files.empty() ? nullptr : LoadSound(Def.Files[0]);
	if (!Sound)
	{
		return nullptr;
	}
	// o importador já marca o laço (Sons.csv, coluna laco); garante aqui também, para um asset importado à mão
	if (USoundWave* Wave = Cast<USoundWave>(Sound))
	{
		Wave->bLooping = true;
	}
	return UGameplayStatics::SpawnSoundAttached(Sound, Parent, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true,
		Def.Volume, 1.0f, 0.0f, AttenuationFor(Index, Def.RangeCm), nullptr, false);
}
