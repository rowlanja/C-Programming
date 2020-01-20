// code for a huffman coder


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "huff.h"
#include "bitfile.h"


// create a new huffcoder structure
struct huffcoder *  huffcoder_new()
{
	struct huffcoder * this = malloc(sizeof(struct huffcoder));
	memset(this->freqs, 0, NUM_CHARS * sizeof(int));
	memset(this->code_lengths, 0, NUM_CHARS * sizeof(int));
	memset(this->codes, 0, NUM_CHARS * sizeof(unsigned long long));
	return this;
}

// count the frequency of characters in a file; set chars with zero
// frequency to one
void huffcoder_count(struct huffcoder * this, char * filename)
{
    unsigned char c;       
    FILE * file = fopen(filename, "r");
    assert( file != NULL );
    c = fgetc(file);
    while(!feof(file)) {
        this->freqs[c]++; 
        c = fgetc(file);
    }
    for (int i = 0; i < NUM_CHARS; i++) { 
        if(this->freqs[i] == 0){
            this->freqs[i] = 1; 
        }
    }
    fclose(file); 
}


void swap(struct huffchar ** h1, struct huffchar ** h2) {
	struct huffchar * tmp = *h1;
	*h1 = *h2;
	*h2 = tmp;
}

//Compare huffchars
int compareHuffchars(struct huffchar * h1, struct huffchar * h2) {
	if(h1->freq < h2->freq)return 1;
	if(h1->freq == h2->freq && h1->seqno < h2->seqno)return 1;
	return 0;
}

void bubb_sort(struct huffchar * tree[], int size) {
	for(int allCounter = 0; allCounter < size; allCounter++)
		for(int someCounter = 0; someCounter < size - allCounter - 1; someCounter++)
			if(compareHuffchars(tree[someCounter], tree[someCounter + 1]))
				swap(&tree[someCounter], &tree[someCounter + 1]);
}

//taking two huffchars and making a compund from them
struct huffchar * compoundHuffchar(struct huffchar * h1, struct huffchar * h2, int seqNum) {
	struct huffchar * this = malloc(sizeof(struct huffchar)); 
	this->freq = h1->freq + h2->freq; 
	this->is_compound = 1;
	this->seqno = seqNum;
	if(compareHuffchars(h1, h2)) swap(&h1, &h2);
	this->u.compound.right = h1;
    this->u.compound.left = h2;
	return this;
}

void addLeaves(struct huffcoder * this, struct huffchar * tree[], int size) {
	for(int temp1 = 0; temp1 < size; temp1++) {
		struct huffchar * that = malloc(sizeof(struct huffchar)); 
		that->freq = this->freqs[temp1];
		that->is_compound = 0;
		that->seqno = temp1;
		that->u.c = temp1; 
		tree[temp1] = that; 
	}
}

void huffcoder_build_tree(struct huffcoder * this)
{
	struct huffchar * tree[NUM_CHARS];
	addLeaves(this, tree, NUM_CHARS);
	for(int loop = 0, seqNum = 256; loop < NUM_CHARS - 1; loop++) {
		int arraySize = NUM_CHARS - loop;
		bubb_sort(tree, arraySize); 
		tree[arraySize - 2] = compoundHuffchar(tree[arraySize - 1], tree[arraySize - 2], seqNum++); 
	}
	this->tree = tree[0]; 
}



void recurse_baby(struct huffcoder * this, struct huffchar * node, unsigned long long * path, int depth) {
	if(node->is_compound) 
	{
		*path <<= 1;
		unsigned long long tmp = *path;
		recurse_baby(this, node->u.compound.left, path, depth + 1);
		*path = tmp + 1;
		recurse_baby(this, node->u.compound.right, path, depth + 1);
	} else 
	{
		this->code_lengths[(int) node->u.c] = depth;
		this->codes[(int) node->u.c] = *path;
	}
}

void huffcoder_tree2table(struct huffcoder * this)
{
  recurse_baby(this, this->tree, malloc(sizeof(unsigned long long)), 0);
}

void huffcoder_print_codes(struct huffcoder * this)
{
  int i, j;
  char buffer[NUM_CHARS];

  for ( i = 0; i < NUM_CHARS; i++ ) {
    // put the code into a string
    assert(this->code_lengths[i] < NUM_CHARS);
    for ( j = this->code_lengths[i]-1; j >= 0; j--) {
      buffer[this->code_lengths[i]-1-j] = ((this->codes[i] >> j) & 1) + '0';
    }
    // don't forget to add a zero to end of string
    buffer[this->code_lengths[i]] = '\0';

    // print the code
    printf("char: %d, freq: %d, code: %s\n", i, this->freqs[i], buffer);;
  }
}

void addExtras(struct bitfile * file, unsigned long long code, int code_length) 
{
  for(int length_counter = code_length - 1; length_counter >= 0; length_counter--){
    bitfile_write_bit(file, (int) ((code >> length_counter) % 2));
    }
}

void huffcoder_encode(struct huffcoder * this, char * input_filename,
		      char * output_filename)
{
  FILE * input_file = fopen(input_filename, "r");
  char * write = "w";
  struct bitfile * output_file = bitfile_open(output_filename, write);
  unsigned char character = fgetc(input_file);
  while(!feof(input_file)) {          
    addExtras(output_file, this->codes[(int) character], this->code_lengths[(int) character]);
    character = fgetc(input_file);
  }
  addExtras(output_file, this->codes[4], this->code_lengths[4]);
  
  bitfile_close(output_file);
  fclose(input_file);
}

// decode the input file and write the decoding to the output file
void huffcoder_decode(struct huffcoder * this, char * input_filename,
		      char * output_filename)
{
 //open input file as readable
  char * read = "r";
  struct bitfile * input_file = bitfile_open(input_filename, read);
  //open output file as writable
  FILE * output_file = fopen(output_filename, "w");
  do {
    struct huffchar * tmp = this->tree;
    while(tmp->is_compound) {                //iterate through tree
      if(bitfile_read_bit(input_file))
        tmp = tmp->u.compound.right;
      else
        tmp = tmp->u.compound.left;
    }
    unsigned char character = tmp->u.c; //get charactergiven from code
    if(character == 4)                  //if end of transmission code is used set end of file
      input_file->is_EOF = 1;
    if(!bitfile_end_of_file(input_file))    //if it isn't the end of the file put char to output file
      fputc(character, output_file);
  } while(!bitfile_end_of_file(input_file)); //continue until the end of the file is reached
  
  //close files
  bitfile_close(input_file);
  fclose(output_file);
  
} 




