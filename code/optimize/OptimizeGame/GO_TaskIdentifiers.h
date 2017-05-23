#pragma once

namespace GO
{
	enum class TaskIdentifiers : unsigned int
	{
		CalculateCombat,
		CalculateMovement,
		UpdateInput,
		CalculateFinished,
		ApplyCombatDamage,
		ApplyEntityDeaths,
		ApplyEntitySpawns,
		ApplyEntityMovement,
		DebugInput,
		MaxNumberTasks
	};
}