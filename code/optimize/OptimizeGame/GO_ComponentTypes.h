#pragma once

// TODO(scarroll): Investigate a way to represent unique identifiers for each component at compile
//					time without requiring an enum with all the different types in it
enum class ComponentType : int
{
	HealthComponent,
	MovementComponent,
	RandomMovementComponent,
	PlayerInputComponent,
	SpriteComponent,

	NumComponents
};