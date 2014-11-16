source var.sh
set -x

scp *.c *.h Makefile *.mk $LOGIN@130.245.156.19:~/$PROJ_FOLDER/.
