#include <stdio.h>
#include <string.h>
int main(){
    char str[] = "phungson123";
    char test[100] = "" ;
    int j=0;
    for (int i = 0; i < strlen(str)/2; i++)
    {
        test[j] = (char)str[i];
        test[j+1] = (char)str[strlen(str) - i - 1];
        j+=2;
    }
    if(strlen(str)%2 == 1) 
        test[strlen(str) - 1] = str[strlen(str)/2];
    
    printf("%s\n", test);
    
}