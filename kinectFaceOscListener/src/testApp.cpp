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

	// ( int _meshIndex1 , int _meshIndex2 , string _label ) 

	FeatureRelationship rs1  ;
	rs1.setup( 82 , 84 , "mouth_height" ) ; 
	featureRelations.push_back( rs1 ) ; 

	FeatureRelationship rs2  ;
	rs2.setup( 31 , 64 , "mouth_wide" ) ; 
	featureRelations.push_back( rs2 ) ; 

	FeatureRelationship rs3  ;
	rs3.setup( 15 , 20 , "eyebrow_left" ) ; 
	featureRelations.push_back( rs3 ) ;

	FeatureRelationship rs4  ;
	rs4.setup( 51 , 53 , "eyebrow_right" ) ; 
	featureRelations.push_back( rs4 ) ;
	

	loadFeatureCalibration( "faceCalibrationData.xml" ) ; 
	//cam.setPosition( -500 ,  , -500 ) ; 
	// 
	bTraining = false ; 
	bResetData = false ; 
	bConnectSender = false ; 

	setupUI( ) ; 
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
				rawMesh.clear( ); 
				//Set default high so they get overwritten
				faceBounds = ofRectangle( 10000 , 100000 , -10000 , -10000 ) ; 
				for ( int p = 0 ; p < (numFacePoints) ; p+=2 ) 
				{

					ofPoint _p = ofPoint ( m.getArgAsFloat(p) , m.getArgAsFloat(p+1) ) ;
					int i = p / 2 ; 

					if ( i < VERTS_PER_FACE ) 
					{
						rawMesh.addVertex( _p ) ; 
						lerpedPoints[i] = lerpedPoints[i].interpolate( _p , facePointSmoothing ) ; 
						faceCentroid += lerpedPoints[i] ; 

						if ( lerpedPoints[i].x < faceBounds.x ) 
							faceBounds.x = lerpedPoints[i].x ; 
						if ( lerpedPoints[i].y < faceBounds.y ) 
							faceBounds.y = lerpedPoints[i].y ;
						if ( lerpedPoints[i].x > faceBounds.width )
							faceBounds.width = lerpedPoints[i].x ; 
						if ( lerpedPoints[i].y > faceBounds.height ) 
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

	int meshSize = rawMesh.getNumVertices() ; 
	if ( meshSize > 0 ) 
	{
		for ( int i = 0 ; i < featureRelations.size() ; i++ ) 
		{
			featureRelations[i].update( rawMesh.getVertex( featureRelations[i].meshIndex1 ) ,
									    rawMesh.getVertex( featureRelations[i].meshIndex2 ) , bTraining ) ;  

		}
	}
	
	if ( lerpedPoints.size() > 0 ) 
	{
		if ( bConnectSender == true ) 
		{
			for ( int i = 0 ; i < featureRelations.size() ; i++ ) 
			{
				ofxOscMessage m ;
	
				m.clear() ; 
				m.setAddress( "/" + featureRelations[i].label ) ; 
				m.addFloatArg( featureRelations[i].ratio ) ; 

				sender.sendMessage( m ) ; 
				
				//featureRelations[i].update( rawMesh.getVertex( featureRelations[i].meshIndex1 ) ,
				//					    rawMesh.getVertex( featureRelations[i].meshIndex2 ) , bTraining ) ;  

			}
			
		}
	}
}


//--------------------------------------------------------------
void testApp::draw(){

	ofEnableAlphaBlending() ; 
	string buf;
	buf = "listening for osc messages on port" + ofToString(PORT) + "\n" + "t - training face? " + ofToString( bTraining ) ;
	ofDrawBitmapString(buf, 10, 20);

	for(int i = 0; i < NUM_MSG_STRINGS; i++){
		ofDrawBitmapString(msg_strings[i], 10, 40 + 15 * i);
	}

	ofFill( ) ; 
	ofSetColor( 250 , 250 , 212 , 68) ; 
	ofNoFill() ; 
	//faceMesh.drawWireframe() ; 

	//cam.setDistance( 75.0f ) ; 
	
	ofVec2f faceOffset = ofVec2f ((faceBounds.width - faceBounds.x)/-2 , (faceBounds.height - faceBounds.y)/-2 ) ; 

	cam.setPosition( 320 , 150 , -150 ) ; 
	cam.lookAt( faceCentroid , ofVec3f( 0 , -1 , 0 )  ) ; 
	cam.begin() ; 
	//ofScale( 1 , -1 , 1 ) ; 
	//ofTranslate( 0 , -ofGetHeight() , 0 ) ; 
	
	
	//ofTranslate( -ofGetWidth()   ,  ofGetHeight() ) ; 
	//ofScale( 3 , -3 , 1 ) ; 
	
		ofPushMatrix() ; 
		//	ofTranslate( faceOffset  ) ; 
			faceTriangulate.draw(  ) ;

			ofSetColor( 255 , 105 , 180 ) ;
			rawMesh.setMode( OF_PRIMITIVE_POINTS ) ; 
			glPointSize( 5.0f ) ; 
			rawMesh.draw( ) ; 
			ofPushStyle() ; 
				ofNoFill( ) ; 
				ofSetColor( 255 , 0 , 0 ) ;
				ofSetLineWidth( 5 ) ;
				ofRect( faceBounds.x, faceBounds.y , faceBounds.width - faceBounds.x , faceBounds.height - faceBounds.y ) ; 
			ofPopStyle() ;
		ofPopMatrix() ; 
	cam.end() ; 

	int n = rawMesh.getNumVertices();
	float nearestDistance = 0;
	ofVec2f nearestVertex;
	int nearestIndex;
	ofVec2f mouse(mouseX, mouseY);
	
	for(int i = 0; i < n; i++) {
		ofVec3f cur = cam.worldToScreen(rawMesh.getVertex(i));
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

	if ( lerpedPoints.size() > 0 ) 
	{
		float debugSquare = 150 ; 
		for ( int i = 0 ; i < featureRelations.size() ; i++ ) 
		{
			featureRelations[i].debugDraw( 15 + i * debugSquare , ofGetHeight() - debugSquare - 15 , debugSquare , debugSquare ) ; 
		}
	}

}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	switch ( key ) 
	{
		case 's':
		case 'S':
			saveFeatureCalibration ( "faceCalibrationData.xml" ) ; 
			break ;

			/*
		case 't':
		case 'T':
			bTraining = !bTraining ; 
			break ; 

		case 'r':
		case 'R':
			for ( int i = 0 ; i < featureRelations.size() ; i++ ) 
		{
			featureRelations[i].resetTraining() ;  
		}*/
			break ; 
	}

	cout << "keyPressed :: " << key << endl ;  
}

void testApp::saveFeatureCalibration( string _path ) 
{
	ofxXmlSettings xml ; 
	xml.clear() ; 
	for ( int i = 0 ; i < featureRelations.size() ; i++ ) 
	{
		int tagNum = xml.addTag( "feature" ) ; 
		xml.pushTag( "feature" , tagNum ) ; 
			xml.setValue( "label" , featureRelations[i].label ) ; 
			xml.setValue( "meshIndex1" , featureRelations[i].meshIndex1 ) ; 
			xml.setValue( "meshIndex2" , featureRelations[i].meshIndex2 ) ; 
			xml.setValue( "minDistance" , featureRelations[i].minDistance ) ; 
			xml.setValue( "maxDistance" , featureRelations[i].maxDistance ) ; 
		xml.popTag( ) ; 
	}

	xml.saveFile( _path ) ; 

	cout << "saved file to XML! : " << _path << endl ; 

}

void testApp::loadFeatureCalibration ( string _path ) 
{
	ofxXmlSettings xml ; 
	xml.clear() ; 
	xml.loadFile( _path ) ; 

	int nFeatures = xml.getNumTags( "feature" ) ; 
	if ( nFeatures < 1 ) 
	{
		cout << "no tags found! " << endl ; 
		return ; 
	}

	featureRelations.clear() ; 
	
	for ( int i = 0 ; i < nFeatures ; i++ ) 
	{
		xml.pushTag( "feature" , i ) ; 
			string label = xml.getValue( "label" , "noLabel" ) ; 
			int meshIndex1 = xml.getValue( "meshIndex1" ,  0 ) ; 
			int meshIndex2 = xml.getValue( "meshIndex2" , 1 ) ; 
			float minDistance = xml.getValue( "minDistance" ,  0.0f ) ; 
			float maxDistance = xml.getValue( "maxDistance" , 1.0f ) ; 
			 
			cout << "loading : " << label << " , [" << meshIndex1 << " , " << meshIndex2 << "] , " << minDistance << "<->" << maxDistance << endl ; 

			FeatureRelationship rs1  ;
			rs1.setup( meshIndex1 , meshIndex2 , label , minDistance , maxDistance ) ; 
			featureRelations.push_back( rs1 ) ; 
		xml.popTag( ) ; 
	}

	xml.saveFile( _path ) ; 
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

void testApp::setupUI ( ) 
{
	//sendPort

	float dim = 24; 
	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING; 
    float length = 320-xInit; 
	gui = new ofxUICanvas(0, 0, length+xInit, ofGetHeight());

	gui->addWidgetDown(new ofxUILabel("FACE TRACKING PARAMETERS", OFX_UI_FONT_LARGE ));
	//gui->addWidgetDown(new ofxUISlider(length-xInit,dim, 0.0, 255.0, red, "RED")); 
	gui->addWidgetDown(new ofxUIToggle( dim, dim, false, "ENABLE TRAINING")); 
	gui->addWidgetDown(new ofxUIToggle( dim, dim, false, "RESET TRAINING DATA")); 
	gui->addWidgetDown(new ofxUIToggle( dim, dim, false, "CONNECT SENDER OSC"));
	gui->addWidgetDown(new ofxUITextInput( length-xInit, "SENDING PORT", ofToString( sendPort ) , OFX_UI_FONT_LARGE)); 
	ofAddListener(gui->newGUIEvent,this,&testApp::guiEvent);

	gui->loadSettings( "gui/paramSettings.xml" ) ; 
}

void testApp::guiEvent(ofxUIEventArgs &e)
{
	string name = e.widget->getName(); 
	int kind = e.widget->getKind(); 

	/*

	case 't':
		case 'T':
			bTraining = !bTraining ; 
			break ; 

		case 'r':
		case 'R':
			for ( int i = 0 ; i < featureRelations.size() ; i++ ) 
		{
			featureRelations[i].resetTraining() ;  
		}
		//bConnectSender
		*/
	if(name == "ENABLE TRAINING") 
	{
		bTraining = ( (ofxUIToggle *) e.widget )->getValue() ; 
	}

	if(name == "CONNECT SENDER OSC" ) 
	{ 
		bConnectSender = ( (ofxUIToggle *) e.widget )->getValue() ; 
		if ( bConnectSender == true ) 
		{
			cout << "attempting to connect to : " << " localHost : " << " on port : " << sendPort << endl; 
			sender.setup( "localhost" , sendPort ) ; 
		}
	}

	if(name == "RESET TRAINING DATA")
	{
		bResetData = ( (ofxUIToggle *) e.widget )->getValue() ; 
		if ( bResetData == true ) 
		{
			for ( int i = 0 ; i < featureRelations.size() ; i++ ) 
			{
			featureRelations[i].resetTraining() ;  
			}
		}
		bResetData = false ;
	}	

	if ( name == "SENDING PORT" )
	{
		ofxUITextInput *textinput = (ofxUITextInput *) e.widget; 
        if(textinput->getTriggerType() == OFX_UI_TEXTINPUT_ON_ENTER)
        {
            //cout << "ON ENTER: "; 
//            ofUnregisterKeyEvents((testApp*)this); 
        }
        else if(textinput->getTriggerType() == OFX_UI_TEXTINPUT_ON_FOCUS)
        {
           // cout << "ON FOCUS: "; 
        }
        else if(textinput->getTriggerType() == OFX_UI_TEXTINPUT_ON_UNFOCUS)
        {
          //  cout << "ON BLUR: "; 
//            ofRegisterKeyEvents(this);             
        }        
        string output = textinput->getTextString(); 
		sendPort = ofToInt( output ) ; 
        cout << output << endl; 
	}


}
