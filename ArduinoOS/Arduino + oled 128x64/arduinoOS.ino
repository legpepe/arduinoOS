#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Пины кнопок (оставил как в оригинале)
#define S_RIGHT A2
#define S_UP    A1
#define S_LEFT  A0
#define S_DOWN  A4
#define S_SEL   A3 
#define BEEP    A5 

uint8_t mIdx = 0, gIdx = 0, selCount = 0, vol = 2; 
bool inApps = false, inGames = false, redrawMenu = true, sndOn = true; 
bool appsOn[4] = {1,1,1,1}; // Включил все игры по умолчанию

int px, py, ox, oy, sc, bestSc;
int sx[20], sy[20], sl, sd; 
unsigned long exitT, frameT, gameTimer;
const char* gn[] = {"SNAKE", "RACING", "SHOOTER", "SPACE INV"};

void playSnd(int freq, int dur) {
  if (!sndOn || vol == 0) return;
  tone(BEEP, freq, dur); // Использование стандартной функции tone
}

void setup() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  display.clearDisplay();
  
  vol = EEPROM.read(102); if(vol > 3) vol = 2;
  sndOn = (EEPROM.read(101) != 0); 
  
  pinMode(S_UP, INPUT_PULLUP); pinMode(S_DOWN, INPUT_PULLUP);
  pinMode(S_LEFT, INPUT_PULLUP); pinMode(S_RIGHT, INPUT_PULLUP);
  pinMode(S_SEL, INPUT_PULLUP); 
  pinMode(12, OUTPUT); pinMode(BEEP, OUTPUT);

  drawMain();
}

void drawStats() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("S:"); display.print(sc);
  display.setCursor(80, 0);
  display.print("B:"); display.print(bestSc);
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
}

void drawMain() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 0);
  display.print("ArduinoOS");
  
  const char* mn[] = {"GAMES", "LED CFG", "APP STORE", "RECORDS", "SETTINGS"}; 
  for (uint8_t i = 0; i < 5; i++) {
    display.setCursor(10, 15 + (i * 10));
    if (mIdx == i) display.print("> ");
    else display.print("  ");
    display.print(mn[i]);
  }
  display.display();
}

void resetApp() {
  if (sc > bestSc) EEPROM.update(60 + gIdx, sc);
  if (sc > 0) playSnd(150, 100); 
  display.clearDisplay(); 
  sc = 0; selCount = 0; redrawMenu = true;
  bestSc = EEPROM.read(60 + gIdx);
  
  if(inGames) {
    if(gIdx == 0) { // SNAKE
      sl=3; sd=0; sx[0]=64; sy[0]=32; sx[1]=56; sy[1]=32; sx[2]=48; sy[2]=32;
      ox=(random(1, 15)*8); oy=(random(2, 7)*8); 
    }
    else if(gIdx == 1) { px=60; py=50; ox=random(20,100); oy=-10; }
    else if(gIdx == 2) { px=60; py=30; ox=10; oy=10; }
    else if(gIdx == 3) { px=60; py=55; oy=-10; for(int i=0; i<5; i++) { sx[i]=20+i*20; sy[i]=15; } }
  }
}

void loop() {
  if (!inApps && !inGames) {
    if (digitalRead(S_DOWN) == LOW) { mIdx = (mIdx + 1) % 5; drawMain(); delay(150); }
    if (digitalRead(S_UP) == LOW)   { mIdx = (mIdx + 4) % 5; drawMain(); delay(150); }
    if (digitalRead(S_SEL) == LOW) { playSnd(1500, 30); if (mIdx == 0) inGames = true; else inApps = true; resetApp(); delay(250); }
  } 
  else if (inGames && !inApps) {
    display.clearDisplay();
    display.setCursor(45, 0); display.print("GAMES");
    for(uint8_t i=0; i<4; i++) {
      display.setCursor(10, 15 + (i * 10));
      display.print(gIdx == i ? "> " : "  ");
      display.print(gn[i]);
    }
    display.display();
    if (digitalRead(S_DOWN) == LOW) { gIdx = (gIdx + 1) % 4; delay(150); }
    if (digitalRead(S_UP) == LOW)   { gIdx = (gIdx + 3) % 4; delay(150); }
    if (digitalRead(S_SEL) == LOW) { playSnd(1500, 30); inApps = true; resetApp(); delay(250); }
    if (digitalRead(S_LEFT) == LOW) { inGames = false; drawMain(); delay(250); }
  }
  else {
    runApp();
    if (digitalRead(S_SEL) == LOW) {
      if (millis() - exitT > 300) { selCount++; exitT = millis(); }
      if (selCount >= 3) { resetApp(); inApps = false; inGames = false; drawMain(); delay(250); }
    }
  }
}

void runApp() {
  if (millis() - frameT < 35) return; frameT = millis();
  display.clearDisplay();
  if (inGames) {
    drawStats();
    switch(gIdx) {
      case 0: // SNAKE
        if(digitalRead(S_UP)==LOW && sd!=3) sd=1; else if(digitalRead(S_DOWN)==LOW && sd!=1) sd=3;
        else if(digitalRead(S_LEFT)==LOW && sd!=0) sd=2; else if(digitalRead(S_RIGHT)==LOW && sd!=2) sd=0;
        for(int i=sl-1; i>0; i--) { sx[i]=sx[i-1]; sy[i]=sy[i-1]; }
        if(sd==0) sx[0]+=8; if(sd==2) sx[0]-=8; if(sd==1) sy[0]-=8; if(sd==3) sy[0]+=8;
        if(sx[0] == ox && sy[0] == oy) { sl=min(sl+1, 19); sc++; playSnd(2000, 20); ox=(random(0, 15)*8); oy=(random(2, 7)*8); }
        if(sx[0]<0 || sx[0]>120 || sy[0]<10 || sy[0]>56) resetApp();
        for(int i=0; i<sl; i++) display.fillRect(sx[i], sy[i], 7, 7, SSD1306_WHITE);
        display.drawRect(ox, oy, 7, 7, SSD1306_WHITE); break;

      case 1: // RACING
        if(digitalRead(S_LEFT)==LOW && px>5) px-=8; if(digitalRead(S_RIGHT)==LOW && px<110) px+=8;
        oy+=4; if(oy>64) { oy=-15; ox=random(10,110); sc++; playSnd(1000, 10); }
        if(oy+10>py && abs(px-ox)<10) resetApp();
        display.fillRect(ox, oy, 10, 12, SSD1306_WHITE); 
        display.drawRect(px, py, 10, 12, SSD1306_WHITE); break;

      case 2: // SHOOTER
        if(digitalRead(S_LEFT)==LOW && px>5) px-=4; if(digitalRead(S_RIGHT)==LOW && px<115) px+=4;
        if(digitalRead(S_UP)==LOW && py>15) py-=4; if(digitalRead(S_DOWN)==LOW && py<55) py+=4;
        ox += (ox < px) ? 2 : -2; oy += (oy < py) ? 2 : -2;
        if(digitalRead(S_SEL)==LOW) { playSnd(3000, 10); display.drawLine(px+4, py+4, ox+4, oy+4, SSD1306_WHITE); sc++; ox=random(10,110); oy=0; }
        if(abs(px-ox)<7 && abs(py-oy)<7) resetApp();
        display.fillCircle(ox, oy, 4, SSD1306_WHITE); display.drawRect(px, py, 8, 8, SSD1306_WHITE); break;

      case 3: // SPACE
        if(digitalRead(S_LEFT)==LOW && px>5) px-=6; if(digitalRead(S_RIGHT)==LOW && px<110) px+=6;
        if(oy < 0) { ox=px+5; oy=py-6; } else oy-=8;
        for(int i=0; i<3; i++) { 
          sy[i]+=2; if(sy[i]>64) resetApp(); 
          if(oy>0 && abs(ox-sx[i])<10 && abs(oy-sy[i])<6) { playSnd(2500, 10); sy[i]=-20; oy=-10; sc++; }
          display.fillRect(sx[i], sy[i], 10, 6, SSD1306_WHITE); 
        }
        display.fillRect(px, py, 12, 6, SSD1306_WHITE); if(oy>0) display.drawPixel(ox, oy, SSD1306_WHITE); break;
    }
  } else {
     // Упрощенный вывод настроек/LED
     display.setCursor(10, 20);
     if (mIdx == 1) { 
        display.print("LED: "); display.print(digitalRead(12) ? "ON" : "OFF");
        if(digitalRead(S_UP)==LOW) digitalWrite(12, 1); if(digitalRead(S_DOWN)==LOW) digitalWrite(12, 0);
     } else {
        display.print("APP ACTIVE"); 
     }
  }
  display.display();
}
