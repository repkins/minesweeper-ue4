// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MinesweeperGameStateBase.generated.h"

UCLASS()
class MINESWEEPER_API AMinesweeperGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	AMinesweeperGameStateBase();

	UFUNCTION(BlueprintCallable)
	FString GetLevelPassword();

	UFUNCTION(BlueprintCallable)
	bool HasExplodedCell();

	UFUNCTION(BlueprintCallable)
	FIntPoint GetExplodedCoords();

	void SetLevelPassword(const FString& NewLevelPassword);

	void SetExplodedCell(const FIntPoint& ExplodedCoords);
	void UnsetExplodedCell();

protected:

	FString LevelPassword;

	bool bHasExplodedCell;
	FIntPoint ExplodedCoords;
};
