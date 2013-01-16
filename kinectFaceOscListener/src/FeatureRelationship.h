#pragma once


#include "ofMain.h"

class FeatureRelationship
{
	public : 
		FeatureRelationship( ) { }
		~FeatureRelationship( ) { } 

		void setup ( int _meshIndex1 , int _meshIndex2 , string _label ) 
		{
			meshIndex1 = _meshIndex1 ; 
			meshIndex2 = _meshIndex2 ; 
			label = _label ; 

			resetTraining() ; 
		}

		void setup ( int _meshIndex1 , int _meshIndex2 , string _label , float _minDistance , float _maxDistance ) 
		{
			setup(  _meshIndex1 , _meshIndex2 , _label ) ; 

			minDistance = _minDistance ; 
			maxDistance = _maxDistance ; 

		}

		void loadCalibrationData ( float _minDistance , float _maxDistance ) 
		{
			minDistance = _minDistance ;  
			maxDistance = _maxDistance ; 
		}

		void update( ofVec2f meshVertex1 , ofVec2f meshVertex2 , bool bUpdateTraining ) 
		{
			currentDistance = meshVertex1.distance( meshVertex2 ) ; 
			if ( bUpdateTraining == true ) 
			{
				if ( currentDistance < minDistance ) 
					minDistance = currentDistance ; 
				if ( currentDistance > maxDistance ) 
					maxDistance = currentDistance ; 
			}
			ratio = ofMap( currentDistance , minDistance , maxDistance , 0.0f , 1.0f , true ) ; 
		}

		string generateString ( ) 
		{
			string status = "" ; 
			status = label + "\n" + ofToString( ratio ) ; 
			return status ; 
		}

		void debugDraw ( float x , float y , float width , float height ) 
		{
			ofPushMatrix() ; 
				ofPushStyle() ;
				ofTranslate( x , y ) ; 
					ofSetColor( 255 , 0 , 212 ) ;
					ofNoFill() ; 
					ofCircle( width/2 , height/2 , width * .45 ) ; 
					ofFill( ) ; 
					ofCircle( width/2 , height/2 , width * .45 * ratio ) ;

					ofDrawBitmapStringHighlight( generateString() , 0 , height - 15 ) ; 
				ofPopStyle() ;	
			ofPopMatrix() ; 
		}

		void resetTraining() 
		{
			//Max these outrageously high so that they 
			minDistance = 100000.0f ; 
			maxDistance = -100000.0f ;
		}

		int meshIndex1 , meshIndex2 ; 
		float minDistance , maxDistance , currentDistance ; 
		float ratio ; 
		string label ; 
};