CXX = g++
INCLUDES = -Iinclude/ -I.

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
FLAGS = $(INCLUDES) -Wall -O3 -D__OS_LINUX__ -D__UNIX_JACK__ -c -D__LINUX_ALSASEQ__ -g
LIBS = -lm -lstdc++ -lpthread -lglut -lGL -lGLU -ljack -lasound -lstk
endif
ifeq ($(UNAME), Darwin)
FLAGS = $(INCLUDES) -D__MACOSX_CORE__ -c -g
LIBS = -framework CoreAudio -framework CoreMIDI -framework CoreFoundation \
				-framework GLUT -framework OpenGL \
				-framework IOKit -framework Carbon -L. -lstdc++ -lm -lstk
endif

VPATH = osc ip ip/posix

OBJS = \
			 main.o \
			 Shape.o \
			 Engine.o \
			 Widget.o \
			 Color.o \
			 Point.o \
			 MyAudio.o \
			 Network.o \
			 OscOutboundPacketStream.o \
			 OscPrintReceivedElements.o \
			 OscTypes.o \
			 OscReceivedElements.o \
			 IpEndpointName.o \
			 NetworkingUtils.o \
			 UdpSocket.o \


playround: $(OBJS)
	$(CXX) -o playround $(OBJS) $(LIBS)

main.o: main.cpp
	$(CXX) $(FLAGS) main.cpp

Shape.o: Shape.cpp include/Shape.h include/Point.h
	$(CXX) $(FLAGS) Shape.cpp

Engine.o: Engine.cpp include/Engine.h Widget.cpp include/Widget.h
	$(CXX) $(FLAGS) Engine.cpp

Widget.o: Widget.cpp include/Widget.h
	$(CXX) $(FLAGS) Widget.cpp

Color.o: Color.cpp include/Color.h
	$(CXX) $(FLAGS) Color.cpp

Point.o: Point.cpp include/Point.h
	$(CXX) $(FLAGS) Point.cpp

MyAudio.o: MyAudio.cpp include/MyAudio.h
	$(CXX) $(FLAGS) MyAudio.cpp

Network.o: Network.cpp include/Network.h
	$(CXX) $(FLAGS) Network.cpp

%.o: %.cpp
	$(CXX) $(FLAGS) -c $< -o $@

clean:
	rm -f *~ *.o playround
