source var.sh
set -x

scp *.c $LOGIN@130.245.156.19:~/$PROJ_FOLDER/.
scp *.h $LOGIN@130.245.156.19:~/$PROJ_FOLDER/.
scp Makefile $LOGIN@130.245.156.19:~/$PROJ_FOLDER/.
