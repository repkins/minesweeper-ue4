// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperPlayerControllerBase.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Minesweeper/GameMode/MinesweeperGameModeBase.h"
#include "Minesweeper/GameMode/MinesweeperGameStateBase.h"
#include "Minesweeper/HUD/MinesweeperHUDBase.h"

AMinesweeperPlayerControllerBase::AMinesweeperPlayerControllerBase(): Super()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = false;

	bIsLobbyLeader = false;
	MineGridClass = AMineGridBase::StaticClass();

	MapAreaMaxHalfSizeX = 8;
	MapAreaMaxHalfSizeY = 5;

	PrevPlayerRelativeGridCoords = FIntPoint(-1, -1);
	GridMapAreaVersion = 0;
}

void AMinesweeperPlayerControllerBase::NotifyGameStarted_Implementation()
{
	if (auto MinesweeperHUD = Cast<AMinesweeperHUDBase>(MyHUD))
	{
		MinesweeperHUD->HideGameOver();
		MinesweeperHUD->HideGameWin();
	}
}

void AMinesweeperPlayerControllerBase::NotifyGameOver_Implementation()
{
	if (auto MinesweeperHUD = Cast<AMinesweeperHUDBase>(MyHUD))
	{
		MinesweeperHUD->ShowGameOver();
	}
}

void AMinesweeperPlayerControllerBase::NotifyGameWin_Implementation()
{
	if (auto MinesweeperHUD = Cast<AMinesweeperHUDBase>(MyHUD))
	{
		MinesweeperHUD->ShowGameWin();
	}
}

void AMinesweeperPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	// Finding grid actor
	MineGridActor = FindMineGridActor();
	if (MineGridActor)
	{
		// Bind to on entered coords event of grid actor
		MineGridActor->OnCharacterTriggeredCoords.AddDynamic(this, &AMinesweeperPlayerControllerBase::HandleOnTriggeredCoords);
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MinesweeperPlayerController needs MineGrid actor placed in the level to work."));
		}
	}

	// Initializing LastCoords field
	if (MineGridActor)
	{
		if (APawn* PlayerPawn = GetPawn())
		{
			PrevPlayerRelativeGridCoords = GetPawnRelativeLocationOfGrid(PlayerPawn, MineGridActor);
		}
	}
}

void AMinesweeperPlayerControllerBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (AMinesweeperGameModeBase* MinesweeperGameMode = GetWorld()->GetAuthGameMode<AMinesweeperGameModeBase>())
	{
		const FMineGridMap& FullMineGridMap = MinesweeperGameMode->GetMineGridMap();

		// Compare grid map versions to determine if update is necessary, then proceed with update
		const uint32& FullGridMapVersion = MinesweeperGameMode->GetMineGridMapVersion();

		if (FullGridMapVersion != GridMapAreaVersion)
		{
			// Update GridMapArea values
			UpdateGridMapAreaCellValues(FullMineGridMap);

			// Update map version
			GridMapAreaVersion = FullGridMapVersion;
		}
		else
		{
			// Add&remove marginal cells of GridMapArea as necessary
			AddRemoveGridMapAreaCells(FullMineGridMap);
		}
	}
}

void AMinesweeperPlayerControllerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearAllGridCells();
}

void AMinesweeperPlayerControllerBase::AddRemoveGridMapAreaCells(const FMineGridMap& MineGridMap, bool bForcedAddRemove)
{
	if (MineGridActor)
	{
		if (APawn* PlayerPawn = GetPawn())
		{
			const FIntPoint PawnRelativeGridCoords = GetPawnRelativeLocationOfGrid(PlayerPawn, MineGridActor);

			// Check if need update by determining if pawn (player) is at the same coords as before or 
			// it is forced to update, then proceed with adding & removing marginal cells if new pawn coords is different
			if (PawnRelativeGridCoords != PrevPlayerRelativeGridCoords || bForcedAddRemove)
			{
				// Define reference to grid map dimensions
				const FIntPoint& MapGridDimensions = MineGridMap.GridDimensions;

				// Determine starting & endings coords of map area
				const FIntPoint MapAreaMaxHalfSize = FIntPoint(MapAreaMaxHalfSizeX, MapAreaMaxHalfSizeY);
				const FIntRect MaxBounds(
					PawnRelativeGridCoords - MapAreaMaxHalfSize, 
					PawnRelativeGridCoords + MapAreaMaxHalfSize
				);

				// Allocate max size of authoritive mines area
				const FIntPoint MapAreaMaxSize = MaxBounds.Size() + FIntPoint(1, 1);
				MineGridMapArea.Cells.Reserve(MapAreaMaxSize.X * MapAreaMaxSize.Y);

				// Determine valid starting & endings coords of map "visible" area
				const FIntRect NewBounds(
					FIntPoint(
						MaxBounds.Min.X >= 0 ? MaxBounds.Min.X : FMath::Min(0, MaxBounds.Max.X + 1),
						MaxBounds.Min.Y >= 0 ? MaxBounds.Min.Y : FMath::Min(0, MaxBounds.Max.Y + 1)
					),
					FIntPoint(
						MaxBounds.Max.X <= MapGridDimensions.X - 1 ? MaxBounds.Max.X : FMath::Max(MapGridDimensions.X - 1, MaxBounds.Min.X - 1),
						MaxBounds.Max.Y <= MapGridDimensions.Y - 1 ? MaxBounds.Max.Y : FMath::Max(MapGridDimensions.Y - 1, MaxBounds.Min.Y - 1)
					)
				);

				// Retrieve old starting & endings coords of map area
				const FIntRect OldBounds(MineGridMapArea.StartCoords, MineGridMapArea.EndCoords);

				// Because start and end coords are inclusive, we need to increment by 1
				const FIntPoint MapAreaSize = NewBounds.Size() + FIntPoint(1, 1);

				// Rotated bounds are [Min, Max).
				FBox2D RotatedNewBounds = FBox2D(NewBounds.Min, NewBounds.Max+1);
				FBox2D RotatedOldBounds = FBox2D(OldBounds.Min, OldBounds.Max+1);

				// Get center and extents of both new and old bounds for calculation needs
				FVector2D NewBoundsCenter, NewBoundsExtents;
				RotatedNewBounds.GetCenterAndExtents(NewBoundsCenter, NewBoundsExtents);

				FVector2D OldBoundsCenter, OldBoundsExtents;
				RotatedOldBounds.GetCenterAndExtents(OldBoundsCenter, OldBoundsExtents);

				FVector2D RotatedNewBoundsExtents = NewBoundsExtents;

				FVector2D RotatedOldBoundsCenter = OldBoundsCenter;
				FVector2D RotatedOldBoundsExtents = OldBoundsExtents;

				FBox2D SideBounds;
				FVector2D SideBoundsExtents, SideBoundsCenter;

				// Rotation matrix to do 90 degrees turn
				const FMatrix2x2 RotationMatrix(
					0, 1,
					-1, 0
				);
				FMatrix2x2 AccumulatedRotationMatrix = FMatrix2x2();

				// Sides are max-inclusive
				TArray<FIntRect> AdditiveSides;
				TArray<FIntRect> SubtractiveSides;
				AdditiveSides.Reserve(4);
				SubtractiveSides.Reserve(4);

				do {
					// Calculate difference on left edge of at time rotated old and new bounds
					const FVector2D DiffMin = RotatedOldBounds.Min - RotatedNewBounds.Min;

					if (DiffMin.X != 0)
					{
						bool bIsAdditive = DiffMin.X > 0,
								 bIsSubstractive = DiffMin.X < 0;

						if (bIsAdditive)
						{
							SideBounds.Min = FVector2D(RotatedNewBounds.Min);
							SideBounds.Max = FVector2D(RotatedNewBounds.Min.X + DiffMin.X, RotatedNewBounds.Max.Y);
						}
						else if (bIsSubstractive)
						{
							SideBounds.Min = FVector2D(RotatedOldBounds.Min);
							SideBounds.Max = FVector2D(RotatedOldBounds.Min.X - DiffMin.X, RotatedOldBounds.Max.Y);
						}

						// Rotate newly calculated side bounds to match to default orientation
						SideBounds.GetCenterAndExtents(SideBoundsCenter, SideBoundsExtents);

						SideBoundsCenter = AccumulatedRotationMatrix.TransformPoint(SideBoundsCenter - NewBoundsCenter) + NewBoundsCenter;
						SideBoundsExtents = AccumulatedRotationMatrix.TransformPoint(SideBoundsExtents).GetAbs();

						SideBounds.Min = (SideBoundsCenter - SideBoundsExtents);
						SideBounds.Max = (SideBoundsCenter + SideBoundsExtents);

						// Add ready side bounds to collection
						if (bIsAdditive)
						{
							AdditiveSides.Emplace(SideBounds.Min.IntPoint(), SideBounds.Max.IntPoint()-1);
						}
						else if (bIsSubstractive) 
						{
							SubtractiveSides.Emplace(SideBounds.Min.IntPoint(), SideBounds.Max.IntPoint()-1);
						}
					}

					// Rotate new bounds
					RotatedNewBoundsExtents = RotationMatrix.TransformPoint(RotatedNewBoundsExtents).GetAbs();

					RotatedNewBounds.Min = (NewBoundsCenter - RotatedNewBoundsExtents);
					RotatedNewBounds.Max = (NewBoundsCenter + RotatedNewBoundsExtents);

					// Rotate old bounds around new bounds center
					RotatedOldBoundsCenter = RotationMatrix.TransformPoint(RotatedOldBoundsCenter - NewBoundsCenter) + NewBoundsCenter;
					RotatedOldBoundsExtents = RotationMatrix.TransformPoint(RotatedOldBoundsExtents).GetAbs();

					RotatedOldBounds.Min = (RotatedOldBoundsCenter - RotatedOldBoundsExtents);
					RotatedOldBounds.Max = (RotatedOldBoundsCenter + RotatedOldBoundsExtents);

					AccumulatedRotationMatrix = AccumulatedRotationMatrix.Concatenate(RotationMatrix.Inverse());
				}
				while (!AccumulatedRotationMatrix.IsIdentity());

				// Define removed & added cell containers
				FMineGridMapChanges GridMapChanges;
				GridMapChanges.NewGridDimensions = MapAreaSize;

				// Start with subtractive bounds first
				for (const auto &SubtractiveBounds : SubtractiveSides)
				{
					if (AdditiveSides.Contains(SubtractiveBounds))
					{
						continue;
					}

					for (int32 Y = SubtractiveBounds.Min.Y; Y <= SubtractiveBounds.Max.Y; ++Y)
					{
						for (int32 X = SubtractiveBounds.Min.X; X <= SubtractiveBounds.Max.X; ++X)
						{
							FIntPoint Coords(X, Y);

							GridMapChanges.RemovedGridMapCells.AddUnique(Coords);
						}
					}
				}

				// Then with additive bounds
				for (const auto &AdditiveBounds : AdditiveSides)
				{
					if (SubtractiveSides.Contains(AdditiveBounds))
					{
						continue;
					}

					for (int32 Y = AdditiveBounds.Min.Y; Y <= AdditiveBounds.Max.Y; ++Y)
					{
						for (int32 X = AdditiveBounds.Min.X; X <= AdditiveBounds.Max.X; ++X)
						{
							FIntPoint Coords(X, Y);

							if (!MineGridMap.Cells.Contains(Coords))
							{
								continue;
							}
							EMineGridMapCell CellValue = MineGridMap.Cells[Coords];

							if (!GridMapChanges.AddedGridMapCellCoords.Contains(Coords))
							{
								GridMapChanges.AddedGridMapCellCoords.Add(Coords);
								GridMapChanges.AddedGridMapCellValues.Add(CellValue);
							}
						}
					}
				}

				MineGridMapArea.GridDimensions = MapAreaSize;
				MineGridMapArea.StartCoords = NewBounds.Min;
				MineGridMapArea.EndCoords = NewBounds.Max;

				// Finally apply changes
				ApplyAddedRemovedGridCells(GridMapChanges);

				// Remember player coords used to process
				PrevPlayerRelativeGridCoords = PawnRelativeGridCoords;
			}
		}
	}
}

void AMinesweeperPlayerControllerBase::ClearAllGridCells()
{
	FMineGridMapChanges GridMapChanges;
	GridMapChanges.NewGridDimensions = FIntPoint::ZeroValue;

	for (auto CellToRemove : MineGridMapArea.Cells)
	{
		const FIntPoint CoordsToRemoveAt = CellToRemove.Key;

		GridMapChanges.RemovedGridMapCells.AddUnique(CoordsToRemoveAt);
	}

	ApplyAddedRemovedGridCells(GridMapChanges);
}

void AMinesweeperPlayerControllerBase::UpdateGridMapAreaCellValues(const FMineGridMap& MineGridMap)
{
	FMineGridMapCellUpdates CellsUpdate;

	for (TPair<FIntPoint, EMineGridMapCell>& CoordsCellEntry : MineGridMapArea.Cells)
	{
		const FIntPoint Coords = CoordsCellEntry.Key;
		const EMineGridMapCell NewCellValue = MineGridMap.Cells[Coords];

		if (CoordsCellEntry.Value != NewCellValue)
		{
			CellsUpdate.UpdatedGridMapCellCoords.Emplace(Coords);
			CellsUpdate.UpdatedGridMapCellValues.Emplace(NewCellValue);
		}
	}

	if (CellsUpdate.UpdatedGridMapCellCoords.Num() > 0)
	{
		ApplyUpdatedGridCellValues(CellsUpdate);
	}
}

void AMinesweeperPlayerControllerBase::HandleOnTriggeredCoords(const FIntPoint& EnteredCoords)
{
	OnPlayerTriggeredCoords.Broadcast(EnteredCoords);
}

void AMinesweeperPlayerControllerBase::SelectNewGame_Implementation(const uint8 MapSize)
{
	OnPlayerNewGame.Broadcast(MapSize);
}

void AMinesweeperPlayerControllerBase::ApplyAddedRemovedGridCells_Implementation(const FMineGridMapChanges& GridMapChanges)
{
	// 
	// 1. Update area of map according to new changes
	// 

	for (const FIntPoint& RemovedCellCoords : GridMapChanges.RemovedGridMapCells)
	{
		MineGridMapArea.Cells.Remove(RemovedCellCoords);
	}

	auto AddedCoordsIt = GridMapChanges.AddedGridMapCellCoords.CreateConstIterator();
	auto AddedValuesIt = GridMapChanges.AddedGridMapCellValues.CreateConstIterator();
	for (AddedCoordsIt, AddedValuesIt; AddedCoordsIt && AddedValuesIt; ++AddedCoordsIt, ++AddedValuesIt)
	{
		MineGridMapArea.Cells.Emplace(*AddedCoordsIt, *AddedValuesIt);
	}

	// 
	// 2. Update visible representation of area
	//
	if (MineGridActor)
	{
		MineGridActor->AddOrRemoveGridCells(GridMapChanges);
	}
}

void AMinesweeperPlayerControllerBase::ApplyUpdatedGridCellValues_Implementation(const FMineGridMapCellUpdates& UpdatedCells)
{
	auto UpdatedCoordsIt = UpdatedCells.UpdatedGridMapCellCoords.CreateConstIterator();
	auto UpdatedValuesIt = UpdatedCells.UpdatedGridMapCellValues.CreateConstIterator();
	for (UpdatedCoordsIt, UpdatedValuesIt; UpdatedCoordsIt && UpdatedValuesIt; ++UpdatedCoordsIt, ++UpdatedValuesIt)
	{
		MineGridMapArea.Cells[*UpdatedCoordsIt] = *UpdatedValuesIt;
	}

	if (MineGridActor)
	{
		MineGridActor->UpdateCellValues(UpdatedCells);
	}
}

AMineGridBase* AMinesweeperPlayerControllerBase::FindMineGridActor()
{
	UWorld* World = GetWorld();
	for (TActorIterator<AMineGridBase> ActorIt(World, MineGridClass); ActorIt; ++ActorIt)
	{
		AMineGridBase* MineGrid = *ActorIt;
		if (MineGrid)
		{
			return MineGrid;
		}
	}
	return nullptr;
}

FIntPoint AMinesweeperPlayerControllerBase::GetPawnRelativeLocationOfGrid(APawn* PlayerPawn, AMineGridBase* MineGrid)
{
	// Get relative grid coords of player pawn
	const FVector PawnAbsoluteLocation = PlayerPawn->GetActorLocation();
	const FVector MineGridLocation = MineGrid->GetActorLocation();
	const float CellSize = MineGrid->GetCellSize();

	const FVector PawnRelativeLocation = PawnAbsoluteLocation - MineGridLocation;

	int32 PawnRelativeLocationX = FMath::TruncToInt(PawnRelativeLocation.X);
	int32 PawnRelativeLocationY = FMath::TruncToInt(PawnRelativeLocation.Y);

	FIntPoint PawnRelativeGridCoords = FIntPoint(PawnRelativeLocationX, PawnRelativeLocationY) / CellSize;

	return PawnRelativeGridCoords;
}

void AMinesweeperPlayerControllerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMinesweeperPlayerControllerBase, bIsLobbyLeader);
}
