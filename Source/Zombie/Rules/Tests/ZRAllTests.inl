// Todos os testes das regras puras. Incluído por um único arquivo em cada executor:
// Tools/Regras/executor.cpp (nuvem, g++/clang++) e Rules/Tests/ZRRules.test.cpp (Unreal, Automation).

#pragma once

#include "Rules/Tests/ZRTestKit.h"

#include "Rules/Tests/ZRTestsCity.inl"
#include "Rules/Tests/ZRTestsCombat.inl"
#include "Rules/Tests/ZRTestsDirector.inl"
#include "Rules/Tests/ZRTestsChat.inl"
#include "Rules/Tests/ZRTestsData.inl"
// depois de ZRTestsData.inl: usa ReadDataFile e ExpectNoErrors de lá
#include "Rules/Tests/ZRTestsChallenges.inl"
#include "Rules/Tests/ZRTestsCityMeshes.inl"
#include "Rules/Tests/ZRTestsBase.inl"
#include "Rules/Tests/ZRTestsLocalGrid.inl"
#include "Rules/Tests/ZRTestsSound.inl"
#include "Rules/Tests/ZRTestsLive.inl"
#include "Rules/Tests/ZRTestsLoot.inl"
#include "Rules/Tests/ZRTestsFilter.inl"
#include "Rules/Tests/ZRTestsWound.inl"
