# gcc -Iinclude/ main.c -o main
gcc main.c ../protocol/linklayer.c ../protocol/linklayer.h ../protocol/statemachine.h ../protocol/statemachine.c -o ./main