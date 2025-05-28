#include <Adafruit_NeoPixel.h>
#include <Keyboard.h>

/* SWのスキャン状態の遷移 */
#define SCAN_SW 0
#define TURN_ON 1
#define DETECT_SW 2
#define RELOAD_SW 3

/*キーボードのレイヤー分け*/
#define LAYER_1 1
#define LAYER_2 2
#define LAYER_3 3
#define MAX_LAYERS 3

#define SW_DATA_MAX 0xFFFFF

/* SW判定個別検出用 */
#define DET_SW1   0x00001
#define DET_SW2   0x00002
#define DET_SW3   0x00004
#define DET_SW4   0x00008
#define DET_SW5   0x00010
#define DET_SW6   0x00020
#define DET_SW7   0x00040
#define DET_SW8   0x00080
#define DET_SW9   0x00100
#define DET_SW10  0x00200
#define DET_SW11  0x00400
#define DET_SW12  0x00800
#define DET_SW13  0x01000
#define DET_SW14  0x02000
#define DET_SW15  0x04000
#define DET_SW16  0x08000
#define DET_SW17  0x10000
#define DET_SW18  0x20000
#define DET_SW19  0x40000
#define DET_SW20  0x80000


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
  // COL0, COL1, COL2, COL3
  7, 6, 5, 4
};

const int LEDS[3] = {
  // LEFT, CENTER, RIGHT
  0, 1, 2
};
unsigned char sw_stat[20] = {0};

uint32_t nowtime, starttime;
int now_sw, old_sw;
int pattern;
int layer;
uint32_t datas;

unsigned long cnt0, cnt_cht;  // チャタリング防止のためタイマを設ける
int layer_ischange;           // 1:レイヤ変更がある場合 0:レイヤ変更が検出されない場合
int i;
int isclick, reload_time;

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
  layer = LAYER_1;
  Serial.begin(115200);         // ← これを必ず書く
  while (!Serial) {             // ← 書き込んだ後、シリアルが開くまで待機
    delay(10);
  }
  Keyboard.begin();
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
  debug_led(layer);

  starttime = millis();
}

void loop() {
  nowtime = millis();

  now_sw = get_sw();
  Serial.println(pattern);

  switch (pattern) {
    // SWの全スキャン
    case SCAN_SW:
      digitalWrite(LED_RIGHT, LOW);
      if (now_sw != old_sw) {
        pattern = TURN_ON;
        cnt0 = nowtime;
      }
      break;

    // SWのどれかが押された時
    case TURN_ON:
      nowtime = millis();
      isclick = 0; // 連続押し状態を初期化
      layer_ischange = get_layer_change();
      if (layer_ischange == 1) {   // レイヤ変更があった場合
        layer++;                   // レイヤの階層を1進める
        if (layer > MAX_LAYERS) {  // レイヤ数上限に達した場合は最初に戻す
          layer = LAYER_1;
        }

        for (i = 0; i < 3; i++) {  // レイヤ切り替えをLED点滅で表示
          digitalWrite(LED_LEFT, HIGH);
          digitalWrite(LED_CENTER, HIGH);
          digitalWrite(LED_RIGHT, HIGH);
          delay(100);
          digitalWrite(LED_LEFT, LOW);
          digitalWrite(LED_CENTER, LOW);
          digitalWrite(LED_RIGHT, LOW);
          delay(100);
        }
        debug_led(layer);  // レイヤ切り替え状態を表示
        while (now_sw > 0); // 同時押しが離されるまで待つ
        pattern = SCAN_SW;  // 検出状態を最初に戻す

      } else {
        if (nowtime - cnt0 > 40) { // チャタリング防止:40ms
          pattern = DETECT_SW;
          cnt0 = cnt_cht;
        }
      }
      break;

    // ここから下にSWが押された時の挙動を書く
    case DETECT_SW:
      digitalWrite(LED_RIGHT, HIGH);
      switch (layer) {
        case LAYER_1:
          /* ここにレイヤ1の内容を書く */
          if ( now_sw & DET_SW1 ) {
            //  Keyboard.press('+');
          }
          if ( now_sw & DET_SW2 ) {
            //  Keyboard.press('-');
          }
          if ( now_sw & DET_SW3 ) {
            //  Keyboard.press('*');
          }
          if ( now_sw & DET_SW4 ) {
            //  Keyboard.press('/');
          }
          if ( now_sw & DET_SW5 ) {
            Keyboard.press('7');
          }
          if ( now_sw & DET_SW6 ) {
            Keyboard.press('8');
          }
          if ( now_sw & DET_SW7 ) {
            Keyboard.press('9');
          }
          if ( now_sw & DET_SW8 ) {
            Keyboard.press('+');
          }
          if ( now_sw & DET_SW9 ) {
            Keyboard.press('4');
          }
          if ( now_sw & DET_SW10 ) {
            Keyboard.press('5');
          }
          if ( now_sw & DET_SW11 ) {
            Keyboard.press('6');
          }
          if ( now_sw & DET_SW12 ) {
            // Keyboard.press(KEY_RETURN);
          }
          if ( now_sw & DET_SW13 ) {
            Keyboard.press('1');
          }
          if ( now_sw & DET_SW14 ) {
            Keyboard.press('2');
          }
          if ( now_sw & DET_SW15 ) {
            Keyboard.press('3');
          }
          if ( now_sw & DET_SW16 ) {
            Keyboard.press(KEY_RETURN);
          }
          if ( now_sw & DET_SW17 ) {
            Keyboard.press('0');
          }
          if ( now_sw & DET_SW18 ) {
            //  Keyboard.press(')');
          }
          if ( now_sw & DET_SW19 ) {
            Keyboard.press('.');
          }
          if ( now_sw & DET_SW20 ) {
            //  Keyboard.press(')');
          }
          break;

        case LAYER_2:
          /* ここにレイヤ2の内容を書く */
          if ( now_sw & DET_SW1 ) {
            //  Keyboard.press(KEY_NUMPAD_PLUS);
          }
          if ( now_sw & DET_SW2 ) {
            //  Keyboard.press(KEY_NUMPAD_MINUS);
          }
          if ( now_sw & DET_SW3 ) {
            //  Keyboard.press(KEY_NUMPAD_ASTERIX);
          }
          if ( now_sw & DET_SW4 ) {
            //  Keyboard.press(KEY_NUMPAD_SLASH);
          }
          if ( now_sw & DET_SW5 ) {
            Keyboard.press(KEY_HOME);
          }
          if ( now_sw & DET_SW6 ) {
            Keyboard.press(KEY_UP_ARROW);
          }
          if ( now_sw & DET_SW7 ) {
            Keyboard.press(KEY_PAGE_UP);
          }
          if ( now_sw & DET_SW8 ) {
            Keyboard.press(KEY_BACKSPACE);
          }
          if ( now_sw & DET_SW9 ) {
            Keyboard.press(KEY_LEFT_ARROW);
          }
          if ( now_sw & DET_SW10 ) {
            Keyboard.press(KEY_RETURN);
          }
          if ( now_sw & DET_SW11 ) {
            Keyboard.press(KEY_RIGHT_ARROW);
          }
          if ( now_sw & DET_SW12 ) {
            // Keyboard.press(KEY_RETURN);
          }
          if ( now_sw & DET_SW13 ) {
            Keyboard.press(KEY_END);
          }
          if ( now_sw & DET_SW14 ) {
            Keyboard.press(KEY_DOWN_ARROW);
          }
          if ( now_sw & DET_SW15 ) {
            Keyboard.press(KEY_PAGE_DOWN);
          }
          if ( now_sw & DET_SW16 ) {
            Keyboard.press(KEY_RETURN);
          }
          if ( now_sw & DET_SW17 ) {
            Keyboard.press(KEY_INSERT);
          }
          if ( now_sw & DET_SW18 ) {
            //  Keyboard.press(')');
          }
          if ( now_sw & DET_SW19 ) {
            Keyboard.press(KEY_DELETE);
          }
          if ( now_sw & DET_SW20 ) {
            //  Keyboard.press(')');
          }
          break;

        case LAYER_3:
          /* ここにレイヤ3の内容を書く */
          if ( now_sw & DET_SW1 ) {
            //  Keyboard.press(KEY_NUMPAD_PLUS);
          }
          if ( now_sw & DET_SW2 ) {
            //  Keyboard.press(KEY_NUMPAD_MINUS);
          }
          if ( now_sw & DET_SW3 ) {
            //  Keyboard.press(KEY_NUMPAD_ASTERIX);
          }
          if ( now_sw & DET_SW4 ) {
            //  Keyboard.press(KEY_NUMPAD_SLASH);
          }
          if ( now_sw & DET_SW5 ) {
            Keyboard.press(KEY_HOME);
          }
          if ( now_sw & DET_SW6 ) {
            Keyboard.press(KEY_UP_ARROW);
          }
          if ( now_sw & DET_SW7 ) {
            Keyboard.press(KEY_PAGE_UP);
          }
          if ( now_sw & DET_SW8 ) {
            Keyboard.press(KEY_PRINT_SCREEN);
          }
          if ( now_sw & DET_SW9 ) {
            Keyboard.press(KEY_LEFT_ARROW);
          }
          if ( now_sw & DET_SW10 ) {
            Keyboard.press(KEY_RETURN);
          }
          if ( now_sw & DET_SW11 ) {
            Keyboard.press(KEY_RIGHT_ARROW);
          }
          if ( now_sw & DET_SW12 ) {
            // Keyboard.press(KEY_RETURN);
          }
          if ( now_sw & DET_SW13 ) {
            Keyboard.press(KEY_END);
          }
          if ( now_sw & DET_SW14 ) {
            Keyboard.press(KEY_DOWN_ARROW);
          }
          if ( now_sw & DET_SW15 ) {
            Keyboard.press(KEY_PAGE_DOWN);
          }
          if ( now_sw & DET_SW16 ) {
            Keyboard.press(KEY_RETURN);
          }
          if ( now_sw & DET_SW17 ) {
            Keyboard.press(KEY_ESC);
          }
          if ( now_sw & DET_SW18 ) {
            //  Keyboard.press(')');
          }
          if ( now_sw & DET_SW19 ) {
            Keyboard.press(KEY_DELETE);
          }
          if ( now_sw & DET_SW20 ) {
            //  Keyboard.press(')');
          }
          break;

        default:
          /*NOT REACHED*/
          break;
      }
      isclick++;
      Keyboard.releaseAll();  // 押しているキーがある場合は離す
      cnt0 = nowtime;
      pattern = RELOAD_SW;
      break;

    case RELOAD_SW:
      if (isclick == 1) reload_time = 350;
      else reload_time = 50;

      now_sw = get_sw();

      if (nowtime - cnt0 > reload_time ) {
        cnt0 = cnt_cht;
        if (now_sw > 0) {
          pattern = DETECT_SW;
        }
        else {
          pattern = SCAN_SW;
        }
      }
      else {
        if (now_sw == 0) {
          pattern = SCAN_SW;
        }
      }
      break;

    default:
      /* NOT REACHED */
      break;
  }

  old_sw = now_sw;
}

/************************************************************************/
/* SW取得(20個)                                                          */
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
  ret = SW_DATA_MAX - ret;
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
/* SWのレイヤ変更検出                                                    */
/* 引数 なし　　　　　　　　                                              */
/* 戻り値 1:レイヤ変更検出 0:レイヤ変更しない(そのまま)                    */
/************************************************************************/
int get_layer_change(void) {
  int ret;
  int s;
  ret = 0;
  s = get_sw();
  if ((s & DET_SW5) && (s & DET_SW6)) {
    ret = 1;
  }
  return ret;
}

/************************************************************************/
/* debug用LED                                                           */
/* 引数 led_pcb:PCBnew用LED  led_esm:Eschema用LED layer:現在のレイヤ数    */
/* 戻り値  なし　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　    */
/* メモ 2bitのLEDとして現在のレイヤを識別するために使用                    */
/************************************************************************/
void debug_led(int layer) {
  int i;

  // まず全部消す
  for (i = 0; i < sizeof(LEDS) / sizeof(LEDS[0]); i++) {
    digitalWrite(LEDS[i], LOW);
  }

  digitalWrite(LEDS[0], (0x02 & layer) >> 1);
  digitalWrite(LEDS[1], 0x01 & layer);
}
