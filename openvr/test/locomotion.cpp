#include "locomotion.h"
#include "matrix.h"

LocomotionEngine::LocomotionEngine() {
  char dir[MAX_PATH];
  GetCurrentDirectory(sizeof(dir), dir);
  std::string dirString(dir);
  dirString += "\\openvr\\test\\actions.json";
  vr::EVRInputError err = vr::VRInput()->SetActionManifestPath(dirString.c_str());

  err = vr::VRInput()->GetActionSetHandle("/actions/main", &pActionSetHandle);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy1Press", &pActionJoy1Press);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy1Touch", &pActionJoy1Touch);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy1Axis", &pActionJoy1Axis);

  std::thread([]() -> void {
    uint64_t m_lastFrameIndex = 0;

    for (;;) {
      {
        uint64_t newFrameIndex = 0;
        float lastVSync = 0;
        // while (newFrameIndex < (m_lastFrameIndex + 3)) {
        while (newFrameIndex == m_lastFrameIndex) {
          vr::VRSystem()->GetTimeSinceLastVsync(&lastVSync, &newFrameIndex);
        }
        m_lastFrameIndex = newFrameIndex;
      }
      {
        vr::VRActiveActionSet_t actionSet{};
        actionSet.ulActionSet = pActionSetHandle;
        actionSet.nPriority = vr::k_nActionSetOverlayGlobalPriorityMax;
        vr::EVRInputError err = vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

        err = vr::VRInput()->GetAnalogActionData(pActionJoy1Axis, &pInputJoy1Axis, sizeof(pInputJoy1Axis), vr::k_ulInvalidInputValueHandle);
        err = vr::VRInput()->GetDigitalActionData(pActionJoy1Press, &pInputJoy1Press, sizeof(pInputJoy1Press), vr::k_ulInvalidInputValueHandle);
        err = vr::VRInput()->GetDigitalActionData(pActionJoy1Touch, &pInputJoy1Touch, sizeof(pInputJoy1Touch ), vr::k_ulInvalidInputValueHandle);
      }
      {      
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()
        );
        
        float v = 5.0f * (1.0f + std::sin((float)(ms.count() % 2000) / 2000.0f * (float)M_PI * 2.0f) / 2.0f);
        // d += 0.1;
        float m[16] = {
          1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0,
          0, -v, -d, 1,
        };
        vr::HmdMatrix34_t m2;
        setPoseMatrix(m2, m);
        vr::VRChaperoneSetup()->SetWorkingStandingZeroPoseToRawTrackingPose(&m2);
        vr::VRChaperoneSetup()->ShowWorkingSetPreview();
      }
      sem.unlock();
    }
  }).detach();
}
LocomotionEngine::getLocomotionInputs(std::function<void(float *locomotionInputs)> cb) {
  sem.lock();

  std::lock_guard<Mutex> lock(mut);
  if (!sceneAppLocomotionEnabled) {
    float m[5];
    m[0] = locomotionEngine->pInputJoy1Axis.x;
    m[1] = locomotionEngine->pInputJoy1Axis.y;
    m[2] = locomotionEngine->pInputJoy1Axis.z;
    m[3] = locomotionEngine->pInputJoy1Press.bState ? 1.0f : 0.0f;
    m[4] = locomotionEngine->pInputJoy1Touch.bState ? 1.0f : 0.0f;
    cb(m);
  } else {
    cb(nullptr);
  }
}