using UnrealBuildTool;

public class Zombie : ModuleRules
{
	public Zombie(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Sem unity build (lição do Detonauto): helpers em namespace anônimo com nomes curtos colidiam quando o unity juntava
		// arquivos diferentes. Projeto pequeno: o custo de compilação é baixo.
		bUseUnity = false;

		// Rules/ é C++ puro (std), sem Unreal: compila aqui e também na nuvem com g++ (Tools/Regras/testar.sh).
		// EnhancedInput: controles criados em código (sem asset). WebSockets: leitura do chat da Twitch (Stream/ZMBTwitchChat).
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "WebSockets" });

		// Um módulo só, com subpastas (Rules, Game, Player...): os includes são "Rules/ZRHype.h", "Player/ZMBCharacter.h" etc.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
