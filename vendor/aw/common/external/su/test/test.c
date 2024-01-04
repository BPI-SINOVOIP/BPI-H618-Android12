#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
int main(){
    printf("hello world");
    umask(0);
    return 0;
}
