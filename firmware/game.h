#define SNAKE_GIRTH   2
#define MAX_NODES     40
#define GAMEBOARD_X   64
#define GAMEBOARD_Y   32

uint8_t buffer[4][64];

extern void gameInitBuffer(void);
extern void gameSetMetaPixel(uint8_t x, uint8_t y, uint8_t status);

uint8_t game_running;
uint8_t tail;
uint8_t head;
uint8_t snake_length_limit;
uint8_t snake_length_current;

typedef struct {
  uint8_t x;
  uint8_t y;
} point;

point corners[MAX_NODES];
int8_t dirY;
int8_t dirX;
point fruit;
uint8_t change_dir;

//Variables
static volatile uint32_t timingDelay;
volatile uint8_t move_tick;

//Prototypes
void change_direction(void);
uint8_t absolute_difference(uint8_t a, uint8_t b);
uint8_t neighbors(point node1, point node2);
void make_fruit(void);
uint8_t ate_fruit(uint8_t x, uint8_t y);
void game_over(void);
void move_head(uint8_t new_dir);
void follow_tail(void);
uint8_t collision(void);
void snake_init(void);
uint8_t get_next_node(uint8_t thisNode);
uint8_t get_previous_node(uint8_t thisNode);
uint8_t get_node_list_length(uint8_t node1, uint8_t node2);
extern void serviceGame(void);
