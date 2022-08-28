# Minesweeper (multiplayer-enabled)

Developed with Unreal Engine 4

# Prerequisites

1. Unreal Engine 4.26
2. Windows 10 x64

# How-to play

1. Start the game
2. Open "New Game" menu by invoking "New Game" action (default is one of "ESC" and "Q" keyboard keys)
3. Select one of possible map sizes by clicking on one of "New #" buttons, or click "Resume" to close menu or click "Quit" to quit
4. Start opening cells by moving the character onto them using movement keyboard keys:
    - "W" for forward direction
    - "S" for backward direction
    - "A" for left direction
    - "D" for right direction
5. You can also make character to jump over cells, untriggering them, by using "Jump" action (default is "Space" keyboard key)
6. You can start new game at any time by using the same "New Game" action to open "new game" menu.
7. Continue opening cells until one of the following:
    - mines in untriggered cells became revealed and showing "You Win" HUD overlay;
    - or until last triggered cell become reddish color and revealed and showing "Game Over" HUD overlay
8. In either of two game ends you have the following options by clicking on related buttons:
    - start a new game (opens "new game" menu)
    - quit

# Solution Description
    
Implemented the following units:
1. `GameMode` class is resposible of match control. Communicates only with PlayerControllers and updates values in GameState about match state.
2. `MinesweeperPlayerController` class is responsible of controlling the character, contains action bindings to methods, communicates with `GameMode` for match and cell opening updates. Uses `MineGrid` actor as a representation container of cells, telling him what exactly to represent and listening of "cell triggering" events.
3. `MineGrid` class is a representational container of cells which defines the root location of cells in the game world. Spawns or destroys cell actors in response of player controller, responds to player controller about character triggering coordinates by listening to cells.
4. `MineGridCell` class is a physical representation of a single cell visible in the viewport and interactable with them by player. Contains trigger box to listen for character overlapping events to initiate cell triggering event broadcast chain (up through `MineGrid` and `PlayerController` until `GameMode`). Responds to cell value updates by it's `MineGrid` container actor to update visual representation of it.

Grid, Grid Cells, GameMode and PlayerController core logic are implemented natively in C++ with the possibility in blueprints to:
- change property values or references to assets;
- invoking native methods;
- listening to Blueprint events, overriding native implemention of them.
    
And last note. This solution architecture is networking ready.
