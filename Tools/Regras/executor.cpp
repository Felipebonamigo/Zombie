// Executor dos testes das regras fora do Unreal (nuvem ou qualquer PC com um compilador C++20).
// Uso: executor <pasta Data/> [filtro]   (o filtro é um prefixo do nome, ex.: "Hype." ou "Deriva.")
// Sai 0 se tudo passou, 1 se algum teste falhou ou nenhum teste casou com o filtro.

#include <cstdio>
#include <cstring>
#include <string>

#include "Rules/Tests/ZRAllTests.inl"

int main(int ArgCount, char** Args)
{
	const std::string DataDir = ArgCount > 1 ? Args[1] : "Data";
	const std::string Filter = ArgCount > 2 ? Args[2] : "";
	int Ran = 0;
	int Failed = 0;
	int Checks = 0;
	for (const ZRTest::FCase& Case : ZRTest::Registry())
	{
		if (!Filter.empty() && std::strncmp(Case.Name, Filter.c_str(), Filter.size()) != 0)
		{
			continue;
		}
		ZRTest::FContext Context;
		Context.DataDir = DataDir;
		Case.Fn(Context);
		++Ran;
		Checks += Context.Checks;
		if (Context.Failures.empty())
		{
			std::printf("  [OK    ] %s\n", Case.Name);
		}
		else
		{
			++Failed;
			std::printf("  [FALHOU] %s\n", Case.Name);
			for (const std::string& Failure : Context.Failures)
			{
				std::printf("           %s\n", Failure.c_str());
			}
		}
	}
	std::printf("\nRESUMO: %d testes, %d aprovados, %d reprovados, %d verificações\n", Ran, Ran - Failed, Failed, Checks);
	if (Ran == 0)
	{
		std::printf("Nenhum teste casou com o filtro '%s'.\n", Filter.c_str());
		return 1;
	}
	return Failed == 0 ? 0 : 1;
}
