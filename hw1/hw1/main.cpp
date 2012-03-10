/*****************************************************************************/
/* This is the program skeleton for homework 2 in CS 184 by Ravi Ramamoorthi */
/* Extends HW 1 to deal with shading, more transforms and multiple objects   */
/*****************************************************************************/


#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaders.h"
#include "Transform.h"
#include "QPrint.h"

int amount; // The amount of rotation for each arrow press

vec3 eye; // The (regularly updated) vector coordinates of the eye location 
vec3 up;  // The (regularly updated) vector coordinates of the up location 
vec3 center;
const vec3 eyeinit(0.0,0.0,5.0) ; // Initial eye position, also for resets
const vec3 upinit(0.0,1.0,0.0) ; // Initial up position, also for resets
const vec3 centerinit(0.0,0.0,0.0) ; 
bool useGlu; // Toggle use of "official" opengl/glm transform vs user code
int w, h; // width and height 
float zNear, zFar ;
float fovy ;
GLuint vertexshader, fragmentshader, shaderprogram ; // shaders
static enum {view, translate, scale, zoom} transop ; // which operation to transform by 
float sx, sy ; // the scale in x and y 
float tx, ty ; // the translation in x and y
vec3 resetEye ;
vec3 resetUp ;
vec3 resetCenter ;
float resetFovy ;

// Variables for geometry and transforms
const int tess = 20 ;
float ambientState[] = {0.0, 0.0, 0.0, 1.0} ;
float diffuseState[] = {0.0, 0.0, 0.0, 1.0} ;
float specularState[] = {0.0, 0.0, 0.0, 1.0} ;
float emissionState[] = {0.0, 0.0, 0.0, 1.0} ;
float shininessState[] = {1} ;
std::stack<mat4> transfstack ;
std::vector<std::string> objects ;
std::vector<mat4> objTransforms ;
std::vector<float> objSize ;
std::vector<std::vector<std::vector<float> > > objProps ;

// Variables to set up lighting
GLfloat lightPosition[10][4] ;
GLfloat lightSpecular[10][4] ;
GLfloat lightTransPos[10][4] ;
GLfloat resetLightPos[10][4] ;
GLuint lightposn[10] ;
GLuint lightcolor[10] ;
GLuint lightcount ;
int numLightsOn ;
int selectedLight ;
vec3 lightsUp[10] ;

// Variables to set uniform params for lighting fragment shader 
GLuint islight ;
GLuint ambient ; 
GLuint diffuse ; 
GLuint specular ; 
GLuint emission ;
GLuint shininess ;

// Variables for movement and camera control
const GLuint GLUT_SCROLL_UP = 3 ;
const GLuint GLUT_SCROLL_DOWN = 4 ;
int mouseoldx, mouseoldy ;
const vec3 worldUp = vec3(0.0, 0.0, 1.0) ;
vec3 forward_dir ;
vec3 move_center ;
vec3 forward_velocity ;
vec3 side_velocity ;
vec3 up_velocity ;
float fb_speed ;
float lr_speed ;
float turn_speed ;
const float viscosity = 50.0f ; // Constant scale factor for movement
const float move_amount = 0.6f / viscosity ;
const float turn_amount = -10.0f / viscosity ; // Negative due to the way rotation works
const float jump_amount = 2.0f / viscosity ;
const float g_accel = -.025f / viscosity ;
const float camera_turn_amount = .1f ;
const float zoom_amount = .03f ;
bool align_flag ;
bool aligned ;
bool viewing ;
bool jumping ;
bool key_press_arr[6] ;
bool alt_controls ;

// New helper transformation function to transform vector by modelview 
// May be better done using newer glm functionality.
void transformvec (const GLfloat input[4], GLfloat output[4]) {
  GLfloat modelview[16] ; // in column major order
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview) ; 
  
  for (int i = 0 ; i < 4 ; i++) {
    output[i] = 0 ; 
    for (int j = 0 ; j < 4 ; j++) 
      output[i] += modelview[4*j+i] * input[j] ; 
  }
}

// Uses the Projection matrices (technically deprecated) to set perspective 
// We could also do this in a more modern fashion with glm.  
void reshape(int width, int height){
	w = width;
	h = height;
        mat4 mv ; // just like for lookat

	glMatrixMode(GL_PROJECTION);
        float aspect = (float) w / (float) h ;
        // I am changing the projection stuff to be consistent with lookat
        if (useGlu) mv = glm::perspective(fovy,aspect,zNear,zFar) ; 
        else {
          mv = Transform::perspective(fovy,aspect,zNear,zFar) ; 
          mv = glm::transpose(mv) ; // accounting for row major 
        }
        glLoadMatrixf(&mv[0][0]) ; 

	glViewport(0, 0, w, h);
}

void printHelp() {
  std::cout << "\npress 'h' to print this message again.\n" 
       << "press '+' or '-' to change the amount of rotation that\noccurs with each arrow press.\n" 
            << "press 'g' to switch between using glm::lookAt and glm::Perspective or your own LookAt.\n"       
            << "press 'r' to reset the transformations.\n"
            << "press 'v' 't' 's' to do view [default], translate, scale.\n"
			<< "press '0' '1' ... '9' to select lights for rotation about the scene.\n" 
            << "press ESC to quit.\n" ;  
    
}

vec3 getLightVec(int lightNum) {
	vec3 lightAsVec = vec3(lightPosition[lightNum][0], lightPosition[lightNum][1], lightPosition[lightNum][2]);
	return lightAsVec ;
}

void changeLightPos(int lightNum, vec3 newPos) {
	lightPosition[lightNum][0] = newPos.x ;
	lightPosition[lightNum][1] = newPos.y ;
	lightPosition[lightNum][2] = newPos.z ;
}

void resetAll() {
	eye = resetEye ; 
	up = resetUp ; 
	center = resetCenter ;
	fovy = resetFovy ;
    sx = sy = 1.0 ; 
    tx = ty = 0.0 ;
	useGlu = false;
	zNear = 0.1 ;
	zFar = 99.0 ;
	for (int i = 0; i < numLightsOn; i++) {
		for (int j = 0; j < 3; j++) {
			lightPosition[i][j] = resetLightPos[i][j] ;
		}
	}
}

void resetCamera() {
	eye = resetEye ;
	center = resetCenter ;
	up = resetUp ;
}

void saveResets() {
	resetEye = eye ;
	resetCenter = center ;
	resetUp = up ;
}

void keyboard(unsigned char key, int x, int y) {
	switch(key) {
	case '+':
		amount++;
		std::cout << "amount set to " << amount << "\n" ;
		break;
	case '-':
		amount--;
		std::cout << "amount set to " << amount << "\n" ; 
		break;
	case 'g':
		useGlu = !useGlu;
        reshape(w,h) ; 
		std::cout << "Using glm::LookAt and glm::Perspective set to: " << (useGlu ? " true " : " false ") << "\n" ; 
		break;
	case 'h':
		printHelp();
		break;
    case 27:  // Escape to quit
        exit(0) ;
        break ;
    case 'r': // reset eye and up vectors, scale and translate. 
		resetAll() ;
		break ;   
    case 'v': 
		selectedLight = -1 ;
        transop = view ;
        std::cout << "Operation is set to View\n" ; 
        break ; 
    case 't':
        transop = translate ; 
        std::cout << "Operation is set to Translate\n" ; 
        break ; 
    case 's':
        transop = scale ; 
        std::cout << "Operation is set to Scale\n" ; 
		break ;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		if(key - 48 >= numLightsOn){
				std::cout << "There are only " << numLightsOn << " lights; please select again.\n";
		} else {
				transop = view;

				selectedLight = key - 48;
				vec3 lightEye = getLightVec(selectedLight) ;
				vec3 wVec = glm::normalize(lightEye - center);
				vec3 uVec = glm::normalize(glm::cross(up,wVec));
				vec3 vVec = glm::cross(wVec,uVec);
				lightsUp[selectedLight] = vVec;

				std::cout << "Light " << selectedLight << " selected for rotation\n";
		}
	}
	glutPostRedisplay();
}

/**
 * Recursive timer function used for alignment of center to move_center (camera to forward).
 * Takes and passes decreasing VALUE to determine number of calls.
 */
void alignTimer(int value) {
	glutPostRedisplay() ;
	if (value > 0 && !viewing) { // Make sure the mouse is not in-use
		Transform::centeralign(eye, move_center, center, up, value) ;
		glutTimerFunc(1, alignTimer, --value) ; // Decrement and call recursively
	} else if (viewing) {
		align_flag = true ; // If stopped in the middle of aligning, switch flag
	}
	if (value == 0) { // Completed alignment
		center = move_center ;
		up = worldUp ;
		aligned = true ;
	}
}


/**
 * Set and get key statuses.
 * Accesses key_press_arr according to CMD at location KEY. If setting, uses NEW_VAL.
 */
bool scanKeys(std::string cmd, unsigned char key, bool new_val) {
	int index = -1 ;
	if (key == 'q') index = 0 ;
	else if (key == 'w') index = 1 ;
	else if (key == 'e') index = 2 ;
	else if (key == 'a') index = 3 ;
	else if (key == 's') index = 4 ;
	else if (key == 'd') index = 5 ;

	if (!cmd.compare("get")) return key_press_arr[index] ;
	else if (!cmd.compare("set")) key_press_arr[index] = new_val ;

	return false ;
}

/**
 *  First-person keyboard controls. 
 *  Changes camera movement according to KEY. Ignores mouse X and Y.
 *  Functionality:
 *  w: Move forward
 *  s: Move backward
 *  a: Turn counterclockwise
 *  d: Turn clockwise
 *  q: Move left
 *  e: Move right
 *  z: Activate zoom
 */
void altKeyboard(unsigned char key, int x, int y){
	if (transop == zoom) {
		switch(key) {
		case 'v':
			transop = view ;
			resetCamera() ;
			std::cout << "Operation is set to View\n" ;
			break ;
		}
	} else if (transop == view) {
		if (key != 'z') {
			vec3 move_vec = move_center - eye ;
			vec3 cross_vec = glm::cross(move_vec, worldUp) ;
			vec3 forward_dir = glm::normalize(vec3(move_vec[0], move_vec[1], 0.0)) ;
			vec3 side_dir = glm::normalize(vec3(cross_vec[0], cross_vec[1], 0.0)) ; // Points right of view
	
			vec3 eye_from_m = move_center - eye ;
			vec3 eye_from_c = center - eye ;
			float dot_p = glm::dot(eye_from_m, eye_from_c) / (QPrint::magv(eye_from_m) * QPrint::magv(eye_from_c)) ;

			// Animate the camera
			int steps = 50 ; // Increase for smoother alignment
			if (dot_p < 1.0 && align_flag && center != move_center) { // Last condition avoids weird precision case
				glutTimerFunc(0, alignTimer, steps); // Begin timer
				align_flag = false ; // Avoid calling overlapping timers
				aligned = false ; // Stop movement while aligning
			}
		}

		switch(key){
		case 'w':
			fb_speed = move_amount ;
			break ;
		case 's':
			fb_speed = -move_amount ;
			break ;
		case 'a':
			turn_speed = -turn_amount ;
			break ;
		case 'd':
			turn_speed = turn_amount ;
			break;
		case 'q':
			lr_speed = -move_amount ;
			break;
		case 'e':
			lr_speed = move_amount ;
			break;
		case 32: // Space
			if (!jumping && aligned) {
				up_velocity = jump_amount * worldUp ;
				jumping = true ;
			}
			break ;
		case 'z':
			transop = zoom ;
			saveResets() ; // Maintain "true" position during zooming
			std::cout << "Operation is set to Zoom\n" ;
			break ;
		}
		if (key != 'z' && key != 32) {
			scanKeys("set", key, true) ; // Mark the key as active
		}
	}
	
	glutPostRedisplay();
}

/**
 *  Keyboard release controls.
 *  Changes camera movement according to KEY. Ignores mouse X and Y.
 *  Handles opposing movement cases.
*/
void keyboardUp(unsigned char key, int x, int y) {
	vec3 move_vec = move_center - eye ;
	vec3 cross_vec = glm::cross(move_vec, worldUp) ;
	vec3 forward_dir = glm::normalize(vec3(move_vec[0], move_vec[1], 0.0)) ;
	vec3 side_dir = glm::normalize(vec3(cross_vec[0], cross_vec[1], 0.0)) ; // Points right of view

	// If the counterpart key is active, switch over to that key's function
	switch(key) {
	case 'w':
		// forward_velocity = (scanKeys("get", 's', true)) ? -move_amount * forward_dir : vec3(0.0) ;
		fb_speed = (scanKeys("get", 's', true)) ? -move_amount : 0 ;
		break ;
	case 's':
		// forward_velocity = (scanKeys("get", 'w', true)) ? move_amount * forward_dir : vec3(0.0) ;
		fb_speed = (scanKeys("get", 'w', true)) ? move_amount : 0 ;
		break ;
	case 'q': 
		// side_velocity = (scanKeys("get", 'e', true)) ? move_amount * side_dir : vec3(0.0) ;
		lr_speed = (scanKeys("get", 'e', true)) ? move_amount : 0 ;
		break ;
	case 'e':
		// side_velocity = (scanKeys("get", 'q', true)) ? -move_amount * side_dir : vec3(0.0) ;
		lr_speed = (scanKeys("get", 'q', true)) ? -move_amount : 0 ;
		break ;
	case 'a':
		turn_speed = (scanKeys("get", 'd', true)) ? turn_amount : 0 ;
		break ;
	case 'd':
		turn_speed = (scanKeys("get", 'a', true)) ? -turn_amount : 0 ;
		break ;
	}
	scanKeys("set", key, false) ; // Mark the key inactive
}

/**
 *  Mouse click function.
 *  Registers the STATE of a BUTTON press at X and Y.
 */
void mouse(int button, int state, int x, int y) {
	if (state == GLUT_DOWN) {
		if (button == GLUT_LEFT_BUTTON) {
			mouseoldx = x ;
			mouseoldy = y ;
			viewing = true ; // Stop pending alignment
			align_flag = true ; // Allow alignment
		} else if (button == GLUT_RIGHT_BUTTON) {
			resetCamera() ;
			glutPostRedisplay() ;
		}
	} else if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
		viewing = false ;
	}
}

/**
 *  Mouse drag function.
 *  Takes current mouse position X and Y.
 */
void mouseDrag(int x, int y) {
	int xloc = x - mouseoldx ;
	int yloc = y - mouseoldy ;

	if (!jumping) { // Disable camera while jumping
		if (transop == view) {
			Transform::centerleft(camera_turn_amount * xloc, eye, center, up) ; // Rotate center and up about the z-axis
			Transform::centerup(-camera_turn_amount * yloc, eye, center, up) ; // Rotate center and up about the perpendicular axis
		} else if (transop == zoom) {
			Transform::zoom(zoom_amount * yloc, eye, center) ;
		}
	}
	
	mouseoldy = y ;
	mouseoldx = x ;

	glutPostRedisplay() ;
}

//  You will need to enter code for the arrow keys 
//  When an arrow key is pressed, it will call your transform functions
void specialKey(int key, int x, int y) {
	switch(key) {
	case 100: //left
          if (transop == view) {
			  if (selectedLight == -1) {
				  Transform::left(amount, eye,  up);
			  } else {
				  vec3 lightEye = getLightVec(selectedLight) ;
				  Transform::left(-amount, lightEye, lightsUp[selectedLight]) ;
				  changeLightPos(selectedLight, lightEye) ;
			  }
		  }
          else if (transop == scale) sx -= amount * 0.01 ; 
          else if (transop == translate) tx -= amount * 0.01 ; 
          break;
	case 101: //up
          if (transop == view) {
			  if (selectedLight == -1) {
				  Transform::up(amount, eye, center, up);
			  } else {
				  vec3 lightEye = getLightVec(selectedLight) ;
				  vec3 lightUp = lightsUp[selectedLight] ;
				  Transform::up(amount, lightEye, center, lightUp) ;
				  lightsUp[selectedLight] = lightUp ;
				  changeLightPos(selectedLight, lightEye) ;
			  }
		  }
          else if (transop == scale) sy += amount * 0.01 ; 
          else if (transop == translate) ty += amount * 0.01 ; 
          break;
	case 102: //right
          if (transop == view) {
			  if (selectedLight == -1) {
				  Transform::left(-amount, eye,  up);
			  } else {
				  vec3 lightEye = getLightVec(selectedLight) ;
				  Transform::left(amount, lightEye, lightsUp[selectedLight]) ;
				  changeLightPos(selectedLight, lightEye) ;
			  }
		  }
          else if (transop == scale) sx += amount * 0.01 ; 
          else if (transop == translate) tx += amount * 0.01 ; 
          break;
	case 103: //down
          if (transop == view) {
			  if (selectedLight == -1) {
				  Transform::up(-amount,  eye, center,  up);
			  } else {
				  vec3 lightEye = getLightVec(selectedLight) ;
				  vec3 lightUp = lightsUp[selectedLight] ;
				  Transform::up(-amount, lightEye, center, lightUp) ;
				  lightsUp[selectedLight] = lightUp ;
				  changeLightPos(selectedLight, lightEye) ;
			  }
		  }
          else if (transop == scale) sy -= amount * 0.01 ; 
          else if (transop == translate) ty -= amount * 0.01 ; 
          break;
	}
	glutPostRedisplay();
}

void init() {
  
  // Set up initial position for eye, up and amount
  // As well as booleans 

	eye = eyeinit ; 
	up = upinit ; 
	center = centerinit ;
	amount = 5;
    sx = sy = 1.0 ; 
    tx = ty = 0.0 ;
	useGlu = false;
	zNear = 0.1 ;
	zFar = 99.0 ;
	fovy = 90.0 ;
	numLightsOn = 0 ;
	selectedLight = -1 ;
	
	forward_velocity = side_velocity = up_velocity = vec3(0.0) ;
	fb_speed = 0 ;
	lr_speed = 0 ;
	turn_speed = 0 ;
	alt_controls = true ;
	move_center = vec3(0.0, 2.0, 1.0) ;
	align_flag = true ;
	aligned = true ; // Make sure this is true 
	viewing = false ;
	jumping = false ;

	// Initialize the stack
	transfstack.push(mat4(1.0)) ;

	glEnable(GL_DEPTH_TEST);

  // The lighting is enabled using the same framework as in mytest 3 
  // Except that we use two point lights
  // For now, lights and materials are set in display.  Will move to init 
  // later, per update lights

    vertexshader = initshaders(GL_VERTEX_SHADER, "shaders/light.vert.glsl") ;
    fragmentshader = initshaders(GL_FRAGMENT_SHADER, "shaders/light.frag.glsl") ;
    shaderprogram = initprogram(vertexshader, fragmentshader) ; 
    islight = glGetUniformLocation(shaderprogram,"islight") ;        
	
	char name[20] ;
	for (int i = 0; i < 10; i++) {
		sprintf_s(name, "lightposn[%d]", i) ;
		lightposn[i] = glGetUniformLocation(shaderprogram, name) ;
		sprintf_s(name, "lightcolor[%d]", i) ;
		lightcolor[i] = glGetUniformLocation(shaderprogram, name) ;
	}
	lightcount = glGetUniformLocation(shaderprogram,"lightcount") ;

	ambient = glGetUniformLocation(shaderprogram,"ambient") ;           
	diffuse = glGetUniformLocation(shaderprogram,"diffuse") ;          
	specular = glGetUniformLocation(shaderprogram,"specular") ; 
	emission = glGetUniformLocation(shaderprogram,"emission") ; 
	shininess = glGetUniformLocation(shaderprogram,"shininess") ;       
}

void display() {
	glClearColor(0, 0, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glMatrixMode(GL_MODELVIEW);
	mat4 mv ;

	// Move smoothly if not in the process of alignment
	if (aligned && transop == view) {
		vec3 move_vec = move_center - eye ;
		vec3 cross_vec = glm::cross(move_vec, worldUp) ;
		vec3 forward_dir = glm::normalize(vec3(move_vec[0], move_vec[1], 0.0)) ;
		vec3 side_dir = glm::normalize(vec3(cross_vec[0], cross_vec[1], 0.0)) ; // Points right of view

		forward_velocity = fb_speed * forward_dir ;
		side_velocity = lr_speed * side_dir ;
		// TODO: Allow constant updating of forward direction

		// Forward/backward
		eye += forward_velocity ;
		move_center += forward_velocity ;

		// Side
		eye += side_velocity ;
		move_center += side_velocity ;

		// Turn
		Transform::moveleft(turn_speed, eye, move_center) ;

		// Jump
		eye += up_velocity ;
		move_center += up_velocity ;

		// Apply gravity
		if (jumping) up_velocity += g_accel * worldUp ;

		// Stop upon landing
		if (eye[2] < 1.0 || move_center[2] < 1.0) {
			eye[2] = move_center[2] = center[2] = 1.0 ;
			up_velocity[2] = 0.0 ;
			jumping = false ;
		} 

		// Update if moving
		if (forward_velocity != vec3(0.0) || side_velocity != vec3(0.0) || turn_speed != 0 || up_velocity != vec3(0.0)) {
			center = move_center ;
			up = worldUp ;
		}
	}

    if (useGlu) mv = glm::lookAt(eye,center,up) ; 
	else {
          mv = Transform::lookAt(eye,center,up) ; 
          mv = glm::transpose(mv) ; // accounting for row major
        }
    glLoadMatrixf(&mv[0][0]) ; 

        // Set Light and Material properties for the teapot
        // Lights are transformed by current modelview matrix. 
        // The shader can't do this globally. 
        // So we need to do so manually. 

		glUniform1i(lightcount, numLightsOn) ;
		for (int i = 0; i < numLightsOn; i++) {
			transformvec(lightPosition[i], lightTransPos[i]) ;
			glUniform4fv(lightposn[i], 1, lightTransPos[i]) ;
			glUniform4fv(lightcolor[i], 1, lightSpecular[i]) ;
		}

		mat4 sc(1.0), tr(1.0) ;
		sc = Transform::scale(sx,sy,1.0) ; 
		tr = Transform::translate(tx,ty,0.0) ;
		// Multiply the matrices, accounting for OpenGL and GLM.
		sc = glm::transpose(sc) ; tr = glm::transpose(tr) ;
		for (unsigned int i = 0; i < objects.size() ; i++) {
			mat4 objMat = glm::transpose(objTransforms[i]) ;
			mat4 transf = mv * tr * sc * objMat ;
			glLoadMatrixf(&transf[0][0]) ; 

			// Get material properties
			float tempAmbient[4], tempDiffuse[4], tempSpecular[4], tempEmission[4], tempShininess[1] ;
			for (int j = 0; j < 4; j++) {
				tempAmbient[j] = objProps[i][0][j] ;
				tempDiffuse[j] = objProps[i][1][j] ;
				tempSpecular[j] = objProps[i][2][j] ;
				tempEmission[j] = objProps[i][3][j] ;
			}
			tempShininess[0] = objProps[i][4][0] ;

			glUniform4fv(ambient,1,tempAmbient) ; 
			glUniform4fv(diffuse,1,tempDiffuse) ; 
			glUniform4fv(specular,1,tempSpecular) ; 
			glUniform4fv(emission,1,tempEmission) ;
			glUniform1fv(shininess,1,tempShininess) ; 
			glUniform1i(islight,true) ;
			
			float size = objSize[i] ;

			if (!objects[i].compare("teapot")) {
				glutSolidTeapot(size);
			} else if (!objects[i].compare("sphere")) {
				glutSolidSphere(size, tess, tess) ;
			} else if (!objects[i].compare("cube")) {
				glutSolidCube(size) ;
			}
		}

	glutSwapBuffers();
}

void stopInvalid() {
	std::cerr << "Invalid command and/or number of parameters" << std::endl ;
	throw 2 ;
}

void alterStack(mat4 M) {
	mat4 & T = transfstack.top() ;
	T = M * T ;
}

void saveProps() {
	std::vector<std::vector<float> > props ;
	std::vector<float> temp1, temp2, temp3, temp4, temp5 ;
	for (int i = 0; i < 4; i++) {
		temp1.push_back(ambientState[i]) ;
		temp2.push_back(diffuseState[i]) ;
		temp3.push_back(specularState[i]) ;
		temp4.push_back(emissionState[i]) ;
	}
	temp5.push_back(shininessState[0]) ;

	props.push_back(temp1) ;
	props.push_back(temp2) ;
	props.push_back(temp3) ;
	props.push_back(temp4) ;
	props.push_back(temp5) ;

	objProps.push_back(props) ;
}

// Change the scene or initialize objects depending on command
void execute(std::string cmd, std::vector<std::string> params) {
	int numParams = params.size() ;
	if (!cmd.compare("size")) {
		if (numParams != 2) stopInvalid();
		w = atof(params[0].c_str()) ;
		h = atof(params[1].c_str()) ;
	} else if (!cmd.compare("camera")) {
		if (numParams != 10) stopInvalid();
		float cam[10] ;
		for (int i = 0; i < 10; i++) cam[i] = atof(params[i].c_str()) ;
		eye = vec3(cam[0], cam[1], cam[2]) ;
		center = vec3(cam[3], cam[4], cam[5]) ;
		up = glm::normalize(vec3(cam[6], cam[7], cam[8])) ;
		fovy = cam[9] ;
		resetEye = eye ;
		resetCenter = center ;
		resetUp = up ;
		resetFovy = fovy ;
	} else if (!cmd.compare("light")) {
		if (numParams != 8) stopInvalid();
		for (int i = 0; i < 4; i++) {
			lightPosition[numLightsOn][i] = atof(params[i].c_str()) ;
			resetLightPos[numLightsOn][i] = atof(params[i].c_str()) ;
			lightSpecular[numLightsOn][i] = atof(params[i + 4].c_str()) ;
		}
		numLightsOn++;
	} else if (!cmd.compare("ambient")) {
		if (numParams != 4) stopInvalid();
		for (int i = 0; i < 4; i++) ambientState[i] = atof(params[i].c_str()) ;
	} else if (!cmd.compare("diffuse")) {
		if (numParams != 4) stopInvalid();
		for (int i = 0; i < 4; i++) diffuseState[i] = atof(params[i].c_str()) ;
	} else if (!cmd.compare("specular")) {
		if (numParams != 4) stopInvalid();
		for (int i = 0; i < 4; i++) specularState[i] = atof(params[i].c_str()) ;
	} else if (!cmd.compare("emission")) {
		if (numParams != 4) stopInvalid();
		for (int i = 0; i < 4; i++) emissionState[i] = atof(params[i].c_str()) ;
	} else if (!cmd.compare("shininess")) {
		if (numParams != 1) stopInvalid();
		shininessState[0] = atof(params[0].c_str()) ;
	} else if (!cmd.compare("teapot")) {
		if (numParams != 1) stopInvalid();
		float size = atof(params[0].c_str()) ;
		objects.push_back("teapot") ;
		objTransforms.push_back(transfstack.top()) ;
		objSize.push_back(size) ;
		saveProps() ;
	} else if (!cmd.compare("sphere")) {
		if (numParams != 1) stopInvalid();
		float radius = atof(params[0].c_str()) ;
		objects.push_back("sphere") ;
		objTransforms.push_back(transfstack.top()) ;
		objSize.push_back(radius) ;
		saveProps() ;
	} else if (!cmd.compare("cube")) {
		if (numParams != 1) stopInvalid();
		float size = atof(params[0].c_str()) ;
		objects.push_back("cube") ;
		objTransforms.push_back(transfstack.top()) ;
		objSize.push_back(size) ;
		saveProps() ;
	} else if (!cmd.compare("translate")) {
		if (numParams != 3) stopInvalid();
		float xyz[3];
		for (int i = 0; i < 3; i++) xyz[i] = atof(params[i].c_str()) ;
		alterStack(Transform::translate(xyz[0], xyz[1], xyz[2])) ;
	} else if (!cmd.compare("rotate")) {
		if (numParams != 4) stopInvalid();
		float xyz[3];
		for (int i = 0; i < 3; i++) xyz[i] = atof(params[i].c_str()) ;
		vec3 axis = vec3(xyz[0], xyz[1], xyz[2]) ;
		float degrees = atof(params[3].c_str()) ;
		mat3 rotateMat = Transform::rotate(degrees, axis) ;
		alterStack(mat4(vec4(rotateMat[0], 0.0), vec4(rotateMat[1], 0.0), vec4(rotateMat[2], 0.0), vec4(vec3(0.0), 1.0))) ;
	} else if (!cmd.compare("scale")) {
		if (numParams != 3) stopInvalid();
		float xyz[3];
		for (int i = 0; i < 3; i++) xyz[i] = atof(params[i].c_str()) ;
		alterStack(Transform::scale(xyz[0], xyz[1], xyz[2])) ;
	} else if (!cmd.compare("pushTransform")) {
		if (numParams != 0) stopInvalid();
		transfstack.push(transfstack.top()) ;
	} else if (!cmd.compare("popTransform")) {
		if (numParams != 0) stopInvalid();
		transfstack.pop() ;
	} else {
		stopInvalid() ;
	}
}

void parse(const char * filename) {
	std::string str, ret = "" ; 
    std::ifstream in ; 
    in.open(filename) ; 
    if (!in.is_open()) {
		std::cerr << "Unable to Open File " << filename << "\n" ; 
		throw 2 ;
	}
	getline(in, str) ;
	while (in) {
		if ((str.find_first_not_of("/t/r/n") != std::string::npos) && (str[0] != '#')) {
			std::string cmd, buf = "";
			std::vector<std::string> params ;
			std::stringstream ss(str) ;
			ss >> cmd ;
			while (ss >> buf) {
				params.push_back(buf) ;
			}
			execute(cmd, params) ;
		}
		getline (in, str) ;
    }
}

int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("HW2: Transformations");

	GLenum err = glewInit() ; 

	init();
	parse(argv[1]);
	glutDisplayFunc(display);
	glutSpecialFunc(specialKey);
	// glutKeyboardFunc(keyboard);

	// MICE
	glutMouseFunc(mouse);
	glutMotionFunc(mouseDrag);
	// MICE

	// CONTROL
	if(alt_controls){
		glutKeyboardFunc(altKeyboard);
	}else{
		glutKeyboardFunc(keyboard);
	}
	if (transop == view) glutKeyboardUpFunc(keyboardUp);
	glutIdleFunc(display) ;
	glutReshapeFunc(reshape);
	glutReshapeWindow(600, 400);
	printHelp();
	glutMainLoop();
	return 0;
}