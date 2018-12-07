#include <Arduino.h>
#include <FastLED.h>

#define LED_PIN 5
#define BRIGHTNESS 128
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

#define DELAY_PER_CYCLE 30
#define ADD_PARTICLE_CYCLE 4
#define POSTPROCESS_CYCLE 2

#define WIDTH 5
#define HEIGHT 16
#define NUM_LEDS WIDTH *HEIGHT
#define DO_MASK true

#define MAX_BRIGHTNESS 200
#define MIN_BRIGHTNESS 30
#define BRIBHTNESS_CHANGE_STEP 5

#define COLOR_MAX_VALUE 255

float masking[16][5] = {
    {0.1, 0.2, 1.0, 0.2, 0.1},
    {0.1, 0.3, 1.0, 0.3, 0.1},
    {0.2, 0.4, 1.0, 0.4, 0.2},
    {0.2, 0.5, 1.0, 0.5, 0.2},
    {0.3, 0.7, 1.0, 0.7, 0.3},
    {0.3, 0.9, 1.0, 0.9, 0.3},
    {0.7, 1.0, 1.0, 1.0, 0.7},
    {0.8, 1.0, 1.0, 1.0, 0.8},
    {0.9, 1.0, 1.0, 1.0, 0.9},
    {1.0, 1.0, 1.0, 1.0, 1.0},
    {1.0, 1.0, 1.0, 1.0, 1.0},
    {1.0, 1.0, 1.0, 1.0, 1.0},
    {1.0, 1.0, 1.0, 1.0, 1.0},
    {1.0, 1.0, 1.0, 1.0, 1.0},
    {0.4, 1.0, 1.0, 1.0, 0.4},
    {0.2, 0.5, 1.0, 0.5, 0.2}};

float convolutionMatrix[3][3] = {
    {0, 0, 0},
    {0, 0.7, 0},
    {0.2, 1, 0.2}};

#define ACTIVE_PALETTE_INDEX 0

int palettes[2][3][3] = {
  // realistic fire 1
  {
    {218,51,0},
    {100,55,0},
    {254,200,0}
  },
  // funky
  {
    {0,0,255},
    {0,255,0},
    {255,0,0}
  }
};

float convolutionDivider = 2.1;

int display[WIDTH][HEIGHT][3];
int buffer[WIDTH][HEIGHT][3];
int particleAddedCnt = ADD_PARTICLE_CYCLE;
int postProcessCnt = POSTPROCESS_CYCLE;

CRGB leds[NUM_LEDS];

/**
 * set initial colors for display matrix container array
 */
void initializeDisplayMatrix()
{
  for (int i = 0; i < WIDTH; i++)
  {
    for (int j = 0; j < HEIGHT; j++)
    {
      for (int p = 0; p < 3; p++)
      {
        display[i][j][p] = 0;
        buffer[i][j][p] = 0;
      }
    }
  }
}

/**
 * Copy buffer value to display value
 */
void copyBufferToDisplay()
{
  for (int i = 0; i < WIDTH; i++)
  {
    for (int j = 0; j < HEIGHT; j++)
    {
      for (int p = 0; p < 3; p++)
      {
        display[i][j][p] = buffer[i][j][p];
      }
    }
  }
}

/**
 * Add new particle to the display array
 */
void addNewParticle()
{
  particleAddedCnt++;

  if (particleAddedCnt >= ADD_PARTICLE_CYCLE)
  {
    particleAddedCnt = 0;
    int rx = rand() % WIDTH;
    int ry = ceil(5 * HEIGHT / 6 + rand() % HEIGHT / 6);

    int _ct = rand() % 3;
    int _rc = 1 + rand() % 6;

    for (int p = 0; p < 3; p++)
    {
      display[rx][ry][p] = (int)((float)palettes[ACTIVE_PALETTE_INDEX][_ct][p] / (float)_rc);
    }
  }
}

void setup()
{
  delay(500);
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();

  FastLED.setBrightness(BRIGHTNESS);

  // setup values
  initializeDisplayMatrix();
}

/**
 * Transform coordinates to index and set let
 */
void setLedByCoord(int x, int y, int _color[3])
{
  int idy = ((x % 2 == 0) ? HEIGHT - y - 1 : y);
  int index = (x * HEIGHT) + idy;

  int _cr = _color[0];
  int _cg = _color[1];
  int _cb = _color[2];

  // apply mask

  if (DO_MASK)
  {
    _cr = (int)((float)_cr * (float)masking[y][x]);
    _cg = (int)((float)_cg * (float)masking[y][x]);
    _cb = (int)((float)_cb * (float)masking[y][x]);
  }

  leds[index].red = _cr;
  leds[index].green = _cg;
  leds[index].blue = _cb;
}

int _postProcess[4][5][3] = {
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}};

/**
 * post process
 */
void postProcessCalculate()
{
  // add glowing to the botton with masking

  float mask[4][5] = {
      {0.2, 0.2, 0.5, 0.2, 0.2},
      {0.5, 0.5, 1, 0.5, 0.5},
      {0.5, 1, 1, 1, 0.5},
      {0.2, 0.5, 1, 0.5, 0.2}};

  for (int i = 0; i < WIDTH; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      if (mask[j][i] > 0.0)
      {
        int _color[3];
        int _ct = rand() % 3;
        int _rc = 10 + rand() % 10;

        for (int p = 0; p < 3; p++)
        {
          _postProcess[j][i][p] = (int)(((float)palettes[ACTIVE_PALETTE_INDEX][_ct][p] / (float)_rc) * (float)mask[j][i]);
        }
      }
    }
  }
}

float _brightness = (MAX_BRIGHTNESS - MIN_BRIGHTNESS) / 2;
float _brigntnessSpeedChange = BRIBHTNESS_CHANGE_STEP;

void changeBrightness()
{
  // brightness change over time

  // pingpong effect

  if (_brightness > MAX_BRIGHTNESS)
  {
    _brightness = (float)MAX_BRIGHTNESS;
    _brigntnessSpeedChange = -_brigntnessSpeedChange;
  }

  if (_brightness < MIN_BRIGHTNESS)
  {
    _brightness = (float)MIN_BRIGHTNESS;
    _brigntnessSpeedChange = -_brigntnessSpeedChange;
  }

  // or reverse randomly

  if (rand() % 100 < 10)
  {
    _brigntnessSpeedChange = -_brigntnessSpeedChange;
  }

  // change brightness

  _brightness += _brigntnessSpeedChange;

  FastLED.setBrightness((int)_brightness);
}

/**
 * Add post process stuff here
 */
void postProcess()
{
  postProcessCnt++;

  if (postProcessCnt >= POSTPROCESS_CYCLE)
  {
    postProcessCnt = 0;
    postProcessCalculate();
    changeBrightness();
  }

  for (int i = 0; i < WIDTH; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      setLedByCoord(i, HEIGHT - 4 + j, _postProcess[j][i]);
    }
  }
}

/**
 * show data on led strip from display value
 */
void show()
{
  for (int i = 0; i < WIDTH; i++)
  {
    for (int j = 0; j < HEIGHT; j++)
    {
      setLedByCoord(i, j, display[i][j]);
    }
  }

  postProcess();

  FastLED.show();
}

int getSafePixelData(int x, int y, int colorIndex)
{
  x = x < 0 ? 0 : x;
  x = x > WIDTH - 1 ? WIDTH - 1 : x;

  y = y < 0 ? 0 : y;
  y = y > HEIGHT - 1 ? HEIGHT - 1 : y;

  return display[x][y][colorIndex];
}

/**
 * calculate pixel value for convolution
 */
void claculatePixel(int x, int y)
{
  float _newPixel[] = {0, 0, 0};

  for (int i = -1; i < 2; i++)
  {
    for (int j = -1; j < 2; j++)
    {
      float _pixel[3] = {
          getSafePixelData(x + i, y + j, 0),
          getSafePixelData(x + i, y + j, 1),
          getSafePixelData(x + i, y + j, 2)
      };

      for (int p = 0; p < 3; p++)
      {
        _newPixel[p] = _newPixel[p] + (int)((float)_pixel[p] * (float)convolutionMatrix[j + 1][i + 1]);
      }
    }
  }

  for (int p = 0; p < 3; p++)
  {
    buffer[x][y][p] = (int)min(COLOR_MAX_VALUE, (int)((float)_newPixel[p] / (float)convolutionDivider));
  }

}

/**
 * Do the interpolation to all pixels
 */
void interpolate()
{

  for (int i = 0; i < WIDTH; i++)
  {
    for (int j = 0; j < HEIGHT; j++)
    {
      claculatePixel(i, j);
    }
  }

  copyBufferToDisplay();
}

void loop()
{
  addNewParticle();
  interpolate();
  show();
  delay(DELAY_PER_CYCLE);
}
