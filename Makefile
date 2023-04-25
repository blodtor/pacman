# Для сборки под Linux
#
# чтоб собрать надо запустить: 
# make
# 
# хотя можно и просто: 
# gcc pacman.cpp
#
# или если для Xов то так: 
# g++ -o xpacman xpacman.cpp -lX11

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
all:	clean pacman xpacman

pacman:	pacman.o
	$(CXX) $(LDFLAGS) -o $@ $^
	$(EXTRA_CMDS)

xpacman: xpacman.o
	$(CXX) $(LDFLAGS) -o $@ $^ -lX11 
	$(EXTRA_CMDS) 

pacman.o:	pacman.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

xpacman.o:	xpacman.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $< 

# удвлить фалы сборки под Linux
clean:
	rm -fr pacman pacman.o xpacman xpacman.o $(EXTRA_CLEAN)
