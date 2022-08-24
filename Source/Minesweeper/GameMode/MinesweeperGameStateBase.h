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
	int32 GetNumUndiscoveredClearCells();

	void SetNumUndiscoveredClearCells(const int32 NewNumUndiscoveredClearCells);

	UFUNCTION(BlueprintCallable)
	FString GetLevelPassword();

	void SetLevelPassword(const FString& NewLevelPassword);

protected:

	UPROPERTY(Replicated)
	int32 NumUndiscoveredClearCells;

	UPROPERTY(Replicated)
	FString LevelPassword;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
};
