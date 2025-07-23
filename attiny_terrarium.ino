#include <TinyWireM.h>
#include <Tiny4kOLED.h>

const uint8_t screen_width = 128;
uint8_t screen_height = 32;

uint8_t this_gen[screen_width];
uint8_t next_gen[screen_width];
uint16_t duration = 0;
uint16_t born = 0;
uint16_t dead = 0;


void setup() {
  randomSeed(analogRead(3));

  // oled.begin();
  oled.begin(screen_width, screen_height, sizeof(tiny4koled_init_128x32br), tiny4koled_init_128x32br);    // Two rotations are supported, the begin() method sets the rotation to 1. oled.setRotation(0);
  oled.setFont(FONT6X8);  // Two fonts are supplied with this library, FONT8X16 and FONT6X8, Other fonts are available from the TinyOLED-Fonts library
  oled.clear();
  
  // oled.setCursor(0, 0);
  // oled.print(text_seconds);
  // oled.setCursor(48, 0);
  // oled.print(seconds_elapsed);

  // oled.setCursor(0, 1);
  // oled.print("RANDOM: ");
  // oled.setCursor(42, 1);
  // oled.print(random(0, 2));
  for (uint8_t j = 0; j < screen_width; j++) {
    for (uint8_t i = 0; i < 8; i++) {
      bitWrite(this_gen[j], i, random(0, 2));
    }
  }

  for (uint8_t i = 0; i < screen_width / 4; i++) {
    oled.setCursor(i, 0);
    oled.startData();
    oled.sendData(this_gen[i]);
    oled.endData();

    oled.setCursor(i, 1);
    oled.startData();
    oled.sendData(this_gen[i + (screen_width / 4)]);
    oled.endData();

    oled.setCursor(i, 2);
    oled.startData();
    oled.sendData(this_gen[i + (screen_width / 2)]);
    oled.endData();

    oled.setCursor(i, 3);
    oled.startData();
    oled.sendData(this_gen[i + (3 * screen_width / 4)]);
    oled.endData();
  }


  oled.on();
  delay(1000);
  
}

void loop() {
  uint16_t alive = 0;

  oled.setCursor((screen_width / 4) + 1, 3);
  oled.print(duration);
  oled.print(" seconds");

  oled.setCursor((screen_width / 4) + 1, 1);
  // oled.print("+");
  oled.print(born);
  oled.print(" born");

  oled.setCursor((screen_width / 4) + 1, 2);
  // oled.print("-");
  oled.print(dead);
  oled.print(" dead");


  // LOOP TO ITERATE THROUGH PLAY GRID
  for (uint8_t y = 0; y < screen_height; y++ ) {
    for (uint8_t x = 0; x < screen_width / 4; x++) {
      uint8_t neighbors = countNeighbors(x, y);
      if (readCellValue(x, y) == 1) { // IF THIS CELL IS ALIVE
        if ((neighbors == 2) || (neighbors == 3)) {
          writeCellValue(x, y, 1);  // ALIVE
          alive++;
        } else {
          writeCellValue(x, y, 0);  // DEAD
          dead++;
        }
      } else {  // IF THIS CELL IS DEAD
        if (neighbors == 3) {
          writeCellValue(x, y, 1);  // ALIVE
          born++;
          alive++;
        } else {
          writeCellValue(x, y, 0);  // DEAD
        }

      }
    }
  }

  // DISPLAY NEW GENERATION
  for (uint8_t i = 0; i < screen_width / 4; i++) {
    oled.setCursor(i, 0);
    oled.startData();
    oled.sendData(next_gen[i]);
    oled.endData();

    oled.setCursor(i, 1);
    oled.startData();
    oled.sendData(next_gen[i + (screen_width / 4)]);
    oled.endData();

    oled.setCursor(i, 2);
    oled.startData();
    oled.sendData(next_gen[i + (screen_width / 2)]);
    oled.endData();

    oled.setCursor(i, 3);
    oled.startData();
    oled.sendData(next_gen[i + (3 * screen_width / 4)]);
    oled.endData();
  }


  oled.setCursor((screen_width / 4) + 1, 0);
  // oled.print("a");
  oled.print(alive);
  oled.print(" alive        ");



  // COPY NEW GEN TO OLD GEN
  for (uint8_t i = 0; i < screen_width; i++) {
    this_gen[i] = next_gen[i];
  }



  duration++;
  delay(1000);
}

uint8_t countNeighbors(uint8_t x, uint8_t y) {
  uint8_t count = 0;

  // TOP ROW
  if (y > 0) {
    if (x > 0) {
      count += readCellValue(x - 1, y - 1);  // TOP LEFT
    }

    count += readCellValue(x, y - 1); // TOP CENTER

    if ((x + 1) < (screen_width / 4)) {
      count += readCellValue(x + 1, y - 1); // TOP RIGHT
    }

  }
  

  // CENTER ROW
  if (x > 0) {
    count += readCellValue(x - 1, y); // CENTER LEFT
  }

  if ((x + 1) < (screen_width / 4)) {
    count += readCellValue(x + 1, y); // CENTER RIGHT
  }


  // BOTTOM ROW
  if ((y + 1) < screen_height) {
    if (x > 0) {
      count += readCellValue(x - 1, y + 1); // BOT LEFT
    }

    count += readCellValue(x, y + 1); // BOT CENTER

    if ((x + 1) < (screen_width / 4)) {
      count += readCellValue(x + 1, y + 1); // BOT RIGHT
    }
  }


  return count;
}

uint8_t readCellValue(int x, int y) {
  // RETURNS A CELL VALUE BASED ON X, Y COORDS OF MAP

  uint8_t array_index = 0;

  // COORDINATES OUT OF BOUNDS, RETURN 0
  if (x < 0 || y < 0 || x > screen_width / 4 || y > screen_height) {
    return array_index;
  }

  if (y < 8) {
    array_index = x;
  } else if (y < 16) {
    array_index = x + 32;
  } else if (y < 24) {
    array_index = x + 64;
  } else {
    array_index = x + 96;
  }

  uint8_t bit_position = y % 8;

  return bitRead(this_gen[array_index], bit_position);
}

void writeCellValue(uint8_t x, uint8_t y, uint8_t bit) {

  uint8_t array_index = 0;

  if (y < 8) {
    array_index = x;
  } else if (y < 16) {
    array_index = x + 32;
  } else if (y < 24) {
    array_index = x + 64;
  } else {
    array_index = x + 96;
  }

  uint8_t bit_position = y % 8;

  if (bit == 0) {
    bitClear(next_gen[array_index], bit_position);
  } else if (bit == 1) {
    bitSet(next_gen[array_index], bit_position);
  }

}

