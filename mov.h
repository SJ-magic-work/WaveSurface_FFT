/************************************************************
 ofxHapPlayerを使うと、時間と共にCPU占有率が高くなってしまう現象が見られた。
色々とtestしたみたが、解決せず。
ofVideoPlayer で問題なく動作するので、こちらで対応。
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"

#include "sj_common.h"

/************************************************************
************************************************************/
class MOV : Noncopyable{
private:
	/****************************************
	****************************************/
	ofVideoPlayer video;
	
	/****************************************
	****************************************/
	void print_MovFileList();
	
	void setup_video(ofVideoPlayer& video, bool MovSound_on);
	
public:
	/****************************************
	****************************************/
	MOV();
	~MOV();
	
	bool setup(string FileName, bool MovSound_on);
	bool update();
	
	void draw_to_fbo(ofFbo& fbo);
	void draw(float _x, float _y, float _w, float _h);
	
	void SeekToTop();
	void Seek(float pos);
	void setPaused(bool b_pause);
};

