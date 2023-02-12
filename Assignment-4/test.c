#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>

// int  main(){
//     char a[100], b[100], c[100];
//     int d;
//     char ch;
//     scanf("http://%[^/]s", a);
//     scanf("/%[^:]s", b);
//     scanf(":%d", &d);
//     while ((ch=getchar())!='\n')
//     {
//         putchar(ch);
//     }
    
    
//     printf("+%s+%s+%d+\n", a, b, d);
//     return 0;
// }


// int main() {

    // time_t now;
    // struct tm *tm_now;
    // char buf[40];

    // time(&now);
    // now -= 2*24*60*60;
    // tm_now = gmtime(&now);
    // strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", tm_now);

    // printf("Current time in HTTP-date format: %s\n", buf);

//     char file_path[] = "/path/to/fi.le.ext";
//     strcspn
//     char *extension = strrchr(file_path, '.');
//     if (extension) {
//         printf("The file extension is: %s\n", extension + 1);
//     } else {
//         printf("The file has no extension\n");
//     }



//     return 0;
// }

// C program to find the size of file
#include <stdio.h>
  
long int findSize(char file_name[])
{
    // opening the file in read mode
    FILE* fp = fopen(file_name, "r");
  
    // checking if the file exist or not
    if (fp == NULL) {
        printf("File Not Found!\n");
        return -1;
    }
  
    fseek(fp, 0L, SEEK_END);
  
    // calculating the size of the file
    long int res = ftell(fp);
  
    // closing the file
    fclose(fp);
  
    return res;
}
  
// Driver code
int main()
{
    char file_name[] = "MyHTTP.c";
    long int res = findSize(file_name);
    if (res != -1)
        printf("Size of the file is %ld bytes \n", res);
    return 0;
}
