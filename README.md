# NXT Drawbot
#### Drawbot is a pen plotter built using Lego Mindstorms NXT

The plotter uses a rolling gantry, which has wheels on either side, rolling in parallel slots. The gantry is made up of a trussed beam and a moving carriage that holds the pen. It is controlled using standard G-Code, but somewhat simplified. This G-Code can be saved locally to the NXT and run from the program, or run using the NXT Driver program on a windows computer. Pictures of the plotter can be found in the **Pictures** folder, and a video of it in operation can be found here:
**YOUTUBE LINK**

#### NXT Drawbot Program
The Drawbot program running on the NXT was coded in [NXC](bricxcc.sourceforge.net/nbc/), a c based language for the NXT. NXC is supported and compiled by [Brixcc](bricxcc.sourceforge.net). Additionally, the Drawbot program requires the Enhanced NXC Firmware 1.31, which can be downloaded to the NXT from Bricxcc. The firmware file is found in the Bricxcc folder in your computer's program files(x84) folder.

The Program reads the G-Code file line by line, updating a state variable based on each parameter given in that line. After reading the line, the state is read, and the machine moves to match it. In order to ensure a straight line is taken between its current position and the demanded state, the program interpolates points every mm along the path to the new state. The full source file can be found in the **Software** folder.

#### NXT Driver Program
The NXT Driver program runs on a Windows computer, and sends G-Code files stored on that computer to the NXT via USB or Bluetooth line by line, in order to bypass the storage limitations of the NXT. The Program is written in C++ using MSVS 19, and is based on the [NXT++](https://github.com/corywalker/nxt-plus-plus) library by Cory Walker, which handles all communications with the NXT. The full MSVS project, as well as the Setup Program, can be found in the **Software** folder.

On running, the program will first connect and handshake with the NXT, start the Drawbot program and set it to run remote G-Code. It will ask for a path to the folder that has the G-code, and than search this folder for .txt files to run as G-Code. After the user selects a file to run, it will send the file line by line to the NXT, staying 3 lines ahead to prevent waiting, and filtering out any lines that are repetitive(less than 1 mm change from last line).

#### Fusion 360 Post-Processor
Fusion 360 can be used to create G-Code for the Drawbot. To do this, make a trace milling operation of any flat geometry to be drawn, and post process it using the post-processor found in the **Software** folder. In the milling operation, the cutting feed rate can be set as high as 3600 mm/min.

#### Supported G & M codes:
- G0:     Rapid Move
- G1:     Move at Feedrate
- G4:     pause for P milliseconds
- G20/21: Use Inches/Millimeters, mm is suggested/standard
- G28:    Home Machine
- G90/91: Use Absolute or Relative coordinate system, Absolute is suggested/standard
- M3/4:   Pen Down
- M5:     Pen Up
- M117:   Write to LCD

Note: Z commands can also be used for Pen Up/Down, Z heights <= 0 are down, Z > 0 is up

Note: G4,20,21,28,90, and 91 should be used on their own line. Putting them on the same line as other command could cause unexpected behavior.
