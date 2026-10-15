// Testes: filtro de palavras do Modo Streamer (o que vem da Twitch antes de aparecer na tela).
// Contrato: docs/regras-de-jogo.md, seção 16. Usa ReadDataFile e ExpectNoErrors de ZRTestsData.inl.

#include "Rules/ZRFilter.h"

namespace
{
	ZR::FWordFilter Filtro()
	{
		ZR::FWordFilter Filter;
		Filter.Set({"porra", "cu", "fod*", "merda"});
		return Filter;
	}
}

ZR_TEST(FiltroNormaliza, "Filtro.NormalizaAcentoMaiusculaDisfarceERepeticao")
{
	ZR_EQ(ZR::NormalizeForFilter("PoRRa"), std::string("pora"));
	ZR_EQ(ZR::NormalizeForFilter("P0RR4"), std::string("pora"));
	ZR_EQ(ZR::NormalizeForFilter("p.o.r.r.a!!!"), std::string("pora"));
	ZR_EQ(ZR::NormalizeForFilter("porrrrraaaa"), std::string("pora"));
	ZR_EQ(ZR::NormalizeForFilter("CÚ"), std::string("cu"));
	ZR_EQ(ZR::NormalizeForFilter("Ação"), std::string("acao"));
	ZR_EQ(ZR::NormalizeForFilter("m3rd@$"), std::string("merdas"));
	ZR_EQ(ZR::NormalizeForFilter("!!!"), std::string());
}

ZR_TEST(FiltroTroca, "Filtro.TrocaSoAsLetrasDaPalavraEORestoFicaIgual")
{
	const ZR::FWordFilter Filter = Filtro();
	ZR_EQ(Filter.Mask("que porra é essa"), std::string("que ***** é essa"));
	ZR_EQ(Filter.Mask("PORRA!!! que merda"), std::string("*****!!! que *****"));
	ZR_EQ(Filter.Mask("P0RR4 mano"), std::string("***** mano"));
	// acento conta como uma letra só (UTF-8)
	ZR_EQ(Filter.Mask("vai tomar no cú"), std::string("vai tomar no **"));
	// prefixo: "fod*" pega as variações
	ZR_EQ(Filter.Mask("fodase fodido"), std::string("****** ******"));
	// espaços e quebras ficam como estavam
	ZR_EQ(Filter.Mask("  oi   gente  "), std::string("  oi   gente  "));
}

ZR_TEST(FiltroInocente, "Filtro.NaoPegaPalavraInocente")
{
	const ZR::FWordFilter Filter = Filtro();
	ZR_EQ(Filter.Mask("cuidado com o computador"), std::string("cuidado com o computador"));
	ZR_EQ(Filter.Mask("fofo demais, curitiba"), std::string("fofo demais, curitiba"));
	ZR_EQ(Filter.Mask("explode o bujão!"), std::string("explode o bujão!"));
	// prefixo com letra dobrada não encolhe: "pass*" não pega "pasta", mas pega "passsaro"
	ZR::FWordFilter Double;
	Double.Set({"pass*"});
	ZR_EQ(Double.Mask("pasta passsaro"), std::string("pasta ********"));
	// sem termos, nada muda
	const ZR::FWordFilter Empty;
	ZR_EXPECT(Empty.IsEmpty());
	ZR_EQ(Empty.Mask("porra"), std::string("porra"));
}

ZR_TEST(FiltroNome, "Filtro.NomeDeEspectadorComTermoColado")
{
	const ZR::FWordFilter Filter = Filtro();
	ZR_EXPECT(Filter.IsOffensiveName("xXporraXx"));
	ZR_EXPECT(Filter.IsOffensiveName("Merda_123"));
	ZR_EXPECT(Filter.IsOffensiveName("fodao"));
	ZR_EXPECT(Filter.IsOffensiveName("cu"));
	// termo curto só pega o nome inteiro
	ZR_EXPECT(!Filter.IsOffensiveName("cuca_beludo"));
	ZR_EXPECT(!Filter.IsOffensiveName("Kaique_Brasa"));
	ZR_EXPECT(!Filter.IsOffensiveName(""));
	// colado também respeita a letra dobrada do termo: "porra" não pega "diaspora"
	ZR_EXPECT(!Filter.IsOffensiveName("Diaspora"));
	ZR_EXPECT(Filter.IsOffensiveName("xXporrrraXx"));
}

ZR_TEST(DadosFiltroErros, "Dados.FiltroComErroEhApontado")
{
	const std::string Csv =
		"termo,idioma\n"
		"porra,pt\n"
		"fod*,pt\n"
		"PORRA,pt\n"
		"!!!,pt\n"
		"f*da,pt\n"
		"shit*,en\n";
	std::vector<std::string> Terms;
	std::vector<std::string> Errors;
	ZR::LoadFilterTerms(Csv, Terms, Errors);
	// repetido (depois de normalizar), vazio depois de normalizar e "*" fora do fim
	ZR_EQ(Errors.size(), static_cast<std::size_t>(3));
	ZR_EXPECT(Terms == std::vector<std::string>({"porra", "fod*", "shit*"}));
}

ZR_TEST(DerivaFiltro, "Deriva.FiltroDePalavrasCarregaEPegaOBasico")
{
	std::string Text;
	if (!ReadDataFile(T, "FiltroPalavras.csv", Text))
	{
		return;
	}
	std::vector<std::string> Terms;
	std::vector<std::string> Errors;
	ZR::LoadFilterTerms(Text, Terms, Errors);
	ExpectNoErrors(T, "FiltroPalavras.csv", Errors);
	ZR_EXPECT(Terms.size() >= 20);
	ZR::FWordFilter Filter;
	Filter.Set(Terms);
	ZR_EQ(Filter.Mask("que porra"), std::string("que *****"));
	// as falas do chat simulado e os comandos do jogo não podem ser pegos pelo filtro
	std::string Chat;
	if (ReadDataFile(T, "ChatFalso.csv", Chat))
	{
		std::vector<ZR::FChatLine> Lines;
		std::vector<std::string> ChatErrors;
		ZR::LoadChatLines(Chat, Lines, ChatErrors);
		for (const ZR::FChatLine& Line : Lines)
		{
			if (Filter.Mask(Line.TextPt) != Line.TextPt || Filter.Mask(Line.TextEn) != Line.TextEn)
			{
				T.Fail("o filtro pega a fala do chat simulado: " + Line.TextPt);
			}
		}
	}
	ZR_EQ(Filter.Mask("!zumbi !horda !bujao !caixa"), std::string("!zumbi !horda !bujao !caixa"));
}
