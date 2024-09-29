// ProjectMeltdown.c
// Runs on MSPM0G3507
// Tomi Alaofin, Ben Huynh
// Last Modified: 04/15/2024

#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "../inc/DAC5.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"
#define buttonU 1
#define buttonL 2
#define buttonD 4
#define buttonR 8
#define buttonRR 16

#define Fix 16//has something to do with animation
#define shoot (1<<5)//checks for shoot button
//Flags
uint8_t Flag = 0; //interrupt flag
uint8_t draw_player = 0;//when adc is ready and its time to draw player on the screen
uint8_t shoot_flag;//when new laser has been initialized
uint8_t need_to_pause=0;//when fire pause button is pressed
uint8_t select_language=0;//
uint8_t language_selected;
uint8_t new_laser =1;
uint8_t finished_drawing = 0;
uint8_t finished_drawing_weapon;
uint8_t finished_clearing_entity=1;
uint8_t select_english;
uint8_t select_spanish;
uint8_t display_languages =0;
uint8_t reset_screen =0;
uint8_t enemies_left;
uint32_t max_laser;
uint32_t pos;
uint8_t need_to_draw;
uint32_t spanish_selected;
uint8_t english_selected;
uint8_t need_to_shoot,need_to_pause;
uint8_t collision_detected=0;
static int8_t alien_index = 0, shot_counter =0,lang =0;
static uint8_t x,i, y,xs,ys;
static int8_t oldx1;
static int8_t active_lasers =1;
int8_t end_of_game;
int32_t x1,y1,position;
// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz

void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}


uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}

uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {LANGUAGE,SELECTION_INSTRUCTION} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Select_English[]="Fire to select";
const char Select_Spanish[]="Adi\xA2s";
const char Select_Portuguese[] = "Tchau";
const char Select_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";
const char *Phrases[2][4]={
  //{Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French},
  {Select_English,Select_Spanish,Select_Portuguese,Select_French}

};



typedef enum {dead, alive} status_t;

struct sprite{
    int32_t x;
    int32_t y;
    const uint16_t *image;
    int32_t vx;
    int32_t vy;
    int32_t xsize;
    int32_t ysize;
    int32_t oldx;
    int32_t oldy;
    int32_t number_of_collisions;
    int32_t clear_status;

    status_t life;
};
typedef struct sprite sprite_t;
sprite_t Enemy_spaceship[5] ={
                               {40,150, SmallEnemy30pointB,2,2,16,10,6,6,0,0, alive},
                               {60,10, SmallEnemy30pointB,2,2,16,10,6,6,0,0,alive},
                               {80,10, SmallEnemy30pointB,2,2,18,8,6,6,0,0, alive},
                               {100,10, SmallEnemy30pointB,2,2,18,8, 6,6,0,0,alive},
                               {120,10, SmallEnemy30pointB,2,2,18,8,6,6,0,0,alive}
};

typedef struct player player_t;
struct player{
    int32_t x;
    int32_t y;
    const uint16_t *image;
    int32_t xsize;
    int32_t ysize;
    status_t life;
};
player_t player_ship[1] ={
                               {40,150, PlayerShip0,18,8,alive}
};

typedef struct weapon weapon_t;
struct weapon{
    int32_t x;
    int32_t y;
    const uint16_t *image;
    int32_t vy;
    int32_t xsize;
    int32_t ysize;
    int32_t oldx;
    int32_t oldy;
    status_t life;
};
sprite_t inventory[5] ={
                       {40,150, SmallEnemy30pointB,2,16,10,6,6, alive},
                       {40,150, SmallEnemy30pointB,2,16,10,6,6, alive},
                       {40,150, SmallEnemy30pointB,2,16,10,6,6, alive},
                       {40,150, SmallEnemy30pointB,2,16,10,6,6, alive},
                       {40,150, SmallEnemy30pointB,2,16,10,6,6, alive}

};



void Init(void){
    uint8_t i;
    //initializing enemy struct
    enemies_left = 5;
    alien_index =4;
    for(i =0; i < enemies_left; i++){
        x = Enemy_spaceship[i].x = 20*i+12;
        Enemy_spaceship[i].oldx = x;
        y=Enemy_spaceship[i].y = 10;
        Enemy_spaceship[i].oldy = y;
        Enemy_spaceship[i].image = SmallEnemy30pointB;
        Enemy_spaceship[i].life = alive;
    }
    max_laser = 0;//initialize maximum number of laser to 0

    //initializing inventory struct
    oldx1=player_ship[0].x = 50;//starting ship position
    for(i =0; i < 1; i++){
        x=inventory[i].x = oldx1;
        inventory[i].oldx = x;
        y=inventory[i].y = 150;
        inventory[i].oldy = y;
        inventory[i].vy=8;
        inventory[i].life = dead;
    }
    //initializing enemy's velocity
    player_ship[0].y = 159;
    Enemy_spaceship[0].vx = 2; Enemy_spaceship[0].vy = 6;
    Enemy_spaceship[1].vx = 2; Enemy_spaceship[1].vy = 6;
    Enemy_spaceship[2].vx = 2; Enemy_spaceship[2].vy = 6;
    Enemy_spaceship[3].vx = 2; Enemy_spaceship[3].vy = 6;
    Enemy_spaceship[4].vx = 2; Enemy_spaceship[4].vy = 6;

}
uint8_t weapon_index;
//takes care of laser or weapon movement
void shoot_vertically(void){int shot_counter=0;
    for(shot_counter = 0; shot_counter < 5; shot_counter++){
        if(inventory[shot_counter].life == alive){
            need_to_shoot  = 1;// mark as changed
            if(finished_drawing_weapon){
                if(inventory[shot_counter].y > 0){//check for collision

                    inventory[shot_counter].oldy = inventory[shot_counter].y;
                    inventory[shot_counter].y =  inventory[shot_counter].y - inventory[shot_counter].vy;
                }
                else{
                    inventory[shot_counter].life = dead;//means outside of the border
                    }
            }

        }
    }
}
//psuedo code:

//initialize all weapon to dead
//number of possible shots already initilized to 20//make it 2 for now
//when button is pressed, status is alive
//when button is pressed again, look for a dead one and activate it, only update the position of the ones alive
//when the laser reaches the border, make it dead
void shoot_init(void){int find_inactive=0;
//    for(find_inactive  = 0; find_inactive< 1; find_inactive++ ){
        if(inventory[find_inactive].life == dead){
            inventory[find_inactive].oldx = inventory[find_inactive].x = pos;
            inventory[find_inactive].oldy = inventory[find_inactive].y =156;
            //if(collision_detected==0){
                inventory[find_inactive].life = alive;
            //}
        }
//    }
}
//moves enemy vertically downwards
void Move_vertically(void){int i;
    for(i = 0; i<enemies_left; i++){
        if(Enemy_spaceship[i].life == alive){
            need_to_draw  = 1;// mark as changed
            if(Enemy_spaceship[i].y < 200 && finished_drawing){
                Enemy_spaceship[i].oldy = Enemy_spaceship[i].y;
                y = Enemy_spaceship[i].y =  Enemy_spaceship[i].y + Enemy_spaceship[i].vy;
                x=Enemy_spaceship[i].oldx = Enemy_spaceship[i].x;
            }
            else{
                break;//Enemy_spaceship[i].life = dead;//means dead
            }
        }

    }
}
void Move_sinusoidally(void){int i =0,j=0;
    for(i=0;i<enemies_left;i++){//looks for ememy that hits the border
        if(Enemy_spaceship[i].life == alive ){
            need_to_draw  = 1;// mark as changed
            if((Enemy_spaceship[i].y > 159)){//checks if its not beyond the lower border
                Enemy_spaceship[0].life = dead;
            }

            if(finished_drawing && finished_clearing_entity){
                if((Enemy_spaceship[i].y < 10)||(Enemy_spaceship[i].x > 112)||(Enemy_spaceship[i].x < 0)){//if it hits left or right border
                    for(j=0;j<5;j++){//changes the position of all the enemy sprites one condition is met
                        Enemy_spaceship[j].oldy = Enemy_spaceship[j].y;
                        Enemy_spaceship[j].y =  Enemy_spaceship[j].y + 6;//in y
                        Enemy_spaceship[j].vx *=-1 ;//in opposite x
                        Enemy_spaceship[j].oldx = Enemy_spaceship[j].x;
                        Enemy_spaceship[j].x =  Enemy_spaceship[j].x + Enemy_spaceship[j].vx;
                        if(i == 0 && j==4){
                            Enemy_spaceship[j].oldx = Enemy_spaceship[j].x;
                            Enemy_spaceship[j].x =  Enemy_spaceship[j].x + Enemy_spaceship[j].vx;
                        }
                        else if(Enemy_spaceship[0].life == dead  && i == 1 && j==4){
                            Enemy_spaceship[j].oldx = Enemy_spaceship[j].x;
                            Enemy_spaceship[j].x =  Enemy_spaceship[j].x + Enemy_spaceship[j].vx;
                        }
                        else if(Enemy_spaceship[0].life == dead && Enemy_spaceship[0].life == dead  && i == 2 && j==4){
                            Enemy_spaceship[j].oldx = Enemy_spaceship[j].x;
                            Enemy_spaceship[j].x =  Enemy_spaceship[j].x + Enemy_spaceship[j].vx;
                        }
                        else if(Enemy_spaceship[0].life == dead && Enemy_spaceship[1].life == dead && Enemy_spaceship[2].life == dead && i == 3 && j==4){
                            Enemy_spaceship[j].oldx = Enemy_spaceship[j].x;
                            Enemy_spaceship[j].x =  Enemy_spaceship[j].x + Enemy_spaceship[j].vx;
                        }
                    }
                    break;
                }
                Enemy_spaceship[i].oldx = Enemy_spaceship[i].x;
                Enemy_spaceship[i].x =  Enemy_spaceship[i].x + Enemy_spaceship[i].vx;
            }
            else{
                break;
            }
        }
    }
}


void Detect_collision(void){
    for(int i = 0; i< enemies_left; i++){
        if(Enemy_spaceship[i].life == alive&&inventory[0].life ==alive){
            x1=(Enemy_spaceship[i].x - inventory[0].x);
            y1=( inventory[0].y - Enemy_spaceship[i].y);
            if(x1<0){
                x1*=-1;
            }
            if(y1<0){
                y1*=-1;
            }
            position = x1 + y1 ;
            if(position < 16){
                //Enemy_spaceship[i].number_of_collisions++;//if number of collision exceeds a certain limit, enemy is dead
                //if(Enemy_spaceship[i].number_of_collisions>3){
                    inventory[0].life = dead;
                    Enemy_spaceship[i].number_of_collisions +=1;
                //}
                    collision_detected =1;
            }
        }

    }
}


//makes an index to a specific weapon based on button pressed


//detects collision between sprite and player or sprite and bullet....to be decided
// games  engine runs at 30Hz
void TIMG12_IRQHandler(void){uint32_t button,now,last=0;
uint32_t static i = 0;//keeps track of the number of x movements
  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    if(select_language == 0){//means language has not been selected
        select_language = 1;//sends message to print language select prompt
    }
// game engine goes here
    // 1) sample slide pot
    pos = (120*ADCin())/4095;
    if((pos<115)&&(pos>0)){
        oldx1 = player_ship[i].x;
        player_ship[i].x = pos;
        draw_player = 1;
    }

    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)

    button = Switch_In();// 2) read input switches
    now = button&buttonL;
    if((now!=0)&&(last==0)){//shoot sound if left button is pressed
        shoot_init();
        need_to_pause = 0;
        language_selected = 1;
        if(select_language != 3){
            select_language =2;
        }
    }
    last = now;
    shoot_vertically();
    Detect_collision();
    if(button&buttonR){

    }
    else if(button&buttonRR){
        need_to_pause = 1;
        //this will pause the system
    }
    else if(button&buttonU){
        //select english
        select_english = 1;
        select_spanish = 0;
    }
    else if(button&buttonD){
        //select spanish
        select_spanish = 1;
        select_english = 0;
    }

    Move_sinusoidally();
    //Move_vertically();// 3) move sprites
    if(button&shoot){// 4) start sounds
        Sound_Shoot();
    }

    Flag = 1;// 5) set semaphore
    // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}


//this function handles printing the language on the screen at the beginning of the game
void Display_language(void){
    if(display_languages==0){//if languages have not been displayed yet, print them out
        ST7735_FillScreen(ST7735_BLACK);
        ST7735_OutChar(' ');
        ST7735_OutChar(13);
        ST7735_OutChar(' ');
        ST7735_OutString((char *)Phrases[SELECTION_INSTRUCTION][0]); //displays press fire to shoot
        ST7735_OutChar(' ');
        ST7735_OutChar(13);
        ST7735_DrawBitmap(60, 27, Enemy_spaceship[0].image, 16,10);// update ST7735R

        for(phrase_t myPhrase=LANGUAGE; myPhrase<= LANGUAGE; myPhrase++){
          for(Language_t myL=English; myL<= Spanish; myL++){
            ST7735_OutChar(' ');
            ST7735_OutString((char *)Phrases[LANGUAGE][myL]);
            ST7735_OutChar(' ');
            ST7735_OutChar(13);
          }
        }
        display_languages = 1;//language has been printed onto the screen
    }
    if(select_english){//if the player selects english
        ST7735_FillRect( 10,  10,  127,  10, ST7735_Color565(0,0,0) );
        ST7735_SetCursor(1, 1);
        ST7735_OutString((char *)Phrases[SELECTION_INSTRUCTION][0]); //displays press fire to shoot
        ST7735_OutChar(' ');
        ST7735_DrawBitmap(60, 37, black, 16,10);// update ST7735R
        ST7735_DrawBitmap(60, 27, Enemy_spaceship[0].image, 16,10);// update ST7735R
        select_english =0;//no need to refresh english once printed
        english_selected = 1;
        spanish_selected =0;
    }
    else if(select_spanish){//if the player selects spanish
        ST7735_FillRect( 10,  10,  127,  10, ST7735_Color565(0,0,0) );
        ST7735_SetCursor(1, 1);
        ST7735_OutString((char *)Phrases[SELECTION_INSTRUCTION][1]);
        ST7735_OutChar(' ');
        ST7735_DrawBitmap(60, 27, black, 16,10);// update ST7735R
        ST7735_DrawBitmap(60, 37, Enemy_spaceship[0].image, 16,10);// update ST7735R
        select_spanish=0;//no need to refresh spanish once printed
        english_selected = 0;
        spanish_selected =1;
    }

}
//this function pauses the game when the player presses the fire button
void Pause_the_screen(void){
    ST7735_SetCursor(7, 7);
    ST7735_OutString("Paused:");
    ST7735_SetCursor(3, 8);
    ST7735_OutString("Fire to resume");
    reset_screen = 1;
}
void Draw_enemy_on_screen(void){int i;
finished_drawing = 0;
    for(i=0; i < enemies_left;i++){
        if(Enemy_spaceship[i].life == alive && collision_detected == 0){
            ST7735_DrawBitmap(Enemy_spaceship[i].oldx, Enemy_spaceship[i].y, black, 16,10);// update ST7735R
            ST7735_DrawBitmap(Enemy_spaceship[i].oldx, Enemy_spaceship[i].oldy, black, 16,10);// update ST7735R
            ST7735_DrawBitmap(Enemy_spaceship[i].x, Enemy_spaceship[i].y, Enemy_spaceship[i].image, 16,10);// update ST7735R
        }

    }
}
void Draw_player_on_screen(void){
    ST7735_DrawBitmap(oldx1, player_ship[0].y, black__player, 18,8);
    ST7735_DrawBitmap(player_ship[0].x, player_ship[0].y, PlayerShip0, 18,8);
}

void Draw_weapon_on_screen(void){int i=0;
finished_drawing_weapon = 0;
//    for(i=0;i<1;i++){
        if(inventory[i].life == alive && collision_detected ==0){
            ST7735_DrawBitmap(inventory[i].oldx, inventory[i].oldy, black, 16,10);// update ST7735R
            ST7735_DrawBitmap(inventory[i].x, inventory[i].y, inventory[i].image, 16,10);// update ST7735R
        }


//    }

}
void clear_entity(void){int i,number_of_hits;
    finished_clearing_entity =0;
    for(i =0; i<5;i++){
        if((Enemy_spaceship[i].number_of_collisions>3) && (Enemy_spaceship[i].clear_status ==0)){
            ST7735_DrawBitmap(Enemy_spaceship[i].oldx, Enemy_spaceship[i].oldy, black, 16,10);// update ST7735R
            ST7735_DrawBitmap(Enemy_spaceship[i].x, Enemy_spaceship[i].y, black, 16,10);// update ST7735R
            Enemy_spaceship[i].life = dead;
            Enemy_spaceship[i].clear_status = 1;
        }
    }

        ST7735_DrawBitmap(inventory[0].oldx, inventory[0].oldy, black, 16,10);// update ST7735R
        //ST7735_DrawBitmap(inventory[0].x, inventory[0].y, black, 16,10);
        inventory[0].x = pos;
        inventory[0].y =156;
        collision_detected = 0;

}


// ALL ST7735 OUTPUT MUST OCCUR IN MAIN
int main(void){ // final main
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  ST7735_FillScreen(ST7735_BLACK);
  ADCinit();     //PB18 = ADC1 channel 5, slidepot
  Switch_Init(); // initialize switches
  LED_Init();    // initialize LED
  Sound_Init();  // initialize sound
  Init();//starter position of enemy sprites

//  EdgeTriggered_Init();
  TExaS_Init(0,0,&TExaS_LaunchPadLogicPB27PB26); // PB27 and PB26
    // initialize interrupts on TimerG12 at 30 Hz
  TimerG12_IntArm(80000000/30,3);
  // initialize all data structures
  __enable_irq();
  while(1){
      /**
       * gets the oldx and oldy
       * movement is a function of velocity
       * updates position based on the old position, oldx and oldy and shifts the new coordinate x and y
       */
    if(Flag==1){//game engine ready
        if(select_language == 1){//means it's time to select a language
            Display_language();
        }
        else if(language_selected){
            if(select_language ==2 ){
                ST7735_FillScreen(ST7735_BLACK);
                select_language =3;
            }
            if(need_to_pause){
                Pause_the_screen();
            }
            else {
                if(reset_screen == 1){
                    ST7735_FillScreen(ST7735_BLACK);
                    reset_screen = 0;
                }
                //if we dont need to pause, keep running the game as usual
                if(need_to_draw){
                        Draw_enemy_on_screen();
                        need_to_draw =0;
                        finished_drawing = 1;
                    }
                if(draw_player==1){
                    Draw_player_on_screen();
                }
                draw_player = 0;
                //int detect =buttonL&Switch_In();
                if(need_to_shoot==1){
                    Draw_weapon_on_screen();
                    finished_drawing_weapon = 1;
                }
                if(collision_detected){
                    clear_entity();
                    finished_clearing_entity = 1;
                }
                need_to_shoot=0;
                if(end_of_game==1){
                    ST7735_OutString("GAME OVER");
                    ST7735_SetCursor(1, 2);
                    end_of_game = 0;
                }
            }
        }
        Flag = 0; // clear semaphore;// check for end game or level switch// wait for semaphore
    }
  }
}







