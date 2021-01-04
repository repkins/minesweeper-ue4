// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IWebSocket.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Minesweeper/Includes/MineGridMap.h"

#include "MinesweeperBackendComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNewGameOkDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMapNotStartedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapOkDelegate, const FMineGridMap&, MineGridMap);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOpenOkDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOpenNotStartedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOpenGameOverDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOpenOutOfBoundsDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOpenYouWinDelegate, const FString&, LevelPassword);

/**
 * This component integrates with provided minesweeper game server 
 * using Unreal Engine WebSocket client solution.
 * Defines:
 *   - callable request methods to send to server
 *   - delegates to bind invoked on every corresponding incoming response messages 
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MINESWEEPER_API UMinesweeperBackendComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMinesweeperBackendComponent();

	/**
	 * Sends command telling game server to start new session with provided map size
	 * @param		MapSizeId		Determines which map size to use (1-4 supported)
	 */
	UFUNCTION(BlueprintCallable)
	void SendNewGameCommand(const uint8& MapSizeId);

	/**
	 * Sends command telling game server to send current map data
	 */
	UFUNCTION(BlueprintCallable)
	void SendMapCommand();

	/**
	 * Sends command telling game server to open cell at provided coordinates
	 * @param		CellVector	Coords to open cell at
	 */
	UFUNCTION(BlueprintCallable)
	void SendOpenCellCommand(const FIntPoint& CellCoords);

	// 
	// Define the following events exposed to Blueprints broadcasted from receiving message handle method
	// 

	UPROPERTY(BlueprintAssignable)
	FOnNewGameOkDelegate OnNewGameOk;

	UPROPERTY(BlueprintAssignable)
	FOnMapOkDelegate OnMapOk;
	UPROPERTY(BlueprintAssignable)
	FOnMapNotStartedDelegate OnMapNotStarted;

	UPROPERTY(BlueprintAssignable)
	FOnOpenOkDelegate OnOpenOk;
	UPROPERTY(BlueprintAssignable)
	FOnOpenNotStartedDelegate OnOpenNotStarted;
	UPROPERTY(BlueprintAssignable)
	FOnOpenOutOfBoundsDelegate OnOpenOutOfBounds;
	UPROPERTY(BlueprintAssignable)
	FOnOpenGameOverDelegate OnOpenGameOver;
	UPROPERTY(BlueprintAssignable)
	FOnOpenYouWinDelegate OnOpenYouWinDelegate;

	FORCEINLINE const FMineGridMap& GetLastReceivedMineGridMap() const { return LastReceivedMineGridMap; }

protected:

	const FString ServerEndpointUrl = TEXT("wss://hometask.eg1236.com/game1/");

	// Command names used in request and response messages
	const FString CommandNameNew = TEXT("new");
	const FString CommandNameMap = TEXT("map");
	const FString CommandNameOpen = TEXT("open");

	// Command response status messages receiving from server
	const FString ResponseStatusOk = TEXT("OK");
	const FString ResponseStatusNotStarted = TEXT("Not started");
	const FString ResponseStatusYouLose = TEXT("You lose");
	const FString ResponseStatusEmpty = TEXT("");
	const FString ResponseStatusOutOfBounds = TEXT("Out of bounds");
	const FString ResponseStatusStartingWithYouWin = TEXT("You win.");

	// "map" command response grid values incoming from server
	const FString MapGridCellUndiscovered = TEXT("\u25a1");	// which is "white square" char
	const FString MapGridCellRevealed = TEXT("*");

	// Shared ptr to WebSocket instance
	TSharedPtr<IWebSocket> WebSocket;

	// Receiving buffer of text message
	FString ReceivingStringBuffer;

	// Incomplete UTF-8 char bytes buffer to store between incomplete messages
	TArray<ANSICHAR> IncompleteCharBytesBuffer;

	// Current map received from the server
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	FMineGridMap LastReceivedMineGridMap;

	virtual void InitializeComponent() override;

	virtual void UninitializeComponent() override;

	void HandleReceivingRawMessage(const void* Data, SIZE_T Size, SIZE_T BytesRemaining);

	void HandleReceivingMessage(const FString& Message);
	void ParseMapGridLines(TArray<FString>::TConstIterator& GridLinesIterator, FMineGridMap& MineGridMap);
	void OnCommandSent(const FString& CommandString);

};
