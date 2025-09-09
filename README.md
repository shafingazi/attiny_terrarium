# gen-a-sec

This project is a miniature version of Conway's Game of Life, running on an ATTiny85.

### SIMPLE STATE MACHINE
Bulding out additional functionality will require writing to EEPROM to optimize memory usage.
```
     push_duration >= 2000ms     ┌──────┐    ENTER THE RESET SCREEN     
┌────────────────────────────────┤      │◄─────────────────────────────┐
│                                │ RSET │                              │
│ ┌──────────────────────────────┤      │                              │
│ │ 0ms < push_duration < 2000ms └──────┘                              │
│ │                                                                    │
│ │    RETURN TO CURRENT GAME    ┌──────┐                              │
│ └─────────────────────────────►│      │ 0ms < push_duration < 2000ms │
│                                │ GAME ├──────────────────────────────┘
└───────────────────────────────►│      │                               
   RESTART & GENERATE NEW GAME   └┬────┬┘                               
                                  │    ▲                                
┌──────┐  push_duration >= 2000ms │    │                                
│      │◄─────────────────────────┘    │ RETURN TO CURRENT GAME         
│ MENU │                               │                                
│      ├───────────────────────────────┘                                
└──────┘  push_duration >= 2000ms                                       

```

### WIRING DIAGRAM
```
          ┌─ATTINY85─┐                              
          │1 ○      8├────────┐                     
          │2        7├───┐    │                     
 ┌─BUTTON─┤3        6│   │    │                     
 ├────────┤4        5├──┤│├──┤│├──┐  ┌─────────────┐
 │        └──────────┘   │    │   └──┤1 SDA   0.91"│
 │                 ┌────┐└───┤│├─────┤2 SCK     I2C│
 ├────────────────┤│├┐  └─────┴──────┤3 VCC    OLED│
 │        ┌──────┐ │ └───────────────┤4 GND DISPLAY│
 └─SWITCH─┤USB 5V├─┘                 └─────────────┘
         -└──────┘+                                 
                                                    
             /\                                     
            /││\                                    
             ││                                     
                                                    
           ┌────┐                                   
           │USBC│                                   
          ┌┴────┴┐                                  
          │      │                                  
          │      │                                  
          │      │                                  
          │      │                                  
          └─┬──┬─┘                                  
            │  │                                    
            │  │                                    
            │  │                                    
            /\ │/                                   
           /  \/                                                                                                                   
```
