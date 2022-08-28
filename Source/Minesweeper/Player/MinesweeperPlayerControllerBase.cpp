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

	MapAreaMaxHalfSizeX = 7;
	MapAreaMaxHalfSizeY = 4;

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
				const FIntPoint MaxStartCoords = PawnRelativeGridCoords - MapAreaMaxHalfSize;
				const FIntPoint MaxEndCoords = PawnRelativeGridCoords + MapAreaMaxHalfSize + FIntPoint(1, 1);

				// Allocate max size of authoritive mines area
				const FIntPoint MapAreaMaxSize = MaxEndCoords - MaxStartCoords + FIntPoint(1, 1);
				MineGridMapArea.Cells.Reserve(MapAreaMaxSize.X * MapAreaMaxSize.Y);

				// Determine valid starting & endings coords of map "visible" area
				FIntPoint NewStartCoords;
				NewStartCoords.X = MaxStartCoords.X >= 0 ? MaxStartCoords.X : FMath::Min(0, MaxEndCoords.X + 1);
				NewStartCoords.Y = MaxStartCoords.Y >= 0 ? MaxStartCoords.Y : FMath::Min(0, MaxEndCoords.Y + 1);

				FIntPoint NewEndCoords;
				NewEndCoords.X = MaxEndCoords.X <= MapGridDimensions.X - 1 ? MaxEndCoords.X : FMath::Max(MapGridDimensions.X - 1, MaxStartCoords.X - 1);
				NewEndCoords.Y = MaxEndCoords.Y <= MapGridDimensions.Y - 1 ? MaxEndCoords.Y : FMath::Max(MapGridDimensions.Y - 1, MaxStartCoords.Y - 1);

				// Because start and end coords are inclusive, we need to increment by 1
				const FIntPoint MapAreaSize = NewEndCoords - NewStartCoords + FIntPoint(1, 1);

				const FIntPoint OldStartCoords = MineGridMapArea.StartCoords;
				const FIntPoint OldEndCoords = MineGridMapArea.EndCoords;

				TArray<TCoordsBoundsTuple> AdditiveBounds;
				TArray<TCoordsBoundsTuple> SubtractiveBounds;
				AdditiveBounds.Reserve(4);
				SubtractiveBounds.Reserve(4);

				// Top-left difference
				const FIntPoint LeftTopDiff = OldStartCoords - NewStartCoords;

				// Left side
				if (LeftTopDiff.X > 0)
				{
					// Additive
					const FIntPoint LeftStartCoords(NewStartCoords.X, NewStartCoords.Y);
					const FIntPoint LeftEndCoords(NewStartCoords.X + LeftTopDiff.X - 1, NewEndCoords.Y);
					AdditiveBounds.Emplace(LeftStartCoords, LeftEndCoords);
				}
				else if (LeftTopDiff.X < 0)
				{
					// Subtractive
					const FIntPoint LeftStartCoords(OldStartCoords.X, OldStartCoords.Y);
					const FIntPoint LeftEndCoords(OldStartCoords.X - LeftTopDiff.X - 1, OldEndCoords.Y);
					SubtractiveBounds.Emplace(LeftStartCoords, LeftEndCoords);
				}

				// Top side
				if (LeftTopDiff.Y > 0)
				{
					// Additive
					const FIntPoint TopStartCoords(NewStartCoords.X, NewStartCoords.Y);
					const FIntPoint TopEndCoords(NewEndCoords.X, NewStartCoords.Y + LeftTopDiff.Y - 1);
					AdditiveBounds.Emplace(TopStartCoords, TopEndCoords);
				}
				else if (LeftTopDiff.Y < 0)
				{
					// Subtractive
					const FIntPoint TopStartCoords(OldStartCoords.X, OldStartCoords.Y);
					const FIntPoint TopEndCoords(OldEndCoords.X, OldStartCoords.Y - LeftTopDiff.Y - 1);
					SubtractiveBounds.Emplace(TopStartCoords, TopEndCoords);
				}

				// Bottom-right difference
				const FIntPoint RightBottomDiff = NewEndCoords - OldEndCoords;

				// Right side
				if (RightBottomDiff.X > 0)
				{
					// Additive
					const FIntPoint RightStartCoords(NewEndCoords.X - RightBottomDiff.X + 1, NewStartCoords.Y);
					const FIntPoint RightEndCoords(NewEndCoords.X, NewEndCoords.Y);
					AdditiveBounds.Emplace(RightStartCoords, RightEndCoords);
				}
				else if (RightBottomDiff.X < 0)
				{
					// Subtractive
					const FIntPoint RightStartCoords(OldEndCoords.X + RightBottomDiff.X + 1, OldStartCoords.Y);
					const FIntPoint RightEndCoords(OldEndCoords.X, OldEndCoords.Y);
					SubtractiveBounds.Emplace(RightStartCoords, RightEndCoords);
				}

				// Bottom side
				if (RightBottomDiff.Y > 0)
				{
					// Additive
					const FIntPoint BottomStartCoords(NewStartCoords.X, NewEndCoords.Y - RightBottomDiff.Y + 1);
					const FIntPoint BottomEndCoords(NewEndCoords.X, NewEndCoords.Y);
					AdditiveBounds.Emplace(BottomStartCoords, BottomEndCoords);
				}
				else if (RightBottomDiff.Y < 0)
				{
					// Subtractive
					const FIntPoint BottomStartCoords(OldStartCoords.X, OldEndCoords.Y + RightBottomDiff.Y + 1);
					const FIntPoint BottomEndCoords(OldEndCoords.X, OldEndCoords.Y);
					SubtractiveBounds.Emplace(BottomStartCoords, BottomEndCoords);
				}

				// Define removed & added cell containers
				FMineGridMapChanges GridMapChanges;
				GridMapChanges.NewGridDimensions = MapAreaSize;

				// Start with subtractive bounds first
				for (const TCoordsBoundsTuple& SideBoundsTuple : SubtractiveBounds)
				{
					if (AdditiveBounds.Contains(SideBoundsTuple))
					{
						continue;
					}

					FIntPoint StartCoords = SideBoundsTuple.Get<0>(),
						EndCoords = SideBoundsTuple.Get<1>();

					for (int32 Y = StartCoords.Y; Y <= EndCoords.Y; ++Y)
					{
						for (int32 X = StartCoords.X; X <= EndCoords.X; ++X)
						{
							FIntPoint Coords(X, Y);

							GridMapChanges.RemovedGridMapCells.AddUnique(Coords);
						}
					}
				}

				// Then with additive bounds
				for (const TCoordsBoundsTuple& SideBoundsTuple : AdditiveBounds)
				{
					if (SubtractiveBounds.Contains(SideBoundsTuple))
					{
						continue;
					}

					FIntPoint StartCoords = SideBoundsTuple.Get<0>(),
						EndCoords = SideBoundsTuple.Get<1>();

					for (int32 Y = StartCoords.Y; Y <= EndCoords.Y; ++Y)
					{
						for (int32 X = StartCoords.X; X <= EndCoords.X; ++X)
						{
							FIntPoint Coords(X, Y);
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
				MineGridMapArea.StartCoords = NewStartCoords;
				MineGridMapArea.EndCoords = NewEndCoords;

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
