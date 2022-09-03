# Для сборки под Linux
#
# чтоб собрать надо запустить: 
# make
# 
# хотя можно и просто: 
# gcc pacman.cpp

# объектный файл под linux
OBJS = pacman.o

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
all:	clean pacman

# собрать под Linux
linux: pacman

pacman:	$(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^
	$(EXTRA_CMDS)

pacman.o:	pacman.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

# удвлить фалы сборки под Linux
clean:
	rm -fr pacman $(OBJS) $(EXTRA_CLEAN)
