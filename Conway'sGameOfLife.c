#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define STARTING_ENTITIES 60000

#define CHARACTER " O"

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h> // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

int wait_time = 50;

int MAP_WIDTH = 300;
int MAP_HEIGHT = 300;

int DISPLAY_WIDTH = 1;
int DISPLAY_HEIGHT = 1;

char *map_buffer;
int **board;
int **tmp_board;
int active_entities = 0;

int current_line;
int proccesed_lines = 0;
int coppied_lines = 0;

int running = 1;

int x = 0;
int y = 0;

pthread_mutex_t lock;
pthread_mutex_t pr_lock;

void adjust_window_size()
{
	map_buffer = (char *)malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT + DISPLAY_HEIGHT);
	memset(map_buffer, ' ', DISPLAY_WIDTH * DISPLAY_HEIGHT + DISPLAY_HEIGHT);
}

//Calculates the neighboors
int find_neighboors(int **array, int x, int y)
{
	int xl = (x - 1) * (x != 0) + (x == 0) * (MAP_WIDTH - 1);
	int xh = (x + 1) * (x != MAP_WIDTH - 1);
	int yl = (y - 1) * (y != 0) + (y == 0) * (MAP_HEIGHT - 1);
	int yh = (y + 1) * (y != MAP_HEIGHT - 1);
	return array[yl][x] + array[y][xl] + array[yh][x] + array[y][xh] + array[yl][xl] + array[yh][xh] + array[yl][xh] + array[yh][xl];
}

void sleep_ms(int milliseconds)
{ // cross-platform sleep function
#ifdef WIN32
	Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
#else
	if (milliseconds >= 1000)
		sleep(milliseconds / 1000);
	usleep((milliseconds % 1000) * 1000);
#endif
}

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
	struct timespec ts;
	int res;

	if (msec < 0)
	{
		errno = EINVAL;
		return -1;
	}

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;

	do
	{
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);

	return res;
}

void *print_screen_thread(void *vargp)
{
	WINDOW *gamepanel = newwin(10 - 2, 10 - 1, 0, 0);
	WINDOW *user_panel = newwin(1, 10 - 1, 10 - 1, 0);

	int steps = 0;
	
	int i, j;

	//Game loop
	while (running)
	{
		int buffer_counter = 0;
		//Check if screen dimenstions changed
		int max_y = 0;
		int max_x = 0;

		getmaxyx(stdscr, max_y, max_x);

		if (max_y != DISPLAY_HEIGHT + 1 || max_x != DISPLAY_WIDTH + 1)
		{
			DISPLAY_HEIGHT = max_y - 1;
			DISPLAY_WIDTH = max_x - 1;
			adjust_window_size();
			wresize(gamepanel, DISPLAY_HEIGHT, DISPLAY_WIDTH+1);
			wresize(user_panel, 1, DISPLAY_WIDTH);
			mvwin(gamepanel, 0, 0);
			mvwin(user_panel, DISPLAY_HEIGHT, 0);
		}

		proccesed_lines = 0;
		coppied_lines = 0;
		current_line = 0;
		active_entities = 0;

		while (proccesed_lines < MAP_HEIGHT)
		{
		}
		
		
		for (i = 0; i < DISPLAY_HEIGHT; i++){
			for (j = 0; j < DISPLAY_WIDTH; j++){
				int diplx = j+x;
				if (diplx >= MAP_WIDTH){
					diplx -= MAP_WIDTH;
				}
				else if (diplx < 0){
					diplx += MAP_WIDTH;
				}
				int diply = i+y;
				if (diply >= MAP_HEIGHT){
					diply -= MAP_HEIGHT;
				}
				else if (diply < 0){
					diply += MAP_HEIGHT;
				}
				mvwprintw(gamepanel, i, j, "%c", CHARACTER[tmp_board[diply][diplx]]);
			}
		}
		wrefresh(gamepanel);

		wclear(user_panel);
		mvwprintw(user_panel, 0, 0, "Steps : %d Active Entities : %d X: %d | Y: %d\tMovement: w,a,s,d  Faster/Slower time : o/p", steps++, active_entities, x, y);
		wrefresh(user_panel);

		sleep_ms(wait_time);

	}

	delwin(gamepanel);
    delwin(user_panel);

}

void *process_line(void *vargp)
{
	while (running)
	{
		int i;
		while (1)
		{
			pthread_mutex_lock(&lock);
			if (!running)
			{
				return NULL;
			}
			if (current_line < MAP_HEIGHT)
			{
				i = current_line++;
				pthread_mutex_unlock(&lock);
				break;
			}
			else if (coppied_lines < MAP_HEIGHT)
			{
				i = coppied_lines++;
				pthread_mutex_unlock(&lock);
				int j;
				for (j = 0; j < MAP_WIDTH; j++)
				{
					board[i][j] = tmp_board[i][j];
				}
				continue;
			}
			pthread_mutex_unlock(&lock);
		}

		int j;
		for (j = 0; j < MAP_WIDTH; j++)
		{
			//Get neighboors
			int neighboors = find_neighboors(board, j, i);
			//Apply the rules
			tmp_board[i][j] = ((neighboors == 2 | neighboors == 3) & board[i][j]) | (neighboors == 3 & !board[i][j]);
			//Alive
			if (tmp_board[i][j])
			{
				active_entities++;
			}
		}
		pthread_mutex_lock(&pr_lock);
		proccesed_lines++;
		pthread_mutex_unlock(&pr_lock);
	}
}

int main(int argc, char **argv)
{
	int s_ent = MAP_WIDTH*MAP_HEIGHT / 3;
	int seed = time(NULL);
	current_line = MAP_HEIGHT;

	//Handle arguments
	//<NAME> <STARTING_ENTITIES> <SEED>
	if (argc > 1)
	{
		if (strcmp(argv[1], "-d") != 0)
			s_ent = atoi(argv[1]);
	}
	if (argc > 2)
	{
		if (strcmp(argv[2], "-d") != 0)
			seed = atoi(argv[2]);
	}

	int THREAD_COUNT = 4;

	initscr();
	noecho();
	curs_set(FALSE);

	if (pthread_mutex_init(&lock, NULL) != 0 || pthread_mutex_init(&pr_lock, NULL) != 0)
	{
		printf("\n mutex init has failed\n");
		return 1;
	}

	char clear_screen[MAP_HEIGHT];
	int i, j, k;

	int activation_steps = 5000;
	int test_steps = 100;
	int threshold = 10;

	pthread_t tid[THREAD_COUNT + 1];

	//Gives random seed
	//srand(time(NULL));
	srand(seed);

	int steps = 0;
	board = (int **)malloc(MAP_HEIGHT * sizeof(int *));
	tmp_board = (int **)malloc(MAP_HEIGHT * sizeof(int *));
	//Empty the board
	for (i = 0; i < MAP_HEIGHT; i++)
	{
		board[i] = (int *)malloc(MAP_WIDTH * sizeof(int));
		tmp_board[i] = (int *)malloc(MAP_WIDTH * sizeof(int));
		for (j = 0; j < MAP_WIDTH; j++)
		{
			board[i][j] = 0;
			tmp_board[i][j] = 0;
		}
	}

	//Starting entitie
	//
	for (i = 0; i < s_ent; i++)
	{
			int x = rand() % MAP_WIDTH;
			int y = rand() % MAP_HEIGHT;
			board[y][x] = 1;
	}
	pthread_create(&tid[THREAD_COUNT], NULL, print_screen_thread, (void *)&i);
	for (i = 0; i < THREAD_COUNT; i++)
	{
		pthread_create(&tid[i], NULL, process_line, (void *)&i);
	}

	while (1)
	{
		timeout(-1);
		char ch = getch();
		if (ch == 'q')
		{
			break;
		}
		else if (ch == 'a')
		{

			if (--x == -1){
				x = MAP_WIDTH;
			}
		}
		else if (ch == 'd')
		{
			if (++x == MAP_WIDTH){
				x = 0;
			}
		}
		else if (ch == 'w')
		{
			if (--y == -1){
				y = MAP_HEIGHT;
			}
		}
		else if (ch == 's')
		{
			if (++y == MAP_WIDTH){
				y = 0;
			}
		}
		else if (ch == 'o')
		{
			wait_time -= 5;
			if (wait_time < 0){
				wait_time = 0;
			}
		}
		else if (ch == 'p')
		{
			wait_time += 5;
		}
	}
	running = 0;
	endwin();

	return 0;
}
