# Name: Jingjing 
# Email: 
# ID: 


default: build

build: lab3b.py
	chmod +x lab3b.py
	rm -f lab3b
	ln -s lab3b.py lab3b

clean: 
	rm -f lab3b *.tar.gz

dist: 
	tar -czf lab3b-304567417.tar.gz lab3b.py Makefile README
