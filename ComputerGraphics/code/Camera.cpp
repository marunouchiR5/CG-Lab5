#include "Camera.h"
#include "glm/gtx/string_cast.hpp"
#include <string>
#include <iostream>
#include <cmath>


Camera::Camera() {
	reset();
}

Camera::~Camera() {
}

void Camera::reset() {
	orientLookAt(glm::vec3(0.0f, 0.0f, DEFAULT_FOCUS_LENGTH), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	setViewAngle(VIEW_ANGLE);
	setNearPlane(NEAR_PLANE);
	setFarPlane(FAR_PLANE);
	screenWidth = screenHeight = 200;
	screenWidthRatio = 1.0f;
	rotU = rotV = rotW = 0;
}

//called by main.cpp as a part of the slider callback for controlling rotation
// the reason for computing the diff is to make sure that we are only incrementally rotating the camera
void Camera::setRotUVW(float u, float v, float w) {

	float diffU = u - rotU;
	float diffV = v - rotV;
	float diffW = w - rotW;
	rotateU(diffU);
	rotateV(diffV);
	rotateW(diffW);
	rotU = u;
	rotV = v;
	rotW = w;
}


void Camera::orientLookAt(glm::vec3 eyePoint, glm::vec3 lookatPoint, glm::vec3 upVec) {
	glm::vec3 lookVec = lookatPoint - eyePoint;
	orientLookVec(eyePoint, lookVec, upVec);
}


void Camera::orientLookVec(glm::vec3 _eyePoint, glm::vec3 lookVec, glm::vec3 upVec) {
	eyePoint = _eyePoint;
	modelViewMatrix = glm::mat4(1.0f);
	upV = upVec;
	lookV = lookVec;
	w = -1.0f * lookVec / glm::length(lookVec);
	u = glm::cross(upVec, w) / glm::length(glm::cross(upVec, w));
	v = glm::cross(w, u);
	//std::cout << "lookV: " << glm::to_string(lookV) << std::endl;
	//std::cout << "eyePoint: " << glm::to_string(eyePoint) << std::endl;
	//std::cout << "lookVec: " << glm::to_string(lookVec) << std::endl << "upVec: " << glm::to_string(upVec) << std::endl;
	//std::cout << "U: " << glm::to_string(u) << std::endl << "V: " << glm::to_string(v) << std::endl << "W: " << glm::to_string(w) << std::endl;

	translate(eyePoint);


	glm::mat4 rot(1.0f);

	rot[0] = glm::vec4(v, 0);
	rot[1] = glm::vec4(u, 0);
	rot[2] = glm::vec4(-1.0f * w.x, w.y, w.z, 0);
	rot = glm::transpose(rot);

	//modelViewMatrix = rot * modelViewMatrix;
	//rotateW(rotW);
	//rotateV(rotV);
	//rotateU(rotU);
}

glm::mat4 Camera::getScaleMatrix() {
	// glm::mat4 scaleMat4(1.0);
	// float width = tan(glm::radians(viewAngle/2)) * farPlane;
	// //float height = 1.0;
	// float height = width / getScreenWidthRatio();
	// scaleMat4[0][0] = 1 / width;
	// scaleMat4[1][1] = 1 / height;
	// scaleMat4[2][2] = 1 / farPlane;
	//std::cout << "Screen width: " << screenWidth << " Screen height: " << screenHeight << std::endl;
	float far = farPlane;
	//far = farPlane;
	//std::cout << "Far: " << far << std::endl;
	screenWidthRatio = float(screenHeight) / float(screenWidth);
	//std::cout << "Screen Width Ratio: " << screenWidthRatio << std::endl;
	glm::mat4 scaleMat4(1.0f);
	float width = (tan(glm::radians(viewAngle) / 2.0f) * far); // w/2=tan(theta_w/2)*far
	float height = width * screenWidthRatio; //h/2 = tan(theta_w/2) *screenWidthRation * far
	//std::cout << "Width: " << width << std::endl;
	//std::cout << "Height: " << height << std::endl;
	scaleMat4[0][0] = 1.0f / width;
	scaleMat4[1][1] = 1.0f / height;
	scaleMat4[2][2] = 1.0f / far;
	return scaleMat4;
	//return glm::inverse(getScaleMatrix());
}

glm::mat4 Camera::getInverseScaleMatrix() {
	glm::mat4 invScaleMat4(1.0f);
	invScaleMat4[0][0] = tan(glm::radians(viewAngle) / 2.0f) * farPlane;
	invScaleMat4[1][1] = tan((glm::radians(viewAngle)) / 2.0f) * farPlane * screenWidthRatio;
	invScaleMat4[2][2] = farPlane;
	//return invScaleMat4;
	return glm::inverse(getScaleMatrix());
}

glm::mat4 Camera::getUnhingeMatrix() {
	glm::mat4 unhingeMat4(1.0f);
	float c = -1.0f * nearPlane / farPlane;
	//std::cout << "c: " << c << std::endl;
	// std::cout << c << std::endl;
	unhingeMat4[2][2] = -1.0f / (c + 1.0f);
	unhingeMat4[3][2] = c / (c + 1.0f);
	unhingeMat4[2][3] = -1.0f;
	unhingeMat4[3][3] = 0.0f;
	//glm::mat4 test = glm::translate(glm::mat4(1.0f), eyePoint);
	//std::cout << "test: " << glm::to_string(test) << std::endl;
	//std::cout << "unhingeMat4: " << glm::to_string(unhingeMat4) << std::endl;
	// std::cout << to_string(unhingeMat4) << std::endl;
	return unhingeMat4;
}


glm::mat4 Camera::getProjectionMatrix() {
	glm::mat4 projMat4(1.0f);

	// translate eye to 0,0,0
	glm::mat4 trans = glm::translate(glm::mat4(1.0f), -1.0f * eyePoint);

	// rotate into canonical position
	glm::mat4 rot(1.0f);
	rot[0] = glm::vec4(v, 0);
	rot[1] = glm::vec4(u, 0);
	rot[2] = glm::vec4(w, 0);
	// rot[2] = glm::vec4(-1.0f * w.x, w.y, w.z, 0);
	rot = glm::transpose(rot);

	// scale to easy size
	glm::mat4 scale = getScaleMatrix();
	// unhinge
	glm::mat4 unhinge = getUnhingeMatrix();

	projMat4 = unhinge * scale * projMat4;
	return projMat4;
}

glm::mat4 Camera::getInverseModelViewMatrix() {

	return glm::inverse(getModelViewMatrix());
}

void Camera::setViewAngle(float _viewAngle) {
	viewAngle = _viewAngle;
}

void Camera::setNearPlane(float _nearPlane) {
	nearPlane = _nearPlane;
}

void Camera::setFarPlane(float _farPlane) {
	farPlane = _farPlane;
}

void Camera::setScreenSize(int _screenWidth, int _screenHeight) {
	screenWidth = _screenWidth;
	screenHeight = _screenHeight;
	screenWidthRatio = screenHeight / screenWidth;
}

glm::mat4 Camera::getModelViewMatrix() {

	glm::mat4 trans(1.0f);

	trans = glm::translate(glm::mat4(1.0f), -1.0f * (eyePoint + lookV * nearPlane));
	trans = glm::translate(glm::mat4(1.0f), -1.0f * (eyePoint));

	glm::mat4 rot(1.0f);  //Rotation Matrix

	rot[0] = glm::vec4(u, 0); //V-component
	rot[1] = glm::vec4(v, 0); //U-component
	rot[2] = glm::vec4(w, 0); //W-component
	rot = glm::transpose(rot); //get inverse

	return rot * trans;
}


void Camera::rotateV(float degrees) {
	glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(degrees), v);
	u = rot * glm::vec4(u, 0.0f);
	w = rot * glm::vec4(w, 0.0f);
	lookV = rot * glm::vec4(lookV, 0.0f);
}

void Camera::rotateU(float degrees) {
	glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(degrees), u);
	v = rot * glm::vec4(v, 0.0f);
	w = rot * glm::vec4(w, 0.0f);
	lookV = rot * glm::vec4(lookV, 0.0f);
}

void Camera::rotateW(float degrees) {
	glm::mat4 rot = glm::rotate(glm::mat4(1.0f), -glm::radians(degrees), w);
	u = rot * glm::vec4(u, 0.0f);
	v = rot * glm::vec4(v, 0.0f);
}

void Camera::rotate(glm::vec3 point, glm::vec3 axis, float degrees) {
	/*
	*Update calculation of modelview and translation
	*/
	glm::vec3 xy_axis = axis;
	glm::vec3 x_axis = axis;
	xy_axis.z = 0;
	x_axis.y = 0;
	x_axis.z = 0;
	float xy_dot = glm::dot(axis, xy_axis);
	float xy_x_dot = glm::dot(xy_axis, x_axis);

	float theta1 = acos(xy_dot / glm::length(axis) / glm::length(xy_axis)); //Theta between point an xy plane
	float theta2 = acos(xy_x_dot / glm::length(xy_axis) / glm::length(x_axis)); //Angle between the crushed vector and the x axis


	glm::mat4 trans1 = glm::translate(glm::mat4(1.0f), eyePoint);
	glm::mat4 m1 = glm::rotate(glm::mat4(1.0f), theta1, glm::vec3(0, 1, 0)); //rotate axis into xy plane
	glm::mat4 m2 = glm::rotate(glm::mat4(1.0f), theta2, glm::vec3(0, 0, 1)); //rotate axis into x axis
	glm::mat4 m3 = glm::rotate(glm::mat4(1.0f), glm::radians(degrees), glm::vec3(1, 0, 0));  //rotate around degrees
	glm::mat4 m4 = glm::transpose(m2); //rotate back to xy plane
	glm::mat4 m5 = glm::transpose(m1); //rotate back to xyz space
	glm::mat4 trans2 = glm::translate(glm::mat4(1.0f), -1.0f * eyePoint);

	modelViewMatrix = trans2 * m5 * m4 * m3 * m2 * m1 * trans1 * modelViewMatrix;
}

void Camera::translate(glm::vec3 v) {
	eyePoint = v;
	//trans4 = glm::translate(glm::mat4(1.0f), v);
	//glm::mat4 trans = glm::translate(glm::mat4(1.0f), v);
	//modelViewMatrix = trans * modelViewMatrix;


}


glm::vec3 Camera::getEyePoint() {
	return eyePoint;
}

glm::vec3 Camera::getLookVector() {
	return lookV;
}

glm::vec3 Camera::getUpVector() {
	return v;
}

float Camera::getViewAngle() {
	return viewAngle;
}

float Camera::getNearPlane() {
	return nearPlane;
}

float Camera::getFarPlane() {
	return farPlane;
}

int Camera::getScreenWidth() {
	return screenWidth;
}

int Camera::getScreenHeight() {
	return screenHeight;
}

float Camera::getScreenWidthRatio() {
	return screenWidthRatio;
}