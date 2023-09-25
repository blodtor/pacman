/**
 Super Turbo NET Pac-Man v1.7
 реализация Pac-Man для Windows 98, Windows XP, Windows 10
 на основе Win32 API

 Для сборки из Windows 10  нужен Open Watcom 1.9  - http://www.openwatcom.org/
 скачать open-watcom-c-win32-1.9.exe можно тут https://sourceforge.net/projects/openwatcom/files/open-watcom-1.9/

 Из Windows собирается так
 > wmake wpacman.exe
 или
 > wcl386 -l=nt_win wsock32.lib -bt=nt wpacman.cpp
 или
 > wcl386 -l=nt_win ws2_32.lib -bt=nt wpacman.cpp

 может соеденятся по сети с версией v1.3 (DOS версия и консольная для GNU/Linux)

 Из консоли можно запустить Windows версию так:
 одиночная игра
 > wpacman.exe
 сетевая игра сервер (играем PAC-MAN)
 > wpacman.exe 7777
 сетевая игра клиент (играем PAC-GIRL)
 > wpacman.exe localhost 7777

 или 2ой клик из проводника по wpacman.exe а хост и порт можо ввести с клавиатуры для сетевой игры

 Для сборки из Linux тоже нужен Open Watcom 1.9  - http://www.openwatcom.org/
 скачать open-watcom-c-linux-1.9 можно тут https://sourceforge.net/projects/openwatcom/files/open-watcom-1.9/
 я копирую содержимое архива в /opt/openwatcom

 надо добавить в PATH путь до openwatcom/binl
 > export WATCOM=/opt/openwatcom
 > export PATH=$PATH:$WATCOM/binl

 Из Linux собирается так
 > make wpacman
 или
 > wcl386 -I/opt/openwatcom/h -I/opt/openwatcom/h/nt -l=nt_win ws2_32.lib -bt=nt wpacman.cpp

 Запустить в ОС GNU/Linux версию игры для Windows можно с помащью wine так:
 > wine wpacman.exe
 сетевая игра сервер (играем PAC-MAN)
 > wine wpacman.exe 7777
 сетевая игра клиент (играем PAC-GIRL)
 > wine wpacman.exe localhost 7777

 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

// Win32 API
#include <windows.h>
#include <winsock.h>

// CALLBACK функция для обработки сообщений Win32 API
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// скорость перехода pacman с одной клетки на другую в милисекундах
const long PACMAN_SPEED = 150L;
// скорость перехода pacGirl с одной клетки на другую в милисекундах
const long PACGIRL_SPEED = 150L;
// скорость перехода Красного призрака с одной клетки на другую в милисекундах
const long RED_SPEED = 150L;
// Еда
const char FOOD = '.';
// Поверап позваляющий есть призраков
const char POWER_FOOD = '*';
// Стена лаберинта
// const char WALL = '#';
// Дверь в комнату призраков
const char DOOR = '-';
// Ни кем не занятая клетка
const char EMPTY = ' ';
// Pac-Man
const char PACMAN = 'O';
// Pac Girl
const char PACGIRL = 'Q';
// Красный призрак (SHADOW или BLINKY)
const char RED = '^';
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
// игра не начата
const int NO_GAME = -1;
// не сетевая игра
const int SINGLE_APPLICATION = 0;
// игровой сервер 1-го игрока
const int SERVER_APPLICATION = 1;
// клиент 2-го игрока
const int CLIENT_APPLICATION = 2;
// нужно инициализировать новую игру
const int NEED_INIT_NEW_GAME = 3;

// рамер буфера для сетевого взаимдействия
#define RECV_BUFFER_SIZE (1024)

// массив для сетевого взаимодействия
// весь обмен идет через него в функциях записи / чтения
// четез интерфейс сокетов
unsigned char buffer[RECV_BUFFER_SIZE];

// кнопка нажатая 2 игроком на клиенте 2-го игрока
char player2PressKey = EMPTY;

// тип приложения
// SINGLE_APPLICATION - не сетевая игра
// SERVER_APPLICATION - игровой сервер 1-го игрока
// CLIENT_APPLICATION - клиент 2-го игрока
int appType = NO_GAME;

// значение двери (используется клиентом 2-го игрока)
// (используется клиентом 2-го игрока при сетевом взаимодействии)
char doorVal = DOOR;
// значение для черешни
// (используется клиентом 2-го игрока при сетевом взаимодействии)
char cherryVal = EMPTY;

// есть ли соединение
// 1 - сетевое соединение между сервером 1го игрока
//     и клиентом 2го игрока - разорвано
// 0 - сетевое соединение между сервером 1го игрока
//     и клиентом 2го игрока - создано и работает
int connectionLost = 1;

// текущие координаты PACMAN
int pacmanX = 14;
int pacmanY = 17;

// текущие координаты PACGIRL
int pacGirlX = 17;
int pacGirlY = 3;

// последний спрайт pacman
int pacmanSprite = 1;

// последний спрайт pacGirl
int pacGirlSprite = 1;

// направление движение PACMAN
int dx = 0;
int dy = 0;

// направление движение PACGIRL
int dxPacGirl = 0;
int dyPacGirl = 0;

// старые координаты PACMAN
int oldX = pacmanX;
int oldY = pacmanY;

// старые координаты PACGIRL
int oldPacGirlX = pacGirlX;
int oldPacGirlY = pacGirlY;

// направление движения RED (SHADOW или BLINKY)
int dxRed = 1;
int dyRed = 0;

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

// что лежит на клетке с PACGIRL
char oldPacGirlVal = FOOD;

// количество съеденных FOOD
int score = 1;

// бонус за съедание RED (SHADOW или BLINKY)
int redBonus = 0;

// бонус за съедание POWER_FOOD
int powerBonus = 0;

// бонус за съеденные вишни
int cherryBonus = 0;

// количество FOOD и POWER_FOOD которое нужно съесть чтоб выиграть
int foodToWIN = 0;

// время когда игру запустили
long startTime;

// время последнего обновления положения pacman
long pacmanLastUpdateTime;

// время поседнего обновления RED
long redLastUpdateTime;

// время последнего обновления положения pacGirl
long pacGirlLastUpdateTime;

// размер карты по x
#define MAP_SIZE_Y (23)
// размер карты по y
#define MAP_SIZE_X (32)

// карта занружается из файла MAP.TXT
char map[MAP_SIZE_Y][MAP_SIZE_X];

// серверный socket - ждет подключения клиентов
// использует только сервер
SOCKET serverSocket;

// клиентский socket
// запустились как сервер или как клиент 2-го игрока
// всегда в этот сокет пишем или читаем данные
// отправить на сервер / клиент 2-го игрока
// прочитать данные на сервере / клиенте 2-го игрока
SOCKET clientSocket;

/**
 * Пример карты
 = {
 "7888888888888895788888888888889",
 "4.............654.............6",
 "4*i220.i22220.l8d.i22220.i220*6",
 "4..............Q..............6",
 "4.i220.fxj.i22mxn220.fxj.i220.6",
 "4......654....654....654......6",
 "1xxxxj.65s220.l8d.222e54.fxxxx3",
 "555554.654...........654.655555",
 "555554.654.fxxj-fxxj.654.655555",
 "88888d.l8d.678d l894.l8d.l88888",
 "...........64  %  64..^........",
 "xxxxxj.fxj.61xxxxx34.fxj.fxxxxx",
 "555554.654.l8888888d.654.655555",
 "555554.654...........654.655555",
 "78888d.l8d.i22mxn220.l8d.l88889",
 "4.............654.............6",
 "4.i2mj.i22220.l8d.i22220.fn20.6",
 "4*..64.........O.........64..*6",
 "s20.ld.fxj.i22mxn220.fxj.ld.i2e",
 "4......654....654....654......6",
 "4.i2222y8z220.l8d.i22y8z22220.6",
 "4.............................6",
 "1xxxxxxxxxxxxxxxxxxxxxxxxxxxxx3",
 };
 */

/**
 * Количество изображений спрайтов загруженных для частой перерисовки
 */
#define IMAGES_SIZE (34)

/**
 * Количество видов стен загружаемых из файлов как спрайты
 */
#define WALL_SIZE (22)

// массивы куда загрузим спрайты больших объектов для Win32 Api
/**
 * Изображение спрайтов
 *
 * Спрайты заружаются из файлов:
 * S10.TXT - Спрайт вишни
 * S11.TXT - Спрайт для еды
 * S12.TXT - Спрайт для пустой клетки
 * S13.TXT - Спрайт для поверапа
 * S14.TXT - S22.TXT - Спрайты pac-man (И pac-girl)
 * S23.TXT - S30.TXT - Спрайты red (красный призрак)
 * S31.TXT - S32.TXT - Спрайты shadow (котороо можно есть)
 * S33.TXT - Спрайт двери
 * S34.TXT - S43.TXT - Спрайты pac-girl
 *
 * W1.TXT - WZ.TXT - спрайты стен (НЕ ХРАНЯТСЯ В ЭТОЙ ПЕРЕМЕННОЙ)
 *
 * значения цветов в файлах спрайтов
 * 0  - черный цвет
 * 1  - синий цвет
 * 2  - зеленый цвет
 * 4  - красный цвет
 * 5  - пурпурный цвет
 * 6  - коричневый цвет
 * 7  - серый цвет
 * 8  - желтый цвет (в DOS 14)
 * 9  - белый цвет  (в DOS 15)
 */
HDC pixmaps[IMAGES_SIZE];
HBITMAP hbPixmaps[IMAGES_SIZE];
HBITMAP hmbPixmaps[IMAGES_SIZE];

// массивы куда загрузим спрайты стен для Win32 Api
HDC walls[WALL_SIZE];
HBITMAP hbWalls[WALL_SIZE];
HBITMAP hmbWalls[WALL_SIZE];

// ключи по которым ищем загруженую в память стену
char wkeys[WALL_SIZE];

// ширина окна Win32
#define WIDTH 415

// высота окна Win32
#define HEIGHT 255

// Заголовок окна Win32
#define TITLE "Super Turbo NET Pac-Man v1.7 for Windows"

// Класс программы Win32
#define PRG_CLASS "wpacman"

// максимальное   количество символов для ввода Хоста и порта для X11 окна
#define INPUT_TEXT_LENGHT 30

// данные поля ввода для Хоста и порта сервера
char serverHostPort[INPUT_TEXT_LENGHT];

// Y координата для надписи Server и того что вводим с клавиатуры
#define INPUT_TEXT_Y 196

// выбранный хост или порт если хост не задавался
char param1[INPUT_TEXT_LENGHT];

// выбранный порт
char param2[INPUT_TEXT_LENGHT];

// буфер символов для KeyPress Events
char text[255];

// количество уже введенных символов
int word = 0;

// Размеры изображения
int IMAGE_WIDTH = 14;
int IMAGE_HEIGHT = 14;

// Позиция изображения
int imageX = 5;
int imageY = 5;

COLORREF colorBlack = RGB(0, 0, 0);
COLORREF colorBlue = RGB(0, 0, 255);
COLORREF colorGreen = RGB(0, 255, 0);
COLORREF colorRed = RGB(255, 0, 0);
COLORREF colorViolet = RGB(255, 0, 255);
COLORREF colorBrown = RGB(128, 64, 48);
COLORREF colorGray = RGB(128, 128, 128);
COLORREF colorYellow = RGB(255, 255, 0);
COLORREF colorWhite = RGB(255, 255, 255);

// необходимо выйти из игры - закрыть приложение
int needExitFromGame = 0;

// необходимо прервать текущую игру - продолжить играть не выйдет, только начать новую
int needStopGame = 0;

// нужно ли при WM_PAINT перерисовать все окно (в том числе карту)
int refreshWindow = 1;

// получили ли уже что то от 2-го игрока по сети
// 0 - нет данные по сети сервер не получал
// 1 - сервер получил данные по сети от клиента
// нужно чтоб игра на сервере начиналась после того как клиент отрисовал карту
// тем самым сообщает что 2ой игрок готов к игре (нужно для слабых компов)
int readFromClientSocket = 0;

/**
 * чтение из текстового файла массива
 * fileName - имя файла
 * buff - ссылка на массив куда будем читать
 * n - количество строк массива (сток в файле)
 * m - количество столбцов массива (столбцов в файле)
 */
int readMapFromFile(const char *fileName, char *buff, int n, int m) {
	FILE *fp;
	char s[33];

	fp = fopen(fileName, "r");

	if (fp != NULL) {
		for (int i = 0; i < n; i++) {
			if (fgets(s, m + 1, fp) != NULL) {
				s[m - 1] = '\0';
				strcpy(&buff[i * m], s);
			}
		}
		fclose(fp);
		return 1;
	}
	return 0;
}

/**
 * перевернуть строку
 * s - переворачиваемая строка
 */
void reverse(char s[]) {
	int i, j;
	char c;

	for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

/**
 * преобразовать целое число в строку
 * n - число которое превращаем в строку
 * s - строка с результатом
 */
void itoa(int n, char s[]) {
	int i, sign;

	if ((sign = n) < 0) {
		n = -n;
	}

	i = 0;
	do {
		s[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);

	if (sign < 0) {
		s[i++] = '-';
	}

	s[i] = '\0';
	reverse(s);
}

/**
 * двухзначное число или однозначное отрицательное
 * положить в глобальный массив buffer для дальнейшей
 * передачи данных от сервера на клиент 2 го игрока
 * в виде строки
 *
 * val - число которое надо записать в буфер - buffer
 * i - начиная с какой позиции надо записать
 *     передано по ссылке по этому меняется и в месте
 *     откуда вызвали функцию
 */
void intToBuffer(int val, int *i) {
	char b[3];
	itoa(val, b);
	if (strlen(b) == 1) {
		buffer[(*i)++] = EMPTY;
		buffer[(*i)++] = b[0];
	} else {
		buffer[(*i)++] = b[0];
		buffer[(*i)++] = b[1];
	}
}

/**
 * трехзначное число или двухзначное отрицательное
 * положить в глобальный массив buffer для дальнейшей
 * передачи данных от сервера на клиент 2 го игрока
 * в виде строки
 *
 * val - число которое надо записать в буфер - buffer
 * i - начиная с какой позиции надо записать
 *     передано по ссылке по этому меняется и в месте
 *     откуда вызвали функцию
 */
void intToBuffer3(int val, int *i) {
	char b[4];
	itoa(val, b);
	int n = strlen(b);
	if (n == 1) {
		buffer[(*i)++] = EMPTY;
		buffer[(*i)++] = EMPTY;
		buffer[(*i)++] = b[0];
	} else if (n == 2) {
		buffer[(*i)++] = EMPTY;
		buffer[(*i)++] = b[0];
		buffer[(*i)++] = b[1];
	} else if (n == 3) {
		buffer[(*i)++] = b[0];
		buffer[(*i)++] = b[1];
		buffer[(*i)++] = b[2];
	}
}

/**
 * преобразует строку из массива buffer в число с заданной позиции
 * при этом из массива берется 2 символа начиная с заданного места
 *
 * i - позиция в массиве buffer с которой парсим число
 * return - двухзначное или однозначное отрицательное число
 *          распарсеноое из глобального массива buffer
 *          начиная с заданной позиции
 */
int bufferToInt(int *i) {
	char b[3];
	b[0] = buffer[(*i)++];
	b[1] = buffer[(*i)++];
	b[2] = '\0';
	return atoi(b);
}

/**
 * преобразует строку из массива buffer в число с заданной позиции
 * при этом из массива берется 3 символа начиная с заданного места
 *
 * i - позиция в массиве buffer с которой парсим число
 * return - трехзначное или двухзначное отрицательное число
 *          распарсеноое из глобального массива buffer
 *          начиная с заданной позиции
 */
int bufferToInt3(int *i) {
	char b[4];
	b[0] = buffer[(*i)++];
	b[1] = buffer[(*i)++];
	b[2] = buffer[(*i)++];
	b[3] = '\0';
	return atoi(b);
}

/**
 * Клетка по заданным координатам не стена (WALL)
 * y - координата Y на карте (map[][])
 * x - координата X на карте (map[][])
 * return 1 - не стена, 0 - стена
 */
int isNotWell(int y, int x) {
	if (map[y][x] == PACMAN || map[y][x] == PACGIRL || map[y][x] == RED
			|| map[y][x] == CHERRY || map[y][x] == FOOD
			|| map[y][x] == POWER_FOOD || map[y][x] == EMPTY
			|| map[y][x] == SHADOW) {
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
 * Создаем пакет с данными в buffer для передачи по сети
 */
void createBuffer() {
	int i = 0;
	if (appType == SERVER_APPLICATION) {
		intToBuffer(pacmanX, &i);
		intToBuffer(pacmanY, &i);
		intToBuffer(oldX, &i);
		intToBuffer(oldY, &i);
		intToBuffer(dx, &i);
		intToBuffer(dy, &i);
		intToBuffer(redX, &i);
		intToBuffer(redY, &i);
		intToBuffer(oldXRed, &i);
		intToBuffer(oldYRed, &i);
		intToBuffer(oldRedVal, &i);
		intToBuffer(dxRed, &i);
		intToBuffer(dyRed, &i);
		intToBuffer(pacGirlX, &i);
		intToBuffer(pacGirlY, &i);
		intToBuffer(oldPacGirlX, &i);
		intToBuffer(oldPacGirlY, &i);
		intToBuffer(dxPacGirl, &i);
		intToBuffer(dyPacGirl, &i);
		intToBuffer(cherryX, &i);
		intToBuffer(cherryY, &i);
		intToBuffer(doorX, &i);
		intToBuffer(doorY, &i);
		intToBuffer(redFlag, &i);
		intToBuffer(refreshDoor, &i);
		intToBuffer(refreshCherry, &i);
		intToBuffer3(score, &i);
		intToBuffer3(redBonus, &i);
		intToBuffer3(powerBonus, &i);
		intToBuffer3(cherryBonus, &i);
		intToBuffer3(foodToWIN, &i);
		buffer[i++] = map[doorY][doorX];
		buffer[i++] = map[cherryY][cherryX];
		buffer[i++] = map[pacGirlY][pacGirlX];
	} else {
		// с клиента 2-го игрока на сервер отправляем
		// только нажатую кнопку на клавиатуре
		buffer[i++] = player2PressKey;
	}
}

/**
 * Парсим данные пришедшие по сети
 */
void parseBuffer() {
	int i = 0;
	if (appType == SERVER_APPLICATION) {
		// пришла нажатая кнопка 2 м игроком
		player2PressKey = buffer[i++];
	} else {
		// пришли данные о местоположении объектов и очках
		pacmanX = bufferToInt(&i);
		pacmanY = bufferToInt(&i);
		oldX = bufferToInt(&i);
		oldY = bufferToInt(&i);
		dx = bufferToInt(&i);
		dy = bufferToInt(&i);
		redX = bufferToInt(&i);
		redY = bufferToInt(&i);
		oldXRed = bufferToInt(&i);
		oldYRed = bufferToInt(&i);
		oldRedVal = bufferToInt(&i);
		dxRed = bufferToInt(&i);
		dyRed = bufferToInt(&i);
		pacGirlX = bufferToInt(&i);
		pacGirlY = bufferToInt(&i);
		oldPacGirlX = bufferToInt(&i);
		oldPacGirlY = bufferToInt(&i);
		dxPacGirl = bufferToInt(&i);
		dyPacGirl = bufferToInt(&i);
		cherryX = bufferToInt(&i);
		cherryY = bufferToInt(&i);
		doorX = bufferToInt(&i);
		doorY = bufferToInt(&i);
		redFlag = bufferToInt(&i);
		refreshDoor = bufferToInt(&i);
		refreshCherry = bufferToInt(&i);
		score = bufferToInt3(&i);
		redBonus = bufferToInt3(&i);
		powerBonus = bufferToInt3(&i);
		cherryBonus = bufferToInt3(&i);
		foodToWIN = bufferToInt3(&i);
		doorVal = buffer[i++];
		cherryVal = buffer[i++];
		map[pacGirlY][pacGirlX] = buffer[i++];
	}
}

/**
 * Win32 API
 *
 * Создание сервера
 * port - порт на катором поднимется сервер
 */
void pacmanServerWindows(char *port) {
	// порт сервера на который ждем подключение клиента
	int serverPort;
	// структура содержащая адрес сервера
	struct sockaddr_in serverAddress;
	// структура содержащая адрес клиента
	struct sockaddr_in clientAddress;

	serverPort = atoi(port);

	// инициализация Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
		printf("\nERROR initializing Winsock");
		return;
	}

	// создаем серверный сокет
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		printf("\nERROR opening socket");
	} else {
		int optval = 1;
		// SO_REUSEADDR позволяет использовать занятый порт не дожидаясь таймаута при повторной игре
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
				(const char*) &optval, sizeof(optval));

		memset((char*) &serverAddress, 0, sizeof(serverAddress));

		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		serverAddress.sin_port = htons(serverPort);

		// пробуем связать (bind) серверный сокет с адресом и портом
		if (bind(serverSocket, (struct sockaddr*) &serverAddress,
				sizeof(serverAddress)) == SOCKET_ERROR) {
			printf("\nERROR binding port '%s'\n", port);
		} else {
			// устанавливаем серверный сокет в состояние прослушивания, будет использоваться для
			// приема запросов входящих соединений с помощью accept()
			// очередь ожидающих соединений = 5
			if (listen(serverSocket, 5) < 0) {
				printf("\nERROR listen\n");
			} else {
				int clientAddressSize = sizeof(clientAddress);
				clientSocket = accept(serverSocket,
						(struct sockaddr*) &clientAddress, &clientAddressSize);
				if (clientSocket == INVALID_SOCKET) {
					printf("\nERROR accept\n");
				} else {
					// переводим сокет в неблокирующий режим
					u_long mode = 1;
					if (ioctlsocket(clientSocket, FIONBIO, &mode)
							== SOCKET_ERROR) {
						printf("\nERROR setting socket to non-blocking mode\n");
					} else {
						// сервер поднят успешно и соеденился с клиентом 2-го игрока
						connectionLost = 0;
						// мы являемся сервером
						appType = SERVER_APPLICATION;
					}
				}
			}
		}
	}
}

/**
 * Win32 API
 *
 * Подключение к серверу клиента 2 игрока
 * host - хост на котором работает сервер
 * port - порт на катором работает сервер
 */
void pacmanClientWindows(char *host, char *port) {
	// порт сервера на который ждем подключение клиента 
	int serverPort;
	// структура содержащая адрес сервера 
	struct sockaddr_in serverAddress;
	// структура содержащая адрес клиента 
	struct sockaddr_in clientAddress;
	// хост сервера 
	struct hostent *server;

	serverPort = atoi(port);

	// Инициализация библиотеки Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("\nFailed to initialize Winsock.\n");
	} else {
		// Создаем сокет для отправки сообщений
		clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (clientSocket == INVALID_SOCKET) {
			printf("\nERROR open socket\n");
		} else {
			// получаем хост по имени или ip
			server = gethostbyname(host);
			if (server == NULL) {
				printf("\nERROR gethostbyname\n");
			} else {
				memset((char*) &serverAddress, 0, sizeof(serverAddress));
				serverAddress.sin_family = AF_INET;
				memcpy((char*) &serverAddress.sin_addr.s_addr,
						(char*) server->h_addr, server->h_length);
				serverAddress.sin_port = htons(serverPort);
				if (connect(clientSocket, (struct sockaddr*) &serverAddress,
						sizeof(serverAddress)) == SOCKET_ERROR) {
					printf("\nERROR connect\n");
				} else {
					// переводим сокет в не блокирующий режим
					unsigned long nonBlocking = 1;
					if (ioctlsocket(clientSocket, FIONBIO, &nonBlocking) != 0) {
						printf("\nERROR set non-blocking\n");
					} else {
						// клиент 2-го игрока успешно соединился с сервером
						connectionLost = 0;
						// мы являемся клиентом
						appType = CLIENT_APPLICATION;
					}
				}
			}
		}
	}
}

/**
 * Win32 API
 *
 * отправляет через интерфейс сокетов сформерованный пакет
 * данные записываются в глобальный массив buffer
 * затем записываются в сокет
 *
 * - Cоздает пакет с данными о положении персонажей
 *   для отправки с сервера клиенту 2-го игрока
 *
 * - Отправляет на сервер данные с клиента 2-го игрока
 *   шлем на сервер только нажатую на клавиатуре кнопку 2м игроком
 */
void writeInSocketWindows() {
	// очищаем буфер
	bzero(buffer, RECV_BUFFER_SIZE);

	// создаем пакет с данными в buffer для передачи по сети
	createBuffer();

	int n;
	if (appType == SERVER_APPLICATION) {
		// записываем buffer в сокет (отправляем 2-му игроку)
		n = send(clientSocket, (const char*) buffer, RECV_BUFFER_SIZE, 0);
	} else {
		// записываем buffer в сокет (отправляем на Сервер)
		n = send(clientSocket, (const char*) buffer, 1, 0);
	}

	if (n == SOCKET_ERROR) {
		if (appType == SERVER_APPLICATION) {
			printf("\nPac-Girl OFF\n");
		} else {
			printf("\nPac-Man OFF\n");
		}
		connectionLost = 1;
	}
}

/**
 * Win32 API
 *
 * Читает данные из сокета переданные по сети
 * в глобальную переменную buffer (не пересаоздаем его всевремя)
 * а из нее потом все перекладывается в остальные  переменные (смотри parseBuffer();)
 */
void readFromSocketWindows() {
	fd_set readset;
	FD_ZERO(&readset);
	FD_SET(clientSocket, &readset);

	// Задаём таймаут
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 150;

	// проверяем можно ли что то прочитать из сокета
	int n = select(FD_SETSIZE, &readset, NULL, NULL, &timeout);
	if (n > 0 && FD_ISSET(clientSocket, &readset)) {
		// очищаем буфер
		bzero(buffer, RECV_BUFFER_SIZE);

		// читаем из сокета
		if (appType == SERVER_APPLICATION) {
			n = recv(clientSocket, (char*) buffer, 1, 0);
		} else {
			n = recv(clientSocket, (char*) buffer, RECV_BUFFER_SIZE, 0);
		}

		if (n < 1) {
			if (appType == SERVER_APPLICATION) {
				printf("\nPac-Girl OFF\n");
			} else {
				printf("\nPac-Man OFF\n");
			}
			connectionLost = 1;
		} else if (n > 0 && buffer[0] != 0) {
			parseBuffer();
			readFromClientSocket = 1;
		}
	}
}

/**
 * Win32 API
 *
 * текущее время в милисекундах
 */
long current_timestamp() {
	SYSTEMTIME st;
	GetLocalTime(&st);
	long milliseconds = st.wDay * 86400000L + st.wHour * 3600000L
			+ st.wMinute * 60000L + st.wSecond * 1000L + st.wMilliseconds;
	return milliseconds;
}

/**
 * Win32 API
 *
 * Нарисовать в Pixmap спрайт из буфера
 * n - размер массива по y (количество строк в файле)
 * m - размер массива по x (количество столбцов в файле)
 * buff - массив с данными из текстового файла
 * HDC - спрайт
 */
void drawSprite(int n, int m, char *buff, HDC pixmap) {
	char txt[2];
	COLORREF color;

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			txt[0] = buff[i * n + j + i];
			txt[1] = '\0';
			int val = atoi(txt);

			switch (val) {
			case 0: // черный цвет
				color = colorBlack;
				break;
			case 1: // синий цвет
				color = colorBlue;
				break;
			case 2: // зеленый цвет
				color = colorGreen;
				break;
			case 4: // красный цвет
				color = colorRed;
				break;
			case 5: // пурпурный цвет
				color = colorViolet;
				break;
			case 6: // коричневый цвет
				color = colorBrown;
				break;
			case 7: // серый цвет
				color = colorGray;
				break;
			case 8: // желтый цвет
				color = colorYellow;
				break;
			case 9: // белый цвет
				color = colorWhite;
				break;
			default:
				color = colorBlack;
			}

			// рисуем точку в пикселоной карте
			SetPixel(pixmap, j, i, color);
		}

	}
}

/**
 * Win32 API
 *
 * Нарисовать в Pixmap картинку из файла
 * x - позиция по горизонтали
 * y - позиция по вертекали
 * fileName - файл с данными картинки в текстовом виде
 * n - размер массива по y (количество строк в файле)
 * m - размер массива по x (количество столбцов в файле)
 * pixmap - то где рисуем картинку
 *
 */
void getImage(const char *fileName, int n, int m, HDC pixmap) {
	int k = m + 1;
	char *b = new char[n * k];
	if (readMapFromFile(fileName, b, n, k)) {
		drawSprite(n, m, b, pixmap);
	}
	delete b;
}

/**
 * Win32 API
 *
 * Нарисовать только 1 объект с карты
 * i - строка в массиве карты
 * j - столбец в массиве карты
 * hdc - то где рисуем картинку
 */
void draw(int i, int j, HDC hdc) {
	char val = map[i][j];
	int y = i * 8;
	int x = j * 8;

	if (val == PACMAN) {
		if (pacmanSprite == 1) {
			pacmanSprite = 2;
			if (dx < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[5], 0, 0, SRCCOPY);
			} else if (dx > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[7], 0, 0, SRCCOPY);
			} else if (dy < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[9], 0, 0, SRCCOPY);
			} else if (dy > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[11], 0, 0, SRCCOPY);
			} else {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[4], 0, 0, SRCCOPY);
			}
		} else if (pacmanSprite == 2) {
			pacmanSprite = 3;
			if (dx < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[6], 0, 0, SRCCOPY);
			} else if (dx > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[8], 0, 0, SRCCOPY);
			} else if (dy < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[10], 0, 0, SRCCOPY);
			} else if (dy > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[12], 0, 0, SRCCOPY);
			} else {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[4], 0, 0, SRCCOPY);
			}
		} else if (pacmanSprite == 3) {
			pacmanSprite = 1;
			BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[4], 0, 0, SRCCOPY);
		}
	} else if (val == RED) {
		if (redSprite == 1) {
			redSprite = 2;
			if (dxRed < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[13], 0, 0, SRCCOPY);
			} else if (dxRed > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[15], 0, 0, SRCCOPY);
			} else if (dyRed > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[17], 0, 0, SRCCOPY);
			} else {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[19], 0, 0, SRCCOPY);
			}
		} else {
			redSprite = 1;
			if (dxRed < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[14], 0, 0, SRCCOPY);
			} else if (dxRed > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[16], 0, 0, SRCCOPY);
			} else if (dyRed > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[18], 0, 0, SRCCOPY);
			} else {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[20], 0, 0, SRCCOPY);
			}
		}
	} else if (val == PACGIRL) {
		if (pacGirlSprite == 1) {
			pacGirlSprite = 2;
			if (dxPacGirl < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[25], 0, 0, SRCCOPY);
			} else if (dxPacGirl > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[27], 0, 0, SRCCOPY);
			} else if (dyPacGirl < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[29], 0, 0, SRCCOPY);
			} else if (dyPacGirl > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[31], 0, 0, SRCCOPY);
			} else {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[24], 0, 0, SRCCOPY);
			}
		} else if (pacGirlSprite == 2) {
			pacGirlSprite = 3;
			if (dxPacGirl < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[26], 0, 0, SRCCOPY);
			} else if (dxPacGirl > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[28], 0, 0, SRCCOPY);
			} else if (dyPacGirl < 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[30], 0, 0, SRCCOPY);
			} else if (dyPacGirl > 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[32], 0, 0, SRCCOPY);
			} else {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[24], 0, 0, SRCCOPY);
			}
		} else if (pacGirlSprite == 3) {
			pacGirlSprite = 1;
			if (dxPacGirl != 0) {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[24], 0, 0, SRCCOPY);
			} else {
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[33], 0, 0, SRCCOPY);
			}
		}
	} else if (val == SHADOW) {
		if (redSprite == 1) {
			redSprite = 2;
			BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[21], 0, 0, SRCCOPY);
		} else {
			redSprite = 1;
			BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[22], 0, 0, SRCCOPY);
		}
	} else if (val == FOOD) {
		BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[1], 0, 0, SRCCOPY);
	} else if (val == EMPTY) {
		BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[2], 0, 0, SRCCOPY);
	} else if (val == POWER_FOOD) {
		BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[3], 0, 0, SRCCOPY);
	} else if (val == CHERRY) {
		BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[0], 0, 0, SRCCOPY);
	} else if (val == DOOR) {
		BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[23], 0, 0, SRCCOPY);
	} else {
		BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[2], 0, 0, SRCCOPY);
	}
}

/**
 * Win32 API
 *
 * удалить объекты спрайтов из памяти
 */
void freePixmaps() {
	for (int i = 0; i < IMAGES_SIZE; i++) {
		SelectObject(pixmaps[i], hmbPixmaps[i]);
		DeleteDC(pixmaps[i]);
		DeleteObject(hbPixmaps[i]);
	}

	// удаляем из памяти все стены
	for (int j = 0; j < WALL_SIZE; j++) {
		SelectObject(walls[j], hmbWalls[j]);
		DeleteDC(walls[j]);
		DeleteObject(hbWalls[j]);
	}
}

/**
 * Win32 API
 *
 * загрузить спрайты
 */
void initPixelmap() {
	// заполняем массив ключей нулями
	bzero(wkeys, WALL_SIZE);
	// S__.TXT содержит текстуры для персонажей и объектов что перересовываются
	// например S10.TXT, S22.TXT
	char t[8] = "S__.TXT";
	// буфер для преобразования строк
	char s[3];

	// Получаем контекст устройства для рисования в мапе
	HDC hdcMap;
	hdcMap = GetDC(NULL);
	// загружаем изображения объектов чо могут перерисовыватся во время игры
	for (int i = 0; i < IMAGES_SIZE; i++) {
		// имена файлов 2х значные и начинаются с S10.TXT
		itoa(i + 10, s);
		// заменяем '__' на цифры
		t[1] = s[0];
		t[2] = s[1];

		hbPixmaps[i] = CreateCompatibleBitmap(hdcMap, 14, 14);
		// создаем контекст устройства для работы с битмапом
		pixmaps[i] = CreateCompatibleDC(NULL);
		hmbPixmaps[i] = (HBITMAP) SelectObject(pixmaps[i], hbPixmaps[i]);

		// запоминаем избражение, из файла t, размеом 14 на 14
		getImage(t, 14, 14, pixmaps[i]);

	}

	// W_.TXT содержат текстуры картинок стен лаберинта
	// например W0.TXT, WL.TXT
	strcpy(t, "W_.TXT");

	// рисуем объекты карты которые не перерисовываются или перерисовыатся редко
	char v;
	for (int i = 0; i < MAP_SIZE_Y; i++) {
		for (int j = 0; j < MAP_SIZE_X - 1; j++) {
			v = map[i][j];
			if (v == DOOR || v == FOOD || v == POWER_FOOD) {
				//  ниче не делаем
			} else if (!isNotWellOrDoor(i, j)) {
				// ищем загружен ли уже спрайт или надо из файла прочитать и отрисовать
				for (int k = 0; k < WALL_SIZE; k++) {
					if (wkeys[k] == v) {
						// спрайт загружен
						break;
					} else if (wkeys[k] == 0) {
						// спрайт не загружен, нужно загрузить из файла и отрисовать
						// запоминаем позицию куда загрузим в память спрайт
						wkeys[k] = v;
						// заменяем в "W_.TXT" символ '_' на символ прочитанный из карты
						t[1] = v;

						// ресуем избражение из файла t, в позиции j, i размером 8 на 8 и запоминаем в массив walls
						hbWalls[k] = CreateCompatibleBitmap(hdcMap, 8, 8);
						// создаем контекст устройства для работы с битмапом
						walls[k] = CreateCompatibleDC(NULL);
						hmbWalls[k] = (HBITMAP) SelectObject(walls[k],
								hbWalls[k]);
						// запоминаем избражение, из файла t, размеом 8 на 8
						getImage(t, 8, 8, walls[k]);
						break;
					}

				}
			}
		}
	}

	ReleaseDC(NULL, hdcMap);
}

/**
 * Win32 API
 *
 * Перерисовать Карту полностью (map[][]), PACMAN, Призраков
 * hdc - то где рисуем карту
 */
void showMap(HDC hdc) {
	char v;
	for (int i = 0, y = 0; i < MAP_SIZE_Y; i++, y = i * 8) {
		for (int j = 0, x = 0; j < MAP_SIZE_X - 1; j++, x = j * 8) {
			v = map[i][j];
			if (v == DOOR) {
				//  ниче не делаем
			} else if (v == FOOD) {
				//  рисуем на карте еду
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[1], 0, 0,	SRCCOPY);
			} else if (v == POWER_FOOD) {
				// рисуем на карте поверапы
				BitBlt(hdc, x - 2, y - 2, x + 12, y + 12, pixmaps[3], 0, 0,	SRCCOPY);
			} else if (!isNotWellOrDoor(i, j)) {
				// рисуем на карте стены
				// ищем загружен ли уже спрайт или надо из файла прочитать и отрисовать
				for (int k = 0; k < WALL_SIZE; k++) {
					if (wkeys[k] == v) {
						// спрайт загружен, нужно просто отрисовать
						BitBlt(hdc, x, y, x + 8, y + 8, walls[k], 0, 0,	SRCCOPY);
						break;
					}

				}
			}
		}
	}

	draw(doorY, doorX, hdc);
	draw(cherryY, cherryX, hdc);
	draw(redY, redX, hdc);
	draw(pacmanY, pacmanX, hdc);
	draw(pacGirlY, pacGirlX, hdc);
}

/**
 * Win32 API
 *
 * нарисовать залитый прямоугольник
 * x - x координата левого верхнего угла
 * y - y координата левого верхнего угла
 * w - x координата правого нижнего угла
 * h - y координата правого нижнего угла
 * color - цвет
 */
void drawBox(HDC hdc, int x, int y, int w, int h, COLORREF color) {
	// Создаем кисть для заливки изображения
	HBRUSH hBrush = CreateSolidBrush(color);
	// Заливаем прямоугольник красной кистью
	RECT rect;
	rect.left = x;
	rect.top = y;
	rect.right = w;
	rect.bottom = h;
	FillRect(hdc, &rect, hBrush);
	// Освобождаем кисть
	DeleteObject(hBrush);
}

/**
 * Win32 API
 *
 * Перерисовать только нужные объекты
 * hdc - то где рисуем объекты
 */
void refreshMap(HDC hdc) {
	if (refreshDoor) {
		draw(doorY, doorX, hdc);
		if (map[doorY][doorX] != DOOR) {
			refreshDoor = 0;
		}
	}

	if (refreshCherry) {
		draw(cherryY, cherryX, hdc);
		if (map[cherryY][cherryX] != CHERRY) {
			refreshCherry = 0;
		}
	}

	if (oldXRed != redX || oldYRed != redY) {
		draw(oldYRed, oldXRed, hdc);
		draw(redY, redX, hdc);
	}

	if (oldX != pacmanX || oldY != pacmanY) {
		draw(oldY, oldX, hdc);
		draw(pacmanY, pacmanX, hdc);
	}

	if (oldPacGirlX != pacGirlX || oldPacGirlY != pacGirlY) {
		draw(oldPacGirlY, oldPacGirlX, hdc);
		draw(pacGirlY, pacGirlX, hdc);
	}

	// TODO понять где не стираю для этой координаты
	if (map[cherryY - 1][cherryX] != EMPTY) {
		map[cherryY - 1][cherryX] = EMPTY;
		draw(cherryY - 1, cherryX, hdc);
	}

	char result[5];
	char txt[20];

	// выводим инфу о съеденных вишнях и бонусе
	bzero(result, 5);
	bzero(txt, 20);

	itoa(cherryBonus / SCORE_CHERY_BONUS, result);
	strcat(txt, "x ");
	strcat(txt, result);
	strcat(txt, "=");
	itoa(cherryBonus, result);
	strcat(txt, result);

	BitBlt(hdc, 270, 50, 284, 64, pixmaps[0], 0, 0, SRCCOPY);
	SetBkColor(hdc, colorBlack);
	SetTextColor(hdc, colorWhite);
	TextOut(hdc, 290, 50, txt, strlen(txt));

	// выводим инфу о съеденных призраках
	bzero(result, 5);
	bzero(txt, 20);
	itoa(redBonus / SCORE_RED_EAT, result);
	strcat(txt, "x ");
	strcat(txt, result);
	strcat(txt, "=");
	itoa(redBonus, result);
	strcat(txt, result);

	BitBlt(hdc, 270, 70, 284, 84, pixmaps[21], 0, 0, SRCCOPY);
	TextOut(hdc, 290, 70, txt, strlen(txt));

	// выводим инфу о съеденных поверапах
	bzero(result, 5);
	bzero(txt, 20);
	itoa(powerBonus / SCORE_POWER_BONUS, result);
	strcat(txt, "x ");
	strcat(txt, result);
	strcat(txt, "=");
	itoa(powerBonus + powerBonus / SCORE_POWER_BONUS, result);
	strcat(txt, result);

	BitBlt(hdc, 270, 90, 284, 104, pixmaps[3], 0, 0, SRCCOPY);
	TextOut(hdc, 290, 90, txt, strlen(txt));

	// сколько съедено точек
	bzero(result, 5);
	bzero(txt, 20);
	itoa(score - powerBonus / SCORE_POWER_BONUS, result);
	strcat(txt, "x ");
	strcat(txt, result);
	strcat(txt, "=");
	strcat(txt, result);

	bzero(result, 5);
	itoa(foodToWIN - 4, result);
	strcat(txt, " / ");
	strcat(txt, result);

	BitBlt(hdc, 270, 110, 284, 124, pixmaps[1], 0, 0, SRCCOPY);
	TextOut(hdc, 290, 110, txt, strlen(txt));

	// итого
	bzero(result, 5);
	bzero(txt, 20);
	itoa(score + redBonus + powerBonus + cherryBonus, result);
	strcat(txt, "Score: ");
	strcat(txt, result);

	TextOut(hdc, 300, 150, txt, strlen(txt));

	SetBkColor(hdc, colorWhite);
	SetTextColor(hdc, colorBlack);

	if (appType == CLIENT_APPLICATION) {
		TextOut(hdc, 300, INPUT_TEXT_Y, "Client PAC-GIRL", strlen("Client PAC-GIRL"));
	} else if (appType == SERVER_APPLICATION) {
		TextOut(hdc, 300, INPUT_TEXT_Y, "Server PAC-MAN", strlen("Server PAC-MAN"));
	} else {
		TextOut(hdc, 300, INPUT_TEXT_Y, "Press 'q' or ESC", strlen("Press 'q' or ESC"));
	}

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
		*x = MAP_SIZE_X - 2;
	} else if (*x > MAP_SIZE_X - 2) {
		*x = 0;
	}

	if (*y < 0) {
		*y = MAP_SIZE_Y - 1;
	} else if (*y > MAP_SIZE_Y - 1) {
		*y = 0;
	}
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
			} else if (oldRedVal == POWER_FOOD) {
				// поверап
				score++;
				powerBonus += SCORE_POWER_BONUS;
				// обнавляем время когда RED стал съедобным
				redTime = current_timestamp();
			} else if (oldRedVal == CHERRY) {
				// вишню
				cherryBonus += SCORE_CHERY_BONUS;
			}

			oldRedVal = EMPTY;
		}
	} else if (redY == pacGirlY && redX == pacGirlX) {
		// проверяем что Pac-Girl съела на месте RED
		if (oldRedVal == FOOD) {
			// еду
			score++;
		} else if (oldRedVal == POWER_FOOD) {
			// поверап
			score++;
			powerBonus += SCORE_POWER_BONUS;
			// обнавляем время когда RED стал съедобным
			redTime = current_timestamp();
			// RED становится съедобным
			redFlag = 0;
		} else if (oldRedVal == CHERRY) {
			// вишню
			cherryBonus += SCORE_CHERY_BONUS;
		}

		oldRedVal = EMPTY;
	}
	return 0;
}

/**
 * Отоброзить результат игры
 *
 *  Win32 API
 *  hdc - то где рисуем результат
 */
void showGameResult(HDC hdc) {
	drawBox(hdc, 300, HEIGHT - 60, WIDTH, HEIGHT, colorWhite);

	SetBkColor(hdc, colorWhite);
	SetTextColor(hdc, colorBlack);
	TextOut(hdc, 300, INPUT_TEXT_Y, "Press ENTER", strlen("Press ENTER"));

	if (score >= foodToWIN) {
		SetBkColor(hdc, colorBlack);
		SetTextColor(hdc, colorGreen);
		TextOut(hdc, 290, 20, "YOU WINNER :)", strlen("YOU WINNER :)"));
	} else {
		SetBkColor(hdc, colorBlack);
		SetTextColor(hdc, colorRed);
		TextOut(hdc, 290, 20, "YOU LOOSER :(", strlen("YOU LOOSER :("));
	}
}

/**
 * Начальные настройки по карте
 * 1. Время начала игры
 * 2. Определяем количество очков для победы
 * 3. Начальное положение персонажей
 */
void init() {
	// запоминаем время начала игры
	startTime = current_timestamp();

	pacmanLastUpdateTime = startTime;
	redLastUpdateTime = startTime;
	pacGirlLastUpdateTime = startTime;
	redTime = startTime;
	score = 1;
	redBonus = 0;
	powerBonus = 0;
	cherryBonus = 0;
	foodToWIN = 0;
	redFlag = 1;
	redTime = 0;
	cherryFlag = 0;
	refreshCherry = 0;
	refreshDoor = 0;
	oldRedVal = FOOD;
	oldPacGirlVal = FOOD;
	dxRed = 1;
	dyRed = 0;
	dx = 0;
	dy = 0;
	dxPacGirl = 0;
	dyPacGirl = 0;
	needStopGame = 0;

	// загружаем из файла карту уровня
	readMapFromFile("MAP.TXT", &map[0][0], MAP_SIZE_Y, MAP_SIZE_X);

	for (int i = 0; i < MAP_SIZE_Y; i++) {
		for (int j = 0; j < MAP_SIZE_X; j++) {

			// надо съесть поверапы и всю еду
			if (map[i][j] == FOOD || map[i][j] == POWER_FOOD) {
				// сколько очков нужно для выигрыша
				foodToWIN++;
			} else if (map[i][j] == PACMAN) {
				pacmanY = i;
				pacmanX = j;
				foodToWIN++;
			} else if (map[i][j] == RED) {
				redY = i;
				redX = j;
				foodToWIN++;
			} else if (map[i][j] == PACGIRL) {
				pacGirlY = i;
				pacGirlX = j;
				foodToWIN++;

				if (appType == SERVER_APPLICATION) {
					score++;
				} else if (appType == SINGLE_APPLICATION) {
					map[pacGirlY][pacGirlX] = FOOD;
				}
			} else if (map[i][j] == CHERRY) {
				cherryY = i;
				cherryX = j;
				map[cherryY][cherryX] = EMPTY;
			}
		}
	}

	oldX = pacmanX;
	oldY = pacmanY;

	oldPacGirlX = pacGirlX;
	oldPacGirlY = pacGirlY;

	oldXRed = redX;
	oldYRed = redY;
}

/**
 * Алгоритм призрака гоняющегося за PACMAN
 * return 0 - Конец игры
 *        1 - PACMAN еще жив
 */
int redState() {
	// проверяем, у RED задоно ли направление движения
	if (dxRed != 0 || dyRed != 0) {
		long t1 = current_timestamp();
		if (t1 - redLastUpdateTime > RED_SPEED) {
			redX = redX + dxRed;
			redY = redY + dyRed;
			redLastUpdateTime = t1;

			moveBound(&redX, &redY);

			if (isNotWell(redY, redX)) {
				map[oldYRed][oldXRed] = oldRedVal;
				oldRedVal = map[redY][redX];

				if (redX == 15 && redY >= 7 && redY <= 10) {
					dyRed = -1;
					dxRed = 0;
				} else if (dxRed != 0) {
					if (redFlag && redY != pacmanY) {
						if (isNotWellOrDoor(redY + 1, redX)
								&& isNotWellOrDoor(redY - 1, redX)) {
							if (abs(redY + 1 - pacmanY)	< abs(redY - 1 - pacmanY)) {
								dyRed = 1;
							} else {
								dyRed = -1;
							}
						} else if (isNotWellOrDoor(redY + 1, redX)) {
							if (abs(redY + 1 - pacmanY) < abs(redY - pacmanY)) {
								dyRed = 1;
							}
						} else if (isNotWellOrDoor(redY - 1, redX)) {
							if (abs(redY - 1 - pacmanY) < abs(redY - pacmanY)) {
								dyRed = -1;
							}
						}
					} else {
						if (isNotWellOrDoor(redY + 1, redX)) {
							dyRed = rand() % 2;
						}

						if (isNotWellOrDoor(redY - 1, redX)) {
							dyRed = -1 * (rand() % 2);
						}
					}

					if (dyRed != 0) {
						dxRed = 0;
					}

				} else if (dyRed != 0) {
					if (redFlag && redX != pacmanX) {
						if (isNotWellOrDoor(redY, redX + 1)
								&& isNotWellOrDoor(redY, redX - 1)) {
							if (abs(redX + 1 - pacmanX) < abs(redX - 1 - pacmanX)) {
								dxRed = 1;
							} else {
								dxRed = -1;
							}
						} else if (isNotWellOrDoor(redY, redX + 1)) {
							if (abs(redX + 1 - pacmanX) < abs(redX - pacmanX)) {
								dxRed = 1;
							}
						} else if (isNotWellOrDoor(redY - 1, redX)) {
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
				if (redX == 15 && redY >= 7 && redY <= 10) {
					dyRed = -1;
					dxRed = 0;
				} else {

					redX = oldXRed;
					redY = oldYRed;

					if (dxRed != 0) {
						dxRed = 0;
						if (isNotWellOrDoor(redY + 1, redX)) {
							dyRed = 1;
						} else if (isNotWellOrDoor(redY - 1, redX)) {
							dyRed = -1;
						}
					} else {
						dyRed = 0;
						if (isNotWellOrDoor(redY, redX + 1)) {
							dxRed = 1;
						} else if (isNotWellOrDoor(redY, redX - 1)) {
							dxRed = -1;
						}
					}
				}

			}

			// сеъеи ли PACMAN привидение (или оно нас)
			if (pacmanLooser()) {
				return 0;
			}
		}
	}

	if (redFlag) {
		map[redY][redX] = RED;
	} else {
		map[redY][redX] = SHADOW;
	}

	return 1;
}

/**
 * Создание сервера первого игрока
 * port - порт на катором поднимется сервер
 */
void pacmanServer(char *port) {
	pacmanServerWindows(port);
}

/**
 * Подключение к серверу клиента 2 игрока
 * host - хост на котором работает сервер
 * port - порт на катором работает сервер
 */
void pacmanClient(char *host, char *port) {
	pacmanClientWindows(host, port);
}

/**
 * отправляет через интерфейс сокетов сформерованный пакет
 * данные записываются в глобальный массив buffer
 * затем записываются в сокет
 *
 * - Cоздает пакет с данными о положении персонажей
 *   для отправки с сервера клиенту 2-го игрока
 *
 * - Отправляет на сервер данные с клиента 2-го игрока
 * 	 шлем на сервер только нажатую на клавиатуре кнопку 2м игроком
 */
void writeInSocket() {
	writeInSocketWindows();
}

/**
 * Читаем данные посланные по сети
 * в глобальную переменную buffer (не пересаоздаем его всевремя)
 * - какую кнопку на клавиатуре нажал 2 игрок - для сервера
 * - данные об объектах и очках - для клиента
 */
void readFromSocket() {
	readFromSocketWindows();
}

/**
 * Закрыть сокеты
 */
void closeSockets() {
	// Освобождение ресурсов Winsock
	WSACleanup();
	readFromClientSocket = 0;
}

/**
 * Алгоритм обработки движения PACMAN на карте
 * return 0 - Конец игры
 *        1 - PACMAN еще жив
 */
int pacManState() {
	// проверяем, у PACMAN задоно ли направление движения
	if (dx != 0 || dy != 0) {
		long t = current_timestamp();

		// должен ли PACMAN переместиться на новую клетку
		if (t - pacmanLastUpdateTime > PACMAN_SPEED) {
			pacmanX = pacmanX + dx;
			pacmanY = pacmanY + dy;

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
				dxRed = -1 * dxRed;
				dyRed = -1 * dyRed;
				// запоминаем время когда RED стал съедобным
				redTime = current_timestamp();
				// за POWER_FOOD тоже точка
				score++;
				// и даем еще бонус
				powerBonus += SCORE_POWER_BONUS;
			} else if (map[pacmanY][pacmanX] == CHERRY) {
				cherryBonus += SCORE_CHERY_BONUS;
			}

			if (isNotWellOrDoor(pacmanY, pacmanX)) {
				// если в новой клетке не дверь то в старой делаем пустую клетку
				map[oldY][oldX] = EMPTY;
			} else {
				// если в новой клетке стена WALL или дверь DOOR
				// остаемся на прошлой клетке
				pacmanY = oldY;
				pacmanX = oldX;
				// вектор движения сбрасываем (PACMAN останавливается)
				dx = 0;
				dy = 0;
			}

			// рисуем пакмена в координатах текущей клетки карты
			map[pacmanY][pacmanX] = PACMAN;

			// если съеденны все FOOD и POWER_FOOD - PACMAN выиграл
			if (score >= foodToWIN) {
				return 0;
			}

			// сеъеи ли PACMAN привидение (или оно нас)
			if (pacmanLooser()) {
				return 0;
			}

		}

	}
	return 1;
}

/**
 * Алгоритм обработки движения PACGIRL на карте
 * return 0 - Конец игры
 *        1 - PACMAN еще жив
 */
int pacGirlState() {
	// проверяем, у pacGirl задоно ли направление движения
	if (dxPacGirl != 0 || dyPacGirl != 0) {
		long t1 = current_timestamp();
		if (t1 - pacGirlLastUpdateTime > PACGIRL_SPEED) {
			if (pacGirlLastUpdateTime == startTime
					&& map[pacGirlY][pacGirlX] == FOOD) {
				score++;
			}

			pacGirlX = pacGirlX + dxPacGirl;
			pacGirlY = pacGirlY + dyPacGirl;
			pacGirlLastUpdateTime = t1;

			moveBound(&pacGirlX, &pacGirlY);

			// если текущая клетка с едой, увиличиваем счетчик съеденного
			if (map[pacGirlY][pacGirlX] == FOOD) {
				score++;
			} else if (map[pacGirlY][pacGirlX] == POWER_FOOD) {
				// RED становится съедобным
				redFlag = 0;
				// бежит в обратную сторону
				dxRed = -1 * dxRed;
				dyRed = -1 * dyRed;
				// запоминаем время когда RED стал съедобным
				redTime = current_timestamp();
				// за POWER_FOOD тоже точка
				score++;
				// и даем еще бонус
				powerBonus += SCORE_POWER_BONUS;
			} else if (map[pacGirlY][pacGirlX] == CHERRY) {
				cherryBonus += SCORE_CHERY_BONUS;
			}

			if (isNotWellOrDoor(pacGirlY, pacGirlX)) {
				// если в новой клетке не дверь то в старой делаем пустую клетку
				oldPacGirlVal = map[pacGirlY][pacGirlX];
				map[oldPacGirlY][oldPacGirlX] = EMPTY;
			} else {
				// если в новой клетке стена WALL или дверь DOOR
				// остаемся на прошлой клетке
				pacGirlY = oldPacGirlY;
				pacGirlX = oldPacGirlX;
				// вектор движения сбрасываем (PACMAN останавливается)
				dxPacGirl = 0;
				dyPacGirl = 0;
			}

			// рисуем пакмена в координатах текущей клетки карты
			map[pacGirlY][pacGirlX] = PACGIRL;

			// если съеденны все FOOD и POWER_FOOD - PACMAN выиграл
			if (score >= foodToWIN) {
				return 0;
			}

			// сеъеи ли PACMAN привидение (или оно нас)
			if (pacmanLooser()) {
				return 0;
			}
		}
	}

	return 1;
}

/**
 * Обработка на Сервере нажатых кнопок 2 игроком
 * в своем клиенте
 */
void player2() {
	// сбрасываем значение ранее нажатой кнопки
	player2PressKey = EMPTY;
	// пробуем прочитать данные из сокета
	readFromSocket();
	switch (player2PressKey) {
	// key UP
	case 111:                // XLib
	case 65: // Linux
	case 72: // DOS
		dyPacGirl = -1;
		dxPacGirl = 0;
		break;
		// key DOWN
	case 116: // XLib
	case 66: // Linux
	case 80: // DOS
		dyPacGirl = 1;
		dxPacGirl = 0;
		break;
		// key LEFT
	case 113: // XLib
	case 68: // Linux
	case 75: // DOS
		dxPacGirl = -1;
		dyPacGirl = 0;
		break;
		// key RIGHT
	case 114: // XLib
	case 67: // Linux
	case 77: // DOS
		dxPacGirl = 1;
		dyPacGirl = 0;
		break;
	}
}

/**
 * Обработка нажатых кнопок на Сервере 1 игроком
 * с сервера можно управлять обоими персонажами
 * Pac-Man : стрелочки
 * Pac-Girl:
 * 		a - влево
 * 		s - вниз
 * 		d - вправо
 * 		w - вверх
 *
 *  ch - код нажатой кнопки
 */
void player1(int ch) {
	switch (ch) {
	// key UP
	case 111: // XLib
	case 65: // Linux
	case 72: // DOS
		dy = -1;
		dx = 0;
		break;
	case 25: // XLib
	case 119:
		dyPacGirl = -1;
		dxPacGirl = 0;
		break;
		// key DOWN
	case 116: // XLib
	case 66: // Linux
	case 80: // DOS
		dy = 1;
		dx = 0;
		break;
	case 39: // XLib
	case 115:
		dyPacGirl = 1;
		dxPacGirl = 0;
		break;
		// key LEFT
	case 113: // XLib
	case 68: // Linux
	case 75: // DOS
		dx = -1;
		dy = 0;
		break;
	case 38: // XLib
	case 97:
		dxPacGirl = -1;
		dyPacGirl = 0;
		break;
		// key RIGHT
	case 114: // XLib
	case 67: // Linux
	case 77: // DOS
		dx = 1;
		dy = 0;
		break;
	case 40: // XLib
	case 100:
		dxPacGirl = 1;
		dyPacGirl = 0;
		break;
	}
}

/**
 * Win32 API
 *
 * Обновить карту / персонажей, двери, черешню
 * для Серверной, Одиночной, Кооперативной игры
 *
 * hdc - то где рисуем обнавленную карту
 */
void refreshSingleGame(HDC hdc) {
	if (!cherryFlag && redFlag && !cherryBonus
			&& current_timestamp() - startTime > CHERRY_TIME) {
		openDoors();
	}
	// надо ли перерисовать карту
	if (oldX != pacmanX || oldY != pacmanY || oldXRed != redX || oldYRed != redY
			|| oldPacGirlX != pacGirlX || oldPacGirlY != pacGirlY) {
		// если игра сетевая, надо отправить данные о карте
		// 2у игроку, для чего записываем данные в сокет

		if (appType == SERVER_APPLICATION && !connectionLost) {
			writeInSocket();
		}

		refreshMap(hdc);
	}
	// надо ли RED перейти в режим погони
	if (redFlag == 0 && (current_timestamp() - redTime > RED_TIME)) {
		redFlag = 1;
		if (dyRed == 0 && dxRed == 0) {
			dyRed = -1;
		}
	}
	// запоминаем текущие координаты PACMAN
	oldX = pacmanX;
	oldY = pacmanY;
	// запоминаем текущие координаты RED
	oldXRed = redX;
	oldYRed = redY;
	// запоминаем текущие координаты PACGIRL
	oldPacGirlX = pacGirlX;
	oldPacGirlY = pacGirlY;
}

/**
 * Win32 API
 *
 * Обновить карту / персонажей, двери, черешню
 * для игры 2 игроком на клиенте с другого компьютера
 *
 * hdc - то где рисуем обнавленную карту
 */
void refreshClientGame(HDC hdc) {
	map[oldY][oldX] = EMPTY;
	map[pacmanY][pacmanX] = PACMAN;
	if (redFlag) {
		map[oldYRed][oldXRed] = oldRedVal;
		map[redY][redX] = RED;
	} else {
		map[oldYRed][oldXRed] = oldRedVal;
		map[redY][redX] = SHADOW;
	}
	if (pacGirlY != oldPacGirlY || pacGirlX != oldPacGirlX) {
		map[oldPacGirlY][oldPacGirlX] = EMPTY;
	}
	if (refreshDoor) {
		map[doorY][doorX] = doorVal;
	}
	if (refreshCherry) {
		map[cherryY][cherryX] = cherryVal;
	}
	map[pacGirlY][pacGirlX] = PACGIRL;
	if (oldX != pacmanX || oldY != pacmanY || oldXRed != redX || oldYRed != redY
			|| oldPacGirlX != pacGirlX || oldPacGirlY != pacGirlY) {

		refreshMap(hdc);
		// запоминаем текущие координаты PACMAN
		oldX = pacmanX;
		oldY = pacmanY;
		// запоминаем текущие координаты RED
		oldXRed = redX;
		oldYRed = redY;
		// запоминаем текущие координаты PACGIRL
		oldPacGirlX = pacGirlX;
		oldPacGirlY = pacGirlY;
	}
}

/**
 *  Win32 API
 *
 *  Основной цикл игры
 *  для игры 2 игроком на клиенте с другого компьютера
 *  в сетевом режиме
 *   	appType == CLIENT_APPLICATION
 *  hdc - то где рисуем
 */
int gameClientMode(HDC hdc) {
	MSG msg;
	do {
		// обработка сообщений
		if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Нажата кнопка "ESC"
		if (needExitFromGame) {
			return -1;
		}

		if (connectionLost) {
			// если соединение потеряно, завершаем игру
			return 0;
		}

		// читаем данные о карте / персонажах с сервера
		readFromSocket();

		if (connectionLost) {
			// если соединение потеряно, завершаем игру
			return 0;
		}

		// обновляем карту и все элементы какие нужно на экране
		refreshClientGame(hdc);

		// Выход из игры 'q'
	} while (!needStopGame);

	return 1;
}

/**
 *  Win32 API
 *
 *  Основной цикл игры
 *  для одинойной или кооперативной игры на одном компьютере
 *      appType == SINGLE_APPLICATION
 *  или 1 игроком на сервере в сетевом режиме
 *   	appType == SERVER_APPLICATION
 *
 *  hdc - то где рисуем
 */
int game(HDC hdc) {
	MSG msg;
	do {
		// обработка сообщений
		if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// обновить карту / персонажей на экране
		refreshSingleGame(hdc);

		if (appType == SERVER_APPLICATION && !connectionLost) {
			// Если игра сетевая, нужно обработать нажатые 2м пользователем
			// в своем клиенте кнопки сервером, для этого
			// надо прочитать данные из сокета и обработать
			player2();
		}

		// Pac-Man
		if (!pacManState()) {
			return 0;
		}

		// RED
		if (!redState()) {
			return 0;
		}

		// Pac-Girl
		if (!pacGirlState()) {
			return 0;
		}

		// Нажата кнопка "ESC"
		if (needExitFromGame) {
			return -1;
		}

		// Выход из игры 'q'
	} while (!needStopGame);

	return 1;
}

/**
 * Ждем подключения 2 го игрока к серверу
 * он должен прислать чтонибуть
 * после создания соединения и отресовки карты
 * нужно для медленных компов
 *
 * hdc - то где рисуем
 */
int waitPlayer2(HDC hdc) {

	MSG msg;

	SetBkColor(hdc, colorWhite);
	SetTextColor(hdc, colorBlack);
	TextOut(hdc, 300, INPUT_TEXT_Y, "Wait P2 action", strlen("Wait P2 action"));

	if (appType == SERVER_APPLICATION) {
		do {
			// обработка сообщений
			if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT)
					break;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			// Нажата кнопка "ESC"
			if (needExitFromGame) {
				return -1;
			}

			if (readFromClientSocket) {
				break;
			}


			if (connectionLost) {
				return 0;
			}

			readFromSocket();

			// Выход из игры 'q'
		} while (!needStopGame);
	}
	return 1;
}


/**
 * Win32 API
 *
 * Ввод и редактирование хоста и порта для соединения с игровмм сервером
 * или поднятие своего сервера
 *
 *  inputHost - хост переданный программе как параметр или от пред идущего вызова
 *  inputPort - порт переданный программе как параметр или от пред идущего вызова
 *
 *  return - 0 пользователь хочет выйти из программы
 *  return - 1 нужно запустить игру
 *
 *  hdc - то где рисуем
 */
int inputHostPort(HDC hdc) {
	MSG msg;

	bzero(serverHostPort, INPUT_TEXT_LENGHT);
	if (strlen(param1) > 0 && strlen(param2) > 0 && (strlen(param1) + strlen(param2)) < INPUT_TEXT_LENGHT) {
		strcpy(serverHostPort, param1);
		strcat(serverHostPort, " ");
		strcat(serverHostPort, param2);
	} else if (strlen(param1) > 0 && strlen(param2) < INPUT_TEXT_LENGHT - 1) {
		strcpy(serverHostPort, param1);
	}

	// количество уже введенных символов
	word = strlen(serverHostPort);

	// обновим значение в поле где лежит хост и порт
	SetBkColor(hdc, colorWhite);
	SetTextColor(hdc, colorBlack);
	TextOut(hdc, 125, INPUT_TEXT_Y, serverHostPort, strlen(serverHostPort));

	/* Создадим цикл получения и обработки ошибок */
	while (true) {
		// обработка сообщений
		if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Нажата кнопка "ESC"
		if (needExitFromGame) {
			return -1;
		}

		if (appType == NEED_INIT_NEW_GAME) {
			// нажат ENTER
			if (strlen(param1) > 0 && strlen(param2) > 0) {
				TextOut(hdc, 300, INPUT_TEXT_Y, "Connecting ...", strlen("Connecting ..."));
			} else if (strlen(param1) > 0) {
				TextOut(hdc, 300, INPUT_TEXT_Y, "Server wait ...", strlen("Server wait ..."));
			} else {
				TextOut(hdc, 300, INPUT_TEXT_Y, "Press 'q' or ESC", strlen("Press 'q' or ESC"));
			}
			return 1;
		}
	}

	return 0;
}

/**
 * Win32 API
 *
 * Парсим параметы командной строки переданные приложению из консоли
 * и представляем как стандартные argc и *argv[] 
 *
 * argw - ссылка куда сохраним массив параметров
 * return - количество параметров
 */
int processCommandLine(wchar_t ***argw) {
	WCHAR *cmdline;
	wchar_t *argbuf;
	wchar_t **args;
	int argc_max;
	int i, q, argc;
	char *p = GetCommandLine();
	int nChars = MultiByteToWideChar(CP_ACP, 0, p, -1, NULL, 0);
	// allocate it
	cmdline = new WCHAR[nChars];
	MultiByteToWideChar(CP_ACP, 0, p, -1, (LPWSTR) cmdline, nChars);

	i = wcslen(cmdline) + 1;
	argbuf = (wchar_t*) malloc(sizeof(wchar_t) * i);
	wcscpy(argbuf, cmdline);

	argc = 0;
	argc_max = 64;
	args = (wchar_t**) malloc(sizeof(wchar_t*) * argc_max);
	if (args == NULL) {
		free(argbuf);
		return (0);
	}

	// парсим commandline в argc и *argv[] формат как в нормальной программе на C/C++
	i = 0;
	while (argbuf[i]) {
		while (argbuf[i] == L' ')
			i++;

		if (argbuf[i]) {
			if ((argbuf[i] == L'\'') || (argbuf[i] == L'"')) {
				q = argbuf[i++];
				if (!argbuf[i])
					break;
			} else
				q = 0;

			args[argc++] = &argbuf[i];

			if (argc >= argc_max) {
				argc_max += 64;
				args = (wchar_t**) realloc(args, sizeof(wchar_t*) * argc_max);
				if (args == NULL) {
					free(argbuf);
					return (0);
				}
			}

			while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (argbuf[i] != L' ')))
				i++;

			if (argbuf[i]) {
				argbuf[i] = 0;
				i++;
			}
		}
	}

	args[argc] = NULL;
	*argw = args;

	delete[] cmdline;
	return argc;
}

/**
 * Win32 API
 *
 * точка входа в программу для Win32 приложений
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// обнуляем массивы куда засуну параметры переданные в программу из консоли
	bzero(param1, INPUT_TEXT_LENGHT);
	bzero(param2, INPUT_TEXT_LENGHT);

	wchar_t **argv;

	// достаем параметры переданные в консоли нашей программе
	int argc = processCommandLine(&argv);

	if (argc == 2) {
		// порт был передан как аргумент вызова программы
		wcstombs(param1, argv[1], wcslen(argv[1]) + 1);
	} else if (argc == 3) {
		// хост был передан как аргумент вызова программы
		wcstombs(param1, argv[1], wcslen(argv[1]) + 1);
		// порт был передан как аргумент вызова программы
		wcstombs(param2, argv[2], wcslen(argv[2]) + 1);
	}

	WNDCLASSEX wcex;
	HWND hWnd;
	MSG msg;

	// Регистрация класса окна
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = PRG_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex))
	return FALSE;

	// Создание окна
	hWnd = CreateWindow(PRG_CLASS, TITLE, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// результат игры
	int result = 0;

	HDC hdc = GetDC(hWnd);
	while (inputHostPort(hdc)) {
		// стираем все что было введино
		drawBox(hdc, 0, 0, WIDTH, HEIGHT - 60, colorBlack);
		SetBkColor(hdc, colorWhite);
		SetTextColor(hdc, colorBlack);;

		if (strlen(param1) > 0 && strlen(param2) > 0) {
			// запущен как клиент
			pacmanClient(param1, param2);
			if (appType != CLIENT_APPLICATION) {
				appType = SINGLE_APPLICATION;
			}
		} else if (strlen(param1) > 0) {
			// запущен как сервер
			pacmanServer(param1);

			// ждем подключения 2-го игрока и пока он отрисует у себя карту
			result = waitPlayer2(hdc);

			if (result == -1) {
				break;
			}

			if (appType != SERVER_APPLICATION) {
				appType = SINGLE_APPLICATION;
			}
		} else {
			appType = SINGLE_APPLICATION;
		}

		// Начальные настройки по карте
		init();

		// нарисовать карту и персонажей на экране
		showMap(hdc);

		// игра
		if (appType == CLIENT_APPLICATION) {
			// сообщаем серверу что можно начинать игру передав ' '
			// сервер ждет сообщения от клиента чтоб начать игру когда 2 игрок готов
			// нужно для медленных компов долго грузящих карту на клиенте
			writeInSocket();

			// 2 игрок за PAC-GIRL
			result = gameClientMode(hdc);
		} else {
			// 1 игрок за PAC-MAN
			result = game(hdc);

			// Надо обновить данные на клиенте 2го плеера
			if (appType == SERVER_APPLICATION && !connectionLost) {
				writeInSocket();
			}

		}

		// обновить карту и персонажей на экране
		refreshMap(hdc);

		// отоброзить результат игры, выгрузить картинки из памяти
		showGameResult(hdc);

		// закрыть сокеты
		closeSockets();

		// игра закончена
		appType = NO_GAME;

		if (result == -1) {
			break;
		}
	}

	return 0;
}

/**
 * Win32 API
 * Реализация CALLBACK функции для обработки сообщений Win32 API
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
		LPARAM lParam) {
	if (wParam == SC_RESTORE) {
		// окно было восстановлно по этому нужно перерисовать все содержимое окна
		refreshWindow = 1;
	}

	switch (message) {
	// событие создания окна
	case WM_CREATE:
		// инициализируем игру
		init();
		// Загружаем в память спрайты
		initPixelmap();
		break;

		// событие рисования в окне (полное или частичная перерисовка)
	case WM_PAINT: {
		// Получаем контекст устройства для рисования
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		if (refreshWindow) {
			// перерисовать все надо
			drawBox(hdc, 0, 0, WIDTH, HEIGHT - 60, colorBlack);
			drawBox(hdc, 0, HEIGHT - 60, WIDTH, HEIGHT, colorWhite);
			if (appType == SINGLE_APPLICATION || appType == SERVER_APPLICATION
					|| appType == CLIENT_APPLICATION) {
				// нарисовать карту полностью
				showMap(hdc);
			}
			refreshWindow = 0;
		}

		if (appType == SINGLE_APPLICATION || appType == SERVER_APPLICATION
				|| appType == CLIENT_APPLICATION) {
			refreshMap(hdc);
		}

		drawBox(hdc, 125, INPUT_TEXT_Y, 290, INPUT_TEXT_Y + 18, colorWhite);
		SetBkColor(hdc, colorWhite);
		SetTextColor(hdc, colorBlack);
		TextOut(hdc, 10, INPUT_TEXT_Y, "Host+port or port:",
				lstrlen("Host+port or port:"));
		TextOut(hdc, 125, INPUT_TEXT_Y, serverHostPort, strlen(serverHostPort));

		if (appType == SINGLE_APPLICATION) {
			TextOut(hdc, 300, INPUT_TEXT_Y, "Press 'q' or ESC", strlen("Press 'q' or ESC"));
		} else if (appType == CLIENT_APPLICATION) {
			TextOut(hdc, 300, INPUT_TEXT_Y, "Client PAC-GIRL", strlen("Client PAC-GIRL"));
		} else if (appType == SERVER_APPLICATION) {
			TextOut(hdc, 300, INPUT_TEXT_Y, "Server PAC-MAN", strlen("Server PAC-MAN"));
		} else if (appType == NEED_INIT_NEW_GAME) {
			TextOut(hdc, 300, INPUT_TEXT_Y, "Connecting ...", strlen("Connecting ..."));
		} else {
			TextOut(hdc, 300, INPUT_TEXT_Y, "Press ENTER", strlen("Press ENTER"));
		}

		// Заканчиваем рисование
		EndPaint(hWnd, &ps);
	}
		break;

		// событие нажатия кнопки клавиатуры
	case WM_KEYDOWN: {
		// Обрабатываем нажатие клавиши.
		if (wParam == VK_ESCAPE) {
			// был нажат ESC - выход из приложения
			needExitFromGame = 1;
		} else if (appType == SINGLE_APPLICATION
				|| appType == SERVER_APPLICATION) {
			// нажата кнопка во время одиночной игры или сетевой у игрока 1
			switch (wParam) {
			case VK_UP:
				player1(72);
				break;
			case 'W':
				player1(25);
				break;
			case VK_DOWN:
				player1(80);
				break;
			case 'S':
				player1(39);
				break;
			case VK_LEFT:
				player1(75);
				break;
			case 'A':
				player1(38);
				break;
			case VK_RIGHT:
				player1(77);
				break;
			case 'D':
				player1(40);
				break;
			case 'Q':
				needStopGame = 1;
				break;
			}
		} else if (appType == CLIENT_APPLICATION) {
			// Обработка нажатых кнопок 2м игроком при сетевой игре
			switch (wParam) {
			case VK_UP:
				player2PressKey = 65;
				break;
			case VK_DOWN:
				player2PressKey = 66;
				break;
			case VK_LEFT:
				player2PressKey = 68;
				break;
			case VK_RIGHT:
				player2PressKey = 67;
				break;
			case 'Q':
				needStopGame = 1;
				break;
			}

			// отправляем на сервер что было нажато 2ым игроком
			writeInSocket();
		} else if (appType == NO_GAME) {
			// обработка когда игра еще не начата
			if (wParam == VK_RETURN) {
				// нажат ENTER
				bzero(param1, INPUT_TEXT_LENGHT);
				bzero(param2, INPUT_TEXT_LENGHT);
				if (strlen(serverHostPort) > 0) {
					// парсим что введено пользователем
					int j = INPUT_TEXT_LENGHT;
					for (int i = 0; i < INPUT_TEXT_LENGHT; i++) {
						if (serverHostPort[i] == ' ') {
							if (j == INPUT_TEXT_LENGHT) {
								j = 0;
							} else {
								break;
							}
						} else if (j == INPUT_TEXT_LENGHT) {
							param1[i] = serverHostPort[i];
						} else {
							param2[j] = serverHostPort[i];
							j++;
						}
					}
				}
				appType = NEED_INIT_NEW_GAME;
			}
		}
	}
		break;

		// событие нажатия кнопки которую можно отобразить
	case WM_CHAR:
		if (appType == NO_GAME) {
			// игра еще не начата, ввод хоста и порта
			if (wParam == VK_BACK) {
				// Нажат BACKSPACE
				if (word > 0) {
					// уменьшаем количество введенных символо
					word--;

					// стираем последний символ
					serverHostPort[word] = 0;

					// перересовать окно
					InvalidateRect(hWnd, NULL, false);
				}
			} else if (word < INPUT_TEXT_LENGHT - 1) {
				// Нажата любая другая кнопка кроме BACKSPACE

				// добавляем к уже введенному еще символ
				serverHostPort[word] = wParam;

				// увеличиваем количество введенных символов
				word++;

				// перересовать окно
				InvalidateRect(hWnd, NULL, false);
			}
		}
		break;

		// событие уничтожения окна
	case WM_DESTROY:
		// освобождаем ресурсы
		freePixmaps();
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

