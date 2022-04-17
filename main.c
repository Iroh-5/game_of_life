#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#define LOG(...)\
    fprintf(stderr, "Function: %s, Line: %d\nMessage: ", __func__, __LINE__);\
    fprintf(stderr, __VA_ARGS__);\
    fputc('\n', stderr);\

#define LOG_ERROR(...)\
    fprintf(stderr, "\033[0;31m" "[ERROR] ");\
    LOG(__VA_ARGS__);\
    fprintf(stderr, "\033[0m");\

#define LOG_INFO(...)\
    fprintf(stderr, "\033[0;32m" "[INFO] ");\
    LOG(__VA_ARGS__);\
    fprintf(stderr, "\033[0m");\

enum
{
    CELL_CHAR         = ' ',
    
    CELL_NUM          = 1,
    EMPTY_SPACE_NUM   = 0,
    
    FILLED_CELL_COLOR = 1,
    EMPTY_SPACE_COLOR = 2,

    STATUS_BAR_WIDTH  = 60,
    LEFT_MSG_LEN      = 25,
    MID_MSG_LEN       = 25,
    RIGHT_MSG_LEN     = 25,

    PAUSE_MODE_KEY    = 'p',
    MANUAL_MODE_KEY   = 'm',
    EXIT_MMODE_KEY    = 'x',
    FASTER_KEY        = 'f',
    SLOWER_KEY        = 's',
    EXIT_KEY          = 'q',
    NEXTG_KEY         = 'n',
};

typedef enum 
{
    AUTO,
    PAUSE,
    MANUAL
} mode;

typedef struct
{
    int width;
    int height;
    int delay;
    size_t pop_count;
    size_t alive;

    WINDOW* mainwin;
    WINDOW* status_bar;

    int* screen_buffer;
} game_t;

bool game_init(game_t* game, int width, int height)
{
    if (width < STATUS_BAR_WIDTH)
    {
	LOG_ERROR("Your terminal is too narrow");
	goto NORES;
    }

    game->width = width;
    game->height = height - 2;
    game->delay = 100;
    game->pop_count = 0;
    game->alive = 0;

    game->mainwin = newwin(game->height, game->width, 0, 0);
    if (game->mainwin == NULL)
    {
	LOG_ERROR("Could not open a window");
	goto NORES;
    }

    game->status_bar = newwin(2, game->width, game->height + 1, 0);
    if (game->status_bar == NULL)
    {
	LOG_ERROR("Could not open a window");
	goto MAINWIN;
    }

    game->screen_buffer = malloc(sizeof(int) * game->width * game->height);
    if (game->screen_buffer == NULL)
    {
	LOG_ERROR("Memory allocation error");
	goto WINDOWS;
    }

    return true;

WINDOWS:
    delwin(game->status_bar);
MAINWIN: 
    delwin(game->mainwin);
NORES:
    return false;
}

void fill_buffer_randomly(game_t* game, int freq)
{
    // NOTE: maps from [1, 10] to [11, 2]
    const int prob = 12 - freq;

    for (int y = 0; y < game->height; ++y)
    {
	for (int x = 0; x < game->width; ++x)
	{
	    // NOTE: perhaphs better RNG should be used 
	    //       but rand() is sufficient at least for now
	    if (rand() % prob == 0)
		game->screen_buffer[y * game->width + x] = CELL_NUM;
	    else
		game->screen_buffer[y * game->width + x] = EMPTY_SPACE_NUM;
	}
    }
}

bool load_initial_state_from_file(game_t* game, const char* fname, int y, int x)
{
    bool status = true;

    FILE* file = fopen(fname, "r");
    if (file == NULL)
    {
	LOG_ERROR("Could not open a file \"%s\"", fname);
	status = false;
	goto NORES;
    }

    char line[1024];
    int width = 0;
    int height = 0;
    while (fgets(line, 1024, file))
    {
	height++;
	int line_size = strlen(line) - 1; // NOTE: -1 because fgets read newline character
	if (line_size > width)
	    width = line_size;
    }
    fseek(file, 0, SEEK_SET);

    if (y + height >= game->height || x + width >= game->width)
    {
	LOG_ERROR("Wrong coordinates. Figure doesn't fit into the screen");
	status = false;
	goto INFILE;
    }

    int ix = x;
    while (fgets(line, 1024, file))
    {
	char* p = line;
	while (*p)	
	{
	    game->screen_buffer[y * game->width + x] = ((*p - '0') == CELL_NUM);
	    x++;
	    p++;
	}
	x = ix;
	y++;
    }

INFILE:
    fclose(file);
NORES:
    return status;
}

void update_game_state(game_t* game)
{
    static const int dy[] = { 0, -1, -1, -1, 0, 1, 1, 1 };
    static const int dx[] = { -1, -1, 0, 1, 1, 1, 0, -1 };

    int tmp_buffer[game->height][game->width];
    for (int y = 0; y < game->height; ++y)
	for (int x = 0; x < game->width; ++x)
	    tmp_buffer[y][x] = EMPTY_SPACE_NUM;

    size_t alive = 0;
    for (int y = 0; y < game->height; ++y)
    {
	for (int x = 0; x < game->width; ++x)
	{
	    int ncount = 0;
	    for (int k = 0; k < 8; ++k)	
	    {
		int ny = y + dy[k];
		int nx = x + dx[k];
		
		if (ny < 0)      
		    ny = game->height - 1;
		if (ny >= game->height) 
		    ny = 0;
		if (nx < 0)      
		    nx = game->width - 1;
		if (nx >= game->width)  
		    nx = 0;

		if (game->screen_buffer[ny * game->width + nx] == CELL_NUM)
		    ncount++;
	    }

	    if (game->screen_buffer[y * game->width + x] == CELL_NUM) 
	    {
		if (ncount == 2 || ncount == 3)
		    tmp_buffer[y][x] = CELL_NUM;
	    }
	    else 
	    {
		if (ncount == 3)
		    tmp_buffer[y][x] = CELL_NUM;
	    }

	    if (tmp_buffer[y][x] == CELL_NUM)
		alive++;
	}
    }

    for (int y = 0; y < game->height; ++y)
	for (int x = 0; x < game->width; ++x)
	    game->screen_buffer[y * game->width + x] = tmp_buffer[y][x];

    game->pop_count++;
    game->alive = alive;
}

void draw_to_screen(game_t* game)
{
    for (int y = 0; y < game->height; ++y)
    {
	for (int x = 0; x < game->width; ++x)
	{
	    // NOTE: color here is unsigned to prevent compiler warnings
	    unsigned color;
	    if (game->screen_buffer[y * game->width + x] == CELL_NUM) 
		color = COLOR_PAIR(FILLED_CELL_COLOR);
	    else 
		color = COLOR_PAIR(EMPTY_SPACE_COLOR);

	    mvwaddch(game->mainwin, y, x, CELL_CHAR | color);
	}
    }

    mvwprintw(game->status_bar, 0, 0, "generation # %zu", game->pop_count);
    mvwprintw(game->status_bar, 0, (game->width - MID_MSG_LEN) / 2, "alive cells: %zu", game->alive);
    mvwprintw(game->status_bar, 0, game->width - RIGHT_MSG_LEN - 1, "speed: %f", 1000.0f / game->delay);

    wrefresh(game->mainwin);
    wrefresh(game->status_bar);
}

void game_destroy(game_t* game)
{
    free(game->screen_buffer);
    delwin(game->mainwin);
    delwin(game->status_bar);
}

int main(int argc, char** argv)
{
    int status = EXIT_SUCCESS;

    const char* const log_fname = "log";
    FILE* log_file = freopen(log_fname, "w", stderr);
    if (log_file == NULL)
    {
	LOG_ERROR("Could not open file \"%s\" for logging", log_fname);
	status = EXIT_FAILURE;
	goto NORES;
    }

    initscr();
    cbreak();
    noecho();
    curs_set(0);

    if (!has_colors())
    {
	LOG_ERROR("Your terminal doesn't support colors");
	status = EXIT_FAILURE;
	goto NCURSES_CLEAR;
    }

    use_default_colors();
    start_color();

    init_pair(FILLED_CELL_COLOR, COLOR_WHITE, COLOR_WHITE);
    init_pair(EMPTY_SPACE_COLOR, COLOR_WHITE, -1);

    game_t game;
    if (!game_init(&game, COLS, LINES))
    {
	status = EXIT_FAILURE;
	goto NCURSES_CLEAR;
    }

    mode mode = AUTO;

    timeout(game.delay);

    // NOTE: I didn't provide checks for errors in the given command line args
    //       so you should only run program with allowed number and type of args
    //       in allowed order
    if (argc <= 3)	
    {
	const int freq = (argc >= 2) ? atoi(argv[1]) : 5;
	if (freq < 1 || freq > 10)
	{
	    LOG_ERROR("Incorrect frequency factor. You should use values from 1 to 10");
	    goto GAME_CLEAR;
	}

	fill_buffer_randomly(&game, freq);

	if (argc == 3)
	    mode = (strcmp(argv[2], "-m") == 0) ? MANUAL : AUTO;
    }
    else if (argc <= 5)
    {
	if (!load_initial_state_from_file(&game, argv[1], atoi(argv[2]), atoi(argv[3])))
	{
	    status = EXIT_FAILURE;
	    goto GAME_CLEAR; 
	}

	if (argc == 3)
	    mode = (strcmp(argv[2], "-m") == 0) ? MANUAL : AUTO;
    }
    else
    {
	static const char* const usage = 
	    "./gof [frequency][-m]\n"
	    "./gof *file* *y* *x* [-m]";

	LOG_INFO("Wrong usage!\n%s", usage);

	status = EXIT_FAILURE;
	goto GAME_CLEAR;
    }

    while (true)
    {
	int ans = getch();

	switch (mode)
	{
	    case PAUSE:
	    {
		while (true)
		{
		    ans = getch();

		    if (ans == PAUSE_MODE_KEY)
			break;

		    if (ans == EXIT_KEY)
			goto EXIT;
		}
	    } break;

	    case MANUAL:
	    {
		while (true)
		{
		    ans = getch();

		    if (ans == EXIT_MMODE_KEY)
		    {
			mode = AUTO;
			break;
		    }

		    if (ans == EXIT_KEY)
			goto EXIT;

		    if (ans == NEXTG_KEY)
			break;
		}
	    } break;
	    
	    case AUTO:
	    {
		if (ans == FASTER_KEY)
		{
		    if ((game.delay -= 10) <= 0)
			game.delay = 10;
		    timeout(game.delay);
		}

		if (ans == SLOWER_KEY)
		{
		    if ((game.delay += 10) >= 1000)
			game.delay = 1000;
		    timeout(game.delay);
		}

		if (ans == MANUAL_MODE_KEY)
		    mode = MANUAL;

		if (ans == PAUSE_MODE_KEY)
		    mode = PAUSE;

		if (ans == EXIT_KEY)
		    goto EXIT;

	    } break;
	}

	update_game_state(&game);
	draw_to_screen(&game);
    }
    
EXIT:

GAME_CLEAR:
    game_destroy(&game);
NCURSES_CLEAR:
    endwin();
LOG_CLEAR:
    fclose(log_file);
NORES:
    return status;
}
