# Conway-s-game-of-life-in-C-ncurses-
The classic conway's game of life, written in a threaded ncurses program in c. The game map is connected both in the X and Y axis so essentaily it is on a sphere ! enjoy


To compile : gcc -o Conway Conway'sGameOfLife.c -lpthread -lncurses<br><br>
To run : ./Conway<br>
Additional arguments are ./Conway <STARTING_ENTITIES> <SEED> <MAP_WIDTH> <MAP_HEIGHT>
