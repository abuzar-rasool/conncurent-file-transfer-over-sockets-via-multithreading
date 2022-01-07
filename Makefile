sender:
	gcc sender.c -o sender -lpthread

reciver:
	gcc reciver.c -o reciver -lpthread

clean:
	rm reciver
	rm sender

all: sender reciver
