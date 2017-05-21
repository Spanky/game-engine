#pragma once

namespace GO
{
	enum class TaskIdentifiers : unsigned int
	{
		CalculateCombat,
		CalculateMovement,
		CalculateFinished,
		ApplyCombatDamage,
		ApplyEntityDeaths,
		ApplyEntitySpawns,
		ApplyEntityMovement,
		MaxNumberTasks
	};
}