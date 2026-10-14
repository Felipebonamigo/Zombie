// Os testes das regras puras dentro do Unreal: os MESMOS casos que Tools/Regras/testar.sh roda na nuvem com g++,
// registrados como testes de Automation "Zombie.Regras.<Grupo>.<Caso>" (Tools/Unreal/test.ps1 -Filter Zombie.Regras).

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Rules/Tests/ZRAllTests.inl"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FZombieRulesTest, "Zombie.Regras", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FZombieRulesTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const ZRTest::FCase& Case : ZRTest::Registry())
	{
		OutBeautifiedNames.Add(FString(UTF8_TO_TCHAR(Case.Name)));
		OutTestCommands.Add(FString(UTF8_TO_TCHAR(Case.Name)));
	}
}

bool FZombieRulesTest::RunTest(const FString& Parameters)
{
	const std::string Wanted(TCHAR_TO_UTF8(*Parameters));
	for (const ZRTest::FCase& Case : ZRTest::Registry())
	{
		if (Wanted != Case.Name)
		{
			continue;
		}
		ZRTest::FContext Context;
		const FString DataDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Data")));
		Context.DataDir = std::string(TCHAR_TO_UTF8(*DataDir));
		Case.Fn(Context);
		for (const std::string& Failure : Context.Failures)
		{
			AddError(FString(UTF8_TO_TCHAR(Failure.c_str())));
		}
		return Context.Failures.empty();
	}
	AddError(FString::Printf(TEXT("o teste '%s' não existe"), *Parameters));
	return false;
}

#endif
