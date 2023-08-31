# Для сборки из под Linux консольной версии игры для GNU/Linux и версии под X11 а также для Windows 98, Windows XP, Windows 10 
#
# чтоб собрать все надо запустить: 
# > make
#
# чтоб собрать только консольную версию для Linux
# > make pacman
#
# хотя можно и просто: 
# > gcc pacman.cpp
#
# чтоб собрать только версию для Linux под X11
# > make xpacman
#
# или так: 
# > g++ -o xpacman xpacman.cpp -lX11
#
# чтоб собрать только версию для Windows 98, Windows XP, Windows 10 нужен openwatcom 1.9 
# надо добавить в PATH путь до openwatcom/binl
#
# > export WATCOM=/opt/openwatcom
# > export PATH=$PATH:$WATCOM/binl
#
# сама сборка:
# > make wpacman
#
# или так 
# > wcl386 -I/opt/openwatcom/h -I/opt/openwatcom/h/nt -l=nt_win ws2_32.lib -bt=nt wpacman.cpp
# 
# запустить после сборки версию для Windows из Linux можно так: 
# > wine wpacman.exe
#
# собрать под linux для debug 
#	CFLAGS += -g
# собрать под linux для run (бинарник только с оптимизацией)
CFLAGS += -O2
# собрать под linux с linuxtools, надо раскоментировать флаги ниже 
#	CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
#	LDFLAGS += -pg -fprofile-arcs -ftest-coverage
#	EXTRA_CLEAN += pacman.gcda pacman.gcno $(PROJECT_ROOT)gmon.out
#	EXTRA_CMDS = rm -rf pacman.gcda


# собираем под Linux с удалением файлов прошлой сборки
all:	clean pacman xpacman wpacman

# сборка для Linux (консольная версия) 
# запустить после сборки можно так: ./pacman
pacman:	pacman.o
	$(CXX) $(LDFLAGS) -o $@ $^
	$(EXTRA_CMDS)

# сборка для Linux под Xы (графическая версия)
# запустить после сборки можно так: ./xpacman
xpacman: xpacman.o
	$(CXX) $(LDFLAGS) -o $@ $^ -lX11
	$(EXTRA_CMDS) 

pacman.o:	pacman.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

xpacman.o:	xpacman.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $< 

# сборка для Windows 98, Windows XP, Windows 10
# Нужен openwatcom 1.9
# запустить после сборки можно так: wine wpacman
wpacman:	
	wcl386 -I/opt/openwatcom/h -I/opt/openwatcom/h/nt -l=nt_win ws2_32.lib -bt=nt wpacman.cpp
# удалить фалы сборки
clean:
	rm -fr pacman pacman.o xpacman xpacman.o wpacman.exe wpacman.o wpacman.obj $(EXTRA_CLEAN)
