// Dados do jogo: lê Data/*.csv (pelas regras puras de Rules/ZRData.h) uma vez por GameInstance e guarda os materiais de
// cor compartilhados. Tabelas e colunas: docs/dados.md. Erro de dado não derruba o jogo: vale o padrão do código e o
// erro vai para o log (LogZombie) e para o HUD.
//
// Materiais: se o projeto tiver /Game/Zombie/Materials/M_ZMBSurface e M_ZMBEmissive (criados por
// Tools/Unreal/criar_materiais.py), usa os dois; senão cai no BasicShapeMaterial da engine (sem brilho próprio).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Rules/ZRData.h"
#include "ZMBGameData.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;

enum class EZMBShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone,
};

UCLASS()
class ZOMBIE_API UZMBGameData : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Nulo fora de um mundo de jogo. */
	static UZMBGameData* Get(const UObject* WorldContext);

	/** Lê de novo as tabelas (console: zmb.dados.recarregar). Vale para o que nascer depois. */
	void Reload();
	/** Quantas vezes as tabelas foram lidas: quem guarda algo por linha (o som) compara e limpa ao mudar. */
	int32 GetReloadCount() const { return ReloadCount; }

	const ZR::FTuning& Tuning() const { return LoadedTuning; }
	const std::vector<ZR::FZombieType>& Zombies() const { return LoadedZombies; }
	const std::vector<ZR::FWeaponType>& Weapons() const { return LoadedWeapons; }
	const std::vector<ZR::FExplosiveType>& Explosives() const { return LoadedExplosives; }
	const std::vector<ZR::FHypeEventDef>& HypeEvents() const { return LoadedHypeEvents; }
	const std::vector<ZR::FChatLine>& ChatLines() const { return LoadedChatLines; }
	const std::vector<ZR::FChallengeDef>& Challenges() const { return LoadedChallenges; }
	/** Marcos de seguidores da live (Data/Marcos.csv), em ordem crescente. */
	const std::vector<ZR::FMilestoneDef>& Milestones() const { return LoadedMilestones; }
	/** Sons (Data/Sons.csv); quem toca é o Audio/ZMBAudioSubsystem. */
	const std::vector<ZR::FSoundDef>& Sounds() const { return LoadedSounds; }
	/** Peças do modo construção, na ordem da tabela (Data/Construcoes.csv). */
	const std::vector<ZR::FBuildPieceType>& BuildPieces() const { return LoadedBuildPieces; }
	/** Filtro de palavras do Modo Streamer (Data/FiltroPalavras.csv; ligado por chat.filtro). */
	const ZR::FWordFilter& WordFilter() const { return LoadedWordFilter; }
	/** Lugares de saque (Data/Saque.csv) e o que cada um rende (Data/SaqueItens.csv). */
	const std::vector<ZR::FLootSpotType>& LootSpots() const { return LoadedLootSpots; }
	const std::vector<ZR::FLootItemDef>& LootItems() const { return LoadedLootItems; }

	/** Uma malha da cidade já carregada (Data/Cidade_Malhas.csv). */
	struct FCityMesh
	{
		UStaticMesh* Mesh = nullptr;
		float Scale = 1.0f;
	};
	/**
	 * As variantes carregadas de um id da tabela de malhas (arvore, poste...). Vazio quando a tabela não tem o id ou o asset
	 * não existe no disco (conteúdo da Epic fica fora do git): quem chama volta ao bloco do protótipo.
	 */
	TArray<FCityMesh> CityMeshes(const char* Id);

	const ZR::FZombieType* FindZombie(const char* Id) const;
	const ZR::FExplosiveType* FindExplosive(const char* Id) const;

	/** Erros da última leitura (vazio = tudo certo). */
	const TArray<FString>& DataErrors() const { return Errors; }

	/** Material de cor lisa, compartilhado (um por combinação de cor e aspereza). */
	UMaterialInterface* ColorMaterial(const FLinearColor& Color, float Roughness = 0.7f, float Metallic = 0.0f);

	/**
	 * SuperfÃ­cie com textura PBR projetada pelo mundo (asfalto, calÃ§ada, fachada: Tools/Unreal/criar_superficies.ps1), com um tom
	 * (Tint) que multiplica a cor da textura. Sem a textura no disco (ela fica fora do git), volta Ã  cor lisa Fallback.
	 */
	UMaterialInterface* SurfaceMaterial(const TCHAR* Name, const FLinearColor& Tint, const FLinearColor& Fallback, float FallbackRoughness);

	/** Material que brilha (bola de fogo, rastro de tiro, janela acesa). Intensity é o multiplicador da cor emitida. */
	UMaterialInstanceDynamic* GlowMaterial(const FLinearColor& Color, float Intensity);

	/** Formas básicas da engine (/Engine/BasicShapes): 100 cm de lado ou diâmetro, pivô no centro. */
	static UStaticMesh* Shape(EZMBShape Kind);

	static FLinearColor ToLinear(std::uint32_t Rgb);
	static FString ToFString(const std::string& Utf8);

	/** Português (cultura do jogo começa com "pt"): escolhe a coluna _pt ou _en das tabelas. */
	static bool IsPortuguese();

private:
	bool LoadText(const TCHAR* FileName, std::string& Out);

	ZR::FTuning LoadedTuning;
	std::vector<ZR::FZombieType> LoadedZombies;
	std::vector<ZR::FWeaponType> LoadedWeapons;
	std::vector<ZR::FExplosiveType> LoadedExplosives;
	std::vector<ZR::FHypeEventDef> LoadedHypeEvents;
	std::vector<ZR::FChatLine> LoadedChatLines;
	std::vector<ZR::FChallengeDef> LoadedChallenges;
	std::vector<ZR::FBuildPieceType> LoadedBuildPieces;
	std::vector<ZR::FSoundDef> LoadedSounds;
	std::vector<ZR::FMilestoneDef> LoadedMilestones;
	std::vector<ZR::FLootSpotType> LoadedLootSpots;
	std::vector<ZR::FLootItemDef> LoadedLootItems;
	std::vector<ZR::FCityMeshDef> LoadedCityMeshes;
	TSet<FString> MissingCityMeshes;
	ZR::FWordFilter LoadedWordFilter;
	TArray<FString> Errors;
	int32 ReloadCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SurfaceParent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlowParent;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UMaterialInstanceDynamic>> Materials;

	/** Malhas carregadas por caminho (a referência forte segura contra a coleta de lixo). */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UStaticMesh>> CityMeshCache;

	/** InstÃ¢ncias de superfÃ­cie carregadas por caminho; o que falhou aparece uma vez no log e nÃ£o Ã© tentado de novo. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UMaterialInterface>> SurfaceCache;
	TSet<FString> MissingSurfaces;
};
