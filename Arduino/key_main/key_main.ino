/*
 * FIC_2(Fast Input Controller Ver2)
 * 2025.05.01 初版
 *
 * 片手入力デバイス用のキー入力ソフト
 * TCSW x9, ロータリーエンコーダ x1 
 * 最大3レイヤ、9個のキーを割り当て可能
 * 日本語配列を使用するため、#include <Keyboard_jp.h>　が必須
 * keyboard_jp.h の入手方法：
 * ①https://github.com/I-himawari/Keyboard_jp/tree/main からKeyboard.cppとkeyboard.hを入手
 * ②"C:\Users\(ユーザー名)\Documents\Arduino\libraries\Keyboard_jp" のディレクトリを作成し、
 * ③そこにKeyboard_jp.cppとKeyboard_jp.hとしてリネームして配置すること
 * ブロック図は"FIC2.drawio"を参照
 */
#include <Mouse.h>
#include <Keyboard_jp.h>

#define DEBUG_LED 1  // 1:使用中にLEDをON / 0:OFF

/* SWのスキャン状態の遷移 */
#define SCAN_SW 0
#define TURN_ON 1
#define DETECT_SW 2

/*キーボードのレイヤー分け*/
#define LAYER_1 1
#define LAYER_2 2
#define LAYER_3 3
#define MAX_LAYERS 3

#define SW_DATA_MAX 0x1FF  // 9bit(SWの数) MAX値

/* SW判定個別検出用 */
#define DET_SW2 0x001
#define DET_SW3 0x002
#define DET_SW4 0x004
#define DET_SW5 0x008
#define DET_SW6 0x010
#define DET_SW7 0x020
#define DET_SW8 0x040
#define DET_SW9 0x080
#define DET_SW10 0x100

const int SW[9] = {
  // SW2, SW3, SW4, SW5, SW6, SW7, SW8, SW9, SW10
  7, 4, 12, A0, A5, A4, A3, A2, A1
};

/* 接続端子定義 */
const int ENC_A = 2;  // エンコーダA相
const int ENC_B = 3;  // エンコーダB相
const int LED_P = 6;  // Pcbnew側LED
const int LED_E = 5;  // Eeschema側LED
const int STAT = 13;  // ステータス用LED

const int enc_table[16] = {
  0, 1, -1, 0, -1, 0, 0, 1,
  1, 0, 0, -1, 0, -1, 1, 0
};

/* グローバル変数 */
int pattern = SCAN_SW;  // SWのスキャン状態を初期化
int layer = LAYER_1;
long cnt_enc = 0;     // エンコーダのカウント値
long oldcnt_enc = 0;  // 1サイクル前のエンコーダ値
short dir = 0;        // 0:静止 1:+方向 -1:-方向
int now_sw, old_sw;
unsigned long nowtime, starttime;

/* プロトタイプ宣言 */
void wheel_go(unsigned char led_port);
int get_sw(void);
void sw_process(void);
int get_layer_change(void);
void debug_led(unsigned char led_pcb, unsigned char led_esm, int layer);

void setup() {
  // put your setup code here, to run once:
  int i;
  // 割り込み入力
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);

  // SWの入力をINPUTに設定
  for (i = 0; i < sizeof(SW) / sizeof(SW[0]); i++) {
    pinMode(SW[i], INPUT);
  }
  pinMode(LED_P, OUTPUT);
  pinMode(LED_E, OUTPUT);
  pinMode(STAT, OUTPUT);
  attachInterrupt(0, enc_read, CHANGE);  // INT0
  attachInterrupt(1, enc_read, CHANGE);  // INT1

  // 出力ポート初期化
  digitalWrite(LED_E, HIGH);
  digitalWrite(LED_P, HIGH);
  digitalWrite(STAT, HIGH);
  Keyboard.begin();
  Mouse.begin();

  debug_led(LED_P, LED_E, layer);  // レイヤ切り替え状態を2進数表示
  //Serial.begin(115200);
  //while (!Serial);
  starttime = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  nowtime = millis();

  if (nowtime - starttime > 1) {
    starttime = nowtime;
    wheel_go(STAT);
    sw_process();
    //Serial.println(get_sw(), BIN);
  }
}
/************************************************************************/
/* ロータリーエンコーダ読み取り(ピン2,3の外部割込みで実行)                 */
/* 引数 なし                                                             */
/* 戻り値 なし                                                           */
/* メモ attachInterruptで指定                                            */
/************************************************************************/
void enc_read(void) {
  static unsigned char sum_cur = 0;
  sum_cur = (sum_cur << 1) + digitalRead(ENC_A);
  sum_cur = (sum_cur << 1) + digitalRead(ENC_B);
  sum_cur &= 0x0f;
  cnt_enc += enc_table[sum_cur];
}

/************************************************************************/
/* ロータリーエンコーダ操作                                              */
/* 引数 ステータスLEDの端子                                              */
/* 戻り値 なし                                                           */
/* メモ attachInterruptで指定                                            */
/************************************************************************/
void wheel_go(unsigned char led_port) {
  int enc_diff;
  enc_diff = (cnt_enc - oldcnt_enc);
  if (enc_diff > 0) {
    dir = 1;
    //Keyboard.press(KEY_LEFT_CTRL);
    Mouse.move(0, 0, enc_diff);
#if DEBUG_LED
    digitalWrite(led_port, LOW);
#endif  // DEBUG_LED
    digitalWrite(led_port, LOW);
  } else if (enc_diff < 0) {
    dir = -1;
    //Keyboard.press(KEY_LEFT_CTRL);
    Mouse.move(0, 0, enc_diff);
    digitalWrite(led_port, LOW);
  } else if (enc_diff == 0) {
    dir = 0;
    digitalWrite(led_port, HIGH);
  }
  oldcnt_enc = cnt_enc;
}


/************************************************************************/
/* SW取得(10個)                                                          */
/* 引数 なし　　　　　　　　                                              */
/* 戻り値 1:ON 0:OFF                                                     */
/************************************************************************/
int get_sw(void) {
  int ret;
  unsigned char sw_stat[9];
  int i;
  int portdata = 0x001;

  // SWの状態を一気に取得
  for (i = 0; i < sizeof(sw_stat) / sizeof(sw_stat[0]); i++) {
    sw_stat[i] = digitalRead(SW[i]);
  }
  ret = 0;
  for (i = 0; i < sizeof(sw_stat) / sizeof(sw_stat[0]); i++) {
    ret += sw_stat[i] * portdata;
    portdata <<= 1;
  }
  ret = SW_DATA_MAX - ret;  // 9bitの最大値(511)
  return ret;
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
        cnt0 = millis();
      }
      break;

    // SWのどれかが押された時
    case TURN_ON:
      cnt_cht = millis();
      layer_ischange = get_layer_change();
      if (layer_ischange == 1) {   // レイヤ変更があった場合
        layer++;                   // レイヤの階層を1進める
        if (layer > MAX_LAYERS) {  // レイヤ数上限に達した場合は最初に戻す
          layer = LAYER_1;
        }
        for (i = 0; i < 3; i++) {  // レイヤ切り替えをLED点滅で表示
          debug_led(LED_P, LED_E, 3);
          delay(100);
          debug_led(LED_P, LED_E, 0);
          delay(100);
        }
        debug_led(LED_P, LED_E, layer);  // レイヤ切り替え状態を2進数表示
        while (get_sw() > 0)
          ;                 // 同時押しが離されるまで待つ
        pattern = SCAN_SW;  // 検出状態を最初に戻す

      } else {                      // レイヤー変更の場合はのちに押されるSW検出を無視する
        if (cnt_cht - cnt0 > 40) {  // チャタリング防止:40ms
          pattern = DETECT_SW;
          cnt0 = cnt_cht;
        }
      }
      break;

    // ここから下にSWが押された時の挙動を書く
    case DETECT_SW:

      switch (layer) {
        case LAYER_1:  // レイヤ1(初期状態)のキー設定割りあて:
          // SW2
          if (now_sw & DET_SW2) {
            Keyboard.press(KEY_ESC);
          }
          // SW3
          if (now_sw & DET_SW3) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('z');
          }
          // SW4
          if (now_sw & DET_SW4) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('s');
          }
          // SW5
          if (now_sw & DET_SW5) {
            //  Keyboard.press(KEY_F2);
            Keyboard.press('_');
          }
          // SW6
          if (now_sw & DET_SW6) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('f');
          }
          // SW7
          if (now_sw & DET_SW7) {
            Keyboard.press(KEY_LEFT_GUI);
            Keyboard.press('r');
          }
          // SW8
          if (now_sw & DET_SW8) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('c');
          }
          // SW9
          if (now_sw & DET_SW9) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('v');
          }
          // SW10
          if (now_sw & DET_SW10) {
            Keyboard.press(KEY_RETURN);
          }

          break;

        case LAYER_2:  // レイヤ2のキー設定割りあて:

          // SW2
          if (now_sw & DET_SW2) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('a');
          }
          // SW3
          if (now_sw & DET_SW3) {
            Keyboard.press(KEY_F4);
          }
          // SW4
          if (now_sw & DET_SW4) {
            Keyboard.press('+');
          }
          // SW5
          if (now_sw & DET_SW5) {
            Keyboard.press('-');
          }
          // SW6
          if (now_sw & DET_SW6) {
            Keyboard.press('*');
          }
          // SW7
          if (now_sw & DET_SW7) {
            Keyboard.press('/');
          }
          // SW8
          if (now_sw & DET_SW8) {
            Keyboard.press('(');
          }
          // SW9
          if (now_sw & DET_SW9) {
            Keyboard.press(')');
          }
          // SW10
          if (now_sw & DET_SW10) {
            Keyboard.press(KEY_RETURN);
          }

          break;

        case LAYER_3:  // レイヤ3のキー設定割りあて:
                       // SW2
          if (now_sw & DET_SW2) {
            Keyboard.press(KEY_ESC);
          }
          // SW3
          if (now_sw & DET_SW3) {
            Keyboard.press(KEY_LEFT_GUI);
            Keyboard.press('v');
          }
          // SW4
          if (now_sw & DET_SW4) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('s');
          }
          // SW5
          if (now_sw & DET_SW5) {
            Keyboard.press(KEY_F2);
          }
          // SW6
          if (now_sw & DET_SW6) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('f');
          }
          // SW7
          if (now_sw & DET_SW7) {
            Keyboard.press(KEY_LEFT_GUI);
            Keyboard.press('r');
          }
          // SW8
          if (now_sw & DET_SW8) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('c');
          }
          // SW9
          if (now_sw & DET_SW9) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.press('v');
          }
          // SW10
          if (now_sw & DET_SW10) {
            Keyboard.press(KEY_RETURN);
          }
          break;

        default:
          /* NOT REACHED */
          break;
      }
      Keyboard.releaseAll();  // 押しているキーがある場合は離す
      pattern = SCAN_SW;      // 初期状態に戻す
      break;

    default:
      /* NOT REACHED */
      break;
  }

  old_sw = now_sw;
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
  if ((s & DET_SW8) && (s & DET_SW9)) {
    ret = 1;
  }
  return ret;
}

/************************************************************************/
/* debug用LED                                                           */
/* 引数 led_pcb:PCBnew用LED  led_esm:Eschema用LED layer:現在のレイヤ数    */
/* 戻り値 1:レイヤ変更検出 0:レイヤ変更しない(そのまま)                    */
/* メモ 2bitのLEDとして現在のレイヤを識別するために使用                    */
/************************************************************************/
void debug_led(unsigned char led_pcb, unsigned char led_esm, int layer) {
  int portdata;

  portdata = ~layer;

#if DEBUG_LED
  digitalWrite(led_pcb, (0x02 & portdata) >> 1);
  digitalWrite(led_esm, 0x01 & portdata);
#endif  // DEBUG_LED
}
