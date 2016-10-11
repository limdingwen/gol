#include <SDL2/SDL.h>
#include <time.h>
#include <stdlib.h>

#define GRID_WIDTH 80 // Can only be in multiples of 8
#define GRID_HEIGHT 80
#define GRID_WIDTH_MI (GRID_WIDTH - 1) // MI = Maximum Index
#define GRID_HEIGHT_MI (GRID_HEIGHT - 1) // This is just GRID_WIDTH/HEIGHT - 1
#define GRID_BYTES_PER_ROW (GRID_WIDTH / 2) // This is just GRID_WIDTH / 2
#define GRID_SIZE (GRID_WIDTH * GRID_HEIGHT)

#define GRID_BYTE(ptr, x, y) ((ptr)[(y) * GRID_BYTES_PER_ROW + (x) / 8])
#define GRID_GET_BIT(ptr, x, y) GET_BIT(GRID_BYTE((ptr), (x), (y)), (x) % 8)
#define GRID_SET_BIT(ptr, x, y) SET_BIT(GRID_BYTE((ptr), (x), (y)), (x) % 8)
#define GRID_CLR_BIT(ptr, x, y) CLR_BIT(GRID_BYTE((ptr), (x), (y)), (x) % 8)

#define GRID_FROM_PIXEL_X(x) ((x) / GRID_PIXEL_WIDTH)
#define GRID_FROM_PIXEL_Y(y) ((y) / GRID_PIXEL_HEIGHT)

#define GRID_PIXEL_WIDTH 10
#define GRID_PIXEL_HEIGHT 10

#define GET_BIT(byte, loc) (((byte) >> (7 - (loc))) & 1)
#define SET_BIT(byte, loc) ((byte) |= 1 << (7 - (loc)))
#define CLR_BIT(byte, loc) ((byte) &= ~(1 << (7 - (loc))))

#define FRAME_WAIT 0

#define EXIT_OK 0
#define EXIT_ERR -1

void* memory;
char* grid;
char* grid1; // Used to store new grid so old grid can be updated without affecting uncalculated areas
char** cGrid; // Current grid
char** nGrid; // New grid

char* running; // If running is false, main loop exits
char* paused; // If paused is true, grid doesn't update
char* userSetting; // 0: Mouse not down, 1/2: Mouse down (1: Set, 2: Clear)

char* colorR;
char* colorG;
char* colorB;

SDL_Window** window;
SDL_Surface** screenSurface;

int main(int argc, char** argv) {
	
	// ----------
	// INITIALIZATION
	// ----------
	
	// Initialize memory
		
	// Allocate the amount of memory we need
	memory = malloc(
		GRID_SIZE * 2 + // The grids of bytes
		sizeof(void*) * 4 + // The pointers
		6); // The bytes
		
	grid = (void*) memory;
	grid1 = (void*) grid + GRID_SIZE;
	cGrid = (void*) grid1 + GRID_SIZE;
	nGrid = (void*) cGrid + sizeof(void*);
	running = (void*) nGrid + sizeof(void*);
	paused = (void*) running + 1;
	userSetting = (void*) paused + 1;
	colorR = (void*) userSetting + 1;
	colorG = (void*) colorR + 1;
	colorB = (void*) colorG + 1;
	window = (void*) colorB + 1;
	screenSurface = (void*) window + sizeof(void*);
	
	// Initialize program

	srand(time(NULL));
	
	for (char xb = 0; xb < GRID_BYTES_PER_ROW; xb++) {
		for (char y = 0; y < GRID_HEIGHT; y++) {
			grid[y * GRID_BYTES_PER_ROW + xb] = 0;
			grid1[y * GRID_BYTES_PER_ROW + xb] = 0;
		}
	}
	
	*cGrid = grid;
	*nGrid = grid1;
	
	*running = 1;
	*paused = 1;
	*userSetting = 0;
		
	*colorR = 255;
	*colorG = 255;
	*colorB = 255;
	
	// Initialize SDL
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}
	
	// Create window
	
	*window = SDL_CreateWindow(
		"Game of Life", 
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED, 
		GRID_PIXEL_WIDTH * GRID_WIDTH, 
		GRID_PIXEL_HEIGHT * GRID_HEIGHT, 
		SDL_WINDOW_SHOWN);
	if (!(*window)) {
		printf("Unable to create window: %s\n", SDL_GetError());
		return EXIT_ERR;
	}
	*screenSurface = SDL_GetWindowSurface(*window);
	
	// ----------
	// MAIN LOOP
	// ----------
		
	while (*running) {
				
		// ----------
		// HANDLE EVENTS
		// ----------
		
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			switch (e.type) {
				case SDL_QUIT:
					*running = 0;
					break;
					
				case SDL_MOUSEBUTTONDOWN:
					if (e.button.button == SDL_BUTTON_LEFT) {
						if (GRID_GET_BIT(*cGrid, GRID_FROM_PIXEL_X(e.button.x), GRID_FROM_PIXEL_Y(e.button.y))) {
							GRID_CLR_BIT(*cGrid, GRID_FROM_PIXEL_X(e.button.x), GRID_FROM_PIXEL_Y(e.button.y));
							*userSetting = 2;
						}
						else {
							GRID_SET_BIT(*cGrid, GRID_FROM_PIXEL_X(e.button.x), GRID_FROM_PIXEL_Y(e.button.y));
							*userSetting = 1;
						}
					}
					break;
					
				case SDL_MOUSEBUTTONUP:
					if (e.button.button == SDL_BUTTON_LEFT)
						*userSetting = 0;
					break;
					
				case SDL_MOUSEMOTION:
					if (*userSetting == 1) GRID_SET_BIT(*cGrid, GRID_FROM_PIXEL_X(e.motion.x), GRID_FROM_PIXEL_Y(e.motion.y));
					else if (*userSetting == 2) GRID_CLR_BIT(*cGrid, GRID_FROM_PIXEL_X(e.motion.x), GRID_FROM_PIXEL_Y(e.motion.y));
					break;
					
				case SDL_KEYDOWN:
					switch (e.key.keysym.sym) {
						case SDLK_SPACE:
							*paused = *paused ? 0 : 1;
							break;
							
						case SDLK_ESCAPE:
							// Clear grid
							for (char x = 0; x < GRID_WIDTH; x++)
								for (char y = 0; y < GRID_HEIGHT; y++)
									GRID_CLR_BIT(*cGrid, x, y);
							break;
							
						case SDLK_c:
							*colorR = rand() % 256;
							*colorG = rand() % 256;
							*colorB = rand() % 256;
							break;
							
						case SDLK_v:
							*colorR = 255;
							*colorG = 255;
							*colorB = 255;
							break;
						
						case SDLK_r:
							// Generate random grid
							for (char x = 0; x < GRID_WIDTH; x++)
								for (char y = 0; y < GRID_HEIGHT; y++)
									if (rand() < RAND_MAX / 2) GRID_SET_BIT(*cGrid, x, y);
									else GRID_CLR_BIT(*cGrid, x, y);
					}
					break;
			}
		}
		
		// ----------
		// RENDER FRAME
		// ----------
		
		// Clear screen
		
		if (*paused) SDL_FillRect(*screenSurface, NULL, SDL_MapRGB((**screenSurface).format, 0, 0, 255));
		else SDL_FillRect(*screenSurface, NULL, SDL_MapRGB((**screenSurface).format, 0, 0, 0));
		
		// Render grid
				
		for (char x = 0; x < GRID_WIDTH; x++) {
			for (char y = 0; y < GRID_HEIGHT; y++) {				
				if (GRID_GET_BIT(*cGrid, x, y)) {
					SDL_FillRect(
						*screenSurface, 
						&(SDL_Rect){
							x * GRID_PIXEL_WIDTH,
							y * GRID_PIXEL_HEIGHT,
							GRID_PIXEL_WIDTH,
							GRID_PIXEL_HEIGHT},
						SDL_MapRGB((**screenSurface).format, *colorR, *colorG, *colorB));
				}
			}
		}
		
		// Update window surface with new render data
		
		SDL_UpdateWindowSurface(*window);
		
		// ----------
		// UPDATE FRAME
		// ----------
		
		if (!(*paused)) {
			for (char x = 0; x < GRID_WIDTH; x++) {
				for (char y = 0; y < GRID_HEIGHT; y++) {
					char aliveNeighbours = 0;
					if (x != 0 && 										GRID_GET_BIT(*cGrid, x - 1, y)) aliveNeighbours++;
					if (x != 0 && y != 0 && 							GRID_GET_BIT(*cGrid, x - 1, y - 1)) aliveNeighbours++;
					if (x != 0 && y != GRID_HEIGHT_MI && 				GRID_GET_BIT(*cGrid, x - 1, y + 1)) aliveNeighbours++;
					if (y != 0 && 										GRID_GET_BIT(*cGrid, x, y - 1)) aliveNeighbours++;
					if (y != GRID_HEIGHT_MI && 							GRID_GET_BIT(*cGrid, x, y + 1)) aliveNeighbours++;
					if (x != GRID_WIDTH_MI && 							GRID_GET_BIT(*cGrid, x + 1, y)) aliveNeighbours++;
					if (x != GRID_WIDTH_MI && y != 0 && 				GRID_GET_BIT(*cGrid, x + 1, y - 1)) aliveNeighbours++;
					if (x != GRID_WIDTH_MI && y != GRID_HEIGHT_MI && 	GRID_GET_BIT(*cGrid, x + 1, y + 1)) aliveNeighbours++;
											
					if (GRID_GET_BIT(*cGrid, x, y)) {
						if (aliveNeighbours < 2 || aliveNeighbours > 3) GRID_CLR_BIT(*nGrid, x, y);
						else GRID_SET_BIT(*nGrid, x, y);
					}
					else {
						if (aliveNeighbours == 3) GRID_SET_BIT(*nGrid, x, y);
						else GRID_CLR_BIT(*nGrid, x, y);
					}
				}
			}
			
			// Swap grid pointers to let new grid become the new current grid			
			char* temp = *cGrid;
			*cGrid = *nGrid;
			*nGrid = temp;
		}
				
		// ----------
		// WAIT FRAME
		// ----------
		
		SDL_Delay(FRAME_WAIT);
	}
	
	// ----------
	// RELEASE AND EXIT
	// ----------
	
	SDL_FreeSurface(*screenSurface);
	SDL_DestroyWindow(*window);
	SDL_Quit();
	
	free(memory);
		
	return EXIT_OK;
}
