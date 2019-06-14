/************************************************************
************************************************************/
#include "mov.h"

/* for dir search */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h> 
#include <dirent.h>
#include <string>

using namespace std;

/* */



/************************************************************
************************************************************/

/******************************
******************************/
MOV::MOV()
{
}

/******************************
******************************/
MOV::~MOV()
{
}

/******************************
******************************/
bool MOV::setup(string FileName, bool MovSound_on)
{
	/********************
	********************/
	if(FileName == "NotSet"){
		print_MovFileList();
		// std::exit(1);
		return false;
	}
	
	/********************
	********************/
	if(video.load("mov/" + FileName)){
		setup_video(video, MovSound_on);
	}else{
		printf("> No mov file.\n");
		fflush(stdout);
		return false;
	}
	
	return true;
}

/******************************
******************************/
void MOV::print_MovFileList()
{
	/********************
	********************/
	const string dirname = "../../../data/mov";
	
	DIR *pDir;
	struct dirent *pEnt;
	struct stat wStat;
	string wPathName;

	pDir = opendir( dirname.c_str() );
	if ( NULL == pDir ) { ERROR_MSG(); std::exit(1); }
	
	printf("> movie files\n");
	pEnt = readdir( pDir );
	while ( pEnt ) {
		// .と..は処理しない
		if ( strcmp( pEnt->d_name, "." ) && strcmp( pEnt->d_name, ".." ) ) {
		
			wPathName = dirname + "/" + pEnt->d_name;
			
			// ファイルの情報を取得
			if ( stat( wPathName.c_str(), &wStat ) ) {
				printf( "Failed to get stat %s \n", wPathName.c_str() );
				break;
			}
			
			if ( S_ISDIR( wStat.st_mode ) ) {
				// nothing.
			} else {
			
				vector<string> str = ofSplitString(pEnt->d_name, ".");
				if( (str[str.size()-1] == "mp4") ||  (str[str.size()-1] == "mov") ){
					// string str_NewFileName = wPathName;
					// string str_NewFileName = pEnt->d_name;
					string* str_NewFileName = new string(pEnt->d_name);
					
					// SoundFileNames.push_back(str_NewFileName);
					printf("\t%s\n", str_NewFileName->c_str());
				}
			}
		}
		
		pEnt = readdir( pDir ); // 次のファイルを検索する
	}

	closedir( pDir );
	
	printf("\n");
}

/******************************
******************************/
void MOV::setup_video(ofVideoPlayer& video, bool MovSound_on)
{	
	if(video.isLoaded()){
		/*
		printf("loaded movie : (%f, %f)\n", video.getWidth(), video.getHeight());
		fflush(stdout);
		*/
		
		video.setLoopState(OF_LOOP_NORMAL);
		// video.setLoopState(OF_LOOP_PALINDROME);
		
		video.setSpeed(1);
		
		if(MovSound_on)	video.setVolume(1.0);
		else			video.setVolume(0.0);
		
		video.play();
	}
}

/******************************
******************************/
void MOV::SeekToTop()
{
	if(video.isLoaded()){
		video.setPosition(0); // percent.
	}
}

/******************************
******************************/
void MOV::Seek(float pos)
{
	if(video.isLoaded()){
		if(video.isPaused()) video.setPosition(pos); // percent.
	}
}

/******************************
******************************/
void MOV::setPaused(bool b_pause)
{
	if(video.isLoaded()){
		video.setPaused(b_pause);
	}
}

/******************************
******************************/
bool MOV::update()
{
	if(video.isLoaded()){
		video.update();
		if(video.isFrameNew())	return true;
		else					return false;
	}
}

/******************************
******************************/
void MOV::draw_to_fbo(ofFbo& fbo)
{
	if(!video.isLoaded()) return;
	
	fbo.begin();
		/* */
		ofEnableAlphaBlending();
		// ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		/* */
		video.draw(0, 0, fbo.getWidth(), fbo.getHeight());
	fbo.end();
}

/******************************
******************************/
void MOV::draw(float _x, float _y, float _w, float _h)
{
	ofSetColor(255, 255, 255, 255);
	video.draw(_x, _y, _w, _h);
}

