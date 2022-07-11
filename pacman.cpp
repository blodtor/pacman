/**
Super Turbo Net Pac-Man
реализация Pac-Man для DOS и GNU/Linux

Для Linux собирается gcc
> gcc pacman.cpp

Для DOS собирается Open Watcom - http://www.openwatcom.org/
> wmake

Драйвер для Intel 8∕16 LAN Adapter
https://www.vogonswiki.com/index.php/Intel_8%E2%88%9516_LAN_Adapter

A Guide to Networking in DOS
http://dosdays.co.uk/topics/networking_in_dos.php

mTCP - TCP/IP applications for your PC compatible retro-computers
http://brutmanlabs.org/mTCP/
*/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

#include <errno.h>

// Linux
#ifdef __linux__
#include <sys/time.h>
#include <termios.h>
#else
// DOS
#include <dos.h>   // _dos_gettime
#include <conio.h> // kbhit
#include <graph.h> // 2D graphics 
#endif

// скорость перехода пакмена с одной клетки на другую в милисекундах
const long PACMAN_SPEED = 150L;
// скорость перехода Красного призрака с одной клетки на другую в милисекундах
const long RED_SPEED = 150L;
// Еда
const char FOOD   = '.';
// Поверап позваляющий есть призраков
const char POWER_FOOD   = '*';
// Стена лаберинта
// const char WALL = '#';
// Дверь в комнату призраков
const char DOOR = '-';
// Ни кем не занятая клетка
const char EMPTY  = ' ';
// Pac-Man
const char PACMAN = 'O';
// Красный призрак (SHADOW или BLINKY)
const char RED    = '^';
// Розовый призрак (SPEEDY или PIKKEY)
const char PINK   = '<';
// Голубой призрак (BASHFUL или INKY)
const char BLUE  = '>';
// Желтый призрак (POKEY или CLYDE)
const char YELOW  = '$';
// Красный призрак когда его можно съесть (SHADOW или BLINKY)
const char SHADOW = '@';
// Вишня
const char CHERRY = '%';
// Время через которое появляеться вишня
const long CHERRY_TIME = 30000L;
// время через которое RED перестает быть съедобным
const long RED_TIME = 20000L;
// количество очков за поедание вишни
const int SCORE_CHERY_BONUS = 200;
// количество очков за поедание POWER_FOOD
const int SCORE_POWER_BONUS = 25;
// количество очков за поедание RED
const int SCORE_RED_EAT = 50;
// черный цвет
const int COLOR_BLACK = 0;
// синий цвет
const int COLOR_BLUE = 1;
// зеленый цвет
const int COLOR_GREEN = 2;
// голубой цвет
const int COLOR_LIGHT_BLUE = 3;
// красный цвет
const int COLOR_RED = 4;
// пурпурный цвет
const int COLOR_PURPLE = 5;
// коричневый цвет
const int COLOR_BROWN = 6;
// берюзовый цвет
const int COLOR_TURQUOISE = 11;
// желтый цвет
const int COLOR_YELLOW = 14;
// белый цвет
const int COLOR_WHITE = 15;
// Стена - угол верх лево полный
const char WALL_7 = '7';
// Стена - верхняя
const char WALL_8 = '8';
// Стена - угол верх право полный
const char WALL_9 = '9';
// Стена - левая
const char WALL_4 = '4';
// Стена - полная
const char WALL_5 = '5';
// Стена - правая
const char WALL_6 = '6';
// Стена - угол низ лево полная
const char WALL_1 = '1';
// Стена - угол низ право полная
const char WALL_3 = '3';
// Стена - низ узкая
const char WALL_2 = '2';
// Стена - скругление низ лево
const char WALL_0 = '0';
// Стена - скругление низ право
const char WALL_I = 'i';
// Стена - скругление верх лево
const char WALL_L = 'l';
// Стена - скругление верх право
const char WALL_D = 'd';
// Стена - низ полная
const char WALL_X = 'x';
// Стена - скругление низ лево полная
const char WALL_J  = 'j';
// Стена - скругление низ право полная
const char WALL_F = 'f';
// Стена - угол низ лево
const char WALL_S = 's';
// Стена - угол низ право
const char WALL_E = 'e';
// Стена - угол верх право
const char WALL_M = 'm';
// Стена - угол верх лево
const char WALL_N = 'n';
// Стена - угол низ право тонкий
const char WALL_Y = 'y';
// Стена - угол низ лево тонкий
const char WALL_Z = 'z';

// текущие координаты PACMAN
int pacmanX = 14;
int pacmanY = 17;

// последний спрайт pacman
int pacmanSprite = 1;

// направление движение PACMAN
int dx=0;
int dy=0;

// старые координаты PACMAN
int oldX = pacmanX;
int oldY = pacmanY;

// направление движения RED (SHADOW или BLINKY)
int dxRed=1;
int dyRed=0;

// координаты RED (SHADOW или BLINKY)
int redX = 21;
int redY = 10;

// последний спрайт RED
int redSprite = 1;

// 1 - RED в режиме охоты
// 0 - PACMAN съел POWER_FOOD и RED сейчас съедобен
int redFlag = 1;

// время когда RED стал съедобным последний раз
long redTime = 0;

// 1 - Вишня есть
// 0 - Вишни нет
int cherryFlag = 0;

// надо перерисовать черешню
int refreshCherry = 0;

// x координата черешни
int cherryX = 15;
// y координата черешни
int cherryY = 10;

// x координата двери
int doorX = 15;
// y координата двери
int doorY = 8;

// надо перерисовать дверь
int refreshDoor = 0;


// старые координаты RED (SHADOW или BLINKY)
int oldXRed = redX;
int oldYRed = redY;

// что лежит на клетке с RED (SHADOW или BLINKY)
char oldRedVal = FOOD;

// количество съеденных FOOD
int score = 1;

// бонус за съедание RED (SHADOW или BLINKY)
int redBonus = 0;

// бонус за съедание POWER_FOOD
int powerBonus = 0;

// бонус за съеденные вишни
int cherryBonus = 0;

// размер карты по x
int mapSizeX = 23;
// размер карты по y
int mapSizeY = 32;

// количество FOOD и POWER_FOOD которое нужно съесть чтоб выиграть
int foodToWIN = 0;

// время когда игру запустили
long startTime;

// время последнего обновления положения pacman
long pacmanLastUpdateTime;

// время поседнего обновления RED
long redLastUpdateTime;

// карта
char map[23][32] = {
    "7888888888888895788888888888889",
    "4.............654.............6",
    "4*i220.i22220.l8d.i22220.i220*6",
    "4.............................6",
    "4.i220.fxj.i22mxn220.fxj.i220.6",
    "4......654....654....654......6",
    "1xxxxj.65s220.l8d.222e54.fxxxx3",
    "555554.654...........654.655555",
    "555554.654.fxxj-fxxj.654.655555",
    "88888d.l8d.678d l894.l8d.l88888",
    "...........64     64...........",
    "xxxxxj.fxj.61xxxxx34.fxj.fxxxxx",
    "555554.654.l8888888d.654.655555",
    "555554.654...........654.655555",
    "78888d.l8d.i22mxn220.l8d.l88889",
    "4.............654.............6",
    "4.i2mj.i22220.l8d.i22220.fn20.6",
    "4*..64...................64..*6",
    "s20.ld.fxj.i22mxn220.fxj.ld.i2e",
    "4......654....654....654......6",
    "4.i2222y8z220.l8d.i22y8z22220.6",
    "4.............................6",
    "1xxxxxxxxxxxxxxxxxxxxxxxxxxxxx3",
    };

/**
 * Спрайт pac-man
 * 1 вид
 */
char SPRITE_PACMAN_LEFT1[11][12] = {
        "00011111000",
        "00111111100",
        "01111111110",
        "00011111111",
        "00000111111",
        "00000001111",
        "00000111111",
        "00011111111",
        "01111111110",
        "00111111100",
        "00011111000",
    };

/**
 * Спрайт pac-man
 * 2 вид
 */
char SPRITE_PACMAN_LEFT2[11][12] = {
        "00111111000",
        "00011111100",
        "00001111110",
        "00000111111",
        "00000011111",
        "00000001111",
        "00000011111",
        "00000111111",
        "00001111110",
        "00011111100",
        "00111111000",
};

/**
 * Спрайт pac-man
 * 3 вид
 */
char SPRITE_PACMAN_FULL[11][12] = {
        "00011111000",
        "00111111100",
        "01111111110",
        "11111111111",
        "11111111111",
        "11111111111",
        "11111111111",
        "11111111111",
        "01111111110",
        "00111111100",
        "00011111000",

};

/**
 * Спрайт pac-man
 * 4 вид
 */
char SPRITE_PACMAN_RIGHT1[11][12] = {
        "00011111000",
        "00111111100",
        "01111111110",
        "11111111000",
        "11111100000",
        "11110000000",
        "11111100000",
        "11111111000",
        "01111111110",
        "00111111100",
        "00011111000",
    };

/**
 * Спрайт pac-man
 * 5 вид
 */
char SPRITE_PACMAN_RIGHT2[11][12] = {
        "00011111100",
        "00111111000",
        "01111110000",
        "11111100000",
        "11111000000",
        "11110000000",
        "11111000000",
        "11111100000",
        "01111110000",
        "00111111000",
        "00011111100",
};

/**
 * Спрайт pac-man
 * 6 вид
 */
char SPRITE_PACMAN_UP1[11][12] = {
        "00000000000",
        "00100000100",
        "01100000110",
        "11110001111",
        "11110001111",
        "11111011111",
        "11111011111",
        "11111111111",
        "01111111110",
        "00111111100",
        "00011111000",
    };

/**
 * Спрайт pac-man
 * 7 вид
 */
char SPRITE_PACMAN_UP2[11][12] = {
        "00000000000",
        "00000000000",
        "10000000001",
        "11000000011",
        "11100000111",
        "11110001111",
        "11111011111",
        "11111111111",
        "01111111110",
        "00111111100",
        "00011111000",
};

/**
 * Спрайт pac-man
 * 8 вид
 */
char SPRITE_PACMAN_DOWN1[11][12] = {
        "00011111000",
        "00111111100",
        "01111111110",
        "11111111111",
        "11111011111",
        "11111011111",
        "11110001111",
        "11110001111",
        "01100000110",
        "00100000100",
        "00000000000",
    };

/**
 * Спрайт pac-man
 * 9 вид
 */
char SPRITE_PACMAN_DOWN2[11][12] = {
        "00011111000",
        "00111111100",
        "01111111110",
        "11111111111",
        "11111011111",
        "11110001111",
        "11100000111",
        "11000000011",
        "10000000001",
        "00000000000",
        "00000000000",
};


/**
 * Спрайт привидения которое хочет съесть нас
 * 1 вид
 */
char SPRITE_RED_SPIRIT_LEFT1[14][15] = {
        "00000111100000",
        "00011111111000",
        "00111111111100",
        "01111111111110",
        "01331111331110",
        "03333113333111",
        "12233112233111",
        "12233112233111",
        "11331111331111",
        "11111111111111",
        "11111111111111",
        "11111111111111",
        "11110111101111",
        "01100011000110",

};

/**
 * Спрайт привидения которое хочет съесть нас
 * 2 вид
 */
char SPRITE_RED_SPIRIT_LEFT2[14][15] = {
        "00000111100000",
        "00011111111000",
        "00111111111100",
        "01111111111110",
        "01331111331110",
        "03333113333111",
        "12233112233111",
        "12233112233111",
        "11331111331111",
        "11111111111111",
        "11111111111111",
        "11111111111111",
        "11011100111011",
        "10001100110001",
};

/**
 * Спрайт привидения которое хочет съесть нас
 * 3 вид
 */
char SPRITE_RED_SPIRIT_RIGHT1[14][15] = {
        "00000111100000",
        "00011111111000",
        "00111111111100",
        "01111111111110",
        "01113311113310",
        "11133331133330",
        "11133221133221",
        "11133221133221",
        "11113311113311",
        "11111111111111",
        "11111111111111",
        "11111111111111",
        "11110111101111",
        "01100011000110",
};

/**
 * Спрайт привидения которое хочет съесть нас
 * 4 вид
 */
char SPRITE_RED_SPIRIT_RIGHT2[14][15] = {
        "00000111100000",
        "00011111111000",
        "00111111111100",
        "01111111111110",
        "01113311113310",
        "11133331133330",
        "11133221133221",
        "11133221133221",
        "11113311113311",
        "11111111111111",
        "11111111111111",
        "11111111111111",
        "11011100111011",
        "10001100110001",
};


/**
 * Спрайт привидения которое можно съесть
 * 1 вид
 */
char SPRITE_SHADOW_SPIRIT1[14][15] = {
        "00000111100000",
        "00011111111000",
        "00111111111100",
        "01111111111110",
        "01111111111110",
        "01111111111110",
        "11112211221111",
        "11112211221111",
        "11111111111111",
        "11111111111111",
        "11331133113311",
        "13113311331131",
        "11110111101111",
        "01100011000110",
    };

/**
 * Спрайт привидения которое можно съесть
 * 2 вид
 */
char SPRITE_SHADOW_SPIRIT2[14][15] = {
        "00000111100000",
        "00011111111000",
        "00111111111100",
        "01111111111110",
        "01111111111110",
        "01111111111110",
        "11112211221111",
        "11112211221111",
        "11111111111111",
        "11111111111111",
        "11331133113311",
        "13113311331131",
        "11011100111011",
        "10001100110001",
};

/**
 * Спрайт вишни
 */
char SPRITE_CHERRY[12][13] = {
        "000000000022",
        "000000002222",
        "000000220200",
        "000002000200",
        "011121002000",
        "111211020000",
        "131100121100",
        "113101121110",
        "011101111110",
        "000001311110",
        "000001131110",
        "000000111100",
    };


// активная видео страница при запуске  
int old_apage;

// отображаемая видео страница  
int old_vpage;

int activePage = 0;

#ifdef __linux__
/**
 *
 * Linux
 *
 * Нжата ли кнопка клавиатуры
 * return - 1 да, 0 нет
 */
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

/**
 *
 * Linux
 *
 * Какая кнопка клавиатуры нажата
 * return - код клавиши
 */
int getch()
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

/**
 * Текущее время в милисекундах
 * printf("milliseconds: %lld\n", milliseconds); - распечатать
 *
 * return - милисекунды
 */

/**  Linux */
long long current_timestamp() {
    struct timeval te;
    // получить текущее время
    gettimeofday(&te, nullptr);
    // вычисляем милисекунды
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    return milliseconds;
}

#else
/** DOS **/
/* Установить разрешение 320 x 200 с 16 цетами 
 */
void setVideoMode_DOS() {
    // 320 x 200 16 colors
    _setvideomode(_MRES16COLOR); 
}

/** DOS **/
/*
 * Получить активную видео страницу  
 */
int getGetActivePage_DOS() {
    return _getactivepage();
}

/** DOS **/
/*
 * Получить отображаемую видео страницу  
 */
int getVisualPage_DOS() {
    return _getvisualpage();
}

/** DOS **/
/*
 * Установить назад видеорежим что был при старте
 */
void setBackStartVideoMode_DOS(int old_apage, int old_vpage) {
    _setactivepage(old_apage);
    _setvisualpage(old_vpage);
    _setvideomode(_DEFAULTMODE);

    printf("   Super Turbo Net Pac-Man\n");
    printf("\nScore %d", score + redBonus + powerBonus + cherryBonus);
    printf("\n          GAME OVER!\n");
}

 void drawEmpty(int x, int y, int c) {
    int dx=8*x-3, dy=8*y-2; // смещение
    _setcolor(c);
    for (int i=0; i<14; i++) {
        for (int j=0; j<14; j++) {
            _setpixel(j+dx,i+dy); // рисуем точку
        }
    }
 }

 void drawUpWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=0;j<6;j++) {
            _setpixel(i+dx,j+dy);
        }
    }
 }

void drawLeftWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<4;i++) {
        for (int j=0;j<8;j++) {
            _setpixel(i+dx,j+dy);
        }
    }
 }


void drawRightWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=4;i<8;i++) {
        for (int j=0;j<8;j++) {
            _setpixel(i+dx,j+dy);
        }
    }
 }

void drawDownWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=5;j<6;j++) {
            _setpixel(i+dx,j+dy);
        }
    }
 }

void drawDownFullWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=5;j<8;j++) {
            _setpixel(i+dx,j+dy);
        }
    }
 }


void drawUpLeftCornerWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=0;j<8;j++) {
            if (i<4 || j<6) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }

void drawUpLeftCornerFullWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        if (i<4) {
            for (int j=5;j<8;j++) {
                _setpixel(i+dx,j+dy);
            }
        }

        _setpixel(i+dx,5+dy);
    }
 }

 void drawDownLeftCornerWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=0;j<8;j++) {
            if (i<4 || j>4) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }

 void drawDownLeftCornerFullWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        if (i<4) {
           for (int j=0;j<8;j++) {
                _setpixel(i+dx,j+dy);
           }
        }
        _setpixel(i+dx,5+dy);
    }
 }

 void drawDownLeftCornerSmallWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        if (i<4) {
           for (int j=0;j<6;j++) {
                _setpixel(i+dx,j+dy);
           }
        }
        _setpixel(i+dx,5+dy);
    }
 }


 void drawDownRightCornerWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=0;j<8;j++) {
            if (i>3 || j>4) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }


 void drawDownRightCornerFullWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        if (i>3) {
            for (int j=0;j<8;j++) {
                _setpixel(i+dx,j+dy);
            }
        }
       _setpixel(i+dx,5+dy);

    }
 }

 void drawDownRightCornerSmallWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        if (i>3) {
            for (int j=0;j<6;j++) {
                _setpixel(i+dx,j+dy);
            }
        }
       _setpixel(i+dx,5+dy);

    }
 }

void drawUpRightCornerWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=0;j<8;j++) {
            if (i>3 || j<6) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }

void drawUpRightCornerFullWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        if (i>3) {
            for (int j=5;j<8;j++) {
                _setpixel(i+dx,j+dy);
            }
        } else {
            _setpixel(i+dx,5+dy);
        }

    }
 }

void drawFullWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=0;j<8;j++) {
            _setpixel(i+dx,j+dy);
        }
    }
 }

void drawLeftDownPointWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=5;j<6;j++) {
            if (i<4 && j>4) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }

void drawLeftDownPointFullWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=5;j<8;j++) {
            if (i<4 && j>4) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }

 void drawRightDownPointWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=5;j<6;j++) {
            if (i>3 && j>4) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }

 void drawRightDownPointFullWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=5;j<8;j++) {
            if (i>3 && j>4) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }

 void drawRightUpPointWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=0;j<8;j++) {
            if (i>3 && j<6) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }

 void drawLeftUpPointWall(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=0;i<8;i++) {
        for (int j=0;j<8;j++) {
            if (i<4 && j<6) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }


 void drawFood(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    _setpixel(3+dx,4+dy);
    _setpixel(3+dx,5+dy);
    _setpixel(4+dx,5+dy);
    _setpixel(4+dx,4+dy);

 }

 void drawPowerFood(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=2;i<6;i++) {
        for (int j=2;j<6;j++) {
            if (i>=2 && i<6 || j>2 && j<6) {
                _setpixel(i+dx,j+dy);
            }
        }
    }
 }

 void drawDoor(int x, int y, int c) {
    int dx=8*x, dy=8*y;

    _setcolor(c);
    for(int i=-2;i<11;i++) {
       _setpixel(i+dx,6+dy);
    }

 }


/**
 * Нарисовать в DOS PAC-MAN
 * x - координата х где нарисовать
 * y - координата y где нарисовать
 * c0 - цвет фона
 * c1 - цвет PAC-MAN
 * p - спрайт привидения 11x11 пикселей - 2 цвета
 *     '0' - цвет фона
 *     '1' - цвет PAC-MAN
 */
void drawPacMen(int x, int y, int c0, int c1, char p[11][12]) {
    int dx=8*x-1, dy=8*y;

    for (int i=0; i<11; i++) {
        for (int j=0; j<11; j++) {
            if (p[i][j] == '1') {
                _setcolor(c1); // цвет PAC-MAN
            } else {
                _setcolor(c0); // цвет фона все остальное
            }
            _setpixel(j+dx,i+dy); // рисуем точку
        }
    }
 }

/**
 * Нарисовать в DOS привидение
 * которое хочет съесть нас
 * x - координата х где нарисовать
 * y - координата y где нарисовать
 * c0 - цвет фона
 * c1 - цвет привидения
 * c2 - цвет зрачков
 * c3 - цвет белков глаза
 * p - спрайт привидения 14x14 пикселей - 4 цвета
 *     '0' - цвет фона
 *     '1' - цвет привидения
 *     '2' - цвет зрачков
 *     '3' - цвет белков глаза
 */
 void drawRed(int x, int y, int c0, int c1, int c2, int c3, char p[14][15]) {
    int dx=8*x-3, dy=8*y-2; // смещение
    for (int i=0; i<14; i++) {
        for (int j=0; j<14; j++) {
            if (p[i][j] == '1') {
                _setcolor(c1); // цвет призрака
            } else if (p[i][j] == '2') {
                _setcolor(c2); // цвет зрачков
            } else if (p[i][j] == '3') {
                _setcolor(c3); // цвет белков глаз
            } else {
                _setcolor(c0); // цвет фона все остальное
            }
            _setpixel(j+dx,i+dy); // рисуем точку
        }
    }
 }


/**
 * Нарисовать в DOS привидение
 * которое можно съесть
 * x - координата х где нарисовать
 * y - координата y где нарисовать
 * c0 - цвет фона
 * c1 - цвет привидения
 * c2 - цвет рта
 * c3 - цвет глаз
 * p - спрайт привидения 14x14 пикселей - 4 цвета
 *     '0' - цвет фона
 *     '1' - цвет привидения
 *     '2' - цвет рта
 *     '3' - цвет глаз
 */
 void drawShadow(int x, int y, int c0, int c1, int c2, int c3, char p[14][15]) {
    int dx=8*x-3, dy=8*y-2; // смещение
    for (int i=0; i<14; i++) {
        for (int j=0; j<14; j++) {
            if (p[i][j] == '1') {
                _setcolor(c1); // цвет призрака
            } else if (p[i][j] == '2') {
                _setcolor(c2); // цвет рта
            } else if (p[i][j] == '3') {
                _setcolor(c3); // цвет глаз
            } else {
                _setcolor(c0); // цвет фона все остальное
            }
            _setpixel(j+dx,i+dy); // рисуем точку
        }
    }
 }

/**
 * Нарисовать в DOS черешню
 * x - координата х где нарисовать
 * y - координата y где нарисовать
 * c0 - цвет фона
 * c1 - цвет черешни
 * c2 - цвет веточки
 * c3 - цвет отблиска
 * p - спрайт привидения 14x14 пикселей - 4 цвета
 *     '0' - цвет фона
 *     '1' - цвет черешни
 *     '2' - цвет веточки
 *     '3' - цвет отблиска
 */
 void drawCherry(int x, int y, int c0, int c1, int c2, int c3, char p[12][13]) {
    int dx=8*x-2, dy=8*y-3;

    for (int i=0; i<12; i++) {
        for (int j=0; j<12; j++) {
            if (p[i][j] == '1') {
                _setcolor(c1); // цвет черешни
            } else if (p[i][j] == '2') {
                _setcolor(c2); // цвет веточки
            } else if (p[i][j] == '3') {
                _setcolor(c3); // цвет отблисков
            } else {
                _setcolor(c0); // цвет фона все остальное
            }
            _setpixel(j+dx,i+dy); // рисуем точку
        }
    }

 }

/** DOS */
/* 
 *  Перерисовать карту в графическом режим
 */
void draw_DOS() {
    if (activePage == 0) {
        activePage = 1;
    } else {
        activePage = 0;
    }
    _setactivepage(activePage);

    for (int i = 0; i < mapSizeX; i++) {
        for (int j =0; j < mapSizeY ; j++) {
            char val = map[i][j] ;
            if (val == WALL_8) {
                drawUpWall(j, i, COLOR_BLUE);
            } else if(val == WALL_4) {
                drawLeftWall(j, i, COLOR_BLUE);
            } else if (val == WALL_7) {
                drawUpLeftCornerWall(j, i, COLOR_BLUE);
            } else if (val == WALL_6) {
                drawRightWall(j, i, COLOR_BLUE);
            } else if (val == WALL_1) {
                drawDownLeftCornerWall(j, i, COLOR_BLUE);
            } else if (val == WALL_9) {
                drawUpRightCornerWall(j, i, COLOR_BLUE);
            } else if (val == WALL_5) {
                drawFullWall(j, i, COLOR_BLUE);
            } else if (val == WALL_3) {
                drawDownRightCornerWall(j, i, COLOR_BLUE);
            } else if (val == WALL_2) {
                drawDownWall(j, i, COLOR_BLUE);
            } else if (val == WALL_0) {
                drawLeftDownPointWall(j, i, COLOR_BLUE);
            } else if (val == WALL_I) {
                drawRightDownPointWall(j, i, COLOR_BLUE);
            } else if (val == WALL_L) {
                drawRightUpPointWall(j, i, COLOR_BLUE);
            } else if (val == WALL_D) {
                drawLeftUpPointWall(j, i, COLOR_BLUE);
            } else if (val == WALL_X) {
                drawDownFullWall(j, i, COLOR_BLUE);
            } else if (val == WALL_J) {
                drawLeftDownPointFullWall(j, i, COLOR_BLUE);
            } else if (val == WALL_F) {
                drawRightDownPointFullWall(j, i, COLOR_BLUE);
            } else if (val == WALL_S) {
                drawDownLeftCornerFullWall(j, i, COLOR_BLUE);
            } else if (val == WALL_E) {
                drawDownRightCornerFullWall(j, i, COLOR_BLUE);
            } else if (val == WALL_M) {
                drawUpRightCornerFullWall(j, i, COLOR_BLUE);
            } else if (val == WALL_N) {
                drawUpLeftCornerFullWall(j, i, COLOR_BLUE);
            } else if (val == WALL_Y) {
                drawDownRightCornerSmallWall(j, i, COLOR_BLUE);
            } else if (val ==  WALL_Z) {
                drawDownLeftCornerSmallWall(j, i, COLOR_BLUE);
            } else if (val == PACMAN) {
                if (pacmanSprite == 1) {
                    pacmanSprite = 2;
                    drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_LEFT1);
                } else if (pacmanSprite == 2) {
                    pacmanSprite = 3;
                    drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_LEFT2);
                } else if (pacmanSprite == 3) {
                    pacmanSprite = 1;
                    drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_FULL);
                }
            } else if (val == RED) {
                if (redSprite == 1) {
                    redSprite = 2;
                    drawRed(j, i, COLOR_BLACK, COLOR_TURQUOISE, COLOR_BLUE, COLOR_WHITE, SPRITE_RED_SPIRIT_LEFT1);
                } else {
                    redSprite = 1;
                    drawRed(j, i, COLOR_BLACK, COLOR_TURQUOISE, COLOR_BLUE, COLOR_WHITE, SPRITE_RED_SPIRIT_LEFT2);
                }
            } else if (val == FOOD) {
                drawFood(j, i, 7);
            } else if (val == EMPTY) {
               // drawEmpty(j, i, COLOR_BLACK);
            } else if (val == POWER_FOOD) {
                drawPowerFood(j, i, 2);
            } else if (val == DOOR) {
                drawDoor(j, i, 6);
            } else if (val == SHADOW) {
                if (redSprite == 1) {
                    redSprite = 2;
                    drawShadow(j, i, COLOR_BLACK, COLOR_PURPLE, COLOR_BLUE, COLOR_WHITE, SPRITE_SHADOW_SPIRIT1);
                } else {
                    redSprite = 1;
                    drawShadow(j, i, COLOR_BLACK, COLOR_PURPLE, COLOR_BLUE, COLOR_WHITE, SPRITE_SHADOW_SPIRIT2);
                }
            } else if (val == CHERRY) {
                drawCherry(j, i, COLOR_BLACK, COLOR_RED, COLOR_BROWN, COLOR_WHITE, SPRITE_CHERRY);
            }
        }
    }

    _setvisualpage(activePage);
}

void draw(int i, int j) {
    char val = map[i][j] ;
    if (val == WALL_8) {
        drawUpWall(j, i, COLOR_BLUE);
    } else if(val == WALL_4) {
        drawLeftWall(j, i, COLOR_BLUE);
    } else if (val == WALL_7) {
        drawUpLeftCornerWall(j, i, COLOR_BLUE);
    } else if (val == WALL_6) {
        drawRightWall(j, i, COLOR_BLUE);
    } else if (val == WALL_1) {
        drawDownLeftCornerWall(j, i, COLOR_BLUE);
    } else if (val == WALL_9) {
        drawUpRightCornerWall(j, i, COLOR_BLUE);
    } else if (val == WALL_5) {
        drawFullWall(j, i, COLOR_BLUE);
    } else if (val == WALL_3) {
        drawDownRightCornerWall(j, i, COLOR_BLUE);
    } else if (val == WALL_2) {
        drawDownWall(j, i, COLOR_BLUE);
    } else if (val == WALL_0) {
        drawLeftDownPointWall(j, i, COLOR_BLUE);
    } else if (val == WALL_I) {
        drawRightDownPointWall(j, i, COLOR_BLUE);
    } else if (val == WALL_L) {
        drawRightUpPointWall(j, i, COLOR_BLUE);
    } else if (val == WALL_D) {
        drawLeftUpPointWall(j, i, COLOR_BLUE);
    } else if (val == WALL_X) {
        drawDownFullWall(j, i, COLOR_BLUE);
    } else if (val == WALL_J) {
        drawLeftDownPointFullWall(j, i, COLOR_BLUE);
    } else if (val == WALL_F) {
        drawRightDownPointFullWall(j, i, COLOR_BLUE);
    } else if (val == WALL_S) {
        drawDownLeftCornerFullWall(j, i, COLOR_BLUE);
    } else if (val == WALL_E) {
        drawDownRightCornerFullWall(j, i, COLOR_BLUE);
    } else if (val == WALL_M) {
        drawUpRightCornerFullWall(j, i, COLOR_BLUE);
    } else if (val == WALL_N) {
        drawUpLeftCornerFullWall(j, i, COLOR_BLUE);
    } else if (val == WALL_Y) {
        drawDownRightCornerSmallWall(j, i, COLOR_BLUE);
    } else if (val ==  WALL_Z) {
        drawDownLeftCornerSmallWall(j, i, COLOR_BLUE);
    } else if (val == PACMAN) {
        if (pacmanSprite == 1) {
            pacmanSprite = 2;
            if (dx < 0) {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_LEFT1);
            } else if (dx > 0) {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_RIGHT1);
            } else if (dy < 0) {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_UP1);
            } else if (dy > 0) {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_DOWN1);
            } else {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_FULL);
            }
        } else if (pacmanSprite == 2) {
            pacmanSprite = 3;
            if (dx < 0) {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_LEFT2);
            } else if (dx > 0) {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_RIGHT2);
            } else if (dy < 0) {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_UP2);
            } else if (dy > 0) {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_DOWN2);
            } else {
                drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_FULL);
            }
        } else if (pacmanSprite == 3) {
            pacmanSprite = 1;
            drawPacMen(j, i, COLOR_BLACK, COLOR_YELLOW, SPRITE_PACMAN_FULL);
        }
    } else if (val == RED) {
        if (redSprite == 1) {
            redSprite = 2;
            if (dxRed < 0) {
                drawRed(j, i, COLOR_BLACK, COLOR_TURQUOISE, COLOR_BLUE, COLOR_WHITE, SPRITE_RED_SPIRIT_LEFT1);
            } else {
                drawRed(j, i, COLOR_BLACK, COLOR_TURQUOISE, COLOR_BLUE, COLOR_WHITE, SPRITE_RED_SPIRIT_RIGHT1);
            }
        } else {
            redSprite = 1;
            if (dxRed < 0) {
                drawRed(j, i, COLOR_BLACK, COLOR_TURQUOISE, COLOR_BLUE, COLOR_WHITE, SPRITE_RED_SPIRIT_LEFT2);
            } else {
                drawRed(j, i, COLOR_BLACK, COLOR_TURQUOISE, COLOR_BLUE, COLOR_WHITE, SPRITE_RED_SPIRIT_RIGHT2);
            }
        }
    } else if (val == FOOD) {
        drawFood(j, i, 7);
    } else if (val == EMPTY) {
        drawEmpty(j, i, COLOR_BLACK);
    } else if (val == POWER_FOOD) {
        drawPowerFood(j, i, COLOR_GREEN);
    } else if (val == DOOR) {
        drawDoor(j, i, 6);
    } else if (val == SHADOW) {
        if (redSprite == 1) {
            redSprite = 2;
            drawShadow(j, i, COLOR_BLACK, COLOR_PURPLE, COLOR_BLUE, COLOR_WHITE, SPRITE_SHADOW_SPIRIT1);
        } else {
            redSprite = 1;
            drawShadow(j, i, COLOR_BLACK, COLOR_PURPLE, COLOR_BLUE, COLOR_WHITE, SPRITE_SHADOW_SPIRIT2);
        }
    } else if (val == CHERRY) {
        drawCherry(j, i, COLOR_BLACK, COLOR_RED, COLOR_BROWN, COLOR_WHITE, SPRITE_CHERRY);
    }
}

void refresh_DOS() {
    //if (activePage == 0) {
     //   activePage = 1;
    //} else {
     //   activePage = 0;
    //}
    //_setactivepage(activePage);

    if (refreshCherry) {
        draw(cherryY, cherryX);
        //refreshCherry = 0;
    }

    if (refreshDoor) {
        draw(doorY, doorX);
        refreshDoor = 0;
    }

    if (oldXRed != redX || oldYRed != redY) {
        drawEmpty(oldXRed, oldYRed, COLOR_BLACK);
        draw(oldYRed, oldXRed);
        draw(redY, redX);
    }

    if (oldX != pacmanX || oldY != pacmanY) {
        draw(oldY, oldX);
        draw(pacmanY, pacmanX);
    }

    //_setvisualpage(activePage);
}

long current_timestamp() {
    struct dostime_t time;
    // получить текущее время
    _dos_gettime(&time);
    // вычисляем милисекунды
    long milliseconds =  time.hour * 24L * 60000L + time.minute * 60000L + time.second * 1000L + time.hsecond * 10L;

    return milliseconds;
}
#endif
/**
 * Перерисовать Карту (map[][]), PACMAN, Призраков
 */
void showMap() {
    /** Linux */
    #ifdef __linux__
    system("clear");
    printf("   Super Turbo Net Pac-Man\n");
    for(int i = 0; i < mapSizeX; i++) {
        printf("%s\n",map[i]);
    }
    printf("Score %d! To win %d / %d!", score + redBonus + powerBonus + cherryBonus, score, foodToWIN);
    #else
    draw_DOS();
    #endif
}

void refreshMap() {
    /** Linux */
    #ifdef __linux__
    system("clear");
    printf("   Super Turbo Net Pac-Man\n");
    for(int i = 0; i < mapSizeX; i++) {
        printf("%s\n",map[i]);
    }
    printf("Score %d! To win %d / %d!", score + redBonus + powerBonus + cherryBonus, score, foodToWIN);
    #else
    refresh_DOS();
    #endif
}

/**
 * Корректировка координат PACMAN или Призрака
 * если вышел за поле (появление с другой стороны поля)
 * x - координата по X на карте (map[][])
 * y - координата по y на карте (map[][])
 * значение передаются по ссылке, по этому они меняются
 */
void moveBound(int *x, int *y) {
    if (*x < 0) {
        *x = mapSizeY - 2;
    } else if (*x > mapSizeY - 2) {
        *x = 0;
    }

    if (*y < 0) {
        *y = mapSizeX - 1;
    } else if (*y > mapSizeX - 1) {
        *y = 0;
    }
}


/**
 * Клетка по заданным координатам не стена (WALL)
 * y - координата Y на карте (map[][])
 * x - координата X на карте (map[][])
 * return 1 - не стена, 0 - стена
 */
int isNotWell(int y, int x) {
    if (map[y][x] != WALL_0 && map[y][x] != WALL_1 && map[y][x] != WALL_2 && map[y][x] != WALL_3
        && map[y][x] != WALL_4 && map[y][x] != WALL_5 && map[y][x] != WALL_6 && map[y][x] != WALL_7
        && map[y][x] != WALL_8 && map[y][x] != WALL_9 && map[y][x] != WALL_I && map[y][x] != WALL_L
        && map[y][x] != WALL_D && map[y][x] != WALL_X && map[y][x] != WALL_J && map[y][x] != WALL_F
        && map[y][x] != WALL_S && map[y][x] != WALL_E && map[y][x] != WALL_M && map[y][x] != WALL_N
        && map[y][x] != WALL_Y && map[y][x] != WALL_Z
        ) {
        return 1;

    }
    return 0;
}


/**
 * Клетка по заданным координатам не стена и не дверь (WALL, DOOR)
 * y - координата Y на карте (map[][])
 * x - координата X на карте (map[][])
 * return 1 - не стена и не дверь, 0 - стена или дверь
 */
int isNotWellOrDoor(int y, int x) {
    if (isNotWell(y, x) && map[y][x] != DOOR) {
        return 1;

    }
    return 0;
}

/**
 * Открыть двери к вишне и дому призраков
 */
void openDoors() {
    map[doorY][doorX] = EMPTY;
    cherryFlag = 1;
    map[cherryY][cherryX] = CHERRY;
    refreshCherry = 1;
    refreshDoor = 1;
}

/**
 * Закрыть двери к дому призраков
 * если вишню не съел PACMAN она появится еще
 */
void closeDoors() {
    map[doorY][doorX] = DOOR;
    cherryFlag = 0;
    refreshDoor = 1;
}


/**
 * Проиграл ли PACMAN или он мог съесть призрака
 */
int pacmanLooser() {
    // Если RED и PACMAN на одной клетке поля
    if (redY == pacmanY && redX == pacmanX) {
        // RED не съедобен
        if (redFlag) {
            // Конец игры - PACMAN съеден
            map[pacmanY][pacmanX] = RED;
            // TODO showMap();
            printf(" Pac-Man LOOSER !!!\n");
            return 1;
        } else {
            // RED съедобен в данный момент
            // Отправляем его в дом Приведений
            redY = 10;
            redX = 15;
            // бездвиживаем
            dyRed = 0;
            dxRed = 0;
            // закрываем дверь в дом привидений
            closeDoors();
            // отображаем RED на карте как съедобного
            map[redY][redX] = SHADOW;
            // даем бонус за то что RED съели
            redBonus += SCORE_RED_EAT;
            // проверяем что пакмен съел вместе с RED
            if (oldRedVal == FOOD) {
                // еду
                score++;
            } else if(oldRedVal == POWER_FOOD) {
                // поверап
                score++;
                powerBonus+=SCORE_POWER_BONUS;
                // обнавляем время когда RED стал съедобным
                redTime = current_timestamp();
            } else if (oldRedVal == CHERRY) {
                // вишню
                cherryBonus+=SCORE_CHERY_BONUS;
            }

            oldRedVal = EMPTY;
        }
    }
    return 0;
}

/**
 *  Запомнить видеорежим для DOS
 */
void videoMode() {
    #ifdef __linux__
     //  ничего не делаем, пока в консоли работает
    #else
    setVideoMode_DOS();

    old_apage = getGetActivePage_DOS();
    old_vpage = getVisualPage_DOS();
    #endif
}

/**
 * Вернуть видеорежим
 */
void setBackStartVideoMode(int old_apage,int old_vpage) {
    #ifdef __linux__
     // ничего не делаем, пока в консоли работает
    #else
    setBackStartVideoMode_DOS(old_apage, old_vpage);
    #endif
}


/**
 * Определяем количество очков для победы
 */
void scoreForWin() {
    for (int i = 0; i < mapSizeX; i++) {
        for (int j =0; j < mapSizeY ; j++) {
            // надо съесть поверапы и всю еду
            if (map[i][j] == FOOD || map[i][j] == POWER_FOOD) {
                foodToWIN++;
            }
        }
    }
}


int main() {    
    srand(time(NULL));

    int ch;

    startTime = current_timestamp();
    pacmanLastUpdateTime = startTime;
    redLastUpdateTime = startTime;


    redTime = startTime;
    // сколько очков нужно для выигрыша
    scoreForWin();

    map[pacmanY][pacmanX] = PACMAN;
    map[redY][redX] = RED;

    // запомнить видеорежим
    videoMode();
    showMap();

    do {


        if (!cherryFlag && redFlag && !cherryBonus && current_timestamp()-startTime > CHERRY_TIME) {
            openDoors();
        }


        // надо ли перерисовать карту
        if (oldX != pacmanX || oldY != pacmanY || oldXRed != redX || oldYRed != redY) {
            refreshMap();
        }

        // надо ли RED перейти в режим погони
        if (redFlag == 0 && (current_timestamp()-redTime > RED_TIME)) {
            redFlag = 1;
            if (dyRed == 0 && dxRed == 0) {
                dyRed=-1;
            }
        }


        // запоминаем текущие координаты PACMAN
        oldX = pacmanX;
        oldY = pacmanY;

        // запоминаем текущие координаты RED
        oldXRed = redX;
        oldYRed = redY;

        // проверяем нажата ли кнопка
        if (kbhit()) {

            // оределяем какая кнопка нажата
            ch = getch(); // Linux getchar() ;
            if (ch == 0) ch = getch(); // only DOS
            switch(ch)
            {
                // key UP
                case 65:  // Linux
                case 72:  // DOS
                case 119:
                    dy=-1;
                    dx=0;
                    break;
                // key DOWN
                case 66:  // Linux
                case 80:  // DOS
                case 115:
                    dy=1;
                    dx=0;
                    break;
                // key LEFT
                case 68:  // Linux
                case 75:  // DOS
                case 97:
                    dx=-1;
                    dy=0;
                    break;
                // key RIGHT
                case 67:  // Linux
                case 77:  // DOS 
                case 100:
                    dx=1;
                    dy=0;
                    break;
            }
        }

        /** Pac-Man **/
        // проверяем, у PACMAN задоно ли направление движения
        if (dx!=0 || dy!=0) {
            long t = current_timestamp();

            // должен ли PACMAN переместиться на новую клетку
            if (t-pacmanLastUpdateTime > PACMAN_SPEED) {
                pacmanX=pacmanX+dx;
                pacmanY=pacmanY+dy;

                // запоминаем время перехода на новую клетку
                pacmanLastUpdateTime = t;

                // корректируем координаты PACMAN если надо (чтоб не вышел с поля)
                moveBound(&pacmanX, &pacmanY);

                // если текущая клетка с едой, увиличиваем счетчик съеденного
                if (map[pacmanY][pacmanX] == FOOD) {
                   score++;
                } else if (map[pacmanY][pacmanX] == POWER_FOOD) {
                    // RED становится съедобным
                    redFlag = 0;
                    // бежит в обратную сторону
                    dxRed=-1*dxRed;
                    dyRed=-1*dyRed;
                    // запоминаем время когда RED стал съедобным
                    redTime = current_timestamp();
                    // за POWER_FOOD тоже точка
                    score++;
                    // и даем еще бонус
                    powerBonus+=SCORE_POWER_BONUS;
                } else if (map[pacmanY][pacmanX] == CHERRY) {
                    cherryBonus+=SCORE_CHERY_BONUS;
                }


                if (isNotWellOrDoor(pacmanY, pacmanX)) {
                    // если в новой клетке не дверь то в старой делаем пустую клетку
                    map[oldY][oldX]=EMPTY;
                } else {
                    // если в новой клетке стена WALL или дверь DOOR
                    // остаемся на прошлой клетке
                    pacmanY = oldY;
                    pacmanX = oldX;
                    // вектор движения сбрасываем (PACMAN останавливается)
                    dx=0;
                    dy=0;
                }

                // рисуем пакмена в координатах текущей клетки карты
                map[pacmanY][pacmanX] = PACMAN;

                // если съеденны все FOOD иPOWER_FOOD - PACMAN выиграл
                if (score >= foodToWIN) {
                    // TODO showMap();
                    setBackStartVideoMode(old_apage, old_vpage);
                    printf("          Pac-Man WINER !!!\n");
                    return 0;
                }


                // сеъеи ли PACMAN привидение (или оно нас)                
                if (pacmanLooser()) {
                    setBackStartVideoMode(old_apage, old_vpage);
                    return 0;
                }

            }

        }


        /** RED */
        // проверяем, у RED задоно ли направление движения
        if (dxRed!=0 || dyRed!=0) {
            long t1 = current_timestamp();
            if (t1-redLastUpdateTime > RED_SPEED) {
                redX=redX+dxRed;
                redY=redY+dyRed;
                redLastUpdateTime = t1;

                moveBound(&redX, &redY);

                if (isNotWell(redY,redX)) {
                    map[oldYRed][oldXRed]=oldRedVal;
                    oldRedVal = map[redY][redX];

                    if (redX == 15 && redY >= 7 && redY <= 10 ) {
                        dyRed = -1;
                        dxRed = 0;
                    } else if (dxRed !=0) {
                        if (redFlag && redY != pacmanY) {
                            if (isNotWellOrDoor(redY+1, redX) && isNotWellOrDoor(redY-1, redX)) {
                                if (abs(redY + 1 - pacmanY) < abs(redY - 1 - pacmanY)) {
                                    dyRed = 1;
                                } else {
                                    dyRed = -1;
                                }
                            } else if (isNotWellOrDoor(redY+1, redX)) {
                                if (abs(redY + 1 - pacmanY) < abs(redY - pacmanY)) {
                                    dyRed = 1;
                                }
                            }  else if (isNotWellOrDoor(redY-1, redX)) {
                                if (abs(redY - 1 - pacmanY) < abs(redY - pacmanY)) {
                                    dyRed = -1;
                                }

                            }
                        } else {
                            if (isNotWellOrDoor(redY+1, redX)) {
                                dyRed = rand() % 2;
                            }

                            if (isNotWellOrDoor(redY-1, redX)) {
                                dyRed = -1 * (rand() % 2);
                            }
                        }


                        if (dyRed != 0) {
                            dxRed = 0;
                        }

                    } else if (dyRed !=0) {
                        if (redFlag && redX != pacmanX) {
                            if (isNotWellOrDoor(redY, redX + 1) && isNotWellOrDoor(redY, redX - 1)) {
                                if (abs(redX + 1 - pacmanX) < abs(redX - 1 - pacmanX)) {
                                    dxRed = 1;
                                } else {
                                    dxRed = -1;
                                }
                            } else if (isNotWellOrDoor(redY, redX + 1)) {
                                if (abs(redX + 1 - pacmanX) < abs(redX - pacmanX)) {
                                    dxRed = 1;
                                }
                            }  else if (isNotWellOrDoor(redY-1, redX)) {
                                if (abs(redX - 1 - pacmanX) < abs(redX - pacmanX)) {
                                    dxRed = -1;
                                }

                            }
                        } else {

                            if (isNotWellOrDoor(redY, redX + 1)) {
                                dxRed = rand() % 2;
                            }

                            if (isNotWellOrDoor(redY, redX - 1)) {
                                dxRed = -1 * (rand() % 2);
                            }

                         }

                        if (dxRed != 0) {
                            dyRed = 0;
                        }

                    }

                } else {
                     if (redX == 15 && redY >= 7 && redY <= 10 ) {
                        dyRed = -1;
                        dxRed = 0;
                     }  else {

                        redX = oldXRed;
                        redY = oldYRed;

                        if (dxRed != 0) {
                            dxRed = 0;
                            if (isNotWellOrDoor(redY+1, redX)) {
                                dyRed = 1;
                            } else if (isNotWellOrDoor(redY-1, redX)) {
                                dyRed = -1;
                            }
                        } else {
                            dyRed = 0;
                            if (isNotWellOrDoor(redY, redX+1)) {
                                dxRed = 1;
                            } else if (isNotWellOrDoor(redY, redX-1)) {
                                dxRed = -1;
                            }
                        }
                    }

                }


                // сеъеи ли PACMAN привидение (или оно нас)
                if (pacmanLooser()) {
                    setBackStartVideoMode(old_apage, old_vpage);
                    return 0;
                }
            }
        }


        if (redFlag) {
            map[redY][redX] = RED;
        } else {
            map[redY][redX] = SHADOW;
        }



    // Выход из игры 'q'
    } while(ch != 'q');

    setBackStartVideoMode(old_apage, old_vpage);
    return 0;
}
