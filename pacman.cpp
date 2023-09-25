/**
 Super Turbo NET Pac-Man v1.7
 реализация Pac-Man для DOS (IBM PC XT Intel 8088) и консольная версия для GNU/Linux

 Из Linux, консольная версия для GNU/Linux собирается так:
 > make pacman
  или
 > gcc pacman.cpp

 запустить консольную одиночную игру для GNU/Linux
 > ./pacman
 или сетевую как сервер на порту 7777 (порт можно любой)
 > ./pacman 7777
 или сетевую как клиент который соединится с хостом localhost на порт 7777 (хост и порт любые можно)
 > ./pacman localhost 7777

 Для DOS собирается в Windows 10 с помащью Open Watcom 1.9 - http://www.openwatcom.org/
 скачать open-watcom-c-win32-1.9.exe можно тут https://sourceforge.net/projects/openwatcom/files/open-watcom-1.9/
 и понадобится исходный код всего проекта MTCP - http://brutmanlabs.org/mTCP/
 все файлы проекта PACMAN надо положить в проект MTCP по пути C:/MTCP/APPS/PACMAN
 > cd C:/MTCP/APPS/PACMAN
 > wmake pacman

 Из консоли в DOS можно запустить так:
 одиночная игра
 > pacman.exe
 сетевая игра сервер (играем PAC-MAN)
 > pacman.exe 7777
 сетевая игра клиент (играем PAC-GIRL)
 > pacman.exe localhost 7777

 Драйвер для Intel 8∕16 LAN Adapter
 https://www.vogonswiki.com/index.php/Intel_8%E2%88%9516_LAN_Adapter

 A Guide to Networking in DOS
 http://dosdays.co.uk/topics/networking_in_dos.php

 mTCP - TCP/IP applications for your PC compatible retro-computers
 http://brutmanlabs.org/mTCP/
 
 Запуск DOSBox-X для отладки в Windows программы для DOS по сети
 > DOSBox-X -conf ne2000.conf

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
#include <sys/time.h>   //gettime
#include <termios.h>
#include <sys/types.h>  // TCP/IP
#include <sys/socket.h> // TCP/IP
#include <netinet/in.h> // TCP/IP
#include <netdb.h>      // TCP/IP 
#else
// DOS
#include <dos.h>      	// gettime
#include <conio.h>    	// kbhit
#include <graph.h>    	// 2D graphics 
#include <malloc.h>   	// malloc / free 
#include <bios.h>     	// mTCP keybord handlers
#include <io.h>       	// open file
#include "types.h"    	// mTCP
#include "packet.h"   	// mTCP
#include "arp.h"      	// mTCP
#include "udp.h"      	// mTCP
#include "dns.h"      	// mTCP
#include "tcp.h"      	// mTCP
#include "tcpsockm.h" 	// mTCP
#endif

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
// не сетевая игра
const int SINGLE_APPLICATION = 0;
// игровой сервер 1-го игрока
const int SERVER_APPLICATION = 1;
// клиент 2-го игрока
const int CLIENT_APPLICATION = 2;

// рамер буфера для сетевого взаимдействия
#define RECV_BUFFER_SIZE (1024)

// массив для сетевого взаимодействия
// весь обмен идет через него в функциях записи / чтения
// четез интерфейс сокетов
unsigned char buffer[RECV_BUFFER_SIZE];

// кнопка нажатая 2 игроком на клиенте 2-го игрока
char player2PressKey;

// тип приложения
// SINGLE_APPLICATION - не сетевая игра
// SERVER_APPLICATION - игровой сервер 1-го игрока
// CLIENT_APPLICATION - клиент 2-го игрока
int appType = SINGLE_APPLICATION;

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
char *images[IMAGES_SIZE];

/**
 * Количество видов стен загружаемых из файлов как спрайты
 */
#define WALL_SIZE (23)

// активная видео страница при запуске  
int old_apage;

// отображаемая видео страница  
int old_vpage;

// активная видео страница
int activePage = 0;

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
 * Тут идут функции только для Linux
 * Linux Linux Linux Linux Linux Linux Linux Linux Linux Linux Linux 
 */
#ifdef __linux__

// серверный socket - ждет подключения клиентов
// использует только сервер
int serverSocket;

// клиентский socket
// запустились как сервер или как клиент 2-го игрока
// всегда в этот сокет пишем или читаем данные
// отправить на сервер / клиент 2-го игрока
// прочитать данные на сервере / клиенте 2-го игрока
int clientSocket;

/**
 * Linux
 *
 * Создание сервера
 * port - порт на катором поднимется сервер
 */
void pacmanServerLinux(char *port) {
	// порт сервера на который ждем подключение клиента
	int serverPort;
	// структура содержащая адрес сервера
	struct sockaddr_in serverAddress;
	// структура содержащая адрес клиента
	struct sockaddr_in clientAddress;
	serverPort = atoi(port);

	// создаем ерверный сокет
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0) {
		printf("\nERROR open socket");
	} else {

		int optval = 1;
		// SO_REUSEPORT позваляет использовать занятый порт не дожидаясь таймаута при повторной игре
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

		bzero((char*) &serverAddress, sizeof(serverAddress));

		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		serverAddress.sin_port = htons(serverPort);

		// пробуем занять порт
		if (bind(serverSocket, (struct sockaddr*) &serverAddress,
				sizeof(serverAddress)) < 0) {
			printf("\nERROR bind\n");
		} else {
			// устанавливаем  serverSocket в состояние прослушивания, будет использоваться для
			// приёма запросов входящих соединений с помощью accept()
			// очередь ожидающих соединений  = 5
			if (listen(serverSocket, 5) < 0) {
				printf("\nERROR listen\n");
			} else {
				socklen_t clilen = sizeof(clientAddress);
				clientSocket = accept(serverSocket,
						(struct sockaddr*) &clientAddress, &clilen);
				if (clientSocket < 0) {
					printf("\nERROR accept\n");
				} else {
					// переводим сокет в не блокирующий режим
					fcntl(clientSocket, F_SETFL, O_NONBLOCK);
					// сервер поднят успешно и соеденился с клиентом 2-го игрока
					connectionLost = 0;
					// мы являемся сервером
					appType = SERVER_APPLICATION;
				}
			}
		}
	}
}

/**
 * Linux
 *
 * Подключение к серверу клиента 2 игрока
 * host - хост на котором работает сервер
 * port - порт на катором работает сервер
 */
void pacmanClientLinux(char *host, char *port) {
	//printf("Порт: '%s' Хост: '%s'\n", port, host);
	// порт сервера на который ждем подключение клиента
	int serverPort;
	// структура содержащая адрес сервера
	struct sockaddr_in serverAddress;
	// структура содержащая адрес клиента
	struct sockaddr_in clientAddress;
	// хост сервера
	struct hostent *server;

	serverPort = atoi(port);

	// Создаем сокет для отправки сообщений
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0) {
		printf("\nERROR open socket\n");
	} else {
		// получаем хост по имени или ip
		server = gethostbyname(host);
		if (server == NULL) {
			printf("\nERROR gethostbyname\n");
		} else {
			bzero((char*) &serverAddress, sizeof(serverAddress));
			serverAddress.sin_family = AF_INET;
			bcopy((char*) server->h_addr, (char *)&serverAddress.sin_addr.s_addr, server->h_length);
			serverAddress.sin_port = htons(serverPort);
			if (connect(clientSocket, (struct sockaddr*) &serverAddress,
					sizeof(serverAddress)) < 0) {
				printf("\nERROR connect\n");
			} else {
				// переводим сокет в не блокирующий режим
				fcntl(clientSocket, F_SETFL, O_NONBLOCK);
				// клиент 2-го игрока успешно соединился с сервером
				connectionLost = 0;
				// мы являемся клиентом
				appType = CLIENT_APPLICATION;
			}
		}
	}
}

/**
 * Linux
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
void writeInSocketLinux() {
	// очищаем буфер
	bzero(buffer, RECV_BUFFER_SIZE);

	// создаем пакет с данными в buffer для передачи по сети
	createBuffer();

	int n;
	if (appType == SERVER_APPLICATION) {
		// записываем buffer в сокет (отправляем 2-му игроку)
		n = write(clientSocket, buffer, RECV_BUFFER_SIZE);
	} else {
		// записываем buffer в сокет (отправляем на Сервер)
		n = write(clientSocket, buffer, 1);
	}

	if (n < 0) {
		if (appType == SERVER_APPLICATION) {
			printf("\nPac-Girl OFF\n");
		} else {
			printf("\nPac-Man OFF\n");
		}
		connectionLost = 1;
	}
}

/**
 * Linux
 * Читает данные из сокета переданные по сети
 * в глобальную переменную buffer (не пересаоздаем его всевремя)
 * а из нее потом все перекладывается в остальные  переменные (смотри parseBuffer();)
 */
void readFromSocketLinux() {
	fd_set readset;
	FD_ZERO(&readset);
	FD_SET(clientSocket, &readset);

	// Задаём таймаут
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 150;

	// проверяем можно ли что то прочитать из сокета
	int n = select(clientSocket + 1, &readset, NULL, NULL, &timeout);
	if (n > 0 && FD_ISSET(clientSocket, &readset)) {
		// очищаем буфер
		bzero(buffer, RECV_BUFFER_SIZE);

		// читоаем из сокета
		if (appType == SERVER_APPLICATION) {
			n = read(clientSocket, buffer, 1);
		} else {
			n = read(clientSocket, buffer, RECV_BUFFER_SIZE);
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
		}
	}
}

/**
 * Linux
 *
 * Закрыть сокеты
 */
void closeSocketsLinux() {
	if (appType == SERVER_APPLICATION) {
		// закрываем сокеты
		close(clientSocket);
		close(serverSocket);
	} else if (appType == CLIENT_APPLICATION) {
		close(clientSocket);
	}
}

/**
 *
 * Linux
 *
 * Нжата ли кнопка клавиатуры
 * return - 1 да, 0 нет
 */
int kbhit(void) {
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

	if (ch != EOF) {
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
int getch() {
	struct termios oldattr, newattr;
	int ch;
	tcgetattr( STDIN_FILENO, &oldattr);
	newattr = oldattr;
	newattr.c_lflag &= ~( ICANON | ECHO);
	tcsetattr( STDIN_FILENO, TCSANOW, &newattr);
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &oldattr);
	return ch;
}

/**
 * Linux
 *
 * Текущее время в милисекундах
 * printf("milliseconds: %lld\n", milliseconds); - распечатать
 *
 * return - милисекунды
 */
long long current_timestamp() {
	struct timeval te;
	// получить текущее время
	gettimeofday(&te, NULL);
	// вычисляем милисекунды
	long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000LL;
	return milliseconds;
}

/**
 *  Linux 
 *
 *  Перерисовать карту в терменале!
 */
void draw_Linux() {
	system("clear");
	printf("  Super Turbo Net Pac-Man 1.7\n");
	for (int i = 0; i < MAP_SIZE_Y; i++) {
		printf("%s\n", map[i]);
	}
	printf("Score %d! To win %d / %d!", score + redBonus + powerBonus + cherryBonus, score, foodToWIN);
}

/**
 * END Linux END Linux END Linux END Linux END Linux END Linux END Linux 
 */
#else
/**
 * DOS DOS DOS DOS DOS DOS DOS DOS DOS DOS DOS
 * Тут идут функции и переменные только для DOS
 */

// Нажата ли ctrl Break
volatile uint8_t ctrlBreakDetected = 0;

// серверный socket - ждет подключения клиентов
// использует только сервер
TcpSocket *serverSocket;

// клиентский socket
// запустились как сервер или как клиент 2-го игрока
// всегда в этот сокет пишем или читаем данные
// отправить на сервер / клиент 2-го игрока
// прочитать данные на сервере / клиенте 2-го игрока
TcpSocket *clientSocket;

/**
 * DOS
 *
 * callback нажатия Сtrl+Break
 */
void __interrupt __far ctrlBreakHandler() {
	ctrlBreakDetected = 1;
}

/**
 * DOS
 *
 * callback нажатия Ctrl+C
 */
void __interrupt __far ctrlCHandler() {
	// Ничего не делаем
}

/**
 * DOS
 *
 * выгрузить TCP/IP стек
 */
static void shutdown() {
    // Если этого не сделать, операционная система перестанет работать из-за зависшего прерывания таймера.
    Utils::endStack();
}

/**
 * DOS
 *
 * Создание сервера
 * port - порт на катором поднимется сервер
 */
void pacmanServerDOS(char * port) {
	// порт сервера на который ждем подключение клиента
	uint16_t serverPort = atoi(port);
    // Все программы mTCP должны прочитать файл конфигурации mTCP, чтобы узнать, какое прерывание использует пакетный драйвер,
    // каков IP-адрес и сетевая маска, а также некоторые другие необязательные параметры.
	if (Utils::parseEnv() != 0) {
		printf("\nERROR parseEnv\n");
	} else {
	    // Инициализировать TCP/IP стек.
	    // Первый параметр - количество создаваемых TCP сокетов.
	    // Второй параметр - количество исходящих TCP-буферов, которые необходимо создать.
	    // Параметры три и четыре - пользовательские обработчики Ctrl-Break и Ctrl-C.
	    // Если функция возвращает не нуль, это говорит о том, что была ошибка, и программа должна завершиться.
	    // Наиболее распространённой ошибкой является невозможность найти пакетный драйвер, поскольку он не был загружен или был загружен с неправильным номером прерывания.
	    if (Utils::initStack(2, TCP_SOCKET_RING_SIZE, ctrlBreakHandler, ctrlCHandler)) {
	    	printf("\nERROR failed to initialize TCP/IP\n");
	    } else {
	    	// создали серверный сокет
	    	serverSocket = TcpSocketMgr::getSocket();
	    	// пробуем занять порт
	    	serverSocket->listen(serverPort, RECV_BUFFER_SIZE);

	        // Ждем подключения клиента is non-blocking!
	        while (1) {

	          if (ctrlBreakDetected) {
	        	// Пользователь нажал Ctrl+C.
	            break;
	          }

	          // PACKET_PROCESS_SINGLE - это макрос, проверяющий наличие новых пакетов от драйвера пакетов.
	          // Если таковые обнаружены, макрос обрабатывает эти пакеты.
	          PACKET_PROCESS_SINGLE;
	          // Arp::driveArp() используется для проверки ожидающих запросов ARP и повторения их при необходимости.
	          Arp::driveArp();
	          // Tcp::drivePackets() используется для отправки пакетов, которые были поставлены в очередь для отправки.
	          Tcp::drivePackets();
	          clientSocket = TcpSocketMgr::accept();
	          if (clientSocket != NULL) {
	        	  serverSocket->close();
	        	  TcpSocketMgr::freeSocket(serverSocket);
				  // сервер поднят успешно и соеденился с клиентом 2-го игрока
				  connectionLost = 0;
				  // мы являемся сервером
				  appType = SERVER_APPLICATION;
	        	  break;
	          }

	          if (_bios_keybrd(1) != 0) {
				char c = _bios_keybrd(0);

				if ((c == 27) || (c == 3)) {
					break;
				}
	          }
	        }

	        if (appType != SERVER_APPLICATION) {
	        	shutdown();
	        }
	    }
	}
}


/**
 * DOS
 *
 * Подключение к серверу клиента 2 игрока
 * host - хост на котором работает сервер
 * port - порт на катором работает сервер
 */
void pacmanClientDOS(char * host, char * port) {
	// порт сервера на который ждем подключение клиента
	uint16_t serverPort = atoi(port);
    // Все программы mTCP должны прочитать файл конфигурации mTCP, чтобы узнать, какое прерывание использует пакетный драйвер,
    // каков IP-адрес и сетевая маска, а также некоторые другие необязательные параметры.
	if (Utils::parseEnv() != 0) {
		printf("\nERROR parseEnv\n");
	} else {
	    // Инициализировать TCP/IP стек.
	    // Первый параметр - количество создаваемых TCP сокетов.
	    // Второй параметр - количество исходящих TCP-буферов, которые необходимо создать.
	    // Параметры три и четыре - пользовательские обработчики Ctrl-Break и Ctrl-C.
	    // Если функция возвращает не нуль, это говорит о том, что была ошибка, и программа должна завершиться.
	    // Наиболее распространённой ошибкой является невозможность найти пакетный драйвер, поскольку он не был загружен или был загружен с неправильным номером прерывания.
	    if (Utils::initStack(2, TCP_SOCKET_RING_SIZE, ctrlBreakHandler, ctrlCHandler)) {
	    	printf("\nERROR initialize TCP/IP\n");
	    } else {
			// структура содержащая адрес сервера
			IpAddr_t serverAddress;

			// Если был задан числовой IP-адрес, первый вызов Dns::resolve разрешит его, и не придётся ждать.
			// Иначе, нужно в цикле дождаться завершения работы DNS.
			int8_t rc2 = Dns::resolve(host, serverAddress, 1);
			if (rc2 < 0) {
				printf("\nError resolving Server\n" );
				shutdown();
			} else {
		        while (1) {
		            if (ctrlBreakDetected) break;
		            if (!Dns::isQueryPending()) break;

		            // PACKET_PROCESS_SINGLE - это макрос, проверяющий наличие новых пакетов от драйвера пакетов.
		            // Если таковые обнаружены, макрос обрабатывает эти пакеты.
		            PACKET_PROCESS_SINGLE;
		            // Arp::driveArp() используется для проверки ожидающих запросов ARP и повторения их при необходимости.
		            Arp::driveArp();
		            // Tcp::drivePackets() используется для отправки пакетов, которые были поставлены в очередь для отправки.
		            Tcp::drivePackets();

		            // Реализация DNS основана на UDP.
		            // Dns::drivePendingQuery нужен потому, что пакеты UDP могут быть потеряны, и нужен способ определить, нужно ли нам повторно отправить наш DNS-запрос.
		            Dns::drivePendingQuery();
		        }
		        // Ещё один вызов Dns::resolve вернёт окончательный результат.
		        rc2 = Dns::resolve(host, serverAddress, 0);
		        if (rc2 < 0) {
					printf("\nError resolving Server\n" );
					shutdown();
				} else {
			        // Выделить сокет.
			        // mTCP владеет структурами данных сокета. Пользователь получает указатель на них.
			        // Вызов TcpSocketMgr::getSocket() предоставляет сокет для использования.
			        // Когда работа будет закончена, его нужно вернуть с помощью вызова TcpSocketMgr::freeSocket().
					clientSocket = TcpSocketMgr::getSocket();

			        // Установить размер приёмного буфера.
					clientSocket->setRecvBuffer(RECV_BUFFER_SIZE);

					// Выполнить неблокирующее соединение, ожидать 10 секунд перед тем, как выдать ошибку.
					printf("\nconnect");
					int8_t rc = clientSocket->connect(2048, serverAddress, serverPort, 10000);
					if (rc != 0) {
					    printf("ERROR Socket open failed\n" );
					    shutdown();
					} else {
						//printf("-OK\n");
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
 * DOS
 *
 * Читает клиентом 2 игрока данные из сокета переданные сервером
 * 1 игрока в глобальную переменную buffer (не пересаоздаем его всевремя)
 * а из нее потом все перекладывается в остальные  переменные
 */
void readFromSocketDOS() {
    // Обработка входящих пакетов.
    // PACKET_PROCESS_SINGLE - это макрос, проверяющий наличие новых пакетов от драйвера пакетов.
    // Если таковые обнаружены, макрос обрабатывает эти пакеты.
    PACKET_PROCESS_SINGLE;
    // Arp::driveArp() используется для проверки ожидающих запросов ARP и повторения их при необходимости.
    Arp::driveArp();
    // Tcp::drivePackets() используется для отправки пакетов, которые были поставлены в очередь для отправки.
    Tcp::drivePackets();

    if (clientSocket->isRemoteClosed()) {
        // Сокет был закрыт удалённой стороной.
    	connectionLost = 1;
    } else {
		// очищаем буфер
		bzero(buffer, RECV_BUFFER_SIZE);

		// проверяем можно ли что то прочитать из сокета
        // Если сокет не закрыт, то код попытается получить на него данные.
        // Код возврата 0 указывает на отсутствие данных.
        // Отрицательный код возврата указывает на ошибку сокета, а положительный код возврата - количество полученных байтов.
        int16_t n = 0;
		if (appType == SERVER_APPLICATION) {
			n = clientSocket->recv(buffer, 1);
		} else {
			n = clientSocket->recv(buffer, RECV_BUFFER_SIZE);
		}
		
       	if (n < 0) {
			connectionLost = 1;
		} else if (n > 0 && buffer[0] != 0) {
			// парсим данные пришедшие по сети
			parseBuffer();
		}
    }
}


/**
 * DOS
 *
 * Отправляет на сервер данные с клиента 2-го игрока
 * шлем на сервер только нажатую на клавиатуре кнопку
 * 2м игроком
 */
void writeInSocketDOS() {
    // Обработка входящих пакетов.
    // PACKET_PROCESS_SINGLE - это макрос, проверяющий наличие новых пакетов от драйвера пакетов.
    // Если таковые обнаружены, макрос обрабатывает эти пакеты.
    PACKET_PROCESS_SINGLE;
    // Arp::driveArp() используется для проверки ожидающих запросов ARP и повторения их при необходимости.
    Arp::driveArp();
    // Tcp::drivePackets() используется для отправки пакетов, которые были поставлены в очередь для отправки.
    Tcp::drivePackets();

    if (clientSocket->isRemoteClosed()) {
        // Сокет был закрыт удалённой стороной.
    	connectionLost = 1;
    } else {
		// очищаем буфер
		bzero(buffer, RECV_BUFFER_SIZE);
		
		// создаем пакт данных в buffer для отправки по сети
		createBuffer();
		
		int8_t n;
		if (appType == SERVER_APPLICATION) {
			// записываем buffer в сокет (отправляем 2-му игроку)
			n = clientSocket->send(buffer, RECV_BUFFER_SIZE);
		} else {
			// записываем buffer в сокет (отправляем на Сервер)
			n = clientSocket->send(buffer, 1);
		}
		
		if (n < 0) {
			connectionLost = 1;
		}
    }
}

/**
 * DOS
 *
 * Закрыть сокеты
 */
void closeSocketsDOS() {
	if (appType == CLIENT_APPLICATION || appType == SERVER_APPLICATION) {
	     if (clientSocket != NULL) {
	    	 clientSocket->close();
	    	 TcpSocketMgr::freeSocket(clientSocket);
	     }
	     shutdown();
	}
}

/**
 * DOS
 *
 * Установить разрешение 320 x 200 с 16 цетами 
 */
void setVideoMode_DOS() {
    // 320 x 200 16 colors
    _setvideomode(_MRES16COLOR); 
}

/** 
 * DOS
 *
 * Получить активную видео страницу  
 */
int getGetActivePage_DOS() {
    return _getactivepage();
}

/** 
 * DOS
 *
 * Получить отображаемую видео страницу  
 */
int getVisualPage_DOS() {
    return _getvisualpage();
}

/** 
 * DOS
 *
 * Установить назад видеорежим что был при старте
 */
void setBackStartVideoMode_DOS(int old_apage, int old_vpage) {
    _setactivepage(old_apage);
    _setvisualpage(old_vpage);
    _setvideomode(_DEFAULTMODE);

    printf("\n   Super Turbo Net Pac-Man v1.7 for DOS (IBM PC XT Intel 8088)\n");
}

/**
 * DOS
 *
 * Нарисовать на экране спрайт из буфера
 * x - позиция по горизонтали
 * y - позиция по вертекали
 * n - размер массива по y (количество строк в файле)
 * m - размер массива по x (количество столбцов в файле)
 * buff - массив с данными из текстового файла
 */
void drawSprite(int x, int y, int n, int m, char *buff) {
   int dx = x*8;
   int dy = y*8;
   char txt[2];
   for(int i = 0; i < n; i++) {
	   for (int j=0; j < m; j++) {
		   txt[0] = buff[i * n + j + i]; 
		   txt[1] = '\0';
		   int val = atoi(txt);
		   if (val > 7) {
			   // желтый в файле 8 а на самам деле 14
			   // белый в файле 9 а на самом деле 15
			   val+=6;
		   } 
		   _setcolor(val);
		   _setpixel(dx+j, dy+i);
	   }

   }
}

/**
 * DOS
 * 
 * Нарисовать на экране картинку из файла
 * x - позиция по горизонтали
 * y - позиция по вертекали
 * n - размер массива по y (количество строк в файле)
 * m - размер массива по x (количество столбцов в файле)
 */
void drawImage(int x, int y, const char *fileName, int n, int m) {
	int k=m+1;
	char *b = new char[n*k];
	if (readMapFromFile(fileName, b, n, k)) {
		drawSprite(x, y, n, m, b);
	}
	delete b;
}

/**
 * DOS
 *
 * Прочитать спрайт из текстового файла
 * Нарисовать загруженную картинку
 * fileName - имя файла содержащего картинку в текстовом виде
 * sx - координата x (левый верхний угол спрайта) где будет отрисован спрайт для загрузки затем в память
 * sy - координата y (левый верхний угол спрайта) где будет отрисован спрайт для загрузки затем в память
 * n - количество строк в файле
 * m - количество столбцов в файле
 * return - картика содержащая спрайт из указанного файла
 *
 * функция так же меняет 
 */
char * getImage(int sx, int sy, const char *fileName, int n, int m) {
	int x = sx * 8;
	int y = sy * 8;
	drawImage(sx, sy, fileName, n, m);
	char *im = (char*) malloc(_imagesize(x, y, x+n, y+m));
	_getimage(x, y, x+n-1, y+m-1, im);
	return im;
}

/**
 * DOS
 *
 * Нарисовать только 1 объект с карты
 * i - строка в массиве карты
 * j - столбец в массиве карты
 */
void draw(int i, int j) {
    char val = map[i][j] ;
    int y=i*8;
    int x=j*8;

    if (val == PACMAN) {
        if (pacmanSprite == 1) {
            pacmanSprite = 2;
            if (dx < 0) {
                _putimage(x-2, y-2, images[5], _GPSET);
            } else if (dx > 0) {
                _putimage(x-2, y-2, images[7], _GPSET);
            } else if (dy < 0) {
                _putimage(x-2, y-2, images[9], _GPSET);
            } else if (dy > 0) {
                _putimage(x-2, y-2, images[11], _GPSET);
            } else {
                _putimage(x-2, y-2, images[4], _GPSET);
            }
        } else if (pacmanSprite == 2) {
            pacmanSprite = 3;
            if (dx < 0) {
                _putimage(x-2, y-2, images[6], _GPSET);
            } else if (dx > 0) {
                _putimage(x-2, y-2, images[8], _GPSET);
            } else if (dy < 0) {
                _putimage(x-2, y-2, images[10], _GPSET);
            } else if (dy > 0) {
                _putimage(x-2, y-2, images[12], _GPSET);
            } else {
                _putimage(x-2, y-2, images[4], _GPSET);
            }
        } else if (pacmanSprite == 3) {
            pacmanSprite = 1;
            _putimage(x-2, y-2,  images[4], _GPSET);
        }
    } else if (val == RED) {
        if (redSprite == 1) {
            redSprite = 2;
            if (dxRed < 0) {
            	_putimage(x-2, y-2,images[13], _GPSET);
            } else if (dxRed > 0) {
            	_putimage(x-2, y-2,images[15], _GPSET);
            } else if (dyRed > 0) {
            	_putimage(x-2, y-2,images[17], _GPSET);
            } else {
            	_putimage(x-2, y-2,images[19], _GPSET);
            }
        } else {
            redSprite = 1;
            if (dxRed < 0) {
            	_putimage(x-2, y-2,images[14], _GPSET);
            } else if (dxRed > 0) {
            	_putimage(x-2, y-2,images[16], _GPSET);
            } else if (dyRed > 0) {
            	_putimage(x-2, y-2,images[18], _GPSET);
            } else {
            	_putimage(x-2, y-2,images[20], _GPSET);
            }
        }
    } else if (val == PACGIRL) {
        if (pacGirlSprite == 1) {
        	pacGirlSprite = 2;
            if (dxPacGirl < 0) {
            	_putimage(x-2, y-2, images[25], _GPSET);
            } else if (dxPacGirl > 0) {
            	_putimage(x-2, y-2, images[27], _GPSET);
            } else if (dyPacGirl < 0) {
            	_putimage(x-2, y-2, images[29], _GPSET);
            } else if (dyPacGirl > 0) {
            	_putimage(x-2, y-2, images[31], _GPSET);
            } else {
            	_putimage(x-2, y-2, images[24], _GPSET);
            }
        } else if (pacGirlSprite == 2) {
        	pacGirlSprite = 3;
            if (dxPacGirl < 0) {
            	_putimage(x-2, y-2, images[26], _GPSET);
            } else if (dxPacGirl > 0) {
            	_putimage(x-2, y-2, images[28], _GPSET);
            } else if (dyPacGirl < 0) {
            	_putimage(x-2, y-2, images[30], _GPSET);
            } else if (dyPacGirl > 0) {
            	_putimage(x-2, y-2, images[32], _GPSET);
            } else {
            	_putimage(x-2, y-2, images[24], _GPSET);
            }
        } else if (pacGirlSprite == 3) {
        	pacGirlSprite = 1;
			if (dxPacGirl != 0) {
				_putimage(x-2, y-2,  images[24], _GPSET);
			} else {
				_putimage(x-2, y-2,  images[33], _GPSET);
			}
        }
    } else if (val == SHADOW) {
        if (redSprite == 1) {
            redSprite = 2;
            _putimage(x-2, y-2, images[21], _GPSET);
        } else {
            redSprite = 1;
            _putimage(x-2, y-2, images[22], _GPSET);
        }
    } else if (val == FOOD) {
    	_putimage(x-2, y-2, images[1], _GPSET);
    } else if (val == EMPTY) {
    	_putimage(x-2, y-2, images[2], _GPSET);
    } else if (val == POWER_FOOD) {
    	_putimage(x-2, y-2, images[3], _GPSET);
    } else if (val == CHERRY) {
    	_putimage(x-2, y-2, images[0], _GPSET);
    } else if (val == DOOR) {
    	_putimage(x-2, y-2, images[23], _GPSET);
    }
}

/**
 * DOS
 *
 * Загрузить спрайты и нарисовать всю карту
 */
void drawMap() {
	// массив куда загрузим спрайты стен
	char * walls[WALL_SIZE];
	// ключи по которым ищем загруженую в память стену
	char wkeys[WALL_SIZE];

	// координаты где будет рисоваться спрайт чтоб затем его скопировать в память
	// после этого спрайт будет рисоваться из памяти сразу весь
	int sx = 0;
	int sy = 0;
	
	// S__.TXT содержит текстуры для персонажей и объектов что перересовываются
	// например S10.TXT, S22.TXT
	char t[8] = "S__.TXT";
	// буфер для преобразования строк
	char s[3];
	
	// загружаем изображения объектов чо могут перерисовыватся во время игры
	for (int i=0; i < IMAGES_SIZE; i++) {
		// имена файлов 2х значные и начинаются с S10.TXT
		itoa(i+10, s);
		
		// заменяем '__' на цифры
		t[1]=s[0];
		t[2]=s[1];
		
		// запоминаем избражение, из файла t, размеом 14 на 14
		images[i] = getImage(sx, sy, t, 14, 14);
		
		sx+=2;
		if (sx > MAP_SIZE_X) {
			sx = 0;
			sy+=2;
		}
	}

    // переключаюсь на другую активную страницу (хотя можно просто очистить экран)
	activePage = 1;
	_setactivepage(activePage);
    _setvisualpage(activePage);
	
	
	// заполняем массив ключей нулями
	bzero(wkeys, WALL_SIZE);
	
    // W_.TXT содержат текстуры картинок стен лаберинта 
	// например W0.TXT, WL.TXT
	strcpy(t, "W_.TXT");
    
	// рисуем объекты карты которые не перерисовываются или перерисовыатся редко
	char v;
    for (int i=0, y=0; i<MAP_SIZE_Y; i++, y=i*8) {
        for (int j=0, x=0; j<MAP_SIZE_X-1 ; j++, x=j*8) {
        	v = map[i][j];
			if (v == FOOD) {
				// рисуем на карте еду
            	_putimage(x-2, y-2, images[1], _GPSET);
            } else if (v == POWER_FOOD) {
				// рисуем на карте поверапы
            	_putimage(x-2, y-2, images[3], _GPSET);
            } else if (!isNotWellOrDoor(i, j)) {
				// рисуем на карте стены
				// ищем загружен ли уже спрайт или надо из файла прочитать и отрисовать
				for (int k=0; k < WALL_SIZE; k++) {
					if (wkeys[k] == v) {
						// спрайт загружен, нужно просто отрисовать
						_putimage(x, y, walls[k], _GPSET);
						break; 
					} else if (wkeys[k] == 0) {
						// спрайт не загружен, нужно загрузить из файла и отрисовать
						// запоминаем позицию куда загрузим в память спрайт
						wkeys[k] = v;		
						// заменяем в "W_.TXT" символ '_' на символ прочитанный из карты
						t[1] = v;
						// ресуем избражение из файла t, в позиции j, i размером 8 на 8 и запоминаем в массив walls
						walls[k] = getImage(j, i, t, 8, 8);
						break;
					}
				}
			}
        }
    }

    draw(doorY, doorX);
    draw(cherryY, cherryX);
    draw(redY, redX);
	draw(pacmanY, pacmanX);
	draw(pacGirlY, pacGirlX);
	
	// удаляем из памяти все стены т.к. их повторно не отрисовываем
	for(int i=0; i < WALL_SIZE; i++) {
		free(walls[i]);
	}
}

/** 
 * DOS 
 *
 * Перерисовать карту в графическом режиме
 */
void draw_DOS() {
    _setactivepage(activePage);
    _setvisualpage(activePage);

    drawMap();
}

/**
 * DOS
 *
 * обновить только поменявшиеся объекты на карте
 */
void refresh_DOS() {
    if (refreshDoor) {
        draw(doorY, doorX);
		if (map[doorY][doorX] != DOOR) {
			refreshDoor = 0;
		}
    }
	
    if (refreshCherry) {
        draw(cherryY, cherryX);
		if (map[cherryY][cherryX] != CHERRY) {
				refreshCherry = 0;
		}
    }

    if (oldXRed != redX || oldYRed != redY) {
        draw(oldYRed, oldXRed);
        draw(redY, redX);
    }

    if (oldX != pacmanX || oldY != pacmanY) {
        draw(oldY, oldX);
        draw(pacmanY, pacmanX);
    }

    if (oldPacGirlX != pacGirlX || oldPacGirlY != pacGirlY) {
        draw(oldPacGirlY, oldPacGirlX);
        draw(pacGirlY, pacGirlX);
    }

    // TODO понять где не стираю для этой координаты
    if (map[cherryY-1][cherryX] != EMPTY) {
		map[cherryY-1][cherryX] = EMPTY;
		draw(cherryY-1, cherryX);
    }
}

/**
 * DOS
 *
 * текущее время в милисекундах
 */
long current_timestamp() {
	struct dosdate_t date;
    struct dostime_t time;

    // получить текущую дату
    _dos_getdate(&date);
    // получить текущее время
    _dos_gettime(&time);
    // вычисляем милисекунды
    long milliseconds = date.day * 34560000L + time.hour * 1440000L + time.minute * 60000L + time.second * 1000L + time.hsecond * 10L;

    return milliseconds;
}

/**
 * DOS
 *
 * удалить объекты спрайтов из памяти
 */
void freeImages() {
	for(int i=0; i < IMAGES_SIZE;i++) {
		free(images[i]);
	}
}

/**
 * END DOS END DOS END DOS END DOS END DOS END DOS END DOS END DOS  
 */
#endif

/**
 * Перерисовать Карту (map[][]), PACMAN, Призраков
 */
void showMap() {
#ifdef __linux__
	draw_Linux();
#else
	draw_DOS();
#endif
}

/**
 * Перерисовать только нужные объекты
 */
void refreshMap() {
#ifdef __linux__
	// TODO рисуем все сейчас каждый раз
	draw_Linux();
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
 * выгрузить картинки из памяти
 */
void setBackStartVideoMode(int old_apage, int old_vpage) {
#ifdef __linux__
	printf("\n");
	system("clear");
	printf("\n Super Turbo Net Pac-Man v1.7 for Linux\n");
#else
	// вернуть старый видеорежим
	setBackStartVideoMode_DOS(old_apage, old_vpage);
	// выгрузить из памяти спрайты
	freeImages();
#endif
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

	printf("\nPress ENTER\n");
	getchar();
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
							if (abs(redY + 1 - pacmanY) < abs(redY - 1 - pacmanY)) {
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
#ifdef __linux__
	pacmanServerLinux(port);
#else
	pacmanServerDOS(port);
#endif
}

/**
 * Подключение к серверу клиента 2 игрока
 * host - хост на котором работает сервер
 * port - порт на катором работает сервер
 */
void pacmanClient(char *host, char *port) {
#ifdef __linux__
	pacmanClientLinux(host, port);
#else
	pacmanClientDOS(host, port);
#endif
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
#ifdef __linux__
	writeInSocketLinux();
#else
	writeInSocketDOS();
#endif
}

/**
 * Читаем данные посланные по сети
 * в глобальную переменную buffer (не пересаоздаем его всевремя)
 * - какую кнопку на клавиатуре нажал 2 игрок - для сервера
 * - данные об объектах и очках - для клиента
 */
void readFromSocket() {
#ifdef __linux__
	readFromSocketLinux();
#else
	readFromSocketDOS();
#endif
}

/**
 * Закрыть сокеты
 */
void closeSockets() {
#ifdef __linux__
	closeSocketsLinux();
#else
	closeSocketsDOS();
#endif
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
	case 65: // Linux
	case 72: // DOS
		dyPacGirl = -1;
		dxPacGirl = 0;
		break;
		// key DOWN
	case 66: // Linux
	case 80: // DOS
		dyPacGirl = 1;
		dxPacGirl = 0;
		break;
		// key LEFT
	case 68: // Linux
	case 75: // DOS
		dxPacGirl = -1;
		dyPacGirl = 0;
		break;
		// key RIGHT
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
 */
void player1(int ch) {
	switch (ch) {
	// key UP
	case 65: // Linux
	case 72: // DOS
		dy = -1;
		dx = 0;
		break;
	case 119:
		dyPacGirl = -1;
		dxPacGirl = 0;
		break;
		// key DOWN
	case 66: // Linux
	case 80: // DOS
		dy = 1;
		dx = 0;
		break;
	case 115:
		dyPacGirl = 1;
		dxPacGirl = 0;
		break;
		// key LEFT
	case 68: // Linux
	case 75: // DOS
		dx = -1;
		dy = 0;
		break;
	case 97:
		dxPacGirl = -1;
		dyPacGirl = 0;
		break;
		// key RIGHT
	case 67: // Linux
	case 77: // DOS
		dx = 1;
		dy = 0;
		break;
	case 100:
		dxPacGirl = 1;
		dyPacGirl = 0;
		break;
	}
}

/**
 * Обновить карту / персонажей, двери, черешню
 * для Серверной, Одиночной, Кооперативной игры
 */
void refreshSingleGame() {
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

		refreshMap();
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
 * Обновить карту / персонажей, двери, черешню
 * для игры 2 игроком на клиенте с другого компьютера
 */
void refreshClientGame() {
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
		refreshMap();
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
 *  Основной цикл игры
 *  для игры 2 игроком на клиенте с другого компьютера
 *  в сетевом режиме
 *   	appType == CLIENT_APPLICATION
 */
int gameClientMode() {
	// нажатая клавиша на клавиатуре
	int ch;

	do {
		// читаем данные о карте / персонажах с сервера
		readFromSocket();

		if (connectionLost) {
			// если соединение потеряно, завершаем игру
			return 0;
		}

		// обновляем карту и все элементы какие нужно на экране
		refreshClientGame();

		if (kbhit()) {
			// оределяем какая кнопка нажата
			ch = getch(); // Linux getchar() ;
			if (ch == 0)
				ch = getch(); // only DOS
			player2PressKey = ch;

			writeInSocket();

			if (connectionLost) {
				// если соединение потеряно, завершаем игру
				return 0;
			}
		}
	} while (ch != 'q');
	return 1;
}
/**
 *  Основной цикл игры
 *  для одинойной или кооперативной игры на одном компьютере
 *      appType == SINGLE_APPLICATION
 *  или 1 игроком на сервере в сетевом режиме
 *   	appType == SERVER_APPLICATION
 */
int game() {
	// нажатая клавиша на клавиатуре
	int ch;
	do {
		// обновить карту / персонажей на экране
		refreshSingleGame();

		// проверяем нажата ли кнопка
		if (kbhit()) {

			// оределяем какая кнопка нажата
			ch = getch(); // Linux getchar() ;
			if (ch == 0)
				ch = getch(); // only DOS

			// Обработка нажатых кнопок
			// в оденочной игре или в кооперативной с одного компьютера
			// и на Сервере 1 игроком при сетевой игре
			// в этом режиме можно управлять обоими персонажами
			player1(ch);
		}

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

		// Выход из игры 'q'
	} while (ch != 'q');

	return 1;
}

int main(int argc, char *argv[]) {
	printf("\nSuper Turbo NET Pac-Man v1.7\n");
	if (argc == 2) {
		// запущен как сервер

		printf("Server wait PAC-GIRL!\n");
		pacmanServer(argv[1]);
		if (appType != SERVER_APPLICATION) {
			printf("No Net GAME!\n");
		}

	} else if (argc == 3) {
		printf("Client\n");
		// запущен как клиент
		pacmanClient(argv[1], argv[2]);
		if (appType != CLIENT_APPLICATION) {
			printf("No Net GAME!\n");
		}
	}

	srand(time(NULL));

	// Начальные настройки по карте
	init();

	// запомнить видеорежим
	videoMode();

	// нарисовать карту и персонажей на экране
	showMap();

	if (appType == CLIENT_APPLICATION) {
		gameClientMode();
	} else {
		game();
		// Надо обновить данные на клиенте 2го плеера

		if (appType == SERVER_APPLICATION && !connectionLost) {
			writeInSocket();
		}

	}

	// обновить карту и персонажей на экране
	refreshMap();

	// вернуть видеорежим и вынрузить картинки из памяти для DOS
	setBackStartVideoMode(old_apage, old_vpage);

	// набранные очки 
	printf("\nScore %d\n", score + redBonus + powerBonus + cherryBonus);

	// если съеденны все FOOD и POWER_FOOD - PACMAN выиграл
	if (score >= foodToWIN) {
		printf("\n YOU WINNER :)!\n");
	} else {
		printf("\n YOU LOOSER :(\n");
	}

	// закрыть сокеты
	closeSockets();
	printf(" GAME OVER!\n \nDeveloper: BlodTor\nDisigners: Eva & Lisa\n\n\t2023\n");
	return 0;
}
