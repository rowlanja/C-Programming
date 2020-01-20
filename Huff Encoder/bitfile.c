// C code file for  a file ADT where we can read a single bit at a
// time, or write a single bit at a time

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "bitfile.h"

struct bitfile * bitfile_open(char * filename, char * mode)
{
	struct bitfile * result = malloc(sizeof(struct bitfile));
	result->file = fopen(filename, mode);
	result->buffer = 0;
	result->index = 7;
	result->is_EOF = 0;
	return result;
	
}

void bitfile_write_bit(struct bitfile * this, int bitBruh)
{	
    this->buffer |= (bitBruh << (7 - this->index));
	if(this->index == 0) {        
    fputc(this->buffer, this->file);
    this->buffer = 0;
    this->index = 7;
  }
  else this->index--;         
}

int bitfile_read_bit(struct bitfile * this)
{
  if(this->index == -1) this->index = 7; 
  if(this->index == 7) this->buffer = fgetc(this->file);
  return (this->buffer >> (7 - this->index--)) % 2;
}


void bitfile_close(struct bitfile * this) {
    if (!this->is_read_mode && this->index != 7)  fputc(this->buffer, this->file);
    fclose(this->file);
    this->is_EOF = 1;    
}

// check for end of file
int bitfile_end_of_file(struct bitfile * this)
{  
  if (this->is_EOF || feof(this->file))
     return 1;
       else return 0;
}


