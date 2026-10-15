// Testes: filtro de palavras do Modo Streamer (o que vem da Twitch antes de aparecer na tela).
// Contrato: docs/regras-de-jogo.md, seção 16. Usa ReadDataFile e ExpectNoErrors de ZRTestsData.inl.

#include "Rules/ZRFilter.h"

namespace
{
	ZR::FWordFilter Filtro()
	{
		ZR::FWordFilter Filter;
		Filter.Set(std::vector<std::string>{"porra", "cu", "fod*", "merd*", "caralh*", "puta"});
		return Filter;
	}

	ZR::FFilterTerm Colado(const char* Text)
	{
		ZR::FFilterTerm Term;
		Term.Text = Text;
		Term.bGlued = true;
		return Term;
	}

	ZR::FFilterTerm Solto(const char* Text)
	{
		ZR::FFilterTerm Term;
		Term.Text = Text;
		return Term;
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
	ZR_EQ(Filter.Mask("p.o.r.r.a"), std::string("*.*.*.*.*"));
	// acento conta como uma letra só (UTF-8)
	ZR_EQ(Filter.Mask("vai tomar no cú"), std::string("vai tomar no **"));
	// prefixo: "fod*" pega as variações
	ZR_EQ(Filter.Mask("fodase fodido"), std::string("****** ******"));
	// menção: o @ do começo fica (não vira "a")
	ZR_EQ(Filter.Mask("@porra"), std::string("@*****"));
	// espaços e quebras ficam como estavam
	ZR_EQ(Filter.Mask("  oi   gente  "), std::string("  oi   gente  "));
}

ZR_TEST(FiltroColadas, "Filtro.PalavrasColadasPorPontuacao")
{
	const ZR::FWordFilter Filter = Filtro();
	ZR_EQ(Filter.Mask("porra,caralho"), std::string("*****,*******"));
	ZR_EQ(Filter.Mask("filho-da-puta"), std::string("filho-da-****"));
	ZR_EQ(Filter.Mask("vai/se/fod3r"), std::string("vai/se/*****"));
	ZR_EQ(Filter.Mask("puta_merda!"), std::string("****_*****!"));
}

ZR_TEST(FiltroEsticada, "Filtro.LetraEsticadaNaoEscapaEDobradaNormalNaoEhPega")
{
	ZR::FWordFilter Filter;
	Filter.Set(std::vector<std::string>{"nigg*", "asshol*", "faggot*", "porra", "fod*", "shit*", "pass*"});
	// letra esticada (3 ou mais) não escapa, em qualquer posição
	ZR_EQ(Filter.Mask("niiigga aaasshole faaaggot porrrra fooooda"), std::string("******* ********* ******** ******* *******"));
	// a letra dobrada do termo precisa estar dobrada: "night" e "pasta" não são "nigg*" e "pass*"
	ZR_EQ(Filter.Mask("night pasta passsaro"), std::string("night pasta ********"));
	// letra dobrada é a grafia normal de muita palavra: não vale como esticada
	ZR_EQ(Filter.Mask("food foods fodder f00d shiitake Shiite"), std::string("food foods fodder f00d shiitake Shiite"));
}

ZR_TEST(FiltroInocente, "Filtro.NaoPegaPalavraInocente")
{
	const ZR::FWordFilter Filter = Filtro();
	ZR_EQ(Filter.Mask("cuidado com o computador"), std::string("cuidado com o computador"));
	ZR_EQ(Filter.Mask("fofo demais, curitiba, deputado"), std::string("fofo demais, curitiba, deputado"));
	ZR_EQ(Filter.Mask("explode o bujão!"), std::string("explode o bujão!"));
	// sem termos, nada muda
	const ZR::FWordFilter Empty;
	ZR_EXPECT(Empty.IsEmpty());
	ZR_EQ(Empty.Mask("porra"), std::string("porra"));
}

ZR_TEST(FiltroNome, "Filtro.NomeDeEspectadorPorPedacoEColadoSoComMarca")
{
	ZR::FWordFilter Filter;
	Filter.Set(std::vector<ZR::FFilterTerm>{Colado("porra"), Solto("puta"), Solto("cock"), Solto("dick"), Solto("shit*"),
		Solto("cunt*"), Solto("bicha"), Solto("merd*"), Solto("fuck*"), Solto("fod*"), Solto("cu")});
	// pedaço do nome (separado por _ ou pela troca de minúscula para maiúscula) que é termo
	ZR_EXPECT(Filter.IsOffensiveName("Merda_123"));
	ZR_EXPECT(Filter.IsOffensiveName("Fuckboy99"));
	ZR_EXPECT(Filter.IsOffensiveName("puta_merda"));
	ZR_EXPECT(Filter.IsOffensiveName("BigDick"));
	ZR_EXPECT(Filter.IsOffensiveName("cu"));
	// colado no meio do nome: só termo marcado como "colado"
	ZR_EXPECT(Filter.IsOffensiveName("xXporraXx"));
	ZR_EXPECT(Filter.IsOffensiveName("xXporrrraXx"));
	// nomes comuns que contêm um termo não marcado
	const char* Comuns[] = {"computador_br", "deputado", "Disputa", "Hancock", "Peacock", "Dickson", "Matsushita", "Scunthorpe",
		"Bicharada", "FoodLover", "cuca_beludo", "Kaique_Brasa", "Diaspora", ""};
	for (const char* Name : Comuns)
	{
		if (Filter.IsOffensiveName(Name))
		{
			T.Fail(std::string("o nome comum virou espectador: ") + Name);
		}
	}
}

ZR_TEST(DadosFiltroErros, "Dados.FiltroComErroEhApontado")
{
	const std::string Csv =
		"termo,idioma,colado\n"
		"porra,pt,1\n"
		"fod*,pt,0\n"
		"PORRA,pt,0\n"
		"!!!,pt,0\n"
		"f*da,pt,0\n"
		"filho da puta,pt,0\n"
		"shit*,en,talvez\n"
		"cock,en,0\n";
	std::vector<ZR::FFilterTerm> Terms;
	std::vector<std::string> Errors;
	ZR::LoadFilterTerms(Csv, Terms, Errors);
	// repetido (depois de normalizar), sem letra, "*" fora do fim, com espaço e colado que não é 0 ou 1
	ZR_EQ(Errors.size(), static_cast<std::size_t>(5));
	ZR_EQ(Terms.size(), static_cast<std::size_t>(3));
	ZR_EXPECT(Terms.size() == 3 && Terms[0].Text == "porra" && Terms[0].bGlued);
	ZR_EXPECT(Terms.size() == 3 && Terms[1].Text == "fod*" && !Terms[1].bGlued && Terms[2].Text == "cock");
}

ZR_TEST(DerivaFiltro, "Deriva.FiltroDePalavrasCarregaEPegaOBasico")
{
	std::string Text;
	if (!ReadDataFile(T, "FiltroPalavras.csv", Text))
	{
		return;
	}
	std::vector<ZR::FFilterTerm> Terms;
	std::vector<std::string> Errors;
	ZR::LoadFilterTerms(Text, Terms, Errors);
	ExpectNoErrors(T, "FiltroPalavras.csv", Errors);
	ZR_EXPECT(Terms.size() >= 20);
	ZR::FWordFilter Filter;
	Filter.Set(Terms);
	ZR_EQ(Filter.Mask("que porra"), std::string("que *****"));
	// palavras comuns que lembram um termo não podem ser pegas
	const char* Inocentes[] = {"food", "fodder", "shiitake", "computador", "deputado", "viaduto", "pasta", "night", "retardar",
		"cuidado", "curitiba", "bicharada", "passaro", "assim", "Hancock", "Dickson", "Scunthorpe", "assassino", "classe"};
	for (const char* Word : Inocentes)
	{
		if (Filter.Mask(Word) != Word)
		{
			T.Fail(std::string("o filtro pega a palavra comum: ") + Word);
		}
	}
	// apelidos comuns que contêm um termo não podem virar "espectador"
	const char* Apelidos[] = {"Aviador_BR", "computador_br", "Hancock", "Dickson", "Scunthorpe", "Matsushita", "Bicharada",
		"FoodLover", "Classe_A", "Assassino", "Deputado", "Disputa", "Cockpit", "Retardant"};
	for (const char* Name : Apelidos)
	{
		if (Filter.IsOffensiveName(Name))
		{
			T.Fail(std::string("o apelido comum vira espectador: ") + Name);
		}
	}
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
