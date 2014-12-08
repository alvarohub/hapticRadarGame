#ifndef _OBSTACLE_H_
#define _OBSTACLE_H_

#include "ofMain.h"

#define OBJECT_RESOLUTION 20 // this is the number of points forming the disk of the coin
#define WALL_HEIGHT

struct Obstacle {
    
    static int scoreCoins;
    
    static ofImage imageCoin;
    inline static void loadImageCoin() {
        imageCoin.loadImage("images/oneReal.png");
    }
    
    static ofSoundPlayer soundCoin;
    inline static void loadSoundCoin() {
        soundCoin.loadSound("sounds/moneySound2.wav");
    }
    
	float x,y;
    float radius;
    
	ofMesh object;
	ofPolyline object2d;
	
    // Modes and graphical properties:
    
	// In the line of sight of the haptic radar
	bool isTouched;
    bool tooClose; // this means the object is ready to be "eaten"
    bool eaten;
    
    unsigned long long startTouchedTimer;
    
    Obstacle( ofVec2f pos, float _r ) {
		eaten = false;
        tooClose=false;
        createRepresentation( pos.x, pos.y, _r );
        
    }
    
	Obstacle( float _x, float _y, float _r ) {
		x = _x;
		y = _y;
        radius=_r;
        
		eaten = false;
        tooClose=false;
        
        createRepresentation(_x, _y, _r);
    }
    
	/*Obstacle( const Obstacle& _rhs ) :
     object(_rhs.object), object2d(_rhs.object2s)
     {
     x = _rhs.x;
     y = _rhs.y;
     radius = _rhs.radius;
     eaten = _rhs.eaten;
     isTouched = _rhs.isTouched;
     }*/
    
    
    
    inline void createRepresentation(float _x, float _y, float _r) {
        float mazeWallHeight = 1;
        ofFloatColor color(0.5,0.5,0.5);
        ofFloatColor colorupper(0.5,0.8,0.5);
        
        object.clear();
        object2d.clear();
        
		radius = _r;
		x = _x;
		y = _y;
        
        //1) 3D representation:
        // NOTE: use a mesh, or better in OF8 a mesh based cylinder... in OF7 it is better to just use the glut cylinder...
        //		for( int i=0; i<OBJECT_RESOLUTION; ++i ) {
        //
        //			object.addVertex( ofVec3f(_x + _r*cos(i*2*PI/OBJECT_RESOLUTION),
        //                                                     _y + _r*sin(i*2*PI/OBJECT_RESOLUTION), 1.4) );
        //			object.addColor(color);
        //			object.addVertex( ofVec3f(_x + _r*cos((i+1)*2*PI/OBJECT_RESOLUTION),
        //                                                     _y + _r*sin((i+1)*2*PI/OBJECT_RESOLUTION), mazeWallHeight/CONVERSION_SVG) );
        //			object.addColor(colorupper);
        //			object.addVertex( ofVec3f(_x + _r*cos(i*2*PI/OBJECT_RESOLUTION),
        //                                                     _y + _r*sin(i*2*PI/OBJECT_RESOLUTION),  mazeWallHeight/CONVERSION_SVG) );
        //			object.addColor(colorupper);
        //
        //			object.addVertex( ofVec3f(_x + _r*cos(i*2*PI/OBJECT_RESOLUTION),
        //                                                     _y + _r*sin(i*2*PI/OBJECT_RESOLUTION), 1.4) );
        //			object.addColor(color);
        //			object.addVertex( ofVec3f(_x + _r*cos((i+1)*2*PI/OBJECT_RESOLUTION),
        //                                                     _y + _r*sin((i+1)*2*PI/OBJECT_RESOLUTION), mazeWallHeight/CONVERSION_SVG) );
        //			object.addColor(colorupper);
        //			object.addVertex( ofVec3f(_x + _r*cos((i+1)*2*PI/OBJECT_RESOLUTION),
        //                                                     _y + _r*sin((i+1)*2*PI/OBJECT_RESOLUTION), 1.4) );
        //			object.addColor(color);
        //        }
        
        //2) 2D Representation:
        // IMPORTANT: this NEEDS to be a polyline for rangefinder detection!
        for( int i=0; i<OBJECT_RESOLUTION; ++i ) {
	        object2d.addVertex(ofPoint(_x + _r*cos(i*2*PI/OBJECT_RESOLUTION),_y + _r*sin(i*2*PI/OBJECT_RESOLUTION)));
	        object2d.lineTo(ofPoint(_x + _r*cos((i+1)*2*PI/OBJECT_RESOLUTION),_y + _r*sin((i+1)*2*PI/OBJECT_RESOLUTION)));
		}
    }
    
    inline void wasTouched() {
        if (!eaten) { // not eaten yet, I will "hold" the coin for a second
            tooClose=true;
            startTouchedTimer=ofGetElapsedTimeMillis();
            // produce the money sound once:
            soundCoin.play();
        }
    }
    
    inline void updateEffect() {
        if (tooClose&&!eaten) { // otherwise do nothing
            if (ofGetElapsedTimeMillis()-startTouchedTimer>0) {
                scoreCoins++;
                eaten=true; // make it dissapear
            }
        }
    }
    
    // ======================== DRAWING ========================
	inline void draw3d(bool overriden) { // mode true for an overriden obstacle!
        ofSetCircleResolution(OBJECT_RESOLUTION);
        ofSetLineWidth(2);
        if (eaten) {
            
        } else {
            if (overriden){
                //            ofNoFill();
                //            ofSetColor(255, 255, 0, 100);
                //            ofCone(x, y, 0.01, radius, 1.0);
                GLUquadricObj *quadratic;
                quadratic = gluNewQuadric();
                
                ofSetColor(255, 255, 255, 100);
                ofPushMatrix();
                ofTranslate(x, y, .01);
                ofNoFill();
                gluCylinder(quadratic,radius,radius,radius/20,32,1);
                ofEnableAlphaBlending();
                ofTranslate(0,0, radius/20);
                imageCoin.draw(-radius, radius, 2*radius, -2*radius);
                ofPopMatrix();
            }
            else {
                int col;
                if (tooClose)
                    col=200+55*cos(3*2*PI*1.0*ofGetElapsedTimeMillis()/1000);
                else col=255;
                //            ofFill();
                //            ofSetColor(col, col, 0, 255);
                //            ofCone(x, y, 0.01, radius, 1.0);
                //            ofNoFill();
                //            ofSetColor(col, col, 0, 100);
                //            ofCone(x, y, 0.01, radius, 1.0);
                
                GLUquadricObj *quadratic;
                quadratic = gluNewQuadric();
                
                //ofSetColor(col, col, 0, 255);
                ofSetColor(col, col, col, 255);
                ofPushMatrix();
                ofTranslate(x, y, .01);
                ofFill();
                gluCylinder(quadratic,radius,radius,radius/20,32,1);
                ofEnableAlphaBlending();
                ofTranslate(0,0, radius/20);
                imageCoin.draw(-radius, radius, 2*radius, -2*radius);
                ofPopMatrix();
            }
        }
        
	}
    
	inline void draw2d(bool overriden) {
        if (eaten) {
            
            // draw a number? or a greyed cross? a hole?
            
        } else {
            
            if (overriden) {
                ofSetColor(255, 0, 0, 100);
                object2d.draw();
            }
            else {
                int col;
                if (tooClose)
                    col=200+55*cos(3*2*PI*1.0*ofGetElapsedTimeMillis()/1000); // period is 1 sec
                else col=255;
                
                ofPushMatrix();
                ofTranslate(0,0,.1);
                
                ofSetColor(col, col, col, 255);
                ofEnableAlphaBlending();
                imageCoin.draw(x-radius, y+radius, 2*radius, -2*radius);
                
                // ofTranslate(0,0,.1);
                // ofSetColor(255, 0, 0, 255);
                // object2d.draw();
                
                ofPopMatrix();
            }
        }
    }
};

    
    
#endif // _OBSTACLE_H_