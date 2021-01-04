// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperBackendComponent.h"

#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Misc/DefaultValueHelper.h"

#include "Engine.h"
#include "Modules/ModuleManager.h"

// Sets default values for this component's properties
UMinesweeperBackendComponent::UMinesweeperBackendComponent()
{
	// Disable TickComponent(...) invoking
	PrimaryComponentTick.bCanEverTick = false;

	bWantsInitializeComponent = true;
}

void UMinesweeperBackendComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		FModuleManager::Get().LoadModule("WebSockets");
	}

	WebSocket = FWebSocketsModule::Get().CreateWebSocket(ServerEndpointUrl);

	WebSocket->OnConnected().AddLambda([]() -> void {
		UE_LOG(LogTemp, Display, TEXT("Connection to Minesweeper game server successful."));
	});

	WebSocket->OnConnectionError().AddLambda([](const FString& ErrMsg) -> void {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Could not open connection to Minesweeper game server. Check output log."));

		UE_LOG(LogTemp, Error, TEXT("Connection error to Minesweeper game server. Reason: %s"), *ErrMsg);
	});

	WebSocket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean) -> void {
		UE_LOG(LogTemp, Display, TEXT("Connection closed to Minesweeper game server. StatusCode: %i, Reason: %s"), StatusCode, *Reason);
	});

	WebSocket->OnMessageSent().AddUObject(this, &UMinesweeperBackendComponent::OnCommandSent);
	WebSocket->OnRawMessage().AddUObject(this, &UMinesweeperBackendComponent::HandleReceivingRawMessage);

	WebSocket->Connect();
}

void UMinesweeperBackendComponent::UninitializeComponent()
{
	Super::UninitializeComponent();

	WebSocket->Close();
}


void UMinesweeperBackendComponent::HandleReceivingRawMessage(const void* Data, SIZE_T Size, SIZE_T BytesRemaining)
{
	const ANSICHAR* DataBytes = static_cast<const ANSICHAR*>(Data);

	// Proceed with encoding conversion
	FUTF8ToTCHAR Convert(DataBytes, Size);
	const TCHAR* ConvertedCharsPtr = Convert.Get();
	int32 ConvertedCharsLength = Convert.Length();

	// Check if has incomplete char bytes in buffer, then count them, combine with previous, 
	// pull and convert them and append to string buffer. Also advance converted chars ptr until meets 
	// valid character, respectively decrementing num of those converted chars.
	if (IncompleteCharBytesBuffer.Num() > 0)
	{
		SIZE_T FirstBogusCharCount = 0;
		while (*ConvertedCharsPtr == TEXT(UNICODE_BOGUS_CHAR_CODEPOINT))
		{
			FirstBogusCharCount++;

			ConvertedCharsPtr++;
			ConvertedCharsLength--;
		}
		UE_LOG(LogTemp, Warning, TEXT("First bogus chars. (%u chars)"), FirstBogusCharCount);

		// Concat separated bytes between two message fragments and convert
		IncompleteCharBytesBuffer.Append(DataBytes, FirstBogusCharCount);

		const ANSICHAR* ConcatenatedBytes = IncompleteCharBytesBuffer.GetData();
		int32 ConcatenatedNum = IncompleteCharBytesBuffer.Num();

		if (FirstBogusCharCount == 2)
		{
			ConcatenatedBytes++;
			ConcatenatedNum--;
		}

		FUTF8ToTCHAR ConvertPart(ConcatenatedBytes, ConcatenatedNum);
		IncompleteCharBytesBuffer.Reset();

		ReceivingStringBuffer.Append(ConvertPart.Get(), ConvertPart.Length());
	}

	if (BytesRemaining > 0) 
	{
		// Locate ending bogus chars if there will come next message fragment
		const TCHAR* LastCharsPtr = &ConvertedCharsPtr[ConvertedCharsLength - 1];

		SIZE_T LastBogusCharCount = 0;
		while (*LastCharsPtr == TEXT(UNICODE_BOGUS_CHAR_CODEPOINT))
		{
			LastBogusCharCount++;
			LastCharsPtr--;
		}

		// If has ending bogus chars then add related original char bytes to buffer so they will be 
		// concatenated with remaining char bytes from next message fragment.
		if (LastBogusCharCount > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Last bogus chars detected. (%u chars)"), LastBogusCharCount);

			ConvertedCharsLength -= LastBogusCharCount;

			// Because of UE4's encoding convert function not including last bogus character, needs to
			// take additional byte from the end of data for use in remaining message fragment if necessary, 
			// assuming that separated bytes of UTF-8 character is "White Box" char, which is 3 bytes long.
			LastBogusCharCount++;

			IncompleteCharBytesBuffer.Append(&DataBytes[Size - LastBogusCharCount], LastBogusCharCount);
		}
	}

	// Add converted chars to string buffer 
	ReceivingStringBuffer.Append(ConvertedCharsPtr, ConvertedCharsLength);

	// Then pull from string buffer and handle them to handler method if this is the last message fragment
	if (BytesRemaining == 0)
	{
		HandleReceivingMessage(ReceivingStringBuffer);
		ReceivingStringBuffer.Empty();
	}
}

void UMinesweeperBackendComponent::SendNewGameCommand(const uint8& MapSizeId)
{
	checkf(FMath::IsWithinInclusive(MapSizeId, (uint8)1, (uint8)4), TEXT("Unsupported map size identifier: %i."), MapSizeId);

	FString CmdString = FString::Printf(TEXT("%s %d"), *CommandNameNew, MapSizeId);
	WebSocket->Send(CmdString);
}

void UMinesweeperBackendComponent::SendMapCommand()
{
	FString CmdString = FString::Printf(TEXT("%s"), *CommandNameMap);
	WebSocket->Send(CmdString);
}

void UMinesweeperBackendComponent::SendOpenCellCommand(const FIntPoint& CellCoords)
{
	FString CmdString = FString::Printf(TEXT("%s %i %i"), *CommandNameOpen, CellCoords.X, CellCoords.Y);
	WebSocket->Send(CmdString);
}

void UMinesweeperBackendComponent::HandleReceivingMessage(const FString& Message)
{
	TArray<FString> MessageLines;
	Message.ParseIntoArrayLines(MessageLines);

	// Create iterator to walk lines manually
	TArray<FString>::TConstIterator MessageLinesIterator = MessageLines.CreateConstIterator();

	// Get first line string containing command name and status and then advance iterator
	const FString FirstLineWithCmdName = *MessageLinesIterator++;

	UE_LOG(LogTemp, Verbose, TEXT("Command response received from Minesweeper game server: %s"), *FirstLineWithCmdName);

	FString CmdNameStr;
	FString StatusStr;
	FirstLineWithCmdName.Split(TEXT(":"), &CmdNameStr, &StatusStr);

	StatusStr.TrimStartInline();

	bool bNotHandled = false;

	// Handle "new" command response
	if (CmdNameStr == CommandNameNew)
	{
		if (StatusStr == ResponseStatusOk)
		{
			OnNewGameOk.Broadcast();
		}
		else {
			bNotHandled = true;
		}
	}
	// Handle "map" command response
	else if (CmdNameStr == CommandNameMap)
	{
		if (StatusStr == ResponseStatusEmpty)
		{
			// Parse grid data payload in response message and store it to field
			ParseMapGridLines(MessageLinesIterator, LastReceivedMineGridMap);
			
			OnMapOk.Broadcast(LastReceivedMineGridMap);
		}
		else if (StatusStr == ResponseStatusNotStarted)
		{
			OnMapNotStarted.Broadcast();
		}
		else
		{
			bNotHandled = true;
		}
	}
	// Handle "open" command response
	else if (CmdNameStr == CommandNameOpen)
	{
		if (StatusStr == ResponseStatusOk)
		{
			OnOpenOk.Broadcast();
		}
		else if (StatusStr == ResponseStatusNotStarted)
		{
			OnOpenNotStarted.Broadcast();
		}
		else if (StatusStr == ResponseStatusYouLose)
		{
			OnOpenGameOver.Broadcast();
		}
		else if (StatusStr == ResponseStatusOutOfBounds)
		{
			OnOpenOutOfBounds.Broadcast();
		}
		else if (StatusStr.StartsWith(ResponseStatusStartingWithYouWin))
		{
			FString LevelPassword,
							Description;

			StatusStr.Split(TEXT(":"), &Description, &LevelPassword);
			LevelPassword.TrimStartInline();

			OnOpenYouWinDelegate.Broadcast(LevelPassword);
		}
		else
		{
			bNotHandled = true;
		}
	}
	else
	{
		bNotHandled = true;
	}

	if (bNotHandled)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unsupported minesweeper server response: %s "), *FirstLineWithCmdName);
	}
}

void UMinesweeperBackendComponent::ParseMapGridLines(TArray<FString>::TConstIterator& MessageLinesIterator, FMineGridMap& OutMineGridMap)
{
	TMap<FIntPoint, EMineGridMapCell>& OutGridCells = OutMineGridMap.Cells;

	OutMineGridMap.StartCoords = FIntPoint(0, 0);
	int32 Y = OutMineGridMap.StartCoords.Y,
		X = OutMineGridMap.StartCoords.X;
	for (MessageLinesIterator; MessageLinesIterator; ++MessageLinesIterator)
	{
		const FString RowLineStr = *MessageLinesIterator;

		UE_LOG(LogTemp, VeryVerbose, TEXT("RowLineStr = \"%s\" (%i)"), *RowLineStr, RowLineStr.Len());

		X = 0;
		for (TCHAR InCellChar : RowLineStr)
		{
			// Parse cell value

			EMineGridMapCell OutCellValue;
			if (InCellChar == **MapGridCellUndiscovered)
			{
				OutCellValue = EMineGridMapCell::MGMC_Undiscovered;
			}
			else if (InCellChar == **MapGridCellRevealed)
			{
				OutCellValue = EMineGridMapCell::MGMC_Revealed;
			}
			else
			{
				// Everything else is a integer number to be parsed and casted to enum if valid

				int32 ParsedValue;
				if (FDefaultValueHelper::ParseInt(FString::Printf(TEXT("%c"), InCellChar), ParsedValue))
				{
					if (FMath::IsWithinInclusive(ParsedValue, 0, 8))
					{
						OutCellValue = (EMineGridMapCell)ParsedValue;
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Unsupported cell value: %i"), ParsedValue);
					}
				} 
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Unsupported cell value: %c"), InCellChar);
				}
			}

			OutGridCells.Emplace(FIntPoint(X, Y), OutCellValue);

			X++;
		}

		Y++;
	}

	// Setting up grid dimensions, assuming that incoming map has equal count of columns and rows
	OutMineGridMap.GridDimensions.X = X;
	OutMineGridMap.GridDimensions.Y = Y;

	OutMineGridMap.EndCoords.X = X;
	OutMineGridMap.EndCoords.Y = Y;

	UE_LOG(LogTemp, Verbose, TEXT("GridDimentions: %s"), *OutMineGridMap.GridDimensions.ToString());
}

void UMinesweeperBackendComponent::OnCommandSent(const FString& CommandString)
{
	UE_LOG(LogTemp, Verbose, TEXT("Command sent to Minesweeper game server: %s"), *CommandString);
}
