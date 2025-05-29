CFLAGS=`pkg-config --cflags --libs wireplumber-0.5`

make:
	gcc -o dwlb-status -g main.c $(CFLAGS)
laptop:
	gcc -o dwlb-status -g main.c -DLAPTOP $(CFLAGS)
install: make
	cp -f dwlb-status /usr/local/bin/dwlb-status
uninstall:
	rm -f /usr/local/bin/dwlb-status
