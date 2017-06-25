#pragma once

namespace GO
{
	enum class TaskIdentifiers : unsigned int
	{
		CalculateCombat,
		CalculateMovement,
		UpdateInput,
		UpdatePlayerInput,
		CalculateFinished,
		ApplyCombatDamage,
		ApplyEntityDeaths,
		ApplyEntitySpawns,
		ApplyEntityMovement,
		DebugInput,
		MaxNumberTasks
	};
}