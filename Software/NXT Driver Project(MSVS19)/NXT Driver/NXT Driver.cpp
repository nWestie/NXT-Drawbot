#include <iostream>
#include <stdio.h>
#include <fstream>
#include <time.h>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// includes for NXT++ 0.6
#include "nxtpp_07\include\NXT++.h"
#pragma comment (lib, "nxtpp_07/lib/fantom.lib" )

//predeclare functions(defined below)
void sendMessage(Comm::NXTComm* pComm, string msg, uint8_t mailbox); //send string to NXT
void recieveMessage(Comm::NXTComm* comm, string& msgOut, uint8_t mailbox); //recieve string from NXT
void delay(unsigned int millis); //pause for x ms
void exit(int val); // keeps window open before returning
bool goodLine(string line); //makes sure line to be sent is not repetative

int main() {
	//searching for availible bricks
	
	std::cout << "Search Bluetooth for NXTs?(y|n)\n";
	char input;
	std::cin >> input;
	
	printf("Searching for NXT devices... (please wait) \n\n");
	std::vector<std::vector<std::string> > devicelist;
	devicelist = Comm::ListNXTDevices(tolower(input) == 'y' ? true : false);//searching for NXT connections, Bricks must be paired to use BT
	printf("Search complete! \n\n");
	for (unsigned int i = 0; i < devicelist.size(); i++) {//printing list of found devices
		std::cout << "Device # " << i << "   Name: " << devicelist[i][0] << "   MAC Address: " << devicelist[i][1] << endl;
	}
	std::cout << endl;
	Comm::NXTComm comm1;
	unsigned int brickNum = 0;
	if (devicelist.size() == 0) {//return if no devices are found.
		std::cout << "No NXTs found.";
		exit(1);
		return 1;
	}

	if (devicelist.size() != 1) {//select brick automaticly if more than 1 is found
		std::cout << "Enter the Device # of a NXT to connect to it.\n";
		std::cin >> brickNum;
	}
	//connecting to choosen brick, getting data.
	std::cout << "Connecting...";
	if (!NXT::OpenNXTDevice(&comm1, devicelist[brickNum][0], true)) {
		exit(2);
		return 2;//return if err connecting to brick
	}
	printf("\n\nConnected to brick!\n");
	// Get NXT device information
	double protocol = NXT::GetProtocolVersion(&comm1);
	double version = NXT::GetFirmwareVersion(&comm1);
	int battery = NXT::BatteryLevel(&comm1);
	string name = NXT::GetName(&comm1);
	// Output information to console
	printf("Protocol version: %f \n", protocol);
	printf("Firmware version: %f \n", version);
	printf("Battery Level: %i \n", battery);
	printf("Name: %s \n", name.data());
	NXT::PlayTone(&comm1, 440, 150); // Play tone from NXT
	delay(200);
	NXT::StopProgram(&comm1);//clearing any running program
	delay(1000);
sendSequence://comes back here if user chooses to run another file
	NXT::StartProgram(&comm1, "Drawbot.rxe");//starting progam. Make sure program is on NXT, or an error will occur
	delay(1000);
	sendMessage(&comm1, "SlaveMode", 1);//inform NXT program it is a slave
	string msgIn;
	while (msgIn != "Ready") //wait for NXT to recieve and confirm it is opperating as slave
		recieveMessage(&comm1, msgIn, 1);

	ifstream fPath;
	fPath.open("FolderPath.txt");//stores name of folder to search for GCode
	ofstream pathWrite;
	string fName;
	getline(fPath, fName);
	fPath.close();
	if (fName.size() == 0) {//if FolderPath is empty, prompt user for path and save it to file.
		std::cout << "Please enter a valid path for the folder to search for Gcode. Absolute Paths are recommended.\n";
		cin.ignore();
		getline(cin, fName);
		pathWrite.open("FolderPath.txt");
		pathWrite << fName;
		pathWrite.close();
	}
	fPath.close();
	
	//searching folder for .txt files, assumed to hold G-Code
	vector<fs::path> list;
	for (fs::directory_iterator iFile(fName), end; iFile != end; ++iFile) {
		if (iFile->path().extension() == ".txt") {
			list.push_back(iFile->path());//adding files to list if text files
		}
	}
	std::cout << "\n\nGcode files:\n" << list[0].parent_path() << endl;
	for (int i = 0; i < list.size(); i++)
		std::cout << i << ": " << list[i].filename() << endl;//printing files to be selected by number

	std::cout << "Input the # of a Gcode file to run it\n";
	int fInd;
	std::cin >> fInd;//getting responce from user
	std::cout << "file " << list[fInd].filename() << " selected, sending to NXT..." << endl; 

	fstream file(list[fInd]);//opening chosen file
	string lnText, sLnNum;
	unsigned int slaveLnNum = 0, hostLnNum = 0;
	std::cout << "Sending File..." << endl;
	while (getline(file, lnText)) {
		if (!goodLine(lnText))continue;//skip sending line if not nessisary
		sendMessage(&comm1, lnText, 1); //sending line
		hostLnNum++;
		std::cout << hostLnNum << ":  " << lnText << endl;
		while (hostLnNum >= slaveLnNum + 3) {     //keep only 3 messages in mailbox cue, wait til confirmed by NXT to send more.
			recieveMessage(&comm1, sLnNum, 1);    //NXT will send back the line it is on, this program tries to stay 3 ahead of that number.
			if (sLnNum.size()) slaveLnNum = stoi(sLnNum);//only convert to int if a message was recieved.
		}
	}
	file.close();
	sendMessage(&comm1, "endProg", 1);//tell NXT file is done, it will end it's program
	while (hostLnNum > slaveLnNum) {//waiting for NXT to finish
		recieveMessage(&comm1, sLnNum, 1);
		if (sLnNum.size()) slaveLnNum = stoi(sLnNum);
	}
	
	std::cout << "Send another file?(y|n)\n";//loop back to send sequence to send another file
	std::cin >> input;
	if (tolower(input) == 'y')goto sendSequence;
	NXT::StopProgram(&comm1);
	return 0;
}

void sendMessage(Comm::NXTComm* comm, string msg, uint8_t mailbox) {
	vector<ViByte> buff;//send buffer to be filled
	buff.push_back(0x09);//add direct command number for send to mailbox
	buff.push_back(mailbox); //add mailbox num
	buff.push_back(msg.size() + 1);//add message size
	for (int i = 0; i < msg.size(); i++)//add string message
		buff.push_back(msg[i]);
	buff.push_back(0x00);//add null terminator
	comm->SendDirectCommand(false, reinterpret_cast<ViByte*>(buff.data()), buff.size(), NULL, 0);//cast vector to byte array and send to NXT
}
void recieveMessage(Comm::NXTComm* comm, string& msgOut, uint8_t mailbox) {
	ViByte sendBuff[] = { 0x13, (mailbox + 10), mailbox, true };//command to read message from NXT
	char recBuff[63] = {};
	comm->SendDirectCommand(true, reinterpret_cast<ViByte*>(sendBuff), sizeof(sendBuff), reinterpret_cast<ViByte*>(recBuff), sizeof(recBuff));//send command to NXT
	if (recBuff[1] == 0) {//if returns with no errors
		recBuff[1] = 2;//keeps string from being terminated early by null byte
		msgOut = (string)recBuff;//cast to string
		msgOut = msgOut.substr(4, msgOut.length() - 1);//trim message header from string
	}
	return;
}
void delay(unsigned int millis) {
	clock_t start_time = clock();
	while (clock() < start_time + millis);
}
void exit(int val) {
	cout << "\nProgram Returned " << val << ", press enter to close.\n";
	cin.ignore();
	cin.get();//waits for user to press enter
}
bool goodLine(string line) {//based on "readgcodeline" function in Drawbot NXC program
	bool commentFlag = false;
	char ltr = 0;
	uint16_t numStart;
	float num;
	static float prevX = 0, prevY = 0;
	float x=0, y=0;
	for (uint16_t i = 0; i < line.size(); i++) {
		ltr = line[i];
		if (ltr == ';')//if comment, goto checking
			goto checkXY;
		else if (ltr == '(' || ltr == ')') //other comment type, sets flag on '(', loops til ')'
			commentFlag = !commentFlag;
		else if (ltr <= ' ' || commentFlag);//continue if in comment or space
		
		//spaces, %, comments removed
		else if (isalpha(ltr)) {
			numStart = ++i;//increment i, set this as start of number
			ltr = tolower(ltr);
			while (isdigit(line[i]) || line[i] == '.')i++;//increment i until end of the number. When finished, i is the index 1 after the end of the number.
			num = stof(line.substr(numStart, (i - numStart)));//read number from line text
			i--;//decrement i to point to last digit of the number
			switch (ltr) {
			case 'x':
				x = num;
				break;
			case 'y':
				y = num;
				break;
			case 'g':
				if (num < 2)continue;

			default:
				return true;//returns true if the line contains any other commands, as these all must be sent to NXT
			}//end switch
		}//end else if
	}//end for loop
checkXY:
	if (abs(x-prevX) < 1 && abs(y-prevY) < 1)//if change in X and Y are both < 1 mm, do not send line
		return false;
	else {//if siginificant change in X and Y, update prevs and send line
		prevX = x;
		prevY = y;
		return true;
	}
}