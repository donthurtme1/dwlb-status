make:
	gcc -o dwlb-status -g main.c -I/usr/include/pipewire-0.3 -I/usr/include/spa-0.2
install: make
	cp -f dwlb-status /usr/local/bin/dwlb-status
uninstall:
	rm -f /usr/local/bin/dwlb-status
