# SRTB-Analyzer

## Using SRTB Analyzer  
#### Instructions for linux command terminal
first, build the project:  
```make ```
    
to analyze a chart:  
```analyze [filename] [difficulty]```  
- filename points to a .srtb file
- difficulty is a number from 0 (easy) to 4 (XD)
  
  
## What can SRTB Analyzer do? 
#### Currently, it will 
* Show you basic stats about the notes in the chart
* Show you basic stats about colors and movement in the chart
  * there is a cheap system in place to counteract extra movement added by aesthetic sliders and match strings
* Show you the max score and combo attainable
* Mention any erroneous notes
  
  
## What features are planned? 
* Analyze the difficulty of the chart in various categories, including:
  * Endurance
  * Movement
  * Rhythm complexity
  * Readability
  * CTM or Controller-specific challenge
* Use the various difficulties to suggest an approximate difficulty value
  * This will be based on a database of standardized charts
* Analyze the chart for specific semi- or non-standard patterns
* Analyze the chart for specific low-difficulty standards

## Your code sucks  
  oh..
