#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "LocalizedRangeScanAndFinder.h"
#include "ScanMatcher.h"
#include "SensorManager.h"

class Wrapper {
  LaserRangeFinder *rangeFinder;
  Name name;
  ScanMatcher *matcher;
  // SensorManager *sensor_manager;

public:
  Wrapper(std::string sensorName, double angularResolution, double angleMin,
          double angleMax) {
    this->name = Name(sensorName);
    this->rangeFinder = new LaserRangeFinder(this->name);
    this->rangeFinder->SetAngularResolution(angularResolution);
    this->rangeFinder->SetMinimumAngle(angleMin);
    this->rangeFinder->SetMaximumAngle(angleMax);
    this->rangeFinder->Update();
    // this->sensor_manager = new SensorManager();
    SensorManager::GetInstance()->RegisterSensor(this->rangeFinder);
    ScanMatcherConfig * config = new ScanMatcherConfig();

    // CorrelationSearchSpaceDimension
    // CorrelationSearchSpaceResolution
    // CorrelationSearchSpaceSmearDeviation
    // rangeThreshold
    this->matcher = ScanMatcher::Create(config, 0.3, 0.01, 0.03, 12);
  }

  LaserRangeFinder *getRangeFinder() { return this->rangeFinder; }

  Name getName() { return this->name; }

  bool ProcessLocalizedRangeScan(std::vector<double> ranges, double x, double y,
                                 double heading) {
    auto scan = new LocalizedRangeScan(this->name, ranges);
    scan->SetOdometricPose(Pose2(x, y, heading));
    scan->SetCorrectedPose(Pose2(x, y, heading));

    return true;
  }

  LocalizedRangeScan *MakeScan(std::vector<double> ranges, double x, double y,
                               double yaw) {
    auto scan = new LocalizedRangeScan(this->name, ranges);
    scan->SetOdometricPose(Pose2(x, y, yaw));
    scan->SetCorrectedPose(Pose2(x, y, yaw));

    return scan;
  }

  double MatchScan(LocalizedRangeScan *query,
                   const LocalizedRangeScanVector &base) {
    Pose2 mean;
    Matrix3 covariance;

    return this->matcher->MatchScan(query, base, mean, covariance);
  }

  ~Wrapper() {
    delete this->rangeFinder;
    delete this->matcher;
  }
};

namespace py = pybind11;

PYBIND11_MODULE(mp_slam_cpp, m) {
  py::class_<Pose2>(m, "Pose2")
      .def(py::init<double, double, double>())
      .def_property("x", &Pose2::GetX, &Pose2::SetX)
      .def_property("y", &Pose2::GetY, &Pose2::SetY)
      .def_property("yaw", &Pose2::GetHeading, &Pose2::SetHeading)
      .def("__repr__", [](const Pose2 &a) {
        std::stringstream buffer;
        buffer << "(x: " << a.GetX() << ", y: " << a.GetY()
               << ", heading: " << a.GetHeading() << ")\n";
        return buffer.str();
      });

  py::class_<Vector2<double>>(m, "Vec2_d")
      .def(py::init<double, double>())
      .def_property("x", &Vector2<double>::GetX, &Vector2<double>::SetX)
      .def_property("y", &Vector2<double>::GetY, &Vector2<double>::SetY)
      .def("__repr__", [](const Vector2<double> &a) {
        std::stringstream buffer;
        buffer << "(x: " << a.GetX() << ", y:" << a.GetY() << ")\n";
        return buffer.str();
      });

  py::class_<Name>(m, "Name").def(py::init<const std::string &>());

  py::class_<LaserRangeFinder>(m, "LaserRangeFinder")
      .def(py::init<std::string &>())
      .def("set_offset_pose", &LaserRangeFinder::SetOffsetPose)
      .def("set_angular_resolution", &LaserRangeFinder::SetAngularResolution)
      .def("set_minimum_range", &LaserRangeFinder::SetMinimumRange)
      .def("set_minimum_angle", &LaserRangeFinder::SetMinimumAngle)
      .def("set_maximum_range", &LaserRangeFinder::SetMaximumRange)
      .def("set_maximum_angle", &LaserRangeFinder::SetMaximumAngle)
      .def("set_angular_resolution", &LaserRangeFinder::SetAngularResolution)
      .def("set_range_threshold", &LaserRangeFinder::SetRangeThreshold);

  py::class_<LocalizedRangeScan>(m, "LocalizedRangeScan")
      .def(py::init<Name, std::vector<double>>())
      .def("set_odometric_pose", &LocalizedRangeScan::SetOdometricPose)
      .def("get_odometric_pose", &LocalizedRangeScan::GetOdometricPose)
      .def("set_corrected_pose", &LocalizedRangeScan::SetCorrectedPose)
      .def("get_corrected_pose", &LocalizedRangeScan::GetCorrectedPose);

  py::class_<Wrapper>(m, "Wrapper")
      .def(py::init<std::string, double, double, double>())
      .def("process_scan", &Wrapper::ProcessLocalizedRangeScan)
      .def("make_scan", &Wrapper::MakeScan, py::return_value_policy::reference)
      .def("match_scan", &Wrapper::MatchScan,
           py::return_value_policy::reference)
      .def_property_readonly("name", &Wrapper::getName)
      .def_property_readonly("range_finder", &Wrapper::getRangeFinder);

#ifdef VERSION_INFO
  m.attr("__version__") = VERSION_INFO;
#else
  m.attr("__version__") = "dev";
#endif
}
