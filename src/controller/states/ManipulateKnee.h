#pragma once

#include <mc_control/CompletionCriteria.h>
#include <mc_control/fsm/State.h>
#include <mc_tasks/TransformTask.h>
#include <mc_trajectory/SequenceInterpolator.h>
// #include "../../bonetag/BoneTagSerial.h"
#include "../../plugins/ProtoTMR/Serial.h"
#include "../../plugins/bonetag/BoneTagSerial.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

struct ReadCSV
{
  void clear();
  void load(const std::string & path);

  void generateFromConfiguration(const mc_rtc::Configuration & config);

  std::deque<Eigen::Vector3d> femurTranslationVector;
  std::deque<Eigen::Vector3d> femurRotationVector;
  std::deque<Eigen::Vector3d> tibiaTranslationVector;
  std::deque<Eigen::Vector3d> tibiaRotationVector;
};

struct PTransformInterpolator
{
  sva::PTransformd operator()(const sva::PTransformd & p1, const sva::PTransformd & p2, double t)
  {
    return sva::interpolate(p1, p2, t);
  }
};

template<typename SensorDataT>
struct Result
{
  unsigned int controllerIter;
  // Robot effective motion
  Eigen::Vector3d femurRotation;
  Eigen::Vector3d tibiaRotation;
  Eigen::Vector3d femurTranslation;
  Eigen::Vector3d tibiaTranslation;

  // Sensor measurements
  SensorDataT sensorData;
};

struct ProtoTMRResult : Result<io::Serial::TimedRawData>
{
};

struct BoneTagResult : Result<io::BoneTagSerial::Data>
{
};

template<typename ResultT>
struct ResultHandler
{
  void clear()
  {
    results_.clear();
  }

  void addResult(const ResultT & result)
  {
    results_.push_back(result);
  }

  inline const std::vector<ResultT> results() const noexcept
  {
    return results_;
  }

protected:
  std::vector<ResultT> results_;
};

void write_csv_bonetag();
void write_csv_prototmr();

struct ManipulateKnee : mc_control::fsm::State
{
  void start(mc_control::fsm::Controller & ctl) override;

  bool run(mc_control::fsm::Controller & ctl) override;

  void teardown(mc_control::fsm::Controller & ctl) override;

protected:
  void resetToZero();

  void play()
  {
    if(file_.tibiaRotationVector.empty())
    {
      mc_rtc::log::error("[{}] Cannot play, empty file", name());
      return;
    }
    resultPath_ = results_dir_ + "/" + trajectory_file_;
    play_ = true;
    measuredSamples_ = 0;
  }

  void stop()
  {
    play_ = false;
    triggerSaveResults(true);
    resultsBoneTag_.clear();
    resultsProtoTMR_.clear();
    trajOffsets_.reset();
  }

  /**
   * Saving is done in a thread to avoid slowing down the real-time control
   *
   * This function copies the available results data (locks on saveResultsMutex_, thread-safe)
   * Calling triggerSaveResults() will notify the saveResultsThread_ that new data is available to save. When
   * force=false, it will only trigger when there are a multiple of resultSaveAfterN results available.
   *
   * @params force Force copying the latest available results (regardless of resultSaveAfterN value)
   */
  void triggerSaveResults(bool force = false);

  inline void forceNext() noexcept
  {
    mc_rtc::log::warning("[{}] Manually forcing next position/measurement", name());
    iter_ = 0;
    measuredSamples_ = 0;
    next_ = true;
  }

  void setRate(double rate, double timeStep)
  {
    iterRate_ = std::max(1u, static_cast<unsigned>(ceil(1 / (1 / rate * timeStep))));
  }

  double getRate(double timeStep) const noexcept
  {
    return iterRate_ * timeStep;
  }

  inline void updateTibiaOffset(const sva::PTransformd & offset)
  {
    tibiaOffsetInterpolationTime_ = 0;
    tibiaOffsetInterpolator_.values({{0., tibiaOffset_}, {offsetDuration_, offset}});
  }

  inline void updateFemurOffset(const sva::PTransformd & offset)
  {
    femurOffsetInterpolationTime_ = 0;
    femurOffsetInterpolator_.values({{0., femurOffset_}, {offsetDuration_, offset}});
  }

  /// Rotation axis for the knee joint
  void updateAxes(mc_control::fsm::Controller & ctl);

  /// Returns true when all measurements have been taken
  bool measure(mc_control::fsm::Controller & ctl);
  void measure_bonetag(mc_control::fsm::Controller & ctl);
  void measure_prototmr(mc_control::fsm::Controller & ctl);

protected:
  ReadCSV file_;
  std::string trajectory_dir_ = "";
  std::string trajectory_file_ = "";

  bool play_ = false;
  bool manualLogging_ = false;
  bool next_ = true;
  bool hasConverged_ = false;

  bool gotMeasurement_ = false;
  std::string sensorType_ = "None";
  bool allowMissingSensor_ = false;
  size_t iter_ = 0;
  size_t iterRate_ = 1;

  struct Convergence
  {
    bool tibiaAngularError = true;
    bool tibiaLinearError = true;
    bool femurAngularError = true;
    bool femurLinearError = true;
    bool femurVelocityError = true;
    bool tibiaVelocityError = true;

    double translationTreshold_ = 0.1; ///< Convergence threshold on translation [mm]
    double rotationTreshold_ = 0.1; ///< Convergence threshold on rotation [deg]
    double velocityThreshold_ = 0.001; ///< Convergence threshold on velocity [m/s]

    sva::MotionVecd tibiaError_ = sva::MotionVecd::Zero();
    sva::MotionVecd femurError_ = sva::MotionVecd::Zero();
    sva::MotionVecd tibiaVelocityError_ = sva::MotionVecd::Zero();
    sva::MotionVecd femurVelocityError_ = sva::MotionVecd::Zero();
  } convergence_;

  unsigned desiredSamples_ = 1;
  unsigned measuredSamples_ = 0;
  bool newFrameRequested_ = false;

  ResultHandler<ProtoTMRResult> resultsProtoTMR_;
  ResultHandler<BoneTagResult> resultsBoneTag_;
  std::vector<ProtoTMRResult> resultsProtoTMRCopy_; // copy for saving
  std::vector<BoneTagResult> resultsBoneTagCopy_; // copy for saving
  unsigned int resultSaveAfterN = 10; // save after 100 points
  std::string results_dir_ = "/tmp";
  std::string resultPath_ = "/tmp/BoneTagResults.csv";

  // Replay trajectory with offsets
  bool repeatWithOffset_ =
      false; /// When true replay the trajectory from the start with an added offset from trajOffsets_
  struct TrajectoryOffsets
  {
    TrajectoryOffsets()
    {
      reset();
    }

    void next()
    {
      tibiaOffsetTranslation_ = tibiaOffsetsTranslation_[offsetIndex_];
      femurOffsetTranslation_ = femurOffsetsTranslation_[offsetIndex_];
      offsetIndex_ = std::min(offsetIndex_ + 1, offsetsNum_);
    }

    bool done() const noexcept
    {
      return offsetIndex_ == offsetsNum_;
    }

    void reset()
    {
      offsetIndex_ = 0;
      tibiaOffsetTranslation_ = Eigen::Vector3d::Zero();
      femurOffsetTranslation_ = Eigen::Vector3d::Zero();
    }

    const Eigen::Vector3d & tibiaOffsetTranslation() const noexcept
    {
      return tibiaOffsetTranslation_;
    }

    const Eigen::Vector3d & femurOffsetTranslation() const noexcept
    {
      return femurOffsetTranslation_;
    }

    unsigned current() const noexcept
    {
      return offsetIndex_;
    }

    unsigned size() const noexcept
    {
      return offsetsNum_;
    }

  protected:
    unsigned offsetIndex_ = 0;
    std::vector<Eigen::Vector3d> tibiaOffsetsTranslation_{{10., 0., 0.},  {0., 10., 0.}, {-10., 0., 0.},
                                                          {0., -10., 0.}, {0., 0., 10.}, {0., 0., -10.}};
    std::vector<Eigen::Vector3d> femurOffsetsTranslation_ = tibiaOffsetsTranslation_;
    unsigned offsetsNum_ = tibiaOffsetsTranslation_.size();
    Eigen::Vector3d tibiaOffsetTranslation_ = Eigen::Vector3d::Zero();
    Eigen::Vector3d femurOffsetTranslation_ = Eigen::Vector3d::Zero();

  } trajOffsets_;

  sva::PTransformd X_0_femurAxisInitial =
      sva::PTransformd::Identity(); ///< Axis around which the Femur frame rotates and translates
  sva::PTransformd X_0_tibiaAxisInitial =
      sva::PTransformd::Identity(); ///< Axis around which the Tibia frame rotates and translates
  sva::PTransformd X_0_femurAxis =
      sva::PTransformd::Identity(); ///< Axis around which the Femur frame rotates and translates
  sva::PTransformd X_0_tibiaAxis =
      sva::PTransformd::Identity(); ///< Axis around which the Tibia frame rotates and translates

  Eigen::Vector3d femurTranslation_ = Eigen::Vector3d::Zero(); ///< Desired joint translation in [mm]
  Eigen::Vector3d femurTranslationActual_ = Eigen::Vector3d::Zero(); ///< Current joint translation in [mm]
  Eigen::Vector3d minFemurTranslation_{-20, -20, -10}; ///< Min translation [mm]
  Eigen::Vector3d maxFemurTranslation_ = {20, 20, 10}; ///< Max translation [mm]

  Eigen::Vector3d tibiaTranslation_ = Eigen::Vector3d::Zero(); ///< Desired Joint translation in [mm]
  Eigen::Vector3d tibiaTranslationActual_ = Eigen::Vector3d::Zero(); ///< Current joint translation in [mm]
  Eigen::Vector3d minTibiaTranslation_{-20, -20, -10}; ///< Min translation [mm]
  Eigen::Vector3d maxTibiaTranslation_ = {20, 20, 10}; ///< Max translation [mm]

  Eigen::Vector3d tibiaRotation_ = Eigen::Vector3d::Zero(); ///< Desired joint rotation in [deg]
  Eigen::Vector3d tibiaRotationActual_ = Eigen::Vector3d::Zero(); ///< Current joint rotation in [deg]
  Eigen::Vector3d minTibiaRotation_{-20, -10, -10}; ///< Min Allowed Rotation [deg]
  Eigen::Vector3d maxTibiaRotation_ = {20, 10, 10}; ///< Max Allowed Rotation [deg]

  Eigen::Vector3d femurRotation_ = Eigen::Vector3d::Zero(); ///< Desired Joint rotation in [deg]
  Eigen::Vector3d femurRotationActual_ = Eigen::Vector3d::Zero(); ///< Current joint rotation in [deg]
  Eigen::Vector3d minFemurRotation_{-20, -10, -10}; ///< Min Allowed Rotation [deg]
  Eigen::Vector3d maxFemurRotation_ = {20, 10, 10}; ///< Max Allowed Rotation [deg]

  sva::PTransformd tibiaOffset_ = sva::PTransformd::Identity(); // Tibia offset [m]
  sva::PTransformd femurOffset_ = sva::PTransformd::Identity(); // Femur offset [m]
  sva::PTransformd tibiaOffsetInitial_ = sva::PTransformd::Identity(); // Initial Tibia offset (used when resetting) [m]
  sva::PTransformd femurOffsetInitial_ = sva::PTransformd::Identity(); // Initial Femur offset (used when resetting) [m]

  std::shared_ptr<mc_tasks::TransformTask> tibia_task_;
  std::shared_ptr<mc_tasks::TransformTask> femur_task_;
  mc_trajectory::LinearInterpolation<sva::MotionVecd> stiffnessInterp_;
  mc_trajectory::LinearInterpolation<Eigen::Vector6d> dimWeightInterp_;
  bool continuous_ = false;
  bool measure_ = true;

  mc_trajectory::SequenceInterpolator<sva::PTransformd, PTransformInterpolator> tibiaOffsetInterpolator_,
      femurOffsetInterpolator_;
  double offsetDuration_ = 2;
  double tibiaOffsetInterpolationTime_ = 0;
  double femurOffsetInterpolationTime_ = 0;

  unsigned int controllerIter_ = 0;

  /** Thread for saving results */
  mutable std::mutex saveResultsMutex_; // should be locked when accessing resultsBoneTagCopy_ and resultsProtoTMRCopy_;
  std::condition_variable saveResultsCv_; // call notify_one on this condition variable to wake up the thread
  bool saveResultsThreadRunning_ = false;
  std::thread saveResultsThread_;
  void saveResultsThread();
};
