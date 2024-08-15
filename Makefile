make:
	gcc -o dwlb-status dwlb-status.c
install: make
	cp -f dwlb-status /usr/local/bin/dwlb-status
uninstall:
	rm -f /usr/local/bin/dwlb-status
