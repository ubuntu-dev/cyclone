/****************** Start NSIEVE C Source Code ************************/

/****************************************************************/
/*                          NSIEVE                              */
/*                     C Program Source                         */
/*          Sieve benchmark for variable sized arrays           */
/*                Version 1.2b, 26 Sep 1992                     */
/*              Al Aburto  (aburto@nosc.mil)                    */
/*                                                              */
/*                                                              */
/* This Sieve of Eratosthenes program works with variable size  */
/* arrays. It is a straight forward extension of the original   */
/* Gilbreath version ( Gilbreath, Jim. "A High-Level Language   */
/* Benchmark." BYTE, September 1981, p. 180, and also Gilbreath,*/ 
/* Jim and Gary. "Eratosthenes Revisited: Once More Through the */
/* Sieve." BYTE January 1983, p. 283 ). Unlike the Sieve of     */
/* Gilbreath, NSIEVE uses register long variables, pointers,and */ 
/* large byte arrays via 'malloc()'.  Maximum array size is     */
/* currently set at 2.56 MBytes but this can be increased or    */
/* decreased by changing the program LIMIT constant.  NSIEVE    */
/* provides error checking to ensure correct operation.  Timer  */
/* routines are provided for several different systems. NSIEVE  */
/* results won't generally agree with the Gilbreath Sieve       */
/* results because NSIEVE specifically uses register long       */
/* variables. NSIEVE, and Sieve, are programs designed          */
/* specifically to generate and printout prime numbers (positive*/ 
/* integers which have no other integral factor other than      */
/* itself and unity, as 2,3,5,7,11, ... ). NSIEVE does not      */
/* conduct the 'typical' instructions one might expect from the */
/* mythical 'typical program'. NSIEVE results can be used to    */
/* gain a perspective into the relative performance of different*/
/* computer systems, but they can not be used in isolation to   */
/* categorize the general performance capabilities of any       */
/* computer system (no single benchmark program currently can do*/
/* this).                                                       */
/*                                                              */
/* The program uses a maximum array size of 2.56 MBytes. You can*/
/* increase or lower this value by changing the 'LIMIT' define  */
/* from 9 to a higher or lower value.  Some systems (IBM PC's   */
/* and clones) will be unable to work beyond 'LIMIT = 3' which  */
/* corresponds to an array size of only 40,000 bytes. Be careful*/
/* when specifying LIMIT > 3 for these systems as the system may*/ 
/* crash or hang-up. Normally NSIEVE will stop program execution*/  
/* when 'malloc()' fails.                                       */
/*                                                              */
/* The maximum array size is given by:                          */
/*              size = 5000 * ( 2 ** LIMIT ).                   */
/*                                                              */
/* The array 'Number_Of_Primes[LIMIT]' is intended to hold the  */
/* correct number of primes found for each array size up to     */
/* LIMIT = 20, but the array is only currently defined up to    */
/* LIMIT = 13.                                                  */
/*                                                              */
/* Program outputs to check for correct operation:              */
/*    Array Size  LIMIT    Primes Found      Last Prime         */
/*     (Bytes)                                                  */
/*         8191       0            1899           16381         */
/*        10000       1            2261           19997         */
/*        20000       2            4202           39989         */
/*        40000       3            7836           79999         */
/*        80000       4           14683          160001         */
/*       160000       5           27607          319993         */
/*       320000       6           52073          639997         */
/*       640000       7           98609         1279997         */
/*      1280000       8          187133         2559989         */
/*      2560000       9          356243         5119997         */
/*      5120000      10          679460        10239989         */
/*     10240000      11         1299068        20479999         */
/*     20480000      12         2488465        40960001         */
/*     40960000      13         4774994        81919993         */
/*     81920000      14         -------       ---------         */
/****************************************************************/

/* timing code ripped out */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int LIMIT = 9;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

long L_Prime,N_Prime;      /* Last Prime and Number of Primes Found */
long ErrorFlag;

long Number_Of_Primes[21]; /* List of Correct Number of Primes for */
			  /* each sieve array size.               */

long NLoops[21];


/**************************************/
/*  Sieve of Erathosthenes Program    */
/**************************************/

void SIEVE(long m,long n,long p)
{

char *flags;
long i,prime = -1,k,ci;
long count,size;

long  iter,j;

char *ptr;

size  = m - 1;
ptr   = malloc(m);

   ErrorFlag = 0L;
   N_Prime   = 0L;
   L_Prime   = 0L;

   if( !ptr )
     {
     ErrorFlag = 2L;
     return;
     }

   flags = ptr;

   j = 0;
						  /****************/
						  /* Instructions */
						  /*    *iter     */
						  /****************/
   for(iter=1 ; iter<=n ; iter++)                    
   {
   count = 0;                                       

   for(i=0 ; i<=size ; i++)                      
   {
   *(flags+i) = TRUE;                                /* 1*size  */
   }                                                 /* 3*size  */
						   
   ci = 0;                                         
     for(i=0 ; i<=size ; i++)                       
     {
	if(*(flags+i))                                /* 2*size  */
	{                                             /* 1*count */
	count++;                                      /* 1*count */
	prime = i + i + 3;                            /* 3*count */
	for(k = i + prime ; k<=size ; k+=prime)     /* 3*count */
	{
	ci++;                                       /* 1*ci    */
	*(flags+k)=FALSE;                           /* 1*ci    */
	}                                           /* 3*ci    */
						    /* 1*count */
	}
     }                                               /* 3*size  */
						   
   j = j + count;                                   
   }                                               
						   
   free(ptr);

   N_Prime = j / n;
   L_Prime = prime;

   if ( p != 0L )
   {
   printf("  %9ld   %8ld     %8ld\n",m,N_Prime,L_Prime);
   }

}

int main(int argc, char* argv[])
{

long  i,j,k,p;

if(argc > 1)
  LIMIT = atoi(argv[1]);

printf("\n   Sieve of Eratosthenes (Scaled to 10 Iterations)\n");
printf("   Version 1.2b, 26 Sep 1992\n\n");
printf("   Array Size   Number   Last Prime\n");       
printf("    (Bytes)   of Primes\n");

       
		   /*******************************/
		   /* Number of        Array Size */
		   /* Primes Found      (Bytes)   */
Number_Of_Primes[0] =     1899;      /*       8191 */
Number_Of_Primes[1] =     2261;      /*      10000 */
Number_Of_Primes[2] =     4202;      /*      20000 */
Number_Of_Primes[3] =     7836;      /*      40000 */
Number_Of_Primes[4] =    14683;      /*      80000 */
Number_Of_Primes[5] =    27607;      /*     160000 */
Number_Of_Primes[6] =    52073;      /*     320000 */
Number_Of_Primes[7] =    98609;      /*     640000 */
Number_Of_Primes[8] =   187133;      /*    1280000 */
Number_Of_Primes[9] =   356243;      /*    2560000 */
Number_Of_Primes[10]=   679460;      /*    5120000 */
Number_Of_Primes[11]=  1299068;      /*   10240000 */
Number_Of_Primes[12]=  2488465;      /*   20480000 */
Number_Of_Primes[13]=  4774994;      /*   40960000 */
Number_Of_Primes[14]=        0;      /*   81920000 */
Number_Of_Primes[15]=        0;      /*  163840000 */

j = 8191;
k = 256;
p = 0;
SIEVE(j,k,p);

for( i=1 ; i<= 20 ; i++)
{
 NLoops[i] = 1;
}

p = 8;

NLoops[0] = 256 * p; 
NLoops[1] = 256 * p; 
NLoops[2] = 128 * p;
NLoops[3] =  64 * p;
NLoops[4] =  32 * p;
NLoops[5] =  16 * p;
NLoops[6] =   8 * p;
NLoops[7] =   4 * p;
NLoops[8] =   2 * p;
NLoops[9] =       p;
NLoops[10] =  p / 2;
NLoops[11] =  p / 4;

i = 0;
j = 8191;
k = NLoops[0];
SIEVE(j,k,p);

j = 5000;
ErrorFlag = 0;

for( i=1 ; i<= LIMIT ; i++)
{
   j = 2 * j;

   k = NLoops[i];

   SIEVE(j,k,p);

   if( ErrorFlag == 0L )
   {
   if( N_Prime != Number_Of_Primes[i] )
   {
   printf("\n   Error --- Incorrect Number of Primes for Array: %ld\n",j);
   printf("   Number of  Primes  Found is: %ld\n",N_Prime);
   printf("   Correct Number of Primes is: %ld\n",Number_Of_Primes[i]);
   ErrorFlag = 1L;
   }
   }

   if( ErrorFlag > 0L ) break;

}

if( ErrorFlag == 2L )
{
printf("\n   Could Not Allocate Memory for Array Size: %ld\n",j);
return(-1);
}

return(0);

}

/*------ End of nsieve.c, say goodnight Liz! (Sep 1992) ------*/
