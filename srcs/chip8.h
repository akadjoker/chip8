/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   chip8.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lrosa-do  <lrosa-do@student.42lisboa>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/14 15:44:33 by lrosa-do          #+#    #+#             */
/*   Updated: 2022/03/18 19:46:24 by lrosa-do         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHIP_H
#define CHIP_H


#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
//https://chip-8.github.io/links/
//https://www.freecodecamp.org/news/creating-your-very-own-chip-8-emulator/
//https://multigesture.net/wp-content/uploads/mirror/goldroad/chip8.shtml
//https://en.wikipedia.org/wiki/CHIP-8
//http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#0.1
//https://tobiasvl.github.io/blog/write-a-chip-8-emulator/


typedef unsigned char  BYTE;
typedef unsigned short SHORT;//2 bytes

#define CHIP_WIDTH          64
#define CHIP_HEIGHT         32
#define CHIP_SCALE          12    //scale the pixels to draw
#define CHIP_MEMORY_SIZE    4096 //4kb
#define CHIP_TOTAL_REGISTERS       16
#define CHIP_TOTAL_STACK           16
#define CHIP_TOTAL_KEYS            16
#define CHIP_LOAD_OFFSET            0x200
#define CHIP_SPRITE_HEIGHT 5

const char chip8_default_character[] = {
    0xf0, 0x90, 0x90, 0x90, 0xf0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xf0, 0x10, 0xf0, 0x80, 0xf0,
    0xf0, 0x10, 0xf0, 0x10, 0xf0,
    0x90, 0x90, 0xf0, 0x10, 0x10,
    0xf0, 0x80, 0xf0, 0x10, 0xf0,
    0xf0, 0x80, 0xf0, 0x90, 0xf0,
    0xf0, 0x10, 0x20, 0x40, 0x40,
    0xf0, 0x90, 0xf0, 0x90, 0xf0,
    0xf0, 0x90, 0xf0, 0x10, 0xf0,
    0xf0, 0x90, 0xf0, 0x90, 0x90,
    0xe0, 0x90, 0xe0, 0x90, 0xe0,
    0xf0, 0x80, 0x80, 0x80, 0xf0,
    0xe0, 0x90, 0x90, 0x90, 0xe0,
    0xf0, 0x80, 0xf0, 0x80, 0xf0, 
    0xf0, 0x80, 0xf0, 0x80, 0x80
};

typedef struct Chip8
{
    struct 
    {
        BYTE   buffer[CHIP_MEMORY_SIZE];       
    }memory;
    
    struct 
    {
        BYTE   V[CHIP_TOTAL_REGISTERS]; 
        BYTE   dt;     //register delay timer 60 fps -1
        BYTE   st;     //register sound timer 60hz
        BYTE   SP;     //program counter 2 bytes
        SHORT  I;      //memory adresses
        SHORT  PC;     //stack pointer index
    }registers;

    bool        screen[CHIP_HEIGHT][CHIP_WIDTH];
    bool        keys[CHIP_TOTAL_KEYS]; //virtual keys
    bool        pause;
    SHORT       stack[CHIP_TOTAL_STACK];
    float         delta;
    bool coliders; //dibale colliders in game
}Chip8;

Chip8 *chip8_int();
void  chip8_free(Chip8 *chip);
void  chip8_load(Chip8 *chip, const char *buffer, size_t size);
void  chip8_update(Chip8 *chip);
void  chip8_tinks(Chip8 *chip,float dt);

void  c8memory_set(Chip8 *chip,int index, BYTE buffer);
BYTE  c8memory_get(Chip8 *chip,int index);
SHORT c8memory_get_short(Chip8 *chip,int index);

void  c8stack_push(Chip8 *chip, SHORT value);
SHORT c8stack_pop(Chip8 *chip);


void  c8screen_set(Chip8 *chip, int x, int y);
bool  c8screen_get(Chip8 *chip,int x,int y);
void  c8screen_clear(Chip8 *chip);
bool  c8screen_draw_sprite(Chip8 *chip,int x, int y,const char* sprite,int num);


int  c8map_keys(const char* map,char key);
void c8key_set_down(Chip8 * chip, int key);
void c8key_set_up  (Chip8 * chip, int key);
bool c8key_is_down (Chip8 * chip, int key);

#endif

#ifdef CHIP8_IMPLEMNT

#include <memory.h>
#include <time.h>




Chip8 *chip8_int()
{
   Chip8 *chip=(Chip8 *) malloc(sizeof(Chip8));
   memset(chip,0,sizeof(Chip8));
   memset(&chip->keys,0,sizeof(chip->keys));
   memset(&chip->stack,0,sizeof(chip->stack));
   memset(&chip->screen,0,sizeof(chip->screen));
   memcpy(&chip->memory.buffer,chip8_default_character,sizeof(chip8_default_character));
   srand(time(NULL));
   
   memset(&chip->registers.V,0,sizeof(chip->registers.V)); 
   chip->registers.I=0;
   chip->registers.PC=0;
   chip->registers.dt=0;
   chip->registers.st=0;
   chip->pause=false;
   chip->delta =0;
   chip->coliders=true;
   return chip;
}
void chip8_free(Chip8 *chip)
{
    assert(chip != NULL);
    free(chip);
}

static void memory_bound(Chip8 *chip, int index)
{
    assert(chip != NULL);
    assert(index >= 0 && index < CHIP_MEMORY_SIZE);
}


static void stack_bound(Chip8 *chip)
{
    assert(chip != NULL);
    assert(chip->registers.SP < sizeof(chip->stack));
}


static void keys_bound(Chip8 *chip,int key)
{
    assert(chip != NULL);
    assert(key >=0 && key < CHIP_TOTAL_KEYS);
}


void c8memory_set(Chip8 *chip, int index, BYTE buffer)
{
    memory_bound(chip,index);
    chip->memory.buffer[index] =buffer;
}

BYTE c8memory_get(Chip8 *chip, int index)
{
    memory_bound(chip,index);
    return (chip->memory.buffer[index]);
}
SHORT c8memory_get_short(Chip8 *chip,int index)
{
     memory_bound(chip,index);
     BYTE b1 = chip->memory.buffer[index];
     BYTE b2 = chip->memory.buffer[index + 1];
    return (b1 << 8 | b2);
}
void c8stack_push(Chip8 *chip, SHORT value)
{
    chip->registers.SP += 1;
    stack_bound(chip);
    chip->stack[chip->registers.SP] = value;
}
SHORT c8stack_pop(Chip8 *chip)
{
    stack_bound(chip);
    SHORT value = chip->stack[chip->registers.SP];
    chip->registers.SP -= 1;
    return (value);
}

void c8key_set_down(Chip8 *chip,int key)
{
    keys_bound(chip,key);
    chip->keys[key] = true;
    if (chip->pause)
        chip->pause=false;

}

void c8key_set_up  (Chip8 *chip,int key)
{
    keys_bound(chip,key);
    chip->keys[key] = false;
    if (chip->pause)
        chip->pause=false;
}

bool c8key_is_down (Chip8 *chip,int key)
{
    keys_bound(chip,key);
    return chip->keys[key];
}

int c8map_keys(const char* map, char key)
{
    for (int i = 0; i < CHIP_TOTAL_KEYS;i++)
    {
        if (map[i]==key)
            return (i);
    }
    return (-1);
}

void  c8screen_set(Chip8 *chip, int x, int y)
{
 
    chip->screen[y][x] = true;
}

bool c8screen_get(Chip8 *chip, int x, int y)
{
 
    return chip->screen[y][x];
}

void c8screen_clear(Chip8 *chip)
{
    memset(&chip->screen, 0, sizeof(chip->screen)); 
}

bool c8screen_draw_sprite(Chip8 *chip,int x, int y,const char* sprite,int num)
{
    bool pixel_collison = false;
    for (int ly = 0; ly < num; ly++)
    {
        char c = sprite[ly];
        for (int lx = 0; lx < 8; lx++)
        {
            if ((c & (0b10000000 >> lx)) == 0)
                continue;
            //sprite is bounds (if x > width x =0 ele if x <0 x =width using modulos)
            if (chip->coliders)
            {
                if (chip->screen[(ly+y) % CHIP_HEIGHT][(lx+x) % CHIP_WIDTH])
                    pixel_collison = true;
            }
            
            chip->screen[(ly+y) % CHIP_HEIGHT][(lx+x) % CHIP_WIDTH] ^= true;
        }
    }
    return pixel_collison;
}



void chip8_load(Chip8 *chip, const char *buffer, size_t size)
{
    assert(chip != NULL);
    assert(size + CHIP_LOAD_OFFSET < CHIP_MEMORY_SIZE);
    //chip8  programs start as 0x200 offset
    memcpy(&chip->memory.buffer[CHIP_LOAD_OFFSET], buffer, size);  
    //set te register offset at 0x200
    chip->registers.PC = CHIP_LOAD_OFFSET;
  
}

static void chip8_exec_operatores(Chip8* chip8, SHORT opcode)
{
    BYTE x = (opcode >> 8) & 0x000f;
    BYTE y = (opcode >> 4) & 0x000f;
    BYTE final_four_bits = opcode & 0x000f; //operation to run 
    SHORT tmp = 0;
  switch(final_four_bits)
    {
        // 8xy0 - LD Vx, Vy. Vx = Vy
        case 0x00:
            chip8->registers.V[x] = chip8->registers.V[y];
        break;

        // 8xy1 - OR Vx, Vy. Performs a bitwise OR on Vx and Vy stores the result in Vx
        case 0x01:
            chip8->registers.V[x] = chip8->registers.V[x] | chip8->registers.V[y];
        break;

        // 8xy2 - AND Vx, Vy. Performs a bitwise AND on Vx and Vy stores the result in Vx
        case 0x02:
            chip8->registers.V[x] = chip8->registers.V[x] & chip8->registers.V[y];
        break;

        // 8xy3 - XOR Vx, Vy. Performs a bitwise XOR on Vx and Vy stores the result in Vx
        case 0x03:
            chip8->registers.V[x] = chip8->registers.V[x] ^ chip8->registers.V[y];
        break;

        // 8xy4 - ADD Vx, Vy. Set Vx = Vx + Vy, set VF = carry
        case 0x04:
            tmp = chip8->registers.V[x] + chip8->registers.V[y];
            chip8->registers.V[0x0f] = false;
            if (tmp > 0xff)
            {
                chip8->registers.V[0x0f] = true;
            }

            chip8->registers.V[x] = tmp;
        break;

        // 8xy5 - SUB Vx, Vy. Set vx = Vx - Vy, set VF = Not borrow
        case 0x05:
            chip8->registers.V[0x0f] = false;
            if (chip8->registers.V[x] > chip8->registers.V[y])
            {
                chip8->registers.V[0x0f] = true;
            }
            chip8->registers.V[x] = chip8->registers.V[x] - chip8->registers.V[y];
        break;

        // 8xy6 - SHR Vx {, Vy}
        case 0x06:
            chip8->registers.V[0x0f] = chip8->registers.V[x] & 0x01;
            chip8->registers.V[x] = chip8->registers.V[x] / 2;
        break;

        // 8xy7 - SUBN Vx, Vy
        case 0x07:
            chip8->registers.V[0x0f] = chip8->registers.V[y] > chip8->registers.V[x];
            chip8->registers.V[x] = chip8->registers.V[y] - chip8->registers.V[x];
        break;

        // 8xyE - SHL Vx {, Vy}
        case 0x0E:
            chip8->registers.V[0x0f] = chip8->registers.V[x] & 0b10000000;
            chip8->registers.V[x] = chip8->registers.V[x] * 2;
        break;
    }
}




static void chip8_exec_extended_ex(Chip8* chip, SHORT opcode)
{
    
    BYTE x = (opcode >> 8) & 0x000f;

    switch (opcode & 0x00ff)
    {
        // fx07 - LD Vx, DT. Set Vx to the delay timer value
        case 0x07:
            chip->registers.V[x] = chip->registers.dt;
        break;

        // fx0a - LD Vx, K
        case 0x0A:
        {

           /* bool keyPress = false;
           

            
                for (int i = 0; i < CHIP_TOTAL_KEYS; ++i)
                {
                    if (chip->keys[i] != 0)
                    {
                        chip->registers.V[x] = i;
                        keyPress = true;
                    }
                }
      */

            chip->pause=true;
          
        }
        break; 

        // fx15 - LD DT, Vx, set the delay timer to Vx
        case 0x15:
            chip->registers.dt = chip->registers.V[x];
        break;

        // fx18 - LD ST, Vx, set the sound timer to Vx
        case 0x18:
            chip->registers.st = chip->registers.V[x];
        break;


        // fx1e - Add I, Vx
        case 0x1e:
            chip->registers.I += chip->registers.V[x];
        break;

        // fx29 - LD F, Vx
        case 0x29:
            chip->registers.I = chip->registers.V[x] * CHIP_SPRITE_HEIGHT;
        break;

        // fx33 - LD B, Vx
        case 0x33:
        {
            BYTE hundreds = chip->registers.V[x] / 100;
            BYTE tens = chip->registers.V[x] / 10 % 10;
            BYTE units = chip->registers.V[x] % 10;
            c8memory_set(chip, chip->registers.I, hundreds);
            c8memory_set(chip, chip->registers.I+1, tens);
            c8memory_set(chip, chip->registers.I+2, units);
        }
        break;

        // fx55 - LD [I], Vx
        case 0x55:
        {
            for (int i = 0; i <= x; i++)
            {
                c8memory_set(chip, chip->registers.I+i, chip->registers.V[i]);
            }
        }
        break;

        // fx65 - LD Vx, [I]
        case 0x65:
        {
            for (int i = 0; i <= x; i++)
            {
                chip->registers.V[i] = c8memory_get(chip, chip->registers.I+i);
            }
        }
        break;

    }
}

static void chip8_exec_extended(Chip8 *chip,SHORT opcode)
{
    SHORT nnn = opcode & 0x0fff;
    BYTE x = (opcode >> 8) & 0x000f;
    BYTE y = (opcode >> 4) & 0x000f;
    BYTE kk = opcode & 0x00ff;
    BYTE n = opcode & 0x000f;

    switch(opcode & 0xf000)
    {
        // JP addr, 1nnn Jump to location nnn's
        case 0x1000:
            chip->registers.PC = nnn;
        break;

        // CALL addr, 2nnn Call subroutine at location nnn
        case 0x2000:
            c8stack_push(chip, chip->registers.PC);
            chip->registers.PC = nnn;
        break;

        // SE Vx, byte - 3xkk Skip next instruction if Vx=kk
        case 0x3000:
            if (chip->registers.V[x] == kk)
            {
                chip->registers.PC += 2;
            }
        break;

         // SNE Vx, byte - 3xkk Skip next instruction if Vx!=kk
        case 0x4000:
            if (chip->registers.V[x] != kk)
            {
                chip->registers.PC += 2;
            }
        break;

        // 5xy0 - SE, Vx, Vy, skip the next instruction if Vx = Vy
        case 0x5000:
            if (chip->registers.V[x] == chip->registers.V[y])
            {
                chip->registers.PC += 2;
            }
        break;

        // 6xkk - LD Vx, byte, Vx = kk
        case 0x6000:
            chip->registers.V[x] = kk;
        break;

        // 7xkk - ADD Vx, byte. Set Vx = Vx + kk
        case 0x7000:
            chip->registers.V[x] += kk;
        break;

        case 0x8000:
        {
            chip8_exec_operatores(chip, opcode);
        }
        break;

        // 9xy0 - SNE Vx, Vy. Skip next instruction if Vx != Vy
        case 0x9000:
            if (chip->registers.V[x] != chip->registers.V[y])
            {
                chip->registers.PC += 2;
            }
        break;

        // Annn - LD I, addr. Sets the I register to nnn
        case 0xA000:
            chip->registers.I = nnn;
        break;

        // bnnn - Jump to location nnn + V0
        case 0xB000:
            chip->registers.PC = nnn + chip->registers.V[0x00];
        break;

        // Cxkk - RND Vx, byte
        case 0xC000:
            srand(clock());
            chip->registers.V[x] = (rand() % 255) & kk;
        break;

        // Dxyn - DRW Vx, Vy, nibble. Draws sprite to the screen
        case 0xD000:
        {
            const char* sprite = (const char*) &chip->memory.buffer[chip->registers.I];
            chip->registers.V[0x0f] =c8screen_draw_sprite(chip, chip->registers.V[x], chip->registers.V[y], sprite, n);

        }
        break;

        // Keyboard operations
        case 0xE000:
        {
            switch(opcode & 0x00ff)
            {
                // Ex9e - SKP Vx, Skip the next instruction if the key with the value of Vx is pressed
                case 0x9e:
                    if (c8key_is_down(chip, chip->registers.V[x]))
                    {
                        chip->registers.PC += 2;
                    }
                break;

                // Exa1 - SKNP Vx - Skip the next instruction if the key with the value of Vx is not pressed
                case 0xa1:
                    if (!c8key_is_down(chip, chip->registers.V[x]))
                    {
                        chip->registers.PC += 2;
                    }
                break;
            }
        }
        break;
        
        case 0xF000:
        {
            chip8_exec_extended_ex(chip, opcode);
        }
        break;

    }
}

void chip8_excute(Chip8 *chip,SHORT opcode)
{
  // printf(" excute : %x , %d \n",opcode,chip->registers.PC);
    switch(opcode)
    {
        // CLS: Clear The Display
        case 0x00E0:
            c8screen_clear(chip);
        break;
        // Ret: Return from subroutine
        case 0x00EE:
            chip->registers.PC = c8stack_pop(chip);
        break;

        default:
            chip8_exec_extended(chip, opcode);
    }
    
}

void chip8_run(Chip8 *chip)
{
    
    SHORT opcode = c8memory_get_short(chip,chip->registers.PC);
    chip->registers.PC +=2; // 2 bytes per opcodes = sHORT  
    chip8_excute(chip, opcode);
}

//from stack over flow 
void	cSleep(float ms)
{
	struct timespec	req;
	time_t			sec;

	bzero(&req, 0);
	sec = (int)(ms / 1000.0f);
	ms -= (sec * 1000);
	req.tv_sec = sec;
	req.tv_nsec = ms * 1000000L;
	while (nanosleep(&req, &req) == -1)
		continue ;
}
void chip8_tinks(Chip8 *chip,float dt)
{
    assert(chip != NULL);
    if (!chip->pause)
    {
        // smoth the mulator , nor perfet but... ;)
        chip->delta += dt;
      //  printf("%f \n",chip->delta);
        while(chip->delta > 1)
        {
            chip->delta -= 1;
       
            //if delay timer is set , count to 0
            if (chip->registers.dt > 0)
            {
               // cSleep(0.5f);//  dont have a clue that this work :(
                chip->registers.dt-=1;
            }

            //if sound delay timer is set , count to 0
            if (chip->registers.st>0)
            {
            //  Beep(1500,100 * chip->registers.st); //todo , the sounds from chip8 f*** my head :D
                chip->registers.st-=1;
            }   
        }
    }
}
void chip8_update(Chip8 *chip)
{
    assert(chip != NULL);
    if (!chip->pause)
    {
     chip8_run(chip);
    }
}

#endif
