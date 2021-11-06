#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Note {
  double time;
  int8_t type;
  int8_t colorIndex;
  int8_t column;
  int8_t m_size;
};

const int MAX_FILESIZE = 1600000;

/* 
  time: time in seconds since the first tempo marker
  type:
    0 : match
    1 : beat
    2 : spin right
    3 : spin left
    4 : slider
    5 : note end
    6 : 
    7 : 
    8 : tap
    9 :
    10:
    11: beat hold end (soft or hard)
    12: scratch
    
  colorIndex:
    0: blue
    1: red
    
  column:
    [-3, 3] is standard
    [-4, 4] is visible
  
  m_size:
    tap, match, beat, spin, scratch default to and can only be 0
    
    sliders and slider midpoints default to 0, but once modified, range from [1, 5]
    sliders ends default to 0, but once modified, range from [1, 2]
    
    beat holds default to 2 (soft), but once modified, range from [1, 2]

    extra notes:
      sliders start at 0, which is identical to 1
      0 and 1 are cosine, 2 is curve-out, 3 is curve-in, 4 is line, 5 is 90deg
*/

struct Note parseNote(char *in) {
  struct Note ret;
  
  in = strstr(in, "time") + 7;
  ret.time = atof(in);
  
  in = strstr(in, "type") + 7;
  ret.type = atoi(in);
  
  in = strstr(in, "colorIndex") + 13;
  ret.colorIndex = atoi(in);
  
  in = strstr(in, "column") + 9;
  ret.column = atoi(in);
  
  in = strstr(in, "m_size") + 9;
  ret.m_size = atoi(in);
  
  return ret;
}

int import(struct Note *out, char *filename, int difficulty) {
  //arg check
  if(difficulty < 0 || difficulty > 4) {
    fprintf(stderr, "difficulty must be in range [0, 4]\n");
    exit(1);
  }
  
  //open the file
  FILE *infp = fopen(filename, "r");
  if(!infp) {
    fprintf(stderr, "failed to open file");
    exit(1);
  }
  
  //read the file to string
  char contentsArr[MAX_FILESIZE];
  char *contents = contentsArr;
  fgets(contents, MAX_FILESIZE, infp);
  
  //locate the data for the right difficulty
  contents = strstr(contents, "largeStringValuesContainer");
  for(int i = 0; i <= difficulty; i++)
    contents = strstr(contents, "TrackData_TrackData_") + 21;
  contents = strstr(contents, "notes");
  contents = strstr(contents, "time");
  
  //process each note and add to the array
  int nNote = 0;
  while(contents) {
    out[nNote] = parseNote(contents);
    contents++;
    nNote++;
    if((!strstr(contents, "time")) || strstr(contents, "time") - strstr(contents, "]") > 0)
      break;
    contents = strstr(contents, "time");
    
    if(nNote > 7998) {
      printf("Somehow, we hit %d notes.\n", nNote);
      break;
    }
  }
  
  return nNote;
  //options for other import methods later
}


/*
int main(int argc, char *argv[]) {
  struct Note notes[MAX_NOTES];
  int nNote = import(notes, argv[1], atoi(argv[2]));
  basicPrintout(notes, nNote);
}
*/