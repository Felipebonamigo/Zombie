// Kit mínimo de testes das regras puras. Sem Unreal: o mesmo arquivo de testes roda
//  - na nuvem, compilado com g++/clang++ por Tools/Regras/testar.sh;
//  - no PC, dentro do editor, como teste de Automation "Zombie.Regras.*" (Rules/Tests/ZRRules.test.cpp).
// Cada teste se registra sozinho com ZR_TEST; o executor percorre ZRTest::Registry().

#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ZRTest
{
	struct FContext
	{
		/** Pasta Data/ do repositório (os testes de deriva leem as tabelas de verdade). */
		std::string DataDir;
		std::vector<std::string> Failures;
		int Checks = 0;

		void Expect(bool bOk, const char* What, const char* File, int Line)
		{
			++Checks;
			if (!bOk)
			{
				Failures.push_back(std::string(File) + ":" + std::to_string(Line) + ": " + What);
			}
		}

		void Near(double A, double B, double Tolerance, const char* What, const char* File, int Line)
		{
			++Checks;
			if (!(std::fabs(A - B) <= Tolerance))
			{
				char Buffer[160];
				std::snprintf(Buffer, sizeof(Buffer), " (obtido %.6g, esperado %.6g, tolerância %.3g)", A, B, Tolerance);
				Failures.push_back(std::string(File) + ":" + std::to_string(Line) + ": " + What + Buffer);
			}
		}

		void Fail(const std::string& Message)
		{
			++Checks;
			Failures.push_back(Message);
		}
	};

	using FTestFn = void (*)(FContext&);

	struct FCase
	{
		const char* Name;
		FTestFn Fn;
	};

	inline std::vector<FCase>& Registry()
	{
		static std::vector<FCase> Cases;
		return Cases;
	}

	struct FRegistrar
	{
		FRegistrar(const char* Name, FTestFn Fn)
		{
			Registry().push_back(FCase{Name, Fn});
		}
	};
}

/** Declara e registra um teste. Name: "Grupo.Descricao", sem acento (vira o nome do teste no Unreal). */
#define ZR_TEST(Fn, Name) \
	static void Fn(ZRTest::FContext& T); \
	static const ZRTest::FRegistrar Fn##Registrar(Name, &Fn); \
	static void Fn(ZRTest::FContext& T)

#define ZR_EXPECT(Cond) T.Expect((Cond), #Cond, __FILE__, __LINE__)
#define ZR_EQ(A, B) T.Expect((A) == (B), #A " == " #B, __FILE__, __LINE__)
#define ZR_NEAR(A, B, Tol) T.Near(static_cast<double>(A), static_cast<double>(B), static_cast<double>(Tol), #A " ~ " #B, __FILE__, __LINE__)
