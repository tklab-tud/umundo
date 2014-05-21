#ifndef POSEPUBLISHER_H_N39C3O1T
#define POSEPUBLISHER_H_N39C3O1T

#include <umundo/core.h>
#include <umundo/s11n.h>
#include <aruco/aruco.h>
#include <aruco/cvdrawingutils.h>
#include <list>

#include "protobuf/generated/Pose.pb.h"

namespace umundo {

using namespace cv;
using namespace aruco;

class UMUNDO_API ArucoPosePublisher : public Thread {
public:
	ArucoPosePublisher(const string&, const string&);
	virtual ~ArucoPosePublisher();
	void setWindowSize(int windowSize);

	void run();

protected:
	TypedPublisher* _typedPub;
	Node* _node;
	Mat _inputImage;
	VideoCapture _videoCapture;
	MarkerDetector _markerDetector;
	CameraParameters _camParameters;
	float _markerSize;
	unsigned int _windowSize;
	float _alpha;
	std::map<string, std::list<Pose*> > _markerHistory;
};

}

#endif /* end of include guard: POSEPUBLISHER_H_N39C3O1T */
