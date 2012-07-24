#include "ArucoPosePublisher.h"

namespace umundo {

using namespace cv;
using namespace aruco;

ArucoPosePublisher::ArucoPosePublisher(const string& domain, const string& calibrationFile) {
	setWindowSize(3);
	if (calibrationFile == "") {
		// default to iSight parameters, actual parameters are generated via opencv and a printed checkboard pattern:
		// $ bin/calibration -w 8 -h 5 -o camera.yml
		int w = 1280, h = 720;
		cv::Mat camera = cv::Mat::eye(3,3,CV_32FC1);
		cv::Mat dist = cv::Mat::zeros(5,1,CV_32FC1);

		camera.at<float>(0,0) = 9.8519920533248728e+02;
		camera.at<float>(0,2) = 6.3855446891811812e+02;
		camera.at<float>(1,1) = 9.8890679500708472e+02;
		camera.at<float>(1,2) = 3.6109373151816021e+02;
		dist.at<float>(0,0) = -1.8543589470473218e-01;
		dist.at<float>(1,0) = 5.9627971053004869e-01;
		dist.at<float>(2,0) = 3.3091593359972763e-03;
		dist.at<float>(3,0) = -1.3846777725374246e-03;
		dist.at<float>(4,0) = -7.7041303098132452e-01;

		_camParameters.setParams(camera, dist, cv::Size(w, h));
	} else {
		_camParameters.readFromXMLFile(calibrationFile);
	}

	_videoCapture.open(0);
	_videoCapture >> _inputImage;
	_camParameters.resize(_inputImage.size());

	_markerSize = 1024;

	_node = new Node(domain);
	_typedPub = new TypedPublisher("pose");
	_typedPub->registerType("Pose", new Pose());
	_node->addPublisher(_typedPub);
}

ArucoPosePublisher::~ArucoPosePublisher() {

}

void ArucoPosePublisher::setWindowSize(int windowSize) {
	_windowSize = windowSize;
	// use a heuristic for alpha
	_alpha = (double)2 / (_windowSize + 1);
}

void ArucoPosePublisher::run() {
	vector<Marker> markers;
	double thresParam1, thresParam2;
	_markerDetector.getThresholdParams(thresParam1, thresParam2);

	while(isStarted() && _videoCapture.grab()) {

		uint64_t now = Thread::getTimeStampMs();
		_videoCapture.retrieve(_inputImage);
		_markerDetector.detect(_inputImage, markers, _camParameters, _markerSize);

		for (unsigned int k = 0; k < markers.size(); k++) {

			// get rotation matrix from compact rotation vector
			cv::Mat Rot(3,3,CV_32FC1);
			cv::Rodrigues(markers[k].Rvec, Rot);

			// get axes to make code cleaner
			double axes[3][3];
			// x axis
			axes[0][0] = -Rot.at<float>(0,0);
			axes[0][1] = -Rot.at<float>(1,0);
			axes[0][2] = +Rot.at<float>(2,0);
			// y axis
			axes[1][0] = -Rot.at<float>(0,1);
			axes[1][1] = -Rot.at<float>(1,1);
			axes[1][2] = +Rot.at<float>(2,1);
			// for z axis, we use cross product
			axes[2][0] = axes[0][1]*axes[1][2] - axes[0][2]*axes[1][1];
			axes[2][1] = - axes[0][0]*axes[1][2] + axes[0][2]*axes[1][0];
			axes[2][2] = axes[0][0]*axes[1][1] - axes[0][1]*axes[1][0];

			/*
			 *  |
			 *  | y
			 *  |     x
			 *  /-------
			 * / z
			 *
			 * The values are calculated as if we were looking on an airplane from above.
			 * Pitch and roll can only be between -PI/2,PI/2 as we would not see the pattern otherwise.
			 * The formulaes below are dot product of the various axis with the normal vector
			 * of the various geometric planes. As each vector's length is 1, we can just use acos.
			 */

			double pitch = acos(-1 * axes[2][2]) - M_PI_2; // z to xy plane
			double roll = acos(-1 * axes[0][2]) - M_PI_2; // x to yx plane
			double yaw1 = acos(axes[0][0]); // x to yz plane
			double yaw2 = acos(axes[2][0]); // z to yz plane

			/* Clockwise rotation of pattern
			 *
			 * yaw1 | yaw2 => desired
			 * PI   | PI/2 => 0
			 * PI/2 | 0    => 1 PI/2
			 * 0    | PI/2 => 2 PI/2
			 * PI/2 | PI   => 3 PI/2
			 */

			double yaw = -1;
			if (yaw1 < M_PI && yaw2 < M_PI_2)
				yaw = M_PI - yaw1;
			else {
				yaw = M_PI + yaw1;
			}

			pitch = fmod(pitch, 2 * M_PI);
			roll = fmod(roll, 2 * M_PI);
			yaw = fmod(yaw, 2 * M_PI);

			Pose* pose = new Pose();
			pose->mutable_orientation()->set_pitch(pitch);
			pose->mutable_orientation()->set_roll(roll);
			pose->mutable_orientation()->set_yaw(yaw);

			// TODO: This is most likely false, but we ignore the position for now anyway
			pose->mutable_position()->set_iswgs84(false);
			pose->mutable_position()->set_latitude(markers[k].Tvec.at<double>(0));
			pose->mutable_position()->set_longitude(markers[k].Tvec.at<double>(1));
			pose->mutable_position()->set_height(markers[k].Tvec.at<double>(2));

			pose->set_timestamp(now);
			stringstream ss;
			ss << markers[k].id;
			string markerId = ss.str();

			_markerHistory[markerId].push_front(pose);

			// make sure window does not grow too large
			while (_markerHistory[markerId].size() > _windowSize) {
				Pose* oldPose = _markerHistory[markerId].back();
				_markerHistory[markerId].pop_back();
				delete oldPose;
			}

			// initilize smoothed pose values with oldest pose entry
			double smoothPitch = _markerHistory[markerId].front()->orientation().pitch();
			double smoothRoll = _markerHistory[markerId].front()->orientation().roll();
			double smoothYaw = _markerHistory[markerId].front()->orientation().yaw();
			double smoothLat = _markerHistory[markerId].front()->position().latitude();
			double smoothLong = _markerHistory[markerId].front()->position().longitude();
			double smoothHeight = _markerHistory[markerId].front()->position().height();

			double histPitch, histRoll, histYaw = 0;
			double histLat, histLong, histHeight = 0;

			std::list<Pose*>::reverse_iterator poseIter = _markerHistory[markerId].rbegin();
			poseIter++; // we know this is possible as we pushed the current pose above

			while(poseIter != _markerHistory[markerId].rend()) {
				// is history pose recent enough?
				if (now - (*poseIter)->timestamp() < 500) {
					histPitch = (*poseIter)->orientation().pitch();
					histRoll = (*poseIter)->orientation().roll();
					histYaw = (*poseIter)->orientation().yaw();
					histLat = (*poseIter)->position().latitude();
					histLong = (*poseIter)->position().longitude();
					histHeight = (*poseIter)->position().height();

					// find smallest angle between history and smoothed value
#if 0
					if (smoothPitch - histPitch > M_PI)
						histPitch -= 2 * M_PI;
					else if (smoothPitch - histPitch < M_PI)
						histPitch += 2 * M_PI;
					if (smoothRoll - histRoll > M_PI)
						histRoll -= 2 * M_PI;
					else if (smoothRoll - histRoll < M_PI)
						histRoll += 2 * M_PI;
					if (smoothYaw - histYaw > M_PI)
						histYaw -= 2 * M_PI;
					else if (smoothYaw - histYaw < M_PI)
						histYaw += 2 * M_PI;
#endif
					smoothPitch = smoothPitch * _alpha + (1 - _alpha) * histPitch;
					smoothRoll = smoothRoll * _alpha + (1 - _alpha) * histRoll;
					smoothYaw = smoothYaw * _alpha + (1 - _alpha) * histYaw;
					smoothLat = smoothLat * _alpha + (1 - _alpha) * histLat;
					smoothLong = smoothLong * _alpha + (1 - _alpha) * histLong;
					smoothLat = smoothHeight * _alpha + (1 - _alpha) * histHeight;
				}
				poseIter++;
			}
//			std::cout << "Pitch: " << std::setw(5) << smoothPitch << " ";
//			std::cout << "Roll: " << std::setw(5) << smoothRoll << " ";
//			std::cout << "Yaw: " << std::setw(5) << smoothYaw << std::endl;

			// make sure values are between 0-2PI again
			smoothPitch = fmod(smoothPitch, 2 * M_PI);
			smoothRoll = fmod(smoothRoll, 2 * M_PI);
			smoothYaw = fmod(smoothYaw, 2 * M_PI);

			// we will use an exponentially weighted moving average to smoothen the values
			Pose* smoothPose = new Pose();

			smoothPose->mutable_orientation()->set_pitch(smoothPitch);
			smoothPose->mutable_orientation()->set_roll(smoothRoll);
			smoothPose->mutable_orientation()->set_yaw(smoothYaw);
			smoothPose->mutable_position()->set_latitude(smoothLat);
			smoothPose->mutable_position()->set_longitude(smoothLong);
			smoothPose->mutable_position()->set_height(smoothHeight);
			smoothPose->mutable_position()->set_iswgs84(false);
			smoothPose->set_timestamp(now);

			Message* msg = _typedPub->prepareMsg("Pose", smoothPose);
			msg->setMeta("markerId", markerId);

			LOG_DEBUG("Publishing pose of marker %d", k);
			_typedPub->send(msg);
			delete msg;

//      Thread::sleepMs(500);
		}
	}
}

}