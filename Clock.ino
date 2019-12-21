// -- motor constants begin
// x axis
const int MXSTEP = 6;
const int MXDIR = 7;
const int MXEN = 8;

// y axis
const int MYSTEP = 11;
const int MYDIR = 10;
const int MYEN = 12;

// hour axis
const int MHSTEP = A2; //A2
const int MHDIR = A3; //A3
const int MHEN = A1; //A1

const unsigned char DIR_MINUS = HIGH;
const unsigned char DIR_PLUS = LOW;

const int MOTOR_POLLING_TIMEOUT = 900;
// -- motor constants end

// buttons and switches
const int LSWITCHY = 2;
const int LSWITCHX = 3;
const int LSWITCHH = A6;
const int HOURBUTTON = 5;
const int MINBUTTON = 4;

// magic
unsigned MAGIC_GRID_X_OFFSET = 40*8*5 + 40;
unsigned MAGIC_GRID_Y_OFFSET = 60*8*5 - 20;

// the hour clock ticks from 0 degrees to 180 degrees. This gives us 100 rotations and a total of 800 steps
unsigned long HOUR_TIME_MILLI_TICK = 21557;

void h_step(unsigned char dir) {
  dir = dir == HIGH ? LOW : HIGH;
  Serial.println("in h_step");
  digitalWrite(MHDIR, dir);
  digitalWrite(MHSTEP, HIGH);
  delayMicroseconds(MOTOR_POLLING_TIMEOUT);
  digitalWrite(MHSTEP, LOW);
}

void x_step(unsigned char dir) {
 dir = dir == HIGH ? LOW : HIGH;
 digitalWrite(MXDIR, dir);
 digitalWrite(MXSTEP, HIGH);
 delayMicroseconds(MOTOR_POLLING_TIMEOUT);
 digitalWrite(MXSTEP, LOW);
}

void y_step(unsigned char dir) {
  digitalWrite(MYDIR, dir);
  digitalWrite(MYSTEP, HIGH);
  delayMicroseconds(MOTOR_POLLING_TIMEOUT);
  digitalWrite(MYSTEP, LOW);
}

void zeroy() {
  while (digitalRead(LSWITCHY)) {
    y_step(DIR_MINUS);
  }
}

void zerox() {
  while (digitalRead(LSWITCHX)) {
    x_step(DIR_MINUS);
  }
}

void zeroh() {
  Serial.println("zeroing h");
  while (digitalRead(LSWITCHH)) {
    h_step(DIR_MINUS);
  }
  Serial.println("done zeroing h");
}

// our path
const unsigned long grid_x = 8520;
const unsigned long grid_y = 7408;

const unsigned grid_x_step = grid_x/9;
const unsigned grid_y_step = grid_y/5;

const unsigned long grid_path_size = grid_x*6;
const unsigned long grid_time_per_tick = 70;

unsigned long pos_x = 0;
unsigned long pos_y = 0;

unsigned long g_x = 0;
unsigned long g_y = 0;

void update_g(unsigned long d)
{
  g_x = (d % grid_x) + MAGIC_GRID_X_OFFSET;
  g_y = d/grid_x*grid_y_step + MAGIC_GRID_Y_OFFSET;
  Serial.println(g_x);
}

void move_towards() {
  for (int i=0; i<7; ++i)
    if (pos_x > g_x) {x_step(DIR_MINUS); --pos_x;}
    
  if (pos_x < g_x) {x_step(DIR_PLUS);  ++pos_x;}
  
  if (pos_y < g_y) {y_step(DIR_PLUS);  ++pos_y;}
  if (pos_y > g_y) {y_step(DIR_MINUS); --pos_y;}
}

const unsigned long stepsPerHour = 167;
const unsigned long totalHSteps = 2000;
unsigned long h = 0;
unsigned long h_pos = 0;
void update_h(unsigned long d)
{
  if (d < stepsPerHour*5)
    h = d;
  else
    h = d + 167;

      Serial.println(h);
}

void rotate_towards()
{
  if (h_pos < h) { h_step(DIR_PLUS); ++h_pos; }
  if (h_pos > h) { h_step(DIR_MINUS); --h_pos; }
}

void setup() {
  // Sets the two pins as Outputs
  Serial.begin(9600);

  // x motor setup
  pinMode(MXSTEP, OUTPUT);
  pinMode(MXDIR, OUTPUT);
  pinMode(MXEN, OUTPUT);

  digitalWrite(MXEN, LOW);

  // y motor setup
  pinMode(MYSTEP, OUTPUT);
  pinMode(MYDIR, OUTPUT);
  pinMode(MYEN, OUTPUT);

  digitalWrite(MYEN, LOW);

  // clock setup
  pinMode(MHSTEP, OUTPUT);
  pinMode(MHDIR, OUTPUT);
  pinMode(MHEN, OUTPUT);

  digitalWrite(MHEN, LOW);

  // zeroing switch setup
  pinMode(LSWITCHY, INPUT_PULLUP);
  pinMode(LSWITCHX, INPUT_PULLUP);
  pinMode(LSWITCHH, INPUT_PULLUP);
  pinMode(HOURBUTTON, INPUT_PULLUP);
  pinMode(MINBUTTON, INPUT_PULLUP);

  zerox();
  zeroy();
  //zeroh();
  pos_x = pos_y = h_pos = 0;
}

unsigned long dist = 0;
unsigned long h_dist = 0;
unsigned long distTimeCtr = 0;
unsigned long hourTimeCtr = 0;
unsigned long time = 0;
unsigned long lastTime = 0;
void loop()
{
  time = millis();
  if (lastTime == 0) lastTime = time;
  unsigned dt = time - lastTime;
  lastTime = time;
  
  distTimeCtr += dt;
  hourTimeCtr += dt;

  // hour handling
  if (digitalRead(HOURBUTTON) == LOW)
    ++h_dist;
    
  if (hourTimeCtr >= HOUR_TIME_MILLI_TICK)
  {
    ++h_dist;
    hourTimeCtr -= HOUR_TIME_MILLI_TICK;
  }

  // normal handling of distance increment
  if (distTimeCtr >= grid_time_per_tick/10)
  {
    dist++;
    distTimeCtr -= grid_time_per_tick/10;
  }

 if (digitalRead(MINBUTTON) == LOW)
   dist++;

  if (dist >= grid_path_size) dist -= grid_path_size;

  // updating of gantry towards global position
  update_g(dist);
  update_h(h_dist);
  move_towards(); //move motors towards minute place
  rotate_towards();
}
