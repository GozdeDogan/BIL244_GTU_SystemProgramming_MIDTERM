all: 
	  clear
	  gcc -c timeServer.c
	  gcc timeServer.o -o timeServer
	  gcc -c seeWhat.c
	  gcc seeWhat.o -o seeWhat
	  gcc -c showResult.c
	  gcc showResult.o -o showResult
clean:
	  rm *.o timeServer
	  rm *.o seeWhat
	  rm *.o showResult
