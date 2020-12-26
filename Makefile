bin=HttpServer
cc=g++ -std=c++11
LADFAGS=-lpthread

.PHONY:all
all:$(bin) cal


$(bin):HttpServer.cc
	$(cc) -o $@ $^ $(LADFAGS)
cal:cal.cc
	gcc -o $@ $^

.PHONY:clean
clean:
	rm -f $(bin) cal
