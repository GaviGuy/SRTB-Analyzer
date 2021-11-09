#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Loader.h"

const int MAX_NOTES = 8000;

struct Note *notes;

//items directly in chart
int nNote = 0;
int nRealNote = 0;
int nMatch = 0;
int nTap = 0;
int nBeat = 0;
int nBeathold = 0;
int nBeatholdRelease = 0;
int nSlider = 0;
int nNoteEnd = 0;
int nSliderEnd = 0;
int nSliderRelease = 0;
int nSpin = 0;
int nSpinLeft = 0;
int nSpinRight = 0;
int nScratch = 0;
int nBlue = 0;
int nRed = 0;

//items identified from chart elements 
int nColorSwap = 0;
int nSameLaneSwap = 0;
int nMovement = 0;
int nMovementLeft = 0;
int nMovementRight = 0;
int maxScore = 0;
int nAesRight = 0;
int nAesLeft = 0;
float duration;

//transient helper items
/*
  used to identify movement for a variety of things
    9 at start and after spins (arbitrary sentinel value)
    Note.column if current note is a match, tap, slider, or sliderEnd
*/
//int prevLane;

/*
  used to identify color swaps and same-lane color swaps
    -1 if none (at start, after spins)
    set after matches, taps, or sliders
      0 if blue, 1 if red
*/
//int prevColor = -1;

/*
  used to identify backspins, darnocs, conrads, and post-scratch variants
    do not change if there is no movement
    account for how a human would move, not how metalman would
    -value after left movement
    +value after right movement
    0 at start and after spins
*/
//int prevMovement = 0;

/*
  used to calculate movementSinceReset
    set to the value of current note.column - prevLane if this note is a match, tap, slider, or sliderEnd
      value is adjusted for color swaps
      special case if prevLane is 9
    set to 0 for spins, scratches
*/
//int movement = 0;

/*
  used to identify cases of drift or backwards windup before a spin
    values exceeding +/-16 are considered mild, 20 is moderate, 24 is major, 28+ is DANGEROUS!!!
    movement is added for each note
    set to 0 after spins
    set to 0 after extended periods with no lane notes
*/
//int movementSinceReset = 0;

int sortNotes() {
  for(int i = 0; i < nNote; i++) {
    switch(notes[i].type) {
      case 0: //match
        nMatch++;
        if(notes[i].colorIndex == 0)
          nBlue++;
        else
          nRed++;
        break;
      case 1: //beat
        nBeat++;
        break;
      case 2: //spin right
        nSpinRight++;
        break;
      case 3: //spin left
        nSpinLeft++;
        break;
      case 4: //slider start
        nSlider++;
        if(notes[i].colorIndex == 0)
          nBlue++;
        else
          nRed++;
        break;
      case 5: //slider end
        nSliderEnd++;
        
        if(notes[i].m_size > 1) { //if this is >1, it might be a hard release
          for(int j = i + 1; j < nNote; j++) {
            //if next note is slider end, this isn't a release
            if(notes[j].type == 5)
              break;
            //if next note is spin, scratch, or slider, or there are no more notes, this is a release
            if(notes[j].type == 2 || notes[j].type == 3 || notes[j].type == 4 || notes[j].type == 12 || j + 1 == nNote) {
              nSliderRelease++;
            break;
            }
          //if next note is none of the above, check the one after it
          }
        }
        break;
      case 8: //tap
        nTap++;
        if(notes[i].colorIndex == 0)
          nBlue++;
        else
          nRed++;
        break;
      case 11: //beathold
        nBeathold++;
        if(notes[i].m_size==1)
          nBeatholdRelease++;
        break;
      case 12:
        nScratch++;
        break;
      default: //none of the above
        printf("Found an unknown note of type: %d\n", notes[i].type);
    }
  }
  nRealNote = nNote - nBeathold + nBeatholdRelease - nSliderEnd + nSliderRelease;
  nSpin = nSpinLeft + nSpinRight;
  duration = notes[nNote-1].time - notes[0].time;
}


//TODO: Account for vibrato strings
int calculateMovement() {
  int prevLane;
  int prevColor = -1;
  
  //aesthetic countermeasures
  int movesSinceMove = 0;
  int side = 0;
  
  
  for(int i = 0; i < nNote; i++) {
    switch(notes[i].type) {
      case 0:
      case 4:
      case 5:
      case 8:
        if(prevColor == -1) { //first note or post-spin
          if(notes[i].type == 5) //noteEnd either ending spin or erroneous
            //TODO: Check for error noteEnd
            break;
          
          //reset aesthetic checks
          movesSinceMove = 0;
          if(notes[i].column > 0)
            side = 1;
          else if(notes[i].column < 0)
            side = -1;
          else
            side = 0;
        }
        else if(prevColor != notes[i].colorIndex && notes[i].type != 5) {
          nColorSwap++;
          if(prevLane == notes[i].column) { //same lane swap
            
            //reset aesthetic checks
            movesSinceMove = 0;
            if(notes[i].column > 0)
              side = 1;
            else if(notes[i].column < 0)
              side = -1;
            else
              side = 0;
            
            nSameLaneSwap++;
            nMovement += 4;
            if(notes[i].column > 0) //assume swapping outward (mid-swaps are ambiguous)
              nMovementRight += 4;
            else
              nMovementLeft += 4;
          }
          else if(prevLane - notes[i].column != 4 && prevLane - notes[i].column != -4) { //non-standard swap
            printf("Found a nonstandard color swap at time: %f\n", notes[i].time);
            //TODO: Check for non-standard swap
            //TODO: potential false-positive on a match chord color transition
            movesSinceMove = 0;
            if(prevLane > notes[i].column) {
              nMovementLeft += 4 - (prevLane - notes[i].column);
              nMovement += 4 - (prevLane - notes[i].column);
            }
            else {
              nMovementRight += 4 - (notes[i].column - prevLane);
              nMovement += 4 - (notes[i].column - prevLane);
            }
          }
        }
        else if(prevLane != notes[i].column) { //non-swap movement
          //aesthetic and movement check
          /*
            if more than 8 lanes of movement have occured since the last
            instance of a color swap or entering/passing the standard lane
            opposite the current side, movements will be discarded
            
            this should reduce the effect of aesthetic sliders/matches that
            don't *actually* require movement
            
            some patterns will result in false-positives (repeated 1-3 swaps and related)
            some patterns will result in false-negatives (slider on Wight to Remain, color shenanigans)
          */
          if(prevLane > notes[i].column) {
            int mov = (prevLane - notes[i].column);
            nMovementRight += mov;
            nMovement += mov;
            movesSinceMove += mov;
            if(movesSinceMove > 4) //aesthetic check
              nAesRight += mov;
          }
          else {
            int mov = (notes[i].column - prevLane);
            nMovementLeft += mov;
            nMovement += mov;
            movesSinceMove += mov;
            if(movesSinceMove > 4) //aesthetic check
              nAesLeft += mov;
          }
        }
        if(notes[i].column * side <= -2) { //changed sides, reset aesthetic checks
          movesSinceMove = 0;
          if(notes[i].column > 0)
            side = 1;
          else if(notes[i].column < 0)
            side = -1;
          else
            side = 0;
        }
        
        prevLane = notes[i].column;
        if(notes[i].type != 5)
          prevColor = notes[i].colorIndex;
        break;
      case 2:
        nMovement += 4;
        nMovementRight += 4;
        prevColor = -1;
        break;
      case 3:
        nMovement += 4;
        nMovementLeft += 4;
        prevColor = -1;
        break;
      
    }
  }
}

int calculateScore() {
  maxScore = 0;
  for(int i = 0; i < nNote; i++) {
    double dur;
    int valid = 0;
    switch(notes[i].type) {
      case 0: //match, 5
        maxScore += 5;
        break;
        
      case 1: //beat, 16
        maxScore += 16;
        break;
        
      case 2:  //spin
      case 3:  //spin
      case 12: //scratch
        dur = 1.0;
        for(int j = i + 1; j < nNote; j++) {
          switch (notes[j].type) {
            case 0:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 8:
            case 12:
              dur = notes[j].time - notes[i].time;
              valid = 1;
          }
          if(valid)
            break;
        }
        switch(notes[i].type) {
          case 2:  //spin, 12 + 20/s
          case 3:  //spin, 12 + 20/s
            maxScore += 12;
            maxScore += (int) (dur * 20);
            break;
          case 12: //scratch, 40/s
            maxScore += (int) (dur * 40);
        }
        break;
        
      case 4: //slider, 12 + 20/s
        //go forward until spin, scratch, slider, or end of song
        //  grab the last noteEnd before said spin, etc.
        //  calculate duration
        for(int j = i + 1; j < nNote; j++) {
          if(notes[j].type == 5)
            valid = j;
          if(notes[j].type == 2 || notes[j].type == 3 || notes[j].type == 4 || notes[j].type == 12)
            break;
        }
        if(valid) {
          dur = notes[valid].time - notes[i].time;
          maxScore += (int) (dur * 30);
          maxScore += 12;
          if(notes[valid].m_size > 1)
            maxScore += 12;
        }
        else { //special case: invalid sliders score 3 points
          maxScore += 3;
          printf("Found an invalid slider at t: %f\n", notes[i].time);
          //TODO: report invalid slider
        }
        break;
      
      case 8: //tap, 12
        maxScore += 12;
        break;
        
      case 11: //beathold, 1 + 40/s
        //trace it back to the last beat
        //40 per second
        for(int j = i - 1; j >= 0; j--) {
          if(notes[j].type == 1) {
            valid = 1;
            dur = notes[i].time - notes[j].time;
            break;
          }
          if(notes[j].type == 11) {
            break;
          }
        }
        if(!valid) {
          //TODO: track this error
          printf("Found an invalid beathold end at t: %f\n", notes[i].time);
          break;
        }
        maxScore += (int) (dur * 40) + 1;
        
        //beathold release, 16
        if(notes[i].m_size < 2) {
          maxScore += 16;
        }
        break;
    }
  }
  maxScore *= 4; //multiplier
}

int scorePrintout() {
  printf("Highest base score:     %d\n", maxScore);
  printf("PFC/perfect acc bonus:  %d\n", (int) round(((float)maxScore)/10));
  printf("Highest possible score: %d\n", maxScore + ((int) round(((float)maxScore)/10)) * 2);
  printf("\n");
  printf("Highest possible combo: %d\n", nRealNote);
  printf("\n");
  
}

int basicPrintout() {
  printf("# of notes total: %d\n", nNote);
  printf("\n");
  printf("# of Matches:   %d\n", nMatch);
  printf("# of Taps:      %d\n", nTap);
  printf("# of Beats:     %d\n", nBeat);
  printf("# of Spins:     %d\n", nSpin);
  printf("# of Scratches: %d\n", nScratch);
  printf("\n");
  printf("# of Sliders:           %d\n", nSlider);
  printf("# of Slider Releases:   %d\n", nSliderRelease);
  printf("# of Beatholds:         %d\n", nBeathold);
  printf("# of Beathold Releases: %d\n", nBeatholdRelease);
  printf("\n");
  printf("# of Note Ends:         %d\n", nSliderEnd);
  printf("\n");
  printf("# of Color Swaps:     %d\n", nColorSwap);
  printf("# of Same-Lane Swaps: %d\n", nSameLaneSwap);
  printf("\n");
}

int ratioPrintout() {
  printf("Color ratio (blue:red):  %d:%d (%.0f:%.0f)\n", nBlue, nRed, roundf((float) nBlue / (float) (nRed + nBlue) * 100), roundf((float) nRed / (float) (nRed + nBlue) * 100));
  printf("Spin ratio (left:right): %d:%d (%.0f:%.0f)\n", nSpinLeft, nSpinRight, roundf((float) nSpinLeft / (float) nSpin * 100), roundf((float) nSpinRight / (float) nSpin * 100));
  printf("\n");
  printf("Magnitude of movement: %d\n", nMovement);
  printf("Ratio of L:R movement: %d:%d (%.0f:%.0f)\n", nMovementLeft, nMovementRight, roundf((float) nMovementLeft / (float) (nMovementLeft + nMovementRight) * 100), roundf((float) nMovementRight / (float) (nMovementLeft + nMovementRight) * 100));
  printf("\n");
  int nMoveAdj = nMovement - nAesLeft - nAesRight;
  int nMoveLeftAdj = nMovementLeft - nAesLeft;
  int nMoveRightAdj = nMovementRight - nAesRight;
  printf("Magnitude of movement (adjusted): %d\n", nMoveAdj);
  printf("Ratio of L:R movement (adjusted): %d:%d (%.0f:%.0f)\n", nMoveLeftAdj, nMoveRightAdj, roundf((float) nMoveLeftAdj / (float) (nMoveLeftAdj + nMoveRightAdj) * 100), roundf((float) nMoveRightAdj / (float) (nMoveLeftAdj + nMoveRightAdj) * 100));
  printf("\n");
}

int ratePrintout() {
  printf("Avg notes/second:          %f\n", (float) nNote / duration);
  printf("Avg taps & sliders/second: %f\n", (float) (nTap + nSlider) / duration);
  printf("Avg beats/second:          %f\n", (float) nBeat / duration);
  printf("Avg movement/second:       %f\n", (float) nMovement / duration);
  printf("Avg mvmt/sec (adjusted):   %f\n", (float) (nMovement - nAesLeft- nAesRight) / duration);
  printf("\n");
}

int main(int argc, char *argv[]) {
  notes = malloc(sizeof(struct Note) * MAX_NOTES);
  nNote = import(notes, argv[1], atoi(argv[2]));
  sortNotes();
  
  //aimottle's base score should be 11192
  printf("\n");
  calculateScore();
  calculateMovement();
  scorePrintout();
  basicPrintout();
  ratioPrintout();
  ratePrintout();
  
  free(notes);
}