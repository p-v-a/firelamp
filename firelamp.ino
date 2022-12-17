//#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <Arduino.h>

#define LED_PIN 2
#define N_LEDS 60

inline float RandomFloat()
{
  float r = random(1000000L) / 1000000.0f;
  return r;
}

class ConwaysGameOfLife {
  protected:
    int m_MatrixWidth;
    int m_MatrixHeight;

    bool ***m_Cells;
    uint32_t m_FrameIdx;

    /**
      Map logical x,y coordinate into pixel index
    **/
    uint16_t MapPixel(uint16_t x, uint16_t y) {
      return (m_MatrixHeight * (m_MatrixWidth - x) - 1) - y;
    }

  public:
    ConwaysGameOfLife (uint16_t matrixWidth = 5, uint16_t matrixHeight = 8) {
      m_MatrixWidth = matrixWidth;
      m_MatrixHeight = matrixHeight;

      m_Cells = new bool **[2];

      // Two frames needed
      m_Cells[0] = new bool *[m_MatrixWidth];
      m_Cells[1] = new bool *[m_MatrixWidth];
      for (int f = 0; f < 2; f++) {
        for (int i = 0; i < m_MatrixWidth; i++) {
          m_Cells[f][i] = new bool[m_MatrixHeight];
        }
      }

      for (int f = 0; f < 2; f++) {
        for (int y = m_MatrixHeight - 1; y >= 0; y--) {
          for (int x = m_MatrixWidth - 1; x >= 0; x--) {
            m_Cells[f][x][y] = false;
          }
        }
      }

      uint32_t m_LastDraw = millis();
      m_FrameIdx = 0;

      // Simplest oscilator pattern
      m_Cells[0][1][6] = true;
      m_Cells[0][1][7] = true;
      m_Cells[0][2][6] = true;
      m_Cells[0][2][7] = true;
      m_Cells[0][3][5] = true;
      m_Cells[0][3][4] = true;
      m_Cells[0][4][5] = true;
      m_Cells[0][4][4] = true;

      m_Cells[0][0][1] = true;
      m_Cells[0][1][1] = true;
      m_Cells[0][2][1] = true;
    }

    uint8_t GetNeighboursCount (int x, int y) {
      uint8_t neighbours = 0;
      // Check horizontal neighbours
      if (x + 1 < m_MatrixWidth) {
        if (m_Cells[GetPresentFrame()][x + 1][y] == true)
          neighbours++;
      }
      if (x - 1 >= 0 ) {
        if (m_Cells[GetPresentFrame()][x - 1][y] == true)
          neighbours++;
      }

      // Check vertical neighbours
      if (y + 1 < m_MatrixHeight) {
        if (m_Cells[GetPresentFrame()][x][y + 1] == true)
          neighbours++;
      }
      if (y - 1 >= 0 ) {
        if (m_Cells[GetPresentFrame()][x][y - 1] == true)
          neighbours++;
      }

      // Check diagonal neighbours
      if ((x + 1 < m_MatrixWidth) && (y + 1 < m_MatrixHeight)) {
        if (m_Cells[GetPresentFrame()][x + 1][y + 1] == true)
          neighbours++;
      }
      if ((x + 1 < m_MatrixWidth) && (y - 1 >= 0)) {
        if (m_Cells[GetPresentFrame()][x + 1][y - 1] == true)
          neighbours++;
      }

      if ((x - 1 >= 0) && (y + 1 < m_MatrixHeight)) {
        if (m_Cells[GetPresentFrame()][x - 1][y + 1] == true)
          neighbours++;
      }
      if ((x - 1 >= 0) && (y - 1 >= 0)) {
        if (m_Cells[GetPresentFrame()][x - 1][y - 1] == true)
          neighbours++;
      }

      return neighbours;
    }

    uint32_t GetPresentFrame () {
      return m_FrameIdx % 2;
    }

    uint32_t GetFutureFrame () {
      return (m_FrameIdx + 1) % 2;
    }

    void ProgressFrame () {
      m_FrameIdx++;
    }

    void DrawEffect () {
      for (int y = 0; y < m_MatrixHeight; y++) {
        for (int x = 0; x < m_MatrixWidth; x++) {
          uint8_t neighbours = GetNeighboursCount (x, y);
          
          if (m_Cells[GetPresentFrame()][x][y] == true) {
            // This is Live Cell
            if (neighbours < 2 || neighbours > 3)
              m_Cells[GetFutureFrame()][x][y] = false;
            else 
              m_Cells[GetFutureFrame()][x][y] = true;
          } else {
            // This is Dead Cell
            if (neighbours == 3)
              m_Cells[GetFutureFrame()][x][y] = true;
            else
              m_Cells[GetFutureFrame()][x][y] = false;
          }
        }
      }
      
      for (int y = m_MatrixHeight - 1; y >= 0; y--) {
        for (int x = m_MatrixWidth - 1; x >= 0; x--) {
          FastLED.leds()[MapPixel(x, y)] = m_Cells[GetPresentFrame()][x][y] ? CRGB (0, 128, 0) : CRGB (0, 0, 0);
        }
      }

      ProgressFrame ();
    }
};

class FireEffect {
  protected:
    float **m_Temperatures;
    uint32_t m_LastDraw;                    // the time of the last draw

    const float SPREADRATE_KNOB = 10.0f;  // Constant for flame spread rate
    const float SPARKRATE_KNOB  = 4.0f;   // Constant for spark ignition rate

    float   m_Cooling;                    // Pixel cooldown rate
    int     m_SparkHeight;                // Ignition zone for where new pixels start up
    float   m_SparkProbability;           // Probability of a spark in each ignition zone pixel
    float   m_SpreadRate;                 // Rate at which fire spreads pixel to pixel

    int m_MatrixWidth;
    int m_MatrixHeight;

    /**
      Map logical x,y coordinate into pixel index
    **/
    uint16_t MapPixel(uint16_t x, uint16_t y) {
      //return (m_MatrixHeight * (m_MatrixWidth - x) - 1) - y;
      uint16_t i;
      //horizontal serpentine, l2r, b2t
      if ( y & 0x01) { //if y is odd
          i = m_MatrixWidth * (y + 1) - x - 1;
        } else {
          i = m_MatrixWidth * y + x;
        }
      return i;
    }

  public:
    FireEffect (uint16_t matrixWidth = 12, uint16_t matrixHeight = 5, int sparkHeight = 0, float sparkProbability = 0.125, float cooling = 1.0f, float spreadRate = 0.8f) {
      m_MatrixWidth = matrixWidth;
      m_MatrixHeight = matrixHeight;

      m_Cooling = cooling;
      m_SparkHeight = sparkHeight;

      m_SparkProbability = sparkProbability * SPARKRATE_KNOB / ((sparkHeight + 1) * m_MatrixWidth);

      m_SpreadRate = spreadRate * SPREADRATE_KNOB;

      m_Temperatures = new float *[m_MatrixWidth];
      for (int i = 0; i < m_MatrixWidth; i++) {
        m_Temperatures[i] = new float[m_MatrixHeight];
      }

      for (int y = m_MatrixHeight - 1; y >= 0; y--) {
        for (int x = m_MatrixWidth - 1; x >= 0; x--) {
          m_Temperatures[x][y] = 0.0f;
        }

      }

      uint32_t m_LastDraw = millis();
    }

    ~FireEffect() {
      for (int i = 0; i < m_MatrixWidth; i++) {
        delete [] m_Temperatures[i];
      }
      delete [] m_Temperatures;
    }

    void HeatTransferFunction (uint16_t toX, uint16_t toY, uint16_t fromX, uint16_t fromY, float elapsedSeconds) {
      float spreadAmount = min (0.25f, m_Temperatures[fromX][fromY]) * m_SpreadRate * elapsedSeconds;
      spreadAmount = min (m_Temperatures[fromX][fromY], spreadAmount);

      m_Temperatures[toX][toY] += spreadAmount;
      m_Temperatures[fromX][fromY] -= spreadAmount;
    }

    void DrawEffect () {
      uint32_t currentMilliSeconds = millis();
      float elapsedSeconds = (currentMilliSeconds - m_LastDraw) / 1000.0;
      m_LastDraw = currentMilliSeconds;

      float cooldown = 1.0f * RandomFloat() * m_Cooling * elapsedSeconds;

      for (int y = m_MatrixHeight - 1; y >= 0; y--) {
        for (int x = m_MatrixWidth - 1; x >= 0; x--) {
          if (y == m_MatrixHeight - 1) {
            // Dissipate more heat at the very top of the chamber, otherwise it tends to glow white hot
            m_Temperatures[x][y] = _max (0.0f, m_Temperatures[x][y] - 4 * cooldown);
          }
          else {
            // Dissipate some heat
            m_Temperatures[x][y] = _max (0.0f, m_Temperatures[x][y] - cooldown);
          }

          // Heat transfer from lower layer toward the top.
          if (y < m_MatrixHeight - 1) {
            // always transfer heat from pixel underneath
            HeatTransferFunction (x, y + 1, x, y, elapsedSeconds);

            // Add random heat transfer from botom left or bottom right or both, makes fire less symetric and more alive.
            if (x - 1 >= 0 && RandomFloat() < m_SpreadRate * elapsedSeconds )
              HeatTransferFunction (x - 1, y + 1, x, y, elapsedSeconds);

            if (x + 1 < m_MatrixWidth && RandomFloat() < m_SpreadRate * elapsedSeconds)
              HeatTransferFunction (x + 1, y + 1, x, y, elapsedSeconds);
          }

          // Make sure we have small energy influx at the bottom, so fire is always burning
          if (y == 0)
            m_Temperatures[x][y] = _max(0.125, m_Temperatures[x][y]);

          // Add some sparks randomly within spark region (bottom of the chanber to m_SparkHeight)
          if (y <= m_SparkHeight && RandomFloat() < m_SparkProbability * elapsedSeconds) {
            m_Temperatures[x][y] += 8.0;
          }
        }
      }
      for (int y = m_MatrixHeight - 1; y >= 0; y--) {
        for (int x = m_MatrixWidth - 1; x >= 0; x--) {
          FastLED.leds()[MapPixel(x, y)] += HeatColor(148 * _min(1.0f, m_Temperatures[x][y]));
        }
      }
    }
};

CRGB g_LEDs[N_LEDS] = {0};
FireEffect *fire;
ConwaysGameOfLife *life;

void setup() {
  // Set LED_PIN as an output.
  pinMode(LED_PIN, OUTPUT);

  Serial.begin (9600);

  /*
     Configure FastLED library
  */
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(g_LEDs, N_LEDS).setCorrection (TypicalSMD5050);
  FastLED.setBrightness (255);
  FastLED.setMaxPowerInMilliWatts (500 * 5);
  fire = new FireEffect ();
  life = new ConwaysGameOfLife();
}

void loop() {
  FastLED.clear();
  //life->DrawEffect();
  fire->DrawEffect();
  FastLED.show(128);
  delay(50);
}
