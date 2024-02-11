# Для сборки из под Linux консольной версии игры для GNU/Linux и версии под X11 
# а также для Windows 98, Windows XP, Windows 10 
# ну и для DOS (IBM PC XT Intel 8088)
#
# чтоб собрать версию для DOS, Windows 98, Windows XP, Windows 10 нужен openwatcom 1.9 https://openwatcom.org/ftp/install/
# надо добавить в PATH путь до openwatcom/binl
#
# чтоб собрать версию для Linux нужен gcc, хотите версию для X11 будет нужна библиотека XLib
#
# чтоб собрать под DOS вам нужен mTCP http://brutmanlabs.org/mTCP/ 
# (но нужные исходники я уже положил в проект в папки  TCPINC и TCPLIB)
#
# > export WATCOM=/opt/openwatcom
# > export PATH=$PATH:$WATCOM/binl
#
# предполагается что openwatcom 1.9 для Linux установлен в opt/openwatcom
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
# сборка под Windows:
# > make wpacman
#
# или так 
# > wcl386 -I/opt/openwatcom/h -I/opt/openwatcom/h/nt -l=nt_win ws2_32.lib -bt=nt wpacman.cpp
# 
# запустить после сборки версию для Windows из Linux можно так: 
# > wine wpacman.exe
#
# чтоб собрать для DOS 
# > make pacman.exe
#
# запустить после сборки версию для DOS из Linux можно так: 
# > dosbox-x -conf ne2000.conf
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

# заголовочные файлы библиотек из проекта mTCP необходимые для сборки под DOS
tcp_h_dir = TCPINC/

# исходники библиотек из проекта mTCP необходимые для сборки под DOS
tcp_c_dir = TCPLIB/

# заголовочные файлы библиотек из проекта mTCP необходимые для сборки под DOS
common_h_dir = INCLUDE/

# заголовочные файлы библиотек openwatcom необходимые для сборки под DOS и Windows
openwatcom_h_dir = /opt/openwatcom/h

# заголовочные файлы библиотек для Win32 API необходимые для сборки под Windows
openwatcom_h_nt_dir = /opt/openwatcom/h/nt

# модель памяти при сборке под DOS
memory_model = -mc

# опции компиляции под DOS
compile_options = -0 $(memory_model) -oh -os -s -zp2 -zpw -we

# опции компиляции под DOS - пути к исходникам и заголовочным файлам
compile_options += -i=$(openwatcom_h_dir) -i=$(tcp_h_dir) -i=$(common_h_dir)

# собираем из под Linux исполняемые файлы для DOS, Windows, Linux с удалением файлов прошлой сборки
all: clean pacman xpacman wpacman pacman.exe

# сборка для Linux (консольная версия) 
# запустить после сборки можно так: ./pacman
pacman:	pacman-linux.o
	$(CXX) $(LDFLAGS) -o $@ $^
	$(EXTRA_CMDS)

# сборка для Linux под Xы (графическая версия)
# запустить после сборки можно так: ./xpacman
xpacman: xpacman.o
	$(CXX) $(LDFLAGS) -o $@ $^ -lX11
	$(EXTRA_CMDS) 

pacman-linux.o: pacman.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

xpacman.o: xpacman.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $< 

# сборка для Windows 98, Windows XP, Windows 10
# Нужен openwatcom 1.9
# запустить после сборки можно так: wine wpacman
wpacman: wpacman.cpp	
	wcl386 -i=$(openwatcom_h_dir) -i=$(openwatcom_h_nt_dir) -l=nt_win ws2_32.lib -bt=nt wpacman.cpp

# сборка для DOS
# Нужен openwatcom 1.9
# запустить после сборки можно так: dosbox-x -conf ne2000-linux.conf
pacman.exe: ipasm.o packet.o arp.o ip.o tcp.o tcpsockm.o udp.o utils.o timer.o trace.o dns.o eth.o pacman.o
	wlink system dos option map option eliminate option stack=4096 name $@ file ipasm.o, packet.o, arp.o, ip.o, tcp.o, tcpsockm.o, udp.o, utils.o, timer.o, trace.o, dns.o, eth.o, pacman.o

ipasm.o: $(tcp_c_dir)/ipasm.asm
	wasm -0 $(memory_model) $(tcp_c_dir)/ipasm.asm

packet.o: $(tcp_c_dir)/packet.cpp
	wpp $(tcp_c_dir)/packet.cpp $(compile_options)

arp.o: $(tcp_c_dir)/arp.cpp
	wpp $(tcp_c_dir)/arp.cpp $(compile_options)	

eth.o: $(tcp_c_dir)/eth.cpp
	wpp $(tcp_c_dir)/eth.cpp $(compile_options)

ip.o: $(tcp_c_dir)/ip.cpp
	wpp $(tcp_c_dir)/ip.cpp $(compile_options)

tcp.o: $(tcp_c_dir)/tcp.cpp
	wpp $(tcp_c_dir)/tcp.cpp $(compile_options)

tcpsockm.o: $(tcp_c_dir)/tcpsockm.cpp
	wpp $(tcp_c_dir)/tcpsockm.cpp $(compile_options)

udp.o: $(tcp_c_dir)/udp.cpp
	wpp $(tcp_c_dir)/udp.cpp $(compile_options)

utils.o: $(tcp_c_dir)/utils.cpp
	wpp $(tcp_c_dir)/utils.cpp $(compile_options)

dns.o: $(tcp_c_dir)/dns.cpp
	wpp $(tcp_c_dir)/dns.cpp $(compile_options)

timer.o: $(tcp_c_dir)/timer.cpp
	wpp $(tcp_c_dir)/timer.cpp $(compile_options)

trace.o: $(tcp_c_dir)/trace.cpp
	wpp $(tcp_c_dir)/trace.cpp $(compile_options)

pacman.o: pacman.cpp 
	wpp pacman.cpp $(compile_options)

# удалить все фалы сборки
clean:
	rm -fr pacman pacman.exe xpacman wpacman.exe *.o *.obj *.map $(EXTRA_CLEAN)
