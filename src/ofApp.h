#ifndef _TEST_APP
#define _TEST_APP


#include "ofMain.h"
#include "ofxGameCameraMod.h"
#include "ofxSVG.h"
//#include "ofxPanel.h"
#include "ofxUI.h"

#include "obstacle.h"


// NOTE: ALL "REAL" DIMENSIONS IN METERS:
// The Illustrator (SVG) file is such that 1cm = 28.34 points, and 1cm will represent 1 meter in "real" space.
#define CONVERSION_SVG_M 0.03528581// = 1.0/28.34

enum rangeToTactileFunction {LINEAR, LOGARITHMIC};
enum  rangeToTactileLevels {DISCRETE, CONTINUOUS};


#define REQUEST_RESEND_PERIOD 500 // time for resend data request in case of lost connectivity (in milliseconds)

#define SEND_MOTOR_VALUES_PERIOD 50 // in ms, meaning 20 times per sec

class ofApp : public ofBaseApp{
    
public:
    void setup();
    void update();
    void draw();
    void exit();
    
    void keyPressed  (int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    
    
    void loadDrawing(string svgFileName);
    
    // generation of the maze (programmatically). It return the total number of obstacles.
    // NOTE: care should be taken when setting the parameters, as the method can never return... perhaps having a timer?
    int generateObstacles(ofRectangle& mazeArea, float density, float radius, float minSeparation);
    void generateObstacles(ofRectangle& mazeRectangle, int _numObstacles, float radius, float minSeparation);
    
    vector <ofSerialDeviceInfo> deviceList ;
    void processSerial();
    void sendRangeValues();
    void beep(bool shouldBeep);
    
    void pauseBuzzersBeeper();
    
    void resetToInitialPosition();
    
    void setProperPerspective();
    void resizeViewports();
    
    void recordZeroPosition(ofQuaternion& quat);
    
    void toggleViewMode();
    void toggleRecording();
    void toggleTrajectoryMode();
    void toggleHeadCompass();
    void toggleDipSpeed();
    void toggleSideWaysControl();
    void toggleOverrideWalls();
    void toggleOverrideCoins();
    
    void saveRecording();
    void clearRecording();
    
    // Custom drawing functions:
    void drawViewportOutline(const ofRectangle & viewport);
    void drawTrajectory();
    void drawCustomGrid(float x1, float x2, float stepX, float y1, float y2, float stepY, float z);
    void drawReferenceFrame();
    
    // User interface
    ofxUICanvas *gui;
    ofxUIRadio *viewModeRadio;
    ofxUILabelButton* experimentButton;
    void guiEvent(ofxUIEventArgs &e);
    
    ofCamera cam2d;
    ofLight pointLight;
    ofMaterial material;
    ofColor materialColor;
    
    // Variables related to the maze
    //ofxSVG svgReader;
    ofMesh labyrinth;
    vector<ofPolyline> labyrinth2d;
    vector<Obstacle> obstacles;
    vector<int> tooCloseIdx;
    vector<vector<int> > tooCloseIdxObstacles;
    std::vector<int> smaldists;
    
    // Factors for resizing the drawing read on svgReader:
    // Note: SVG file record positions and size of document in PIXELS, assuming a resolution of 72 dpi (or 28.34 pixels/cm).
    // We will assume that 1cm (of printed doc) corresponds to 1 meter in the "real" world, so we need to multiply SVG coordinates by CONVERSION_SVG_M=1/28.34
    float spaceWidth, spaceHeight; // in meters
    ofRectangle labyrinthBoundingBox;
    ofRectangle spaceBoundingBox;
    
    // Height of the maze walls:
    float mazeWallHeight; // in meters (can depend on the line color code)
    
    // border around the space for situating the camera at init, and also for being able to "go outside" the drawing and still see the avatar on the screen:
    float borderSpace; // in meters
    
    float ratioViewPort;
    float fovy;
    float distNearZ,distFarZ;
    
    // Units are DECIMETER/DEGREE/FRAME PERIOD. More physically meaningful value can be computed if we know the frame rate or use a timer.
    // To get speedFactor in dec/sec/degree, we can just divide speedFactor by the frameRate (but this is variable).
    ofVec3f cameraPosition, oldPos; // in dm
    float eyesHeight;// the initial height of the eyes (in dm)
    float outOfBodyDistance; // to "see" own's own avatar in 3d
    
    bool enableHeadCompass; // this is to enable/disable heading control using just head motion
    bool enableDipSpeed; // this is to enable/disable the motion controlled by the head tilt
    bool enableSideWays; // this enable/disable the possibility to move sideways using the joystick.
    float speedFactor; // speed is proportional to the "dip" angle of the head (in any direction) by this factor.
    
    
    // for manual control of motion:
    int turnSign;
    int moveUpSign, moveSideSign;
    
    // Pause buzzers/motors:
    bool pauseVibrationMode;
    
    //keyboard control to override walls or obstacles to be able to place the subject at the start:
    bool overrideWalls;
    bool overrideObstacles;
    bool shouldBeep; // when hitting a wall
    
    int numberObstacles; // to be controlled by a slider
    float sizeObstacle; // the diameter of the obstacle (slider)
    
    vector<float> buzzerDirs;
    vector<float> buzzerRanges;
    vector<int> obstacleType;
    
    vector<ofVec2f> crossings;
    vector<ofMesh> drawCrossings;
    
    // ========= Recording data ==========================================
    
    vector<unsigned long long> timeStampArray;
    vector<ofVec3f> trajectoryArray, savedTrajectoryArray;
    vector<ofVec3f> poseArray;; // the 3d pose is the direction of the direction of the sight
    vector<float> speedArray;; // the 3d pose is the direction of the direction of the sight
    vector<float> headingArray;;
    vector<float> minWallDistanceArray;
    float minWallDistance;
    
    // int scoreCoins; // this will be reset each time we START the experiment
    unsigned long long startExperimentTime;
    unsigned long long durationExperiment;
    
    bool recordingOn;
    unsigned long long startTimeRecording; // note: unsigned long long ofGetElapsedTimeMillis()
    unsigned long long intervalRecording;
    unsigned long long lastRecord;
    
    string svgFileName, svgFileNameNoPath;
    ofFile logDataFile;
    string logDataFileName;
    
    // ========= PARAMETERS for SENSING and STIMULATION ===================
    
    // Head size and sensing ranges (all in meters):
    float radiusHead; // = 0.20;
    float closeLimit;// = 0.3f;
    float farLimit;// = 1.8;
    
    // Feedback mode (range to stimulation conversion):
    bool periodicBuzzerCoin; // this is to make the buzzers vibrate periodically in case of coins (to differentiate from walls)
    rangeToTactileFunction conversionFunctionMode;
    rangeToTactileLevels conversionLevels;
    unsigned char numTactileLevels; // in case of continuous, this is just the maximum number of levels achievable by the motors, i.e. 256;
    vector<unsigned char> buzzerMotorLevels; // each value between 0 and 255, and when not in continuous mode, we will probably limit this to 0-9 or even 0-2
    void computeMotorLevels();
    unsigned long long lastTimeSendRangesMotors;
    
    // ========= UI ==========
    //ofxPanel panel;
    ofTrueTypeFont  franklinBook14;
    
    // ===== VIEWS and VIEWPORTS =====
    int hres,vres;
    int viewMode;
    bool view3D;
    bool view2D;
    ofRectangle view_fp;
    ofRectangle view_2d;
    
    // ==== DRAWING MODES ======
    bool drawTrajectoryMode;
    
    // ==== POSE VARIABLES (MATRICES, QUATERNIONS) ==========
    float rotMat[9];
    
    ofMatrix4x4 rotz;
    ofQuaternion rotq;
    ofQuaternion inverseZeroRot;
    
    // Quaternion:
    bool firstTime;
    bool firstInitializeQuat; // this is used for slerp-ing
    ofQuaternion rot, oldrot;
    ofMatrix4x4 currot;
    
    ofVec3f orientation_h;
    
    //Euler angles:
    float alpha, beta, gamma;
    float heading, altitude, bank;
    
    // Manual control:
    float headingManual;
    
    float motionHeading;
    ofVec2f motionVector;
    float speedNorm;
    float maxSpeed;
    
    
    // Control from the Accelerometer/Magnetometer on the headband:
    ofVec3f mag; // magnetic vector in magnetometer/accelerometer reference frame
    ofVec3f acc; // acceleration vector in magnetometer/accelerometer reference frame
    ofVec3f north, east;
    float dipAngleVertical, dipAngleHeading;
    float angleMaxDipSpeed;
    
    // control from the joystick:
    ofVec2f joystickVector;
    ofVec2f joystickVectorZero;
    ofVec2f joystickVectorRaw;
    
    // Vibration motors mode
    bool continuousMode;
    
    // ==== SERIAL COMMUNICATION =====
    ofSerial	mySerial;
    int defaultCOM_PORT_ID;
    
    // Handshake control:
    unsigned long long timerRestartHanshake; // to check for lost connectivity
    bool newDataReceived;
    
    // String to store ALPHANUMERIC DATA (i.e., integers, floating point numbers, unsigned ints, etc represented as DEC):
    // TO DO: use string objects!
    char receivedStringData[24]; // note: an integer is two bytes long, represented with a maximum of 5 digits, but we may send floats or unsigned int...
    int indexStringData; //position of the byte in the string
    
    // An auxiliary buffer to store parameters data, or matrices, etc during serial communication
    float auxDataBuffer[128];
    int auxDataBuffer_index;
    
    //ofImage coinImage;
    
};

#endif
