#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



#define TRUE 1
#define FALSE 0

#define DICTSIZE 4096                     /* allow 4096 entries in the dict  */
#define ENTRYSIZE 32

unsigned char dict[DICTSIZE][ENTRYSIZE];  /* of 30 chars max; the first byte */
                                          /* is string length; index 0xFFF   */
                                          /* will be reserved for padding    */
                                          /* the last byte (if necessary)    */

// These are provided below
int read12(FILE *infil);
int write12(FILE *outfil, int int12);
void strip_lzw_ext(char *fname);
void flush12(FILE *outfil);
void encode(FILE *in, FILE *out);
void decode(FILE *in, FILE *out);
int findFun(unsigned char *str);
int main(int argc, char **argv) {
    if (argc < 3){
        printf("Error: No input file specified!");
        exit(1);
    }
    if (argc >= 1){
        if (strcmp(argv[2],"e") != 0 && strcmp(argv[2],"d") != 0){
            printf("invald Usage, expected: RLE {%s} [e | d] to the console", argv[1]);
            exit(4);
        }
    }
    FILE *filePointer1;
    filePointer1 = fopen(argv[1], "rb");
    if (access(argv[1], R_OK) == -1){
	printf("Read error: file not found or cannot be read");
	exit(2);
    }
    if (strcmp(argv[2], "e") == 0){
	FILE *filePointerR;/*有可能有问题，filename问题*/
	filePointerR = fopen(argv[1], "rb");
	FILE *filePointerW;
        filePointerW = fopen(strcat(argv[1], ".LZW"), "wb");
	encode(filePointerR, filePointerW);
    }
    if (strcmp(argv[2], "d") == 0){
	FILE *filePointerR;
	filePointerR = fopen(argv[1],"rb");
	char *a;
	a = argv[1];
	strip_lzw_ext(a);
	FILE *filePointerW;
        filePointerW = fopen(a, "wb");
	decode(filePointerR, filePointerW);
    }
}

/*****************************************************************************/
/* encode() performs the Lempel Ziv Welch compression from the algorithm in  */
/* the assignment specification. The strings in the dictionary have to be    */
/* handled carefully since 0 may be a valid character in a string (we can't  */
/* use the standard C string handling functions, since they will interpret   */
/* the 0 as the end of string marker). Again, writing the codes is handled   */
/* by a separate function, just so I don't have to worry about writing 12    */
/* bit numbers inside this algorithm.                                        */
void encode(FILE *in, FILE *out) {
    unsigned char k = 0;
    int i;
    int a;
    int j;
    for (i = 0; i < DICTSIZE; i++){
	dict[i][0] = '0';
    }
    for (i = 0; i < 255; i++){
	dict[i][0] = (unsigned char)(1 + '0');
	dict[i][1] = (unsigned char)i;
    }
    unsigned char w[ENTRYSIZE];
    unsigned char wk[ENTRYSIZE];
    int len;
    w[0] = 0 + '0';
    wk[0] = 0 + '0';
    int index = -1;
    int count = 256;
    while ((a = fgetc(in)) != EOF){
	k = (unsigned char) a; //w+k
	len = w[0] - '0';
	for (i = 0; i < len + 1; i++){
	    wk[i] = w[i];
	}
	wk[len+1] = k;
	wk[0] = (len+1) + '0';
	index = findFun(wk);
	if (index >= 0){
            len =  wk[0] - '0';
            for (i = 0; i<len+1; i++){
                w[i] = wk[i];
            }
	}
	else{
	    write12(out, findFun(w));
	    len = wk[0] - '0';
	    for (j = 0; j < len + 1; j++){
		dict[count][j] = wk[j];
	    }
	    count++;
	    w[0] = 1 + '0';
	    w[1] = k;
	}
    }
    index = findFun(w);
    write12(out, findFun(w));
    flush12(out);
    fclose(out);
    // TODO implement
}

int findFun(unsigned char *str){
    int i;
    int j;
    int code = -1;
    for (i = 0; i < DICTSIZE; i++){
	if (str[0] == str[0]){
	    for (j=0; j < (str[0] - '0') + 1; j++){
		if (str[j] != dict[i][j]){
		    code = -1;
		    break;
		}
		else{
		    code = i;
		}
	    }
	    if (code >= 0){
		return code;
	    }
	}
    }
    return -1;
}

/*****************************************************************************/
/* decode() performs the Lempel Ziv Welch decompression from the algorithm   */
/* in the assignment specification.                                          */
void decode(FILE *in, FILE *out) {
    int c;
    int i;
    int len;
    int count = 256;
    unsigned char w[ENTRYSIZE];

    for (i = 0; i < DICTSIZE; i++){
        dict[i][0] = '0';
    }

    for (i = 0; i <= 255; i++){
        dict[i][0] = (unsigned char)(1 + '0');
        dict[i][1] = (unsigned char)i;
    }

    c = read12(in);
    len = dict[c][0] - '0';
    for (i = 1; i < (len +1);i++){
	   putc(dict[c][i], out);
    }

    len = dict[c][0] - '0';
    for (i = 0; i < (len + 1); i++){
        w[i] = dict[c][i];
    }

    while(  (c = read12(in)) >= 0){
    	if ((dict[c][0] - '0') != 0){
    	    len = dict[c][0] - '0';
    	    for (i = 1; i < len+1; i++){
    		  putc(dict[c][i], out);
    	    }
    	    len = w[0] - '0';
    	    for (i = 1; i <len + 1; i++){
    		  dict[count][i] = w[i];
    	    }
    	    dict[count][len+1] = dict[c][1];
    	    dict[count][0]= (len+1) + '0';
    	    count++;
    	}
    	else{
    	    len = w[0] - '0';
    	    for (i = 1; i < len+1; i++){
    		dict[count][i] = w[i];
    	    }
    	    dict[count][len+1] = w[1];
    	    dict[count][0] = (len + 1) + '0';
    	    count++;
    	    for (i = 1; i <len; i++){
    		  putc(w[i], out);
    	    }
    	}
    	len = dict[c][0] - '0';
    	for (i = 0; i < len + 1; i++){
    	    w[i] = dict[c][i];
    	}
    }
    // TODO implement
}


/*****************************************************************************/
/* read12() handles the complexities of reading 12 bit numbers from a file.  */
/* It is the simple counterpart of write12(). Like write12(), read12() uses  */
/* static variables. The function reads two 12 bit numbers at a time, but    */
/* only returns one of them. It stores the second in a static variable to be */
/* returned the next time read12() is called.                                */
int read12(FILE *infil)
{
 static int number1 = -1, number2 = -1;
 unsigned char hi8, lo4hi4, lo8;
 int retval;

 if(number2 != -1)                        /* there is a stored number from   */
    {                                     /* last call to read12() so just   */
     retval = number2;                    /* return the number without doing */
     number2 = -1;                        /* any reading                     */
    }
 else                                     /* if there is no number stored    */
    {
     if(fread(&hi8, 1, 1, infil) != 1)    /* read three bytes (2 12 bit nums)*/
        return(-1);
     if(fread(&lo4hi4, 1, 1, infil) != 1)
        return(-1);
     if(fread(&lo8, 1, 1, infil) != 1)
        return(-1);

     number1 = hi8 * 0x10;                /* move hi8 4 bits left            */
     number1 = number1 + (lo4hi4 / 0x10); /* add hi 4 bits of middle byte    */

     number2 = (lo4hi4 % 0x10) * 0x0100;  /* move lo 4 bits of middle byte   */
                                          /* 8 bits to the left              */
     number2 = number2 + lo8;             /* add lo byte                     */

     retval = number1;
    }

 return(retval);
}

/*****************************************************************************/
/* write12() handles the complexities of writing 12 bit numbers to file so I */
/* don't have to mess up the LZW algorithm. It uses "static" variables. In a */
/* C function, if a variable is declared static, it remembers its value from */
/* one call to the next. You could use global variables to do the same thing */
/* but it wouldn't be quite as clean. Here's how the function works: it has  */
/* two static integers: number1 and number2 which are set to -1 if they do   */
/* not contain a number waiting to be written. When the function is called   */
/* with an integer to write, if there are no numbers already waiting to be   */
/* written, it simply stores the number in number1 and returns. If there is  */
/* a number waiting to be written, the function writes out the number that   */
/* is waiting and the new number as two 12 bit numbers (3 bytes total).      */
int write12(FILE *outfil, int int12)
{
 static int number1 = -1, number2 = -1;
 unsigned char hi8, lo4hi4, lo8;
 unsigned long bignum;

 if(number1 == -1)                         /* no numbers waiting             */
    {
     number1 = int12;                      /* save the number for next time  */
     return(0);                            /* actually wrote 0 bytes         */
    }

 if(int12 == -1)                           /* flush the last number and put  */
    number2 = 0x0FFF;                      /* padding at end                 */
 else
    number2 = int12;

 bignum = number1 * 0x1000;                /* move number1 12 bits left      */
 bignum = bignum + number2;                /* put number2 in lower 12 bits   */

 hi8 = (unsigned char) (bignum / 0x10000);                     /* bits 16-23 */
 lo4hi4 = (unsigned char) ((bignum % 0x10000) / 0x0100);       /* bits  8-15 */
 lo8 = (unsigned char) (bignum % 0x0100);                      /* bits  0-7  */

 fwrite(&hi8, 1, 1, outfil);               /* write the bytes one at a time  */
 fwrite(&lo4hi4, 1, 1, outfil);
 fwrite(&lo8, 1, 1, outfil);

 number1 = -1;                             /* no bytes waiting any more      */
 number2 = -1;

 return(3);                                /* wrote 3 bytes                  */
}

/** Write out the remaining partial codes */
void flush12(FILE *outfil)
{
 write12(outfil, -1);                      /* -1 tells write12() to write    */
}                                          /* the number in waiting          */

/** Remove the ".LZW" extension from a filename */
void strip_lzw_ext(char *fname)
{
    char *end = fname + strlen(fname);

    while (end > fname && *end != '.' && *end != '\\' && *end != '/') {
        --end;
    }
    if ((end > fname && *end == '.') &&
        (*(end - 1) != '\\' && *(end - 1) != '/')) {
        *end = '\0';
    }
}








