/************************************************************
************************************************************/
#include "ofApp.h"

/************************************************************
************************************************************/

/******************************
******************************/
ofApp::ofApp(int _soundStream_Input_DeviceId, int _soundStream_Output_DeviceId, int _Cam_id, string _FileName, bool _MovSound_on)
: soundStream_Input_DeviceId(_soundStream_Input_DeviceId)
, soundStream_Output_DeviceId(_soundStream_Output_DeviceId)
, Cam_id(_Cam_id)
, b_DispGui(true)
, png_id(0)
, VideoCam(NULL)
, FileName(_FileName)
, MovSound_on(_MovSound_on)
{
	/********************
	********************/
	for(int i = 0; i < NUM_AUDIO_CHS; i++) { fft_thread[i] = new THREAD_FFT(); }
	
	/********************
	********************/
	font[FONT_S].load("font/RictyDiminished-Regular.ttf", 10, true, true, true);
	font[FONT_M].load("font/RictyDiminished-Regular.ttf", 12, true, true, true);
	font[FONT_L].load("font/RictyDiminished-Regular.ttf", 30, true, true, true);
	
	/********************
	********************/
	fp_Log			= fopen("../../../data/Log.csv", "w");
	fp_Log_main		= fopen("../../../data/Log_main.csv", "w");
	fp_Log_Audio 	= fopen("../../../data/Log_Audio.csv", "w");
	fp_Log_fft 		= fopen("../../../data/Log_fft.csv", "w");
}

/******************************
******************************/
ofApp::~ofApp()
{
	if(fp_Log)			fclose(fp_Log);
	if(fp_Log_main)		fclose(fp_Log_main);
	if(fp_Log_Audio)	fclose(fp_Log_Audio);
	if(fp_Log_fft)		fclose(fp_Log_fft);
	
	if(VideoCam) delete VideoCam;
}

/******************************
******************************/
void ofApp::exit(){
	/********************
	ofAppとaudioが別threadなので、ここで止めておくのが安全.
	********************/
	soundStream.stop();
	soundStream.close();
	
	/********************
	********************/
	for(int i = 0; i < NUM_AUDIO_CHS; i++){
		fft_thread[i]->exit();
		try{
			/********************
			stop済みのthreadをさらにstopすると、Errorが出るようだ。
			********************/
			while(fft_thread[i]->isThreadRunning()){
				fft_thread[i]->waitForThread(true);
			}
			
		}catch(...){
			printf("Thread exiting Error\n");
		}
		
		delete fft_thread[i];
	}
	
	
	/********************
	********************/
	printf("\n> Good bye\n");
}	

/******************************
******************************/
void ofApp::setup(){
	/********************
	********************/
	ofSetWindowTitle("WaveSurface");
	
	ofSetWindowShape( WINDOW_WIDTH, WINDOW_HEIGHT );
	ofSetVerticalSync(true);
	ofSetFrameRate(30);
	ofSetEscapeQuitsApp(false);
	
	/********************
	********************/
	setup_Gui();
	
	/********************
	********************/
	for(int i = 0; i < NUM_AUDIO_CHS; i++){
		Vboset_fft_Raw[i].setup(AUDIO_BUF_SIZE/2 * 4); /* square */
		Vboset_fft_Raw[i].set_singleColor(ofColor(255, 255, 255, 110));
		
		Vboset_fft_Corrected[i].setup(AUDIO_BUF_SIZE/2 * 4); /* square */
		Vboset_fft_Corrected[i].set_singleColor(ofColor(255, 255, 255, 110));
	}
	
	/********************
	GRAPH__MOV,
	GRAPH__FFT_L,
	
	GRAPH__CAM,
	GRAPH__FFT_R,
	
	
	FBO_FFT_WIDTH		= 600,
	FBO_FFT_HEIGHT		= 340,
	
	FBO_VIDEO_WIDTH		= 640,
	FBO_VIDEO_HEIGHT	= 360,
	********************/
	fbo[GRAPH__MOV].allocate(FBO_VIDEO_WIDTH, FBO_VIDEO_HEIGHT, GL_RGBA);
	fbo[GRAPH__FFT_L].allocate(FBO_FFT_WIDTH, FBO_FFT_HEIGHT, GL_RGBA);
	
	fbo[GRAPH__CAM].allocate(FBO_VIDEO_WIDTH, FBO_VIDEO_HEIGHT, GL_RGBA);
	fbo[GRAPH__FFT_R].allocate(FBO_FFT_WIDTH, FBO_FFT_HEIGHT, GL_RGBA);
	
	fbo[GRAPH__MOV_AND_CAM].allocate(FBO_VIDEO_WIDTH * 2, FBO_VIDEO_HEIGHT, GL_RGBA);
	
	
	for(int i = 0; i < NUM_GRAPHS; i++) { Clear_fbo(fbo[i]); }
	
	/********************
	********************/
	AudioSample.resize(AUDIO_BUF_SIZE);

	for(int i = 0; i < NUM_AUDIO_CHS; i++) { fft_thread[i]->setup(); fft_thread[i]->startThread(); }
	
	/********************
	********************/
	Refresh_FFTVerts();
	
	/********************
	********************/
	shader_WaveSurface.load( "sj_shader/WaveSurface.vert", "sj_shader/WaveSurface.frag" );
	
	/********************
	********************/
	int ErrorCount = 0;
	
	if(!setup_Cam(Cam_id)) ErrorCount++;
	if(!mov.setup(FileName, MovSound_on)) ErrorCount++;
	
	/********************
	settings.setInListener(this);
	settings.setOutListener(this);
	settings.sampleRate = 44100;
	settings.numInputChannels = 2;
	settings.numOutputChannels = 2;
	settings.bufferSize = bufferSize;
	
	soundStream.setup(settings);
	********************/
	soundStream.printDeviceList();
	
	/********************
	soundStream.setup()の位置に注意:最後
		setup直後、audioIn()/audioOut()がstartする.
		これらのmethodは、fft_threadにaccessするので、start前にReStart()によって、fft_threadが初期化されていないと、不正accessが発生してしまう.
	********************/
	if(!setup_SoundStream()) ErrorCount++;
	
	/********************
	********************/
	if(0 < ErrorCount)  std::exit(1);
}

/******************************
******************************/
bool ofApp::setup_Cam(int _Cam_id)
{
	/********************
	********************/
	VideoCam = new ofVideoGrabber;
	
	ofSetLogLevel(OF_LOG_VERBOSE);
    VideoCam->setVerbose(true);
	
	vector< ofVideoDevice > Devices = VideoCam->listDevices();// 上 2行がないと、List表示されない.
	
	if(_Cam_id == -1){
		// std::exit(1);
		return false;
	}else{
		if(Devices.size() <= _Cam_id) { ERROR_MSG(); std::exit(1); }
		
		VideoCam->setDeviceID(_Cam_id);
		VideoCam->initGrabber(FBO_VIDEO_WIDTH, FBO_VIDEO_HEIGHT);
		
		printf("> Cam size asked = (%d, %d), actual = (%d, %d)\n", int(FBO_VIDEO_WIDTH), int(FBO_VIDEO_HEIGHT), int(VideoCam->getWidth()), int(VideoCam->getHeight()));
		fflush(stdout);
	}
	
	return true;
}

/******************************
******************************/
bool ofApp::setup_SoundStream()
{
	/********************
	settings.setInListener(this);
	settings.setOutListener(this);
	settings.sampleRate = 44100;
	settings.numInputChannels = 2;
	settings.numOutputChannels = 2;
	settings.bufferSize = bufferSize;
	
	soundStream.setup(settings);
	********************/
	ofSoundStreamSettings settings;
	
	if( soundStream_Input_DeviceId == -1 ){
		return false;
		/*
		ofExit();
		return;
		*/
		
	}else{
		vector<ofSoundDevice> devices = soundStream.getDeviceList();
		
		if( soundStream_Input_DeviceId != -1 ){
			settings.setInDevice(devices[soundStream_Input_DeviceId]);
			settings.setInListener(this);
			settings.numInputChannels = AUDIO_BUFFERS;
		}else{
			settings.numInputChannels = 0;
		}
		
		if( soundStream_Output_DeviceId != -1 ){
			if(devices[soundStream_Output_DeviceId].name == "Apple Inc.: Built-in Output"){
				printf("!!!!! prohibited to use [%s] for output ... by SJ for safety !!!!!\n", devices[soundStream_Output_DeviceId].name.c_str());
				fflush(stdout);
				
				settings.numOutputChannels = 0;
				
			}else{
				settings.setOutDevice(devices[soundStream_Output_DeviceId]);
				settings.numOutputChannels = AUDIO_BUFFERS;
				settings.setOutListener(this); /* Don't forget this */
			}
		}else{
			settings.numOutputChannels = 0;
		}
		
		settings.numBuffers = 4;
		settings.sampleRate = AUDIO_SAMPLERATE;
		settings.bufferSize = AUDIO_BUF_SIZE;
	}
	
	/********************
	soundStream.setup()の位置に注意:最後
		setup直後、audioIn()/audioOut()がstartする.
		これらのmethodは、fft_threadにaccessするので、start前にReStart()によって、fft_threadが初期化されていないと、不正accessが発生してしまう.
	********************/
	soundStream.setup(settings);
	// soundStream.start();
	
	return true;
}

/******************************
description
	memoryを確保は、app start後にしないと、
	segmentation faultになってしまった。
******************************/
void ofApp::setup_Gui()
{
	/********************
	********************/
	Gui_Global = new GUI_GLOBAL;
	Gui_Global->setup("Wave", "gui.xml", 1000, 10);
}

/******************************
******************************/
void ofApp::Clear_fbo(ofFbo& fbo)
{
	fbo.begin();
	
	// Clear with alpha, so we can capture via syphon and composite elsewhere should we want.
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	ofClear(0, 0, 0, 0);
	
	fbo.end();
}

/******************************
******************************/
void ofApp::Refresh_FFTVerts()
{
	/********************
	********************/
	const float BarWidth = GRAPH_BAR_WIDTH__FFT_GAIN;
	const float BarSpace = GRAPH_BAR_SPACE__FFT_GAIN;
	
	const float CorrectionBarHeight = 5;
	
	
	/********************
	********************/
	/* */
	float Gui_DispGain = Gui_Global->Val_DispMax__FFTGain;
	for(int ch = 0; ch < NUM_AUDIO_CHS; ch++){
		for(int j = 0; j < AUDIO_BUF_SIZE/2; j++){
			double _Gain = fft_thread[ch]->getArrayVal_x_DispGain(j, Gui_DispGain, FBO_FFT_HEIGHT, true, false);
			Vboset_fft_Raw[ch].VboVerts[j * 4 + 0].set( BarSpace * j            , 0 );
			Vboset_fft_Raw[ch].VboVerts[j * 4 + 1].set( BarSpace * j            , _Gain );
			Vboset_fft_Raw[ch].VboVerts[j * 4 + 2].set( BarSpace * j  + BarWidth, _Gain );
			Vboset_fft_Raw[ch].VboVerts[j * 4 + 3].set( BarSpace * j  + BarWidth, 0 );
			
			_Gain = fft_thread[ch]->getArrayVal_x_DispGain(j, Gui_DispGain, FBO_FFT_HEIGHT, true, true);
			Vboset_fft_Corrected[ch].VboVerts[j * 4 + 0].set( BarSpace * j            , _Gain -  CorrectionBarHeight);
			Vboset_fft_Corrected[ch].VboVerts[j * 4 + 1].set( BarSpace * j            , _Gain );
			Vboset_fft_Corrected[ch].VboVerts[j * 4 + 2].set( BarSpace * j  + BarWidth, _Gain );
			Vboset_fft_Corrected[ch].VboVerts[j * 4 + 3].set( BarSpace * j  + BarWidth, _Gain -  CorrectionBarHeight );
		}
		
		Vboset_fft_Raw[ch].update();
		Vboset_fft_Corrected[ch].update();
	}
}

/******************************
******************************/
void ofApp::update(){
	/********************
	********************/
	static int t_LastINT = 0;
	int now = ofGetElapsedTimeMillis();
	
	/********************
	cal
	********************/
	for(int i = 0; i < NUM_AUDIO_CHS; i++) fft_thread[i]->update();
	
	/********************
	********************/
	update_spectrumImage();
	
	/********************
	vbo
	********************/
	Refresh_FFTVerts();
	
	/********************
	********************/
	VideoCam->update();
	if(VideoCam->isFrameNew()) { drawFbo_Cam(); }
	
	if(mov.update()) { mov.draw_to_fbo(fbo[GRAPH__MOV]); }
	
	drawFbo_Mov_And_Cam();
	
	/********************
	draw to fbo
	********************/
	/* */
	drawFbo_FFT(fbo[GRAPH__FFT_L], Vboset_fft_Raw[AUDIO_CH_L], Vboset_fft_Corrected[AUDIO_CH_L]);
	drawFbo_FFT(fbo[GRAPH__FFT_R], Vboset_fft_Raw[AUDIO_CH_R], Vboset_fft_Corrected[AUDIO_CH_R]);
	
	/********************
	********************/
	t_LastINT = now;
}


/******************************
******************************/
void ofApp::update_spectrumImage(){
	for(int ch = 0; ch < NUM_AUDIO_CHS; ch++){
		float Gain[AUDIO_BUF_SIZE/2];
		
		for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
			Gain[i] = ofMap(fft_thread[ch]->getArrayVal(i, true), 0, Gui_Global->Val_DispMax__FFTGain, 0, 1.0, true);
		}
		
		spectrumImage[ch].setFromPixels( Gain, AUDIO_BUF_SIZE/2, 1, OF_IMAGE_GRAYSCALE );
	}
}

/******************************
******************************/
void ofApp::draw(){
	/********************
	********************/
	ofDisableAlphaBlending();
	
	/********************
	********************/
	float now = ofGetElapsedTimef();
	
	/********************
	********************/
	// ofClear(0, 0, 0, 255);
	ofBackground(30);
	ofSetColor(255, 255, 255, 255);
	
	drawFbo_toScreen( fbo[GRAPH__FFT_L], Fbo_DispPos[GRAPH__FFT_L], fbo[GRAPH__FFT_L].getWidth(), fbo[GRAPH__FFT_L].getHeight() );
	drawFbo_toScreen( fbo[GRAPH__FFT_R], Fbo_DispPos[GRAPH__FFT_R], fbo[GRAPH__FFT_R].getWidth(), fbo[GRAPH__FFT_R].getHeight() );
	
	// drawFbo_toScreen_via_WaveSurfaceShader( fbo[GRAPH__MOV], Fbo_DispPos[GRAPH__MOV], fbo[GRAPH__MOV].getWidth(), fbo[GRAPH__MOV].getHeight(), spectrumImage[AUDIO_CH_L], ofVec2f(fbo[GRAPH__MOV].getWidth(), fbo[GRAPH__MOV].getHeight()/2) );
	// drawFbo_toScreen_via_WaveSurfaceShader( fbo[GRAPH__CAM], Fbo_DispPos[GRAPH__CAM], fbo[GRAPH__CAM].getWidth(), fbo[GRAPH__CAM].getHeight(), spectrumImage[AUDIO_CH_R], ofVec2f(0, fbo[GRAPH__MOV].getHeight()/2) );
	drawFbo_toScreen_via_WaveSurfaceShader( fbo[GRAPH__MOV_AND_CAM], Fbo_DispPos[GRAPH__MOV_AND_CAM], fbo[GRAPH__MOV_AND_CAM].getWidth(), fbo[GRAPH__MOV_AND_CAM].getHeight(), spectrumImage[AUDIO_CH_L], ofVec2f(fbo[GRAPH__MOV_AND_CAM].getWidth()/2, fbo[GRAPH__MOV_AND_CAM].getHeight()/2) );
	
	/********************
	********************/
	if(b_DispGui) Gui_Global->gui.draw();
	
	/*
	if(b_DispFrameRate){
		ofSetColor(255, 0, 0, 255);
		
		char buf[BUF_SIZE_S];
		sprintf(buf, "%5.1f", ofGetFrameRate());
		
		font[FONT_M].drawString(buf, 30, 30);
	}
	*/
}

/******************************
******************************/
void ofApp::drawFbo_toScreen(ofFbo& _fbo, const ofPoint& Coord_zero, const int Width, const int Height)
{
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(Coord_zero);
		
		_fbo.draw(0, 0, Width, Height);
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::drawFbo_toScreen_via_WaveSurfaceShader(ofFbo& _fbo, const ofPoint& Coord_zero, const int Width, const int Height, ofFloatImage& _spectrumImage, ofVec2f WaveCenter)
{
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(Coord_zero);
		
		shader_WaveSurface.begin();
			
			shader_WaveSurface.setUniform1f( "center_x", WaveCenter.x );
			shader_WaveSurface.setUniform1f( "center_y", WaveCenter.y );
			
			shader_WaveSurface.setUniformTexture( "texture_spectrum", _spectrumImage.getTextureReference(), 1 );
			
			ofSetColor( 255, 255, 255 );
			_fbo.draw(0, 0, Width, Height);
		
		shader_WaveSurface.end();
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::drawFbo_Mov_And_Cam()
{
	fbo[GRAPH__MOV_AND_CAM].begin();
		/********************
		********************/
		ofEnableAlphaBlending();
		// ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		/********************
		********************/
		fbo[GRAPH__MOV].draw(0, 0);
		fbo[GRAPH__CAM].draw(fbo[GRAPH__MOV_AND_CAM].getWidth()/2, 0);
		
	fbo[GRAPH__MOV_AND_CAM].end();
}

/******************************
******************************/
void ofApp::drawFbo_Cam()
{
	fbo[GRAPH__CAM].begin();
		/********************
		********************/
		ofEnableAlphaBlending();
		// ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		/********************
		********************/
		VideoCam->draw(0, 0, fbo[GRAPH__CAM].getWidth(), fbo[GRAPH__CAM].getHeight());
		
	fbo[GRAPH__CAM].end();
}

/******************************
******************************/
void ofApp::drawFbo_FFT(ofFbo& fbo, VBO_SET& _Vboset_fft_Raw, VBO_SET& _Vboset_fft_Corrected)
{
 	float Screen_y_max = fbo.getHeight();
	float Val_Disp_y_Max = Gui_Global->Val_DispMax__FFTGain;
	
	/********************
	********************/
	fbo.begin();
		ofEnableAlphaBlending();
		// ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		ofPushStyle();
		ofPushMatrix();
			/********************
			********************/
			ofTranslate(0, fbo.getHeight() - 1);
			ofScale(1, -1, 1);
			
			/********************
			y目盛り
			********************/
			if(0 < Val_Disp_y_Max){
				const int num_lines = 5;
				const double y_step = Screen_y_max/num_lines;
				for(int i = 0; i < num_lines; i++){
					int y = int(i * y_step + 0.5);
					
					ofSetColor(ofColor(50));
					ofSetLineWidth(1);
					ofNoFill();
					ofDrawLine(0, y, fbo.getWidth(), y);
		
					/********************
					********************/
					char buf[BUF_SIZE_S];
					sprintf(buf, "%7.4f", Val_Disp_y_Max/num_lines * i);
					
					ofSetColor(ofColor(200));
					ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
					font[FONT_S].drawString(buf, fbo.getWidth() - 1 - font[FONT_S].stringWidth(buf) - 10, -y); // y posはマイナス
					ofScale(1, -1, 1); // 戻す.
				}
			}
			
			/********************
			zoneを示す縦Line
			********************/
			for(int i = 0; i < NUM_FREQ_ZONES; i++){
				int _x = ZoneFreqId_From[i] * GRAPH_BAR_SPACE__FFT_GAIN;
				
				ofSetColor(ofColor(50));
				ofSetLineWidth(1);
				ofNoFill();
				
				ofDrawLine(_x, 0, _x, fbo.getHeight());
			}
			
			/********************
			********************/
			ofSetColor(255);
			glPointSize(1.0);
			glLineWidth(1);
			ofFill();
			
			_Vboset_fft_Raw.draw(GL_QUADS);
			_Vboset_fft_Corrected.draw(GL_QUADS);
			
		ofPopMatrix();
		ofPopStyle();
	fbo.end();
}

/******************************
******************************/
void ofApp::keyPressed(int key){
	switch(key){
		case 'd':
			b_DispGui = !b_DispGui;
			break;
			
		case 't':
			mov.SeekToTop();
			break;
			
		case ' ':
			{
				b_Log = true;
				
				char buf[BUF_SIZE_S];
				
				sprintf(buf, "image_%d.png", png_id);
				ofSaveScreen(buf);
				// ofSaveFrame();
				printf("> %s saved\n", buf);
				
				png_id++;
			}
			break;
	}
}

/******************************
audioIn/ audioOut
	同じthreadで動いている様子。
	また、audioInとaudioOutは、同時に呼ばれることはない(多分)。
	つまり、ofAppからaccessがない限り、変数にaccessする際にlock/unlock する必要はない。
	ofApp側からaccessする時は、threadを立てて、安全にpassする仕組みが必要
******************************/
void ofApp::audioIn(ofSoundBuffer & buffer)
{
    for (int i = 0; i < buffer.getNumFrames(); i++) {
        AudioSample.Left[i] = buffer[2*i];
		AudioSample.Right[i] = buffer[2*i+1];
    }
	
	/********************
	FFT Filtering
	1 process / block.
	********************/
	fft_thread[AUDIO_CH_L]->update__Gain(AudioSample.Left);
	fft_thread[AUDIO_CH_R]->update__Gain(AudioSample.Right);
}  

/******************************
******************************/
void ofApp::audioOut(ofSoundBuffer & buffer)
{
	/********************
	x	:input -> output
	o	:No output.
	********************/
    for (int i = 0; i < buffer.getNumFrames(); i++) {
		buffer[2*i  ] = AudioSample.Left[i];
		buffer[2*i+1] = AudioSample.Right[i];
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
