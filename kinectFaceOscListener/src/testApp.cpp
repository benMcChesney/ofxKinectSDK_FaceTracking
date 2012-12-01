#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

	// listen on the given port
	cout << "listening for osc messages on port " << PORT << "\n";
	receiver.setup(PORT);

	current_msg_string = 0;

	ofBackground(30, 30, 130);
	ofSetFrameRate( 30 ); 
	faceTriangulate.reset() ; 

	for ( int i = 0 ; i < VERTS_PER_FACE * 2 ; i++ ) 
	{
		lerpedPoints.push_back( ofPoint( ofRandomWidth() , ofRandomHeight() ) ) ; 
	}
	// 

}

//--------------------------------------------------------------
void testApp::update(){


	ofSetWindowTitle( "FPS: "+ ofToString( ofGetFrameRate() )) ; 
	// hide old messages
	for(int i = 0; i < NUM_MSG_STRINGS; i++){
		if(timers[i] < ofGetElapsedTimef()){
			msg_strings[i] = "";
		}
	}

	// check for waiting messages
	while(receiver.hasWaitingMessages()){
		// get the next message
		ofxOscMessage m;
		receiver.getNextMessage(&m);

		
		// check for mouse moved message
		if(m.getAddress() == "faceMesh/")
		{
			int numFacePoints = m.getNumArgs() ; 
			if ( numFacePoints > 45 ) 
			{
				//cout << "numFacePoints " << numFacePoints << endl ; 
				//faceTriangulate.reset() ; 
				bool addPoints = false ; 
				if ( lerpedPoints.size() < 1 ) 
					addPoints = true ; 

			
				faceCentroid = ofPoint() ; 
				for ( int p = 0 ; p < (numFacePoints) ; p+=2 ) 
				{

					ofPoint _p = ofPoint ( m.getArgAsFloat(p) , m.getArgAsFloat(p+1) ) ;
					int i = p / 2 ; 

					if ( i < VERTS_PER_FACE ) 
					{
						lerpedPoints[i] = lerpedPoints[i].interpolate( _p , 0.5f ) ; 
						faceCentroid += lerpedPoints[i] ; 
					//	Tweenzor::add( &lerpedPoints[i].x , lerpedPoints[i].x , _p.x , 0.0f , 0.12f , EASE_OUT_QUAD ) ; 
					//	Tweenzor::add( &lerpedPoints[i].y , lerpedPoints[i].x , _p.y , 0.0f , 0.12f , EASE_OUT_QUAD ) ; 
					}
				}

				faceCentroid /= ( float ) VERTS_PER_FACE ; 
			}
			
		}
		else{
			// unrecognized message: display on the bottom of the screen
			string msg_string;
			msg_string = m.getAddress();
			msg_string += ": ";
			for(int i = 0; i < m.getNumArgs(); i++){
				// get the argument type
				msg_string += m.getArgTypeName(i);
				msg_string += ":";
				// display the argument - make sure we get the right type
				if(m.getArgType(i) == OFXOSC_TYPE_INT32){
					msg_string += ofToString(m.getArgAsInt32(i));
				}
				else if(m.getArgType(i) == OFXOSC_TYPE_FLOAT){
					msg_string += ofToString(m.getArgAsFloat(i));
				}
				else if(m.getArgType(i) == OFXOSC_TYPE_STRING){
					msg_string += m.getArgAsString(i);
				}
				else{
					msg_string += "unknown";
				}
			}
			// add to the list of strings to display
			msg_strings[current_msg_string] = msg_string;
			timers[current_msg_string] = ofGetElapsedTimef() + 5.0f;
			current_msg_string = (current_msg_string + 1) % NUM_MSG_STRINGS;
			// clear the next line
			msg_strings[current_msg_string] = "";
		}

	}

	if ( lerpedPoints.size() > 0 ) 
	{
		faceTriangulate.reset() ; 
		for ( int i = 0 ; i < VERTS_PER_FACE ; i++ ) 
		{
			faceTriangulate.addPoint ( lerpedPoints[i] ) ;
		}
		faceTriangulate.triangulate() ; 
	}
	
}


//--------------------------------------------------------------
void testApp::draw(){

	string buf;
	buf = "listening for osc messages on port" + ofToString(PORT);
	ofDrawBitmapString(buf, 10, 20);

	for(int i = 0; i < NUM_MSG_STRINGS; i++){
		ofDrawBitmapString(msg_strings[i], 10, 40 + 15 * i);
	}

	ofFill( ) ; 
	ofSetColor( 250 , 250 , 212 ) ; 
	ofNoFill() ; 
	//faceMesh.drawWireframe() ; 

	cam.begin() ; 
	//ofScale( 1 , -1 , 1 ) ; 
	//ofTranslate( 0 , -ofGetHeight() , 0 ) ; 
	
	ofTranslate( -ofGetWidth()   ,  ofGetHeight() ) ; 
	ofScale( 3 , -3 , 1 ) ; 
	faceTriangulate.draw( ) ;

	cam.end() ; 


}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	switch ( key ) 
	{
	}

	cout << "keyPressed :: " << key << endl ;  
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){

}
