#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxDelaunay.h"

// listen on port 12345
#define PORT 12345
#define NUM_MSG_STRINGS 20
#define VERTS_PER_FACE 86 

class testApp : public ofBaseApp {
	public:

		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		ofxOscReceiver receiver;

		int current_msg_string;
		string msg_strings[NUM_MSG_STRINGS];
		float timers[NUM_MSG_STRINGS];

		vector<ofPoint> lerpedPoints ; 
		ofxDelaunay faceTriangulate ; 
		
		ofEasyCam cam ; 
		ofPoint faceCentroid ; 

		ofRectangle faceBounds ; 
		float facePointSmoothing ; 
};
