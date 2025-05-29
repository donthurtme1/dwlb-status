CFLAGS=`pkg-config --cflags --libs wireplumber-0.5`

make:
	gcc -o dwlb-status -g main.c $(CFLAGS)
test:
	gcc -o test test.c $(CFLAGS)
install: make
	cp -f dwlb-status /usr/local/bin/dwlb-status
uninstall:
	rm -f /usr/local/bin/dwlb-status
