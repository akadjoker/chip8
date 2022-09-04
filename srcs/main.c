#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#define MSF_GIF_IMPL
#include "msf_gif.h"   // GIF recording functionality

#include <mlx.h>
#include <mlx_int.h>

#define CHIP8_IMPLEMNT
#include "chip8.h"


#define MLX_ERROR 1

#define CRED   0xFF0000
#define CGREEN 0x00FF00
#define CBLUE  0x0000FF
#define CWHITE 0xFFFFFF
#define CBLACK 0x000000


const char keyboard_map[CHIP_TOTAL_KEYS] = {
    49, 50, 51, 52,     //1 2 3 4 
    113, 119, 101, 114, //q w e r
    97, 115, 100, 102,  //a s d f
    122, 120, 99, 118}; //z x c v

static int speed = 12;

int WINDOW_WIDTH  = 600;
int WINDOW_HEIGHT = 450;

static int gifFrameCounter = 0;             // GIF frames counter
static bool gifRecording = false;           // GIF recording state
static MsfGifState gifState = { 0 };        // MSGIF context state
static int screenshotCounter = 0;  


typedef struct IScreen
{
    void	*buffer;
	char	*addr;
	int		bpp;
	int		line_len;
	int		endian;
	void	*mlx;
	void	*win;
}IScreen;

typedef struct IRect
{

	int		x;
	int		y;
	int		w;
	int	h;

}IRect;

typedef    struct Timer
{
        double current; 
        double previous;
        double update;  
        double draw;    
        double frame;   
        double target;  
        unsigned int frameCounter;
		unsigned long long base;
} Timer;

IScreen  screen ={0};
Timer timer = {0};
int	local_endian;
Chip8 *chip;


double GetTime(void)
{
    struct timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long long int time = (unsigned long long int)ts.tv_sec*1000000000LLU + (unsigned long long int)ts.tv_nsec;
    return (double)(time - timer.base)*1e-9;  
}


float GetDeltaTime(void)
{
    return (float)timer.frame;
}
int GetFPS(void)
{
    int fps = 0;

    #define FPS_CAPTURE_FRAMES_COUNT    30      // 30 captures
    #define FPS_AVERAGE_TIME_SECONDS   0.5f     // 500 millisecondes
    #define FPS_STEP (FPS_AVERAGE_TIME_SECONDS/FPS_CAPTURE_FRAMES_COUNT)

    static int index = 0;
    static float history[FPS_CAPTURE_FRAMES_COUNT] = { 0 };
    static float average = 0, last = 0;
    float fpsFrame = GetDeltaTime();

    if (fpsFrame == 0) return 0;

    if ((GetTime() - last) > FPS_STEP)
    {
        last = (float)GetTime();
        index = (index + 1)%FPS_CAPTURE_FRAMES_COUNT;
        average -= history[index];
        history[index] = fpsFrame/FPS_CAPTURE_FRAMES_COUNT;
        average += history[index];
    }
    fps = (int)roundf(1.0f/average);
    return fps;
}
void set_target_fps(int fps)
{
    if (fps < 1) timer.target = 0.0;
    else timer.target = 1.0/(double)fps;
    printf("Target time per frame: %02.03f milliseconds \n", (float)timer.target*1000.0f);
}
void init_timer(void)
{
    struct timespec now = { 0 };

    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0)  
    {
        timer.base = (unsigned long long int)now.tv_sec*1000000000LLU + (unsigned long long int)now.tv_nsec;
    }
    timer.previous = GetTime();  
	timer.frameCounter  =0;
	set_target_fps(60);
}

void	Sleep(double ms)
{
	struct timespec	req;
	time_t			sec;
	bzero(&req, 0);
	sec = (int)(ms / 1000.0);
	ms -= (sec * 1000);
	req.tv_sec = sec;
	req.tv_nsec = ms * 1000000L;
	while (nanosleep(&req, &req) == -1)
		continue ;
}
void begin_time()
{
    timer.current = GetTime();      
    timer.update = timer.current - timer.previous;
    timer.previous = timer.current;
}


void end_time()
{

    timer.current = GetTime();
    timer.draw = timer.current - timer.previous;
    timer.previous = timer.current;
    timer.frame = timer.update + timer.draw;
    if (timer.frame < timer.target)
    {
        Sleep((timer.target - timer.frame)*1000.0);
        timer.current = GetTime();
        double waitTime = timer.current - timer.previous;
        timer.previous = timer.current;
        timer.frame += waitTime;    // Total frame time: update + draw + wait
    }
    timer.frameCounter++;
}











const char	*format_text(const char *text, ...)
{
	static char	buffers[4][1024];
	static int	index;
	char		*c_buffer;
	va_list		args;

	index = 0;
	c_buffer = buffers[index];
	memset(c_buffer, 0, 1024);
	va_start(args, text);
	vsnprintf(c_buffer, 1024, text, args);
	va_end(args);
	index += 1;
	if (index >= 4)
		index = 0;
	return (c_buffer);
}





void	bblit(int x, int y, int color)
{
	char    *pixel;
	int		i;

	i = screen.bpp - 8;
    pixel = screen.addr + (y * screen.line_len + x * (screen.bpp / 8));
	while (i >= 0)
	{

		if (screen.endian != 0)
			*pixel++ = (color >> i) & 0xFF;

		else
			*pixel++ = (color >> (screen.bpp - 8 - i)) & 0xFF;
		i -= 8;
	}
}



void	Clear( int color)
{
	int	i;
	int	j;

	if (screen.win == NULL)
		return ;
	i = 0;
	while (i < WINDOW_HEIGHT)
	{
		j = 0;
		while (j < WINDOW_WIDTH)
			bblit( j++, i, color);
		++i;
	}
}

int DrawRect(int x, int y, int width, int height,int color)
{
	int	i;
	int j;
    if (screen.win == NULL)
		return (1);
	i = y;
	while (i < y + height)
	{
		j = x;
		while (j < x + width)
			bblit(j++, i, color);
		++i;
	}
	return (0);
}



void DrawText(const char* text ,int x, int y, int color)
{
mlx_string_put(screen.mlx,screen.win, x,y,color,(char*)text);
}

void StopRecord()
{
     if (gifRecording)
    {
        MsfGifResult result = msf_gif_end(&gifState);
        msf_gif_free(result);
        gifRecording = false;
    }
}

bool SaveFileData(const char *fileName, void *data, unsigned int bytesToWrite)
{
    bool success = false;

        FILE *file = fopen(fileName, "wb");

        if (file != NULL)
        {
            unsigned int count = (unsigned int)fwrite(data, sizeof(unsigned char), bytesToWrite, file);

            int result = fclose(file);
            if (result == 0) success = true;
        }
        else printf("Failed to open file %s", fileName);
    return success;
}
void StartRecord()
{
	       if (gifRecording)
            {
                gifRecording = false;
                MsfGifResult result = msf_gif_end(&gifState);
                SaveFileData(format_text("%s/screenrec%03i.gif","./", screenshotCounter), result.data, (unsigned int)result.dataSize);
                msf_gif_free(result);
                printf("Finish  GIF recording \n");
            }
            else
            {
                gifRecording = true;
                gifFrameCounter = 0;
				msf_gif_begin(&gifState, WINDOW_WIDTH, WINDOW_HEIGHT);
                screenshotCounter++;
                printf("Start  GIF recording: %s", format_text("screenrec%03i.gif \n", screenshotCounter));
            }
}

void RecordEvent()
{

	 if (gifRecording)
    {
        #define GIF_RECORD_FRAMERATE    5 // record per seconds
        gifFrameCounter++;
        if ((gifFrameCounter%GIF_RECORD_FRAMERATE) == 0)
               msf_gif_frame(&gifState, screen.addr, 10, 16, (int)((float)WINDOW_WIDTH)*4);
        if (((gifFrameCounter/15)%2) == 1)
			DrawText("RECORDING",10,WINDOW_HEIGHT-20,CRED);
	}
    
}

int	close_win(void *p)
{
    mlx_destroy_window(screen.mlx,screen.win);
    mlx_destroy_display(screen.mlx);
    free(screen.mlx);
	chip8_free(chip);
	StopRecord();
    printf("Exit\n");
        exit(0);
    return 0;
}

int	on_key_down(int key,void *p)
{
	 
int vkey = c8map_keys(keyboard_map, key);
if (vkey != -1)
{
	c8key_set_down(chip, vkey);
}

 return 0;
}

int	on_key_up(int key,void *p)
{
  //printf("Key up Win : %d\n",key);
  if (key==0xFF1B)
      close_win(p);
int vkey = c8map_keys(keyboard_map, key);
if (vkey != -1)
{
	c8key_set_up(chip, vkey);
}
if (key==57) //9
	speed-=1;
if (key==48) //0
	speed+=1;

if (key==65470) //f1
{
	StartRecord();
}

 return 0;
}





void Flip()
{
mlx_put_image_to_window(screen.mlx,screen.win, screen.buffer, 0, 0);
}


int	render(void *p)
{

if (screen.win == NULL)
		return (1);

begin_time();


Clear(CBLACK);




chip8_tinks(chip,GetDeltaTime()*100);
for (int i = 0; i < speed; i++) 
{
	chip8_update(chip);      
}

//DrawRect(WINDOW_WIDTH - 100, WINDOW_HEIGHT - 100,100, 100, CGREEN);
//DrawRect(0, 0, 300, 100, CRED);
 for (int x = 0; x < CHIP_WIDTH; x++)
        {
            for (int y = 0; y < CHIP_HEIGHT; y++)
            {
                if (c8screen_get(chip, x, y))
                {
                    DrawRect(x * CHIP_SCALE, y * CHIP_SCALE, CHIP_SCALE, CHIP_SCALE, CWHITE);
                }
            }
        }



Flip();


RecordEvent();
DrawText(format_text("FPS [%d] Speed [%d]", GetFPS(),speed),5,15,CGREEN);

end_time();

       



return 1;
}


int main(int argc, char** args)
{
    
const char *fileName;//="../../roms/42.ch8";


  if (argc < 2)
    {
        printf("You must provide a file to load\n");
        return -1;
    }
    fileName = args[1];




    FILE* f = fopen(fileName, "rb");
    if (!f)
    {
        printf("Failed to open the file");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char buf[size];
    int res = fread(buf, size, 1, f);
    if (res != 1)
    {
        printf("Failed to read from file");
        return -1;
    }
    

    chip = chip8_int();
    chip8_load(chip, buf, size);



  int	a;
  a = 0x11223344;
  if (((unsigned char *)&a)[0] == 0x11)
    local_endian = 1;
  else
    local_endian = 0;

  if (!(screen.mlx = mlx_init()))
    {
      printf(" !! KO !!\n");
      exit(1);
    }
  
  WINDOW_WIDTH  = CHIP_WIDTH  * CHIP_SCALE;
  WINDOW_HEIGHT = CHIP_HEIGHT * CHIP_SCALE;
  screen.win = mlx_new_window(screen.mlx, WINDOW_WIDTH, WINDOW_HEIGHT,"Chip8 by Luis Santos AKA DJOKER");

    if (screen.win == NULL)
	{
		free(screen.win);
		return (MLX_ERROR);
	}



screen.buffer = mlx_new_image(screen.mlx, WINDOW_WIDTH, WINDOW_HEIGHT);
screen.addr = mlx_get_data_addr(screen.buffer, &screen.bpp,&screen.line_len, &screen.endian);


  init_timer();

  mlx_loop_hook(screen.mlx, &render, 0);
  mlx_hook(screen.win, 17, 1L<<5, close_win, 0);
  mlx_hook(screen.win, 2, 1L<<0, on_key_down, 0);
  mlx_hook(screen.win, 3, 1L<<1, on_key_up, 0);


  printf("Esc to  quit.\n");

  mlx_loop(screen.mlx);

  mlx_destroy_image(screen.mlx, screen.buffer);
  mlx_destroy_display(screen.mlx);
  free(screen.mlx);

  return (0);
}
