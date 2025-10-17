// Serial control for D22, D26, D27 (Arduino Mega) @115200
// Most blue relay boards are active-LOW (LOW = ON). Flip ACTIVE_LOW if needed.

bool ACTIVE_LOW = true;   // send 'i' in Serial to toggle at runtime

const uint8_t PINS[] = {22, 26, 27};

inline void setOn(uint8_t p){ digitalWrite(p, ACTIVE_LOW ? LOW  : HIGH); }
inline void setOff(uint8_t p){ digitalWrite(p, ACTIVE_LOW ? HIGH : LOW ); }
inline bool isOn(uint8_t p){ return ACTIVE_LOW ? (digitalRead(p)==LOW) : (digitalRead(p)==HIGH); }

String line;

void setup(){
  Serial.begin(115200);
  for (uint8_t i=0;i<sizeof(PINS);i++){
    pinMode(PINS[i], OUTPUT);
    setOff(PINS[i]);
  }
  Serial.println(F("Commands: on <pin> | off <pin> | pulse <pin> <ms> | all on | all off | i | s"));
  Serial.println(F("Pins: 22, 26, 27"));
}

void loop(){
  while (Serial.available()){
    char c = Serial.read();
    if (c=='\r') continue;
    if (c=='\n'){ handle(line); line=""; }
    else line += c;
  }
}

bool validPin(int p){
  for (uint8_t i=0;i<sizeof(PINS);i++) if (PINS[i]==p) return true;
  return false;
}

void handle(const String& cmd){
  if (!cmd.length()) return;

  if (cmd=="all on"){
    for (uint8_t i=0;i<sizeof(PINS);i++) setOn(PINS[i]);
    Serial.println(F("ALL ON"));
    return;
  }
  if (cmd=="all off"){
    for (uint8_t i=0;i<sizeof(PINS);i++) setOff(PINS[i]);
    Serial.println(F("ALL OFF"));
    return;
  }
  if (cmd=="i"){
    ACTIVE_LOW = !ACTIVE_LOW;
    Serial.println(ACTIVE_LOW ? F("Polarity: LOW=ON") : F("Polarity: HIGH=ON"));
    // keep outputs consistent after flip
    for (uint8_t i=0;i<sizeof(PINS);i++) if (isOn(PINS[i])) setOn(PINS[i]); else setOff(PINS[i]);
    return;
  }
  if (cmd=="s"){
    Serial.print(F("Polarity=")); Serial.println(ACTIVE_LOW?F("LOW=ON"):F("HIGH=ON"));
    for (uint8_t i=0;i<sizeof(PINS);i++){
      Serial.print(F("D")); Serial.print(PINS[i]); Serial.print('=');
      Serial.println(isOn(PINS[i])?F("ON"):F("OFF"));
    }
    return;
  }

  // parse 2- or 3-part commands
  int sp1 = cmd.indexOf(' ');
  String op = (sp1==-1) ? cmd : cmd.substring(0, sp1);
  String rest = (sp1==-1) ? ""  : cmd.substring(sp1+1);

  if (op=="on" || op=="off"){
    int pin = rest.toInt();
    if (!validPin(pin)){ Serial.println(F("Bad pin. Use 22, 26, 27.")); return; }
    if (op=="on")  setOn(pin); else setOff(pin);
    Serial.print(F("D")); Serial.print(pin); Serial.print(F(" ")); Serial.println(op=="on"?F("ON"):F("OFF"));
    return;
  }

  if (op=="pulse"){
    int sp2 = rest.indexOf(' ');
    if (sp2==-1){ Serial.println(F("Usage: pulse <pin> <ms>")); return; }
    int pin = rest.substring(0, sp2).toInt();
    unsigned long ms = rest.substring(sp2+1).toInt();
    if (!validPin(pin)){ Serial.println(F("Bad pin. Use 22, 26, 27.")); return; }
    if (ms==0) ms = 1000;
    Serial.print(F("Pulse D")); Serial.print(pin); Serial.print(F(" ")); Serial.print(ms); Serial.println(F(" ms"));
    setOn(pin); delay(ms); setOff(pin);
    Serial.println(F("Done"));
    return;
  }

  Serial.println(F("Unknown cmd. Use: on/off <pin>, pulse <pin> <ms>, all on/off, i, s"));
}
