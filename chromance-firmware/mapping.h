/*
 * Maps hex topology onto LED's
 * (C) Voidstar Lab LLC 2021
 */

#ifndef MAPPING_H_
#define MAPPING_H_

// I accidentally noted these down 1-indexed and I'm too tired to adjust them
#define headof(S)           ((S * LEDS_PER_SEGMENTS))
#define tailof(S)           (headof(S) + (LEDS_PER_SEGMENTS -1))

#define NUMBER_OF_NODES     24
#define SIDES_PER_NODES     6

#define NUMBER_OF_SEGMENTS  38
#define SIDES_PER_SEGMENT   2
#define NUM_OF_PIXELS       (NUMBER_OF_SEGMENTS*LEDS_PER_SEGMENTS)

#define LEDS_PER_SEGMENTS   12


// Beam 0 is at 12:00 and advance clockwise
// -1 means nothing connected on that side
// First node is the upper left one, they are numbered from left to right 1 row at the time
// The value is the connected segment number
int nodeConnections[NUMBER_OF_NODES][SIDES_PER_NODES] = {
  {-1, -1, 2, -1, 3, -1},
  {-1, -1, 20, -1, 19, -1},
  {-1, -1, 22, -1, 21, -1},
  
  {-1, 3, 7, 4, -1, -1},
  {-1, 19, 18, 1, 8, 2},
  {-1, 21, 24, 26, 25, 20},
  {-1, -1, -1, 29, 23, 22},

  {-1, 8, -1, 6, -1, 7},
  {-1, 25, -1, 17, -1, 18},
  {-1, 23, -1, 30, -1, 24},

  {4, -1, 5, -1, -1, -1},
  {1, -1, 0, -1, 9, -1},
  {26, -1, 27, -1, 36, -1},
  {29, -1, -1, -1, 28, -1},
  
  {6, 9, 15, 10, -1, 5},
  {17, 36, 35, 13, 14, 0},
  {30, 28, -1, 31, 37, 27},

  {-1, 14, -1, 16, -1, 15},
  {-1, 37, -1, 34, -1, 35},
  
  {10, -1, 11, -1, -1, -1},
  {13, -1, 33, -1, 12, -1},
  {31, -1, -1, -1, 32, -1},
  
  {16, 12, -1, -1, -1, 11},
  {34, 32, -1, -1, -1, 33}
};
// int nodeConnections[NUMBER_OF_NODES][SIDES_PER_NODES] = {
//   {-1, -1, 3, -1, 4, -1},
//   {-1, -1, 21, -1, 22, -1},
//   {-1, -1, 23, -1, 22, -1},
  
//   {-1, 4, 8, 5, -1, -1},
//   {-1, 20, 19, 2, 9, 3},
//   {-1, 22, 25, 27, 26, 21},
//   {-1, -1, -1, 30, 24, 23},

//   {-1, 9, -1, 7, -1, 8},
//   {-1, 26, -1, 18, -1, 19},
//   {-1, 24, -1, 31, -1, 25},

//   {5, -1, 6, -1, -1, -1},
//   {2, -1, 1, -1, 10, -1},
//   {27, -1, 28, -1, 37, -1},
//   {30, -1, -1, -1, 29, -1},
  
//   {7, 10, 16, 11, -1, 6},
//   {18, 37, 36, 14, 15, 1},
//   {31, 29, -1, 32, 38, 28},

//   {-1, 15, -1, 17, -1, 16},
//   {-1, 38, -1, 35, -1, 36},
  
//   {11, -1, 12, -1, -1, -1},
//   {14, -1, 34, -1, 13, -1},
//   {32, -1, -1, -1, 33, -1},
  
//   {17, 13, -1, -1, -1, 12},
//   {35, 33, -1, -1, -1, 34}
// };

// int nodeConnections[NUMBER_OF_NODES][SIDES_PER_NODES] = {
//   {-1, -1, 4, -1, 3, -1},
//   {-1, -1, 5, -1, 4, -1},
//   {-1, -1, 6, -1, 5, -1},
  
//   {-1, 0, 7, 10, -1, -1},
//   {-1, 1, 8, 11,  7,  0},
//   {-1, 2, 9, 12, 8, 1},
//   {-1, -1, -1, 13, 9, 2},

//   {-1, 4, -1, 14, -1, 3},
//   {-1, 5, -1, 15, -1, 4},
//   {-1, 6, -1, 16, -1, 5},

//   {3, -1, 14, -1, -1, -1},
//   {4, -1, 15, -1, 14, -1},
//   {5, -1, 16, -1, 15, -1},
//   {6, -1, -1, -1, 16, -1},
  
//   {7, 11, 17, 19, -1, -1},
//   {8, 12, 18, 20, 17, 11},
//   {9, 13, -1, 21, 18, 12},

//   {-1, 15, -1, 22, -1, 14},
//   {-1, 16, -1, 23, -1, 15},
  
//   {14, -1, 22, -1, -1, -1},
//   {15, -1, 23, -1, 22, -1},
//   {16, -1, -1, -1, 23, -1},
  
//   {17, 20, -1, -1, -1, 19},
//   {18, 21, -1, -1, -1, 20}
// };

// First member: Node closer to ceiling
// Second: Node closer to floor
int segmentConnections[NUMBER_OF_SEGMENTS][SIDES_PER_SEGMENT] = {
  {11, 15},
  {4, 11},
  {0, 4},
  {0, 3},
  {3, 10},
  {10, 14},
  {7, 14},
  {3, 7},
  {4, 7},
  {11, 14},
  {14, 19},
  {19, 22},
  {20, 22},
  {15, 20},
  {15, 17},
  {14, 17},
  {17, 22},
  {8, 15},
  {4, 8},
  {1, 4},
  {1, 5},
  {2, 5},
  {2, 6},
  {6, 9}, // ayy
  {5, 9},
  {5, 8},
  {5, 12},
  {12, 16},
  {13, 16},
  {6, 13},
  {9, 16},
  {16, 21},
  {21, 23},
  {20, 23},
  {18, 23},
  {15, 18},
  {12, 15},
  {16, 18}
};

// First member: Strip number
// Second: LED index closer to ceiling
// Third: LED index closer to floor
int ledAssignments[NUMBER_OF_SEGMENTS][3] = {
  {0, tailof(0), headof(0)},
  {0, tailof(1), headof(1)},
  {0, tailof(2), headof(2)},
  {0, headof(3), tailof(3)},
  {0, headof(4), tailof(4)},
  {0, headof(5), tailof(5)},
  {0, tailof(6), headof(6)},
  {0, tailof(7), headof(7)},
  {0, tailof(8), headof(8)},
  {0, headof(9), tailof(9)},
  {0, headof(10), tailof(10)},
  {0, headof(11), tailof(11)},
  {0, tailof(12), headof(12)},
  {0, tailof(13), headof(13)},
  {0, headof(14), tailof(14)},
  {0, tailof(15), headof(15)},
  {0, headof(16), tailof(16)},
  {0, tailof(17), headof(17)},
  {0, tailof(18), headof(18)},
  {0, tailof(19), headof(19)},
  {0, headof(20), tailof(20)},
  {0, tailof(21), headof(21)},
  {0, headof(22), tailof(22)},
  {0, headof(23), tailof(23)},
  {0, tailof(24), headof(24)},
  {0, headof(25), tailof(25)},
  {0, headof(26), tailof(26)},
  {0, headof(27), tailof(27)},
  {0, tailof(28), headof(28)},
  {0, tailof(29), headof(29)},
  {0, headof(30), tailof(30)},
  {0, headof(31), tailof(31)},
  {0, headof(32), tailof(32)},
  {0, tailof(33), headof(33)},
  {0, tailof(34), headof(34)},
  {0, tailof(35), headof(35)},
  {0, tailof(36), headof(36)},
  {0, headof(37), tailof(37)}
};

// Border nodes are on the very edge of the network.
// Ripples fired here don't look very impressive.
int numberOfBorderNodes = 13;
int borderNodes[] = {0, 1, 2, 3, 6, 10, 13, 14, 16, 19, 21 , 22 , 23};

// Cube nodes link three equiangular segments
// Firing ripples that always turn in one direction will draw a cube
int numberOfCubeNodes = 7;
int cubeNodes[] = {7, 8, 9, 11, 12, 17, 18};

int numberOfTriNodes = 13;
int triNodes[] = {4,5, 7, 8, 9, 11, 12, 14,15,16, 17, 18, 20};

// Firing ripples that always turn in one direction will draw a starburst
int starburstNode = 15;

#endif
