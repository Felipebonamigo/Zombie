#include "Combat/ZMBExplosion.h"
#include "Zombie.h"
#include "Combat/ZMBExplosive.h"
#include "Combat/ZMBFx.h"
#include "Player/ZMBCharacter.h"
#include "Stream/ZMBStreamSubsystem.h"
#include "Zombies/ZMBHordeDirector.h"
#include "Zombies/ZMBZombie.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace ZMBExplosion
{
	FResult Detonate(UWorld* World, const FVector& Origin, const ZR::FBlastConfig& Blast, AController* Instigator, AActor* Source,
		int32 ChainDepth, float Noise)
	{
		FResult Result;
		if (!World)
		{
			return Result;
		}

		FCollisionObjectQueryParams Objects;
		Objects.AddObjectTypesToQuery(ECC_Pawn);
		Objects.AddObjectTypesToQuery(ECC_ZMBZombie);
		Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
		Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
		Objects.AddObjectTypesToQuery(ECC_Vehicle);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ZMBExplosion), false, Source);
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Blast.OuterRadiusCm), Params);

		FZMBHitContext Context;
		Context.Instigator = Instigator;
		Context.Cause = EZMBKillCause::Blast;
		Context.ChainDepth = ChainDepth;

		TSet<AActor*> DoneActors;
		TSet<UPrimitiveComponent*> PushedComponents;
		AZMBCharacter* Player = nullptr;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Actor = Overlap.GetActor();
			if (!Actor || Actor == Source)
			{
				continue;
			}
			if (!DoneActors.Contains(Actor))
			{
				DoneActors.Add(Actor);
				const float Distance = FVector::Dist(Actor->GetActorLocation(), Origin);
				const float Damage = ZR::BlastDamageAt(Blast, Distance);
				if (AZMBZombie* Zombie = Cast<AZMBZombie>(Actor))
				{
					const ZR::FHitOutcome Outcome = Zombie->TakeBlast(Damage, Origin, Blast.Impulse * ZR::BlastFalloff(Blast, Distance), Context);
					Result.Kills += Outcome.bKilled ? 1 : 0;
					Result.Gibs += Outcome.bGibbed ? 1 : 0;
				}
				else if (AZMBExplosive* Explosive = Cast<AZMBExplosive>(Actor))
				{
					Explosive->TakeExplosiveDamage(Damage, Instigator, ChainDepth + 1);
				}
				else if (AZMBCharacter* Character = Cast<AZMBCharacter>(Actor))
				{
					Player = Character;
					// a própria explosão machuca menos que nos zumbis (GTA: dá para sobreviver colado num bujão, por pouco)
					UGameplayStatics::ApplyDamage(Character, Damage * 0.45f, Instigator, Source, nullptr);
					const FVector Away = (Character->GetActorLocation() - Origin).GetSafeNormal2D();
					Character->LaunchCharacter((Away + FVector(0, 0, 0.6f)) * Blast.Impulse * 0.35f * ZR::BlastFalloff(Blast, Distance), true, true);
				}
			}
			UPrimitiveComponent* Component = Overlap.GetComponent();
			if (Component && Component->IsSimulatingPhysics() && !PushedComponents.Contains(Component))
			{
				PushedComponents.Add(Component);
				Component->AddRadialImpulse(Origin, Blast.OuterRadiusCm, Blast.Impulse, ERadialImpulseFalloff::RIF_Linear, true);
			}
		}

		AZMBFx::Explosion(World, Origin, Blast.OuterRadiusCm);

		// tremor de câmera pela distância até o jogador (mesmo fora do raio de dano)
		if (!Player)
		{
			Player = Cast<AZMBCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
		}
		if (Player)
		{
			// 1 até meio raio, cai em linha reta até 0 a quatro raios
			const float Distance = FVector::Dist(Player->GetActorLocation(), Origin);
			const float Near = Blast.OuterRadiusCm * 0.5f;
			const float Span = FMath::Max(Blast.OuterRadiusCm * 3.5f, 1.0f);
			Player->AddTrauma(1.0f - FMath::Clamp((Distance - Near) / Span, 0.0f, 1.0f));
		}

		if (AZMBHordeDirector* Director = AZMBHordeDirector::Find(World))
		{
			Director->ReportExplosion(Origin, Noise);
		}
		const bool bPlayerCaused = Instigator && Instigator->IsPlayerController();
		if (bPlayerCaused)
		{
			if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(World))
			{
				Stream->ReportExplosion(Result.Kills, Result.Gibs, ChainDepth, Origin);
			}
		}
		UE_LOG(LogZombie, Verbose, TEXT("Explosão em %s: %d abates, %d despedaçados, cadeia %d"), *Origin.ToString(), Result.Kills, Result.Gibs, ChainDepth);
		return Result;
	}
}
