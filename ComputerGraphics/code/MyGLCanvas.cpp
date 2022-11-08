#include "MyGLCanvas.h"
#include <glm/gtc/type_ptr.hpp>

MyGLCanvas::MyGLCanvas(int x, int y, int w, int h, const char *l) : Fl_Gl_Window(x, y, w, h, l) {
	mode(FL_RGB | FL_ALPHA | FL_DEPTH | FL_DOUBLE);
	
	eyePosition = glm::vec3(0.0f, 0.0f, 3.0f);
	lookatPoint = glm::vec3(0.0f, 0.0f, 0.0f);
	rotVec = glm::vec3(0.0f, 0.0f, 0.0f);

	wireframe = 0;
	viewAngle = 60;
	clipNear = 0.01f;
	clipFar = 10.0f;

	castRay = false;
	drag = false;
	mouseX = 0;
	mouseY = 0;
	spherePosition = glm::vec3(0, 0, 0);

	myObject = new SceneObject(175);
	camera.setViewAngle(viewAngle);
	camera.setNearPlane(clipNear);
	camera.setFarPlane(clipFar);
	// Set the mode so we are modifying our objects.
	camera.orientLookVec(eyePosition, glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
}

MyGLCanvas::~MyGLCanvas() {
}

/* The generateRay function accepts the mouse click coordinates
	(in x and y, which will be integers between 0 and screen width and 0 and screen height respectively).
   The function returns the ray
*/
glm::vec3 MyGLCanvas::generateRay(int pixelX, int pixelY) {
	glm::vec3 eyePoint = camera.getEyePoint();
	glm::vec3 lookVector = camera.getLookVector();
	float nearPlane = camera.getNearPlane();
	int screenWidth = camera.getScreenWidth(); // do we use pixelWidth?
	int screenHeight = camera.getScreenHeight();
	float viewAngle = camera.getViewAngle();
	float screenWidthRatio = (float)screenHeight / (float)screenWidth;
	glm::vec3 upVector = camera.getUpVector();
	glm::vec3 w = -1.0f * lookVector / glm::length(lookVector);
	glm::vec3 u = glm::cross(upVector, w) / glm::length(glm::cross(upVector, w));
	glm::vec3 v = glm::cross(w, u);
	float width = (tan(glm::radians(viewAngle) / 2.0f) * nearPlane); // w/2=tan(theta_w/2)*far
	float height = width * screenWidthRatio;

	glm::vec3 Q = eyePoint + nearPlane * lookVector;
	float a = -width + 2.0f * width * ((float)pixelX / (float)screenWidth);
	float b = -height + 2.0f * height * ((float)pixelY / (float)screenHeight);

	glm::vec3 S = Q + a * u + b * v;
	glm::vec3 dHat = glm::normalize(S - eyePoint);
	dHat.y = -dHat.y;
	return dHat;
}

glm::vec3 MyGLCanvas::getEyePoint() {
	return camera.getEyePoint();
}

/* The getIsectPointWorldCoord function accepts three input parameters:
	(1) the eye point (in world coordinate)
	(2) the ray vector (in world coordinate)
	(3) the "t" value

	The function should return the intersection point on the sphere
*/
glm::vec3 MyGLCanvas::getIsectPointWorldCoord(glm::vec3 eye, glm::vec3 ray, float t) {
	glm::vec3 p = eye + t * ray;
	return p;
}

/* The intersect function accepts three input parameters:
	(1) the eye point (in world coordinate)
	(2) the ray vector (in world coordinate)
	(3) the transform matrix that would be applied to there sphere to transform it from object coordinate to world coordinate

	The function should return:
	(1) a -1 if no intersection is found
	(2) OR, the "t" value which is the distance from the origin of the ray to the (nearest) intersection point on the sphere
*/
double MyGLCanvas::intersect (glm::vec3 eyePointP, glm::vec3 rayV, glm::mat4 transformMatrix) {
	double t = -1;

	glm::vec4 eyePointPO = glm::inverse(transformMatrix) * glm::vec4(eyePointP, 1);
	glm::vec4 d = glm::inverse(transformMatrix) * glm::vec4(rayV, 0);

	float r = 0.5;
	float a = glm::dot(glm::vec3(d), glm::vec3(d));
	float b = 2 * glm::dot(glm::vec3(eyePointPO), glm::vec3(d));
	float c = glm::dot(glm::vec3(eyePointPO), glm::vec3(eyePointPO)) - r * r;
	double delta = b * b - 4 * a * c;

	if (delta <= 0) {
		return t;
	}
	else {
		return std::min((-b + sqrt(delta)) / (2 * a), (-b - sqrt(delta)) / (2 * a));
	}

	return t;
}


void MyGLCanvas::draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!valid()) {  //this is called when the GL canvas is set up for the first time or when it is resized...
		printf("establishing GL context");

		// Set the base texture of our object. Note that loading gl texture can 
		//  only happen after the gl context has been established
		if (myObject->baseTexture == NULL) {
			myObject->setTexture(0, "./data/pink.ppm");
		}
		// Set a second texture layer to our object
		if (myObject->blendTexture == NULL) {
			myObject->setTexture(1, "./data/smile.ppm");
		}

		glViewport(0, 0, w(), h());
		updateCamera(w(), h());

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		//glShadeModel(GL_SMOOTH);
		glShadeModel(GL_FLAT);

		GLfloat light_pos0[] = { eyePosition.x, eyePosition.y, eyePosition.z, 0.0f };
		GLfloat ambient[] = { 0.7f, 0.7f, 0.7f, 1.0f };
		GLfloat diffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };

		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_POSITION, light_pos0);

		glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		/****************************************/
		/*          Enable z-buferring          */
		/****************************************/

		glEnable(GL_DEPTH_TEST);
		glPolygonOffset(1, 1);
	}

	// Clear the buffer of colors in each bit plane.
	// bit plane - A set of bits that are on or off (Think of a black and white image)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene();
}

void MyGLCanvas::drawScene() {
	glMatrixMode(GL_MODELVIEW);
	// Set the mode so we are modifying our objects.
	camera.orientLookVec(eyePosition, glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
	glLoadMatrixf(glm::value_ptr(camera.getModelViewMatrix()));

	if (castRay == true) {
		glm::vec3 eyePointP = getEyePoint();
		glm::vec3 rayV = generateRay(mouseX, mouseY);
		glm::vec3 sphereTransV(spherePosition[0], spherePosition[1], spherePosition[2]);

		float t = intersect(eyePointP, rayV, glm::translate(glm::mat4(1.0), sphereTransV));
		glm::vec3 isectPointWorldCoord = getIsectPointWorldCoord(eyePointP, rayV, t);

		if (t > 0) {
			glColor3f(1, 0, 0);
			glPushMatrix();
				glTranslated(spherePosition[0], spherePosition[1], spherePosition[2]);
				glutWireCube(1.0f);
			glPopMatrix();
			glPushMatrix();
				glTranslatef(isectPointWorldCoord[0], isectPointWorldCoord[1], isectPointWorldCoord[2]);
				glutSolidSphere(0.05f, 10, 10);
			glPopMatrix();
			printf("hit!\n");
		}
		else {
			printf("miss!\n");
		}
	}

	glPushMatrix();

	//move the sphere to the designated position
	glTranslated(spherePosition[0], spherePosition[1], spherePosition[2]);

	glDisable(GL_POLYGON_OFFSET_FILL);
	glColor3f(1.0, 1.0, 1.0);
	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	glPushMatrix();
		glRotatef(90, 0, 1, 0);
		myObject->drawTexturedSphere();
	glPopMatrix();

	glPopMatrix();
}


void MyGLCanvas::updateCamera(int width, int height) {
	float xy_aspect;
	xy_aspect = (float)width / (float)height;

	camera.setScreenSize(width, height);

	// Determine if we are modifying the camera(GL_PROJECITON) matrix(which is our viewing volume)
	// Otherwise we could modify the object transormations in our world with GL_MODELVIEW
	glMatrixMode(GL_PROJECTION);
	// Reset the Projection matrix to an identity matrix
	glLoadIdentity();
	glm::mat4 projection = camera.getProjectionMatrix();
	glLoadMatrixf(glm::value_ptr(projection));
}


int MyGLCanvas::handle(int e) {
	//printf("Event was %s (%d)\n", fl_eventnames[e], e);
	switch (e) {
	case FL_DRAG:
		mouseX = (int)Fl::event_x();
		mouseY = (int)Fl::event_y();
		if (drag == true) {
			printf("drag and move\n");

			//idea before lab ends:
			// we click once to start dragging (FL_PUSH - gives us old_t, old_center, etc)
			// for each frame of FL_DRAG:
			//	//NOTE: we dont need to do x and y independently, can just use the vector
				// compute change in x (old_x[intersection x] - new_x) and change in y from previous
				//increment sphere_center by those changes in x and y
			//move it depth wise based on old_t

			glm::vec3 eyePointP = getEyePoint();
			glm::vec3 dHat = generateRay(mouseX, mouseY);
			//glm::vec3 sphereTransV(spherePosition[0], spherePosition[1], spherePosition[2]);
			//float t = intersect(eyePointP, rayV, glm::translate(glm::mat4(1.0), sphereTransV));
			glm::vec3 isectPointWorldCoord = getIsectPointWorldCoord(eyePointP, dHat, oldT);


			//glm::vec3 mousePos = glm::vec3((float) mouseX / camera.getScreenWidth(), mouseY / camera.getScreenHeight(), 0);

			glm::vec3 distance = oldIsectPoint - oldCenter;
			//glm::vec3 distance = isectPointWorldCoord - oldIsectPoint;

			spherePosition = isectPointWorldCoord - distance;

			//spherePosition.z = oldCenter.z;

			oldCenter = spherePosition;
			oldIsectPoint = isectPointWorldCoord;

			//TODO: compute the new spherePosition as you drag your mouse. spherePosition represents the coordinate for the center of the sphere
			//HINT: use the old t value (computed from when you first intersect the sphere (before dragging starts)) to determine the new spherePosition
			//spherePosition;
		}
		return (1);
	case FL_MOVE:
		Fl::belowmouse(this);
		//printf("mouse move event (%d, %d)\n", (int)Fl::event_x(), (int)Fl::event_y());
		mouseX = (int)Fl::event_x();
		mouseY = (int)Fl::event_y();

		break;
	case FL_PUSH:
		printf("mouse push\n");
		if ((Fl::event_button() == FL_LEFT_MOUSE) && (castRay == false)) { //left mouse click -- casting Ray
			castRay = true;
		}
		else if ((Fl::event_button() == FL_RIGHT_MOUSE) && (drag == false)) { //right mouse click -- dragging
			//this code is run when the dragging first starts (i.e. the first frame). 
			//it stores a bunch of values about the sphere's "original" position and information
			glm::vec3 eyePointP = getEyePoint();
			glm::vec3 rayV = generateRay(mouseX, mouseY);
			glm::vec3 sphereTransV(spherePosition[0], spherePosition[1], spherePosition[2]);
			float t = intersect(eyePointP, rayV, glm::translate(glm::mat4(1.0), sphereTransV));
			glm::vec3 isectPointWorldCoord = getIsectPointWorldCoord(eyePointP, rayV, t);

			if (t > 0) {
				drag = true;
				printf("drag is true\n");
				oldCenter = spherePosition;
				oldIsectPoint = isectPointWorldCoord;
				oldT = t;
			}
		}
		return (1);
	case FL_RELEASE:
		printf("mouse release\n");
		if (Fl::event_button() == FL_LEFT_MOUSE) {
			castRay = false;
		}
		else if (Fl::event_button() == FL_RIGHT_MOUSE) {
			drag = false;
		}
		return (1);
	case FL_KEYUP:
		printf("keyboard event: key pressed: %c\n", Fl::event_key());
		switch (Fl::event_key()) {
		case 'w': eyePosition.y += 0.05f;  break;
		case 'a': eyePosition.x += 0.05f; break;
		case 's': eyePosition.y -= 0.05f;  break;
		case 'd': eyePosition.x -= 0.05f; break;
		}
		updateCamera(w(), h());
		break;
	case FL_MOUSEWHEEL:
		printf("mousewheel: dx: %d, dy: %d\n", Fl::event_dx(), Fl::event_dy());
		eyePosition.z += Fl::event_dy() * -0.05f;
		updateCamera(w(), h());
		break;
	}

	return Fl_Gl_Window::handle(e);
}

void MyGLCanvas::resize(int x, int y, int w, int h) {
	Fl_Gl_Window::resize(x, y, w, h);
	puts("resize called");
}

void MyGLCanvas::drawAxis() {
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
		glColor3f(1.0, 0.0, 0.0);
		glVertex3f(0, 0, 0); glVertex3f(1.0, 0, 0);
		glColor3f(0.0, 1.0, 0.0);
		glVertex3f(0, 0, 0); glVertex3f(0.0, 1.0, 0);
		glColor3f(0.0, 0.0, 1.0);
		glVertex3f(0, 0, 0); glVertex3f(0, 0, 1.0);
	glEnd();
	glEnable(GL_LIGHTING);
}