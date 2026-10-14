// Dia e noite (Night of the Dead: de dia se prepara, de noite se sobrevive). Regras puras: sem Unreal.
// Contrato: docs/regras-de-jogo.md, seção 7.

#pragma once

namespace ZR
{
	struct FDayConfig
	{
		/** Um dia inteiro de 24 h dura isto em tempo real [s]. */
		float DayLengthSeconds = 1440.0f;
		/** Hora em que o jogo começa (fim de tarde: luz dourada). */
		float StartHour = 17.0f;
		float NightStartHour = 19.5f;
		float NightEndHour = 5.5f;
		/** A cada quantas noites vem a Noite da Horda (0 = nunca). A primeira noite é a de índice 0. */
		int HordeEveryNights = 3;
		/** Altura máxima do sol ao meio-dia [graus]. */
		float MaxSunElevationDeg = 72.0f;
	};

	/** Hora do dia em [0, 24) depois de Seconds segundos de jogo. */
	double HourAt(const FDayConfig& Config, double Seconds);

	/** Quantas vezes já passou da meia-noite desde o começo. */
	int DayIndexAt(const FDayConfig& Config, double Seconds);

	/** Noite em [NightStartHour, 24) U [0, NightEndHour) quando o início é maior que o fim. */
	bool IsNight(const FDayConfig& Config, double Hour);

	/** Elevação do sol: 0 às 6 h e às 18 h, MaxElevation ao meio-dia, -MaxElevation à meia-noite. */
	double SunElevationDegrees(double Hour, float MaxElevationDeg);

	/** A noite de índice NightIndex (0, 1, 2...) é Noite da Horda? */
	bool IsHordeNight(const FDayConfig& Config, int NightIndex);
}
