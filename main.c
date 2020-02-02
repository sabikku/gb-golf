#include <gb/gb.h> // include this file for Game Boy functions
#include <gb/drawing.h>
#include <stdio.h>
#include <string.h>
#include "data_bkg.c"
#include "data_spr.c"
#include "intro.c"
#include "map.c"
#include "win.c"

// config
#define SCROLL_COOLDOWN 5
#define SWING_COOLDOWN 50
#define FLY_COOLDOWN 50
#define MAX_SPEED 6

// consts/helpers
#define MAP_WIDTH_PIX MapWidth*8
#define MAP_HEIGHT_PIX MapHeight*8
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define STAGE_INTRO 4
#define STAGE_LOOK 0
#define STAGE_SWING 1
#define STAGE_FLY 2
#define STAGE_OUTRO 3

#define CH_SAND 0x03
#define CH_HOLE 0x05

// math
#define abs(x) ((x<0)?-x:x)
#define max(x,y) ((x>y)?x:y)

void init();
void intro_init();
void intro_up();
void look_init();
void look_up();
void look_end();
void swing_init();
void swing_up();
void swing_end();
void fly_init();
void fly_up();
void fly_end();
void outro_init();
void outro_up();
void outro_end();

void window_rewrite(unsigned char* text);

void center_scroll_on(UINT16*, UINT16*);
void win_up();
void move_in_dir(UINT8*, UINT8*, INT8);
char under_ball();
UINT8 hole_near();

UINT16 frame;
UINT8 i, a, b, c, d;
INT16 sa16, sb16;
UINT8 stage;
UINT16 timer_block_input = 0;
UINT8 timer_block_scroll = 0;
UINT8 timer_fly = 0;
UINT8 ball_height = 0;
INT16 ball_x, ball_y;
INT16 p1_ball_x, p1_ball_y;
INT16 p2_ball_x, p2_ball_y;

UINT8 turn = 0;
UINT8 ball_dir=0, ball_speed=1, ball_shoot_speed;
INT16 scroll_x = 0, scroll_y = 0;
INT16 centering_offset_x = 78, centering_offset_y = 76;

UINT8 win_shown = 0;
UINT8 win_y = 192;

unsigned UINT8 speed_to_duration[] = {
    0,7,7,6,6,5,4,3//0, 7, 4, 3, 2, 2
};
unsigned UINT8 speed_to_target_heigth[] = {
    1, 1, 2, 3, 3, 4, 4
};

// ---

void main(){
    init();
    stage = STAGE_INTRO;
    intro_init();

    frame = 0;
    while(1){
        if ( frame == 65535 ) frame = 0; else frame++;
        switch ( stage ) {
            case STAGE_INTRO:
                intro_up();
                break;
            case STAGE_LOOK:
                look_up();
                break;
            case STAGE_SWING:
                swing_up();
                break;
            case STAGE_FLY:
                fly_up();
                break;
            case STAGE_OUTRO:
                outro_up();
                break;
        }
        win_up();
        move_sprite(0, p1_ball_x-scroll_x, p1_ball_y-scroll_y);
        move_sprite(1, p2_ball_x-scroll_x, p2_ball_y-scroll_y);
    }
}

void init() {

    p1_ball_x = 48;
    p1_ball_y = 64;
    p2_ball_x = p1_ball_x;
    p2_ball_y = p1_ball_y;

    OBP0_REG = 0x87; // secondary palette: dark, light, white, black
    
    S_PALETTE;
    SPRITES_8x8;

    DISPLAY_ON;
    SHOW_SPRITES;
    SHOW_WIN;
    SHOW_BKG;

    set_sprite_data(0, 66, Spr); // all sprites
    set_bkg_data(0,128,Bkg); // all tiles
    set_win_data(0,127,Bkg);

    set_win_tiles(0,0,WinWidth,WinHeight,Win);

    move_win(7,win_y);

    set_sprite_tile(0, 1); // ball
    set_sprite_tile(1, 1); // ball

}

void intro_init() {
    set_bkg_tiles(0,0,IntroWidth,IntroHeight,Intro);
    win_shown = 1;
}

void intro_up() {
    if ( timer_block_scroll > 0 ) {
        timer_block_scroll--;
    } else {
        timer_block_scroll = 255;
        if ( turn )
            window_rewrite("   PRESS START");
        else
            window_rewrite("           ");
        if ( turn ) turn = 0;
        else turn = 1;

        if ( joypad() == J_START ) {
            waitpadup();
            set_bkg_tiles(0,0,MapWidth,MapHeight,Map);

            turn = 0;
            stage = STAGE_LOOK;
            look_init();
        }
    }
}

void look_init() {
    set_sprite_tile(2, 16);
    set_sprite_tile(3, 17);
    set_sprite_tile(4, 18);
    set_sprite_tile(5, 19);
    win_shown = 0;
}

void look_end() {
    set_sprite_tile(2, 0);
    set_sprite_tile(3, 0);
    set_sprite_tile(4, 0);
    set_sprite_tile(5, 0);
}

void look_up() {

    a = (frame/64)%4;
    move_sprite(2, 16-a, 84);
    move_sprite(3, 84, 24-a);
    move_sprite(4, SCREEN_WIDTH-8+a, 84);
    move_sprite(5, 84, SCREEN_HEIGHT+a);

    if ( timer_block_input ) {
        timer_block_input--;
    } else {
        if(joypad()==J_RIGHT) {
            if (scroll_x<(MAP_WIDTH_PIX-SCREEN_WIDTH)) {
                scroll_x++;
                move_bkg(scroll_x,scroll_y);
            }
            timer_block_input=SCROLL_COOLDOWN;
        }
        
        if(joypad()==J_LEFT) {
            if (scroll_x>0) {
                scroll_x--;
                move_bkg(scroll_x,scroll_y);
            }
            timer_block_input=SCROLL_COOLDOWN;
        }
        if(joypad()==J_DOWN) {
            if (scroll_y<(MAP_HEIGHT_PIX-SCREEN_HEIGHT)) {
                scroll_y++;
                move_bkg(scroll_x,scroll_y);
            }
            timer_block_input=SCROLL_COOLDOWN;
        }
        
        if(joypad()==J_UP) {
            if (scroll_y>0) {
                scroll_y--;
                move_bkg(scroll_x,scroll_y);
            }
            timer_block_input=SCROLL_COOLDOWN;
        }
        
        if(joypad()==J_A || joypad()==J_B) {
            waitpadup();
            stage = STAGE_SWING;
            look_end();
            swing_init();
        }
    }
}

void swing_init() {
    if ( turn == 0 ) {
        window_rewrite( "P1 A=SHOOT B=LOOK" );
    } else {
        window_rewrite( "P2 A=SHOOT B=LOOK" );
    }
    win_shown=1;
    set_sprite_tile(2, 8);
    set_sprite_tile(3, 8);
    set_sprite_tile(4, 8);
}

void swing_end() {
    set_sprite_tile(2, 0);
    set_sprite_tile(3, 0);
    set_sprite_tile(4, 0);
    ball_shoot_speed = ball_speed;
}

void swing_up() {
    ball_x = ( turn == 0 ) ? p1_ball_x : p2_ball_x;
    ball_y = ( turn == 0 ) ? p1_ball_y : p2_ball_y;
    center_scroll_on( &ball_x, &ball_y );

    // trajectory
    a=(UINT8)(ball_x-scroll_x);
    b=(UINT8)(ball_y-scroll_y);
    move_in_dir(&a, &b, 1);
    move_sprite(2, a, b);
    move_in_dir(&a, &b, ball_speed);
    move_sprite(3, a, b);
    move_in_dir(&a, &b, ball_speed+1);
    move_sprite(4, a, b);

    centering_offset_x = 78; centering_offset_y = 76;
    move_in_dir(&centering_offset_x, &centering_offset_y, -5);

    // input
    if ( timer_block_input ) {
        timer_block_input--;
    } else {
        if(joypad()==J_RIGHT) {
            if (ball_dir < 15) {
                ball_dir++;
            } else {
                ball_dir = 0;
            }
            timer_block_input=SWING_COOLDOWN;
        }
        
        if(joypad()==J_LEFT) {
            if (ball_dir > 0) {
                ball_dir--;
            } else {
                ball_dir = 15;
            }
            timer_block_input=SWING_COOLDOWN;
        }
        if(joypad()==J_UP) {
            if (ball_speed < MAX_SPEED) {
                ball_speed++;
                timer_block_input=SWING_COOLDOWN;
            }
        }
        
        if(joypad()==J_DOWN) {
            if (ball_speed > 1) {
                ball_speed--;
                timer_block_input=SWING_COOLDOWN;
            }
        }
    }
    if(joypad()==J_B) {
        waitpadup();
        stage = STAGE_LOOK;
        swing_end();
        look_init();
    }
    if(joypad()==J_A) {
        waitpadup();
        stage = STAGE_FLY;
        swing_end();
        fly_init();
    }
}


void fly_init() {
    win_shown = 0;
    timer_block_input = 1;
    timer_fly = 0;
    // sound
    NR52_REG = 0x80;
    NR51_REG = 0x11;
    NR50_REG = 0x77;
    NR10_REG = 0x1E;
    NR11_REG = 0x10;
    NR12_REG = 0xF3;
    NR13_REG = 0x00;
    NR14_REG = 0x87;
}

void fly_end() {
    set_sprite_tile(turn, 1);

    if ( hole_near() == 1 ) {
        stage = STAGE_OUTRO;
        set_sprite_tile(turn, 0); // hide the ball
        outro_init();
    } else {
        stage = STAGE_SWING;
        if ( turn == 0 ) turn = 1; else turn = 0;
        swing_init();
    }
}

void fly_up() {
    ball_x = ( turn == 0 ) ? p1_ball_x : p2_ball_x;
    ball_y = ( turn == 0 ) ? p1_ball_y : p2_ball_y;
    center_scroll_on( &ball_x, &ball_y );

    centering_offset_x = 78; centering_offset_y = 76;
    move_in_dir(&centering_offset_x, &centering_offset_y, -5);

    // physics!
    a = speed_to_duration[ ball_speed ];
    if ( timer_block_input > 0 ) {
        timer_block_input--;
        // height
        if ( timer_fly < (a >> 1) + (a >> 3) ) {
            // first half - higher
            if ( ball_height < speed_to_target_heigth[ ball_speed ] ) {
                ball_height=speed_to_target_heigth[ ball_speed ];
                set_sprite_tile(turn, ball_height);
            } else if ( ball_height > 1 ) {
                ball_height--;
                set_sprite_tile(turn, ball_height);
            }
        }

    } else {
        if ( timer_fly < a ) {
            timer_fly++;
        } else {
            timer_fly = 0;
            ball_speed--;
            if ( ball_speed == 1 && hole_near ) {
                fly_end();
                return;
            }
        }

        if ( ( under_ball() == CH_SAND ) && ball_speed == 1 ) {
            if ( ball_shoot_speed == 1 ) {
                // slow down
                timer_fly++;
            } else {
                // stop
                ball_speed = 0;
            }
        }

        if ( ball_speed >= 1 ) {
            move_in_dir(&ball_x, &ball_y, 1);
            timer_block_input = FLY_COOLDOWN;
        } else {
            ball_speed = 1;
            fly_end();
            return;
        }
    }

    if ( turn == 0 ) {
        p1_ball_x = ball_x;
        p1_ball_y = ball_y;
    } else {
        p2_ball_x = ball_x;
        p2_ball_y = ball_y;
    }
}

void outro_init() {
    win_shown = 1;
    if ( turn == 0 ) {
        window_rewrite( "P1 WON" );
    } else {
        window_rewrite( "P2 WON" );
    }
    timer_block_input = 2000;
}

void outro_up() {
    if ( timer_block_input > 0 ) {
        timer_block_input--;
    } else {
        stage = STAGE_INTRO;
        intro_init();
        p1_ball_x = 48;
        p1_ball_y = 64;
        p2_ball_x = p1_ball_x;
        p2_ball_y = p1_ball_y;
        set_sprite_tile(0, 1); // ball
        set_sprite_tile(1, 1); // ball
        timer_block_input = 0;
        timer_block_scroll = 0;
        timer_fly = 0;
        ball_height = 0;
        turn = 0;
        ball_dir=0;
        ball_speed=1;
        scroll_x = 0;
        scroll_y = 0;
        centering_offset_x = 78;
        centering_offset_y = 76;
        move_bkg(0,0);
    }
}

void outro_end() {

}

void window_rewrite(unsigned char* text) {
    a = strlen( text );
    for (i = 0; i < 18; i++) {
        if ( i < a ) {
            switch (text[i]) {
                case ' ':
                    b=Win[20];
                    break;
                case '=':
                    b=90;
                    break;
                case '.':
                    b=101;
                    break;
                default:
                    if ( text[i] < 'A') {
                        b = 91+(text[i]-'0');    
                    } else {
                        b = 64+(text[i]-'A');
                    }
            }
        } else {
            b = Win[20]; //empty
        }
        Win[21+i] = b;
    }
    set_win_tiles(0,0,WinWidth,WinHeight,Win);
}

void win_up() {
    if ( win_shown && win_y > 124 ) {
        win_y -= 1;
        move_win(7,win_y);
    } else if ( !win_shown && win_y < 192 ) {
        win_y += 1;
        move_win(7,win_y);
    }
}

// centers camera on the ball
void center_scroll_on(UINT16 *x, UINT16 *y) {
    if ( timer_block_scroll > 0 ) {
        timer_block_scroll--;
        return;
    }
    sa16 = scroll_x - (*x) + centering_offset_x; // diff x

    a = ( sa16 > 16 || sa16 < -16 ) ? 3 : 1;
    if ( sa16 > 0 && scroll_x > 0 ) {
        scroll_x -= a;
    }
    if ( sa16 < 0 && scroll_x < MAP_WIDTH_PIX-SCREEN_WIDTH ) {
        scroll_x += a;
    }

    sb16 = scroll_y - (*y) + centering_offset_y; // diff y
    a = ( sb16 > 16 || sb16 < -16 ) ? 2 : 1;
    if ( sb16 > 0 && scroll_y > 0 ) {
        scroll_y -= a;
    }
    if ( sb16 < 0 && scroll_y < MAP_HEIGHT_PIX-SCREEN_HEIGHT ) {
        scroll_y += a;
    }
    if ( sa16 != 0 || sb16 != 0 ) {
        move_bkg(scroll_x,scroll_y);
        timer_block_scroll = 15;
    }
}

void move_in_dir(UINT8* x, UINT8* y, INT8 d) {
    switch (ball_dir) {
        case 0:  (*x) += 4*d; (*y) += 0; break;
        case 1:  (*x) += 4*d; (*y) += 2*d; break;
        case 2:  (*x) += 3*d; (*y) += 3*d; break;
        case 3:  (*x) += 2*d; (*y) += 4*d; break;
        case 4:  (*x) += 0; (*y) += 4*d; break;
        case 5:  (*x) -= 2*d; (*y) += 4*d; break;
        case 6:  (*x) -= 3*d; (*y) += 3*d; break;
        case 7:  (*x) -= 4*d; (*y) += 2*d; break;
        case 8:  (*x) -= 4*d; (*y) += 0; break;
        case 9:  (*x) -= 4*d; (*y) -= 2*d; break;
        case 10: (*x) -= 3*d; (*y) -= 3*d; break;
        case 11: (*x) -= 2*d; (*y) -= 4*d; break;
        case 12: (*x) -= 0; (*y) -= 4*d; break;
        case 13: (*x) += 2*d; (*y) -= 4*d; break;
        case 14: (*x) += 3*d; (*y) -= 3*d; break;
        case 15: (*x) += 4*d; (*y) -= 2*d; break;
    }
}

char under_ball() {
    return Map[ (ball_x-8)/8 + (((ball_y-8)/8)*MapWidth) ];
}

UINT8 hole_near() {
    if ( Map[ (ball_x- 8)/8 + (((ball_y-8)/8)*MapWidth) ] == CH_HOLE ) return 1;
    if ( Map[ (ball_x-16)/8 + (((ball_y-8)/8)*MapWidth) ] == CH_HOLE ) return 1;
    if ( Map[ (ball_x- 0)/8 + (((ball_y-8)/8)*MapWidth) ] == CH_HOLE ) return 1;
    if ( Map[ (ball_x- 8)/8 + (((ball_y-16)/8)*MapWidth) ] == CH_HOLE ) return 1;
    if ( Map[ (ball_x-16)/8 + (((ball_y-16)/8)*MapWidth) ] == CH_HOLE ) return 1;
    if ( Map[ (ball_x- 0)/8 + (((ball_y-16)/8)*MapWidth) ] == CH_HOLE ) return 1;
    if ( Map[ (ball_x- 8)/8 + (((ball_y-0)/8)*MapWidth) ] == CH_HOLE ) return 1;
    if ( Map[ (ball_x-16)/8 + (((ball_y-0)/8)*MapWidth) ] == CH_HOLE ) return 1;
    if ( Map[ (ball_x- 0)/8 + (((ball_y-0)/8)*MapWidth) ] == CH_HOLE ) return 1;
    return 0;
}