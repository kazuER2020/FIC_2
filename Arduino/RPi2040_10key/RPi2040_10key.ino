#include <Adafruit_NeoPixel.h>

#define SCAN_SW 0
#define TURN_ON 1
#define DETECT_SW 2

#define SW_DATA_MAX 0xFFFFF

/* ポート定義 */
const int LED_PIN = 16;  // RPi2040上のRGB LED(GP16)
const int ROW0 = 28;
const int ROW1 = 27;
const int ROW2 = 26;
const int ROW3 = 15;
const int ROW4 = 14;

const int COL0 = 7;
const int COL1 = 6;
const int COL2 = 5;
const int COL3 = 4;

const int LED_LEFT = 0;
const int LED_CENTER = 1;
const int LED_RIGHT = 2;

const int rows[5] = {
  // ROW0, ROW1, ROW2, ROW3, ROW4
  28, 27, 26, 15, 14
};
const int cols[4] = {
  7, 6, 5, 4
};

const int LEDS[3] = {
  0, 1, 2
};
unsigned char sw_stat[20] = {0};

uint32_t nowtime, starttime;
int now_sw, old_sw;
int pattern;
uint32_t datas;

#define COLOR_REPEAT 2

// create a pixel strand with 1 pixel on PIN_NEOPIXEL
Adafruit_NeoPixel pixels(1, LED_PIN);

uint8_t color = 0, count = 0;
uint32_t colors[] = { pixels.Color(125, 0, 0), pixels.Color(0, 125, 0), pixels.Color(0, 0, 125), pixels.Color(125, 125, 125) };
const uint8_t COLORS_LEN = (uint8_t)(sizeof(colors) / sizeof(colors[0]));

void setup() {
  pinMode(ROW0, OUTPUT);
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);
  pinMode(ROW3, OUTPUT);
  pinMode(ROW4, OUTPUT);

  pinMode(COL0, INPUT_PULLUP);
  pinMode(COL1, INPUT_PULLUP);
  pinMode(COL2, INPUT_PULLUP);
  pinMode(COL3, INPUT_PULLUP);

  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_CENTER, OUTPUT);
  pinMode(LED_RIGHT, OUTPUT);

  pixels.begin();  // initialize the pixel(RPI2040上のRGB LED)
  pattern = SCAN_SW;
  Serial.begin(115200);         // ← これを必ず書く
  while (!Serial) {             // ← 書き込んだ後、シリアルが開くまで待機
    delay(10);
  }
  for (int i = 0; i < (sizeof(LEDS) / sizeof(LEDS[0])); i++) {
    digitalWrite(LEDS[i], HIGH);
    delay(300);
  }
  delay(200);
  for (int i = 2; i > -1; i--) {
    digitalWrite(LEDS[2 - i], LOW);
    delay(300);
  }

  Serial.println("Hello from RP2040!");


  starttime = millis();
}

void loop() {
  nowtime = millis();
  if (nowtime - starttime > 1) {
    starttime = nowtime;
    datas = get_sw();
    Serial.println(datas, BIN);
    sw_process();

  }
}

/************************************************************************/
/* SW取得(13個)                                                          */
/* 引数 なし　　　　　　　　                                              */
/* 戻り値 1:ON 0:OFF                                                     */
/************************************************************************/
uint32_t get_sw(void) {
  uint32_t ret;
  int i, j, k;
  int now_row;
  uint32_t portdata = 0x0001;

  k = 0; // 現在のSW読み込み位置
  for (i = 0; i < (sizeof(rows) / sizeof(rows[0])); i++) { // ROW側
    set_ROW(i);
    for (j = 0; j < (sizeof(cols) / sizeof(cols[0])); j++) { //COL側
      sw_stat[k] = digitalRead(cols[j]);
      k++;
    }
  }
  ret = 0;

  for (i = 0; i < (sizeof(sw_stat) / sizeof(sw_stat[0])); i++) {
    ret += sw_stat[i] * portdata;
    portdata <<= 1;
  }
  ret = SW_DATA_MAX - ret;  // 11bitの最大値(511)
  return ret;
}

void set_ROW(int row) {
  int i;
  // まず全部消す
  for (i = 0; i < 5; i++) {
    digitalWrite(rows[i], HIGH);
  }
  // ROWxxをGPIOの位置に変換
  digitalWrite(rows[row], LOW);
}

/************************************************************************/
/* SWのキーコード割り当て                                                 */
/* 引数 なし　　　　　　　　                                              */
/* 戻り値 なし                                                           */
/************************************************************************/
void sw_process(void) {
  unsigned long cnt0, cnt_cht;  // チャタリング防止のためタイマを設ける
  int layer_ischange;           // 1:レイヤ変更がある場合 0:レイヤ変更が検出されない場合
  int i;
  switch (pattern) {
    // SWの全スキャン
    case SCAN_SW:
      now_sw = get_sw();
      if (now_sw != old_sw) {
        pattern = TURN_ON;
        digitalWrite(LED_RIGHT, HIGH);
        cnt0 = millis();
      }
      break;

    // SWのどれかが押された時
    case TURN_ON:
      cnt_cht = millis();
      while (get_sw() > 0)
        ;                 // 同時押しが離されるまで待つ
      pattern = SCAN_SW;  // 検出状態を最初に戻す


      if (cnt_cht - cnt0 > 40) {  // チャタリング防止:40ms
        digitalWrite(LED_RIGHT, LOW);
        pattern = DETECT_SW;
        cnt0 = cnt_cht;
      }
      break;

    // ここから下にSWが押された時の挙動を書く
    case DETECT_SW:
      pattern = SCAN_SW;

      break;
  }
}
