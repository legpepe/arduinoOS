#include <SparkFunColorLCDShield.h>
#include <EEPROM.h>

#define S_RIGHT A2
#define S_UP    A1
#define S_LEFT  A0
#define S_DOWN  A4
#define S_SEL   A3 
#define BEEP    A5 

LCDShield lcd;
uint8_t mIdx = 0, gIdx = 0, selCount = 0, cst = 45, vol = 2; 
bool inApps = false, inGames = false, redrawMenu = true, sndOn = true; 
bool appsOn[4] = {0,0,0,0};

int px, py, ox, oy, sc, bestSc;
int sx[20], sy[20], sl, sd; 
unsigned long exitT, frameT, gameTimer;
const char* gn[] = {"SNAKE", "RACING", "SHOOTER", "SPACE INV"};

// Функция звука с регулировкой громкости
void playSnd(int freq, int dur) {
  if (!sndOn || vol == 0) return;
  int pulse = (vol == 1) ? 1 : (vol == 2) ? 5 : 10; // Длительность импульса для громкости
  long delayMicro = 1000000L / freq / 2;
  long startTime = millis();
  while (millis() - startTime < dur) {
    digitalWrite(BEEP, HIGH); delayMicroseconds(delayMicro / (11 - pulse)); 
    digitalWrite(BEEP, LOW);  delayMicroseconds(delayMicro);
  }
}

void setup() {
  lcd.init(PHILIPS); 
  cst = EEPROM.read(100); if(cst > 100 || cst < 20) cst = 45;
  vol = EEPROM.read(102); if(vol > 3) vol = 2; // Загрузка громкости
  sndOn = (EEPROM.read(101) != 0); 
  lcd.contrast(cst);
  for(uint8_t i=0; i<4; i++) appsOn[i] = (EEPROM.read(50+i) == 1);
  pinMode(S_UP, INPUT_PULLUP); pinMode(S_DOWN, INPUT_PULLUP);
  pinMode(S_LEFT, INPUT_PULLUP); pinMode(S_RIGHT, INPUT_PULLUP);
  pinMode(S_SEL, INPUT_PULLUP); 
  pinMode(12, OUTPUT); pinMode(BEEP, OUTPUT);
  lcd.clear(WHITE);
  drawMain();
}

void drawStats() {
  char buf[8];
  lcd.setStr("S:", 2, 2, BLACK, WHITE);
  itoa(sc, buf, 10); lcd.setStr(buf, 2, 15, RED, WHITE);
  lcd.setStr("B:", 2, 70, BLACK, WHITE);
  itoa(bestSc, buf, 10); lcd.setStr(buf, 2, 85, BLUE, WHITE);
}

void drawMain() {
  lcd.clear(WHITE);
  lcd.setStr("ArduinoOS", 6, 25, BLACK, WHITE);
  const char* mn[] = {"GAMES", "LED CFG", "APP STORE", "RECORDS", "SETTINGS"}; 
  for (uint8_t i = 0; i < 5; i++) {
    lcd.setStr((char*)(mIdx == i ? ">" : " "), 25+(i*15), 5, RED, WHITE);
    lcd.setStr((char*)mn[i], 25+(i*15), 25, (mIdx == i) ? RED : BLACK, WHITE);
  }
}

void resetApp() {
  if (sc > bestSc) EEPROM.update(60 + gIdx, sc);
  if (sc > 0) playSnd(150, 100); 
  lcd.clear(WHITE); sc = 0; selCount = 0; redrawMenu = true;
  bestSc = EEPROM.read(60 + gIdx);
  if(inGames) {
    if(gIdx == 0) { 
      sl=3; sd=0; sx[0]=64; sy[0]=64; sx[1]=56; sy[1]=64; sx[2]=48; sy[2]=64;
      ox=(random(1, 14)*8); oy=(random(4, 14)*8); 
    }
    else if(gIdx == 1) { px=60; py=110; ox=random(20,110); oy=-20; }
    else if(gIdx == 2) { px=60; py=60; ox=10; oy=10; }
    else if(gIdx == 3) { px=60; py=115; oy=-10; for(int i=0; i<5; i++) { sx[i]=20+i*20; sy[i]=30; } }
  }
}

void loop() {
  if (!inApps && !inGames) {
    if (digitalRead(S_DOWN) == LOW) { mIdx = (mIdx + 1) % 5; drawMain(); delay(150); }
    if (digitalRead(S_UP) == LOW)   { mIdx = (mIdx + 4) % 5; drawMain(); delay(150); }
    if (digitalRead(S_SEL) == LOW) { playSnd(1500, 30); if (mIdx == 0) inGames = true; else inApps = true; resetApp(); delay(250); }
  } 
  else if (inGames && !inApps) {
    if(redrawMenu) { lcd.clear(WHITE); lcd.setStr("GAMES", 5, 40, BLUE, WHITE); redrawMenu = false; }
    for(uint8_t i=0; i<4; i++) {
      if(!appsOn[i]) continue;
      lcd.setStr((char*)(gIdx == i ? ">" : " "), 30+(i*15), 5, RED, WHITE);
      lcd.setStr((char*)gn[i], 30+(i*15), 25, (gIdx == i ? RED : BLACK), WHITE);
    }
    if (digitalRead(S_DOWN) == LOW) { gIdx = (gIdx + 1) % 4; delay(150); }
    if (digitalRead(S_UP) == LOW)   { gIdx = (gIdx + 3) % 4; delay(150); }
    if (digitalRead(S_SEL) == LOW && appsOn[gIdx]) { playSnd(1500, 30); inApps = true; resetApp(); delay(250); }
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
  if (inGames) {
    drawStats();
    switch(gIdx) {
      case 0: // SNAKE
        if (millis() - gameTimer < 150) return; gameTimer = millis();
        lcd.setRect(sy[sl-1], sx[sl-1], sy[sl-1]+7, sx[sl-1]+7, 1, WHITE);
        if(digitalRead(S_UP)==LOW && sd!=3) sd=1; else if(digitalRead(S_DOWN)==LOW && sd!=1) sd=3;
        else if(digitalRead(S_LEFT)==LOW && sd!=0) sd=2; else if(digitalRead(S_RIGHT)==LOW && sd!=2) sd=0;
        for(int i=sl-1; i>0; i--) { sx[i]=sx[i-1]; sy[i]=sy[i-1]; }
        if(sd==0) sx[0]+=8; if(sd==2) sx[0]-=8; if(sd==1) sy[0]-=8; if(sd==3) sy[0]+=8;
        if(sx[0] == ox && sy[0] == oy) { sl=min(sl+1, 19); sc++; playSnd(2000, 20); ox=(random(1, 14)*8); oy=(random(4, 14)*8); }
        if(sx[0]<0 || sx[0]>120 || sy[0]<15 || sy[0]>120) resetApp();
        for(int i=0; i<sl; i++) lcd.setRect(sy[i], sx[i], sy[i]+7, sx[i]+7, 1, GREEN);
        lcd.setRect(oy, ox, oy+7, ox+7, 1, RED); break;

      case 1: // RACING
        lcd.setRect(oy, ox, oy+12, ox+10, 1, WHITE); lcd.setRect(py, px, py+12, px+10, 1, WHITE);
        if(digitalRead(S_LEFT)==LOW && px>5) px-=8; if(digitalRead(S_RIGHT)==LOW && px<110) px+=8;
        oy+=10; if(oy>130) { oy=-20; ox=random(10,110); sc++; playSnd(1000, 10); }
        if(oy+12>py && abs(px-ox)<10) resetApp();
        lcd.setRect(oy, ox, oy+12, ox+10, 1, BLACK); lcd.setRect(py, px, py+12, px+10, 1, BLUE); break;

      case 2: // SHOOTER
        lcd.setRect(oy, ox, oy+8, ox+8, 1, WHITE); lcd.setRect(py, px, py+8, px+8, 1, WHITE);
        if(digitalRead(S_LEFT)==LOW && px>5) px-=6; if(digitalRead(S_RIGHT)==LOW && px<115) px+=6;
        if(digitalRead(S_UP)==LOW && py>20) py-=6; if(digitalRead(S_DOWN)==LOW && py<115) py+=6;
        ox += (ox < px) ? 2 : -2; oy += (oy < py) ? 2 : -2;
        if(digitalRead(S_SEL)==LOW) { playSnd(3000, 10); lcd.setLine(px+4, py+4, ox+4, oy+4, RED); sc++; ox=random(10,110); oy=0; delay(30); lcd.clear(WHITE); }
        if(abs(px-ox)<7 && abs(py-oy)<7) resetApp();
        lcd.setRect(oy, ox, oy+8, ox+8, 1, BLACK); lcd.setRect(py, px, py+8, px+8, 1, BLUE); break;

      case 3: // SPACE
        lcd.setRect(py, px, py+6, px+12, 1, WHITE); lcd.setRect(oy, ox, oy+5, ox+2, 1, WHITE);
        if(digitalRead(S_LEFT)==LOW && px>5) px-=8; if(digitalRead(S_RIGHT)==LOW && px<110) px+=8;
        if(oy < 0) { ox=px+5; oy=py-6; } else oy-=12;
        for(int i=0; i<3; i++) { 
          lcd.setRect(sy[i], sx[i], sy[i]+6, sx[i]+10, 1, WHITE); sy[i]+=2;
          if(sy[i]>120) resetApp(); if(oy>0 && abs(ox-sx[i])<10 && abs(oy-sy[i])<6) { playSnd(2500, 10); sy[i]=-20; oy=-10; sc++; }
          lcd.setRect(sy[i], sx[i], sy[i]+6, sx[i]+10, 1, RED); 
        }
        lcd.setRect(py, px, py+6, px+12, 1, BLUE); if(oy>0) lcd.setRect(oy, ox, oy+5, ox+2, 1, BLACK); break;
    }
  } else {
    switch(mIdx) {
      case 1: // LED
        if(redrawMenu) { lcd.clear(WHITE); lcd.setStr("LED", 5, 50, BLUE, WHITE); redrawMenu=false; }
        if(digitalRead(S_UP)==LOW) digitalWrite(12, 1); if(digitalRead(S_DOWN)==LOW) digitalWrite(12, 0);
        lcd.setStr(digitalRead(12)?"ON ":"OFF", 70, 50, RED, WHITE); break;
      case 4: // SETTINGS
        static uint8_t sS = 0;
        if(redrawMenu) { lcd.clear(WHITE); lcd.setStr("SETTINGS", 5, 30, BLUE, WHITE); lcd.setStr("BRIGHT", 50, 20, BLACK, WHITE); lcd.setStr("VOLUME", 70, 20, BLACK, WHITE); lcd.setStr("ERASE", 90, 20, RED, WHITE); redrawMenu=false; }
        lcd.setStr(sS==0?">":" ", 50, 5, RED, WHITE); lcd.setStr(sS==1?">":" ", 70, 5, RED, WHITE); lcd.setStr(sS==2?">":" ", 90, 5, RED, WHITE);
        if(digitalRead(S_DOWN)==LOW) { sS = (sS + 1) % 3; delay(150); }
        if(sS == 0) { // Яркость
          char cB[4]; itoa(cst, cB, 10); lcd.setStr(cB, 50, 95, RED, WHITE);
          if(digitalRead(S_RIGHT)==LOW && cst < 63) { cst++; lcd.contrast(cst); EEPROM.update(100, cst); delay(50); }
          if(digitalRead(S_LEFT)==LOW && cst > 20) { cst--; lcd.contrast(cst); EEPROM.update(100, cst); delay(50); }
        } else if(sS == 1) { // Громкость
          const char* vL[] = {"OFF", "LOW", "MID", "MAX"}; lcd.setStr((char*)vL[vol], 70, 95, RED, WHITE);
          if(digitalRead(S_RIGHT)==LOW) { vol = (vol + 1) % 4; EEPROM.update(102, vol); playSnd(1500, 40); delay(200); }
        } else if(sS == 2 && digitalRead(S_SEL)==LOW) { for(int i=0; i<256; i++) EEPROM.write(i, 0); setup(); }
        if(digitalRead(S_LEFT)==LOW && sS != 0) { inApps=false; drawMain(); delay(250); } break;
      case 2: // STORE
        if(redrawMenu) { lcd.clear(WHITE); lcd.setStr("STORE", 5, 45, RED, WHITE); redrawMenu=false; }
        for(int i=0; i<4; i++) { lcd.setStr((char*)gn[i], 30+i*15, 5, (gIdx==i?RED:BLACK), WHITE); lcd.setStr(appsOn[i]?"ON ":"OFF", 30+i*15, 85, appsOn[i]?GREEN:GRAY, WHITE); }
        if(digitalRead(S_DOWN)==LOW) { gIdx=(gIdx+1)%4; delay(150); } if(digitalRead(S_UP)==LOW) { gIdx=(gIdx+3)%4; delay(150); }
        if(digitalRead(S_RIGHT)==LOW) { appsOn[gIdx]=!appsOn[gIdx]; EEPROM.update(50+gIdx, appsOn[gIdx]); delay(200); }
        if(digitalRead(S_LEFT)==LOW) { inApps=false; drawMain(); delay(250); } break;
      case 3: // RECORDS
        if(redrawMenu) { lcd.clear(WHITE); lcd.setStr("RECORDS", 5, 30, BLUE, WHITE); for(int i=0; i<4; i++) { lcd.setStr((char*)gn[i], 30+i*15, 5, BLACK, WHITE); char vB[6]; itoa(EEPROM.read(60+i), vB, 10); lcd.setStr(vB, 30+i*15, 85, RED, WHITE); } redrawMenu=false; }
        if(digitalRead(S_LEFT)==LOW) { inApps=false; drawMain(); delay(250); } break;
    }
  }
}
