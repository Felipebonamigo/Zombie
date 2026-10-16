// Testes: tabela de malhas da cidade (Data/Cidade_Malhas.csv). A cidade final troca os blocos do protótipo por malhas de
// conteúdo da Epic, que fica só em Content/Externo/ (fora do git); a tabela é o que liga um ao outro.

ZR_TEST(CidadeMalhasLeVariantes, "Cidade.MalhasLeVariantesPorId")
{
	const std::string Csv =
		"id,malha,escala\n"
		"arvore,/Game/Externo/CitySample/Prop/A/Arvore_A,1\n"
		"arvore,/Game/Externo/CitySample/Prop/A/Arvore_B,1.2\n"
		"poste,/Game/Externo/CitySample/Prop/B/Poste,\"0,9\"\n";
	std::vector<ZR::FCityMeshDef> Meshes;
	std::vector<std::string> Errors;
	ZR::LoadCityMeshes(Csv, Meshes, Errors);
	ZR_EQ(Errors.size(), static_cast<std::size_t>(0));
	ZR_EQ(Meshes.size(), static_cast<std::size_t>(3));
	ZR_EQ(ZR::CityMeshesFor(Meshes, "arvore").size(), static_cast<std::size_t>(2));
	ZR_EQ(ZR::CityMeshesFor(Meshes, "poste").size(), static_cast<std::size_t>(1));
	ZR_EQ(ZR::CityMeshesFor(Meshes, "lixeira").size(), static_cast<std::size_t>(0));
	ZR_EXPECT(ZR::CityMeshesFor(Meshes, "poste")[0]->Scale > 0.89f && ZR::CityMeshesFor(Meshes, "poste")[0]->Scale < 0.91f);
}

ZR_TEST(CidadeMalhasErros, "Cidade.MalhaComErroSaiENaoEntraNaTabela")
{
	const std::string Csv =
		"id,malha,escala\n"
		"naoexiste,/Game/Externo/CitySample/Prop/X,1\n"
		"arvore,/Game/Zombie/Meshes/Arvore,1\n"
		"arvore,/Game/Externo/CitySample/Prop/A/Arvore_A,0\n"
		"arvore,,1\n"
		"arvore,/Game/Externo/CitySample/Prop/A/Arvore_B,1\n";
	std::vector<ZR::FCityMeshDef> Meshes;
	std::vector<std::string> Errors;
	ZR::LoadCityMeshes(Csv, Meshes, Errors);
	ZR_EQ(Meshes.size(), static_cast<std::size_t>(1));
	ZR_EQ(Errors.size(), static_cast<std::size_t>(4));
	bool bMentionsExterno = false;
	for (const std::string& Error : Errors)
	{
		bMentionsExterno = bMentionsExterno || Error.find("/Game/Externo/") != std::string::npos;
	}
	ZR_EXPECT(bMentionsExterno);
}

ZR_TEST(DerivaCidadeMalhas, "Deriva.CidadeMalhasCarregamEFicamEmExterno")
{
	std::string Text;
	if (!ReadDataFile(T, "Cidade_Malhas.csv", Text))
	{
		return;
	}
	std::vector<ZR::FCityMeshDef> Meshes;
	std::vector<std::string> Errors;
	ZR::LoadCityMeshes(Text, Meshes, Errors);
	ExpectNoErrors(T, "Cidade_Malhas.csv", Errors);
	for (const char* Id : {"arvore", "poste", "caixa_dagua", "ar_condicionado", "lixeira"})
	{
		if (ZR::CityMeshesFor(Meshes, Id).empty())
		{
			T.Fail(std::string("Cidade_Malhas.csv sem nenhuma malha para ") + Id);
		}
	}
}
