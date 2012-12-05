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
	facePointSmoothing = 0.5f ; 

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
				//Set default high so they get overwritten
				faceBounds = ofRectangle( 10000 , 100000 , -10000 , -10000 ) ; 
				for ( int p = 0 ; p < (numFacePoints) ; p+=2 ) 
				{

					ofPoint _p = ofPoint ( m.getArgAsFloat(p) , m.getArgAsFloat(p+1) ) ;
					int i = p / 2 ; 

					if ( i < VERTS_PER_FACE ) 
					{
						lerpedPoints[i] = lerpedPoints[i].interpolate( _p , facePointSmoothing ) ; 
						faceCentroid += lerpedPoints[i] ; 

						if ( lerpedPoints[i].x < faceBounds.x ) 
							faceBounds.x = lerpedPoints[i].x ; 
						if ( lerpedPoints[i].y < faceBounds.y ) 
							faceBounds.y = lerpedPoints[i].y ;
						if ( lerpedPoints[i].x > faceBounds.width )
							faceBounds.width = lerpedPoints[i].x ; 
						if ( lerpedPoints[i].y < faceBounds.height ) 
							faceBounds.height = lerpedPoints[i].y ; 


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

	cam.setDistance( 365.0f ) ; 
	cam.lookAt( faceCentroid ) ; 
	cam.begin() ; 
	//ofScale( 1 , -1 , 1 ) ; 
	//ofTranslate( 0 , -ofGetHeight() , 0 ) ; 
	
	//ofTranslate( -ofGetWidth()   ,  ofGetHeight() ) ; 
	//ofScale( 3 , -3 , 1 ) ; 
	
		ofPushMatrix() ; 
			faceTriangulate.draw( ) ;
			ofPushStyle() ; 
				ofNoFill( ) ; 
				ofSetColor( 255 , 0 , 0 ) ;
				ofSetLineWidth( 10 ) ;
				ofRect( faceBounds.x, faceBounds.y , faceBounds.width - faceBounds.x , faceBounds.height - faceBounds.y ) ; 
			ofPopStyle() ;
		ofPopMatrix() ; 
	cam.end() ; 

	int n = faceTriangulate.triangleMesh.getNumVertices();
	float nearestDistance = 0;
	ofVec2f nearestVertex;
	int nearestIndex;
	ofVec2f mouse(mouseX, mouseY);
	for(int i = 0; i < n; i++) {
		ofVec3f cur = cam.worldToScreen(faceTriangulate.triangleMesh.getVertex(i));
		float distance = cur.distance(mouse);
		if(i == 0 || distance < nearestDistance) {
			nearestDistance = distance;
			nearestVertex = cur;
			nearestIndex = i;
		}
	}

	 


	ofSetColor(ofColor::gray);
	ofLine(nearestVertex, mouse);
	
	ofNoFill();
	ofSetColor(ofColor::yellow);
	ofSetLineWidth(2);
	ofCircle(nearestVertex, 4);
	ofSetLineWidth(1);
	
	ofVec2f offset(10, -10);
	ofDrawBitmapStringHighlight(ofToString(nearestIndex), mouse + offset);


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
