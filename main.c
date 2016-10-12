#include <SDL2/SDL.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

// Grid constants

#define GRID_WIDTH			80					// Can only be in multiples of 8
#define GRID_HEIGHT			80
#define GRID_WIDTH_MI		(GRID_WIDTH - 1)	// MI = Maximum Index
#define GRID_HEIGHT_MI		(GRID_HEIGHT - 1)	// This is just GRID_WIDTH/HEIGHT - 1
#define GRID_BYTES_PER_ROW	(GRID_WIDTH / 2)	// This is just GRID_WIDTH / 2
#define GRID_SIZE			(GRID_WIDTH * GRID_HEIGHT)

// Grid bit operations

#define GRID_BYTE(ptr, x, y)	((ptr)[(y) * GRID_BYTES_PER_ROW + (x) / 8])
#define GRID_GET_BIT(ptr, x, y)	GET_BIT(GRID_BYTE((ptr), (x), (y)), (x) % 8)
#define GRID_SET_BIT(ptr, x, y)	SET_BIT(GRID_BYTE((ptr), (x), (y)), (x) % 8)
#define GRID_CLR_BIT(ptr, x, y)	CLR_BIT(GRID_BYTE((ptr), (x), (y)), (x) % 8)

// Grid pixel operations

#define GRID_FROM_PIXEL_X(x)	((x) / GRID_PIXEL_WIDTH)
#define GRID_FROM_PIXEL_Y(y)	((y) / GRID_PIXEL_HEIGHT)

#define GRID_PIXEL_WIDTH	10
#define GRID_PIXEL_HEIGHT	10

// Bit operations

#define GET_BIT(byte, loc)	(((byte) >> (7 - (loc))) & 1)
#define SET_BIT(byte, loc)	((byte) |= 1 << (7 - (loc)))
#define CLR_BIT(byte, loc)	((byte) &= ~(1 << (7 - (loc))))

// Framerate limiter

#define FRAME_WAIT	0

// Exit codes

#define EXIT_OK		0
#define EXIT_ERR	-1

// Memory

#define PTRSIZE	sizeof(void*)
#define MEMSIZE (\
	GRID_SIZE * 2 +	/* GRID, GRID1 */ \
	PTRSIZE * 4 +	/* C_GRID, N_GRID, WINDOW, SCREEN_SURFACE, BUFFER */ \
	6)				/* RUNNING, PAUSED, USER_SETTING, COLOR_R, COLOR_G, COLOR_B */

#define GRID			((uint8_t*)			memstart)
#define GRID1			((uint8_t*)			memstart + GRID_SIZE)					// Used to store new grid so old grid can be updated without affecting uncalculated areas
#define C_GRID			((uint8_t**)		memstart + GRID_SIZE*2)					// Current grid
#define N_GRID			((uint8_t**)		memstart + GRID_SIZE*2 + PTRSIZE)		// New grid

#define RUNNING			((uint8_t*)			memstart + GRID_SIZE*2 + PTRSIZE*2)		// If running is false, main loop exits
#define PAUSED			((uint8_t*)			memstart + GRID_SIZE*2 + PTRSIZE*2 + 1)	// If paused is true, grid doesn't update
#define USER_SETTING	((uint8_t*)			memstart + GRID_SIZE*2 + PTRSIZE*2 + 2)	// 0: Mouse not down, 1/2: Mouse down (1: Set, 2: Clear)

#define COLOR_R			((uint8_t*)			memstart + GRID_SIZE*2 + PTRSIZE*2 + 3)
#define COLOR_G			((uint8_t*)			memstart + GRID_SIZE*2 + PTRSIZE*2 + 4)
#define COLOR_B			((uint8_t*)			memstart + GRID_SIZE*2 + PTRSIZE*2 + 5)

#define WINDOW			((SDL_Window**)		memstart + GRID_SIZE*2 + PTRSIZE*2 + 6)
#define SCREEN_SURFACE	((SDL_Surface**)	memstart + GRID_SIZE*2 + PTRSIZE*3 + 6)

void* memstart;

int main(int argc, char** argv) {
	
	// ----------
	// INITIALIZATION
	// ----------
	
	// Initialize memory

	memstart = malloc(MEMSIZE);
	if (!memstart) {
		puts("Unable to allocate memory");
		return EXIT_ERR;
	}
	
	// Initialize SDL
	// There's this weird bug where SDL_Init would corrupt N_GRID
	// no matter what I tried. So now it inits first, then the variables.
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
		return EXIT_ERR;
	}
	
	// Initialize program

	srand(time(NULL));
	
	for (uint8_t xb = 0; xb < GRID_BYTES_PER_ROW; xb++) {
		for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
			GRID[y * GRID_BYTES_PER_ROW + xb] = 0;
			GRID1[y * GRID_BYTES_PER_ROW + xb] = 0;
		}
	}
	
	*RUNNING = 1;
	*PAUSED = 1;
	*USER_SETTING = 0;
		
	*COLOR_R = 255;
	*COLOR_G = 255;
	*COLOR_B = 255;
	
	*C_GRID = GRID;
	*N_GRID = GRID1;
	
	// Create window

	*WINDOW = SDL_CreateWindow(
		"Game of Life", 
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED, 
		GRID_PIXEL_WIDTH * GRID_WIDTH, 
		GRID_PIXEL_HEIGHT * GRID_HEIGHT, 
		SDL_WINDOW_SHOWN);
	if (!*WINDOW) {
		printf("Unable to create window: %s\n", SDL_GetError());
		return EXIT_ERR;
	}
	*SCREEN_SURFACE = SDL_GetWindowSurface(*WINDOW);
	
	// ----------
	// MAIN LOOP
	// ----------
		
	while (*RUNNING) {
				
		// ----------
		// HANDLE EVENTS
		// ----------
		
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			switch (e.type) {
				case SDL_QUIT:
					*RUNNING = 0;
					break;
					
				case SDL_MOUSEBUTTONDOWN:
					if (e.button.button == SDL_BUTTON_LEFT) {
						if (GRID_GET_BIT(*C_GRID, GRID_FROM_PIXEL_X(e.button.x), GRID_FROM_PIXEL_Y(e.button.y))) {
							GRID_CLR_BIT(*C_GRID, GRID_FROM_PIXEL_X(e.button.x), GRID_FROM_PIXEL_Y(e.button.y));
							*USER_SETTING = 2;
						}
						else {
							GRID_SET_BIT(*C_GRID, GRID_FROM_PIXEL_X(e.button.x), GRID_FROM_PIXEL_Y(e.button.y));
							*USER_SETTING = 1;
						}
					}
					break;
					
				case SDL_MOUSEBUTTONUP:
					if (e.button.button == SDL_BUTTON_LEFT)
						*USER_SETTING = 0;
					break;
					
				case SDL_MOUSEMOTION:
					if (*USER_SETTING == 1) GRID_SET_BIT(*C_GRID, GRID_FROM_PIXEL_X(e.motion.x), GRID_FROM_PIXEL_Y(e.motion.y));
					else if (*USER_SETTING == 2) GRID_CLR_BIT(*C_GRID, GRID_FROM_PIXEL_X(e.motion.x), GRID_FROM_PIXEL_Y(e.motion.y));
					break;
					
				case SDL_KEYDOWN:
					switch (e.key.keysym.sym) {
						case SDLK_SPACE:
							*PAUSED = *PAUSED ? 0 : 1;
							break;
							
						case SDLK_ESCAPE:
							// Clear grid
							for (uint8_t x = 0; x < GRID_WIDTH; x++)
								for (uint8_t y = 0; y < GRID_HEIGHT; y++)
									GRID_CLR_BIT(*C_GRID, x, y);
							break;
							
						case SDLK_c:
							*COLOR_R = rand() % 256;
							*COLOR_G = rand() % 256;
							*COLOR_B = rand() % 256;
							break;
							
						case SDLK_v:
							*COLOR_R = 255;
							*COLOR_G = 255;
							*COLOR_B = 255;
							break;
						
						case SDLK_r:
							// Generate random grid
							for (uint8_t x = 0; x < GRID_WIDTH; x++)
								for (uint8_t y = 0; y < GRID_HEIGHT; y++)
									if (rand() < RAND_MAX / 2) GRID_SET_BIT(*C_GRID, x, y);
									else GRID_CLR_BIT(*C_GRID, x, y);
					}
					break;
			}
		}
		
		// ----------
		// RENDER FRAME
		// ----------
		
		// Clear screen
		
		if (*PAUSED) SDL_FillRect(*SCREEN_SURFACE, NULL, SDL_MapRGB((**SCREEN_SURFACE).format, 0, 0, 255));
		else SDL_FillRect(*SCREEN_SURFACE, NULL, SDL_MapRGB((**SCREEN_SURFACE).format, 0, 0, 0));
		
		// Render grid
				
		for (uint8_t x = 0; x < GRID_WIDTH; x++) {
			for (uint8_t y = 0; y < GRID_HEIGHT; y++) {				
				if (GRID_GET_BIT(*C_GRID, x, y)) {
					SDL_FillRect(
						*SCREEN_SURFACE, 
						&(SDL_Rect){
							x * GRID_PIXEL_WIDTH,
							y * GRID_PIXEL_HEIGHT,
							GRID_PIXEL_WIDTH,
							GRID_PIXEL_HEIGHT},
						SDL_MapRGB((**SCREEN_SURFACE).format, *COLOR_R, *COLOR_G, *COLOR_B));
				}
			}
		}
		
		// Update window surface with new render data
		
		SDL_UpdateWindowSurface(*WINDOW);
		
		// ----------
		// UPDATE FRAME
		// ----------
		
		if (!*PAUSED) {
			for (uint8_t x = 0; x < GRID_WIDTH; x++) {
				for (uint8_t y = 0; y < GRID_HEIGHT; y++) {	
					uint8_t aliveNeighbours = 0;
					if (x != 0 && 										GRID_GET_BIT(*C_GRID, x - 1, y)) aliveNeighbours++;
					if (x != 0 && y != 0 && 							GRID_GET_BIT(*C_GRID, x - 1, y - 1)) aliveNeighbours++;
					if (x != 0 && y != GRID_HEIGHT_MI && 				GRID_GET_BIT(*C_GRID, x - 1, y + 1)) aliveNeighbours++;
					if (y != 0 && 										GRID_GET_BIT(*C_GRID, x, y - 1)) aliveNeighbours++;
					if (y != GRID_HEIGHT_MI && 							GRID_GET_BIT(*C_GRID, x, y + 1)) aliveNeighbours++;
					if (x != GRID_WIDTH_MI && 							GRID_GET_BIT(*C_GRID, x + 1, y)) aliveNeighbours++;
					if (x != GRID_WIDTH_MI && y != 0 && 				GRID_GET_BIT(*C_GRID, x + 1, y - 1)) aliveNeighbours++;
					if (x != GRID_WIDTH_MI && y != GRID_HEIGHT_MI && 	GRID_GET_BIT(*C_GRID, x + 1, y + 1)) aliveNeighbours++;
											
					if (GRID_GET_BIT(*C_GRID, x, y)) {
						if (aliveNeighbours < 2 || aliveNeighbours > 3) GRID_CLR_BIT(*N_GRID, x, y);
						else GRID_SET_BIT(*N_GRID, x, y);
					}
					else {
						if (aliveNeighbours == 3) GRID_SET_BIT(*N_GRID, x, y);
						else GRID_CLR_BIT(*N_GRID, x, y);
					}
				}
			}
			
			// Swap grid pointers to let new grid become the new current grid			
			uint8_t* temp = *C_GRID;
			*C_GRID = *N_GRID;
			*N_GRID = temp;
		}
				
		// ----------
		// WAIT FRAME
		// ----------
		
		SDL_Delay(FRAME_WAIT);
	}
	
	// ----------
	// RELEASE AND EXIT
	// ----------
	
	SDL_FreeSurface(*SCREEN_SURFACE);
	SDL_DestroyWindow(*WINDOW);
	SDL_Quit();
	
	free(memstart);
		
	return EXIT_OK;
}
