/*****************************************************************************
Copyright (c) 2011-2014, The OpenBLAS Project
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
   3. Neither the name of the OpenBLAS project nor the names of 
      its contributors may be used to endorse or promote products 
      derived from this software without specific prior written 
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**********************************************************************************/


/*********************************************************************/
/* Copyright 2009, 2010 The University of Texas at Austin.           */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#define CPU_UNKNOWN         0
#define CPU_MIPS64_GENERIC  1
#define CPU_SICORTEX        2
#define CPU_LOONGSON3R3     3
#define CPU_LOONGSON3R4     4
#define CPU_I6400           5
#define CPU_P6600           6
#define CPU_I6500           7

static char *cpuname[] = {
  "UNKNOWN",
  "MIPS64_GENERIC"
  "SICORTEX",
  "LOONGSON3R3",
  "LOONGSON3R4",
  "I6400",
  "P6600",
  "I6500"
};

int detect(void){

#ifdef linux
  FILE *infile;
  char buffer[512], *p;

  p = (char *)NULL;
  //Check model name for Loongson3
  infile = fopen("/proc/cpuinfo", "r");
  while (fgets(buffer, sizeof(buffer), infile)){
    if (!strncmp("model name", buffer, 10)){
      p = strchr(buffer, ':') + 2;
      break;
    }
  }
  fclose(infile);
  if (p != NULL){
    if (strstr(p, "Loongson-3A3000") || strstr(p, "Loongson-3B3000")){
      return CPU_LOONGSON3R3;
    } else if (strstr(p, "Loongson-3A4000") || strstr(p, "Loongson-3B4000")){
      return CPU_LOONGSON3R4;
    } else{
      return CPU_SICORTEX;
    }
  }

  return CPU_MIPS64_GENERIC;
#else
  return CPU_UNKNOWN;
#endif
}

char *get_corename(void){
  return cpuname[detect()];
}

void get_architecture(void){
  printf("MIPS64");
}

void get_subarchitecture(void){
  if(detect()==CPU_LOONGSON3R3) {
    printf("LOONGSON3R3");
  }else if(detect()==CPU_LOONGSON3R4){
    printf("LOONGSON3R4");
  }else if(detect()==CPU_I6400){
    printf("I6400");
  }else if(detect()==CPU_P6600){
    printf("P6600");
  }else if(detect()==CPU_I6500){
    printf("I6500");
  }else if(detect()==CPU_SICORTEX){
    printf("SICORTEX");
  }else{
    printf("MIPS64_GENERIC");
  } 
}

void get_subdirname(void){
  printf("mips64");
}

void get_cpuconfig(void){
  if(detect()==CPU_LOONGSON3R3) {
    printf("#define LOONGSON3R3\n");
    printf("#define L1_DATA_SIZE 65536\n");
    printf("#define L1_DATA_LINESIZE 32\n");
    printf("#define L2_SIZE 512488\n");
    printf("#define L2_LINESIZE 32\n");
    printf("#define DTB_DEFAULT_ENTRIES 64\n");
    printf("#define DTB_SIZE 4096\n");
    printf("#define L2_ASSOCIATIVE 4\n");
  }else if(detect()==CPU_LOONGSON3R4){
    printf("#define LOONGSON3R4\n");
    printf("#define L1_DATA_SIZE 65536\n");
    printf("#define L1_DATA_LINESIZE 32\n");
    printf("#define L2_SIZE 512488\n");
    printf("#define L2_LINESIZE 32\n");
    printf("#define DTB_DEFAULT_ENTRIES 64\n");
    printf("#define DTB_SIZE 4096\n");
    printf("#define L2_ASSOCIATIVE 4\n");
  }else if(detect()==CPU_I6400){
    printf("#define I6400\n");
    printf("#define L1_DATA_SIZE 65536\n");
    printf("#define L1_DATA_LINESIZE 32\n");
    printf("#define L2_SIZE 1048576\n");
    printf("#define L2_LINESIZE 32\n");
    printf("#define DTB_DEFAULT_ENTRIES 64\n");
    printf("#define DTB_SIZE 4096\n");
    printf("#define L2_ASSOCIATIVE 8\n");
  }else if(detect()==CPU_P6600){
    printf("#define P6600\n");
    printf("#define L1_DATA_SIZE 65536\n");
    printf("#define L1_DATA_LINESIZE 32\n");
    printf("#define L2_SIZE 1048576\n");
    printf("#define L2_LINESIZE 32\n");
    printf("#define DTB_DEFAULT_ENTRIES 64\n");
    printf("#define DTB_SIZE 4096\n");
    printf("#define L2_ASSOCIATIVE 8\n");
  }else if(detect()==CPU_I6500){
    printf("#define I6500\n");
    printf("#define L1_DATA_SIZE 65536\n");
    printf("#define L1_DATA_LINESIZE 32\n");
    printf("#define L2_SIZE 1048576\n");
    printf("#define L2_LINESIZE 32\n");
    printf("#define DTB_DEFAULT_ENTRIES 64\n");
    printf("#define DTB_SIZE 4096\n");
    printf("#define L2_ASSOCIATIVE 8\n");
  }else{
    printf("#define SICORTEX\n");
    printf("#define L1_DATA_SIZE 32768\n");
    printf("#define L1_DATA_LINESIZE 32\n");
    printf("#define L2_SIZE 512488\n");
    printf("#define L2_LINESIZE 32\n");
    printf("#define DTB_DEFAULT_ENTRIES 32\n");
    printf("#define DTB_SIZE 4096\n");
    printf("#define L2_ASSOCIATIVE 8\n");
  }
#ifndef NO_MSA
  if (get_feature("msa")) printf("#define HAVE_MSA\n");
#endif
}

void get_libname(void){
  if(detect()==CPU_LOONGSON3R3) {
    printf("loongson3r3\n");
  }else if(detect()==CPU_LOONGSON3R4) {
    printf("loongson3r4\n");
  }else if(detect()==CPU_I6400) {
    printf("i6400\n");
  }else if(detect()==CPU_P6600) {
    printf("p6600\n");
  }else if(detect()==CPU_I6500) {
    printf("i6500\n");
  }else {
    printf("mips64_generic\n");
  }
}

int get_feature(char *search)
{

#ifdef __linux
        FILE *infile;
        char buffer[2048], *p,*t;
        p = (char *) NULL ;

        infile = fopen("/proc/cpuinfo", "r");

        while (fgets(buffer, sizeof(buffer), infile))
        {

                if (!strncmp("Features", buffer, 8) || !strncmp("ASEs implemented", buffer, 16))
                {
                        p = strchr(buffer, ':') + 2;
                        break;
                }
        }

        fclose(infile);

        if( p == NULL ) return 0;

        t = strtok(p," ");
        while( t = strtok(NULL," "))
        {
                if (strstr(t, search))   { return(1); }
        }

#endif
        return(0);
}

