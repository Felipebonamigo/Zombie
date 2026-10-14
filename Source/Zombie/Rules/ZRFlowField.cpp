#include "Rules/ZRFlowField.h"

#include <cmath>
#include <functional>
#include <queue>
#include <utility>

namespace ZR
{
	namespace
	{
		struct FFlowNeighbor
		{
			int Dx;
			int Dy;
			int Step;
		};

		constexpr FFlowNeighbor FlowNeighbors[8] = {
			{1, 0, 10}, {-1, 0, 10}, {0, 1, 10}, {0, -1, 10},
			{1, 1, 14}, {1, -1, 14}, {-1, 1, 14}, {-1, -1, 14},
		};
	}

	bool FFlowField::Walk(int X, int Y) const
	{
		return X >= 0 && Y >= 0 && X < Width && Y < Height && Open[static_cast<std::size_t>(Y * Width + X)] != 0;
	}

	void FFlowField::Build(const std::vector<std::uint8_t>& Walkable, int InWidth, int InHeight, int TargetX, int TargetY)
	{
		Width = InWidth > 0 ? InWidth : 0;
		Height = InHeight > 0 ? InHeight : 0;
		TargetCellX = TargetX;
		TargetCellY = TargetY;
		const std::size_t Total = static_cast<std::size_t>(Width) * static_cast<std::size_t>(Height);
		Open.assign(Total, 0);
		for (std::size_t i = 0; i < Total && i < Walkable.size(); ++i)
		{
			Open[i] = Walkable[i] != 0 ? 1 : 0;
		}
		Cost.assign(Total, -1);
		bValid = Walk(TargetX, TargetY);
		if (!bValid)
		{
			return;
		}

		using FEntry = std::pair<int, int>; // custo, índice
		std::priority_queue<FEntry, std::vector<FEntry>, std::greater<FEntry>> Frontier;
		Cost[static_cast<std::size_t>(TargetY * Width + TargetX)] = 0;
		Frontier.push(FEntry(0, TargetY * Width + TargetX));
		while (!Frontier.empty())
		{
			const FEntry Top = Frontier.top();
			Frontier.pop();
			const int X = Top.second % Width;
			const int Y = Top.second / Width;
			if (Top.first != Cost[static_cast<std::size_t>(Top.second)])
			{
				continue;
			}
			for (const FFlowNeighbor& N : FlowNeighbors)
			{
				const int NextX = X + N.Dx;
				const int NextY = Y + N.Dy;
				if (!Walk(NextX, NextY))
				{
					continue;
				}
				if (N.Dx != 0 && N.Dy != 0 && (!Walk(X + N.Dx, Y) || !Walk(X, Y + N.Dy)))
				{
					continue;
				}
				const std::size_t NextIndex = static_cast<std::size_t>(NextY * Width + NextX);
				const int NewCost = Top.first + N.Step;
				if (Cost[NextIndex] < 0 || NewCost < Cost[NextIndex])
				{
					Cost[NextIndex] = NewCost;
					Frontier.push(FEntry(NewCost, NextY * Width + NextX));
				}
			}
		}
	}

	int FFlowField::CostAt(int X, int Y) const
	{
		if (X < 0 || Y < 0 || X >= Width || Y >= Height)
		{
			return -1;
		}
		return Cost[static_cast<std::size_t>(Y * Width + X)];
	}

	bool FFlowField::StepToward(int X, int Y, int& OutX, int& OutY) const
	{
		const int Here = CostAt(X, Y);
		if (Here <= 0)
		{
			return false;
		}
		int Best = Here;
		bool bFound = false;
		for (const FFlowNeighbor& N : FlowNeighbors)
		{
			const int NextX = X + N.Dx;
			const int NextY = Y + N.Dy;
			const int NextCost = CostAt(NextX, NextY);
			if (NextCost < 0 || NextCost >= Best)
			{
				continue;
			}
			// a mesma regra de quina da construção: o passo diagonal precisa das duas ortogonais livres
			if (N.Dx != 0 && N.Dy != 0 && (!Walk(X + N.Dx, Y) || !Walk(X, Y + N.Dy)))
			{
				continue;
			}
			Best = NextCost;
			OutX = NextX;
			OutY = NextY;
			bFound = true;
		}
		return bFound;
	}

	bool FFlowField::Direction(int X, int Y, double& OutDx, double& OutDy) const
	{
		int NextX = 0;
		int NextY = 0;
		if (!StepToward(X, Y, NextX, NextY))
		{
			return false;
		}
		const double Dx = NextX - X;
		const double Dy = NextY - Y;
		const double Length = std::sqrt(Dx * Dx + Dy * Dy);
		OutDx = Dx / Length;
		OutDy = Dy / Length;
		return true;
	}
}
