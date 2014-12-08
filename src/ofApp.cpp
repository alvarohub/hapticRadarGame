#include "ofApp.h"

ofImage Obstacle::imageCoin;
ofSoundPlayer Obstacle::soundCoin;
int Obstacle::scoreCoins=0;

// Get the horizontal and vertical screen sizes in pixel
void GetDesktopResolution(int& horizontal, int& vertical)
{
    horizontal = ofGetScreenWidth();
    vertical = ofGetScreenHeight();
}

//--------------------------------------------------------------
void ofApp::setup() {
    
    ofSetFrameRate(60);
    
    //old OF default is 96 - but this results in fonts looking larger than in other programs.
    ofTrueTypeFont::setGlobalDpi(30);
    franklinBook14.loadFont("frabk.ttf", 45);
    franklinBook14.setLineHeight(18.0f);
    franklinBook14.setLetterSpacing(1.037);
    
    // call to static method of Obstacle (only one instance of the image in memory...)
    Obstacle::loadImageCoin();
    Obstacle::loadSoundCoin();
    sizeObstacle=0.5; // DIAMETER of the obstacle in meters
    numberObstacles=10;
    
    indexStringData = 0;
    auxDataBuffer_index = 0;
    
    
    // (1) Set default sensing/stimulation feedback paramterers and other things:
    // NOTE: this is only used the first time, when there is no XML file. Otherwise parameters are set when loading
    // the XML file, and modified/saved by an UI interface):
    
    defaultCOM_PORT_ID=0;
    
    radiusHead = 0.10; // all in meters
    closeLimit = 0.30; // note that close limit cannot - in principle - be smaller than the "radius" of the head
    farLimit   = 1.80; // (~similar to the sharp rangefinders...)
    
    conversionFunctionMode=LINEAR;
    conversionLevels=DISCRETE;
    numTactileLevels=4; // in case of continuous, this is just the maximum number of levels achievable by the motors, i.e. 256, (and the current value of numTactileLevels will be ignored)
    
    // Program default mode start:
    recordingOn=false;
    // startTimeRecording; // will be reset to current time when recordingOn goes from FALSE to TRUE
    intervalRecording = 100;// record every 100 milliseconds.
    
    // Name of the file where to save the measurements:
    svgFileName="labyrinths/square7x9m.svg";
    svgFileNameNoPath="square7x9m.svg";
    logDataFileName="testFile.txt"; // this should be opened only when saving (and the name can be changed by the ofxUI).
    
    
    indexStringData = 0;
    auxDataBuffer_index = 0;
    newDataReceived = false;
    
    
    GetDesktopResolution(hres,vres);
    
    ofSetWindowPosition(0, 50);
    ofSetWindowShape(hres, vres-100);
    vres=ofGetWindowHeight();
    hres=ofGetWindowWidth();
    
    // border around the space for situating the camera at init, and also for being able to "go outside" the drawing and still see the avatar on the screen:
    borderSpace=1; // in meters
    
    // Other default fixed values (read from XML file?)
    mazeWallHeight = 1.5; // height of the maze wall (could depend on the color code!!) in meters
    
    
    // Load the DEFAULT svg file:
    loadDrawing(svgFileName);
    
    // generate a random obstacle map by default?
    //generateObstacles(labyrinthBoundingBox, 20, 1.0, 2.0); // to modify: use the slider parameters
    
    // ========== OPENGL default parameters, light and material properties ==========
    ofSetSmoothLighting(true);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    //ofEnableAlphaBlending();
    ofDisableAlphaBlending();
    
    pointLight.setDiffuseColor( ofColor(255.f,255.f,255.f));
    pointLight.setSpecularColor( ofColor(255.f,255.f,255.f));
    pointLight.setPosition(10,10,100);
    pointLight.setAmbientColor(ofFloatColor(0.5,0.5,0.5));
    
    material.setShininess( 64 );
    materialColor.setBrightness(255.f);
    materialColor.setSaturation(100);
    materialColor.setHue(100);
    material.setSpecularColor(materialColor);
    
    //FOG:
    GLfloat fogColor[4] = {.9,.6,.6, 0.7}; //{240, 240, 240, 255};
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, 1);
    glHint(GL_FOG_HINT, GL_DONT_CARE);
    glFogf(GL_FOG_START, 0);
    glFogf(GL_FOG_END, 18);
    glDisable(GL_FOG);//glEnable(GL_FOG);
    
    
    
    // Initial Viewports (will be changed when toggling view modes)
    view_fp = ofRectangle(0, 0, hres*2/3, vres);
    view_2d = ofRectangle(hres*2/3,0,hres*1/3,vres);
    
    // INITIAL CAMERA POSITION (in meters) this should be either interactive, or from an XML file corresponding to each SVG drawing. By default, we will appear in a fixed position in the SVG drawing (relative to the size):
    
    eyesHeight=0.5;//1.0; // Default height (eye levels, in meters)
    outOfBodyDistance=0.6;
    
    // Set the position of the avatar:
    // Note: this uses the variable spaceWidth, so we need to have called loadDrawing first:
    resetToInitialPosition();
    
    maxSpeed = 1.0f; // this is the "maximum" speed (in m/s)
    angleMaxDipSpeed= 30; // in degrees, this correspond to the angle that produces the "maxSpeed". If the tilt is higher than this, the speed won't change.
    
    viewMode = 0;
    view3D = view2D = true;
    
    //1) Set perspective matrix for the 3d view: will depend on the viewport
    fovy=45; // in degrees
    distNearZ= radiusHead/2;
    distFarZ = 1000; // physically meaning we can see one kilometer...
    setProperPerspective();
    
    //2) 2d view (ORTHOGRAPHICS PROJECTION CAMERA)
    cam2d.setNearClip(-100);
    cam2d.setFarClip(1000);
    cam2d.enableOrtho();
    //cam2d.setGlobalOrientation(ofQuaternion(-90,ofVec3f(1,0,0)));
    cam2d.setPosition(0,0,0);
    
    
    //mySerial.listDevices();
    deviceList = mySerial.getDeviceList(); // this will be used to create a dropdown menu for selecting the proper COM port (after checking on the console listing)
    cout <<"Connected COM ports: " << endl;
    for(int k = 0; k < (int)deviceList.size(); k++){
        cout << "[" << deviceList[k].getDeviceID() << "] = "<< deviceList[k].getDeviceName().c_str() << endl;
    }
    
    // Setup GUI
#define UI_HEIGHT_VISIBLE 750
#define UI_HEIGHT_MINIMIZED 25
    gui = new ofxUICanvas(20,20,320,UI_HEIGHT_MINIMIZED);
    gui -> setTheme(OFX_UI_THEME_DEFAULT);//OFX_UI_THEME_MINLIME);//OFX_UI_THEME_MIDNIGHT);//OFX_UI_THEME_HACKER);//OFX_UI_THEME_BERLIN);//OFX_UI_THEME_METALGEAR2);
    gui -> setColorBack(ofColor( 0,0,50,150 ));
    
    //gui->addWidgetDown(new ofxUILabel("Controls", OFX_UI_FONT_LARGE));
    ofxUIToggle *minimtoggle=new ofxUIToggle("Minimize",true,16,16,300,5);
    gui->addWidgetRight( minimtoggle, OFX_UI_ALIGN_RIGHT );
    
    // LOAD LABYRINTH FILE
    gui->addSpacer(10);
    gui->addSpacer(10);
    gui->addWidgetDown( new ofxUILabelButton( "Load Labyrinth", false, 150, OFX_UI_FONT_MEDIUM));//OFX_UI_FONT_SMALL) );
    // Fill the space with "coins":
    gui->addWidgetRight( new ofxUILabelButton( "Fill with Coins", false, 150,OFX_UI_FONT_MEDIUM));//OFX_UI_FONT_SMALL) );
    
    gui->addSpacer(10);
    gui->addLabel("Number of coins:", OFX_UI_FONT_MEDIUM);
    gui->addNumberDialer("Number coins", 0, 50, numberObstacles, 0);
    gui->addLabel("Diameter coin:", OFX_UI_FONT_MEDIUM);
    gui->addNumberDialer("Size coin", 0.1, 1.0, sizeObstacle, 1);
    
    
    // CALIBRATING SENSOR
    gui->addSpacer(10);
    gui->addSpacer(10);
    gui->addWidgetDown( new ofxUILabelButton(  "Calibrate", false, 150, OFX_UI_FONT_MEDIUM));
    
    // SET POSITION AVATAR
    gui->addWidgetRight( new ofxUILabelButton("Reset Position", false,  150, OFX_UI_FONT_MEDIUM) );
    gui->setWidgetFontSize(OFX_UI_FONT_MEDIUM);
    
    // TOGGLE RECORDING
    gui->addSpacer(10);
    //ofxUIToggle* recToggle = new  ofxUIToggle( "Toggle Recording", false, 16,16);
    experimentButton = new ofxUILabelButton( "Start Experiment", false, 150, OFX_UI_FONT_MEDIUM);
    gui->addWidgetDown( experimentButton );
    gui->setWidgetFontSize(OFX_UI_FONT_MEDIUM);
    
    // SAVING RECORDING
    gui->addWidgetRight( new ofxUILabelButton( "Save Recording", false, 150, OFX_UI_FONT_MEDIUM) );
    gui->setWidgetFontSize(OFX_UI_FONT_MEDIUM);
    
    
    // DETECTION RANGE
    gui->addSpacer(10);
    gui->addSpacer(10);
    gui->addWidgetDown(new ofxUILabel("Detection Range", OFX_UI_FONT_MEDIUM) );
    gui->addRangeSlider("[min-max]",radiusHead,3.f,closeLimit,farLimit,300,16);
    
    // VIBRATION MODE for COINS:
    //gui->addWidgetDown(new ofxUIToggle("Periodic Coin Vibration", false, 16, 16) );
    gui->addToggle("Periodic Coin Vibration", false, 16,16);
    
    // SET MAX SPEED
    gui->addSpacer(10);
    gui->addSpacer(10);
    gui->addWidgetDown(new ofxUILabel("Speed", OFX_UI_FONT_MEDIUM) );
    gui->addSlider("Max Speed",0.2,5.0,maxSpeed,300,16);
    
    ofxUIToggle *mrtoggle = gui->addToggle("Manual Rotation", false, 16,16);
    ofxUIToggle *mmtoggle = gui->addToggle("Manual Motion", true, 16,16);
    ofxUIToggle *swtoggle = gui->addToggle("Enable sideways joystick", false, 16,16);
    
    // VIEW MODE
    gui->addSpacer(10);
    gui->addSpacer(10);
    std::vector<std::string> viewRadioTitles;
    viewRadioTitles.push_back("2D & 3D"); viewRadioTitles.push_back("2D Only"); viewRadioTitles.push_back("3D Only");
    viewModeRadio = new ofxUIRadio("View Mode", viewRadioTitles, OFX_UI_ORIENTATION_HORIZONTAL,16,16);
    gui->addLabel("View Mode", OFX_UI_FONT_MEDIUM);
    gui->addWidgetDown( viewModeRadio );
    
    // Show trajectory
    gui->addWidgetDown(new ofxUIToggle("Show Trajectory", false, 16, 16) );
    
    // Override walls or object detection for placement:
    gui->addWidgetDown(new ofxUIToggle("Override Walls", true, 16, 16) );
    gui->addWidgetDown(new ofxUIToggle("Override Coins", true, 16, 16) );
    
    // BUZZER MODE
    /*	std::vector<std::string> buzzerRadioTitles;
     buzzerRadioTitles.push_back("Continuous"); buzzerRadioTitles.push_back("Discrete");
     ofxUIRadio* buzzerRadioBtnz = new ofxUIRadio("Vibration Mode", buzzerRadioTitles, OFX_UI_ORIENTATION_HORIZONTAL,16,16);
     gui->addWidgetDown( buzzerRadioBtnz );
     */
    
    // SAVING SETTINGS
    //gui->addSpacer()->setPadding(10);
    gui->addSpacer(10);
    gui->addSpacer(10);
    //gui->addWidgetDown(new ofxUILabel("Save UI Settings", OFX_UI_FONT_MEDIUM) );
    gui->addWidgetDown( new ofxUILabelButton( "Save Settings", false, 150, OFX_UI_FONT_MEDIUM));//OFX_UI_FONT_SMALL) );
    
    gui->addSpacer(10);
    gui->addSpacer(10);
    vector<string> comPortID;
    for(int k = 0; k < (int)deviceList.size(); k++){
        comPortID.push_back(deviceList[k].getDeviceName());
        //cout << "[" << deviceList[k].getDeviceID() << "] = "<< deviceList[k].getDeviceName().c_str() << endl;
    }
    
    gui->addDropDownList("COM PORT", comPortID, 200);
    
    gui->setVisible(true);
    //gui->setAutoDraw(false);

    
    //gui->autoSizeToFitWidgets();
    ofAddListener(gui->newGUIEvent,this,&ofApp::guiEvent);
    //gui->loadSettings("GUI/guiSettings.xml"); // this is principle will also call the listener and update all the variables!
    // start with a minimized gui
    gui->setDimensions(gui->getRect()->width, 25);
    
    // ============== DEFAULT PRESETS (overrides some of the GUI saved settings, which is important for STARTING the experiment) ===================
    
    //mySerial.enumerateDevices();
    // this should be set to whatever com port your serial device is connected to.
    // (ie, COM4 on a pc, /dev/tty.... on linux, /dev/tty... on a mac)
    // arduino users check in arduino app....
    //int baud = 57600;
    //mySerial.setup(defaultCOM_PORT_ID, baud); //open the first device: NOTE THIS WILL BE OPENEND WHEN READING THE GUI ACTION
    timerRestartHanshake=ofGetElapsedTimeMillis();
    lastTimeSendRangesMotors=ofGetElapsedTimeMillis();
    
    // overrideWalls=true; // let's start without walls to be able to move freely (note: STARTING the experiment will put this variable to false automatically). Controlled by keyboard RETURN key.
    // overrideObstacles=true; // same things for objects.
    
    enableHeadCompass=false;
    enableDipSpeed=false;
    enableSideWays=false;
    // update the buttons with their default values:
    mrtoggle->setValue(!enableHeadCompass);
    mmtoggle->setValue(!enableDipSpeed);
    swtoggle->setValue(enableSideWays);
    
    minimtoggle->setValue(true);
    
	   
    // Default values of the rotation matrix in case we did not yet received data from the microcontroller:
    mag.set(0,1,0); // north towards up of the screen
    acc.set(0,0,1);
    east=mag.getCrossed(acc);
    east.normalize();
    acc.normalize();
    north=acc.getCrossed(east);
    rotz = ofMatrix4x4(
                       east.x, north.x, acc.x, 0,
                       east.y, north.y, acc.y, 0,
                       east.z, north.z, acc.z, 0,
                       0, 0, 0, 1
                       );
    joystickVector.set(0,0);
    joystickVectorZero.set(0,0);
    joystickVectorRaw.set(0,0);
    
    
    firstInitializeQuat = true;
    recordingOn = false;
    
    // Drawing details:
    // drawTrajectoryMode=false;
    
    // Manual control:
    headingManual=0;
    turnSign=moveUpSign=moveSideSign=0;
    
    pauseVibrationMode=false;
    
    // scoreCoins=0;
    durationExperiment=0;
    
    ofToggleFullscreen();
}



void ofApp::generateObstacles(ofRectangle& mazeRectangle, int _numObstacles, float diameter, float minSeparation) {
    // - mazeRectangle is the interaction space, in meters
    // - _numObstacles is the target number of obstacles
    // - radius is the size of the obstacles
    // - When generating them randomly, we will avoid having them superimposed, at least separated by minSeparation
    obstacles.clear();
    
    int targetNumberObstacles=_numObstacles;
    int numObstacles=0;
    cout << "Generating: " <<targetNumberObstacles << " coins" << endl;
    
    float x, y; // random position of the obstacle.
    while (numObstacles<targetNumberObstacles) {
        x=ofRandom(mazeRectangle.x+diameter/2, mazeRectangle.x+mazeRectangle.width-diameter/2);
        y=ofRandom(mazeRectangle.y+diameter/2, mazeRectangle.y+mazeRectangle.height-diameter/2);
        ofVec2f newPos(x, y);
        // check that no other obstacle is too close to this new position, if there is, choose another number again:
        bool proximityTest=false;
        for (int i=0; i<obstacles.size(); i++) {
            ofVec2f pos(obstacles[i].x, obstacles[i].y);
            if (((newPos-pos).length()<minSeparation)||((cameraPosition-newPos).length()<(diameter/2+closeLimit))) {proximityTest=true; break;}
        }
        
        if (!proximityTest) {
            Obstacle o(newPos, diameter/2);
            obstacles.push_back(o);
            numObstacles++;
        }
    }
   
}

int ofApp::generateObstacles(ofRectangle& mazeRectangle, float density, float radius, float minSeparation) {
    // - mazeRectangle is the interaction space, in meters
    // - Density is the surface density of the obstacles (obstacles/m^2)
    // - radius is the size of the obstacles
    // - When generating them randomly, we will avoid having them superimposed, at least separated by minSeparation
    
    generateObstacles (mazeRectangle, mazeRectangle.getArea()*density,  radius,  minSeparation);
    
    return(mazeRectangle.getArea()*density);
}

void ofApp::loadDrawing(string svgFileName) {
    // This function loads the svg file, and creates the ofMesh labyrinth. Also, it computes the size of the drawing (spaceWidth, spaceHeight), which is important
    // to properly set the drawing inside the viewports and position the camera at init.
    
    ofFloatColor color(0.5,0.5,0.5);
    ofFloatColor colorupper(0.5,0.8,0.5);
    
    // svgReader = ofxSVG();
    ofxSVG svg;
    
    svg.load(svgFileName);
    
    // SIZE of the REAL space in dm (from SVG file: 283.4 pixels -> 1m)
    spaceWidth= CONVERSION_SVG_M * svg.getWidth();// in meters
    spaceHeight= CONVERSION_SVG_M * svg.getHeight();
    cout << "Space size (meters): " << spaceWidth<< "x" << spaceHeight << endl << endl;
    spaceBoundingBox.x=0; spaceBoundingBox.width=spaceWidth;
    spaceBoundingBox.y=0; spaceBoundingBox.height=spaceHeight;
    
    vector<ofPolyline> outlines; // auxiliary variable before resizing:
    for (int i = 0; i < svg.getNumPath(); i++){
        ofPath p = svg.getPathAt(i);
        // svg defaults to non zero winding which doesn't look so good as contours
        p.setPolyWindingMode(OF_POLY_WINDING_ODD);
        vector<ofPolyline>& lines = p.getOutline();
        for(int j=0;j<(int)lines.size();j++){
           // outlines.push_back(lines[j].getResampledBySpacing(1));
              outlines.push_back(lines[j]);
        }
    }
    
    // Resize and save in labyrinth2d (collection of ofPolylines) and an ofMesh (for making 3d walls):
    
    //int lineFactor = 1;
    //Compute the total bounding box of the labyrinth (not normalized yet):
    //(initialize it with the first bounding box of the first polyline, but resized):
    labyrinthBoundingBox.x=outlines[0].getBoundingBox().x;
    labyrinthBoundingBox.y=outlines[0].getBoundingBox().y;
    labyrinthBoundingBox.width=outlines[0].getBoundingBox().width;
    labyrinthBoundingBox.height=outlines[0].getBoundingBox().height;
    
    // resize the coordinates in the polyline vector:
    labyrinth2d.clear();
    for( int i=0; i<outlines.size(); i++ ) {
           ofPolyline pl;
        for( int j=0; j<outlines[i].size()-1; j++ ) {
            pl.addVertex(CONVERSION_SVG_M*ofPoint(outlines[i][j].x,outlines[i][j].y));
            pl.lineTo(CONVERSION_SVG_M*ofPoint(outlines[i][j+1].x,outlines[i][j+1].y));
        }
        labyrinth2d.push_back(pl);
        
        
        // Update the maximal bounding box (not normalized yet):
        // Attention: width and height are not really with and height, but max values for x and y!!:
        ofRectangle currRect;
        currRect.x=outlines[i].getBoundingBox().x;
        currRect.y=outlines[i].getBoundingBox().y;
        currRect.width=outlines[i].getBoundingBox().width;
        currRect.height=outlines[i].getBoundingBox().height;
        
        labyrinthBoundingBox.x = min(labyrinthBoundingBox.x,currRect.x);
        labyrinthBoundingBox.width = max(labyrinthBoundingBox.width,currRect.width);
        labyrinthBoundingBox.y= min(labyrinthBoundingBox.y,currRect.y);
        labyrinthBoundingBox.height = max(labyrinthBoundingBox.height,currRect.height);
        
    }
    
    // Now, resize and save in the ofMesh (note: we don't need a vector of meshes...)
    labyrinth.clear(); // this is an ofMesh
    for( int i=0; i<outlines.size(); i++ ) {
        for( int j=0; j<outlines[i].size()-1; j++ ) {
        
        // ofMesh:
        labyrinth.addVertex(CONVERSION_SVG_M*ofVec3f(outlines[i][j].x,outlines[i][j].y,0));
        
        // The coded colors on the line can be used for e.g.
        // have different functions of the objects, or their height
        // CURRENTLY NOT USED
      //  ofColor faceColor = ((ofxSVGLine*)layer.objects[i])->fillColor;
      //  ofColor outlineColor = ((ofxSVGLine*)layer.objects[i])->strokeColor;
        
        // Create two triangles for each standing wall:
        labyrinth.addColor(color);
        labyrinth.addVertex(CONVERSION_SVG_M*ofVec3f(outlines[i][j].x,outlines[i][j].y,mazeWallHeight/CONVERSION_SVG_M));
        labyrinth.addColor(colorupper);
        labyrinth.addVertex(CONVERSION_SVG_M*ofVec3f(outlines[i][j+1].x,outlines[i][j+1].y,mazeWallHeight/CONVERSION_SVG_M));
      
        labyrinth.addColor(colorupper);
        labyrinth.addVertex(CONVERSION_SVG_M*ofVec3f(outlines[i][j].x,outlines[i][j].y,0));
        labyrinth.addColor(color);
        labyrinth.addVertex(CONVERSION_SVG_M*ofVec3f(outlines[i][j+1].x,outlines[i][j+1].y,mazeWallHeight/CONVERSION_SVG_M));
        labyrinth.addColor(colorupper);
            labyrinth.addVertex(CONVERSION_SVG_M*ofVec3f(outlines[i][j+1].x,outlines[i][j+1].y,0));
        labyrinth.addColor(color);

    }
    }
    
    // Attention: width and height are not really with and height, but max values for x and y!!:
    labyrinthBoundingBox.width-= labyrinthBoundingBox.x;
    labyrinthBoundingBox.height-= labyrinthBoundingBox.y;
    // Convert into meters:
    labyrinthBoundingBox.x *= CONVERSION_SVG_M;
    labyrinthBoundingBox.width *= CONVERSION_SVG_M;
    labyrinthBoundingBox.y *= CONVERSION_SVG_M;
    labyrinthBoundingBox.height *= CONVERSION_SVG_M;
}



void ofApp::resetToInitialPosition() {
    // Initial position as the CENTER of the space, or the entrance of the labyrinth?
    // cameraPosition = ofVec3f(spaceWidth/2, -borderSpace, eyesHeight); // note: we assume the HORIZONTAL PLANE is (X,Y)
    cameraPosition = ofVec3f(spaceWidth/2, spaceHeight/2, eyesHeight);
    oldPos = cameraPosition;
}

void ofApp::pauseBuzzersBeeper() {
    // Turn off buzzers/beepers
    mySerial.writeByte('z');
    mySerial.writeByte('e');
}


void ofApp::exit()
{
    
    // Turn of buzzers/beepers
    pauseBuzzersBeeper();
    
    // Before saving the GUI current settings, let's put "recording" to false again so when it starts next time it does not start recording!
    if (recordingOn) toggleRecording();
    // gui->saveSettings("GUI/guiSettings.xml");
    
    delete gui;
}





void ofApp::setProperPerspective() {
    ratioViewPort=view_fp.width/view_fp.height;//=1.0*screenWidth_gl/screenHeight_gl;
    gluPerspective(fovy, ratioViewPort, distNearZ, distFarZ);//
}


ofVec2f projectVec( const ofVec2f& vec1, const ofVec2f& vec2 ) {
    
    ofVec2f v1_n = vec1;
    ofVec2f v2_n = vec2;
    v1_n.normalize();
    v2_n.normalize();
    
    float cosalpha = v1_n.dot(v2_n);
    
    return vec2.length()*cosalpha*v1_n;
    
}


//ofVec2f normalvec( ofxSVGLine* line, ofVec3f center, bool print = false ) {
//    
//    
//    ofVec2f p2(line->x2,line->y2);
//    ofVec2f p1(line->x1,line->y1);
//    ofVec2f c(center.x,center.y);
//    
//    ofVec2f projVec = projectVec( p2-p1, c-p1 );
//    
//    ofVec2f perpVec = (c-p1) - projVec;
//    
//    if( print ) {
//        std::cout << "p1, p2: " << p1 << "; " << p2 << std::endl;
//        std::cout << "c: " << c << std::endl;
//        std::cout << "projVec " << projVec << std::endl;
//        std::cout << "perpVec " << perpVec << std::endl;
//    }
//    
//    return perpVec;
//}

/*	Takes a line and the position of the player, computes
 the perpendicular vector to the line or, if the position
 is outside the line, the vector to the closest endpoint
 of the line
 */
ofVec4f positionToLine( ofPoint& line0, ofPoint& line1, ofVec3f center, bool print = false ) {
    
    ofVec2f p2(line1.x,line1.y);
    ofVec2f p1(line0.x,line0.y);
    ofVec2f c(center.x,center.y);
    
    ofVec2f v1 = p2-p1;
    ofVec2f v2 = c-p1;
    
    ofVec2f projVec = projectVec(v1, v2);
    
    float alpha = v1.dot(v2);
    
    //std::cout << "p1: " << p1 << ", p2: " << p2 << ", c: " << c << std::endl;
    ofVec2f returnvec = v2-projVec;
    ofVec2f fromvec = p1+projVec;
    if( v1.length() < projVec.length() ) {
        //  std::cout << "to the right\n";
        returnvec = c-p2;
        fromvec = p2;
    } else if( alpha < 0 ) {
        //std::cout << "to the left\n";
        returnvec = c-p1;
        fromvec = p1;
    }
    
    return ofVec4f(returnvec.x, returnvec.y, fromvec.x, fromvec.y);
}



void ofApp::sendRangeValues() {
    
    char valString[4];
    
    for( int i=0; i<buzzerMotorLevels.size(); ++i ) {
        // first check if we are too close:
        if( buzzerRanges[i] <= closeLimit +0.01 ) {
            //cout << "Close: (" << i << ") " << buzzerRanges[i] << endl;
            mySerial.writeBytes((unsigned char*)"255",3);
            mySerial.writeByte(65+i);
        } else {
            //  std::cout << "[" << i << "]: " << (int)buzzerMotorLevels[i] << " -- ";
            sprintf(valString, "%d", buzzerMotorLevels[i]);
            mySerial.writeBytes((unsigned char*)valString, strlen(valString));
            //mySerial.writeByte('0');
            mySerial.writeByte(65+i);
        }
    }
    
    //std::cout << std::endl;
    
}

// This function will compute the value to send to the motor vibrators, depending on all the sensing/feeback modes:
void ofApp::computeMotorLevels() {
    buzzerMotorLevels.clear();
    float range;
    float interval=255.0/numTactileLevels;
    
    if (conversionFunctionMode==LINEAR) {
        
        for( int i=0; i<buzzerRanges.size(); ++i ) {
            //float ofMap(float value, float inputMin, float inputMax, float outputMin, float outputMax, bool clamp}
            range=ofMap(buzzerRanges[i],closeLimit, farLimit, 255, 0, true); // true is for clamping to those levels
            
            // discretize?
            if (conversionLevels==DISCRETE) {
                range=interval*ceil(1.0*range/interval);
            }
            
            if (periodicBuzzerCoin&&obstacleType[i]==1) { // obstacle type 1 is an coin
                int mask=((ofGetElapsedTimeMillis()%1000)<800? 1 : 0); // period is 1 sec, duty cycle is 5% or 10% (OFF)
                // Remember that we send only data for motors every SEND_MOTOR_VALUES_PERIOD millisec!
                range*=mask;
            };
            
            buzzerMotorLevels.push_back((unsigned char)range);
        }
        
    } else { // LOG SCALE:
        
        for( int i=0; i<buzzerRanges.size(); ++i ) {
            //float ofMap(float value, float inputMin, float inputMax, float outputMin, float outputMax, bool clamp}
            range=ofMap(buzzerRanges[i],closeLimit, farLimit, 255, 0, true); // true is for clamping to those levels
            
            // discretize?
            if (conversionLevels==DISCRETE) {
                range=interval*ceil(1.0*range/interval);
            }
            
            buzzerMotorLevels.push_back((unsigned char)range);
        }
        
    };
    
}


//--------------------------------------------------------------
void ofApp::update() {
    
    // this should not be necessary, but in the setup the values are WRONG:
    resizeViewports();
    
    // Get latest data from the microcontroller. This will set the newDataReceived to true ONLY when we receieved ENOUGH data to compute new orientation or position (from joystick)
    processSerial();
    
    // Check for lost connectivity:
    if ((!newDataReceived)&&(timerRestartHanshake-ofGetElapsedTimeMillis()> REQUEST_RESEND_PERIOD)) {
        //cout << "Request data resent because of lost connectivity" << endl;
        mySerial.writeByte('s');
    }
    
    // if there is a communication problem, the values of orientation will be stuck (we don't recompute the rotz matrix): this is ok... but not for the joystick values, because then
    // the avatar may continue to drift. It is better not to keep the old values, but reset the vector to (0,0).
    if(!newDataReceived ) {
        
        //	joystickVector.set(0,0);
        
    } else { // new data received:
        
        // 1) Update camera rotation (otherwise use the old one - this is to avoid having non simultaneous update of magnetometer and accelerometer data)
        ofVec3f east=mag.getCrossed(acc);
        east.normalize();
        acc.normalize();
        north=acc.getCrossed(east); // this is necessary to ensure that we have an orthogonal reference frame (vector is normalized in principle)
        
        rotz = ofMatrix4x4(
                           east.x, north.x, acc.x, 0,
                           east.y, north.y, acc.y, 0,
                           east.z, north.z, acc.z, 0,
                           0, 0, 0, 1
                           );
        
        // 2) Request new data
        mySerial.writeByte('s'); // request new data from the microcontroller
        newDataReceived=false; // it will be equal to true, when I receive data from micro
        timerRestartHanshake=ofGetElapsedTimeMillis(); // Reset the lost connectivity checker timer to current time
    }
    
    
    static ofQuaternion squat;
    if( firstInitializeQuat ) {
        squat = rotz.getInverse().getRotate();
        recordZeroPosition(squat);
        
        if( !isnan(squat.getEuler()[0]) ) {
            firstInitializeQuat = false;
        }
    }
    
    float alpha = 0.9;
    // Compute smoothed raw orientation
    squat.slerp(alpha, rotz.getInverse().getRotate(), squat);
    rotq = squat;
    
    // Get current calibrated orientation (this is a matrix: the *passive* transformation from the initial position of the accelerometer to the current one,
    // meaning that the COLUMNS are the vectors of the current orientation in the initial ("zero") base).
    currot =  inverseZeroRot*rotq;
    
    //Note: currot is the axis passive transformation from the initial position to the current one, i.e., the matrix whose columns are the expression of the current
    // basis vector in the initial basis, or conveserly, the matrix whose ROWS is the initial base vectors in the new one.
    
    // Get heading:
    orientation_h = currot.getInverse().getRowAsVec3f(1); // this is the Y axis of the current accelerometer/magnetometer reference frame in the initial frame (selected by pressing '0').
    heading = atan2(-orientation_h.x, orientation_h.y); // this is the angle between the projection of the Y axis of the acc/mag on the horizontal plane with respect to the Y coordinate of the world system.
    
    
    // Compute head dip angle from direction of top of head:
    // NOTE: the dipAngle is only meaninful using the module data, so we need to compute it here, but we may overwrite the matrices once this is done.
    ofVec3f vertical=currot.getInverse().getRowAsVec3f(2);
    dipAngleVertical=atan2(sqrt(vertical.x*vertical.x+vertical.y*vertical.y), vertical.z);
    dipAngleHeading= atan2(-orientation_h.z, sqrt(orientation_h.x*orientation_h.x+orientation_h.y*orientation_h.y) );
    
    // In case enableHeadCompass is disabled, overwrite the values of mag and acc vector using the current keyboard modified pose matrix:
    // NOTE: this is a hack because we want to compute the dip angle from the accelerometer/magnetometer, EVEN when we don't use heading
    if (!enableHeadCompass) {
        
        headingManual+=1.5*turnSign*PI/180;
        
        mag.set(cos(headingManual+PI/2), sin(headingManual+PI/2), 0);
        acc.set(0,0,1);
        
        // Update camera rotation:
        ofVec3f east=mag.getCrossed(acc);
        east.normalize();
        acc.normalize();
        ofVec3f north=acc.getCrossed(east); // this is necessary to ensure that we have an orthogonal reference frame (vector is normalized in principle)
        
        rotz = ofMatrix4x4(
                           east.x, north.x, acc.x, 0,
                           east.y, north.y, acc.y, 0,
                           east.z, north.z, acc.z, 0,
                           0, 0, 0, 1
                           );
        
        static ofQuaternion squat;
        if( firstInitializeQuat ) {
            squat=rotz.getInverse().getRotate();
            recordZeroPosition(squat);
            
            if( !isnan(squat.getEuler()[0]) ) {
                firstInitializeQuat = false;
            }
        }
        
        rotq=rotz.getInverse().getRotate(); // no smothing for motion by keyboard!
        
        // Get current calibrated orientation (this is a matrix: the *passive* transformation from the initial position of the accelerometer to the current one,
        // meaning that the COLUMNS are the vectors of the current orientation in the initial ("zero") base).
        currot =  rotq;//inverseZeroRot*rotq;
        
        //Note: currot is the axis passive transformation from the initial position to the current one, i.e., the matrix whose columns are the expression of the current
        // basis vector in the initial basis, or conveserly, the matrix whose ROWS is the initial base vectors in the new one.
        
        // Get heading:
        orientation_h = currot.getInverse().getRowAsVec3f(1); // this is the Y axis of the current accelerometer/magnetometer reference frame in the initial frame (selected by pressing '0').
        heading = atan2(-orientation_h.x, orientation_h.y); // this is the angle between the projection of the Y axis of the acc/mag on the horizontal plane with respect to the Y coordinate of the world system.
        
    }
    
    // === UPDATE SPEED AND POSITION ====
    // NOTE: we could also simulate inertia!! to test in future versions.
    
    // NOTE: speed is proportional to the "dip" angle of the head (in any direction), multiplied by a factor equal to maxSpeed/angleMaxDipSpeed.
    // Unit of [maxSpeed/angleMaxDipSpeed]= M/S/DEG. More physically meaningful value can be computed if we know the frame rate or use a timer.
    if (enableDipSpeed) {
        float dipAngle;
        
        // the HEADING direction for the motion (i.e., MOTION HEADING) is equal the angle between the projection of "vertical" onto the "zero" or "initial" orientation of the accelerometer, (with respect to its Y axis):
        if (enableHeadCompass) {
            motionHeading=atan2(-vertical.x, vertical.y);
            dipAngle=180.0/PI*dipAngleVertical;
        } else {
            dipAngle=dipAngleHeading;
            if (dipAngle>=0) motionHeading=heading; else motionHeading=heading+PI;
            dipAngle=180.0/PI*fabs(dipAngle);
        }
        
        
        // The direction of motion is given by the projection of the VERTICAL head orientation onto the original "zero" orientation of the accelerometer:
        // Two ways to get the motion vector:
        motionVector.set(cos(motionHeading+PI/2), sin(motionHeading+PI/2));
        
        // Finally, we need to get the norm of the increment, which is proportional to the head "dipping" angle and maxSpeed/angleMaxDipSpeed:
        if (dipAngle>angleMaxDipSpeed) dipAngle=angleMaxDipSpeed;
        else if (dipAngle<-angleMaxDipSpeed) dipAngle=-angleMaxDipSpeed;
        
        // ALSO: make the speed less important when the tilt of the head is on the sides:
        float dipFactor=dipAngle*(0.7+0.3*cos(motionHeading-heading)*cos(motionHeading-heading)); // this creates two frontal and back "lobes" (like a dipole)
        
        speedNorm=1.0*maxSpeed*dipFactor/angleMaxDipSpeed;
        motionVector=speedNorm*motionVector/ofGetFrameRate(); // dividing by the current frame rate is needed, because the update is done every frame, not every second.
        
        if (fabs(dipAngle)>5) {
            cameraPosition+=motionVector;
        } else // otherwise do nothing (this is for avoiding small "browmnian" like motion, and speeds faster than the current maxSpeed)
            speedNorm=0;
        
        //cout << dipAngle << endl;
        
    } else { // This means we control speed form keyboard AND joystick (I will add their contributions for the time being, for simplicity)
        // NOTE that forward and sideways are given by the orientation vector that may depend on the headband or the keyboard (no joystick control on that)
        
        // 1) from keyboard:
        ofVec2f motionVectorKeyboard=moveSideSign*ofVec2f(orientation_h.y, -orientation_h.x)+moveUpSign*ofVec2f(orientation_h.x, orientation_h.y);
        motionVectorKeyboard.scale(maxSpeed);
        // 2) from joystick:
        // Forward/backward motion:
        ofVec2f motionVectorJoystick=joystickVector.y*maxSpeed*ofVec2f(orientation_h.x, orientation_h.y);
        // Sideways motion:
        if (enableSideWays) motionVectorJoystick+=joystickVector.x*maxSpeed*ofVec2f(orientation_h.y, -orientation_h.x);
        //motionVectorJoystick/=sqrt(2);
        
        // Final motion vector:
        motionVector=motionVectorKeyboard+motionVectorJoystick;
        
        motionHeading=atan2(-motionVector.x, motionVector.y);
        speedNorm=motionVector.length(); // note: it can be 0! (this is not always equal to maxSpeed)
        cameraPosition+=motionVector/ofGetFrameRate();
    }
    
    
    
    // ========= Find small dists and compensate for motion =========
    static bool isBeeping = false;
    shouldBeep=false;
    
    // ==================== COINS ====================
    std::vector<ofVec2f> normals_vec;
    std::vector<ofVec2f> fromvec;
    // Go through all lines in the labyrinth
    tooCloseIdx.clear();
    tooCloseIdxObstacles.clear();
    minWallDistance = 10000; // No wall will be further away than 1^5 meters
    for( int i=0; i<obstacles.size(); ++i ) {
        std::vector<int> tmp;
        bool tooClose = false;
        if( !overrideObstacles && !obstacles[i].eaten ) {
            for( int j=0; j<obstacles[i].object2d.size()-1; ++j ) {
                tmp.push_back(0);
                // Get vector from camera position to closest point on current line
                ofVec4f res = positionToLine(obstacles[i].object2d[j], obstacles[i].object2d[j+1], cameraPosition);
                // Create closest vector to current line
                ofVec2f closestVec(res.x,res.y);
                // Get length of said vector
                float tdist = closestVec.length();
                // Record the min distance
                minWallDistance = min(minWallDistance,tdist);
                // If the distance is too close, add it to lines that are too close
                if( tdist < closeLimit ) {
                    tooClose = true;
                    tmp[j] = 1;
                    //smaldists.push_back(i);
                    // Record the normals of the closest vector to current line
                    normals_vec.push_back(closestVec.normalize());
                    // Records the position from which the closest vector is computed
                    fromvec.push_back(ofVec2f(res.z,res.w));
                }
            }
            if (!overrideObstacles&&tooClose&&!obstacles[i].tooClose) {obstacles[i].wasTouched();}
        }
        if( tooClose ) {
            tooCloseIdxObstacles.push_back( tmp );
        } else {
            tooCloseIdxObstacles.push_back( std::vector<int>() );
        }
    }
    
    if (!overrideObstacles) {
        bool returnToOldpos = false;
        for( int i=0; i<tooCloseIdxObstacles.size(); ++i ) {
            if( tooCloseIdxObstacles[i].size() > 0) {
                returnToOldpos = true;
                break;
            }
        }
        if( returnToOldpos ) {
            // shouldBeep=true; // we won't make it beep for the coins, only walls.
            cameraPosition = oldPos;
        }
    }
    
    // ==================== WALLS collision? ====================
    std::vector<int> smaldists;
	   for( int i=0; i<labyrinth2d.size(); ++i ) {
           // 0 means current line is _not_ too close
           tooCloseIdx.push_back(0);
           // Get vector from camera position to closest point on current line
           ofVec4f res = positionToLine(labyrinth2d[i][0], labyrinth2d[i][1], cameraPosition);
           // Create closest vector to current line
           ofVec2f closestVec(res.x,res.y);
           // Get length of said vector
           float tdist = closestVec.length();
           // Record the min distance
           minWallDistance = min(minWallDistance,tdist);
           // If the distance is too close, add it to lines that are too close
           if( tdist < closeLimit ) {
               tooCloseIdx[i] = 1;
               smaldists.push_back(i);
               // Record the normals of the closest vector to current line
               normals_vec.push_back(closestVec.normalize());
               // Records the position from which the closest vector is computed
               fromvec.push_back(ofVec2f(res.z,res.w));
           }
       }
    
    // std::cout << "min wall: " << minWallDistance << std::endl;
    // If the position is too close, move back to the previously good position
    if (!overrideWalls) {
        if( smaldists.size() >= 1 ) {
            cameraPosition = oldPos;
            shouldBeep=true;// Beep if the object is TOO CLOSE to a WALL (not coin)
        }
    }
    
    
    // Record directions of the buzzer
    buzzerDirs.clear();
    for( int i=0; i<6; ++i ) {
        // NOTE: heading is with respect to the Y axis, so if we want angles with respect to the X axis we need to add PI/2:
        buzzerDirs.push_back( heading+static_cast<float>(i)*PI/3.0f + PI/2);
    }
    
    
    // =========================== Compute distances to respective buzzer
    crossings.clear();
    drawCrossings.clear();
    buzzerRanges.clear();
    obstacleType.clear();
    
    std::vector<int> lineIdx;
    for( int j=0; j<buzzerDirs.size(); ++j ) {
        
        // Direction of
        ofVec2f camdir_2d( cos(buzzerDirs[j]),sin(buzzerDirs[j]) );
        ofVec3f camera_direction(cameraPosition.x+100*cos(buzzerDirs[j]),
                                 cameraPosition.y+100*sin(buzzerDirs[j]),
                                 1);
        ofVec3f camera_center(cameraPosition.x,
                              cameraPosition.y,
                              1);
        ofVec3f camera_line = camera_direction.crossed(camera_center);
        
        
        std::vector<ofVec2f> intersectionsObject;
        if (!overrideObstacles) {
            for( int i=0; i<obstacles.size(); ++i ) {
                if (!obstacles[i].eaten) {
                    ofPolyline& pl = obstacles[i].object2d;
                    obstacles[i].isTouched = false; // is touched is just to change the color of the object for instance
                    for( int j=0; j<pl.size()-1; ++j ) {
                        
                        ofVec3f p1(pl[j].x, pl[j].y,1);
                        ofVec3f p2(pl[j+1].x, pl[j+1].y,1);
                        
                        // Line between the two endpoints (the current line in the maze)
                        ofVec3f line = p1.crossed(p2);
                        
                        // Compute the intersection between the line in the direction of the camera
                        // and the line connecting the two endpoints
                        ofVec3f intersection = camera_line.crossed(line);
                        
                        // Looks like the intersection is too far off
                        if( fabs(intersection.z) < 0.0001 ) {
                            continue;
                        }
                        
                        // Get inhomogeneous intersection
                        ofVec2f point(intersection.x/intersection.z,
                                      intersection.y/intersection.z);
                        
                        // Check if intersection is on the line and on the right side of
                        // the camera center
                        float eps = 1e-3;
                        if( (point.x >= p1.x-eps && point.x <= p2.x+eps) ||
                           (point.x <= p1.x+eps && point.x >= p2.x-eps) ) {
                            if( (point.y >= p1.y-eps && point.y <= p2.y+eps) ||
                               (point.y <= p1.y+eps && point.y >= p2.y-eps) ) {
                                if( camdir_2d.dot(point-ofVec2f(cameraPosition.x,cameraPosition.y)) > 0 ) {
                                    intersectionsObject.push_back(point);
                                    lineIdx.push_back(i);
                                    //obstacles[i].isTouched = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Look for intersections at all lines in the MAZE, not objects:
        std::vector<ofVec2f> intersectionsMaze;
        if (!overrideWalls) {
            for( int i=0; i<labyrinth2d.size(); ++i ) {
                // Vectors to the two endpoints of the line
                ofVec3f p1(labyrinth2d[i][0].x, labyrinth2d[i][0].y,1);
                ofVec3f p2(labyrinth2d[i][1].x, labyrinth2d[i][1].y,1);
                
                // Line between the two endpoints (the current line in the maze)
                ofVec3f line = p1.crossed(p2);
                
                // Compute the intersection between the line in the direction of the camera
                // and the line connecting the two endpoints
                ofVec3f intersection = camera_line.crossed(line);
                
                // Looks like the intersection is too far off
                if( fabs(intersection.z) < 0.0001 ) {
                    continue;
                }
                
                // Get inhomogeneous intersection
                ofVec2f point(intersection.x/intersection.z,
                              intersection.y/intersection.z);
                
                // Check if intersection is on the line and on the right side of
                // the camera center
                float eps = 1e-3;
                if( (point.x >= p1.x-eps && point.x <= p2.x+eps) ||
                   (point.x <= p1.x+eps && point.x >= p2.x-eps) ) {
                    if( (point.y >= p1.y-eps && point.y <= p2.y+eps) ||
                       (point.y <= p1.y+eps && point.y >= p2.y-eps) ) {
                        if( camdir_2d.dot(point-ofVec2f(cameraPosition.x,
                                                        cameraPosition.y)) > 0 ) {
                            intersectionsMaze.push_back(point);
                            lineIdx.push_back(i);
                        }
                    }
                }
            }
        }
        
        
        // No intersections found, so record farLimit+1 to buzzerRanges
        if ((intersectionsObject.size() == 0 ) && (intersectionsMaze.size() == 0 )) {
            crossings.push_back(ofVec2f(-1,-1));
            buzzerRanges.push_back(farLimit+100);
            obstacleType.push_back(-1);
        } else {
            
            float mindistMaze=10000;
            if (intersectionsMaze.size() > 0 ) {
                // Find index of intersection with minimal distance with respecto the MAZE:
                mindistMaze = ofVec2f(cameraPosition.x-intersectionsMaze[0].x,
                                      cameraPosition.y-intersectionsMaze[0].y).length();
                
                int minidx = 0;
                // Check the closest intersection
                for( int k=1; k<intersectionsMaze.size(); ++k ) {
                    float tdist = ofVec2f(cameraPosition.x-intersectionsMaze[k].x,
                                          cameraPosition.y-intersectionsMaze[k].y).length();
                    if( tdist < mindistMaze ) {
                        mindistMaze = tdist;
                        minidx = k;
                    }
                }
            }
            
            float mindistObject=10000;
            if (intersectionsObject.size() > 0 ) {
                // Find index of intersection with minimal distance with respecto the OBJECTS:
                mindistObject = ofVec2f(cameraPosition.x-intersectionsObject[0].x,
                                        cameraPosition.y-intersectionsObject[0].y).length();
                
                int minidx = 0;
                // Check the closest intersection
                for( int k=1; k<intersectionsObject.size(); ++k ) {
                    float tdist = ofVec2f(cameraPosition.x-intersectionsObject[k].x,
                                          cameraPosition.y-intersectionsObject[k].y).length();
                    if( tdist < mindistObject ) {
                        mindistObject = tdist;
                        minidx = k;
                    }
                }
                
            }
            
            // store the smaller:
            if(mindistObject>mindistMaze) {
                if (mindistMaze<farLimit) {
                    buzzerRanges.push_back(mindistMaze);
                    obstacleType.push_back(0); // this means it was a wall
                } else {
                    buzzerRanges.push_back(farLimit+100);
                    obstacleType.push_back(-1); // this means it is out of range
                }
                
            } else {
                if (mindistObject<farLimit) {
                    buzzerRanges.push_back(mindistObject);
                    obstacleType.push_back(1); // this means it was a coin
                } else {
                    buzzerRanges.push_back(farLimit+100);
                    obstacleType.push_back(-1); // this means it is out of range
                }
                
            }
        }
    }
    
    
    
    // Have new cameraPosition, so record this as the old
    oldPos = cameraPosition;
    
    
    
    // ========= Send values to buzzers ==============
    // First, compute values to send to motors (note: the avatar could have moved because of keyboard, or slerp interpolation... even if there is no new data from
    // the microcontroller. So, we need to recompute the motor levels and send the data, at a REASONABLE time interval, that may be independent of the handshake...
    computeMotorLevels();
    
    if (!pauseVibrationMode&&(ofGetElapsedTimeMillis()-lastTimeSendRangesMotors>SEND_MOTOR_VALUES_PERIOD)) {
        sendRangeValues();
        lastTimeSendRangesMotors=ofGetElapsedTimeMillis();
    }
    
    // Beeper?
    beep( shouldBeep );
    
    // Sends handshakes if it seems like the communication is stuck
    //    static int timer = 0;
    //    if( timer++ > 10 ) {
    //        mySerial.writeByte('s');
    //        //        std::cout << "sent handshake again\n";
    //        timer = 0;
    //    }
    
    
    // UPDATE COIN STATES:
    for (int i=0; i<obstacles.size(); i++) obstacles[i].updateEffect();
    
    // update the sound playing system:
    ofSoundUpdate();
    
    // RECORD DATA:
    if ((ofGetElapsedTimeMillis()-lastRecord)>intervalRecording) {
        
        // Recording of the trajectory even when not for saving:
        trajectoryArray.push_back(cameraPosition);
        
        // Record data for file logging:
        if (recordingOn) {
            
            timeStampArray.push_back(ofGetElapsedTimeMillis()-startTimeRecording);
            savedTrajectoryArray.push_back(cameraPosition);
            
            poseArray.push_back(orientation_h); // the 3d pose is the direction of the direction of the head in 3d
            headingArray.push_back(heading);
            minWallDistanceArray.push_back(minWallDistance);
            speedArray.push_back(speedNorm);
            
            lastRecord=ofGetElapsedTimeMillis();
        }
        
        
    }
    
}


void ofApp::beep(bool shouldBeep) {
    
    static bool isBeeping = false;
    
    if( !isBeeping && shouldBeep ) {
        mySerial.writeByte('b');
        isBeeping = true;
    } else if( isBeeping && !shouldBeep ) {
        mySerial.writeByte('e');
        isBeeping = false;
    }
    
}


//--------------------------------------------------------------
void ofApp::drawViewportOutline(const ofRectangle & viewport){
    ofPushStyle();
    ofNoFill();
    ofSetColor(25);
    ofSetLineWidth(1.0f);
    ofRect(viewport);
    ofPopStyle();
}

void ofApp::drawReferenceFrame() {
    
    
    // Note: use text fonts instead of ofDrawBitmapString to have text properly occulded by 3d objects.
    ofVec3f origin(0,0,0);
    ofVec3f xaxis(1,0,0);
    ofVec3f yaxis(0,1,0);
    ofVec3f zaxis(0,0,1);
    
    ofPushMatrix();
    // X axis:
    ofSetColor(255, 0, 0);
    ofLine(origin, xaxis);   //ofDrawBitmapString("X", xaxis);
    ofTranslate(1.1,0, .01);
    ofScale(.01, .01, .01);
    // Text always facing the camera:
    //ofMultMatrix(rotq.inverse());
    franklinBook14.drawString("X", 0,0);
    ofPopMatrix();
    
    ofPushMatrix();
    ofSetColor(0, 255, 0);
    ofLine(origin,yaxis); // ofDrawBitmapString("Y", yaxis);
    ofTranslate(0,1.1,.01);
    ofScale(.01, .01, .01);
    ofRotateX(180);
    franklinBook14.drawString("Y", 0,0);
    ofPopMatrix();
    
    ofPushMatrix();
    ofSetColor(0, 0, 255);
    ofLine(origin,zaxis); // ofDrawBitmapString("Y", yaxis);
    ofTranslate(0,0,1.1);
    ofRotateX(90);
    ofRotateY(180);
    ofScale(.01, .01, .01);
    franklinBook14.drawString("Z", 0,0);
    ofPopMatrix();
    
    
    //      ofDrawAxis(10);
}

//--------------------------------------------------------------
void ofApp::draw(){
    string textString;
    
    
    ofBackground(230, 250, 250);
    
    
    // =============== 3D VIEW =========================
    if(view3D ) {
        ofEnableAlphaBlending();
        ofEnableSmoothing();
        //glEnable(GL_FOG);
        
        glViewport(view_fp.x, view_fp.y, view_fp.width, view_fp.height);
        
        ofPushView();
        // 1) Set the OpenGL projection matrix:
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity(); // reset the projection matrix
        // set the "focal length" of the camera, and the frustum:
        
        gluPerspective(fovy, ratioViewPort, distNearZ, distFarZ);//
        
        
        // 2) Set CAMERA position (using MODELVIEW matrix operated through the gluLookAt function):
        // REM: this is EQUIVALENT TO CHOOSING THE GLOBAL MODELING with function glTranslatef and glRotatef, etc.
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();  //reset modelview matrix
        
        
        // Adjust initial orientation so that the direction "towards the screen" is Y, and up is Z.
        ofRotateX(-90);
        
        // Apply a pre-translation to "see" the avatar:
        glTranslatef(0,outOfBodyDistance,0);
        
        // Head orientation
        ofMultMatrix(rotq);
        
        // For calibration purpose
        if (enableHeadCompass) ofMultMatrix(inverseZeroRot);
        
        // Head position
        glTranslatef(-cameraPosition.x, -cameraPosition.y, -cameraPosition.z);
        
        // DRAW AXIS:
        drawReferenceFrame();
        
        // Draw Floor:
        ofSetColor(200, 200, 200 , 240);
        ofFill();
        ofRect(-100,-100,200,200);
        
        // Draw grid on the floor:
        ofSetColor(200, 0, 100, 100);
        //ofNoFill();
        ofSetLineWidth(2);
        ofPushMatrix();
        ofTranslate(0,0,.01);
        drawCustomGrid(-borderSpace, spaceWidth+borderSpace, .5, -borderSpace, spaceHeight+borderSpace, .5, 0);
        ofPopMatrix();
        
        ofEnableLighting();
        pointLight.enable();
        material.begin();
        
        labyrinth.setMode(OF_PRIMITIVE_TRIANGLES);
        if (!overrideWalls) labyrinth.draw();
        
        material.end();
        ofDisableLighting();
        
        
        ofSetColor(0, 80, 0 , 255);
        ofSetLineWidth(4);
        for( int i=0; i<labyrinth2d.size(); ++i ) {
            // verticals:
            ofLine(labyrinth2d[i][0].x, labyrinth2d[i][0].y, 0, labyrinth2d[i][0].x, labyrinth2d[i][0].y, mazeWallHeight);
            ofLine(labyrinth2d[i][1].x, labyrinth2d[i][1].y, 0, labyrinth2d[i][1].x, labyrinth2d[i][1].y, mazeWallHeight);
            // horizontals:
            ofLine(labyrinth2d[i][0].x, labyrinth2d[i][0].y, 0.015, labyrinth2d[i][1].x, labyrinth2d[i][1].y, .015);
            ofLine(labyrinth2d[i][0].x, labyrinth2d[i][0].y, mazeWallHeight, labyrinth2d[i][1].x, labyrinth2d[i][1].y, mazeWallHeight);
            // Use cylinders?
        }
        
        
        ofSetLineWidth(1);
        // Draw objects (we do this at the end because of alpha channel of coin images)
        for( int i=0; i<obstacles.size(); ++i ) {
            obstacles[i].draw3d(overrideObstacles);
        }
        
        
        //	for( int k=0; k<crossings.size(); ++k ) {
        //		if( crossings[k].x == -1 ) {
        //			continue;
        //		}
        //		ofMesh mesh = sphere(1,ofVec3f(crossings[k].x,crossings[k].y,mazeWallHeight/2));
        //		mesh.draw();
        //	}
        
        //std::cout << std::endl;
        
        
        // Draw trajectory in 3d?
        //if (drawTrajectoryMode) {
        //   drawTrajectory();
        //}
        
        
        ofPopView();
        
    }
    
    // ==== 2D VIEW ====================================================
    
    glViewport(0,0, ofGetWindowWidth(), ofGetWindowHeight());
    
    if( view2D ) {
        ofEnableAlphaBlending();
        ofEnableSmoothing();
        glDisable(GL_FOG);//glEnable(GL_FOG);
        
        drawViewportOutline(view_2d);
        
        //  glViewport(view_2d.x, view_2d.y, view_2d.width, view_2d.height);
        //        glMatrixMode(GL_PROJECTION);
        //        glLoadIdentity();
        //        glOrtho(0, ofGetWidth(), ofGetHeight(), 0, -ofGetHeight(), ofGetHeight());
        //        glMatrixMode(GL_MODELVIEW);
        //        glLoadIdentity();
        //        ofTranslate(0,0,-10);
        
        cam2d.begin(view_2d); // note: this will set the coordinates in the viewport in pixels, as specified in view_2d
        
        // SCALING and TRANSLATING to get the whole drawing inside the current viewport:
        float scaling2D;
        ofPoint translation2D;
        if ((spaceWidth+2*borderSpace)/(spaceHeight+2*borderSpace)>=view_2d.width/view_2d.height) {
            // this means that the drawing is more elongated in the X direction than the viewport:
            // Let's the width of the drawing fit the width of the viewport:
            scaling2D=view_2d.width/(spaceWidth+2*borderSpace);
            translation2D.set(borderSpace*scaling2D,borderSpace*scaling2D+0.5*(view_2d.height-(spaceHeight+2*borderSpace)*scaling2D)); // note: this is in pixels, so translation needs to be done before scaling.
        } else {
            // this means that the drawing is less elongated in the X direction than the viewport:
            // Let's the height of the drawing fit the height of the viewport:
            scaling2D=view_2d.height/(spaceHeight+2*borderSpace);
            translation2D.set(borderSpace*scaling2D+0.5*(view_2d.width-(spaceWidth+2*borderSpace)*scaling2D),borderSpace*scaling2D); // note: this is in pixels, so translation needs to be done before scaling.
        }
        
        ofTranslate(translation2D);
        ofScale(scaling2D, scaling2D);
        
        //ofTranslate(view_2d.width/2,view_2d.height/2);
        //  ofRect(view_2d.width/2,view_2d.height/2,10,10);
        //  ofRect(10,10,view_2d.width-20,view_2d.height-20);
        
        // draw trajectory in 2d:
        if (drawTrajectoryMode) {
            drawTrajectory();
        }
        
        // Draw grid on the floor:
        ofSetColor(200, 100, 0, 40);
        ofNoFill();
        ofSetLineWidth(1);
        ofPushMatrix();
        ofTranslate(0,0,-1);
        drawCustomGrid(-borderSpace, spaceWidth+borderSpace, .5, -borderSpace, spaceHeight+borderSpace, .5, 0);
        ofPopMatrix();
        
        // Draw axis:
        ofSetLineWidth(2);
        drawReferenceFrame() ;
        
        /*for( int i=0; i<obstacles.size()-1; ++i ) {
         ofPolyline& pl = obstacles[i].object2d;
         vector<int> tooClose = tooCloseIdxObstacles[i];
         for( int j=0; j<pl.size(); ++j ) {
         if( !obstacles[i].eaten ) {
         if( tooClose.size() > 0 && tooClose[j] == 1 ) ofSetColor(255, 0, 0);
         else ofSetColor(0, 50, 0);
         pl.draw();
         } else {
         if( tooClose.size() > 0 && tooClose[j] == 1 ) ofSetColor(255, 0, 0, 100);
         else ofSetColor(0, 50, 0, 100);
         pl.draw();
         }
         }
         }*/
        
        
        // Draw the user (i.e., the "camera" position)
        ofSetLineWidth(2);
        ofSetColor(0, 0, 255, 200);
        ofFill();
        ofSetCircleResolution(100);
        ofCircle(cameraPosition.x, cameraPosition.y, radiusHead);
        
        
        // Draw max and min ranges:
        ofNoFill();
        ofSetLineWidth(2);
        ofSetColor(255, 0, 0, 200);
        ofCircle(cameraPosition.x, cameraPosition.y, farLimit);
        ofCircle(cameraPosition.x, cameraPosition.y, closeLimit);
        
        // Draw buzzer ranges:
        ofPushMatrix();
        ofTranslate(cameraPosition.x, cameraPosition.y);
        for( int i=0; i<buzzerRanges.size(); ++i) {
            float range=buzzerRanges[i];
            if (obstacleType[i]==-1) { // this is equivalent to the test : (range<farLimit)
                ofSetColor(255, 0, 0, 60); ofSetLineWidth(1); }
            else if (obstacleType[i]==0) {ofSetColor(255, 0, 0); ofSetLineWidth(2);} // this means a wall
            else if (obstacleType[i]==1) {// this means an coin
                if (periodicBuzzerCoin) {
                    if ((ofGetElapsedTimeMillis()%1000)>950) {
                        ofSetColor(255, 0, 0); ofSetLineWidth(1);
                    } else {
                        ofSetColor(255, 0, 0); ofSetLineWidth(5);
                    }
                } else {ofSetColor(255, 0, 0); ofSetLineWidth(4);}
            }
            // ATTENTION: the buzzerDirs are NOT in "heading" angles (i.e., for the Y axis), but from the X axis. No need to add PI/2
            ofLine(0,0, buzzerRanges[i]*cos(buzzerDirs[i]), buzzerRanges[i]*sin(buzzerDirs[i]));
        }
        ofPopMatrix();
        
        
        // Draw labyrinth
        ofSetLineWidth(2);
        if (!overrideWalls) {
            for( int i=0; i<labyrinth2d.size(); ++i ) {
                if( tooCloseIdx[i] == 0 ) ofSetColor(0, 50, 0);
                else ofSetColor(255, 0, 0);
                labyrinth2d[i].draw();
            }
        } else {
            for( int i=0; i<labyrinth2d.size(); ++i ) {
                if( tooCloseIdx[i] == 0 ) ofSetColor(0, 50, 0, 100);
                else ofSetColor(255, 0, 0, 100);
                labyrinth2d[i].draw();
            }
        }
        
        // Draw objects:
        ofSetLineWidth(1);
        for( int i=0; i<obstacles.size(); ++i ) {
            obstacles[i].draw2d(overrideObstacles);
        }
        
        
        // Draw camera direction and/or field of view:
        ofSetColor(50, 255, 50, 50);
        // NOTE: the camera direction, if computed from the HEADING should be computed as [-sin(heading), cos(heading)] or just add PI/2 (we want the angle from X axis, not Y which is the "north"):
        ofPushMatrix();
        ofTranslate(cameraPosition.x, cameraPosition.y,1); // attention: order of drawing and z coordinate important for alpha blending.
        ofRotateZ(180.0/PI*(heading+PI/2));
        // Field of view:
        float sideTriangle=3;
        float auxY=sideTriangle*sin(PI/180.0*fovy/2);
        float auxX=sideTriangle*cos(PI/180.0*fovy/2);
        glBegin(GL_POLYGON);
        glColor4f(.2, .8, .2, .5); glVertex3f(0,0, .01);
        glColor4f(.9, 1, .9, .01); glVertex3f(auxX, auxY, .01);
        glColor4f(.9, 1, .9, .01); glVertex3f(auxX, -auxY, .01);
        glEnd();
        // Arrow:
        //        ofScale(0.03, 0.03);
        //        ofLine(0,0,8,0);
        //        ofLine(8,0,5,3);
        //        ofLine(8,0,5,-3);
        ofPopMatrix();
        
        // Draw speed magnitude:
        ofSetColor(0, 200, 250, 100);
        ofSetLineWidth(3);
        ofPushMatrix();
        ofTranslate(cameraPosition.x, cameraPosition.y, 1.1);
        ofRotateZ(180.0/PI*(motionHeading+PI/2));
        ofScale(speedNorm, speedNorm);
        ofLine(0,0,1,0);
        ofLine(1,0,.7,.25);
        ofLine(1,0,.7,-.25);
        ofPopMatrix();
        //ofLine(cameraPosition.x,cameraPosition.y, cameraPosition.x+30*motionVector.x,cameraPosition.y+30*motionVector.y);
        
        
        
        // Prints the current (calibrated) rotation on the 2D view
        //		char str[256];
        //		sprintf(str,"orientation 0: %f, %f, %f",currot(0,0),currot(0,1),currot(0,2));
        //		ofDrawBitmapString(str,ofPoint(30,50));
        //		sprintf(str,"orientation 1: %f, %f, %f",currot(1,0),currot(1,1),currot(1,2));
        //		ofDrawBitmapString(str,ofPoint(30,40));
        //		sprintf(str,"orientation 2: %f, %f, %f",currot(2,0),currot(2,1),currot(2,2));
        //		ofDrawBitmapString(str,ofPoint(30,30));
        // sprintf(str,"acc 0: %f, %f, %f",acc.x, acc.y, acc.z);
        //	ofDrawBitmapString(str,ofPoint(30,70));
        
        cam2d.end();
    }
    
    // ==============  DRAW OTHER OVERLAYED ALPHANUMERIC INFO ==============
    
    glViewport(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
    glDisable(GL_FOG);//glEnable(GL_FOG);
    
    // GUI INTERFACE:
    if( gui->isVisible() ) {
        ofSetColor(100,0,0);
        gui->draw();
    }
    
    // Show if recording is ON:
    if (recordingOn) {
        
        ofSetColor(255,0,0);
        textString="SCORE: " + ofToString(Obstacle::scoreCoins) + " BRL"+ "    "+"TIME: " + ofToString(int((ofGetElapsedTimeMillis()-startTimeRecording)/1000)) + " s";
        ofRectangle bounds = franklinBook14.getStringBoundingBox(textString, 0, 0);
        franklinBook14.drawString(textString, ofGetWindowWidth() - bounds.width-150, 60);
        
        int col=255*cos(2*PI*ofGetElapsedTimeMillis()/1000);
        ofSetColor(col,0,0);
        textString="EXPERIMENT RUNNING...:";
        franklinBook14.drawString(textString, ofGetWindowWidth() - bounds.width-150, 30);
        
        
    } else {
        // Show the duration of the last experiment and score:
        ofSetColor(0,0,255);
        textString="SCORE: " + ofToString(Obstacle::scoreCoins) + " BRL"+ "    "+"TIME: " + ofToString(int(1.0*durationExperiment/1000)) + " s";
        ofRectangle bounds = franklinBook14.getStringBoundingBox(textString, 0, 0);
        franklinBook14.drawString(textString, ofGetWindowWidth() - bounds.width-150, 60);
        
        textString="LATEST EXPERIMENT:";
        franklinBook14.drawString(textString, ofGetWindowWidth() - bounds.width-150, 30);
        
    }
    
    //	int textXPos = 30;
    //	if( gui->isVisible() ) {
    //		textXPos += gui->getRect()->getWidth() +  gui->getRect()->getX();
    //	}
    
    int textXPos=ofGetWindowWidth() - 600;
    
    //Name of current file:
    //    ofSetColor(100,0,100);
    //    textString="NAME: "+ svgFileNameNoPath;
    //    franklinBook14.drawString(textString, textXPos, 40);
    //
    //	ofSetColor(0);
    //	//Speed:
    //    textString="SPEED: "+ofToString(speedNorm, 2)+ " m/s";
    //    ofNoFill();
    //    franklinBook14.drawString(textString, textXPos, 80);
    //
    //    //Wall distance:
    //    textString="WALL DISTANCE: "+ofToString(minWallDistance, 2)+ " m";
    //    franklinBook14.drawString(textString, textXPos, 100);
    //
    //    //Position:
    //    textString="POSITION: ("+ofToString(cameraPosition.x, 1)+ ", "+ofToString(cameraPosition.y, 1)+") m";
    //    franklinBook14.drawString(textString, textXPos, 120);
    
    // PAUSE MODE?
    textXPos=100;//ofGetWindowWidth()/2-200;
    float scaleText=1.5;
    if (pauseVibrationMode) {
        ofSetColor(255, 0, 0);
        textString= " BUZZERS IN STAND BY. PRESS [SPACE] TO REACTIVATE ";
        scaleText=2;
    }
    else {
        ofSetColor(0);
        textString=" BUZZERS ACTIVE. PRESS [SPACE] TO DEACTIVATE";
    }
    ofPushMatrix();
    ofTranslate(textXPos, ofGetWindowHeight()-40);
    ofScale(scaleText, scaleText);
    franklinBook14.drawString(textString, 0,0);
    ofPopMatrix();
    
    // OVERRIDE WALL MODE?
    //    if (overrideWalls) {
    //        ofSetColor(0, 0, 100, 200);
    //        textString= "OVERRIDE OBSTACLES MODE:\nPress [ENTER] or START experiment to activate ";
    //        scaleText=1.5;
    //    ofPushMatrix();
    //    ofTranslate(380, 40);// ofGetWindowHeight()-80);
    //    ofScale(scaleText, scaleText);
    //     franklinBook14.drawString(textString, 0,0);
    //    ofPopMatrix();
    //	}
    
    // Frame Rate:
    // textString="FRAME RATE: "+ofToString(ofGetFrameRate())+ " fps";
    // franklinBook14.drawString(textString, textXPos, 70);
    
    //glViewport(view_fp.x, view_fp.height, view_fp.width, 150);
    
    //ofDrawBitmapString("camera controls: click and drag the mouse to look.\nw: forward, s: backwards\na: strafe left\nd: strafe right\ne: boom up\nc: boom down", ofPoint(30, 30));
    
    
}


void ofApp::drawTrajectory() {
    ofPolyline curveTrajectory;
    
    ofPushStyle();
    // ofDisableSmoothing();
    
    // Draw the SAVED trajectory:
    curveTrajectory.clear();
    ofSetColor(0, 0, 250, 180);
    ofSetLineWidth(3);
    // add all the current vertices to cur polyline
    curveTrajectory.addVertices(savedTrajectoryArray);
    curveTrajectory.setClosed(false);
    curveTrajectory.draw();
    
    // Draw all the trajectory:
    ofSetColor(100, 100, 100, 100);
    ofSetLineWidth(1);
    
    // add all the current vertices to cur polyline
    curveTrajectory.clear();
    curveTrajectory.addVertices(trajectoryArray);
    curveTrajectory.setClosed(false);
    curveTrajectory.draw();
    
    
    //poseArray.push_back(orientation_h); // the 3d pose is the direction of the direction of the head in 3d
    //headingArray.push_back(heading);
    //minWallDistanceArray.push_back(1);
    ofPopStyle();
    
}

void ofApp::drawCustomGrid(float x1, float x2, float stepX, float y1, float y2, float stepY, float z) {
    float x=x1, y=y1;
    float lineLengthX=x2-x1;
    glBegin(GL_LINES);
    while (x<=x2+stepX) {
        glVertex3f(x,y1, z);
        glVertex3f(x,y2, z);
        x+=stepX;
    }
    while (y<=y2+stepY) {
        glVertex3f(x1,y, z);
        glVertex3f(x2,y, z);
        y+=stepY;
    }
    glEnd();
}

void ofApp::processSerial()
{
    while (mySerial.available() > 0) {
        char incomingByte = mySerial.readByte();
        
        //std::cout << incomingByte << endl;
        
        // (a) Numerical values:
        // Note: numbers MAY be floats, so we need also the "." and "-"
        if ( (incomingByte >= '0' && incomingByte <= '9')  // numbers
            || (incomingByte >= 'A' && incomingByte <= 'F') // minus sign
            || (incomingByte == '-') // minus sign
            || (incomingByte == '.') // decimal point
            )
        {
            receivedStringData[indexStringData] = incomingByte;
            indexStringData++;
        }
        
        // (b) NUMBER to convert to float and to add to the auxiliary data buffer:
        else if (incomingByte == '#') { // this means that we received a whole number to convert to float
            // (but we don't know yet its meaning)
            receivedStringData[indexStringData] = 0;
            indexStringData = 0;
            float value;
            // convert to float and store in auxiliary "matrix" array:
            //value = (2.f*ofHexToInt(receivedStringData))/(float)(0xFFFF) - 1; // OxFFFF is 65535, so value=0 means received data = 65535/2 = 32767.5... problem!
            // 32737 is 0 from the arduino:
            value = 1.0*(ofHexToInt(receivedStringData)-32767)/65535;
            
            //cout << "val: " << value << endl;
            
            auxDataBuffer[auxDataBuffer_index] = value; // old float representation: atof(receivedStringData);
            auxDataBuffer_index++;
        }
        
        
        // (c) TERMINATOR indicating what to do with the data stored in the auxiliary data buffer:
        else {
            int i;
            ofVec2f joystickVectorNew;
            
            switch(incomingByte) {
                    
                    // =======================  ONLY RELEVANT FOR GAMING COINS MODE ====================
                case 'm': // Magnetometer data (after calibration shift):
                    mag.set(auxDataBuffer[0], auxDataBuffer[1], auxDataBuffer[2]);
                    mag.normalize();
                    
                    //std::cout << "mag: " << mag << std::endl;
                    break;
                    
                case 'a': // Accelerometer data (after calibration shift):
                    acc.set(auxDataBuffer[0], auxDataBuffer[1], auxDataBuffer[2]);
                    acc.normalize();
                    
                    //std::cout << "acc: " << acc << std::endl;
                    break;
                    
                    // Potentiomenter JOYSTICK:
                case 'j':
                    joystickVectorNew.set(2.0*auxDataBuffer[0], 2.0*auxDataBuffer[1]);// the max value is about 1 for each axis!
                    //cout << joystickVectorNew.x << "   "<< joystickVectorNew.y <<endl;
                    // THIS IS A HACK because something is not working wrong...
                    if (joystickVectorNew.x<1)  joystickVectorRaw.x= joystickVectorNew.x;
                    if (joystickVectorNew.y<1)  joystickVectorRaw.y= joystickVectorNew.y;
                    
                    // "processed" joystick vector (can have "inertia", but for the time being we don't do that):
                    joystickVector=joystickVectorRaw-joystickVectorZero; // this is set by default to (0,0), and reset at calibration
                    
                    // IMPORTANT: we assume we receive all data (acceleration, magnetic vector AND JOYSTICK DATA)
                    newDataReceived=true;
                    break;
                    // ===================================================================================
                    
                    // This is not used actually - by the way, my modified library for the magnetometer on the Arduino side is maybe not ready to send proper quaternion data)
                case 'q': // quaternion
                    rot.set(auxDataBuffer[0]/1000, auxDataBuffer[1]/1000, auxDataBuffer[2]/1000, auxDataBuffer[3]/1000);
                    rot.normalize();
                    if (firstTime) {firstTime=false; oldrot=rot;}
                    break;
                    
                case 'e': // Classic Euler angles
                    alpha= auxDataBuffer[0];
                    beta= auxDataBuffer[1];
                    gamma= auxDataBuffer[2];
                    break;
                    
                case 'c': // Cardan (Tait-Brayan Euler angles)
                    heading= auxDataBuffer[0];
                    altitude= auxDataBuffer[1];
                    bank= auxDataBuffer[2];
                    break;
                    
                case 'r': // 3x3 rotation matrix data:
                    for (i=0; i<9; i++) rotMat[i]=auxDataBuffer[i];
                    break;
                    
                default:
                    break;
            }
            auxDataBuffer_index=0;
        }
        
    }
}

void ofApp::resizeViewports() {
    vres=ofGetWindowHeight();
    hres=ofGetWindowWidth();
    
    switch( viewMode ) {
        case 0: // 2d and 3d
            view3D = view2D = true;
            view_fp = ofRectangle(0, 0, hres*2/3, vres);
            view_2d = ofRectangle(hres*2/3,0,hres*1/3,vres);
            setProperPerspective();
            break;
        case 1: // 2d
            view3D = false;
            view2D = true;
            view_2d = ofRectangle(0,0,hres,vres);
            break;
        case 2: // 3d
            view3D = true;
            view2D = false;
            view_fp = ofRectangle(0, 0, hres, vres);
            setProperPerspective();
            break;
    }
    
    
}

void ofApp::toggleViewMode() {
    
    viewMode = (viewMode+1)%3;
    
    // Properly set the button in the GUI:
    std::vector<ofxUIToggle*> toggles = viewModeRadio->getToggles();
    for( int i=0; i<toggles.size(); ++i ) {
        toggles[i]->setValue(i==viewMode);
    }
    
}

void ofApp::toggleOverrideCoins() {
    overrideObstacles=!overrideObstacles;
    // Properly set the button in the GUI (in case value was changed by keyboard):
    // .. to do
}

void ofApp::toggleOverrideWalls() {
    overrideWalls=!overrideWalls;
    // Properly set the button in the GUI (in case value was changed by keyboard):
    // .. to do
}

void ofApp::recordZeroPosition( ofQuaternion& quat ) {
    
    inverseZeroRot = quat.inverse();
    
}


void ofApp::toggleRecording() {
    if (!recordingOn) {
        
        // Prepare for recording:
        recordingOn=true; // recording mode
        
        // Erase all recorded data, if any, and reset score and timer:
        clearRecording();
        Obstacle::scoreCoins=0;
        startExperimentTime=ofGetElapsedTimeMillis();
        
        // Activate walls and coins in case they were overriden?:
        // if (overrideWalls) toggleOverrideWalls();
        // if (overrideObstacles) toggleOverrideCoins();
        
        // show trajectory:
        // if (!drawTrajectoryMode) toggleTrajectoryMode();
        
        // initialize pose matrix from headband ? (or this is done in Calibration?)
        // recordZeroPosition(rotq);
        // firstInitializeQuat=true;
        
        startTimeRecording=ofGetElapsedTimeMillis();
        lastRecord=startTimeRecording;
        
    } else { // this means we are stopping the recording mode (actually the "game" mode):
        recordingOn=false;
        
        durationExperiment=ofGetElapsedTimeMillis()-startExperimentTime;
    }
}

void ofApp::saveRecording() {
    if(logDataFile.open(ofToDataPath(logDataFileName), ofFile::WriteOnly, false)) {
        logDataFile.create();
        // Save data:
        // 1) Header:
        logDataFile << "Duration:\t" << durationExperiment << " (millisec) \n";
        logDataFile << "Score coins:\t" << Obstacle::scoreCoins << "\n";
        
        // 2) Trajectory data:
        for (int i=0; i<savedTrajectoryArray.size(); i++) {
            
            logDataFile << timeStampArray[i] << "\t";
            logDataFile << savedTrajectoryArray[i].x <<"\t"<< savedTrajectoryArray[i].y << "\t";
            logDataFile << headingArray[i] << "\t";
            // logDataFile << poseArray[i].x << "\t"<< poseArray[i].y << "\t"<< poseArray[i].z << "\t";
            logDataFile << minWallDistanceArray[i] << "\t";
            logDataFile << "\n";
        }
        cout << "Data saved into " << logDataFile.path() << " file." << endl;
        logDataFile.close();
    } else {
        cout << "Could not open the file "<< logDataFileName << " to save data!" << endl;
        ofLogError("The file " + logDataFileName + " cannot be opened!");
    }
}

void ofApp::clearRecording() {
    timeStampArray.clear();
    savedTrajectoryArray.clear();
    trajectoryArray.clear();
    headingArray.clear();
    poseArray.clear();
    minWallDistanceArray.clear();
}

void ofApp::toggleTrajectoryMode() {
    drawTrajectoryMode=!drawTrajectoryMode;
    // Properly set the button in the GUI (in case value was changed by keyboard):
    // .. to do
}

void ofApp::toggleSideWaysControl() {
    enableSideWays=!enableSideWays;
    // Properly set the button in the GUI (in case value was changed by keyboard):
    // .. to do
}

void ofApp::toggleHeadCompass() {
    enableHeadCompass=!enableHeadCompass;
    firstInitializeQuat=true;
    // Properly set the button in the GUI (in case value was changed by keyboard):
    // .. to do
    
}

void ofApp::toggleDipSpeed() {
    enableDipSpeed=!enableDipSpeed;
    // Properly set the button in the GUI (in case value was changed by keyboard):
    // .. to do
}

void ofApp::guiEvent(ofxUIEventArgs &e)
{
    string name = e.widget->getName();
    int kind = e.widget->getKind();
    
    
    // Port checking:
    for(int k = 0; k < (int)deviceList.size(); k++){
        // cout << "[" << deviceList[k].getDeviceID() << "] = "<< deviceList[k].getDeviceName().c_str() << endl;
        
        if (name == deviceList[k].getDeviceName() ) {
            ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
            if (tgl->getValue()) {
                
                cout<< " closing old port..." << endl;
                mySerial.close();
                cout<< " Opening new port " << deviceList[k].getDeviceName().c_str() << endl;
                mySerial.setup(deviceList[k].getDeviceName(), 57600);
                mySerial.flush(true, true);
            } else {
                //...
            }
            break;
        }
    }
    
    
    if( name == "Minimize" ) {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() ) {
            gui->setDimensions(gui->getRect()->width, UI_HEIGHT_MINIMIZED);
        } else {
            gui->setDimensions(gui->getRect()->width, UI_HEIGHT_VISIBLE);
        }
    }
    
    if( name == "Calibrate" ) {
        if( e.getButton()->getValue() ) {
            
            // we assume that we received a well formed paquet of orientation data and joystick data at least once..
            
            // initialize pose matrix from headband ? (or this is done in Calibration?)
            recordZeroPosition(rotq);
            firstInitializeQuat=true;
            
            joystickVectorZero=joystickVectorRaw;
            
        }
    }
    
    
    if (name=="Periodic Coin Vibration") {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() ) periodicBuzzerCoin=true; else periodicBuzzerCoin=false;
    }
    
    if( name == "Manual Motion") {
        
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() ) enableDipSpeed=false; else enableDipSpeed=true;
        
        //toggleDipSpeed();
    }
    
    if( name == "Manual Rotation") {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() ) enableHeadCompass =false; else enableHeadCompass=true;
        //toggleHeadCompass();
    }
    
    if (name=="Enable sideways joystick") {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() )  enableSideWays=true; else enableSideWays=false;
        //toggleSideWaysControl();
    }
    
    if( name == "[min-max]" ) {
        ofxUIRangeSlider* sldr = (ofxUIRangeSlider*) e.widget;
        closeLimit = sldr->getScaledValueLow();
        farLimit   = sldr->getScaledValueHigh();
    }
    
    if( name == "Max Speed" ) {
        ofxUISlider* sldr = (ofxUISlider*) e.widget;
        maxSpeed = sldr->getScaledValue();
    }
    
    
    if (name == "Size coin") {
        ofxUINumberDialer* sizecoin = (ofxUINumberDialer*) e.widget;
        sizeObstacle = sizecoin->getValue();
        //cout << sizeObstacle << endl;
        // In case there are already objects in the obstacle list, we can resize the obstacles ALREDY created:
        for (int i=0; i<obstacles.size(); i++) {
            // note: here we re-create the object (createRepresentation clears the vector of vertices/polyline)
            obstacles[i].createRepresentation(obstacles[i].x, obstacles[i].y, sizeObstacle/2); // we pass the radius...
        }
    }
    
    if (name == "Number coins") {
        ofxUINumberDialer* numcoins = (ofxUINumberDialer*) e.widget;
        numberObstacles = int(numcoins->getValue());
        
        generateObstacles(labyrinthBoundingBox, numberObstacles, sizeObstacle, sizeObstacle+0.5);
        
    }
    
    if (name== "Fill with Coins")
    {
       // cout << "here" << endl;
        if( e.getButton()->getValue() ) {
            // generate objects using the current parameters:
            generateObstacles(labyrinthBoundingBox, numberObstacles, sizeObstacle, sizeObstacle+0.5);
            
           // generateObstacles(labyrinthBoundingBox, 5, .2, .2+0.5);
            
            
            // Reset the counters?
            // Obstacle::scoreCoins=0;
        }
    }
    
    if( name == "2D & 3D" ) {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() ) {
            viewMode = -1;
            toggleViewMode();
        }
    }
    if( name == "2D Only" ) {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() ) {
            viewMode = 0;
            toggleViewMode();
        }
    }
    if( name == "3D Only" ) {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() ) {
            viewMode = 1;
            toggleViewMode();
        }
    }
    
    if( name == "Start Experiment") {
        if( experimentButton->getValue() ) {
            experimentButton->setLabelText("Stop");
  //          ofxUILabel* label = experimentButton->getLabel();
            experimentButton->setName("Stop");
   //         label->setName("Stop LABEL");
            toggleRecording();
        }
    }
    
    if( name == "Stop" ) {
        if( experimentButton->getValue() ) {
            experimentButton->setLabelText("Start Experiment");
  //          ofxUILabel* label = experimentButton->getLabel();
            experimentButton->setName("Start Experiment");
 //           label->setName("Start Experiment LABEL");
            toggleRecording();
        }
    }
    
    
    if( name == "Show Trajectory" ) {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() )  drawTrajectoryMode=true; else drawTrajectoryMode=false;
        // toggleTrajectoryMode();
    }
    
    if (name == "Override Walls") {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() )  overrideWalls  =true; else overrideWalls=false;
        
        //toggleOverrideWalls();
    }
    
    if (name == "Override Coins") {
        ofxUIToggle* tgl = (ofxUIToggle*) e.widget;
        if( tgl->getValue() )  overrideObstacles=true; else overrideObstacles=false;
        
        //toggleOverrideCoins();
    }
    
    if( name == "Save Recording" ) {
        if( e.getButton()->getValue() ) {
            // e.getButton()->setValue(0);
            e.getButton()->setState(OFX_UI_STATE_NORMAL);
            ofFileDialogResult result = ofSystemSaveDialog("result.txt", "Save Experiment File");
            if( result.bSuccess ) {
                logDataFileName = result.getPath();
                saveRecording();
            }
        }
    }
    
    if( name == "Load Labyrinth" ) {
        if( e.getButton()->getValue() ) {
            string path = ofToDataPath("labyrinths/");
            ofFileDialogResult result = ofSystemLoadDialog("Open SGV-file");//, false,path);
            if( result.bSuccess ) {
                svgFileName = result.getPath();
                std::cout << "svgFileName: " << svgFileName << std::endl;
                svgFileNameNoPath = result.getName();
                loadDrawing(svgFileName);
                // We also need to reset camera position? (this is optional):
                resetToInitialPosition();
            }
            e.getButton()->setState(OFX_UI_STATE_NORMAL);
        }
    }
    if( name == "Save Settings" ) {
        if( e.getButton()->getValue() ) {
            gui->saveSettings("GUI/guiSettings.xml");
        }
    }
    
    if( name == "Reset Position" ) {
        if( e.getButton()->getValue() ) {
            resetToInitialPosition();
            // Also, clear the trajectory (not the trajectory for saving, this only happens when we start new experiment):
            trajectoryArray.clear();
        }
    }
    
    // note done yet:
    if( name == "Continuous Mode" ) {
        continuousMode = true;
    } else if( name == "Discrete mode" ) {
        continuousMode = false;
    }
    
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    // ATTENTION: the code here is not good: toggling things with the keyboard won't change the marks in the GUI. THIS NEEDS TO BE CORRECTED
    
    // START EXPERIMENT (Before pressing this, ask: "Are you ready?")
    if( key == OF_KEY_RETURN ) {
        
        if ( recordingOn ) { // if it was recording, stop it:
            
            // change menu labels:
            experimentButton->setLabelText("Start Experiment");
      //      ofxUILabel* label = experimentButton->getLabel();
            experimentButton->setName("Start Experiment");
      //      label->setName("Start Experiment LABEL");
            toggleRecording();
            
        } else {
            experimentButton->setLabelText("Stop");
      //      ofxUILabel* label = experimentButton->getLabel();
            experimentButton->setName("Stop");
      //      label->setName("Stop LABEL");
            toggleRecording();
        }
    }
    
    else if( key == OF_KEY_F2 ) {
        recordZeroPosition(rotq);
        firstInitializeQuat=true;
    }
    
    else if (key==OF_KEY_F10 ) {
        toggleHeadCompass();
    }
    else if (key==OF_KEY_F11 ) {
        toggleDipSpeed();
    }// enable/disable motion control by head tilt
    
    
    //else if (key==OF_KEY_F2) saveRecording();
    else if( key ==  OF_KEY_F9) ofToggleFullscreen();
    else if( key == OF_KEY_F8  ) toggleViewMode();
    else if (key==OF_KEY_F7 ) toggleTrajectoryMode();
    // Recording:
    else if (key==OF_KEY_F6 ) toggleRecording();
    else if( key == OF_KEY_F5) gui->setVisible( !gui->isVisible() );
    
    // Control of height: (just for overseeing the labyrinth):
    else if (key=='[') cameraPosition.z+=.05;
    else if (key==']') cameraPosition.z-=.05;
    
    // Field of view:
    else if (key=='@') fovy+=1;
    else if (key==':') fovy-=1;
    
    
    //======= MANUAL MOTION CONTROL =========
    if (!enableDipSpeed) {
        if( key == OF_KEY_UP ) moveUpSign=1;
        else if( key == OF_KEY_DOWN ) moveUpSign=-1;
        else if( key == OF_KEY_LEFT ) moveSideSign=-1;
        else if( key == OF_KEY_RIGHT ) moveSideSign=1;
    }
    
    if(!enableHeadCompass) {
        if (key=='w') turnSign=1;
        else if (key=='q') turnSign=-1;
    }
    
    if( key == ' ' ) { // this is for stopping the vibratiors at any time (for pause)
        pauseVibrationMode=!pauseVibrationMode;
        if (pauseVibrationMode) {pauseBuzzersBeeper(); }
    }
    
    //    if (key==OF_KEY_RETURN) {
    //        toggleOverrideWalls();
    //        toggleOverrideCoins();
    //    }
    
    // if( key == 'p' ) { mySerial.writeByte('p'); }
    // else if( key == 'h' ) { mySerial.writeByte('h'); }
    
    // else if( key == '1' ) { mySerial.writeByte('b'); }
    // else if( key == '2' ) { mySerial.writeByte('e'); }
    
    
}


//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    if (!enableDipSpeed) {
        if( key == OF_KEY_UP ) moveUpSign=0;
        else if( key == OF_KEY_DOWN ) moveUpSign=0;
        else if( key == OF_KEY_LEFT ) moveSideSign=0;
        else if( key == OF_KEY_RIGHT ) moveSideSign=0;
    }
    
    if(!enableHeadCompass) {
        if (key=='w') turnSign=0;
        else if (key=='q') turnSign=0;
    }
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
void ofApp::windowResized(int w, int h){
    
}

// OLD CODE ========

ofMesh sphere( float radius,
              ofVec3f position,
              int res = 12,
              ofPrimitiveMode mode = OF_PRIMITIVE_TRIANGLE_STRIP
              ) {
    
    ofMesh mesh;
    
    float doubleRes = res*2.f;
    float polarInc = PI/(res); // ringAngle
    float azimInc = TWO_PI/(doubleRes); // segAngle //
    
    if(mode != OF_PRIMITIVE_TRIANGLE_STRIP && mode != OF_PRIMITIVE_TRIANGLES) {
        mode = OF_PRIMITIVE_TRIANGLE_STRIP;
    }
    mesh.setMode(mode);
    
    ofVec3f vert;
    ofVec2f tcoord;
    
    for(float i = 0; i < res+1; i++) {
        
        float tr = sin( PI-i * polarInc );
        float ny = cos( PI-i * polarInc );
        
        tcoord.y = i / res;
        
        for(float j = 0; j <= doubleRes; j++) {
            
            float nx = tr * sin(j * azimInc);
            float nz = tr * cos(j * azimInc);
            
            tcoord.x = j / (doubleRes);
            
            vert.set(nx, ny, nz);
            mesh.addNormal(vert);
            vert *= radius;
            mesh.addVertex(position+vert);
            mesh.addColor(ofFloatColor(1.0,0.5,0.5));
            mesh.addTexCoord(tcoord);
        }
    }
    
    int nr = doubleRes+1;
    if(mode == OF_PRIMITIVE_TRIANGLES) {
        
        int index1, index2, index3;
        
        for(float iy = 0; iy < res; iy++) {
            for(float ix = 0; ix < doubleRes; ix++) {
                
                // first tri //
                if(iy > 0) {
                    index1 = (iy+0) * (nr) + (ix+0);
                    index2 = (iy+0) * (nr) + (ix+1);
                    index3 = (iy+1) * (nr) + (ix+0);
                    
                    mesh.addIndex(index1);
                    mesh.addIndex(index3);
                    mesh.addIndex(index2);
                }
                
                if(iy < res-1 ) {
                    // second tri //
                    index1 = (iy+0) * (nr) + (ix+1);
                    index2 = (iy+1) * (nr) + (ix+1);
                    index3 = (iy+1) * (nr) + (ix+0);
                    
                    mesh.addIndex(index1);
                    mesh.addIndex(index3);
                    mesh.addIndex(index2);
                    
                }
            }
        }
        
    } else {
        for(int y = 0; y < res; y++) {
            for(int x = 0; x <= doubleRes; x++) {
                mesh.addIndex( (y)*nr + x );
                mesh.addIndex( (y+1)*nr + x );
            }
        }
    }
    
    
    return mesh;
}
