
#include "SkyMate.h"
#define DEBUG
#include "Debug.h"
#include <TinyGPS++.h>

Navigation::Navigation(TinyGPSPlus &gps) {
	myGPS = &gps;
}

bool Navigation::behindLine(SkyMatePosition currentPosition, NavigationPoint turnPoint, double course) {
	double bearingToPosition = myGPS->courseTo( turnPoint.lat, turnPoint.lon, currentPosition.lat, currentPosition.lon);
	double diff = course - bearingToPosition;
	if (diff > 270) return false;
	if (diff < -270) return false;
	if ((diff > -90) && (diff < 90)) return false;
	return true;
}

void Navigation::loop(SkyMatePosition currentPosition) {
	if (!task.checkTaskValid()) task.taskStatus = TASK_FREE_FLIGHT;

	switch (task.taskStatus)
	{
	case TASK_FREE_FLIGHT:
		if (task.checkTaskValid()) {
			task.taskStatus = TASK_PRESTART;
		}
		break;
	case TASK_PRE_PRESTART:
		if (task.startLine.gateTime > 0) {
			// we have an active task.
			if ((task.startLine.gateTime - currentPosition.time) < 3000) { // If there is less than 5 min to go.
				task.taskStatus = TASK_PRESTART;
			}
		}
		break;
	case TASK_PRESTART:
		if (task.compTask && ((task.startLine.gateTime == 0) || ((task.startLine.gateTime - currentPosition.time) > 0))) { 
			// Start gate is closed
			if (behindLine(currentPosition, task.startLine.turnPoint, task.startLine.course)) {
				// Behind the line
				task.startLine.behindLine = true;
				if (task.startLine.taskInfo.timeIn = 0) {
					task.startLine.taskInfo.timeIn = currentPosition.time;
				}
			}
			else { 
				// In front of the line
				if (task.startLine.taskInfo.timeIn > 0) {
					// We have crossed the line (Pre Start) 
					task.startLine.startPos = currentPosition;
					task.startLine.taskInfo.timeOut = currentPosition.time;
					task.startLine.taskInfo.timeIn = 0;
				}
			}
		}
		else {
			// Start gate is open
			if (behindLine(currentPosition, task.startLine.turnPoint, task.startLine.course)) {
				// Behind the line
				task.startLine.behindLine = true;
				if (task.startLine.taskInfo.timeIn == 0) {
					task.startLine.taskInfo.timeIn = currentPosition.time;
				}
			}
			else {
				// In front of the line
				task.startLine.behindLine = false;
				if (task.startLine.taskInfo.timeIn > 0) {
					// We have crossed the line
					task.startLine.startPos = currentPosition;
					task.startLine.taskInfo.timeOut = currentPosition.time;
					task.taskStatus = TASK_ONTASK;
				}
				else if (task.startLine.taskInfo.timeOut > 0){
					// We started early..... 
					// Penalty maybe.
					task.taskStatus = TASK_ONTASK;
				}
				else {
					// Hang out here if someone goes back over the line and tries to start
				}
			}
		}
		task.distanceToTurnPoint = myGPS->distanceBetween(currentPosition.lat, currentPosition.lon, task.startLine.turnPoint.lat, task.startLine.turnPoint.lon);
		break;

	case TASK_ONTASK:
		// Handle distance to previous point.
		if (task.currentTurnPoint == 0) {
			task.distanceToPreviousPoint = myGPS->distanceBetween(currentPosition.lat, currentPosition.lon, task.startLine.turnPoint.lat, task.startLine.turnPoint.lon);
			if (behindLine(currentPosition, task.startLine.turnPoint, task.startLine.course)) {
				if (task.compTask == 0) {
					task.taskStatus = TASK_PRESTART;
				}
			}
		}
		else {
			task.distanceToPreviousPoint = myGPS->distanceBetween(currentPosition.lat, currentPosition.lon, task.turnPoint[task.currentTurnPoint-1].turnPoint.lat, task.turnPoint[task.currentTurnPoint-1].turnPoint.lon);
			if (task.insideWedge(task.turnPoint[task.currentTurnPoint - 1], currentPosition.lat, currentPosition.lon)) {
				if (!task.turnPoint[task.currentTurnPoint - 1].taskInfo.rounded) {
					// We have gone back to the turnpoint after missing it!
					task.turnPoint[task.currentTurnPoint - 1].taskInfo.rounded = true;
					task.turnPoint[task.currentTurnPoint - 1].taskInfo.timeIn = currentPosition.time;
				}
			}
			
			if (task.distanceToPreviousPoint < task.turnPoint[task.currentTurnPoint - 1].taskInfo.length) {
				if (!task.turnPoint[task.currentTurnPoint - 1].taskInfo.rounded) {
					// We have gone back to the turnpoint after missing it!
					task.turnPoint[task.currentTurnPoint - 1].taskInfo.rounded = true;
					task.turnPoint[task.currentTurnPoint - 1].taskInfo.timeIn = currentPosition.time;
				}
				task.turnPoint[task.currentTurnPoint - 1].taskInfo.timeOut = currentPosition.time;
			}
			if (task.turnPoint[task.currentTurnPoint - 1].taskInfo.nearest > task.distanceToPreviousPoint) {
				task.turnPoint[task.currentTurnPoint - 1].taskInfo.nearest = task.distanceToPreviousPoint;
			}
		}

		if (task.currentTurnPoint < task.turnPointCount) {
			// Heading to a turnpoint
			task.distanceToTurnPoint = myGPS->distanceBetween(currentPosition.lat, currentPosition.lon, task.turnPoint[task.currentTurnPoint].turnPoint.lat, task.turnPoint[task.currentTurnPoint].turnPoint.lon);
			if ((task.turnPoint[task.currentTurnPoint].taskInfo.nearest > task.distanceToTurnPoint) || (task.turnPoint[task.currentTurnPoint].taskInfo.nearest == -1)) {
				task.turnPoint[task.currentTurnPoint].taskInfo.nearest = task.distanceToTurnPoint;
			}

			if (task.insideWedge(task.turnPoint[task.currentTurnPoint], currentPosition.lat, currentPosition.lon)) {
				// If inside the wedge. 
				task.currentTurnPoint++;
			} else if (task.turnPoint[task.currentTurnPoint].taskInfo.nearest < (task.turnPoint[task.currentTurnPoint].taskInfo.length + TURNPOINT_NEAR_THRESHOLD)) {
				// Glider has been close to turnpoint
				if (task.distanceToTurnPoint > (task.turnPoint[task.currentTurnPoint].taskInfo.length + TURNPOINT_MISSED_THRESHOLD)) {
					// But is now heading far away from turnpoint
					// Penalty
					task.currentTurnPoint++;
				}
				if (task.distanceToTurnPoint < task.turnPoint[task.currentTurnPoint].taskInfo.length) {
					// Inside the turnpoint
					task.currentTurnPoint++;
					task.turnPoint[task.currentTurnPoint].taskInfo.rounded = true;
					task.turnPoint[task.currentTurnPoint].taskInfo.timeIn = currentPosition.time;
					task.turnPoint[task.currentTurnPoint].taskInfo.timeOut = currentPosition.time;
				}
			}
		}
		else {
			// Heading for the finish line
			if (behindLine(currentPosition, task.finishLine.turnPoint, task.finishLine.course)) {
				// Haven't made it there yet.
				task.distanceToTurnPoint = myGPS->distanceBetween(currentPosition.lat, currentPosition.lon, task.finishLine.turnPoint.lat, task.finishLine.turnPoint.lon);
			}
			else {
				// Crossed the line!!!!!
				task.distanceToTurnPoint = 0;
				task.finishLine.finishPos = currentPosition;
				task.taskStatus = TASK_FINISHED;
			}
		}
		break;
	case TASK_FINISHED:
		// Done and dusted.
		// Send something back to the server to say that the task was finished.
		break;
	default:
		break;
	}
}

double Navigation::headingToTP(SkyMatePosition currentPosition) {
	if (task.taskStatus == 1) {
		return myGPS->courseTo(currentPosition.lat, currentPosition.lon, task.startLine.turnPoint.lat, task.startLine.turnPoint.lon);
	}
	else if (task.taskStatus == 2){
		if (task.currentTurnPoint < task.turnPointCount) {
			return myGPS->courseTo( currentPosition.lat, currentPosition.lon, task.turnPoint[task.currentTurnPoint].turnPoint.lat, task.turnPoint[task.currentTurnPoint].turnPoint.lon);
		}
		else {
			return myGPS->courseTo( currentPosition.lat, currentPosition.lon, task.finishLine.turnPoint.lat, task.finishLine.turnPoint.lon);
		}
	}
	return 361;
}

double Navigation::relativeBearing(SkyMatePosition currentPosition) {
	double temp = headingToTP(currentPosition);
	if (temp > 360) return 0;
	temp -= currentPosition.heading;
	if (temp > 180) {
		temp -= 360;
	}
	else if (temp < -180) {
		temp += 360;
	}

	return temp;
}

double Navigation::distanceBetween(NavigationTurnPoint point1, NavigationTurnPoint point2) {
	return myGPS->distanceBetween(point1.turnPoint.lat, point1.turnPoint.lon, point2.turnPoint.lat, point2.turnPoint.lon);
}
double Navigation::distanceBetween(NavigationTurnPoint point1, SkyMatePosition point2) {
	return myGPS->distanceBetween(point1.turnPoint.lat, point1.turnPoint.lon, point2.lat, point2.lon);
}
double Navigation::distanceBetween(SkyMatePosition point1, NavigationTurnPoint point2) {
	return myGPS->distanceBetween(point1.lat, point1.lon, point2.turnPoint.lat, point2.turnPoint.lon);
}

bool NavigationTask::checkTaskValid(void)
{
	if (emptyTask) return false;
	if (valid) return true;
	if (!startLine.valid) return false;
	if (!finishLine.valid) return false;
	if (turnPointCount > 0) {
		startLine.course = courseTo(startLine.turnPoint.lat, startLine.turnPoint.lon, turnPoint[0].turnPoint.lat, turnPoint[0].turnPoint.lon);
		for (int i = 0; i < turnPointCount; i++) {
			if (!turnPoint[i].valid) return false;
		}
		if (turnPointCount == 1) {
			turnPoint[0].outerDiv = startLine.course;
			turnPoint[0].innerDiv = startLine.course + 180;
			if (turnPoint[0].innerDiv >= 360) {
				turnPoint[0].innerDiv -= 360;
			}
		}
		else {
			turnPoint[0].outerDiv = calculateOuterDiv(startLine.turnPoint, turnPoint[0].turnPoint, turnPoint[1].turnPoint);
			turnPoint[0].innerDiv = calculateInnerDiv(turnPoint[0].outerDiv);

			if (turnPointCount > 2) {
				for (int i = 1; i < (turnPointCount - 1); i++) {
					turnPoint[i].outerDiv = calculateOuterDiv(turnPoint[i-1].turnPoint, turnPoint[i].turnPoint, turnPoint[i+1].turnPoint);
					turnPoint[i].innerDiv = calculateInnerDiv(turnPoint[i].outerDiv);
				}
			}

			turnPoint[turnPointCount-1].outerDiv = calculateOuterDiv(turnPoint[turnPointCount-2].turnPoint, turnPoint[turnPointCount-1].turnPoint, finishLine.turnPoint);
			turnPoint[turnPointCount-1].innerDiv = calculateInnerDiv(turnPoint[0].outerDiv);
		}
		finishLine.course = courseTo(turnPoint[turnPointCount-1].turnPoint.lat, turnPoint[turnPointCount - 1].turnPoint.lon, finishLine.turnPoint.lat, finishLine.turnPoint.lon);
	}
	else {
		startLine.course = courseTo(startLine.turnPoint.lat, startLine.turnPoint.lon, finishLine.turnPoint.lat, finishLine.turnPoint.lon);
		finishLine.course = startLine.course;
	}

	valid = true;
	return true;
}

double NavigationTask::calculateOuterDiv(NavigationPoint point1, NavigationPoint point2, NavigationPoint point3) {
	double bearing1 = courseTo(point2, point1);
	double bearing2 = courseTo(point2, point1);

	double temp = fabs(bearing2 - bearing1) / 2.0;
	if (temp > 90) {
		temp += bearing2;
	}
	else {
		temp += bearing2 + 180;
	}

	if (temp > 360) return temp - 360;
	return temp;
}

double NavigationTask::insideWedge(NavigationTurnPoint turnPoint, double lat, double lon) {
	double courseToTracker = courseTo(turnPoint.turnPoint.lat, turnPoint.turnPoint.lon, lat, lon);
	double delta = courseToTracker - turnPoint.outerDiv;

	if (delta < 22.5 && delta > -22.5) {
		return true;
	}
	if (delta > 337.5) {
		return true;
	}
	if (delta < -337.5) {
		return true;
	}
	return false;
}

double NavigationTask::calculateInnerDiv(double outerDiv) {
	if (outerDiv > 180) {
		return outerDiv - 180;
	}
	return outerDiv + 180;
}

void NavigationTask::wipeTask(void)
{
	emptyTask = true;
	taskStatus = TASK_FREE_FLIGHT;
	startLine.valid = false;
	startLine.behindLine = false;
	finishLine.valid = false; 
	finishLine.finished = false;
	for (int i = 0; i < turnPointCount; i++) {
		turnPoint[i].valid = false;
		turnPoint[i].taskInfo.resetTaskInfo();
	}
	turnPointCount = 0;
	currentTurnPoint = 0;
	valid = false;
}

void NavigationTask::setTask(uint8_t _turnPointCount)
{
	emptyTask = false;
	turnPointCount = _turnPointCount;
	currentTurnPoint = 0;
}

void NavigationTask::setTurnPoint(uint8_t point, char* name, double lat, double lon, double radius)
{
	strcpy(turnPoint[point].name, name);
	turnPoint[point].turnPoint.lat = lat;
	turnPoint[point].turnPoint.lon = lon;
	turnPoint[point].taskInfo.length = radius;
	turnPoint[point].valid = true;
}

void NavigationTask::setStartLine(char* name, double lat, double lon, double maxAlt, double maxSpeed)
{
	strcpy(startLine.name, name);
	startLine.turnPoint.lat = lat;
	startLine.turnPoint.lon = lon;
	startLine.maxAlt.setFeet(maxAlt);
	startLine.maxSpeed.setKnots(maxSpeed);
	startLine.valid = true;
}

void NavigationTask::setFinishLine(char* name, double lat, double lon, double minAlt)
{
	strcpy(finishLine.name, name);
	finishLine.turnPoint.lat = lat;
	finishLine.turnPoint.lon = lon;
	finishLine.minAlt.setFeet(minAlt);
	finishLine.valid = true;
}
