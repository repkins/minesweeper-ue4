#pragma once

UENUM(BlueprintType)
enum class EMineGridMapCell : uint8
{
	// The following nine enumerations represents surrounding mines count values
	MGMC_Zero = 0,
	MGMC_One = 1,
	MGMC_Two = 2,
	MGMC_Three = 3,
	MGMC_Four = 4,
	MGMC_Five = 5,
	MGMC_Six = 6,
	MGMC_Seven = 7,
	MGMC_Eight = 8,

	// The following remaining emums represents special cell values
	MGMC_Undiscovered, // Is not yet "stepped on"
	MGMC_Revealed, // Mine is revealed
	MGMC_Exploded, // Mine is exploded

	MGMC_MAX
};