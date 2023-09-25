/**
 Super Turbo NET Pac-Man v1.7
 реализация Pac-Man для GNU/Linux под X11 (это НЕ консольная версия, без Xов не будет работать!)
 на основе библиотеки XLib

 Из Linux собирается так:
 > make xpacman
 или
 > g++ -o xpacman xpacman.cpp -lX11

 может соеденятся по сети с версией v1.3 (DOS версия и консольная для GNU/Linux)

 запустить не сетевую игру (хотя можно потом ввести порт или хост порт и играть по сети):
 > ./xpacman
 или как сервер на порту 7777 (порт можно любой)
 > ./xpacman 7777
 или как клиент который соединится с хостом localhost на порт 7777 (хост и порт любые можно)
 > ./xpacman localhost 7777
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

// Linux
#include <sys/time.h>   //gettime
#include <termios.h>
#include <sys/types.h>  // TCP/IP
#include <sys/socket.h> // TCP/IP
#include <netinet/in.h> // TCP/IP
#include <netdb.h>      // TCP/IP

// X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

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
char player2PressKey = EMPTY;

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
 * Количество видов стен загружаемых из файлов как спрайты
 */
#define WALL_SIZE (22)

// серверный socket - ждет подключения клиентов
// использует только сервер
int serverSocket;

// клиентский socket
// запустились как сервер или как клиент 2-го игрока
// всегда в этот сокет пишем или читаем данные
// отправить на сервер / клиент 2-го игрока
// прочитать данные на сервере / клиенте 2-го игрока
int clientSocket;

// массив куда загрузим спрайты больших объектов для X11
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
Pixmap pixmaps[IMAGES_SIZE];

// массив куда загрузим спрайты стен для X11
Pixmap walls[WALL_SIZE];

// цвета для X11
XColor colorRed, colorGreen, colorBlue, colorBlack, colorDarkViolet, colorBrown,
		colorGray, colorYellow, colorWhite;
// максимальное   количество символов для ввода Хоста и порта для X11 окна
#define INPUT_TEXT_LENGHT 30

// данные поля ввода для Хоста и порта сервера
char serverHostPort[INPUT_TEXT_LENGHT];

// Y координата для надписи Server и того что вводим с клавиатуры
#define INPUT_TEXT_Y 215

// Координата X левого верхнего угла окна X11
#define X 0

//Координата Y левого верхнего угла окна X11
#define Y 0

// ширина окна X11
#define WIDTH 400

// высота окна X11
#define HEIGHT 220

// минимальная ширина окна X11
#define WIDTH_MIN 400

// минимальная высота  окна X11
#define HEIGHT_MIN 220

// ширина бардюра окна X11
#define BORDER_WIDTH 5

// Заголовок окна X11
#define TITLE "Super Turbo NET Pac-Man v1.7 for X11"

// Заголовок пиктограммы окна X11
#define ICON_TITLE "xpacman"

// Класс программы X11
#define PRG_CLASS "xpacman"

// Указатель на структуру Display X11
Display *display;

// Номер экрана
int ScreenNumber;

// Графический контекст X11
GC gc;

// Событие от X11
XEvent xEvent;

// Окно X11
Window window;

// выбранный хост или порт если хост не задавался
char param1[INPUT_TEXT_LENGHT];

// выбранный порт
char param2[INPUT_TEXT_LENGHT];

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
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval,
				sizeof(optval));

		bzero((char*) &serverAddress, sizeof(serverAddress));

		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		serverAddress.sin_port = htons(serverPort);

		// пробуем занять порт
		if (bind(serverSocket, (struct sockaddr*) &serverAddress,
				sizeof(serverAddress)) < 0) {
			printf("\nERROR bind port '%s'\n", port);
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
			readFromClientSocket = 1;
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
	readFromClientSocket = 0;
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
 * XLib
 *
 * Нарисовать в Pixmap спрайт из буфера
 * x - позиция по горизонтали
 * y - позиция по вертекали
 * n - размер массива по y (количество строк в файле)
 * m - размер массива по x (количество столбцов в файле)
 * buff - массив с данными из текстового файла
 * pixmap - спрайт рисуем в пиксельную мапу (спрайт потом будем рисовать в окне из этой мапы копированием)
 */
void drawSprite(int n, int m, char *buff, Pixmap pixmap) {
	char txt[2];
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			txt[0] = buff[i * n + j + i];
			txt[1] = '\0';
			int val = atoi(txt);

			switch (val) {
			case 0: // черный цвет
				XSetForeground(display, gc, colorBlack.pixel);
				break;
			case 1: // синий цвет
				XSetForeground(display, gc, colorBlue.pixel);
				break;
			case 2: // зеленый цвет
				XSetForeground(display, gc, colorGreen.pixel);
				break;
			case 4: // красный цвет
				XSetForeground(display, gc, colorRed.pixel);
				break;
			case 5: // пурпурный цвет
				XSetForeground(display, gc, colorDarkViolet.pixel);
				break;
			case 6: // коричневый цвет
				XSetForeground(display, gc, colorBrown.pixel);
				break;
			case 7: // серый цвет
				XSetForeground(display, gc, colorGray.pixel);
				break;
			case 8: // желтый цвет
				XSetForeground(display, gc, colorYellow.pixel);
				break;
			case 9: // белый цвет
				XSetForeground(display, gc, colorWhite.pixel);
				break;
			default:
				XSetForeground(display, gc, colorBlack.pixel);
			}

			// рисуем точку в пикселоной карте
			XDrawPoint(display, pixmap, gc, j, i);
		}

	}
}

/**
 * XLib
 *
 * Нарисовать в Pixmap картинку из файла
 * x - позиция по горизонтали
 * y - позиция по вертекали
 * fileName - файл с данными картинки в текстовом виде
 * n - размер массива по y (количество строк в файле)
 * m - размер массива по x (количество столбцов в файле)
 *
 * pixmap - сюда сохраняем картинку прочитанную из файла в байтовый массив
 */
void getImage(const char *fileName, int n, int m, Pixmap pixmap) {
	int k = m + 1;
	char *b = new char[n * k];
	if (readMapFromFile(fileName, b, n, k)) {
		drawSprite(n, m, b, pixmap);
	}
	delete b;
}

/**
 * XLib
 *
 * Нарисовать только 1 объект с карты
 * i - строка в массиве карты
 * j - столбец в массиве карты
 */
void draw(int i, int j) {
	char val = map[i][j];
	int y = i * 8;
	int x = j * 8;

	if (val == PACMAN) {
		if (pacmanSprite == 1) {
			pacmanSprite = 2;
			if (dx < 0) {
				XCopyArea(display, pixmaps[5], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dx > 0) {
				XCopyArea(display, pixmaps[7], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dy < 0) {
				XCopyArea(display, pixmaps[9], window, gc, 0, 0, 14, 14, x - 2,	y - 2);
			} else if (dy > 0) {
				XCopyArea(display, pixmaps[11], window, gc, 0, 0, 14, 14, x - 2,y - 2);
			} else {
				XCopyArea(display, pixmaps[4], window, gc, 0, 0, 14, 14, x - 2,	y - 2);
			}
		} else if (pacmanSprite == 2) {
			pacmanSprite = 3;
			if (dx < 0) {
				XCopyArea(display, pixmaps[6], window, gc, 0, 0, 14, 14, x - 2,	y - 2);
			} else if (dx > 0) {
				XCopyArea(display, pixmaps[8], window, gc, 0, 0, 14, 14, x - 2,	y - 2);
			} else if (dy < 0) {
				XCopyArea(display, pixmaps[10], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dy > 0) {
				XCopyArea(display, pixmaps[12], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else {
				XCopyArea(display, pixmaps[4], window, gc, 0, 0, 14, 14, x - 2,	y - 2);
			}
		} else if (pacmanSprite == 3) {
			pacmanSprite = 1;
			XCopyArea(display, pixmaps[4], window, gc, 0, 0, 14, 14, x - 2, y - 2);
		}
	} else if (val == RED) {
		if (redSprite == 1) {
			redSprite = 2;
			if (dxRed < 0) {
				XCopyArea(display, pixmaps[13], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dxRed > 0) {
				XCopyArea(display, pixmaps[15], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dyRed > 0) {
				XCopyArea(display, pixmaps[17], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else {
				XCopyArea(display, pixmaps[19], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			}
		} else {
			redSprite = 1;
			if (dxRed < 0) {
				XCopyArea(display, pixmaps[14], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dxRed > 0) {
				XCopyArea(display, pixmaps[16], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dyRed > 0) {
				XCopyArea(display, pixmaps[18], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else {
				XCopyArea(display, pixmaps[20], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			}
		}
	} else if (val == PACGIRL) {
		if (pacGirlSprite == 1) {
			pacGirlSprite = 2;
			if (dxPacGirl < 0) {
				XCopyArea(display, pixmaps[25], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dxPacGirl > 0) {
				XCopyArea(display, pixmaps[27], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dyPacGirl < 0) {
				XCopyArea(display, pixmaps[29], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dyPacGirl > 0) {
				XCopyArea(display, pixmaps[31], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else {
				XCopyArea(display, pixmaps[24], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			}
		} else if (pacGirlSprite == 2) {
			pacGirlSprite = 3;
			if (dxPacGirl < 0) {
				XCopyArea(display, pixmaps[26], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dxPacGirl > 0) {
				XCopyArea(display, pixmaps[28], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dyPacGirl < 0) {
				XCopyArea(display, pixmaps[30], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (dyPacGirl > 0) {
				XCopyArea(display, pixmaps[32], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else {
				XCopyArea(display, pixmaps[24], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			}
		} else if (pacGirlSprite == 3) {
			pacGirlSprite = 1;
			if (dxPacGirl != 0) {
				XCopyArea(display, pixmaps[24], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else {
				XCopyArea(display, pixmaps[33], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			}
		}
	} else if (val == SHADOW) {
		if (redSprite == 1) {
			redSprite = 2;
			XCopyArea(display, pixmaps[21], window, gc, 0, 0, 14, 14, x - 2, y - 2);
		} else {
			redSprite = 1;
			XCopyArea(display, pixmaps[22], window, gc, 0, 0, 14, 14, x - 2, y - 2);
		}
	} else if (val == FOOD) {
		XCopyArea(display, pixmaps[1], window, gc, 0, 0, 14, 14, x - 2, y - 2);
	} else if (val == EMPTY) {
		XCopyArea(display, pixmaps[2], window, gc, 0, 0, 14, 14, x - 2, y - 2);
	} else if (val == POWER_FOOD) {
		XCopyArea(display, pixmaps[3], window, gc, 0, 0, 14, 14, x - 2, y - 2);
	} else if (val == CHERRY) {
		XCopyArea(display, pixmaps[0], window, gc, 0, 0, 14, 14, x - 2, y - 2);
	} else if (val == DOOR) {
		XCopyArea(display, pixmaps[23], window, gc, 0, 0, 14, 14, x - 2, y - 2);
	}
}

/**
 * XLib
 *
 * удалить объекты спрайтов из памяти
 */
void freePixmaps() {
	for (int i = 0; i < IMAGES_SIZE; i++) {
		XFreePixmap(display, pixmaps[i]);
	}
}

// Linux !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/**
 * Перерисовать Карту (map[][]), PACMAN, Призраков
 * XLib
 */
void showMap() {
	// создаем Графический контекст
	gc = XCreateGC(display, window, 0, NULL);

	// эта переменная будет содержать глубину цвета создаваемой
	// пиксельной карты - количество бит, используемых для
	// представления индекса цвета в палитре (количество цветов
	// равно степени двойки глубины)
	int depth = DefaultDepth(display, DefaultScreen(display));

	// ключи по которым ищем загруженую в память стену
	char wkeys[WALL_SIZE];

	// S__.TXT содержит текстуры для персонажей и объектов что перересовываются
	// например S10.TXT, S22.TXT
	char t[8] = "S__.TXT";
	// буфер для преобразования строк
	char s[3];

	// загружаем изображения объектов чо могут перерисовыватся во время игры
	for (int i = 0; i < IMAGES_SIZE; i++) {
		// имена файлов 2х значные и начинаются с S10.TXT
		itoa(i + 10, s);

		// заменяем '__' на цифры
		t[1] = s[0];
		t[2] = s[1];

		// создаем новую пиксельную карту шириной 30 и высотой в 40 пикселей
		pixmaps[i] = XCreatePixmap(display, window, 14, 14, depth);

		// запоминаем избражение, из файла t, размеом 14 на 14
		getImage(t, 14, 14, pixmaps[i]);
	}

	// заполняем массив ключей нулями
	bzero(wkeys, WALL_SIZE);

	// W_.TXT содержат текстуры картинок стен лаберинта
	// например W0.TXT, WL.TXT
	strcpy(t, "W_.TXT");

	// рисуем объекты карты которые не перерисовываются или перерисовыатся редко

	char v;
	for (int i = 0, y = 0; i < MAP_SIZE_Y; i++, y = i * 8) {
		for (int j = 0, x = 0; j < MAP_SIZE_X - 1; j++, x = j * 8) {
			v = map[i][j];
			if (v == DOOR) {
				//  ниче не делаем
			} else if (v == FOOD) {
				//  рисуем на карте еду
				XCopyArea(display, pixmaps[1], window, gc, 0, 0, 14, 14, x - 2, y - 2);
			} else if (v == POWER_FOOD) {
				// рисуем на карте поверапы
				XCopyArea(display, pixmaps[3], window, gc, 0, 0, 14, 14, x - 2,	y - 2);
			} else if (!isNotWellOrDoor(i, j)) {
				// рисуем на карте стены
				// ищем загружен ли уже спрайт или надо из файла прочитать и отрисовать
				for (int k = 0; k < WALL_SIZE; k++) {

					if (wkeys[k] == v) {
						// спрайт загружен, нужно просто отрисовать
						XCopyArea(display, walls[k], window, gc, 0, 0, 8, 8, x,	y);
						break;
					} else if (wkeys[k] == 0) {
						// спрайт не загружен, нужно загрузить из файла и отрисовать
						// запоминаем позицию куда загрузим в память спрайт
						wkeys[k] = v;
						// заменяем в "W_.TXT" символ '_' на символ прочитанный из карты
						t[1] = v;

						// ресуем избражение из файла t, в позиции j, i размером 8 на 8 и запоминаем в массив walls
						walls[k] = XCreatePixmap(display, window, 8, 8, depth);
						getImage(t, 8, 8, walls[k]);
						XCopyArea(display, walls[k], window, gc, 0, 0, 8, 8, x,	y);
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

	XFreeGC(display, gc);
	XFlush(display);

	// удаляем из памяти все стены т.к. их повторно не отрисовываем
	for (int i = 0; i < WALL_SIZE; i++) {
		XFreePixmap(display, walls[i]);
	}
}

/**
 * Перерисовать только нужные объекты
 * XLib
 */
void refreshMap() {
	// создаем Графический контекст
	gc = XCreateGC(display, window, 0, NULL);

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
	if (map[cherryY - 1][cherryX] != EMPTY) {
		map[cherryY - 1][cherryX] = EMPTY;
		draw(cherryY - 1, cherryX);
	}

	char result[5];
	char txt[20];

	XSetForeground(display, gc, BlackPixel(display, 0));
	XFillRectangle(display, window, gc, 300, 20, WIDTH, HEIGHT - 40);

	// выводим инфу о съеденных вишнях и бонусе
	bzero(result, 5);
	bzero(txt, 20);

	itoa(cherryBonus / SCORE_CHERY_BONUS, result);
	strcat(txt, "x ");
	strcat(txt, result);
	strcat(txt, " = ");
	itoa(cherryBonus, result);
	strcat(txt, result);

	XCopyArea(display, pixmaps[0], window, gc, 0, 0, 14, 14, 270, 50);

	XSetForeground(display, gc, WhitePixel(display, 0));
	XDrawString(display, window, gc, 290, 60, txt, strlen(txt));

	// выводим инфу о съеденных призраках
	bzero(result, 5);
	bzero(txt, 20);
	itoa(redBonus / SCORE_RED_EAT, result);
	strcat(txt, "x ");
	strcat(txt, result);
	strcat(txt, " = ");
	itoa(redBonus, result);
	strcat(txt, result);

	XCopyArea(display, pixmaps[21], window, gc, 0, 0, 14, 14, 270, 70);

	XSetForeground(display, gc, WhitePixel(display, 0));
	XDrawString(display, window, gc, 290, 80, txt, strlen(txt));

	// выводим инфу о съеденных поверапах
	bzero(result, 5);
	bzero(txt, 20);
	itoa(powerBonus / SCORE_POWER_BONUS, result);
	strcat(txt, "x ");
	strcat(txt, result);
	strcat(txt, " = ");
	itoa(powerBonus + powerBonus / SCORE_POWER_BONUS, result);
	strcat(txt, result);

	XCopyArea(display, pixmaps[3], window, gc, 0, 0, 14, 14, 270, 90);

	XSetForeground(display, gc, WhitePixel(display, 0));
	XDrawString(display, window, gc, 290, 100, txt, strlen(txt));

	// сколько съедено точек
	bzero(result, 5);
	bzero(txt, 20);
	itoa(score - powerBonus / SCORE_POWER_BONUS, result);
	strcat(txt, "x ");
	strcat(txt, result);
	strcat(txt, " = ");
	strcat(txt, result);

	bzero(result, 5);
	itoa(foodToWIN - 4, result);
	strcat(txt, " / ");
	strcat(txt, result);

	XCopyArea(display, pixmaps[1], window, gc, 0, 0, 14, 14, 270, 110);

	XSetForeground(display, gc, WhitePixel(display, 0));
	XDrawString(display, window, gc, 290, 120, txt, strlen(txt));

	// итого
	bzero(result, 5);
	bzero(txt, 20);
	itoa(score + redBonus + powerBonus + cherryBonus, result);
	strcat(txt, "Score: ");
	strcat(txt, result);

	XSetForeground(display, gc, WhitePixel(display, 0));

	XDrawString(display, window, gc, 300, 150, txt, strlen(txt));

	// устанавливаем белый цвет - которым будем рисовать
	XSetForeground(display, gc, WhitePixel(display, 0));
	XFillRectangle(display, window, gc, WIDTH - 100, HEIGHT - 20, WIDTH, HEIGHT);

	XSetForeground(display, gc, BlackPixel(display, 0));

	if (appType == CLIENT_APPLICATION) {
		XDrawString(display, window, gc, 300, INPUT_TEXT_Y, "Client PAC-GIRL", strlen("Client PAC-GIRL"));
	} else if (appType == SERVER_APPLICATION) {
		XDrawString(display, window, gc, 300, INPUT_TEXT_Y, "Server PAC-MAN", strlen("Server PAC-MAN"));
	} else {
		XDrawString(display, window, gc, 300, INPUT_TEXT_Y, "Press 'q' or ESC", strlen("Press 'q' or ESC"));
	}

	XFreeGC(display, gc);
	XFlush(display);
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
 *  Запомнить видеорежим
 */
void videoMode() {
	//  ничего не делаем, пока в консоли работает
}

/**
 * Отоброзить результат игры
 * выгрузить картинки из памяти
 *
 *  XLib
 */
void showGameResult() {
	// создаем Графический контекст
	gc = XCreateGC(display, window, 0, NULL);

	// устанавливаем белый цвет - которым будем рисовать
	XSetForeground(display, gc, WhitePixel(display, 0));
	XFillRectangle(display, window, gc, WIDTH - 100, HEIGHT - 20, WIDTH, HEIGHT);

	// устанавливаем белый цвет - которым будем рисовать
	XSetForeground(display, gc, BlackPixel(display, 0));
	XDrawString(display, window, gc, 300, INPUT_TEXT_Y, "Press ENTER", strlen("Press ENTER"));

	if (score >= foodToWIN) {
		XSetForeground(display, gc, colorGreen.pixel);
		XDrawString(display, window, gc, 290, 20, "YOU WINNER :)", strlen("YOU WINNER :)"));
	} else {
		XSetForeground(display, gc, colorRed.pixel);
		XDrawString(display, window, gc, 290, 20, "YOU LOOSER :(", strlen("YOU LOOSER :("));
	}

	XFreeGC(display, gc);
	XFlush(display);
	freePixmaps();
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
	pacmanServerLinux(port);
}

/**
 * Подключение к серверу клиента 2 игрока
 * host - хост на котором работает сервер
 * port - порт на катором работает сервер
 */
void pacmanClient(char *host, char *port) {
	pacmanClientLinux(host, port);
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
	writeInSocketLinux();
}

/**
 * Читаем данные посланные по сети
 * в глобальную переменную buffer (не пересаоздаем его всевремя)
 * - какую кнопку на клавиатуре нажал 2 игрок - для сервера
 * - данные об объектах и очках - для клиента
 */
void readFromSocket() {
	readFromSocketLinux();
}

/**
 * Закрыть сокеты
 */
void closeSockets() {
	closeSocketsLinux();
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
 * Обновить карту / персонажей, двери, черешню
 * для Серверной, Одиночной, Кооперативной игры
 */
void refreshSingleGame() {
	long t1 = current_timestamp();
	if (!cherryFlag && redFlag && !cherryBonus
			&& (t1 - startTime > CHERRY_TIME)) {
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
	if (redFlag == 0 && (t1 - redTime > RED_TIME)) {
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
	// событие закрытия окна
	Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &wmDeleteMessage, 1);

	// a dealie-bob to handle KeyPress Events
	KeySym key;

	// буфер символов для KeyPress Events
	char text[255];

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

		if (XPending(display) > 0) {
			// получить очередное событие от X11
			XNextEvent(display, &xEvent);

			switch (xEvent.type) {
			case Expose:
				// Запрос на перерисовку
				if (xEvent.xexpose.count != 0) {
					break;
				}

				refreshMap();
				break;

			case KeyPress:
				/* Выход нажатием клавиши клавиатуры */
				if (xEvent.xkey.keycode == 9) {
					// нажат ESC
					return -1;
				}

				if (XLookupString(&xEvent.xkey, text, 255, &key, 0) == 1) {
					//gc = XCreateGC(display, window, 0, NULL);
					ch = text[0];
				}
				// Обработка нажатых кнопок
				// в оденочной игре или в кооперативной с одного компьютера
				// и на Сервере 1 игроком при сетевой игре
				// в этом режиме можно управлять обоими персонажами
				// нужно перекодировать нажатые клавиши для совместимости с версией 1.3
				switch (xEvent.xkey.keycode) {
				// key UP
				case 111:
					ch = 65;
					break;
					// key DOWN
				case 116:
					ch = 66;
					break;
					// key LEFT
				case 113:
					ch = 68;
					break;
					// key RIGHT
				case 114:
					ch = 67;
					break;
				}

				player2PressKey = ch;

				writeInSocket();

				if (connectionLost) {
					// если соединение потеряно, завершаем игру
					return 0;
				}

				break;

			case ClientMessage:
				// Нажата кнопка "Закрыть" или Alt + F4
				if (xEvent.xclient.data.l[0] == wmDeleteMessage) {
					return -1;
				}
				break;
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

	// событие закрытия окна
	Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &wmDeleteMessage, 1);

	// a dealie-bob to handle KeyPress Events
	KeySym key;

	// буфер символов для KeyPress Events
	char text[255];

	// нажатая клавиша на клавиатуре
	int ch;
	do {
		// обновить карту / персонажей на экране
		refreshSingleGame();

		if (XPending(display) > 0) {
			// получить очередное событие от X11
			XNextEvent(display, &xEvent);

			switch (xEvent.type) {
			case Expose:
				// Запрос на перерисовку
				if (xEvent.xexpose.count != 0) {
					break;
				}

				refreshMap();
				break;

			case KeyPress:
				/* Выход нажатием клавиши клавиатуры */
				if (xEvent.xkey.keycode == 9) {
					// нажат ESC
					return -1;
				}
				if (XLookupString(&xEvent.xkey, text, 255, &key, 0) == 1) {
					//gc = XCreateGC(display, window, 0, NULL);
					ch = text[0];
				}
				// Обработка нажатых кнопок
				// в оденочной игре или в кооперативной с одного компьютера
				// и на Сервере 1 игроком при сетевой игре
				// в этом режиме можно управлять обоими персонажами
				player1(xEvent.xkey.keycode);
				break;

			case ClientMessage:
				// Нажата кнопка "Закрыть" или Alt + F4
				if (xEvent.xclient.data.l[0] == wmDeleteMessage) {
					return -1;
				}
				break;
			}
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

/**
 * Ждем подключения 2 го игрока к серверу
 * он должен прислать чтонибуть
 * после создания соединения и отрисовки карты
 * нужно для медленных компов
 */
int waitPlayer2() {

	XDrawString(display, window, gc, 300, INPUT_TEXT_Y, "Wait P2 action", strlen("Wait P2 action"));

	if (appType == SERVER_APPLICATION) {
		// событие закрытия окна
		Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(display, window, &wmDeleteMessage, 1);

		// a dealie-bob to handle KeyPress Events
		KeySym key;

		// буфер символов для KeyPress Events
		char text[255];

		// нажатая клавиша на клавиатуре
		int ch;
		do {
			if (XPending(display) > 0) {
				// получить очередное событие от X11
				XNextEvent(display, &xEvent);

				switch (xEvent.type) {
				case Expose:
					// Запрос на перерисовку
					if (xEvent.xexpose.count != 0) {
						break;
					}

					break;
				case KeyPress:
					/* Выход нажатием клавиши клавиатуры */
					if (xEvent.xkey.keycode == 9) {
						// нажат ESC
						return -1;
					}
					if (XLookupString(&xEvent.xkey, text, 255, &key, 0) == 1) {
						//gc = XCreateGC(display, window, 0, NULL);
						ch = text[0];
					}
					break;

				case ClientMessage:
					// Нажата кнопка "Закрыть" или Alt + F4
					if (xEvent.xclient.data.l[0] == wmDeleteMessage) {
						return -1;
					}
					break;
				}
			}

			if (readFromClientSocket) {
				break;
			}


			if (connectionLost) {
				return 0;
			}

			readFromSocket();

			if (readFromClientSocket) {
				// устанавливаем белый цвет - которым будем рисовать
				XSetForeground(display, gc, WhitePixel(display, 0));
				XFillRectangle(display, window, gc, WIDTH - 100, HEIGHT - 20, WIDTH, HEIGHT);
			}

			// Выход из игры 'q'
		} while (ch != 'q');
	}

	return 1;
}

/*
 * SetWindowManagerHints - функция, которая передает информацию о
 * свойствах программы менеджеру окон.
 *
 * XLib
 *
 * display - указатель на структуру Display
 * PClass - класс программы
 * argc - число аргументов
 * window - идентификатор окна
 * x - координата x левого верхнего угла окна
 * y - координата y левого верхнего угла окна
 * win_wdt - ширина  окна
 * win_hgt - высота окна
 * win_wdt_min - минимальная ширина окна
 * int win_hgt_min - Минимальная высота окна
 * ptrTitle - заголовок окна
 * ptrITitle - заголовок пиктограммы окна
 * pixmap - рисунок пиктограммы
 */
static void SetWindowManagerHints(Display *display, char *PClass, char *argv[], int argc, Window window, int x, int y, int win_wdt, int win_hgt, int win_wdt_min, int win_hgt_min, char *ptrTitle, char *ptrITitle, Pixmap pixmap) {
	XSizeHints size_hints; /*Рекомендации о размерах окна*/

	XWMHints wm_hints;
	XClassHint class_hint;
	XTextProperty windowname, iconname;

	if (!XStringListToTextProperty(&ptrTitle, 1, &windowname)
			|| !XStringListToTextProperty(&ptrITitle, 1, &iconname)) {
		puts("No memory!\n");
		_exit(1);
	}

	size_hints.flags = PPosition | PSize | PMinSize | PMaxSize;
	size_hints.min_width = win_wdt_min;
	size_hints.min_height = win_hgt_min;
	size_hints.max_height = win_hgt;
	size_hints.max_width = win_wdt;
	wm_hints.flags = StateHint | IconPixmapHint | InputHint;
	wm_hints.initial_state = NormalState;
	wm_hints.input = True;
	wm_hints.icon_pixmap = pixmap;
	class_hint.res_name = argv[0];
	class_hint.res_class = PClass;

	XSetWMProperties(display, window, &windowname, &iconname, argv, argc,
			&size_hints, &wm_hints, &class_hint);
}

/**
 *  Создание окна X window
 *  argc - количество переданных параметров
 *  argv - переданные параметры
 *
 *  return 1 - окно создано
 *  return 0 - не удалось создать соединение с X  cthdthjv
 */
int xinit(int argc, char *argv[]) {
	// Устанавливаем связь с X11 сервером
	if ((display = XOpenDisplay(NULL)) == NULL) {
		// не удалось создать соединение с X11 сервером
		return 0;
	}

	char title[37];
	strcpy(title, TITLE);

	char iconTitle[8];
	strcpy(iconTitle, ICON_TITLE);

	char prgClass[8];
	strcpy(prgClass, PRG_CLASS);

	/* Получаем номер основного экрана */
	ScreenNumber = DefaultScreen(display);

	/* Создаем окно */
	window = XCreateSimpleWindow(display, RootWindow(display, ScreenNumber),
	X, Y, WIDTH, HEIGHT, BORDER_WIDTH, BlackPixel(display, ScreenNumber),
			WhitePixel(display, ScreenNumber));

	/* Задаем рекомендации для менеджера окон */
	SetWindowManagerHints(display, prgClass, argv, argc, window, X, Y,
	WIDTH, HEIGHT, WIDTH_MIN, HEIGHT_MIN, title, iconTitle, 0);

	/* Выбираем события,  которые будет обрабатывать программа */
	XSelectInput(display, window, ExposureMask | KeyPressMask);

	/* Покажем окно */
	XMapWindow(display, window);

	// цветовая карта
	Colormap colormap = DefaultColormap(display, DefaultScreen(display));

	XParseColor(display, colormap, "#000000", &colorBlack);
	XAllocColor(display, colormap, &colorBlack);

	XParseColor(display, colormap, "#00FF00", &colorGreen);
	XAllocColor(display, colormap, &colorGreen);

	XParseColor(display, colormap, "#FF0000", &colorRed);
	XAllocColor(display, colormap, &colorRed);

	XParseColor(display, colormap, "#0000FF", &colorBlue);
	XAllocColor(display, colormap, &colorBlue);

	XParseColor(display, colormap, "#9400D3", &colorDarkViolet);
	XAllocColor(display, colormap, &colorDarkViolet);

	XParseColor(display, colormap, "#A52A2A", &colorBrown);
	XAllocColor(display, colormap, &colorBrown);

	XParseColor(display, colormap, "#BEBEBE", &colorGray);
	XAllocColor(display, colormap, &colorGray);

	XParseColor(display, colormap, "#FFFF00", &colorYellow);
	XAllocColor(display, colormap, &colorYellow);

	XParseColor(display, colormap, "#FFFFFF", &colorWhite);
	XAllocColor(display, colormap, &colorWhite);

	return 1;
}

/**
 * Ввод и редактирование хоста и порта для соединения с игровмм сервером
 * или поднятие своего сервера
 *
 *  inputHost - хост переданный программе как параметр или от пред идущего вызова
 *  inputPort - порт переданный программе как параметр или от пред идущего вызова
 *
 *  return - 0 пользователь хочет выйти из программы
 *  return - 1 нужно запустить игру
 */
int inputHostPort() {
	// событие закрытия окна
	Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &wmDeleteMessage, 1);

	// a dealie-bob to handle KeyPress Events
	KeySym key;

	// буфер символов для KeyPress Events
	char text[255];

	// аттрибуты окна
	XWindowAttributes window_attr;

	bzero(serverHostPort, INPUT_TEXT_LENGHT);

	if (strlen(param1) > 0&& strlen(param2) > 0 && (strlen(param1) + strlen(param2)) < INPUT_TEXT_LENGHT) {
		strcpy(serverHostPort, param1);
		strcat(serverHostPort, " ");
		strcat(serverHostPort, param2);
	} else if (strlen(param1) > 0 && strlen(param2) < INPUT_TEXT_LENGHT - 1) {
		strcpy(serverHostPort, param1);
	}

	// количество уже введенных символов
	int word = strlen(serverHostPort);

	/* Создадим цикл получения и обработки ошибок */
	while (1) {
		// получить очередное событие от X11
		XNextEvent(display, &xEvent);

		switch (xEvent.type) {
		case Expose:
			// Запрос на перерисовку
			if (xEvent.xexpose.count != 0) {
				break;
			}

			// создаем Графический контекст
			gc = XCreateGC(display, window, 0, NULL);

			// устанавливаем белый цвет - которым будем рисовать
			XSetForeground(display, gc, BlackPixel(display, 0));

			// выводим '"Server:'
			XDrawString(display, window, gc, 10, INPUT_TEXT_Y, "Host+port or port:", strlen("Host+port or port:"));

			// выводим введенный хост и порт сервера
			XDrawString(display, window, gc, 120, INPUT_TEXT_Y, serverHostPort, strlen(serverHostPort));

			XDrawString(display, window, gc, 300, INPUT_TEXT_Y, "Press ENTER", strlen("Press ENTER"));

			XFreeGC(display, gc);
			XFlush(display);
			break;

		case KeyPress:
			/* Выход нажатием клавиши клавиатуры */
			if (XLookupString(&xEvent.xkey, text, 255, &key, 0) == 1) {
				gc = XCreateGC(display, window, 0, NULL);
				if (xEvent.xkey.keycode == 9) {
					// нажат ESC
					XFreeGC(display, gc);

					return 0;
				} else if (xEvent.xkey.keycode == 36) {
					// нажат ENTER

					// устанавливаем белый цвет - которым будем рисовать
					XSetForeground(display, gc, BlackPixel(display, 0));
					// стираем все что было введино
					XFillRectangle(display, window, gc, 0, 0, WIDTH, HEIGHT - 20);

					// устанавливаем белый цвет - которым будем рисовать
					XSetForeground(display, gc, WhitePixel(display, 0));
					XFillRectangle(display, window, gc, WIDTH - 100, HEIGHT - 20, WIDTH, HEIGHT);

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

					// устанавливаем белый цвет - которым будем рисовать
					XSetForeground(display, gc, BlackPixel(display, 0));
					if (strlen(param1) > 0 && strlen(param2) > 0) {
						XDrawString(display, window, gc, 300, INPUT_TEXT_Y, "Connecting ...", strlen("Connecting ..."));
					} else if (strlen(param1) > 0) {
						XDrawString(display, window, gc, 300, INPUT_TEXT_Y, "Server wait ...", strlen("Server wait ..."));
					} else {
						XDrawString(display, window, gc, 300, INPUT_TEXT_Y, "Press 'q' or ESC", strlen("Press 'q' or ESC"));
					}

					XFreeGC(display, gc);
					XFlush(display);
					return 1;
				} else if (xEvent.xkey.keycode == 22) {
					// Нажат BACKSPACE
					if (word > 0) {
						// уменьшаем количество введенных символо
						word--;

						// стираем последний символ
						serverHostPort[word] = 0;

						// устанавливаем белый цвет - которым будем рисовать
						XSetForeground(display, gc, WhitePixel(display, 0));

						// стираем все что было введино
						XFillRectangle(display, window, gc, 120, INPUT_TEXT_Y - 10, 180, 12);
					}
				} else {
					// нажато что то еще
					if (word < INPUT_TEXT_LENGHT - 1) {
						// добавляем к уже введенному еще символ
						serverHostPort[word] = text[0];

						// увеличиваем количество введенных символов
						word++;
					}
				}

				// устанавливаем черный цвет - которым будем рисовать
				XSetForeground(display, gc, BlackPixel(display, 0));

				// выводим введенное в строке Server
				XDrawString(display, window, gc, 120, INPUT_TEXT_Y, serverHostPort, strlen(serverHostPort));

				// освобождаем Графический контекст
				XFreeGC(display, gc);

				// отрисовываем все
				XFlush(display);

			}

			break;

		case ClientMessage:
			// Нажата кнопка "Закрыть" или Alt + F4
			if (xEvent.xclient.data.l[0] == wmDeleteMessage) {
				return 0;
			}
		}
	}

	return 0;
}

/** основная функция программы
 *  argc - количество переданных параметров
 *  argv - переданные параметры
 */
int main(int argc, char *argv[]) {
	// результат игры
	int result = 0;
	// обнуляем массивы куда засуну параметры переданные из консоли
	bzero(param1, INPUT_TEXT_LENGHT);
	bzero(param2, INPUT_TEXT_LENGHT);

	if (argc == 2) {
		// порт был передан как аргумент вызова программы
		strcpy(param1, argv[1]);
	} else if (argc == 3) {
		// хост был передан как аргумент вызова программы
		strcpy(param1, argv[1]);
		// порт был передан как аргумент вызова программы
		strcpy(param2, argv[2]);
	}

	srand(time(NULL));

	// Устанавливаем связь с X11 сервером
	if (xinit(argc, argv)) {
		while (inputHostPort()) {

			gc = XCreateGC(display, window, 0, NULL);
			// устанавливаем белый цвет - которым будем рисовать
			XSetForeground(display, gc, WhitePixel(display, 0));
			XFillRectangle(display, window, gc, WIDTH - 100, HEIGHT - 20, WIDTH, HEIGHT);

			XSetForeground(display, gc, BlackPixel(display, 0));
			if (strlen(param1) > 0 && strlen(param2) > 0) {
				//printf("Client\n");
				// запущен как клиент
				pacmanClient(param1, param2);
			} else if (strlen(param1) > 0) {
				// запущен как сервер
				pacmanServer(param1);

				// ждем подключения 2-го игрока и пока он отрисует у себя карту
				waitPlayer2();
			} else {
				appType = SINGLE_APPLICATION;
			}

			XFreeGC(display, gc);
			XFlush(display);

			// Начальные настройки по карте
			init();

			// нарисовать карту и персонажей на экране
			showMap();

			// игра
			if (appType == CLIENT_APPLICATION) {
				// сообщаем серверу что можно начинать игру передав ' '
				// сервер ждет сообщения от клиента чтоб начать игру когда 2 игрок готов
				// нужно для медленных компов долго грузящих карту
				writeInSocket();

				// 2 игрок за PAC-GIRL
				result = gameClientMode();
			} else {
				// 1 игрок за PAC-MAN
				result = game();

				// Надо обновить данные на клиенте 2го плеера
				if (appType == SERVER_APPLICATION && !connectionLost) {
					writeInSocket();
				}

			}

			// обновить карту и персонажей на экране
			refreshMap();

			// отоброзить результат игры, выгрузить картинки из памяти
			showGameResult();

			// закрыть сокеты
			closeSockets();

			if (result == -1) {
				break;
			}
		}

		// закрываем Display X11
		XCloseDisplay(display);

	} else {
		puts("Can not connect to the X server!\n");
	}
	return 0;
}
