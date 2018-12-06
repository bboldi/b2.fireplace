#include <Arduino.h>
#include <FastLED.h>

#define LED_PIN 5
#define BRIGHTNESS 128
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

#define DELAY_PER_CYCLE 20
#define ADD_PARTICLE_CYCLE 2
#define POSTPROCESS_CYCLE 3

#define WIDTH 5
#define HEIGHT 16
#define NUM_LEDS WIDTH*HEIGHT
#define DO_MASK true

#define COLOR_MAX_VALUE 255

float masking[16][5] = {
  {0.1, 0.2,  1.0,  0.2,  0.1},
  {0.2, 0.3,  1.0,  0.3,  0.2},
  {0.3, 0.4,  1.0,  0.4,  0.3},
  {0.4, 0.5,  1.0,  0.5,  0.4},
  {0.5, 0.7,  1.0,  0.7,  0.5},
  {0.6, 0.9,  1.0,  0.9,  0.6},
  {0.7, 1.0,  1.0,  1.0,  0.7},
  {0.8, 1.0,  1.0,  1.0,  0.8},
  {0.9, 1.0,  1.0,  1.0,  0.9},
  {1.0, 1.0,  1.0,  1.0,  1.0},
  {1.0, 1.0,  1.0,  1.0,  1.0},
  {1.0, 1.0,  1.0,  1.0,  1.0},
  {1.0, 1.0,  1.0,  1.0,  1.0},
  {1.0, 1.0,  1.0,  1.0,  1.0},
  {0.4, 1.0,  1.0,  1.0,  0.4},
  {0.2, 0.5,  1.0,  0.5,  0.2}
};

float convolutionMatrix[3][3] = {
  {0,0,0},
  {0,0.7,0},
  {0.2,1,0.2}
    };

float convolutionDivider = 2.2;

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

  if(particleAddedCnt>=ADD_PARTICLE_CYCLE)
  {
    particleAddedCnt=0;
    int rx = rand() % WIDTH;
    int ry = ceil(5 * HEIGHT / 6 + rand() % HEIGHT / 6);

    int _ct = rand() % 2;
    int _rc = 1+rand() % 6;

    switch(_ct){

      case 0:
        display[rx][ry][0] = (int)((float)200 / (float)_rc);
        display[rx][ry][1] = (int)((float)44 / (float)_rc);
        display[rx][ry][2] = (int)((float)12 / (float)_rc);
      break;

      case 1:
        display[rx][ry][0] = (int)((float)100 / (float)_rc);
        display[rx][ry][1] = (int)((float)55 / (float)_rc);
        display[rx][ry][2] = 0;
      break;

      case 2:
        display[rx][ry][0] = (int)((float)245 / (float)_rc);
        display[rx][ry][1] = (int)((float)245 / (float)_rc);
        display[rx][ry][2] = (int)((float)12 / (float)_rc);
      break;
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

  if(DO_MASK)
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
  {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
  {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
  {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
  {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}
};

void postProcessCalculate()
{

  float mask[4][5] = {
    {0.1,0.1,0.5,0.1,0.1},
    {0.2,0.5,1,0.5,0.2},
    {0.5,1,1,1,0.5},
    {0.5,1,1,1,0.5}
  };

  for (int i = 0; i < WIDTH; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      if(mask[j][i] > 0.0)
      {
        int _color[3];
        int _ct = rand() % 2;
        int _rc = 4 + rand() % 4;

        switch (_ct)
        {

          case 0:
            _color[0] = (int)(((float)200 / (float)_rc)*mask[j][i]);
            _color[1] = (int)(((float)44 / (float)_rc)*mask[j][i]);
            _color[2] = (int)(((float)12 / (float)_rc)*mask[j][i]);
            break;

          case 1:
            _color[0] = (int)(((float)100 / (float)_rc)*mask[j][i]);
            _color[1] = (int)(((float)55 / (float)_rc)*mask[j][i]);
            _color[2] = 0;
            break;

          case 2:
            _color[0] = (int)(((float)245 / (float)_rc)*mask[j][i]);
            _color[1] = (int)(((float)245 / (float)_rc)*mask[j][i]);
            _color[2] = (int)(((float)12 / (float)_rc)*mask[j][i]);
            break;
        }
        _postProcess[j][i][0] = _color[0];
        _postProcess[j][i][1] = _color[1];
        _postProcess[j][i][2] = _color[2];
      }
    }
  }

}

void postProcess()
{
  postProcessCnt++;

  if(postProcessCnt>=POSTPROCESS_CYCLE)
  {
    postProcessCnt=0;
    postProcessCalculate();
  }

  for (int i = 0; i < WIDTH; i++)
  {
    for (int j = 0; j < 4; j++)
    {
        setLedByCoord(i, HEIGHT-4+j, _postProcess[j][i]);
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
  // return pixel data safely

  x = x < 0 ? 0 : x;
  x = x > WIDTH - 1 ? WIDTH - 1 : x;

  y = y < 0 ? 0 : y;
  y = y > HEIGHT - 1 ? HEIGHT - 1 : y;
  /*
  if( x<0 || x>WIDTH-1 || y<0 || y>HEIGHT-1)
  {
    return 0;
  }
  */
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
          getSafePixelData(x + i, y + j, 2)};

      for (int p = 0; p < 3; p++)
      {
        _newPixel[p] = _newPixel[p] + (int)((float)_pixel[p] * (float)convolutionMatrix[j + 1][i + 1]);
      }
    }
  }

  for (int p = 0; p < 3; p++)
  {
    _newPixel[p] = min(COLOR_MAX_VALUE, (int)((float)_newPixel[p] / (float)convolutionDivider));
  }

  buffer[x][y][0] = (int)_newPixel[0];
  buffer[x][y][1] = (int)_newPixel[1];
  buffer[x][y][2] = (int)_newPixel[2];
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

int _brightness = 0;

void loop()
{
  /*
  _brightness ++;
  _brightness = _brightness>255 ? 0 : _brightness;
  FastLED.setBrightness(_brightness);
  */
  addNewParticle();
  interpolate();
  show();
  delay(DELAY_PER_CYCLE);
}
