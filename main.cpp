#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( int argc, char** argv ){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

	/********************
	********************/
	int soundStream_Input_DeviceId = -1;
	int soundStream_Output_DeviceId = -1;
	int Cam_id = -1;
	string FileName = "NotSet";
	bool MovSound_on = false;

	/********************
	********************/
	printf("---------------------------------\n");
	printf("> parameters\n");
	printf("\t-i:Audio in(-1)\n");
	printf("\t-o:Audio out(-1)\n");
	printf("\t-c:Cam id(-1)\n");
	printf("\t-f:movie file name\n");
	printf("\t-v:Mov sound on : 0=off,1=on (0)\n");
	printf("---------------------------------\n");
	
	for(int i = 1; i < argc; i++){
		if( strcmp(argv[i], "-i") == 0 ){
			if(i+1 < argc) soundStream_Input_DeviceId = atoi(argv[i+1]);
		}else if( strcmp(argv[i], "-o") == 0 ){
			if(i+1 < argc) soundStream_Output_DeviceId = atoi(argv[i+1]);
		}else if( strcmp(argv[i], "-c") == 0 ){
			if(i+1 < argc){
				Cam_id = atoi(argv[i+1]);
				if(Cam_id < 0) Cam_id = 0;
			}
		}else if( strcmp(argv[i], "-v") == 0 ){
			if(i+1 < argc){
				if(atoi(argv[i+1]) == 1) MovSound_on = true;
			}
		}else if( strcmp(argv[i], "-f") == 0 ){
			if(i+1 < argc){
				FileName = argv[i+1];
			}
		}
	}
	
	ofRunApp(new ofApp(soundStream_Input_DeviceId, soundStream_Output_DeviceId, Cam_id, FileName, MovSound_on));
}
