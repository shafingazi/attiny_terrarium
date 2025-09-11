// NOTES
// USES ATTiny85
// RUN INTO ISSUES WHEN USING > 80% OF DYNAMIC MEMORY, NEED TO STAY UNDER
// 404 BYTES IS ABSOLUTE MAXIMUM
// WILL BE ABLE TO FREE UP MORE SPACE BY WRITING TO EEPROM
// FLAGS TO CONSIDER FOR EEPROM: DEBUG, ROTATION/ORIENTATION

/*
  TODO: MAKE SEPARATE FILE FOR WRITING EEPROM MAP TO MEMORY (ONCE!)
  TODO: MAKE PROGRESS BAR RENDER ON ALL SCREENS (LOOK INTO setPage?)



                               EEPROM MAPPING                              
                                                                           
    0             15 16            31 32            47 48            63    
   ┌────────────────┬────────────────┬────────────────┬────────────────┐   
   │_alive          │                │                │                │   
   │_born           │                │                │                │   
   │_dead           │                │                │                │   
192│_seconds        │                │                │                │255
   ├────────────────┼────────────────┼────────────────┼────────────────┤   
256│                │                │                │                │319
   │                │                │                │                │   
   │                │                │                │                │   
   │                │                │                │                │   
   └────────────────┴────────────────┴────────────────┴────────────────┘   
    448          463 464          479 480          495 496          511    



*/

#include <TinyWireM.h>
#include <Tiny4kOLED.h>
#include <EEPROM.h>

const uint8_t screen_width = 128;   // [1B] SCREEN DIMENSIONS
const uint8_t screen_height = 32;   // [1B]

uint8_t this_gen[screen_width];     // [1B x screen_width] STORES CURRENT GAME MAP
uint8_t next_gen[screen_width];     // [1B x screen_width] STORES NEXT GAME MAP
uint16_t duration = 0;              // [2B] STORES DURATION FOR CURRENT GAME IN SECONDS
uint16_t born = 0;                  // [2B] STORES TOTAL SPAWNED CELLS FOR CURRENT GAME
uint16_t dead = 0;                  // [2B] STORES TOTAL KILLED CELLS FOR CURRENT GAME

// THIS BLOCK TAKES UP 16 BYTES!!!!
unsigned long this_millis = 0;            // [4B] STORES CURRENT RUNTIME IN MILLISECONDS (MAX ~50 DAYS)
unsigned long last_millis = 0;            // [4B] STORES PREVIOUS RUNTIME IN MILLISECONDS
unsigned long this_millis_button = 0;     // [4B]
unsigned long last_millis_button = 0;     // [4B]

uint16_t interval = 1000;                 // [2B] PERIOD FOR GAME TO RUN, IN MILLISECONDS
uint16_t duration_long_press = 2000;      // [3B] LONG-PRESS DURATION FOR BUTTON, IN MILLISECONDS

uint16_t push_duration = 0;               // [2B] HOLDS CURRENT DURATION OF BUTTON PUSH, IN MILLISECONDS
uint16_t last_push_duration = 0;          // [2B] HOLDS LAST DURATION OF BUTTON PUSH, IN MILLISECONDS

bool flag_game = false;                   // [1B] TRIPWIRE FLAG FOR GAME STATE, DETECTS BUTTON RELEASE
bool flag_menu = false;                   // [1B] TRIPWIRE FLAG FOR MENU STATE, DETECTS BUTTON RELEASE
bool flag_rset = false;                   // [1B] TRIPWIRE FLAG FOR RSET STATE, DETECTS BUTTON RELEASE

enum states {                             // [?B] ENUM FOR STATES IN STATE MACHINE
  GAME,
  MENU,
  RSET,
  ROLL
};

uint8_t current_state = GAME;             // [1B] STORES CURRENT STATE WITHIN STATE MACHINE

void setup() {

  // EEPROM.put(0, " alive");
  // EEPROM.put(64, " born");
  // EEPROM.put(128, " dead");
  // EEPROM.put(192, " seconds");

  

  pinMode(4, INPUT_PULLUP);     // INITIALIZE PIN 4 FOR PUSH BUTTON

  randomSeed(analogRead(A3));   // INITIALIZE RNG WITH NEW SEED

  // TODO: IMPLEMENT ROTATION CHANGE
  // Two rotations are supported, the begin() method sets the rotation to 1. oled.setRotation(0);
  oled.begin(screen_width, screen_height, sizeof(tiny4koled_init_128x32br), tiny4koled_init_128x32br);
  oled.setFont(FONT6X8);  // FONT8X16 ALSO AVAILABILE, ADDITIONAL FROM TinyOLED-Fonts LIBRARY
  oled.clear();
  
  // POPULATE THE INITIAL GAME DATA WITH A RANDOM MAP
  for (uint8_t j = 0; j < screen_width; j++) {
    for (uint8_t i = 0; i < 8; i++) {
      bitWrite(this_gen[j], i, random(0, 2));
    }
  }

  // DRAW THE MAP DATA ONTO THE SCREEN
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

  oled.on();  // TURNS ON DISPLAY WITH SENT DATA

}

void loop() {

  
  // TODO: IMPLEMENT A SIMPLE BUTTION HANDLER?
  uint8_t button_value = digitalRead(4);  // PUSH = 0, TODO: CONVERT TO BOOL

  uint16_t alive = 0;

  uint16_t text_pad = 5;

  // !TODO: FIX THIS IMPLEMENTATION WITH TIMESTAMPS ON BUTTON EVENTS
  this_millis = millis(); // GETS CURRENT TIME 
  this_millis_button = millis();


  // CALC push_duration
  if (button_value == 0) {
    push_duration += this_millis_button - last_millis_button;
  } else {
    last_push_duration = push_duration;
    push_duration = 0;
  }



  switch (current_state) {
    case GAME:

      if (button_value == 1) flag_game = true;   // BUTTON RELEASE TRIPS FLAG

      if (flag_game && push_duration >= duration_long_press) {    // GAME > MENU
        flag_game = false;
        current_state = MENU;
        oled.clear();
        return;
      } else if (flag_game && (0 < last_push_duration) && (last_push_duration < duration_long_press) && button_value == 1) {   // GAME > RSET
        flag_game = false;
        current_state = RSET;
        oled.clear();
        return;
      }

      // GAME
      if (this_millis - last_millis >= interval) {

        // SECONDS
        oled.setCursor((screen_width / 4) + text_pad, 3);
        oled.print(duration);
        print_from_eeprom(192, 8);
        oled.clearToEOL();

        // BORN
        oled.setCursor((screen_width / 4) + text_pad, 1);
        oled.print(born);
        print_from_eeprom(64, 5);
        oled.clearToEOL();

        // DEAD
        oled.setCursor((screen_width / 4) + text_pad, 2);
        oled.print(dead);
        print_from_eeprom(128, 5);
        oled.clearToEOL();

        alive = play_play_grid(alive);

        draw_play_grid();

        // ALIVE
        oled.setCursor((screen_width / 4) + text_pad, 0);
        oled.print(alive);
        print_from_eeprom(0, 6);
        oled.clearToEOL();


        // COPY NEW GEN TO OLD GEN
        for (uint8_t i = 0; i < screen_width; i++) {
          this_gen[i] = next_gen[i];
        }

        duration ++;
        last_millis = this_millis;
      }

      // PRINT button_value TO TOP RIGHT CORNER
      // oled.setCursor(screen_width - 10, 0);
      // oled.print(button_value);
      

      break;
    case MENU:

      // oled.clear();
      oled.setCursor(0, 0);
      // oled.print("MENU");
      oled.setCursor(0, 1);
      oled.print(push_duration);
      oled.setCursor(0, 2);
      oled.print(button_value);

      // THIS DOESN'T WORK AT THE SAME TIME AS push_duration's calc?
      if (button_value == 1) flag_menu = true;   // BUTTON RELEASE TRIPS FLAG

      // PROGRESS BAR
      if (flag_menu && 0 < push_duration && push_duration < duration_long_press) {
        oled.setCursor(0, 3);
        oled.fillLength(B10000000, (8 * push_duration) / screen_width);
      } else {
        oled.setCursor(0, 3);
        oled.fillLength(B00000000, screen_width);
      }

      if (flag_menu && push_duration >= duration_long_press) {   // MENU > GAME
        flag_menu = false;


        current_state = GAME;
        return;
      }

      break;
    case RSET:

      if (button_value == 1) flag_rset = true;   // BUTTON RELEASE TRIPS FLAG

      // PROGRESS BAR
      if (flag_rset && 0 < push_duration && push_duration < duration_long_press) {
        oled.setCursor(0, 3);
        oled.fillLength(B10000000, (8 * push_duration) / screen_width);
      }

      if (flag_rset && push_duration >= duration_long_press) {  // RSET > GAME
        flag_rset = false;

        alive = 0;
        born = 0;
        dead = 0;
        duration = 0;

        // // RESET GEN ARRAYS
        // for (uint8_t i = 0; i < screen_width; i++) {
        //   this_gen[i] = 0;
        //   next_gen[i] = 0;
        // }

        randomSeed(analogRead(A3));

        for (uint8_t j = 0; j < screen_width; j++) {
          for (uint8_t i = 0; i < 8; i++) {
            bitWrite(this_gen[j], i, random(0, 2));
          }
        }







        current_state = GAME;
        oled.clear();
        return;
      } else if (flag_rset && (0 < last_push_duration) && (last_push_duration < duration_long_press) && button_value == 1) {   // RSET > GAME
        flag_rset = false;
        current_state = GAME;
        oled.clear();
        return;
      }

      oled.setCursor(0, 0);
      oled.print("?");
      // oled.print("RESET?");

      
      
      break;

    default:

      break;
  }

  // PRINTS STATE INT IN BOT RIGHT CORNER
  // oled.setCursor(screen_width - 10, 3);
  // oled.print(current_state);

  last_millis_button = this_millis_button;

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


void draw_play_grid() {
  // DISPLAY CONTENTS OF NEW GENERATION
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

}


uint16_t play_play_grid(uint16_t alive) {
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

  return alive;

}

void print_from_eeprom(uint8_t idx, uint8_t size) {

  for (uint8_t i = 0; i < size; i++) {
    oled.print(static_cast<char>(EEPROM.read(idx + i)));
  }

  return;
}


// void drawProgressBar(uint8_t x, uint8_t y, uint8_t length) {
//   oled.bitmap(0, 64, length, 64, const uint8_t *bitmap)
//   for (uint8_t i = 0; i < length; i++) {
//     oled.drawPixel(x + i, y);
//   }
// }



// char* read_eeprom(uint8_t idx, uint8_t size) {

//   char text[size];
//   for (uint8_t i = 0; i < size; i++) {
//     text[i] = static_cast<char>(EEPROM.read(idx + i));
//   }

//   return text;

// }
