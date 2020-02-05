#include <stdio.h>
#include <dirent.h>
#include <SDL.h>
#include <sys/time.h>
#include "sal.h"

#define PALETTE_BUFFER_LENGTH	256*2*4
#define SNES_WIDTH  256
#define SNES_HEIGHT 239

SDL_Surface *mScreen = NULL;
static u32 mSoundThreadFlag=0;
static u32 mSoundLastCpuSpeed=0;
static u32 mPaletteBuffer[PALETTE_BUFFER_LENGTH];
static u32 *mPaletteCurr=(u32*)&mPaletteBuffer[0];
static u32 *mPaletteLast=(u32*)&mPaletteBuffer[0];
static u32 *mPaletteEnd=(u32*)&mPaletteBuffer[PALETTE_BUFFER_LENGTH];
static u32 mInputFirst=0;

s32 mCpuSpeedLookup[1]={0};

#include <sal_common.h>

static u32 inputHeld[2] = {0, 0};
static SDL_Joystick *joy[2];

static u32 sal_Input(int held, u32 j)
{
	inputHeld[j] = 0;

	if (SDL_NumJoysticks() > 0) {
		int deadzone = 10000;
		if (joy[j] == NULL) joy[j] = SDL_JoystickOpen(j);

		SDL_JoystickUpdate();

		int joy_x = SDL_JoystickGetAxis(joy[j], 0);
		int joy_y = SDL_JoystickGetAxis(joy[j], 1);

		if (joy_x < -deadzone) inputHeld[j] |= SAL_INPUT_LEFT;
		else if (joy_x > deadzone) inputHeld[j] |= SAL_INPUT_RIGHT;

		if (joy_y < -deadzone) inputHeld[j] |= SAL_INPUT_UP;
		else if (joy_y > deadzone) inputHeld[j] |= SAL_INPUT_DOWN;

		if (SDL_JoystickGetButton(joy[j], 0)) inputHeld[j] |= SAL_INPUT_X;
		if (SDL_JoystickGetButton(joy[j], 1)) inputHeld[j] |= SAL_INPUT_A;
		if (SDL_JoystickGetButton(joy[j], 2)) inputHeld[j] |= SAL_INPUT_B;
		if (SDL_JoystickGetButton(joy[j], 3)) inputHeld[j] |= SAL_INPUT_Y;
		if (SDL_JoystickGetButton(joy[j], 4)) inputHeld[j] |= SAL_INPUT_L;
		if (SDL_JoystickGetButton(joy[j], 5)) inputHeld[j] |= SAL_INPUT_R;
		if (SDL_JoystickGetButton(joy[j], 8)) inputHeld[j] |= SAL_INPUT_SELECT;
		if (SDL_JoystickGetButton(joy[j], 9)) inputHeld[j] |= SAL_INPUT_START;
		if (SDL_JoystickGetButton(joy[j], 8) && SDL_JoystickGetButton(joy[j], 9)) inputHeld[j] |= SAL_INPUT_MENU;
		if (SDL_JoystickGetButton(joy[j], 8) && SDL_JoystickGetButton(joy[j], 4)) inputHeld[j] |= SAL_INPUT_QUICKLOAD;
		if (SDL_JoystickGetButton(joy[j], 8) && SDL_JoystickGetButton(joy[j], 5)) inputHeld[j] |= SAL_INPUT_QUICKSAVE;
	}

	if (j == 0) {
		u8 *keys = SDL_GetKeyState(NULL);

		if (keys[SDLK_LCTRL])		inputHeld[j] |= SAL_INPUT_A;
		if (keys[SDLK_LALT])		inputHeld[j] |= SAL_INPUT_B;
		if (keys[SDLK_SPACE])		inputHeld[j] |= SAL_INPUT_X;
		if (keys[SDLK_LSHIFT])		inputHeld[j] |= SAL_INPUT_Y;
		if (keys[SDLK_TAB])			inputHeld[j] |= SAL_INPUT_L;
		if (keys[SDLK_BACKSPACE])	inputHeld[j] |= SAL_INPUT_R;
		if (keys[SDLK_PAGEDOWN])	inputHeld[j] |= SAL_INPUT_L;
		if (keys[SDLK_PAGEUP])		inputHeld[j] |= SAL_INPUT_R;
		if (keys[SDLK_RETURN])		inputHeld[j] |= SAL_INPUT_START;
		if (keys[SDLK_ESCAPE])		inputHeld[j] |= SAL_INPUT_SELECT;
		if (keys[SDLK_UP])			inputHeld[j] |= SAL_INPUT_UP;
		if (keys[SDLK_DOWN])		inputHeld[j] |= SAL_INPUT_DOWN;
		if (keys[SDLK_LEFT])		inputHeld[j] |= SAL_INPUT_LEFT;
		if (keys[SDLK_RIGHT])		inputHeld[j] |= SAL_INPUT_RIGHT;
		if (keys[SDLK_END] || keys[SDLK_HOME] || (keys[SDLK_ESCAPE] && keys[SDLK_RETURN])) inputHeld[j] |= SAL_INPUT_MENU;
		if (keys[SDLK_ESCAPE] && keys[SDLK_TAB]) inputHeld[j] |= SAL_INPUT_QUICKLOAD;
		if (keys[SDLK_ESCAPE] && keys[SDLK_BACKSPACE]) inputHeld[j] |= SAL_INPUT_QUICKSAVE;

		SDL_Event event;
		if (!SDL_PollEvent(&event)) {
			if (held) return inputHeld[j];
			return 0;
		}
	}

	mInputRepeat = inputHeld[j];
	return inputHeld[j];
}

static int key_repeat_enabled = 1;

u32 sal_InputPollRepeat(u32 j)
{
	if (!key_repeat_enabled) {
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		key_repeat_enabled = 1;
	}
	return sal_Input(0, j);
}

u32 sal_InputPoll(u32 j)
{
	if (key_repeat_enabled) {
		SDL_EnableKeyRepeat(0, 0);
		key_repeat_enabled = 0;
	}
	return sal_Input(1, j);
}

const char* sal_DirectoryGetTemp(void)
{
	return "/tmp";
}

void sal_CpuSpeedSet(u32 mhz)
{
}

u32 sal_CpuSpeedNext(u32 currSpeed)
{
	u32 newSpeed=currSpeed+1;
	if(newSpeed > 500) newSpeed = 500;
	return newSpeed;
}

u32 sal_CpuSpeedPrevious(u32 currSpeed)
{
	u32 newSpeed=currSpeed-1;
	if(newSpeed > 500) newSpeed = 0;
	return newSpeed;
}

u32 sal_CpuSpeedNextFast(u32 currSpeed)
{
	u32 newSpeed=currSpeed+10;
	if(newSpeed > 500) newSpeed = 500;
	return newSpeed;
}

u32 sal_CpuSpeedPreviousFast(u32 currSpeed)
{
	u32 newSpeed=currSpeed-10;
	if(newSpeed > 500) newSpeed = 0;
	return newSpeed;
}

s32 sal_Init(void)
{
	setenv("SDL_NOMOUSE", "1", 1);
	if( SDL_Init( SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK ) == -1 )
	{
		return SAL_ERROR;
	}
	sal_TimerInit(60);

	memset(mInputRepeatTimer,0,sizeof(mInputRepeatTimer));

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	return SAL_OK;
}

u32 sal_VideoInit(u32 bpp)
{
	SDL_ShowCursor(0);

	mBpp=bpp;

	//Set up the screen
	mScreen = SDL_SetVideoMode(SAL_SCREEN_WIDTH, SAL_SCREEN_HEIGHT, bpp, SDL_HWSURFACE |
#ifdef SDL_TRIPLEBUF
    SDL_TRIPLEBUF
#else
    SDL_DOUBLEBUF
#endif
	);

	//If there was an error in setting up the screen
	if( mScreen == NULL )
	{
	sal_LastErrorSet("SDL_SetVideoMode failed");
	return SAL_ERROR;
	}

	return SAL_OK;
}

u32 sal_VideoGetWidth()
{
	return mScreen->w;
}

u32 sal_VideoGetHeight()
{
	return mScreen->h;
}

u32 sal_VideoGetPitch()
{
	return mScreen->pitch;
}

void sal_VideoEnterGame(u32 fullscreenOption, u32 pal, u32 refreshRate)
{
#ifdef GCW_ZERO
	/* Copied from C++ headers which we can't include in C */
	unsigned int Width = 256 /* SNES_WIDTH */,
	             Height = pal ? 239 /* SNES_HEIGHT_EXTENDED */ : 224 /* SNES_HEIGHT */;
	if (fullscreenOption != 3)
	{
		Width = SAL_SCREEN_WIDTH;
		Height = SAL_SCREEN_HEIGHT;
	}
	if (SDL_MUSTLOCK(mScreen)) SDL_UnlockSurface(mScreen);
	mScreen = SDL_SetVideoMode(Width, Height, mBpp, SDL_HWSURFACE |
#ifdef SDL_TRIPLEBUF
		SDL_TRIPLEBUF
#else
		SDL_DOUBLEBUF
#endif
		);
	mRefreshRate = refreshRate;
	if (SDL_MUSTLOCK(mScreen)) SDL_LockSurface(mScreen);
#endif
}

void sal_VideoSetPAL(u32 fullscreenOption, u32 pal)
{
	if (fullscreenOption == 3) /* hardware scaling */
	{
		sal_VideoEnterGame(fullscreenOption, pal, mRefreshRate);
	}
}

void sal_VideoExitGame()
{
#ifdef GCW_ZERO
	if (SDL_MUSTLOCK(mScreen)) SDL_UnlockSurface(mScreen);
	mScreen = SDL_SetVideoMode(SAL_SCREEN_WIDTH, SAL_SCREEN_HEIGHT, mBpp, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (SDL_MUSTLOCK(mScreen)) SDL_LockSurface(mScreen);
#endif
}

void sal_VideoBitmapDim(u16* img, u32 pixelCount)
{
	u32 i;
	for (i = 0; i < pixelCount; i += 2)
		*(u32 *) &img[i] = (*(u32 *) &img[i] & 0xF7DEF7DE) >> 1;
	if (pixelCount & 1)
		img[i - 1] = (img[i - 1] & 0xF7DE) >> 1;
}

void sal_VideoFlip(s32 vsync)
{
	if (SDL_MUSTLOCK(mScreen)) SDL_UnlockSurface(mScreen);
	SDL_Flip(mScreen);
	if (SDL_MUSTLOCK(mScreen)) SDL_LockSurface(mScreen);
}

void *sal_VideoGetBuffer()
{
	return (void*)mScreen->pixels;
}

void sal_VideoPaletteSync()
{

}

void sal_VideoPaletteSet(u32 index, u32 color)
{
	*mPaletteCurr++=index;
	*mPaletteCurr++=color;
	if(mPaletteCurr>mPaletteEnd) mPaletteCurr=&mPaletteBuffer[0];
}

void sal_Reset(void)
{
	for(int j = 0; j < SDL_NumJoysticks(); j++)
		SDL_JoystickClose(joy[j]);
	sal_AudioClose();
	SDL_Quit();
}

int mainEntry(int argc, char *argv[]);

// Prove entry point wrapper
int main(int argc, char *argv[])
{
	return mainEntry(argc,argv);
//	return mainEntry(argc-1,&argv[1]);
}
