//#include "Debug.h"
//
//#include <ugine/Log.h>
//
//namespace ugine::utils {
//
//void OnDebugAttached(const Debug& debug) {
//    //UGINE_TRACE("__DEBUG ATTACHED [FRAME={}]: {}, {}, {}", core::CoreService::Frame(), debug.name, debug.file, debug.line);
//}
//
//void OnDebugDetached(const Debug& debug) {
//    //UGINE_TRACE("__DEBUG DETACHED [FRAME={}]: {}, {}, {}", core::CoreService::Frame(), debug.name, debug.file, debug.line);
//}
//
//void OnDebugTrace(const Debug& debug) {
//    //UGINE_TRACE("__DEBUG TRACE    [FRAME={}]: {}, {}, {}", core::CoreService::Frame(), debug.name, debug.file, debug.line);
//}
//
//void OnDebugAttach(GameObjectRegistry& reg, GameObjectHandle e) {
//    OnDebugAttached(reg.get<Debug>(e));
//}
//
//void OnDebugDetach(GameObjectRegistry& reg, GameObjectHandle e) {
//    OnDebugDetached(reg.get<Debug>(e));
//}
//
//} // namespace ugine::utils
