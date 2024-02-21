all:
ifdef TYPE 
	gcc src/main.c src/include/log.c -o src/main
	gcc src/server.c src/include/log.c -o src/server
	gcc src/userinterface.c src/include/log.c -lncurses -o src/userinterface
	gcc src/drone.c -lm src/include/log.c -o src/drone
	gcc src/targets.c src/include/log.c -o src/targets
	gcc src/obstacles.c src/include/log.c -o src/obstacles
	gcc src/watchdog.c src/include/log.c -o src/watchdog
	sleep 2
	./src/main $(TYPE) $(ADDRESS) $(PORT) $(SECONDS)
else 
	echo 'Arguments incomplete'
endif

	

