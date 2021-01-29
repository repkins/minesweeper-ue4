// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperBackendGameModeBase.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "MinesweeperGameStateBase.h"
#include "Minesweeper/Player/MinesweeperPlayerControllerBase.h"

AMinesweeperBackendGameModeBase::AMinesweeperBackendGameModeBase(): Super()
{
	MinesweeperBackend = CreateDefaultSubobject<UMinesweeperBackendComponent>(TEXT("MinesweeperBackendComponent"));

	// Setting defaults
	LastSentCoordsToOpen = AMinesweeperBackendGameModeBase::DefaultCellCoords;
	bTriggeredCellProcessing = false;

	// Setting up backend delegate handlers
	MinesweeperBackend->OnMapOk.AddDynamic(this, &AMinesweeperBackendGameModeBase::HandleOnMapOk);
	MinesweeperBackend->OnOpenOk.AddDynamic(this, &AMinesweeperBackendGameModeBase::HandleOnOpenOk);
	MinesweeperBackend->OnOpenGameOver.AddDynamic(this, &AMinesweeperBackendGameModeBase::HandleOnOpenGameOver);
	MinesweeperBackend->OnOpenYouWinDelegate.AddDynamic(this, &AMinesweeperBackendGameModeBase::HandleOnOpenYouWin);
	MinesweeperBackend->OnNewGameOk.AddDynamic(this, &AMinesweeperBackendGameModeBase::HandleOnNewGameOk);
}

void AMinesweeperBackendGameModeBase::BeginPlay()
{
	Super::BeginPlay();
}

void AMinesweeperBackendGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AMinesweeperPlayerControllerBase* NewMinesweeperPlayer = Cast<AMinesweeperPlayerControllerBase>(NewPlayer))
	{
		NewMinesweeperPlayer->OnPlayerNewGame.AddDynamic(this, &AMinesweeperBackendGameModeBase::HandleOnPlayerNewGame);
		NewMinesweeperPlayer->OnPlayerTriggeredCoords.AddDynamic(this, &AMinesweeperBackendGameModeBase::HandleOnPlayerTriggeredCoords);
	}
}

void AMinesweeperBackendGameModeBase::HandleOnNewGameOk()
{
}

void AMinesweeperBackendGameModeBase::HandleOnMapOk(const FMineGridMap& NewMineGridMap)
{
	MineGridMapVersion += 1;
}

void AMinesweeperBackendGameModeBase::HandleOnOpenOk()
{
	HandleOnOpen();
}

void AMinesweeperBackendGameModeBase::HandleOnOpenGameOver()
{
	bIsGameOver = true;

	if (AMinesweeperGameStateBase* MinesweeperState = GetGameState<AMinesweeperGameStateBase>())
	{
		MinesweeperState->SetExplodedCell(LastSentCoordsToOpen);
	}

	for (FConstPlayerControllerIterator PlayerIt = GetWorld()->GetPlayerControllerIterator(); PlayerIt; ++PlayerIt)
	{
		if (auto MinespeeperPlayer = Cast<AMinesweeperPlayerControllerBase>(*PlayerIt))
		{
			MinespeeperPlayer->NotifyGameOver();
		}
	}

	HandleOnOpen();
}

void AMinesweeperBackendGameModeBase::HandleOnOpenYouWin(const FString& LevelPassword)
{
	if (AMinesweeperGameStateBase* MinesweeperState = GetGameState<AMinesweeperGameStateBase>())
	{
		MinesweeperState->SetLevelPassword(LevelPassword);
	}

	for (FConstPlayerControllerIterator PlayerIt = GetWorld()->GetPlayerControllerIterator(); PlayerIt; ++PlayerIt)
	{
		if (auto MinespeeperPlayer = Cast<AMinesweeperPlayerControllerBase>(*PlayerIt))
		{
			MinespeeperPlayer->NotifyGameWin();
		}
	}

	HandleOnOpen();
}

void AMinesweeperBackendGameModeBase::HandleOnOpen()
{
	MinesweeperBackend->SendMapCommand();

	if (bIsGameOver)
	{
		RemainingCellsToProcess.Empty();
	}

	FIntPoint NextTriggeredCoords;
	if (RemainingCellsToProcess.Dequeue(NextTriggeredCoords))
	{
		LastSentCoordsToOpen = NextTriggeredCoords;
		MinesweeperBackend->SendOpenCellCommand(NextTriggeredCoords);
	}
	else
	{
		bTriggeredCellProcessing = false;
	}
}

void AMinesweeperBackendGameModeBase::HandleOnPlayerTriggeredCoords(const FIntPoint& EnteredCoords)
{
	if (!bIsGameOver)
	{
		if (!bTriggeredCellProcessing)
		{
			LastSentCoordsToOpen = EnteredCoords;

			MinesweeperBackend->SendOpenCellCommand(EnteredCoords);
			bTriggeredCellProcessing = true;
		}
		else
		{
			RemainingCellsToProcess.Enqueue(EnteredCoords);
		}
	}
}

void AMinesweeperBackendGameModeBase::HandleOnPlayerNewGame(const uint8 MapSize)
{
	Super::HandleOnPlayerNewGame(MapSize);
}

void AMinesweeperBackendGameModeBase::GenerateNewMap(const uint8 MapSize)
{
	MinesweeperBackend->SendNewGameCommand(MapSize);
	MinesweeperBackend->SendMapCommand();
}