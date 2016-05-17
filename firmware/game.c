#include <avr/io.h>
#include "game.h"
#include "oledControl.h"
#include "menu.h"
#include <stdlib.h>

void gameInitBuffer(void) {
    for (uint8_t page=0; page<4; page++) {
        for (uint8_t col=9; col<64; col++) {
            buffer[page][col] = 0x00;
        }
    }
}

void gameSetMetaPixel(uint8_t x, uint8_t y, uint8_t status) {
    //status is ON (non-zero) or OFF (zero)
    //oled is 128x64, our meta board will be 0-64 (x) and 0-32 (y)

    //oled is written in 'pages' of 8-bits. This particular screen
    //cannot be read so the game buffer must be used to ensure writing
    //a new game pixel doesn't overwrite existing pixels.

    //make our changes to the buffer
    uint8_t metaPage = y/8; 
    uint8_t metaPageY = 1<<(y%8);
    if (status) { buffer[metaPage][x] |= metaPageY; }
    else { buffer[metaPage][x] &= ~metaPageY; }

    //write to the screen accounting for 2x pixel sizes meta-v-actual
    uint8_t subMetaPage = (y%8)/4;
    uint8_t actualPage = (metaPage*2) + subMetaPage;
    uint8_t actualX = x*2;
    
    uint8_t metaData = buffer[metaPage][x];
    if (subMetaPage) { metaData>>=4; }  //Shift data to get to upper nibble if necessary
    uint8_t actualData = 0x00;
    for (uint8_t i=0; i<4; i++) {
        if (metaData & (1<<i)) {
            //Each '1' in the meta data is amplified to two bits in the actual
            actualData |= (0b11<<(i*2));
        }
    }
    
    oledSetCursor(actualX,actualPage);
    oledWriteData(actualData);
    oledWriteData(actualData);
}

void change_direction(void)
{
    //Direction cheat sheet:
    //  -1 == left or up
    //  0 == no movement
    //  1 == right or down
    
    // Account for current X, current Y, and C or CC knob movement
    if (dirX != 0) {
        //Currently moving left or right
        if (change_dir > 0) { dirY = 0-dirX; }
        else { dirY = dirX; }
        dirX = 0;
    }
    else {
        //Currently moving up or down
        if (change_dir < 0) { dirX = 0-dirY; }
        else { dirX = dirY; }
        dirY = 0;
    }
    change_dir = 0;
}

void snakeCounterClockwise(void) {
    change_dir = -1;
}

void snakeClockwise(void) {
    change_dir = 1;
}

uint8_t absolute_difference(uint8_t a, uint8_t b)
{
  int8_t unknown = (int8_t)a - b;
  return (uint8_t)(unknown<0?0-unknown:unknown); 
}

uint8_t neighbors(point node1, point node2)
{
  if ((absolute_difference(node1.x,node2.x) == 1) || (absolute_difference(node1.y,node2.y) == 1)) return 1;
  return 0;
}

void make_fruit(void) {
  fruit.x = (uint8_t)(rand()%(GAMEBOARD_X));
  fruit.y = (uint8_t)(rand()%(GAMEBOARD_Y));
  //TODO: Make sure fruit isn't overlapping the snake.
  gameSetMetaPixel(fruit.x, fruit.y, ON);
  //Draw_Box(fruit.x*SNAKE_GIRTH,fruit.y*SNAKE_GIRTH,(fruit.x*SNAKE_GIRTH)+SNAKE_GIRTH-1,(fruit.y*SNAKE_GIRTH)+SNAKE_GIRTH-1,FRUIT_COLOR);
}

uint8_t ate_fruit(uint8_t x, uint8_t y)
{
  if ((fruit.x == x) && (fruit.y == y)) return 1;
  return 0;
}

void game_over(void)
{
    strcpy_P(tempStr, strOptGameOver);
    putString(32,3,tempStr,0);
    game_running = 0;
}

void move_head(uint8_t new_dir)
{
  if (new_dir)
  {
    //Copy head to new position
    head = get_next_node(head); //increment head
    corners[head].x = corners[get_previous_node(head)].x;
    corners[head].y = corners[get_previous_node(head)].y;
    change_direction();  //change direction
  }
  
  //Have we left the game board?
  if ((corners[head].x == 0) && (dirX == -1)) { game_over(); return; }
  if ((corners[head].y == 0) && (dirY == -1)) { game_over(); return; }
  if ((corners[head].x == GAMEBOARD_X-1) && (dirX == 1)) { game_over(); return; }
  if ((corners[head].y == GAMEBOARD_Y-1) && (dirY == 1)) { game_over(); return; }
  corners[head].x += dirX;
  corners[head].y += dirY;
  ++snake_length_current;
}

void follow_tail(void)
{
  --snake_length_current;
  //is tail a neighbor of next?
  if (neighbors(corners[tail],corners[get_next_node(tail)]))
  {
    tail = get_next_node(tail);
  }
  
  //find which axis tail and next have in common
  else
  {
    if (corners[tail].x == corners[get_next_node(tail)].x)
    {
      //These points have the same X, so make adjustment to the Y
      if ((corners[tail].y - corners[get_next_node(tail)].y) < 0) corners[tail].y += 1;
      else corners[tail].y -= 1; 
    }
    else
    {
      //These points have the same Y, so make adjustment to the X
      if ((corners[tail].x - corners[get_next_node(tail)].x) < 0) corners[tail].x += 1;
      else corners[tail].x -= 1; 
    }
  }
}

uint8_t collision(void)
{
  uint8_t lower = 0;
  uint8_t upper = 0;
  uint8_t testpoint = 1;
  uint8_t i=tail;
  uint8_t nextNode = get_next_node(i);;

  //Check to see if we hit part of the snake
  //traverse all nodes from tail forward
  for (int8_t count=get_node_list_length(tail,head)-3; count>0; count--)
  //while (nextNode<head)
    //( check head-3 because you can't run into a segment any close than that to the head)
  { 
    //check to see if head's x or y are shared with the current point
    if ((corners[head].x == corners[i].x) && (corners[i].x == corners[nextNode].x))
    {
      //which point is the higher  number?
      if (corners[i].y < corners[nextNode].y) {lower = corners[i].y; upper = corners[nextNode].y;}
      else {lower = corners[nextNode].y; upper = corners[i].y;}
      testpoint = corners[head].y;
    }
    else if ((corners[head].y == corners[i].y) && (corners[i].y == corners[nextNode].y))
    {
      //which point is the higher  number?
      if (corners[i].x < corners[nextNode].x) {lower = corners[i].x; upper = corners[nextNode].x;}
      else {lower = corners[nextNode].x; upper = corners[i].x;}
      testpoint = corners[head].x;
    }
    else continue;
    
    //Now check to see if head is a point between this node and the next
    if ((lower<=testpoint) && (testpoint<= upper)) return 1;
    
    i = nextNode;
    nextNode = get_next_node(i);
  }
  return 0;
}

void snake_init(void)
{
    tail = 0;
    head = 1;
    dirY = 0;
    dirX = 1;
    change_dir = 0;
    snake_length_limit = 11;    //Change this to alter starting length
    snake_length_current = snake_length_limit;
    corners[head].x = 4+snake_length_current;
    corners[head].y = 4;
    corners[tail].x = 4;
    corners[tail].y = 4;

    for (uint8_t headX = corners[head].x; headX>=corners[tail].x; headX--) {
        gameSetMetaPixel(headX,corners[head].y,ON);
    }

    ++game_running;
    //TODO: implement RAND for fruit generation
    srand(TCNT0);
    make_fruit();
}

/*--------------------------------------------------------------------------
  FUNC: 7/11/12 - Gets index of next node in a ring buffer array
  PARAMS: Current index
  RETURNS: Next index (will go 'around the bend' if necessary)
  NOTE: Depends on the constant MAX_NODES which defines size of array
--------------------------------------------------------------------------*/
uint8_t get_next_node(uint8_t thisNode) {
  uint8_t nextNode = thisNode + 1;
  if (nextNode >= MAX_NODES) nextNode = 0;
  return nextNode;  
}

/*--------------------------------------------------------------------------
  FUNC: 7/11/12 - Gets index of previous node in a ring buffer array
  PARAMS: Current index
  RETURNS: Previous index (will go 'around the bend' if necessary)
  NOTE: Depends on the constant MAX_NODES which defines size of array
--------------------------------------------------------------------------*/
uint8_t get_previous_node(uint8_t thisNode) {
  if (thisNode) return thisNode-1;  // thisNode is not zero so just decrement
  return MAX_NODES-1; //thisNode is zero so go around the bend  
}

/*--------------------------------------------------------------------------
  FUNC: 7/11/12 - Finds length in ring buffer from one node to the next
  PARAMS: Index of first node, Index of second node
  RETURNS: Total number of nodes (inclusive)
  NOTE: Depends on the constant MAX_NODES which defines size of array
        will go 'around the bend' if necessary
--------------------------------------------------------------------------*/
uint8_t get_node_list_length(uint8_t node1, uint8_t node2) {
  if (node1 == node2) return 1;
  if (node1 < node2) return (node2-node1)+1;  //Adding 1 to adjust for 0 index
  else return node2+(MAX_NODES-node1)+1;    //Adding 1 to adjust for 0 index
}

void startGame(void) {
    oledClearScreen(1);
    if (game_running == 0) {
        gameInitBuffer();
        snake_init();
    }
}

void leaveGame(void) {
    game_running = 0;
    homeScreen();
}

void serviceGame(void) {
    move_head(change_dir);
    if (ate_fruit(corners[head].x,corners[head].y))
    {
      snake_length_limit += (snake_length_limit/10);
      make_fruit();
    }
    if (collision()) game_over();
    else {
        gameSetMetaPixel(corners[head].x, corners[head].y, ON); //Redraw
        if (snake_length_current > snake_length_limit)
        {
            gameSetMetaPixel(corners[tail].x, corners[tail].y, OFF); //Erase
            follow_tail();
        }
    }    
    move_tick = 0;
}
