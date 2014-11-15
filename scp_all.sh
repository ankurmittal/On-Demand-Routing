source var.sh
set -x

scp *.c *.h Makefile $LOGIN@130.245.156.19:~/$PROJ_FOLDER/.
