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


# по умолчанию собираем под DOS
# если надо собирать по умолчанию под Linux заменить строчку ниже на 'all:	linux'
all:	clean-dos pacman.exe

# собрать под Linux
linux: pacman

pacman:	$(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^
	$(EXTRA_CMDS)

pacman.o:	pacman.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

# удвлить фалы сборки под Linux
clean-linux:
	rm -fr pacman $(OBJS) $(EXTRA_CLEAN)

# Параметры сборки для DOS
# Possible optimizations for 8088 class processors 
#
# -oa   Relax alias checking
# -ob   Try to generate straight line code
# -oe - expand user functions inline (-oe=20 is default)
# -oh   Enable repeated optimizations
# -oi   generate certain lib funcs inline
# -oi+  Set max inline depth (C++ only, use -oi for C)
# -ok   Flowing of register save into function flow graph
# -ol   loop optimizations
# -ol+  loop optimizations plus unrolling
# -or   Reorder for pipelined (486+ procs); not sure if good to use
# -os   Favor space over time
# -ot   Favor time over space
# -ei   Allocate an "int" for all enum types
# -zp2  Allow compiler to add padding to structs
# -zpw  Use with above; make sure you are warning free!
# -0    8088/8086 class code generation
# -s    disable stack overflow checking
# -zmf  put each function in a new code segment; helps with linking

# For this code performance is not an issue.  Make it small.

# модель памяти
memory_model = -ms
# опции компиляции
compile_options = -0 $(memory_model) -oh -ok -os -s -oa -ei -zp2 -zpw -we
# объектный файл под DOS
objs = pacman.obj

# собрать под DOS
dos: clean-dos pacman.exe

.cpp.obj :
	wpp $[* $(compile_options)

pacman.exe : $(objs)
	wlink system dos option map option eliminate option stack=4096 name $@ file *.obj
	
# удалить фалы сборки под DOS
clean-dos : .symbolic
	@del pacman.exe
	@del *.obj
	@del *.map