#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>   // MPU6050 via I2C

TFT_eSPI tft = TFT_eSPI();

// PINOUT:
// MPU6050 (I2C) — connect to schermside ESP32:
//   VCC  → 3.3V
//   GND  → GND
//   SDA  → GPIO 21  (ESP32 default I2C SDA)
//   SCL  → GPIO 22  (ESP32 default I2C SCL)
//   AD0  → GND      (I2C address 0x68)
//   INT  → not used
//
// ILI9488 TFT (SPI) — pins configured via TFT_eSPI User_Setup.h:
//   Typical values (adjust to your User_Setup.h):
//   MOSI → GPIO 23
//   MISO → GPIO 19
//   SCK  → GPIO 18
//   CS   → GPIO 15
//   DC   → GPIO  2
//   RST  → GPIO  4
//   BL   → GPIO 32  (or directly to 3.3V for always on)
//
//   Touch (XPT2046 via SPI, shared):
//   T_CS → GPIO 14

// MPU6050 settings
#define MPU_ADDR  0x68
#define MPU_SDA   21
#define MPU_SCL   22


// ESP-NOW
uint8_t mac_servo[] = {0xec, 0xe3, 0x34, 0x99, 0xf9, 0xac};
#define BOARD_10 10

typedef struct struct_message {
  int id; int mode;
  uint8_t allAngles[16];
  float tempVal;
} struct_message;
struct_message myData;

struct PIDData {
  unsigned long timeMs;
  float avgTemp;
  float setPoint;
  float pwmPercent;
};
PIDData lastPIDData    = {0, NAN, 0.0f, 0.0f};
bool    pidDataReceived = false;

// STATES 
enum AppState {
  CALIBRATION, MENU,
  GRID_MODE, GRID2_MODE,
  TEMP_MODE, GAME_MODE,
  STROKE_MODE, STROKE2_MODE,
  VIBE_MODE, DRAW_MODE, DEPT_MODE, GRAV_MODE, SNAKE_MODE, BRICK_MODE, BALLOON_MODE, VOL_MODE
};
AppState currentState = CALIBRATION;

// COLORS
#define C_BG      TFT_BLACK
#define C_SURFACE 0x1082
#define C_PRIMARY 0x04FF
#define C_ACCENT  0xFD20
#define C_SUCCESS 0x07E0
#define C_DANGER  0xF800
#define C_TEXT    TFT_WHITE
#define C_SUBTEXT 0x7BEF
#define C_MENUBAR 0x2104
#define C_AAI2    0xF81F
#define C_TEMP    0xFC00
#define C_GRID2   0x07FF
#define C_DEPT    0xFFE0

// LAYOUT 
const int cols=4, rows=4;
const int MENU_H=36, SIDE_M=20, TOP_M=20, BOX_SIZE=28;

// BUTTON STRUCT 
struct Btn { int x,y,w,h; };
Btn btnGrid,btnTemp,btnGame,btnAai,btnAai2,btnTril;
Btn btnGrid2,btnDraw,btnDept;
Btn btnTempMin,btnTempPlus;
Btn btnStrokeMin,btnStrokePlus;
Btn btnStroke2Min,btnStroke2Plus;
Btn btnVibePrev,btnVibeNext,btnVibeTest;
Btn btnDeptMinus,btnDeptPlus;
Btn btnGrav;
Btn btnSnake;
Btn btnBrick;
Btn btnBalloon;
Btn btnVol;
Btn btnVolMinus, btnVolPlus;

// VIBRATION MAP 
const int8_t TRIL_MAP[16] = {
   8, 9, 10, 11, 12, 13, 14, 15, 7, -1, 5, 4, 3, 2, 1, 0,
};

// DEPTH SCALE
float depthScale = 1.0f;

// BUZZER
#define BUZZER_PIN     26   // GPIO26 — passive buzzer (change if needed)
#define BUZZER_CHANNEL  0   // LEDC channel

int buzzerVolume = 7;       // 0=mute, 1–10

#define NOTE_REST  0
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880

struct BuzzerNote { uint16_t freq; uint16_t ms; };

// "Serpentine" — Snake (E minor, ~150 BPM)
const BuzzerNote MELODY_SNAKE[] = {
  {NOTE_E4,200},{NOTE_G4,200},{NOTE_A4,200},{NOTE_B4,400},
  {NOTE_A4,200},{NOTE_G4,200},{NOTE_E4,200},{NOTE_D4,400},
  {NOTE_E4,200},{NOTE_G4,200},{NOTE_A4,200},{NOTE_C5,400},
  {NOTE_B4,200},{NOTE_A4,200},{NOTE_G4,200},{NOTE_E4,400},
  {NOTE_G4,200},{NOTE_A4,200},{NOTE_B4,200},{NOTE_E5,400},
  {NOTE_D5,200},{NOTE_B4,200},{NOTE_A4,200},{NOTE_G4,400},
  {NOTE_E4,200},{NOTE_D4,200},{NOTE_E4,200},{NOTE_G4,200},
  {NOTE_A4,200},{NOTE_B4,400},{NOTE_REST,200},
};
#define MELODY_SNAKE_LEN (sizeof(MELODY_SNAKE)/sizeof(MELODY_SNAKE[0]))

// Tetris (Korobeiniki) — Brick game
const BuzzerNote MELODY_TETRIS[] = {
  {NOTE_E5,400},{NOTE_B4,200},{NOTE_C5,200},{NOTE_D5,400},{NOTE_C5,200},{NOTE_B4,200},
  {NOTE_A4,400},{NOTE_A4,200},{NOTE_C5,200},{NOTE_E5,400},{NOTE_D5,200},{NOTE_C5,200},
  {NOTE_B4,600},{NOTE_C5,200},{NOTE_D5,400},{NOTE_E5,400},
  {NOTE_C5,400},{NOTE_A4,400},{NOTE_A4,400},{NOTE_REST,200},
  {NOTE_D5,400},{NOTE_F5,200},{NOTE_A5,400},{NOTE_G5,200},{NOTE_F5,200},
  {NOTE_E5,600},{NOTE_C5,200},{NOTE_E5,400},{NOTE_D5,200},{NOTE_C5,200},
  {NOTE_B4,400},{NOTE_B4,200},{NOTE_C5,200},{NOTE_D5,400},{NOTE_E5,400},
  {NOTE_C5,400},{NOTE_A4,400},{NOTE_A4,400},{NOTE_REST,200},
};
#define MELODY_TETRIS_LEN (sizeof(MELODY_TETRIS)/sizeof(MELODY_TETRIS[0]))

// "Cloud Pop" — Balloon game (A minor, serene)
const BuzzerNote MELODY_BALLOON[] = {
  {NOTE_A4,400},{NOTE_C5,400},{NOTE_E5,400},{NOTE_A5,600},
  {NOTE_G5,300},{NOTE_E5,300},{NOTE_D5,300},{NOTE_C5,600},
  {NOTE_B4,300},{NOTE_D5,300},{NOTE_F5,300},{NOTE_E5,600},
  {NOTE_C5,400},{NOTE_A4,400},{NOTE_E4,400},{NOTE_A4,600},{NOTE_REST,200},
};
#define MELODY_BALLOON_LEN (sizeof(MELODY_BALLOON)/sizeof(MELODY_BALLOON[0]))

// Game Over jingle — plays once (descending)
const BuzzerNote MELODY_GAMEOVER[] = {
  {NOTE_E5,200},{NOTE_D5,200},{NOTE_C5,200},
  {NOTE_B4,200},{NOTE_A4,200},{NOTE_G4,200},
  {NOTE_E4,600},{NOTE_REST,300},
};
#define MELODY_GAMEOVER_LEN (sizeof(MELODY_GAMEOVER)/sizeof(MELODY_GAMEOVER[0]))

const BuzzerNote* buzzerMelody  = nullptr;
int               buzzerLen     = 0;
int               buzzerIdx     = 0;
unsigned long     buzzerNextAt  = 0;
bool              buzzerPlaying = false;
bool              buzzerDoLoop  = true;


// === MPU6050 / GRAVITY ===
float filtAx  = 0.0f;   // low-pass filtered X-axis acceleration
float filtAy  = 0.0f;   // low-pass filtered Y-axis acceleration
float gravPitch = 0.0f; // no longer used (accel-only)
float gravRoll  = 0.0f; // no longer used (accel-only)
unsigned long gravLastTime   = 0;
unsigned long gravLastUpdate = 0;
#define GRAV_UPDATE_MS  50    // servo update interval in grav mode
// GRAV_ALPHA no longer used — accel-only filter via LP=0.12 in gravLoop
bool mpuOK = false;




// === BALLOON POP GAME ===
#define BAL_MAX          12     // max balloons at once
#define BAL_SPAWN_START 1600    // ms between spawns (start)
#define BAL_SPAWN_MIN    450    // fastest spawn interval
#define BAL_LIVES          3    // lives
#define BAL_GOLDEN_CHANCE 12   // % chance of golden balloon
#define BAL_BOMB_CHANCE   20   // % chance of bomb balloon

enum BalType { BAL_NORMAL=0, BAL_GOLDEN=1, BAL_BOMB=2 };

struct Balloon {
  float   x, y;       // position
  float   vy;         // vertical speed (negative = upward)
  float   r;          // radius
  uint8_t type;       // BAL_NORMAL / BAL_GOLDEN / BAL_BOMB
  uint16_t color;
  bool    alive;
  bool    bursting;
  float   burstR;     // burst animatie straal
};

const uint16_t BAL_NORMAL_COLORS[] = {
  0x07FF, 0xF81F, 0x07E0, 0xFC00, 0x04FF, 0xFFE0
};
#define BAL_N_COLORS 6

Balloon  balloons[BAL_MAX];
int      balScore      = 0;
int      balLives      = BAL_LIVES;
bool     balDead       = false;
unsigned long balLastSpawn  = 0;
unsigned long balSpawnInt   = BAL_SPAWN_START;
unsigned long balLastUpdate = 0;
#define BAL_UPDATE_MS 20

// === BRICK BREAKER ===
// Screen 480x284 (320-36 menubar)
// 12 bricks * 40px = 480px exact width
#define BRICK_COLS          12
#define BRICK_ROWS           4
#define BRICK_W             38    // width per brick
#define BRICK_H             13    // height per brick
#define BRICK_GAP            2    // gap between bricks
#define BRICK_TOP_Y         18    // y-start bricks
#define BRICK_PLAT_W        72    // platform width
#define BRICK_PLAT_H        10    // platform height
#define BRICK_BALL_R         5    // ball radius
#define BRICK_UPDATE_MS     16    // ~60fps
#define BRICK_PLAT_SPEED   9.1f   // pixels/frame at max tilt (6.5 * 1.4)
#define BRICK_BALL_SPEED_X 2.2f   // ball speed X (pixels/frame) — fixed
#define BRICK_BALL_SPEED_Y 2.8f   // ball speed Y (pixels/frame) — fixed

// Color per row (red → orange → yellow → green)
const uint16_t BRICK_ROW_COLORS[BRICK_ROWS] = {
  0xF800,  // red
  0xFD20,  // orange
  0xFFE0,  // yellow
  0x07E0,  // green
};

bool  brickAlive[BRICK_ROWS][BRICK_COLS];
float brickBallX, brickBallY;    // ball position (float for precision)
float brickBallDX, brickBallDY;  // ball velocity
float brickPlatX;                // platform left x
int   brickPlatY;                // platform y (fixed height)
int   brickLeft;                 // remaining bricks
int   brickScore;
bool  brickDead, brickWon;
unsigned long brickLastUpdate;

// === SNAKE GAME ===
#define SNAKE_CELL       20      // pixels per grid cell
#define SNAKE_COLS       24      // 24 * 20 = 480px
#define SNAKE_ROWS       13      // 13 * 20 = 260px (row 14 reserved for score)
#define SNAKE_SPEED_MS   220     // ms per step (start)
#define SNAKE_SPEED_MIN   60     // fastest speed — adjustable here
#define SNAKE_SPEED_STEP   8     // ms faster per eaten piece
#define SNAKE_DIR_THR   0.12f    // IMU tilt threshold for direction

enum SnakeDir { SDIR_RIGHT=0, SDIR_LEFT=1, SDIR_UP=2, SDIR_DOWN=3 };
struct SnakePos { int8_t x; int8_t y; };

SnakePos snakeBody[SNAKE_COLS * SNAKE_ROWS];
int      snakeLen        = 0;
SnakeDir snakeDir        = SDIR_RIGHT;
SnakePos snakeFood       = {0, 0};
int      snakeScore      = 0;
bool     snakeDead       = false;
int      snakeOffX       = 0;
int      snakeOffY       = 0;
unsigned long snakeLastMove  = 0;
unsigned long snakeCurSpeed  = SNAKE_SPEED_MS;
// Obstacles
#define SNAKE_OBS_INTERVAL_FIRST   5000  // first obstacle after 5s
#define SNAKE_OBS_INTERVAL_SECOND 10000  // second obstacle after 10s
#define SNAKE_OBS_INTERVAL        15000  // then every 15s
#define SNAKE_OBS_COLOR     0x000F  // dark blue (RGB565)
#define SNAKE_OBS_MAX       60      // max obstacles (each up to 3 cells = 180 cells)
#define SNAKE_OBS_RESET_THR (SNAKE_COLS * SNAKE_ROWS / 3)  // 1/3 grid = 104 cells

struct SnakeObs { int8_t x; int8_t y; };
SnakeObs snakeObs[SNAKE_OBS_MAX * 3];  // each obstacle up to 3 cells
int      snakeObsCount   = 0;          // total obstacle cell count
unsigned long snakeLastObs = 0;        // timestamp of last obstacle


SnakeDir snakeDirQueue[2]    = {SDIR_RIGHT, SDIR_RIGHT};
int      snakeDirQueueLen    = 0;

// === VARIABLES ===
float currentTemp  = 25.0f;
int   currentMole  = -1;
unsigned long lastMoleTime = 0;
int   gameSpeed=1500; bool moleIsUp=false; int gameScore=0;

int   strokeSpeedLevel = 5;
float strokeY = 0.0f;
unsigned long lastStrokeTime = 0;

int   aai2SpeedLevel = 5;
float aai2Pos = 0.0f;
unsigned long lastAai2Time = 0;

#define STROKE_INTERVAL_MS 18

#define AAI2_STEPS 12
const uint8_t AAI2_SEQ[AAI2_STEPS][2] = {
  { 1, 0},{ 5, 4},{ 9, 8},{13,12},
  {14,13},{10, 9},{ 6, 5},{ 2, 1},
  { 3, 2},{ 7, 6},{11,10},{15,14},
};

int  vibeSelectedMotor = 0;
uint16_t calibrationData[5];
unsigned long backButtonTimer=0; bool isHoldingBack=false;
unsigned long backReleasedAt=0;
unsigned long lastTempRefresh=0;
#define TEMP_REFRESH_MS 1000

// === DRAW MODE ===
#define DRAW_MAX_WP   200
#define DRAW_SPACING  8
#define DRAW_VISIT_PX 10
#define DRAW_DONE_PCT 99

struct DrawWP { int16_t x, y; bool visited; };
DrawWP  drawWP[DRAW_MAX_WP];
int     drawNWP       = 0;
int     drawVisited   = 0;
uint8_t drawShapeType = 255;
int16_t lastDrawX = -1, lastDrawY = -1;
unsigned long drawNextAt = 0;

// HELPERS:

bool btnHit(Btn b, int tx, int ty) {
  return tx>=b.x && tx<=b.x+b.w && ty>=b.y && ty<=b.y+b.h;
}
void getServoPos(int col, int row, int &x, int &y) {
  x = SIDE_M + col*((tft.width()-2*SIDE_M)/3);
  y = TOP_M  + row*((tft.height()-MENU_H-2*TOP_M)/3);
}
void drawTitle(const char* t) {
  tft.fillRect(0,0,tft.width(),36,C_SURFACE);
  tft.drawFastHLine(0,36,tft.width(),C_PRIMARY);
  tft.setTextColor(C_TEXT,C_SURFACE);
  tft.drawCentreString(t,tft.width()/2,10,2);
}
void drawMenuBar(const char* l) {
  int y=tft.height()-MENU_H;
  tft.fillRect(0,y,tft.width(),MENU_H,C_MENUBAR);
  tft.drawFastHLine(0,y,tft.width(),C_SURFACE);
  tft.setTextColor(C_SUBTEXT,C_MENUBAR);
  tft.drawCentreString(l,tft.width()/2,y+MENU_H/2-6,2);
}
void drawBtn(Btn &b, int x, int y, int w, int h,
             uint16_t col, const char* label,
             uint16_t textCol=TFT_WHITE, int fs=2) {
  b={x,y,w,h};
  tft.fillRoundRect(x,y,w,h,6,C_BG);
  tft.fillRoundRect(x+1,y+1,w-2,h-2,5,col);
  tft.setTextColor(textCol,col);
  tft.drawCentreString(label,x+w/2,y+h/2-7*fs/2,fs);
}
void drawValueBox(int cx, int cy, int w, int h, String val, uint16_t accent) {
  tft.fillRoundRect(cx-w/2,cy-h/2,w,h,8,C_SURFACE);
  tft.drawRoundRect(cx-w/2,cy-h/2,w,h,8,accent);
  tft.setTextColor(accent,C_SURFACE);
  tft.drawCentreString(val,cx,cy-8,4);
}

void sendHapticStop() {
  for(int i=0;i<16;i++) myData.allAngles[i]=0;
  myData.id=BOARD_10; myData.mode=5; myData.tempVal=0;
  esp_now_send(mac_servo,(uint8_t*)&myData,sizeof(myData));
  delay(10);
  esp_now_send(mac_servo,(uint8_t*)&myData,sizeof(myData));
  delay(10);
  esp_now_send(mac_servo,(uint8_t*)&myData,sizeof(myData));
}

// DRAW MODE HELPERS:
 
void addWP(int x, int y) {
  if (drawNWP < DRAW_MAX_WP)
    drawWP[drawNWP++] = {(int16_t)x,(int16_t)y,false};
}
void addSegWP(float x0,float y0,float x1,float y1){
  float dx=x1-x0,dy=y1-y0,len=sqrtf(dx*dx+dy*dy);
  int n=max(1,(int)(len/DRAW_SPACING));
  for(int i=0;i<=n&&drawNWP<DRAW_MAX_WP;i++){
    float t=(float)i/n;
    addWP((int)(x0+t*dx),(int)(y0+t*dy));
  }
}
void addArcWP(float cx,float cy,float rx,float ry,float a0,float a1){
  float span=a1-a0;
  float arcLen=fabsf(span)*(rx+ry)/2.0f;
  int n=max(3,(int)(arcLen/DRAW_SPACING));
  bool full=(fabsf(fabsf(span)-2.0f*PI)<0.05f);
  int lim=full?n-1:n;
  for(int i=0;i<=lim&&drawNWP<DRAW_MAX_WP;i++){
    float a=a0+span*((float)i/n);
    addWP((int)(cx+rx*cosf(a)),(int)(cy+ry*sinf(a)));
  }
}
void drawShapeDots(uint16_t color){
  for(int i=0;i<drawNWP;i+=2)
    tft.fillCircle(drawWP[i].x,drawWP[i].y,3,color);
}

// FORWARD DECLARATIONS:

void goToMenu();
void startGridMode(); void drawGridInterface(); void calculateGradient(int x,int y);
void startGrid2Mode(); void drawGrid2Interface(); void calculateGrid2(int x,int y);
void startTempMode(); void drawTempScreen(); void drawTempStatic();
void startGameMode(); void gameLoop(); void spawnMole();
void startStrokeMode(); void strokeLoop();
void startAai2Mode(); void aai2Loop();
void startVibeMode(); void drawVibeMenu();
void startDrawMode(); void nextDrawShape(); void genDrawShape(uint8_t type);
void startDeptMode(); void drawDeptScreen();
void startGravMode(); void gravLoop(); void drawGravDisplay();
void startBrickMode(); void brickLoop();
void startBalloonMode(); void balloonLoop();
void balSpawnOne(); void balDrawOne(int i, uint16_t over);
void balDrawLives(); void balDrawScore(); void balPop(int i, bool bomb);
void brickDrawPlatform(uint16_t color); void brickDrawBall(uint16_t color);
void brickDrawAllBricks(); void brickCheckCollisions(); void brickServoUpdate(); void brickHapticUpdate();
void startSnakeMode(); void snakeLoop(); void snakeMove(); void snakeSpawnFood();
void snakeDrawCell(int gx, int gy, uint16_t color); void snakeDrawFood(); void snakeDrawScore(); void snakeUpdateDir(); void snakeDie();
void snakeSpawnObstacle(); void snakeClearObstacles(); bool snakeIsObs(int x, int y); void snakeDrawAllObs(); void initMPU6050(); void readMPU6050raw(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz);
void sendDrawHaptic(int touchX, int touchY);
void sendHapticStop();
void sendToServo(int mode, float temp);
bool checkBackButton(int tx,int ty);
void buzzerStart(const BuzzerNote* melody, int len, bool loop);
void buzzerStop();
void buzzerUpdate();
void startVolMode(); void drawVolScreen();
void snakeHapticAt(int gx, int gy);

// ESP-NOW CALLBACK:

void OnDataRecv(const esp_now_recv_info_t *info,const uint8_t *data,int len){
  if(len==sizeof(PIDData)){
    memcpy(&lastPIDData,data,sizeof(PIDData));
    pidDataReceived=true;
    lastTempRefresh=0;
  }
}

// SETUP:

void setup(){
  Serial.begin(115200);
  tft.init(); tft.setRotation(1); tft.fillScreen(C_BG);

  tft.fillRect(0,0,tft.width(),40,C_SURFACE);
  tft.setTextColor(C_TEXT,C_SURFACE);
  tft.drawCentreString("CALIBRATION",tft.width()/2,12,2);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Touch the crosshairs",tft.width()/2,60,2);
  tft.calibrateTouch(calibrationData,TFT_WHITE,C_DANGER,15);

  WiFi.mode(WIFI_STA);
  if(esp_now_init()!=ESP_OK) return;
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peer; memset(&peer,0,sizeof(peer));
  memcpy(peer.peer_addr,mac_servo,6);
  peer.channel=0; peer.encrypt=false;
  esp_now_add_peer(&peer);

  // MPU6050 initialization
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.setClock(400000);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00); // wake up
  if (Wire.endTransmission() == 0) {
    // Gyro full scale ±250°/s, Accel full scale ±2g
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x1B); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x1C); Wire.write(0x00); Wire.endTransmission();
    mpuOK = true;
    gravLastTime = millis();
    Serial.println("MPU6050 OK");
  } else {
    Serial.println("MPU6050 NOT FOUND — check SDA/SCL wiring");
  }

  ledcAttach(BUZZER_PIN, 1000, 8);
  ledcWrite(BUZZER_PIN, 0);

  goToMenu();
}

// LOOP:

void loop(){
  buzzerUpdate();
  if(currentState==STROKE_MODE)  strokeLoop();
  if(currentState==STROKE2_MODE) aai2Loop();
  if(currentState==GAME_MODE)    gameLoop();
  if(currentState==GRAV_MODE)    gravLoop();
  if(currentState==SNAKE_MODE)   snakeLoop();
  if(currentState==BRICK_MODE)   brickLoop();
  if(currentState==BALLOON_MODE) balloonLoop();

  if(currentState==TEMP_MODE &&
     millis()-lastTempRefresh>TEMP_REFRESH_MS && pidDataReceived){
    lastTempRefresh=millis(); drawTempScreen();
  }

  uint16_t tx,ty;
  bool touched=tft.getTouch(&tx,&ty);

  if(touched){
    if(currentState!=MENU && currentState!=CALIBRATION)
      if(checkBackButton(tx,ty)) return;

    switch(currentState){

      case MENU:
        if      (btnHit(btnGrid, tx,ty)) startGridMode();
        else if (btnHit(btnGrid2,tx,ty)) startGrid2Mode();
        else if (btnHit(btnTemp, tx,ty)) startTempMode();
        else if (btnHit(btnGame, tx,ty)) startGameMode();
        else if (btnHit(btnAai,  tx,ty)) startStrokeMode();
        else if (btnHit(btnAai2, tx,ty)) startAai2Mode();
        else if (btnHit(btnTril, tx,ty)) startVibeMode();
        else if (btnHit(btnDraw, tx,ty)) startDrawMode();
        else if (btnHit(btnDept, tx,ty)) startDeptMode();
        else if (btnHit(btnGrav,  tx,ty)) startGravMode();
        else if (btnHit(btnSnake, tx,ty)) startSnakeMode();
        else if (btnHit(btnBrick,   tx,ty)) startBrickMode();
        else if (btnHit(btnBalloon,tx,ty)) startBalloonMode();
        else if (btnHit(btnVol,    tx,ty)) startVolMode();
        break;

      case GRID_MODE:
        if(ty<(uint16_t)(tft.height()-MENU_H)) calculateGradient(tx,ty);
        break;

      case GRID2_MODE:
        if(ty<(uint16_t)(tft.height()-MENU_H)) calculateGrid2(tx,ty);
        break;

      case TEMP_MODE:
        if(btnHit(btnTempMin,tx,ty)){
          currentTemp=max(15.0f,currentTemp-0.5f);
          sendToServo(1,currentTemp); drawTempScreen(); delay(120);
        } else if(btnHit(btnTempPlus,tx,ty)){
          currentTemp=min(45.0f,currentTemp+0.5f);
          sendToServo(1,currentTemp); drawTempScreen(); delay(120);
        }
        break;

      case STROKE_MODE:
        if(btnHit(btnStrokeMin,tx,ty)&&strokeSpeedLevel>1){
          strokeSpeedLevel--;
          drawValueBox(tft.width()/2,130,120,56,String(strokeSpeedLevel),C_PRIMARY); delay(150);
        } else if(btnHit(btnStrokePlus,tx,ty)&&strokeSpeedLevel<10){
          strokeSpeedLevel++;
          drawValueBox(tft.width()/2,130,120,56,String(strokeSpeedLevel),C_PRIMARY); delay(150);
        }
        break;

      case STROKE2_MODE:
        if(btnHit(btnStroke2Min,tx,ty)&&aai2SpeedLevel>1){
          aai2SpeedLevel--;
          drawValueBox(tft.width()/2,130,120,56,String(aai2SpeedLevel),C_AAI2); delay(150);
        } else if(btnHit(btnStroke2Plus,tx,ty)&&aai2SpeedLevel<10){
          aai2SpeedLevel++;
          drawValueBox(tft.width()/2,130,120,56,String(aai2SpeedLevel),C_AAI2); delay(150);
        }
        break;

      case GAME_MODE:
        if(ty<(uint16_t)(tft.height()-MENU_H)&&moleIsUp){
          int mx,my; getServoPos(currentMole%cols,currentMole/cols,mx,my);
          if(abs((int)tx-mx)<40&&abs((int)ty-my)<40){
            gameScore++;
            for(int i=0;i<16;i++) myData.allAngles[i]=0;
            sendToServo(2,0);
            tft.fillCircle(mx,my,(BOX_SIZE/2)+5,C_SUCCESS);
            moleIsUp=false; lastMoleTime=millis()-gameSpeed+200; delay(50);
          }
        }
        break;

      case VIBE_MODE:
        if(btnHit(btnVibePrev,tx,ty)){
          if(vibeSelectedMotor>0) vibeSelectedMotor--;
          drawVibeMenu(); delay(150);
        } else if(btnHit(btnVibeNext,tx,ty)){
          if(vibeSelectedMotor<15) vibeSelectedMotor++;
          drawVibeMenu(); delay(150);
        } else if(btnHit(btnVibeTest,tx,ty)){
          tft.fillRoundRect(btnVibeTest.x,btnVibeTest.y,btnVibeTest.w,btnVibeTest.h,8,TFT_WHITE);
          delay(80);
          for(int i=0;i<16;i++) myData.allAngles[i]=0;
          myData.allAngles[vibeSelectedMotor]=1;
          sendToServo(4,0); delay(100); drawVibeMenu();
        }
        break;

      case DRAW_MODE:
        if(ty>=2 && ty<(uint16_t)(tft.height()-MENU_H)){
          if(lastDrawX>=0)
            tft.drawLine(lastDrawX,lastDrawY,(int)tx,(int)ty,TFT_YELLOW);
          tft.fillCircle((int)tx,(int)ty,3,TFT_YELLOW);
          lastDrawX=(int16_t)tx; lastDrawY=(int16_t)ty;
          int t2=DRAW_VISIT_PX*DRAW_VISIT_PX;
          for(int i=0;i<drawNWP;i++){
            if(!drawWP[i].visited){
              int ddx=drawWP[i].x-(int)tx;
              int ddy=drawWP[i].y-(int)ty;
              if(ddx*ddx+ddy*ddy<=t2){
                drawWP[i].visited=true; drawVisited++;
              }
            }
          }
          calculateGradient((int)tx,(int)ty);
          sendDrawHaptic((int)tx,(int)ty);

          if(drawNWP>0&&(drawVisited*100/drawNWP)>=DRAW_DONE_PCT
             &&millis()>=drawNextAt){
            drawNextAt=millis()+1500;
            tft.fillRect(0,0,tft.width(),tft.height()-MENU_H,C_SUCCESS);
            delay(350);
            nextDrawShape();
          }
        }
        break;

      case DEPT_MODE:
        if(btnHit(btnDeptMinus,tx,ty)){
          depthScale=max(0.0f, depthScale-0.05f);
          drawDeptScreen(); delay(120);
        } else if(btnHit(btnDeptPlus,tx,ty)){
          depthScale=min(1.0f, depthScale+0.05f);
          drawDeptScreen(); delay(120);
        } else if(ty>90 && ty<180){
          int sliderX=30, sliderW=tft.width()-60;
          float newScale=(float)((int)tx - sliderX)/(float)sliderW;
          depthScale=constrain(newScale,0.0f,1.0f);
          drawDeptScreen(); delay(30);
        }
        break;

      case GRAV_MODE:
        // No touch interaction — gravity controls servos
        break;

      case SNAKE_MODE:
        if (snakeDead) startSnakeMode();
        break;

      case BRICK_MODE:
        if (brickDead || brickWon) startBrickMode();
        break;

      case VOL_MODE:
        if(btnHit(btnVolMinus,tx,ty) && buzzerVolume>0){
          buzzerVolume--; drawVolScreen(); delay(120);
        } else if(btnHit(btnVolPlus,tx,ty) && buzzerVolume<10){
          buzzerVolume++; drawVolScreen(); delay(120);
        }
        break;

      case BALLOON_MODE:
        if (balDead) { startBalloonMode(); break; }
        // Check which balloon was tapped
        for (int i = 0; i < BAL_MAX; i++) {
          if (!balloons[i].alive || balloons[i].bursting) continue;
          float dx = (int)tx - balloons[i].x;
          float dy = (int)ty - balloons[i].y;
          if (sqrtf(dx*dx+dy*dy) <= balloons[i].r + 8) {
            balPop(i, balloons[i].type == BAL_BOMB);
            break;
          }
        }
        break;

      default: break;
    }

  } else {
    if(currentState==GRID_MODE){
      for(int i=0;i<16;i++) myData.allAngles[i]=0;
      sendToServo(0,0);
    }
    if(currentState==GRID2_MODE){
      sendHapticStop();
    }
    if(currentState==DRAW_MODE){
      lastDrawX=-1; lastDrawY=-1;
      for(int i=0;i<16;i++) myData.allAngles[i]=0;
      sendToServo(0,0);
      sendHapticStop();
    }
    if(isHoldingBack){
      if(backReleasedAt==0) backReleasedAt=millis();
      if(millis()-backReleasedAt > 200){
        isHoldingBack=false; backButtonTimer=0; backReleasedAt=0;
        drawMenuBar("HOLD 2 SEC FOR MENU");
      }
    }
  }
}


// DEPT MODE:

void drawDeptScreen(){
  int W=tft.width();
  tft.fillRect(0,38,W,tft.height()-38-MENU_H,C_BG);
  int pct=(int)roundf(depthScale*100.0f);
  char buf[8]; snprintf(buf,sizeof(buf),"%d%%",pct);
  tft.setTextColor(C_DEPT,C_BG);
  tft.drawCentreString(buf,W/2,55,4);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Maximum servo depth",W/2,100,2);
  int sx=30, sy=125, sw=W-60, sh=30;
  int filled=(int)(sw*depthScale);
  tft.fillRoundRect(sx,sy,sw,sh,8,C_SURFACE);
  if(filled>0) tft.fillRoundRect(sx,sy,filled,sh,8,C_DEPT);
  tft.drawRoundRect(sx,sy,sw,sh,8,C_SUBTEXT);
  int knobX=constrain(sx+filled,sx,sx+sw);
  tft.fillCircle(knobX,sy+sh/2,sh/2+4,C_DEPT);
  tft.fillCircle(knobX,sy+sh/2,sh/2,C_BG);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawString("0%",sx,sy+sh+8,1);
  tft.drawString("100%",sx+sw-24,sy+sh+8,1);
  int bh=50,bw=70,cy=195;
  drawBtn(btnDeptMinus,10,      cy,bw,bh,0x0339,"-",C_TEXT,4);
  drawBtn(btnDeptPlus, W-10-bw, cy,bw,bh,0x6200,"+",C_TEXT,4);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Step: 5%",W/2,cy+bh+6,1);
}

void startDeptMode(){
  currentState=DEPT_MODE;
  tft.fillScreen(C_BG);
  drawTitle("DEPTH");
  drawDeptScreen();
  drawMenuBar("HOLD 2 SEC FOR MENU");
}

// STROKE LOOPS:

void strokeLoop(){
  if(millis()-lastStrokeTime<STROKE_INTERVAL_MS) return;
  lastStrokeTime=millis();
  strokeY+=0.008f*(float)strokeSpeedLevel;
  if(strokeY>3.5f) strokeY=-0.5f;
  for(int r=0;r<rows;r++){
    float d=fabsf(strokeY-(float)r), intensity=0;
    if(d<1.6f){intensity=cosf(d*(PI/2.0f)/1.6f); if(intensity<0)intensity=0;}
    uint8_t h=(uint8_t)(90.0f*intensity);
    for(int c=0;c<cols;c++) myData.allAngles[r*cols+c]=h;
  }
  sendToServo(3,0);
}

void aai2Loop(){
  if(millis()-lastAai2Time<STROKE_INTERVAL_MS) return;
  lastAai2Time=millis();
  aai2Pos+=0.006f*(float)aai2SpeedLevel;
  if(aai2Pos>=(float)AAI2_STEPS) aai2Pos=0.0f;
  int sA=(int)aai2Pos%AAI2_STEPS, sB=(sA+1)%AAI2_STEPS;
  float t=aai2Pos-(float)(int)aai2Pos;
  uint8_t vA=(uint8_t)(90.0f*cosf(t*PI/2.0f));
  uint8_t vB=(uint8_t)(90.0f*sinf(t*PI/2.0f));
  for(int i=0;i<16;i++) myData.allAngles[i]=0;
  myData.allAngles[AAI2_SEQ[sA][0]]=vA; myData.allAngles[AAI2_SEQ[sA][1]]=vA;
  myData.allAngles[AAI2_SEQ[sB][0]]=max(myData.allAngles[AAI2_SEQ[sB][0]],vB);
  myData.allAngles[AAI2_SEQ[sB][1]]=max(myData.allAngles[AAI2_SEQ[sB][1]],vB);
  sendToServo(3,0);
}


// TEMP SCREEN:

void drawTempStatic(){
  int W=tft.width(), cy_sp=90;
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("SETPOINT",W/2,cy_sp-42,1);
  drawBtn(btnTempMin, 10,   cy_sp-30,70,60,0x0339,"-",C_TEXT,4);
  drawBtn(btnTempPlus,W-80, cy_sp-30,70,60,0x6200,"+",C_TEXT,4);
  tft.drawFastHLine(20,133,W-40,C_SURFACE);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("ACTUAL TEMP",W/2,147,1);
}

void drawTempScreen(){
  int W=tft.width(), cy_sp=90, cy_act=175;
  char spBuf[10]; snprintf(spBuf,sizeof(spBuf),"%.1f C",currentTemp);
  tft.fillRoundRect(W/2-75,cy_sp-26,150,52,8,C_SURFACE);
  tft.drawRoundRect(W/2-75,cy_sp-26,150,52,8,C_TEMP);
  tft.setTextColor(C_TEMP,C_SURFACE);
  tft.drawCentreString(spBuf,W/2,cy_sp-8,4);
  tft.fillRect(0,cy_act-10,W,46,C_BG);
  if(!pidDataReceived){
    tft.setTextColor(C_SUBTEXT,C_BG);
    tft.drawCentreString("...",W/2,cy_act,4);
  } else {
    char actBuf[12];
    if(isnan(lastPIDData.avgTemp)) snprintf(actBuf,sizeof(actBuf),"--- C");
    else snprintf(actBuf,sizeof(actBuf),"%.1f C",lastPIDData.avgTemp);
    uint16_t tc=C_SUBTEXT;
    if(!isnan(lastPIDData.avgTemp)){
      float d=fabsf(lastPIDData.avgTemp-currentTemp);
      if(d<1.0f)tc=C_SUCCESS; else if(d<3.0f)tc=C_ACCENT; else tc=C_DANGER;
    }
    tft.setTextColor(tc,C_BG);
    tft.drawCentreString(actBuf,W/2,cy_act,4);
  }
  int bx=20,by=cy_act+42,bw=W-40,bh=16;
  float pct=pidDataReceived?constrain(lastPIDData.pwmPercent,0.0f,100.0f):0.0f;
  tft.fillRoundRect(bx,by,bw,bh,4,C_SURFACE);
  if(pct>0) tft.fillRoundRect(bx,by,(int)(bw*pct/100.0f),bh,4,C_TEMP);
  tft.drawRoundRect(bx,by,bw,bh,4,C_SUBTEXT);
  tft.fillRect(bx,by+bh+2,bw,14,C_BG);
  char pwmBuf[12]; snprintf(pwmBuf,sizeof(pwmBuf),"PWM: %.0f%%",pct);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString(pwmBuf,W/2,by+bh+4,1);
}

void startTempMode(){
  currentState=TEMP_MODE;
  tft.fillScreen(C_BG); drawTitle("TEMPERATURE");
  drawTempStatic(); drawTempScreen();
  drawMenuBar("HOLD 2 SEC FOR MENU");
  sendToServo(1,currentTemp);
}

// MENU:

void goToMenu(){
  buzzerStop();
  currentState=MENU; tft.fillScreen(C_BG);
  tft.fillRect(0,0,tft.width(),42,C_SURFACE);
  tft.drawFastHLine(0,42,tft.width(),C_PRIMARY);
  tft.setTextColor(C_TEXT,C_SURFACE);
  tft.drawCentreString("MAIN MENU",tft.width()/2,13,2);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Choose a mode",tft.width()/2,50,1);

  int W=tft.width(), gap=8, bw=(W-4*gap)/3, bh=38, rowGap=8;

  int r1y=58;
  drawBtn(btnGrid,gap,       r1y,bw,bh,0x0339,"GRID",C_TEXT,2);
  drawBtn(btnTemp,gap*2+bw,  r1y,bw,bh,C_TEMP,"TEMP",C_BG, 2);
  drawBtn(btnGame,gap*3+bw*2,r1y,bw,bh,0x0620,"GAME",C_TEXT,2);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Servos",    btnGrid.x+bw/2,r1y+bh+2,1);
  tft.drawCentreString("Heating",   btnTemp.x+bw/2,r1y+bh+2,1);
  tft.drawCentreString("Mole game", btnGame.x+bw/2,r1y+bh+2,1);

  int r2y=r1y+bh+rowGap;
  drawBtn(btnAai, gap,        r2y,bw,bh,C_PRIMARY,"AAI", C_BG,2);
  drawBtn(btnAai2,gap*2+bw,   r2y,bw,bh,C_AAI2,  "AAI2",C_BG,2);
  drawBtn(btnTril,gap*3+bw*2, r2y,bw,bh,C_ACCENT, "TRIL",C_BG,2);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Wave",   btnAai.x+bw/2, r2y+bh+2,1);
  tft.drawCentreString("Coil",   btnAai2.x+bw/2,r2y+bh+2,1);
  tft.drawCentreString("Haptics",btnTril.x+bw/2,r2y+bh+2,1);

  int r3y=r2y+bh+rowGap;
  drawBtn(btnGrid2,gap,        r3y,bw,bh,C_GRID2,   "GRID2",C_BG,2);
  drawBtn(btnDraw, gap*2+bw,   r3y,bw,bh,TFT_WHITE, "DRAW", C_BG,2);
  drawBtn(btnDept, gap*3+bw*2, r3y,bw,bh,C_DEPT,    "DEPT", C_BG,2);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Haptic grid",btnGrid2.x+bw/2,r3y+bh+2,1);
  tft.drawCentreString("Draw",        btnDraw.x+bw/2, r3y+bh+2,1);
  tft.drawCentreString("Depth",       btnDept.x+bw/2, r3y+bh+2,1);

  // Row 4: GRAV, SNAKE, BRICK (3 per row)
  int r4y=r3y+bh+rowGap;
  uint16_t gravCol = mpuOK ? 0x07E0 : 0x4208;
  drawBtn(btnGrav,  gap,        r4y,bw,bh,gravCol, mpuOK?"GRAV":"GRAV(?)",C_BG,2);
  drawBtn(btnSnake, gap*2+bw,   r4y,bw,bh,0x3666,  "SNAKE",              C_BG,2);
  drawBtn(btnBrick, gap*3+bw*2, r4y,bw,bh,0x04FF,  "BRICK",              C_BG,2);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Gravity",     btnGrav.x+bw/2, r4y+bh+2,1);
  tft.drawCentreString("Snake",       btnSnake.x+bw/2,r4y+bh+2,1);
  tft.drawCentreString("Brick break", btnBrick.x+bw/2,r4y+bh+2,1);

  // Row 5: BALLOON + VOL
  int r5y=r4y+bh+rowGap;
  int bw5=(W-3*gap)/2;
  drawBtn(btnBalloon, gap,       r5y, bw5, bh, 0xF81F, "BALLOON", C_BG, 2);
  drawBtn(btnVol,     gap*2+bw5, r5y, bw5, bh, 0xFFE0, "VOL",     C_BG, 2);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Balloon game", btnBalloon.x+bw5/2, r5y+bh+2, 1);
  tft.drawCentreString("Volume",      btnVol.x+bw5/2,     r5y+bh+2, 1);
}

// GRID (servo):

void drawGridInterface(){
  tft.fillScreen(C_BG); drawTitle("GRID CONTROL");
  for(int r=0;r<rows;r++) for(int c=0;c<cols;c++){
    int cx,cy; getServoPos(c,r,cx,cy); int hs=BOX_SIZE/2;
    tft.fillRoundRect(cx-hs,cy-hs,BOX_SIZE,BOX_SIZE,4,C_SURFACE);
    tft.drawRoundRect(cx-hs,cy-hs,BOX_SIZE,BOX_SIZE,4,C_PRIMARY);
    tft.setTextColor(C_SUBTEXT,C_SURFACE);
    tft.drawCentreString(String(15-(r*cols+c)),cx,cy-5,1);
  }
  drawMenuBar("HOLD 2 SEC FOR MENU");
}
void startGridMode(){currentState=GRID_MODE; drawGridInterface();}

void calculateGradient(int touchX,int touchY){
  for(int i=0;i<16;i++) myData.allAngles[i]=0;
  int uw=tft.width()-2*SIDE_M, uh=(tft.height()-MENU_H)-2*TOP_M;
  float gx=(float)(touchX-SIDE_M)/(uw/3.0f), gy=(float)(touchY-TOP_M)/(uh/3.0f);
  if(gx<0)gx=0; if(gx>3)gx=3; if(gy<0)gy=0; if(gy>3)gy=3;
  if(fabsf(gx-roundf(gx))<0.12f&&fabsf(gy-roundf(gy))<0.12f){
    int mIdx=15-((int)roundf(gy)*cols+(int)roundf(gx));
    if(mIdx>=0&&mIdx<16) myData.allAngles[mIdx]=90;
    sendToServo(0,0); return;
  }
  int x1=(int)gx,y1=(int)gy,x2=x1+1,y2=y1+1;
  if(x2>3)x2=3; if(y2>3)y2=3;
  float dx=gx-x1, dy=gy-y1;
  dx=((int)(dx*20))/20.0f; dy=((int)(dy*20))/20.0f;
  int tl=15-(y1*cols+x1); if(tl>=0&&tl<16) myData.allAngles[tl]=(uint8_t)(90*(1-dx)*(1-dy));
  int tr=15-(y1*cols+x2); if(tr>=0&&tr<16) myData.allAngles[tr]=(uint8_t)(90*dx*(1-dy));
  int bl=15-(y2*cols+x1); if(bl>=0&&bl<16) myData.allAngles[bl]=(uint8_t)(90*(1-dx)*dy);
  int br=15-(y2*cols+x2); if(br>=0&&br<16) myData.allAngles[br]=(uint8_t)(90*dx*dy);
  sendToServo(0,0);
}

// GRID2 (haptic):

void drawGrid2Interface(){
  tft.fillScreen(C_BG); drawTitle("HAPTIC GRID");
  for(int r=0;r<rows;r++) for(int c=0;c<cols;c++){
    int cx,cy; getServoPos(c,r,cx,cy); int hs=BOX_SIZE/2;
    int8_t motor=TRIL_MAP[15-(r*cols+c)];
    uint16_t boxCol=(motor>=0)?C_GRID2:C_SURFACE;
    tft.fillRoundRect(cx-hs,cy-hs,BOX_SIZE,BOX_SIZE,4,boxCol);
    tft.drawRoundRect(cx-hs,cy-hs,BOX_SIZE,BOX_SIZE,4,C_TEXT);
    tft.setTextColor(C_BG,boxCol);
    if(motor>=0){char lbl[4]; snprintf(lbl,sizeof(lbl),"%d",motor+1); tft.drawCentreString(lbl,cx,cy-5,1);}
    else tft.drawCentreString("-",cx,cy-5,1);
  }
  drawMenuBar("HOLD 2 SEC FOR MENU");
}
void startGrid2Mode(){currentState=GRID2_MODE; drawGrid2Interface();}

void calculateGrid2(int touchX,int touchY){
  for(int i=0;i<16;i++) myData.allAngles[i]=0;
  int uw=tft.width()-2*SIDE_M, uh=(tft.height()-MENU_H)-2*TOP_M;
  float gx=(float)(touchX-SIDE_M)/(uw/3.0f), gy=(float)(touchY-TOP_M)/(uh/3.0f);
  if(gx<0)gx=0; if(gx>3)gx=3; if(gy<0)gy=0; if(gy>3)gy=3;
  int c=constrain((int)roundf(gx),0,3);
  int r=constrain((int)roundf(gy),0,3);
  int8_t motor=TRIL_MAP[15-(r*cols+c)];
  if(motor>=0&&motor<16) myData.allAngles[motor]=255;
  sendToServo(5,0);
}


// AAI / AAI2:

void startStrokeMode(){
  currentState=STROKE_MODE; strokeY=0.0f; lastStrokeTime=millis();
  tft.fillScreen(C_BG); drawTitle("STROKE - WAVE MOTION");
  int cy=130,bh=76,bw=80;
  drawBtn(btnStrokeMin, 10,               cy-bh/2,bw,bh,0x0339,"-",C_TEXT,4);
  drawBtn(btnStrokePlus,tft.width()-10-bw, cy-bh/2,bw,bh,0x6200,"+",C_TEXT,4);
  drawValueBox(tft.width()/2,cy,120,56,String(strokeSpeedLevel),C_PRIMARY);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Speed (1-10)",tft.width()/2,cy+50,1);
  drawMenuBar("HOLD 2 SEC FOR MENU");
}

void startAai2Mode(){
  currentState=STROKE2_MODE; aai2Pos=0.0f; lastAai2Time=millis();
  tft.fillScreen(C_BG); drawTitle("STROKE2 - COIL");
  int cy=130,bh=76,bw=80;
  drawBtn(btnStroke2Min, 10,               cy-bh/2,bw,bh,0x0339,"-",C_TEXT,4);
  drawBtn(btnStroke2Plus,tft.width()-10-bw, cy-bh/2,bw,bh,0x6200,"+",C_TEXT,4);
  drawValueBox(tft.width()/2,cy,120,56,String(aai2SpeedLevel),C_AAI2);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Speed (1-10)",tft.width()/2,cy+50,1);
  drawMenuBar("HOLD 2 SEC FOR MENU");
}

// VIBE:

void drawVibeMenu(){
  tft.fillRect(0,42,tft.width(),tft.height()-42-MENU_H,C_BG);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Select motor and press TEST",tft.width()/2,55,1);
  int W=tft.width(),cy=130,tbw=100,tbh=76,abw=64,abh=76;
  int sx=(W-(abw+10+tbw+10+abw))/2;
  drawBtn(btnVibePrev,sx,cy-abh/2,abw,abh,C_SURFACE,"<",C_TEXT,4);
  char ml[6]; snprintf(ml,sizeof(ml),"M%d",vibeSelectedMotor+1);
  drawBtn(btnVibeTest,sx+abw+10,cy-tbh/2,tbw,tbh,C_ACCENT,ml,C_BG,3);
  tft.setTextColor(C_BG,C_ACCENT);
  tft.drawCentreString("TEST",btnVibeTest.x+tbw/2,btnVibeTest.y+tbh-16,1);
  drawBtn(btnVibeNext,sx+abw+10+tbw+10,cy-abh/2,abw,abh,C_SURFACE,">",C_TEXT,4);
  tft.setTextColor(C_SUBTEXT,C_BG);
  char mf[20]; snprintf(mf,sizeof(mf),"Motor %d / 16",vibeSelectedMotor+1);
  tft.drawCentreString(mf,W/2,cy+abh/2+12,2);
}
void startVibeMode(){
  currentState=VIBE_MODE;
  tft.fillScreen(C_BG); drawTitle("VIBRATION MOTORS TEST");
  drawVibeMenu(); drawMenuBar("HOLD 2 SEC FOR MENU");
}

// GAME:

void startGameMode(){
  currentState=GAME_MODE; gameScore=0; gameSpeed=1500;
  currentMole=-1; moleIsUp=false; lastMoleTime=millis();
  tft.fillScreen(C_BG); drawTitle("MOLE GAME");
  for(int r=0;r<rows;r++) for(int c=0;c<cols;c++){
    int cx,cy; getServoPos(c,r,cx,cy);
    tft.drawCircle(cx,cy,(BOX_SIZE/2)+5,C_SURFACE);
  }
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Score: 0",tft.width()/2,tft.height()-MENU_H-18,1);
  drawMenuBar("HOLD 2 SEC FOR MENU");
}


// DRAW MODE:

void genDrawShape(uint8_t type){
  drawNWP=0; drawVisited=0;
  int W=tft.width(), H=tft.height();
  int x1=18, x2=W-18, y1=4, y2=H-MENU_H-6;
  int cx=(x1+x2)/2, cy=(y1+y2)/2;
  int dw=x2-x1, dh=y2-y1;
  switch(type){
    case 0:{
      float r=min(dw,dh)*0.40f;
      addArcWP(cx,cy,r,r,0,2.0f*PI); break;
    }
    case 1:{
      float rw=dw*0.40f,rh=dh*0.38f,cr=min(rw,rh)*0.18f;
      addSegWP(cx-rw+cr,cy-rh,cx+rw-cr,cy-rh);
      addArcWP(cx+rw-cr,cy-rh+cr,cr,cr,-PI/2.0f,0.0f);
      addSegWP(cx+rw,cy-rh+cr,cx+rw,cy+rh-cr);
      addArcWP(cx+rw-cr,cy+rh-cr,cr,cr,0.0f,PI/2.0f);
      addSegWP(cx+rw-cr,cy+rh,cx-rw+cr,cy+rh);
      addArcWP(cx-rw+cr,cy+rh-cr,cr,cr,PI/2.0f,PI);
      addSegWP(cx-rw,cy+rh-cr,cx-rw,cy-rh+cr);
      addArcWP(cx-rw+cr,cy-rh+cr,cr,cr,PI,3.0f*PI/2.0f);
      break;
    }
    case 2:{
      float bw=dw*0.44f,bh=dh*0.42f;
      addSegWP(cx,cy-bh,cx+bw,cy+bh);
      addSegWP(cx+bw,cy+bh,cx-bw,cy+bh);
      addSegWP(cx-bw,cy+bh,cx,cy-bh);
      break;
    }
    case 3:{
      float sclX=dw*0.82f/32.0f, sclY=dh*0.58f/22.0f;
      float yOffset=3.0f*sclY;
      for(int i=0;i<=80&&drawNWP<DRAW_MAX_WP;i++){
        float t=2.0f*PI*i/80.0f;
        float hx=16.0f*powf(sinf(t),3.0f);
        float hy=-(13.0f*cosf(t)-5.0f*cosf(2*t)-2.0f*cosf(3*t)-cosf(4*t));
        addWP(cx+(int)(hx*sclX),cy+(int)(hy*sclY)-(int)yOffset);
      }
      break;
    }
    case 4:{
      float rx=dw*0.42f,ry=dh*0.36f;
      addArcWP(cx,cy,rx,ry,0,2.0f*PI);
      break;
    }
  }
}

void nextDrawShape(){
  uint8_t next;
  if(drawNWP==0){ next=(uint8_t)random(0,5); }
  else { do{ next=(uint8_t)random(0,5); } while(next==drawShapeType); }
  drawShapeType=next;
  tft.fillRect(0,0,tft.width(),tft.height()-MENU_H,C_BG);
  drawMenuBar("HOLD 2 SEC FOR MENU");
  genDrawShape(drawShapeType);
  drawShapeDots(TFT_WHITE);
  lastDrawX=-1; lastDrawY=-1;
  for(int i=0;i<16;i++) myData.allAngles[i]=0;
}

void startDrawMode(){
  currentState=DRAW_MODE;
  tft.fillScreen(C_BG);
  drawMenuBar("HOLD 2 SEC FOR MENU");
  drawNWP=0;
  nextDrawShape();
}


// BACK BUTTON:

bool checkBackButton(int tx,int ty){
  if(ty>(uint16_t)(tft.height()-MENU_H)){
    backReleasedAt=0;  // finger confirmed still on bar
    if(!isHoldingBack){isHoldingBack=true; backButtonTimer=millis();}
    else{
      unsigned long t=millis()-backButtonTimer;
      tft.fillRect(0,tft.height()-MENU_H,map(t,0,2000,0,tft.width()),MENU_H,C_DANGER);
      tft.setTextColor(C_TEXT,C_DANGER);
      tft.drawCentreString("RELEASE...",tft.width()/2,tft.height()-MENU_H+MENU_H/2-6,2);
      if(t>2000){
        isHoldingBack=false; backButtonTimer=0; backReleasedAt=0;
        for(int i=0;i<16;i++) myData.allAngles[i]=0;
        sendToServo(0,0);
        sendHapticStop();
        goToMenu(); return true;
      }
    }
  } else {isHoldingBack=false; backButtonTimer=0; backReleasedAt=0;}  // deliberate touch in game area
  return false;
}

// GAME LOGIC:

void gameLoop(){if(millis()-lastMoleTime>gameSpeed) spawnMole();}
void spawnMole(){
  if(currentMole!=-1){
    int r=currentMole/cols,c=currentMole%cols,cx,cy; getServoPos(c,r,cx,cy);
    tft.fillCircle(cx,cy,(BOX_SIZE/2)+4,C_BG);
    tft.drawCircle(cx,cy,(BOX_SIZE/2)+5,C_SURFACE);
  }
  int nm; do{nm=random(0,16);}while(nm==currentMole);
  currentMole=nm;
  int r=currentMole/cols,c=currentMole%cols,cx,cy; getServoPos(c,r,cx,cy);
  tft.fillCircle(cx,cy,(BOX_SIZE/2)+5,C_DANGER);
  for(int i=0;i<16;i++) myData.allAngles[i]=0;
  myData.allAngles[currentMole]=90; sendToServo(2,0);
  tft.fillRect(0,tft.height()-MENU_H-20,tft.width(),18,C_BG);
  tft.setTextColor(C_SUBTEXT,C_BG);
  tft.drawCentreString("Score: "+String(gameScore),tft.width()/2,tft.height()-MENU_H-18,1);
  lastMoleTime=millis(); moleIsUp=true;
}

// DRAW HAPTIC:

void sendDrawHaptic(int touchX, int touchY){
  // Draw mode has no title bar — use full width/height
  // Divide screen into 4x4 grid over the full drawing area
  int drawW = tft.width();
  int drawH = tft.height() - MENU_H;

  float gx = (float)touchX / (drawW / 4.0f);
  float gy = (float)touchY / (drawH / 4.0f);
  if(gx < 0) gx = 0; if(gx > 3.99f) gx = 3.99f;
  if(gy < 0) gy = 0; if(gy > 3.99f) gy = 3.99f;

  int c = constrain((int)gx, 0, 3);
  int r = constrain((int)gy, 0, 3);
  int8_t motor = TRIL_MAP[15-(r*cols+c)];

  for(int i=0;i<16;i++) myData.allAngles[i]=0;
  if(motor>=0 && motor<16) myData.allAngles[motor]=255;
  myData.id=BOARD_10; myData.mode=5; myData.tempVal=0;
  esp_now_send(mac_servo,(uint8_t*)&myData,sizeof(myData));
}


// MPU6050 RAW READ:

void readMPU6050raw(int16_t &ax, int16_t &ay, int16_t &az,
                    int16_t &gx, int16_t &gy, int16_t &gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)14, (uint8_t)true);
  ax = (Wire.read()<<8)|Wire.read();
  ay = (Wire.read()<<8)|Wire.read();
  az = (Wire.read()<<8)|Wire.read();
  Wire.read(); Wire.read(); // temp — skip
  gx = (Wire.read()<<8)|Wire.read();
  gy = (Wire.read()<<8)|Wire.read();
  gz = (Wire.read()<<8)|Wire.read();
}

void initMPU6050() {} // already done in setup()


// GRAV LOOP — accel-only low-pass filter (gyro removed):

void gravLoop() {
  if (isHoldingBack) return;
  if (!mpuOK) return;
  unsigned long now = millis();

  // Accelerometer only — gyroscope completely removed.
  // The gyro integrates rotations in the body frame of the IMU.
  // With an upside-down mounted IMU and fast small movements,
  // pitch rotation leaks into roll and vice versa (axis mixing).
  // The accelerometer gives absolute orientation without drift or axis mixing.
  int16_t rawAx,rawAy,rawAz,rawGx,rawGy,rawGz;
  readMPU6050raw(rawAx,rawAy,rawAz,rawGx,rawGy,rawGz);

  float ax = rawAx / 16384.0f;
  float ay = rawAy / 16384.0f;
  float az = rawAz / 16384.0f;

  // Low-pass filter directly on the three raw accel axes
  // filtering filtAz prevents singularity when az ≈ 0
  // LP: 0.08=slow/stable  0.15=normal  0.30=fast
  const float LP = 0.15f;
  filtAx += (ax - filtAx) * LP;
  filtAy += (ay - filtAy) * LP;
  static float filtAz = -1.0f;  // init at -1g (upside down)
  filtAz += (az - filtAz) * LP;

  // Angles from filtered accel values
  // -filtAz because IMU is mounted upside down (az ≈ -1g flat)
  float accelRoll  = atan2f(-filtAx, -filtAz) * 57.2958f;
  float accelPitch = atan2f( filtAy, sqrtf(filtAx*filtAx + filtAz*filtAz)) * 57.2958f;

  gravRoll  = accelRoll;
  gravPitch = accelPitch;

  // Send servos only on interval
  if (now - gravLastUpdate < GRAV_UPDATE_MS) return;
  gravLastUpdate = now;

  // Clamp to ±20° — 20° tilt = ball at the edge
  float pitch = constrain( gravPitch, -20.0f, 20.0f);
  float roll  = constrain(-gravRoll,  -20.0f, 20.0f);

  // Normalize to -1..+1 and calculate screen coordinate of lowest point
  int W = tft.width(), H = tft.height() - MENU_H;
  float normRoll  = roll  / 20.0f;
  float normPitch = pitch / 20.0f;

  float targetX = (W / 2.0f) + normRoll  * (W / 2.0f);
  float targetY = (H / 2.0f) + normPitch * (H / 2.0f);
  targetX = constrain(targetX, 0.0f, (float)W);
  targetY = constrain(targetY, 0.0f, (float)H);

  // Second low-pass on ball position for visual smoothness
  static float lowX = -1.0f;
  static float lowY = -1.0f;
  if (lowX < 0.0f) { lowX = W / 2.0f; lowY = H / 2.0f; }
  lowX += (targetX - lowX) * 0.25f;
  lowY += (targetY - lowY) * 0.25f;

  // Servo gradient: closest to lowest point = most extended
  float maxDist = sqrtf((W/2.0f)*(W/2.0f) + (H/2.0f)*(H/2.0f));
  for (int r=0; r<rows; r++) {
    for (int c=0; c<cols; c++) {
      int cx, cy;
      getServoPos(c, r, cx, cy);
      float dx = cx - lowX;
      float dy = cy - lowY;
      float dist = sqrtf(dx*dx + dy*dy);
      float intensity = 1.0f - constrain(dist / maxDist, 0.0f, 1.0f);
      intensity = intensity * intensity;  // squared for more contrast
      int servoIdx = 15 - (r*cols + c);
      myData.allAngles[servoIdx] = (uint8_t)(90.0f * intensity);
    }
  }
  sendToServo(0, 0);
  drawGravDisplay(lowX, lowY, pitch, roll);
}


// GRAV VISUAL DISPLAY:

void drawGravDisplay(float lowX, float lowY, float pitch, float roll) {
  int W = tft.width(), H = tft.height() - MENU_H;

  // Draw 4x4 grid with color intensity per cell
  for (int r=0; r<rows; r++) {
    for (int c=0; c<cols; c++) {
      int cx, cy;
      getServoPos(c, r, cx, cy);
      int hs = BOX_SIZE/2 + 4; // slightly larger for visual effect

      int servoIdx = 15-(r*cols+c);
      float intensity = myData.allAngles[servoIdx] / 90.0f;

      // Color: dark blue (low) → cyan → white (high)
      uint8_t bright = (uint8_t)(intensity * 255.0f);
      // RGB565: interpolate from 0x1082 (dark) to 0x07FF (cyan)
      uint8_t r5 = 0;
      uint8_t g6 = (uint8_t)(intensity * 63.0f);
      uint8_t b5 = (uint8_t)(8 + intensity * 23.0f);
      uint16_t cellColor = (r5 << 11) | (g6 << 5) | b5;

      tft.fillRoundRect(cx-hs, cy-hs, hs*2, hs*2, 4, cellColor);

      // Show intensity value
      tft.setTextColor(TFT_WHITE, cellColor);
      char lbl[4]; snprintf(lbl, sizeof(lbl), "%d", (int)(intensity*100));
      tft.drawCentreString(lbl, cx, cy-5, 1);
    }
  }

  // Draw 'lowest point' circle
  static float prevLowX = -1, prevLowY = -1;
  // Erase previous
  if (prevLowX >= 0)
    tft.fillCircle((int)prevLowX, (int)prevLowY, 8, C_BG);
  // Draw new
  tft.fillCircle((int)lowX, (int)lowY, 8, C_DANGER);
  tft.fillCircle((int)lowX, (int)lowY, 4, TFT_WHITE);
  prevLowX = lowX; prevLowY = lowY;

  // Pitch/Roll text (bottom, above menubar)
  int ty = tft.height() - MENU_H - 18;
  tft.fillRect(0, ty, W, 16, C_BG);
  char info[40];
  snprintf(info, sizeof(info), "P:%.1f  R:%.1f", pitch, roll);
  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawCentreString(info, W/2, ty, 1);
}

void startGravMode() {
  if (!mpuOK) {
    // Show error if MPU not found
    tft.fillScreen(C_BG);
    drawTitle("GRAVITY");
    tft.setTextColor(C_DANGER, C_BG);
    tft.drawCentreString("MPU6050 not found!", tft.width()/2, 100, 2);
    tft.setTextColor(C_SUBTEXT, C_BG);
    tft.drawCentreString("Check SDA=GPIO21 SCL=GPIO22", tft.width()/2, 130, 1);
    drawMenuBar("HOLD 2 SEC FOR MENU");
    currentState = GRAV_MODE;
    return;
  }
  currentState = GRAV_MODE;
  gravPitch = 0.0f; gravRoll = 0.0f;
  gravLastTime   = millis();
  gravLastUpdate = 0;
  tft.fillScreen(C_BG);
  drawMenuBar("HOLD 2 SEC FOR MENU");
}


// SNAKE HAPTIC HELPER:

void snakeHapticAt(int gx, int gy) {
  float sx = snakeOffX + gx * SNAKE_CELL + SNAKE_CELL / 2.0f;
  float sy = snakeOffY + gy * SNAKE_CELL + SNAKE_CELL / 2.0f;
  int W = tft.width(), H = tft.height() - MENU_H;
  int c = constrain((int)(sx / (W / 4.0f)), 0, 3);
  int r = constrain((int)(sy / (H / 4.0f)), 0, 3);
  int8_t motor = TRIL_MAP[15-(r*cols+c)];
  for(int i=0;i<16;i++) myData.allAngles[i]=0;
  if (motor>=0 && motor<16) myData.allAngles[motor]=255;
  myData.id=BOARD_10; myData.mode=5; myData.tempVal=0;
  esp_now_send(mac_servo,(uint8_t*)&myData,sizeof(myData));
}

// BUZZER:

void buzzerStart(const BuzzerNote* melody, int len, bool loop) {
  buzzerMelody  = melody;
  buzzerLen     = len;
  buzzerIdx     = 0;
  buzzerNextAt  = millis();
  buzzerPlaying = true;
  buzzerDoLoop  = loop;
}

void buzzerStop() {
  buzzerPlaying = false;
  ledcWrite(BUZZER_PIN, 0);
}

void buzzerUpdate() {
  if (!buzzerPlaying || !buzzerMelody) return;
  if (millis() < buzzerNextAt) return;
  BuzzerNote n = buzzerMelody[buzzerIdx];
  if (n.freq > 0 && buzzerVolume > 0) {
    ledcChangeFrequency(BUZZER_PIN, n.freq, 8);
    ledcWrite(BUZZER_PIN, (uint32_t)map(buzzerVolume, 1, 10, 3, 127));
  } else {
    ledcWrite(BUZZER_PIN, 0);
  }
  buzzerNextAt = millis() + n.ms;
  buzzerIdx++;
  if (buzzerIdx >= buzzerLen) {
    if (buzzerDoLoop) buzzerIdx = 0;
    else              buzzerStop();
  }
}

// SNAKE GAME FUNCTIONS:


void snakeDrawCell(int gx, int gy, uint16_t color) {
  tft.fillRect(snakeOffX + gx*SNAKE_CELL + 1,
               snakeOffY + gy*SNAKE_CELL + 1,
               SNAKE_CELL - 2, SNAKE_CELL - 2, color);
}

void snakeDrawFood() {
  int px = snakeOffX + snakeFood.x*SNAKE_CELL + SNAKE_CELL/2;
  int py = snakeOffY + snakeFood.y*SNAKE_CELL + SNAKE_CELL/2;
  tft.fillCircle(px, py, SNAKE_CELL/2 - 2, C_DANGER);
}

void snakeDrawScore() {
  // Score is ABOVE the grid (y=2..16), grid starts at y=18
  tft.fillRect(0, 2, tft.width(), 14, C_BG);
  tft.setTextColor(C_SUBTEXT, C_BG);
  char buf[30];
  snprintf(buf, sizeof(buf), "Score: %d   Length: %d", snakeScore, snakeLen);
  tft.drawCentreString(buf, tft.width()/2, 2, 1);
}

void snakeSpawnFood() {
  // Temporarily place food outside the grid so snakeIsFree doesn't see it as occupied
  snakeFood.x = -1;
  snakeFood.y = -1;
  int attempts = 0;
  while (attempts < 1000) {
    attempts++;
    int8_t fx = (int8_t)random(0, SNAKE_COLS);
    int8_t fy = (int8_t)random(0, SNAKE_ROWS);
    if (snakeIsFree(fx, fy)) {
      snakeFood.x = fx;
      snakeFood.y = fy;
      break;
    }
  }
  // Fallback: scan sequentially if random fails (grid nearly full)
  if (snakeFood.x < 0) {
    for (int y = 0; y < SNAKE_ROWS && snakeFood.x < 0; y++) {
      for (int x = 0; x < SNAKE_COLS && snakeFood.x < 0; x++) {
        if (snakeIsFree(x, y)) {
          snakeFood.x = (int8_t)x;
          snakeFood.y = (int8_t)y;
        }
      }
    }
  }
  if (snakeFood.x >= 0) snakeDrawFood();
}

void snakeUpdateDir() {
  if (!mpuOK) return;
  int16_t rawAx,rawAy,rawAz,rawGx,rawGy,rawGz;
  readMPU6050raw(rawAx,rawAy,rawAz,rawGx,rawGy,rawGz);
  float ax = rawAx / 16384.0f;
  float ay = rawAy / 16384.0f;
  float az = rawAz / 16384.0f;
  const float LP = 0.15f;
  filtAx += (ax - filtAx) * LP;
  filtAy += (ay - filtAy) * LP;

  // Axis direction calibrated to IMU mounting
  // filtAx positive = tilt right (after measurement: sign was reversed, corrected)
  float tiltX =  filtAx;  // positive = tilt right (corrected)
  float tiltY =  filtAy;  // positive = tilt forward/down

  SnakeDir newDir = snakeDir;
  if (fabsf(tiltX) > fabsf(tiltY)) {
    if (tiltX >  SNAKE_DIR_THR) newDir = SDIR_RIGHT;
    if (tiltX < -SNAKE_DIR_THR) newDir = SDIR_LEFT;
  } else {
    if (tiltY >  SNAKE_DIR_THR) newDir = SDIR_DOWN;
    if (tiltY < -SNAKE_DIR_THR) newDir = SDIR_UP;
  }

  // Effective direction = last in queue
  SnakeDir effectiveDir = snakeDir;
  if (snakeDirQueueLen > 0) effectiveDir = snakeDirQueue[snakeDirQueueLen - 1];

  bool sameAsEffective   = (newDir == effectiveDir);
  bool reverseOfEffective =
    (newDir==SDIR_RIGHT && effectiveDir==SDIR_LEFT)  ||
    (newDir==SDIR_LEFT  && effectiveDir==SDIR_RIGHT) ||
    (newDir==SDIR_UP    && effectiveDir==SDIR_DOWN)  ||
    (newDir==SDIR_DOWN  && effectiveDir==SDIR_UP);

  if (!sameAsEffective && !reverseOfEffective && snakeDirQueueLen < 2) {
    snakeDirQueue[snakeDirQueueLen++] = newDir;
  }
}

void snakeDie() {
  snakeDead = true;
  buzzerStart(MELODY_GAMEOVER, MELODY_GAMEOVER_LEN, false);
  // Servos off
  for(int i=0;i<16;i++) myData.allAngles[i]=0;
  sendToServo(0,0);
  // Flash red
  for (int i = 0; i < 3; i++) {
    tft.fillRect(snakeOffX, snakeOffY, SNAKE_COLS*SNAKE_CELL, SNAKE_ROWS*SNAKE_CELL, C_DANGER);
    delay(120);
    tft.fillRect(snakeOffX, snakeOffY, SNAKE_COLS*SNAKE_CELL, SNAKE_ROWS*SNAKE_CELL, C_BG);
    delay(80);
  }
  // Redraw snake
  for (int i = snakeLen-1; i >= 0; i--) {
    snakeDrawCell(snakeBody[i].x, snakeBody[i].y, (i==0) ? 0x07E0 : 0x03A0);
  }
  snakeDrawFood();
  // Game over text
  tft.setTextColor(C_DANGER, C_BG);
  tft.drawCentreString("GAME OVER", tft.width()/2, snakeOffY + SNAKE_ROWS*SNAKE_CELL/2 - 20, 4);
  tft.setTextColor(C_SUBTEXT, C_BG);
  char buf[30]; snprintf(buf, sizeof(buf), "Score: %d", snakeScore);
  tft.drawCentreString(buf, tft.width()/2, snakeOffY + SNAKE_ROWS*SNAKE_CELL/2 + 12, 2);
  tft.drawCentreString("Tap screen to restart", tft.width()/2, snakeOffY + SNAKE_ROWS*SNAKE_CELL/2 + 36, 1);
}

void snakeMove() {
  // Get next direction from queue
  if (snakeDirQueueLen > 0) {
    snakeDir = snakeDirQueue[0];
    snakeDirQueue[0] = snakeDirQueue[1];
    snakeDirQueueLen--;
  }

  // New head position
  SnakePos newHead = snakeBody[0];
  switch(snakeDir) {
    case SDIR_RIGHT: newHead.x++; break;
    case SDIR_LEFT:  newHead.x--; break;
    case SDIR_UP:    newHead.y--; break;
    case SDIR_DOWN:  newHead.y++; break;
  }
  // Wrap through walls
  newHead.x = (int8_t)((newHead.x + SNAKE_COLS) % SNAKE_COLS);
  newHead.y = (int8_t)((newHead.y + SNAKE_ROWS) % SNAKE_ROWS);

  // Self-collision check (skip last segment — it moves away)
  for (int i = 1; i < snakeLen - 1; i++) {
    if (newHead.x == snakeBody[i].x && newHead.y == snakeBody[i].y) {
      snakeDie(); return;
    }
  }

  // Obstacle collision check
  if (snakeIsObs(newHead.x, newHead.y)) {
    snakeDie(); return;
  }

  // Food check
  bool ate = (newHead.x == snakeFood.x && newHead.y == snakeFood.y);

  // Erase tail (before shift)
  if (!ate) {
    snakeDrawCell(snakeBody[snakeLen-1].x, snakeBody[snakeLen-1].y, C_BG);
  }

  // Shift body
  int newLen = min(snakeLen + (ate ? 1 : 0), SNAKE_COLS*SNAKE_ROWS);
  for (int i = newLen-1; i > 0; i--) {
    snakeBody[i] = snakeBody[i-1];
  }
  snakeBody[0] = newHead;
  if (ate) snakeLen = newLen;

  if (ate) {
    snakeScore++;
    snakeCurSpeed = max((unsigned long)SNAKE_SPEED_MIN, snakeCurSpeed - SNAKE_SPEED_STEP);
    snakeSpawnFood();
    snakeDrawScore();

    // Servos: short pulse on eating
    for(int i=0;i<16;i++) myData.allAngles[i]=90;
    sendToServo(0,0);
    delay(80);
    for(int i=0;i<16;i++) myData.allAngles[i]=0;
    sendToServo(0,0);
  }

  // Draw old head as body, new head as head
  snakeDrawCell(snakeBody[1].x, snakeBody[1].y, 0x03A0);  // dark green body
  snakeDrawCell(snakeBody[0].x, snakeBody[0].y, 0x07E0);  // bright green head

  // Servos: gradient at head position (movement sensation)
  float hx = (float)snakeBody[0].x / (SNAKE_COLS-1) * (tft.width()-2*SIDE_M) + SIDE_M;
  float hy = (float)snakeBody[0].y / (SNAKE_ROWS-1) * (tft.height()-MENU_H-2*TOP_M) + TOP_M;
  calculateGradient((int)hx, (int)hy);
  // Haptic: vibration at food position (hint of where food is)
  if (snakeFood.x >= 0) snakeHapticAt(snakeFood.x, snakeFood.y);
}

void snakeLoop() {
  if (isHoldingBack) return;
  snakeUpdateDir();

  unsigned long now = millis();

  // First obstacle after 10s, then every 20s
  // 1st after 5s, 2nd after 10s, rest every 15s
  unsigned long obsInterval;
  if      (snakeObsCount == 0) obsInterval = SNAKE_OBS_INTERVAL_FIRST;
  else if (snakeObsCount <= 2) obsInterval = SNAKE_OBS_INTERVAL_SECOND;
  else                         obsInterval = SNAKE_OBS_INTERVAL;
  if (!snakeDead && now - snakeLastObs >= obsInterval) {
    snakeLastObs = now;
    snakeSpawnObstacle();
  }

  if (!snakeDead && now - snakeLastMove >= snakeCurSpeed) {
    snakeLastMove = now;
    snakeMove();
  }
}

void startSnakeMode() {
  currentState = SNAKE_MODE;
  tft.fillScreen(C_BG);
  drawMenuBar("HOLD 2 SEC FOR MENU");

  snakeOffX = (tft.width() - SNAKE_COLS * SNAKE_CELL) / 2;
  snakeOffY = 18;  // fixed offset: space for score text at top

  // Border
  tft.drawRect(snakeOffX-1, snakeOffY-1,
               SNAKE_COLS*SNAKE_CELL+2, SNAKE_ROWS*SNAKE_CELL+2, C_SURFACE);

  // Init snake — length 3, center, direction right
  snakeLen       = 3;
  snakeDir       = SDIR_RIGHT;
  snakeScore     = 0;
  snakeDead      = false;
  snakeCurSpeed  = SNAKE_SPEED_MS;
  snakeLastMove  = millis();
  filtAx = 0.0f; filtAy = 0.0f;
  snakeDirQueue[0] = SDIR_RIGHT;
  snakeDirQueue[1] = SDIR_RIGHT;
  snakeDirQueueLen = 0;
  snakeObsCount   = 0;
  snakeLastObs    = millis();

  int sx = SNAKE_COLS / 2;
  int sy = SNAKE_ROWS / 2;
  snakeBody[0] = {(int8_t)(sx),   (int8_t)sy};
  snakeBody[1] = {(int8_t)(sx-1), (int8_t)sy};
  snakeBody[2] = {(int8_t)(sx-2), (int8_t)sy};

  snakeDrawCell(snakeBody[0].x, snakeBody[0].y, 0x07E0);
  snakeDrawCell(snakeBody[1].x, snakeBody[1].y, 0x03A0);
  snakeDrawCell(snakeBody[2].x, snakeBody[2].y, 0x03A0);

  snakeSpawnFood();
  snakeDrawScore();
  buzzerStart(MELODY_SNAKE, MELODY_SNAKE_LEN, true);
}

// SNAKE OBSTACLE FUNCTIONS:


bool snakeIsObs(int x, int y) {
  for (int i = 0; i < snakeObsCount; i++) {
    if (snakeObs[i].x == x && snakeObs[i].y == y) return true;
  }
  return false;
}

bool snakeIsFree(int x, int y) {
  // Check snake body
  for (int i = 0; i < snakeLen; i++) {
    if (snakeBody[i].x == x && snakeBody[i].y == y) return false;
  }
  // Check food
  if (snakeFood.x == x && snakeFood.y == y) return false;
  // Check obstacles
  if (snakeIsObs(x, y)) return false;
  return true;
}

void snakeDrawAllObs() {
  for (int i = 0; i < snakeObsCount; i++) {
    snakeDrawCell(snakeObs[i].x, snakeObs[i].y, SNAKE_OBS_COLOR);
  }
}

void snakeClearObstacles() {
  // Erase visually
  for (int i = 0; i < snakeObsCount; i++) {
    snakeDrawCell(snakeObs[i].x, snakeObs[i].y, C_BG);
  }
  snakeObsCount = 0;
}

void snakeSpawnObstacle() {
  // Reset if 1/3 of grid is filled with obstacles
  if (snakeObsCount >= SNAKE_OBS_RESET_THR) {
    snakeClearObstacles();
    snakeDrawFood(); // redraw food after clearing
    // Redraw snake
    for (int i = snakeLen-1; i >= 0; i--) {
      snakeDrawCell(snakeBody[i].x, snakeBody[i].y, (i==0) ? 0x07E0 : 0x03A0);
    }
    return;
  }

  // Choose size (1, 2 or 3 cells) and orientation (H or V)
  int size = random(1, 3);  // 1 or 2 cells (max 2)
  bool horiz = random(0, 2);

  // Find a free start position
  int attempts = 0;
  while (attempts < 200) {
    attempts++;
    int sx = random(0, SNAKE_COLS);
    int sy = random(0, SNAKE_ROWS);
    bool fits = true;

    // Check all cells of this obstacle
    for (int j = 0; j < size; j++) {
      int cx = horiz ? (sx + j) % SNAKE_COLS : sx;
      int cy = horiz ? sy : (sy + j) % SNAKE_ROWS;
      if (!snakeIsFree(cx, cy)) { fits = false; break; }
    }

    if (fits && snakeObsCount + size <= SNAKE_OBS_MAX * 3) {
      for (int j = 0; j < size; j++) {
        int cx = horiz ? (sx + j) % SNAKE_COLS : sx;
        int cy = horiz ? sy : (sy + j) % SNAKE_ROWS;
        snakeObs[snakeObsCount].x = (int8_t)cx;
        snakeObs[snakeObsCount].y = (int8_t)cy;
        snakeObsCount++;
        snakeDrawCell(cx, cy, SNAKE_OBS_COLOR);
      }
      break;
    }
  }
}



// BALLOON POP GAME FUNCTIONS:


void balDrawScore() {
  int W = tft.width();
  tft.fillRect(0, 0, W, 14, C_BG);
  // Score centered
  char buf[30];
  snprintf(buf, sizeof(buf), "Score: %d", balScore);
  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawCentreString(buf, W/2, 2, 1);
}

void balDrawOne(int i, uint16_t over) {
  if (over == 0xFFFF) {
    // Draw balloon
    uint16_t col = balloons[i].color;
    int r = (int)balloons[i].r;
    int x = (int)balloons[i].x;
    int y = (int)balloons[i].y;
    tft.fillCircle(x, y, r, col);
    // Gloss (white dot top-left)
    tft.fillCircle(x - r/3, y - r/3, max(2, r/5), TFT_WHITE);
    // Bomb: draw X
    if (balloons[i].type == BAL_BOMB) {
      tft.drawLine(x-r/2, y-r/2, x+r/2, y+r/2, TFT_WHITE);
      tft.drawLine(x+r/2, y-r/2, x-r/2, y+r/2, TFT_WHITE);
    }
    // Golden: star marker
    if (balloons[i].type == BAL_GOLDEN) {
      tft.fillCircle(x, y, r/3, 0xFFFF);
    }
  } else {
    // Erase balloon
    tft.fillCircle((int)balloons[i].x, (int)balloons[i].y, (int)balloons[i].r + 3, C_BG);
  }
}

void balPop(int i, bool bomb) {
  // Erase balloon
  balDrawOne(i, C_BG);

  if (bomb) {
    // Bomb: immediate game over
    balDead = true;
    buzzerStart(MELODY_GAMEOVER, MELODY_GAMEOVER_LEN, false);
    balloons[i].bursting = true;
    balloons[i].burstR   = balloons[i].r;
    balloons[i].color    = C_DANGER;
    // Game over screen
    int W = tft.width(), H = tft.height() - MENU_H;
    delay(200); // short pause so pop is visible
    tft.fillRect(20, H/2-45, W-40, 95, C_BG);
    tft.drawRect(20, H/2-45, W-40, 95, C_DANGER);
    tft.setTextColor(C_DANGER, C_BG);
    tft.drawCentreString("BOOM!", W/2, H/2-30, 4);
    tft.setTextColor(C_SUBTEXT, C_BG);
    char gbuf[30]; snprintf(gbuf, sizeof(gbuf), "Score: %d", balScore);
    tft.drawCentreString(gbuf, W/2, H/2+5, 2);
    tft.drawCentreString("Tap to restart", W/2, H/2+28, 1);
    for(int j=0;j<16;j++) myData.allAngles[j]=0;
    sendToServo(0,0); sendHapticStop();
  } else {
    int pts = (balloons[i].type == BAL_GOLDEN) ? 3 : 1;
    balScore += pts;
    // Burst animation color
    balloons[i].bursting = true;
    balloons[i].burstR   = balloons[i].r;
    balDrawScore();
    // Spawn interval slightly shorter (faster after each point)
    balSpawnInt = max((unsigned long)BAL_SPAWN_MIN, balSpawnInt - 80);
  }

  // Haptic: motor at balloon position
  int W = tft.width(), H = tft.height() - MENU_H;
  int c = constrain((int)(balloons[i].x / (W/4.0f)), 0, 3);
  int r = constrain((int)(balloons[i].y / (H/4.0f)), 0, 3);
  int8_t motor = TRIL_MAP[15-(r*cols+c)];
  for(int j=0;j<16;j++) myData.allAngles[j]=0;
  if (motor>=0 && motor<16) myData.allAngles[motor]=255;
  myData.id=BOARD_10; myData.mode=5; myData.tempVal=0;
  esp_now_send(mac_servo,(uint8_t*)&myData,sizeof(myData));

  // Servos: wave toward balloon position
  calculateGradient((int)balloons[i].x, (int)balloons[i].y);

  balloons[i].alive = false;
}

void balSpawnOne() {
  // Find empty slot
  int slot = -1;
  for (int i = 0; i < BAL_MAX; i++) {
    if (!balloons[i].alive && !balloons[i].bursting) { slot = i; break; }
  }
  if (slot < 0) return;

  int W = tft.width(), H = tft.height() - MENU_H;
  float r = random(14, 28);

  // Determine type
  int rnd = random(0, 100);
  uint8_t type;
  uint16_t col;
  if (rnd < BAL_BOMB_CHANCE) {
    type = BAL_BOMB;
    col  = 0x8000; // dark red
  } else if (rnd < BAL_BOMB_CHANCE + BAL_GOLDEN_CHANCE) {
    type = BAL_GOLDEN;
    col  = 0xFFE0; // gold
  } else {
    type = BAL_NORMAL;
    col  = BAL_NORMAL_COLORS[random(0, BAL_N_COLORS)];
  }

  balloons[slot].x        = random((int)r + 5, W - (int)r - 5);
  balloons[slot].y        = (float)H + r + 2; // start just below game area
    balloons[slot].vy       = -(random(16, 32) / 10.0f); // 1.6 – 3.2 px/frame upward
  balloons[slot].r        = r;
  balloons[slot].type     = type;
  balloons[slot].color    = col;
  balloons[slot].alive    = true;
  balloons[slot].bursting = false;
  balloons[slot].burstR   = r;
}

void balloonLoop() {
  if (isHoldingBack) return;
  if (balDead) return;

  unsigned long now = millis();
  if (now - balLastUpdate < BAL_UPDATE_MS) return;
  float dt = (now - balLastUpdate) / 16.0f;
  balLastUpdate = now;

  int W = tft.width(), H = tft.height() - MENU_H;

  // Spawn new balloon
  if (now - balLastSpawn >= balSpawnInt) {
    balLastSpawn = now;
    balSpawnOne();
  }

  // Update and draw all balloons
  for (int i = 0; i < BAL_MAX; i++) {

    // Burst animation
    if (balloons[i].bursting) {
      int _bx=(int)balloons[i].x, _by=(int)balloons[i].y;
      int _br=(int)balloons[i].burstR + 2;
      tft.fillRect(_bx-_br, _by-_br, _br*2, _br*2, C_BG);
      if (_by - _br < 16) balDrawScore();
      balloons[i].burstR += 2.5f * dt;
      if (balloons[i].burstR > balloons[i].r * 2.2f) {
        balloons[i].bursting = false;
        // Restore haptic stop
        sendHapticStop();
        for(int j=0;j<16;j++) myData.allAngles[j]=0;
        sendToServo(0,0);
      } else {
        tft.drawCircle((int)balloons[i].x, (int)balloons[i].y,
                       (int)balloons[i].burstR, balloons[i].color);
      }
      continue;
    }

    if (!balloons[i].alive) continue;

    // Erase old position via fillRect — faster and no border artifacts
    if (balloons[i].y + balloons[i].r < (float)(H - 2)) {
      int _ex = (int)balloons[i].x, _ey = (int)balloons[i].y;
      int _er = (int)balloons[i].r + 2;
      tft.fillRect(_ex-_er, _ey-_er, _er*2, _er*2, C_BG);
      // Restore score text if erase passed over it
      if (_ey - _er < 16) balDrawScore();
    }

    // Move upward
    balloons[i].y += balloons[i].vy * dt;

    // Balloon leaves screen at top
    if (balloons[i].y + balloons[i].r < 16) {
      balloons[i].alive = false;
      // Bomb may escape — normal/golden may not
      if (balloons[i].type != BAL_BOMB) {
        balDead = true;
        buzzerStart(MELODY_GAMEOVER, MELODY_GAMEOVER_LEN, false);
        tft.fillRect(20, H/2-45, W-40, 95, C_BG);
        tft.drawRect(20, H/2-45, W-40, 95, C_DANGER);
        tft.setTextColor(C_DANGER, C_BG);
        tft.drawCentreString("ESCAPED!", W/2, H/2-30, 4);
        tft.setTextColor(C_SUBTEXT, C_BG);
        char obuf[30]; snprintf(obuf, sizeof(obuf), "Score: %d", balScore);
        tft.drawCentreString(obuf, W/2, H/2+5, 2);
        tft.drawCentreString("Tap to restart", W/2, H/2+28, 1);
        for(int j=0;j<16;j++) myData.allAngles[j]=0;
        sendToServo(0,0); sendHapticStop();
        return;
      }
      continue;  // bomb escaped = no penalty
    }

    // Redraw balloon — clip above score (y>20) and below menubar (y<H-2)
    if (balloons[i].y - balloons[i].r > 20 &&
        balloons[i].y + balloons[i].r < (float)(H - 2)) {
      balDrawOne(i, 0xFFFF);
    }
  }

  // No menubar redraw here — causes glitch during back-button hold
}

void startBalloonMode() {
  currentState = BALLOON_MODE;
  tft.fillScreen(C_BG);
  drawMenuBar("HOLD 2 SEC FOR MENU");

  // Reset all balloons
  for (int i = 0; i < BAL_MAX; i++) {
    balloons[i].alive    = false;
    balloons[i].bursting = false;
  }
  balScore     = 0;
  balLives     = BAL_LIVES;
  balDead      = false;
  balSpawnInt  = BAL_SPAWN_START;
  balLastSpawn = millis();
  balLastUpdate= millis();

  balDrawScore();
  buzzerStart(MELODY_BALLOON, MELODY_BALLOON_LEN, true);
}

// BRICK BREAKER FUNCTIONS:


void brickDrawPlatform(uint16_t color) {
  tft.fillRect((int)brickPlatX, brickPlatY, BRICK_PLAT_W, BRICK_PLAT_H, color);
}

void brickDrawBall(uint16_t color) {
  tft.fillCircle((int)brickBallX, (int)brickBallY, BRICK_BALL_R, color);
}

void brickDrawAllBricks() {
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      int bx = c * (BRICK_W + BRICK_GAP);
      int by = BRICK_TOP_Y + r * (BRICK_H + BRICK_GAP);
      if (brickAlive[r][c]) {
        tft.fillRect(bx, by, BRICK_W, BRICK_H, BRICK_ROW_COLORS[r]);
      }
    }
  }
}

void brickCheckCollisions() {
  int W = tft.width();
  int H = tft.height() - MENU_H;

  // Left/right walls
  if (brickBallX - BRICK_BALL_R <= 0) {
    brickBallX = BRICK_BALL_R + 1;
    brickBallDX = fabsf(brickBallDX);
  }
  if (brickBallX + BRICK_BALL_R >= W) {
    brickBallX = W - BRICK_BALL_R - 1;
    brickBallDX = -fabsf(brickBallDX);
  }

  // Ceiling
  if (brickBallY - BRICK_BALL_R <= 0) {
    brickBallY = BRICK_BALL_R + 1;
    brickBallDY = fabsf(brickBallDY);
  }

  // Platform collision
  if (brickBallDY > 0 &&
      brickBallY + BRICK_BALL_R >= brickPlatY &&
      brickBallY - BRICK_BALL_R <= brickPlatY + BRICK_PLAT_H &&
      brickBallX >= brickPlatX - BRICK_BALL_R &&
      brickBallX <= brickPlatX + BRICK_PLAT_W + BRICK_BALL_R) {
    // Angle depending on contact point on platform
    float relHit = (brickBallX - (brickPlatX + BRICK_PLAT_W / 2.0f)) / (BRICK_PLAT_W / 2.0f);
    relHit = constrain(relHit, -0.9f, 0.9f);
    float speed = sqrtf(BRICK_BALL_SPEED_X*BRICK_BALL_SPEED_X + BRICK_BALL_SPEED_Y*BRICK_BALL_SPEED_Y);
    float angle = relHit * 60.0f * (PI / 180.0f); // max ±60° from vertical
    brickBallDX =  speed * sinf(angle);
    brickBallDY = -speed * cosf(angle);
    brickBallY  = brickPlatY - BRICK_BALL_R - 1;
  }

  // Bottom — game over
  if (brickBallY - BRICK_BALL_R > H) {
    brickDead = true;
    buzzerStart(MELODY_GAMEOVER, MELODY_GAMEOVER_LEN, false);
    // Erase ball
    brickDrawBall(C_BG);
    // Game over screen
    tft.setTextColor(C_DANGER, C_BG);
    tft.drawCentreString("GAME OVER", W/2, H/2 - 20, 4);
    tft.setTextColor(C_SUBTEXT, C_BG);
    char buf[30]; snprintf(buf, sizeof(buf), "Score: %d", brickScore);
    tft.drawCentreString(buf, W/2, H/2 + 15, 2);
    tft.drawCentreString("Tap screen to restart", W/2, H/2 + 38, 1);
    for(int i=0;i<16;i++) myData.allAngles[i]=0;
    sendToServo(0,0);
    return;
  }

  // Brick collision
  for (int r = 0; r < BRICK_ROWS && !brickDead; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      if (!brickAlive[r][c]) continue;
      float bx1 = c * (BRICK_W + BRICK_GAP);
      float by1 = BRICK_TOP_Y + r * (BRICK_H + BRICK_GAP);
      float bx2 = bx1 + BRICK_W;
      float by2 = by1 + BRICK_H;

      // Nearest point on rectangle
      float cx = constrain(brickBallX, bx1, bx2);
      float cy = constrain(brickBallY, by1, by2);
      float dx = brickBallX - cx;
      float dy = brickBallY - cy;

      if (dx*dx + dy*dy <= (float)(BRICK_BALL_R * BRICK_BALL_R)) {
        // Remove brick
        brickAlive[r][c] = false;
        brickLeft--;
        brickScore++;
        tft.fillRect((int)bx1, (int)by1, BRICK_W, BRICK_H, C_BG);
        // Haptic: vibration on motor nearest to hit brick
        {
          float bcx=(bx1+bx2)/2.0f, bcy=(by1+by2)/2.0f;
          int hc=constrain((int)(bcx/(W/4.0f)),0,3);
          int hr=constrain((int)(bcy/(H/4.0f)),0,3);
          int8_t motor=TRIL_MAP[15-(hr*cols+hc)];
          for(int ii=0;ii<16;ii++) myData.allAngles[ii]=0;
          if(motor>=0&&motor<16) myData.allAngles[motor]=255;
          myData.id=BOARD_10; myData.mode=5; myData.tempVal=0;
          esp_now_send(mac_servo,(uint8_t*)&myData,sizeof(myData));
        }

        // Determine bounce: horizontal or vertical impact
        float overlapX = (brickBallX < (bx1+bx2)/2.0f)
                         ? (brickBallX + BRICK_BALL_R - bx1)
                         : (bx2 - (brickBallX - BRICK_BALL_R));
        float overlapY = (brickBallY < (by1+by2)/2.0f)
                         ? (brickBallY + BRICK_BALL_R - by1)
                         : (by2 - (brickBallY - BRICK_BALL_R));
        if (overlapX < overlapY) brickBallDX = -brickBallDX;
        else                     brickBallDY = -brickBallDY;

        // Score update
        tft.fillRect(0, 0, W, 14, C_BG);
        tft.setTextColor(C_SUBTEXT, C_BG);
        char buf[40];
        snprintf(buf, sizeof(buf), "Score: %d   Bricks: %d", brickScore, brickLeft);
        tft.drawCentreString(buf, W/2, 2, 1);

        // Win check
        if (brickLeft == 0) {
          brickWon = true;
          buzzerStart(MELODY_GAMEOVER, MELODY_GAMEOVER_LEN, false);
          brickDrawBall(C_BG);
          tft.setTextColor(C_SUCCESS, C_BG);
          tft.drawCentreString("YOU WIN!", W/2, H/2 - 20, 4);
          tft.setTextColor(C_SUBTEXT, C_BG);
          snprintf(buf, sizeof(buf), "Score: %d", brickScore);
          tft.drawCentreString(buf, W/2, H/2 + 15, 2);
          tft.drawCentreString("Tap screen to restart", W/2, H/2 + 38, 1);
          for(int i=0;i<16;i++) myData.allAngles[i]=0;
          sendToServo(0,0);
        }
        return; // max 1 brick per frame
      }
    }
  }
}

// Servos: platform position → 4x4 servo grid gradient
void brickServoUpdate() {
  float platCenter = brickPlatX + BRICK_PLAT_W / 2.0f;
  int W = tft.width();
  // Map platCenter to servo grid x (0-3)
  float gx = platCenter / (W / 4.0f);
  gx = constrain(gx, 0.0f, 3.99f);
  // Gradient over columns, platform is always at bottom → all rows
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      float dist = fabsf(gx - (float)c);
      float intensity = constrain(1.0f - dist * 0.7f, 0.0f, 1.0f);
      int servoIdx = 15 - (r*cols + c);
      myData.allAngles[servoIdx] = (uint8_t)(90.0f * intensity);
    }
  }
  sendToServo(0, 0);
}

// Haptic: ball position → activate nearest motor
void brickHapticUpdate() {
  int W = tft.width();
  int H = tft.height() - MENU_H;
  int c = constrain((int)(brickBallX / (W / 4.0f)), 0, 3);
  int r = constrain((int)(brickBallY / (H / 4.0f)), 0, 3);
  int8_t motor = TRIL_MAP[15 - (r*cols + c)];
  for(int i=0;i<16;i++) myData.allAngles[i]=0;
  if (motor >= 0 && motor < 16) myData.allAngles[motor] = 255;
  myData.id=BOARD_10; myData.mode=5; myData.tempVal=0;
  esp_now_send(mac_servo,(uint8_t*)&myData,sizeof(myData));
}

void brickLoop() {
  if (isHoldingBack) return;
  if (brickDead || brickWon) return;

  unsigned long now = millis();
  if (now - brickLastUpdate < BRICK_UPDATE_MS) return;
  brickLastUpdate = now;

  // Read IMU + move platform
  int16_t rawAx,rawAy,rawAz,rawGx,rawGy,rawGz;
  readMPU6050raw(rawAx,rawAy,rawAz,rawGx,rawGy,rawGz);
  float ax = rawAx / 16384.0f;
  filtAx += (ax - filtAx) * 0.15f;

  // Erase platform, move, redraw
  brickDrawPlatform(C_BG);
  float move = filtAx * BRICK_PLAT_SPEED;
  brickPlatX = constrain(brickPlatX + move, 0.0f, (float)(tft.width() - BRICK_PLAT_W));
  brickDrawPlatform(C_PRIMARY);

  // Erase ball, move, redraw
  brickDrawBall(C_BG);
  brickBallX += brickBallDX;
  brickBallY += brickBallDY;

  // Check collisions
  brickCheckCollisions();

  if (!brickDead && !brickWon) {
    brickDrawBall(TFT_WHITE);
    brickServoUpdate();
  }
}

void startBrickMode() {
  currentState = BRICK_MODE;
  tft.fillScreen(C_BG);
  drawMenuBar("HOLD 2 SEC FOR MENU");

  int W = tft.width();
  int H = tft.height() - MENU_H;

  // Init all bricks
  brickLeft = 0;
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      brickAlive[r][c] = true;
      brickLeft++;
    }
  }
  brickScore   = 0;
  brickDead    = false;
  brickWon     = false;
  brickLastUpdate = millis();

  // Platform start position (centered)
  brickPlatX = (W - BRICK_PLAT_W) / 2.0f;
  brickPlatY = H - 26;

  // Ball start position and speed
  brickBallX  = W / 2.0f;
  brickBallY  = (float)(BRICK_TOP_Y + BRICK_ROWS*(BRICK_H+BRICK_GAP) + 60);
  brickBallDX =  BRICK_BALL_SPEED_X;
  brickBallDY = -BRICK_BALL_SPEED_Y;

  filtAx = 0.0f;

  // Draw initial state
  brickDrawAllBricks();
  brickDrawPlatform(C_PRIMARY);
  brickDrawBall(TFT_WHITE);

  // Score header
  tft.setTextColor(C_SUBTEXT, C_BG);
  char buf[40];
  snprintf(buf, sizeof(buf), "Score: 0   Bricks: %d", brickLeft);
  tft.drawCentreString(buf, W/2, 2, 1);
  buzzerStart(MELODY_TETRIS, MELODY_TETRIS_LEN, true);
}


// VOLUME CONTROL:

void drawVolScreen() {
  int W = tft.width();
  tft.fillRect(0, 38, W, tft.height()-38-MENU_H, C_BG);
  char buf[8];
  if (buzzerVolume == 0) snprintf(buf, sizeof(buf), "MUTE");
  else                   snprintf(buf, sizeof(buf), "%d", buzzerVolume);
  tft.setTextColor(0xFFE0, C_BG);
  tft.drawCentreString(buf, W/2, 55, 4);
  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawCentreString("Buzzer volume (0-10)", W/2, 100, 2);
  int sx=30, sy=125, sw=W-60, sh=30;
  int filled = (int)(sw * buzzerVolume / 10.0f);
  tft.fillRoundRect(sx, sy, sw, sh, 8, C_SURFACE);
  if (filled>0) tft.fillRoundRect(sx, sy, filled, sh, 8, 0xFFE0);
  tft.drawRoundRect(sx, sy, sw, sh, 8, C_SUBTEXT);
  int knobX = constrain(sx+filled, sx, sx+sw);
  tft.fillCircle(knobX, sy+sh/2, sh/2+4, 0xFFE0);
  tft.fillCircle(knobX, sy+sh/2, sh/2, C_BG);
  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawString("0", sx, sy+sh+8, 1);
  tft.drawString("10", sx+sw-12, sy+sh+8, 1);
  int bh=50, bw=70, cy=195;
  drawBtn(btnVolMinus, 10,       cy, bw, bh, 0x0339, "-", C_TEXT, 4);
  drawBtn(btnVolPlus,  W-10-bw,  cy, bw, bh, 0x6200, "+", C_TEXT, 4);
  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawCentreString("Step: 1", W/2, cy+bh+6, 1);
}

void startVolMode() {
  currentState = VOL_MODE;
  tft.fillScreen(C_BG);
  drawTitle("VOLUME");
  drawVolScreen();
  drawMenuBar("HOLD 2 SEC FOR MENU");
}

// SEND — depthScale applied for servo modes:

void sendToServo(int mode, float temp){
  myData.id=BOARD_10; myData.mode=mode; myData.tempVal=temp;
  if(mode==0 || mode==2 || mode==3){
    for(int i=0;i<16;i++){
      myData.allAngles[i]=(uint8_t)(myData.allAngles[i]*depthScale);
    }
  }
  esp_now_send(mac_servo,(uint8_t*)&myData,sizeof(myData));
}
