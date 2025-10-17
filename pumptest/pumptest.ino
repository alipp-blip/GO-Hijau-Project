// D24 & D25 ON/OFF via Serial (115200)
bool ACTIVE_LOW = true; // flip to false if needed

inline void setOff(uint8_t pin){ digitalWrite(pin, ACTIVE_LOW ? HIGH : LOW); }
inline void setOn (uint8_t pin){ digitalWrite(pin, ACTIVE_LOW ? LOW  : HIGH); }

String line;
void setup(){
  Serial.begin(115200);
  pinMode(24, OUTPUT); pinMode(25, OUTPUT);
  setOff(24); setOff(25);
  Serial.println(F("Commands: off | on | i (invert) | s (status)"));
}
void loop(){
  while (Serial.available()){
    char c = Serial.read();
    if (c=='\r') continue;
    if (c=='\n'){ handle(line); line=""; }
    else line += c;
  }
}
void handle(const String& cmd){
  if (cmd=="off"){ setOff(24); setOff(25); Serial.println(F("D24,D25 OFF")); }
  else if (cmd=="on"){ setOn(24); setOn(25); Serial.println(F("D24,D25 ON")); }
  else if (cmd=="i"){ ACTIVE_LOW=!ACTIVE_LOW; Serial.println(ACTIVE_LOW?F("LOW=ON"):F("HIGH=ON")); }
  else if (cmd=="s"){
    Serial.print(F("Polarity: ")); Serial.println(ACTIVE_LOW?F("LOW=ON"):F("HIGH=ON"));
  } else if (cmd.length()) Serial.println(F("Use: off | on | i | s"));
}
