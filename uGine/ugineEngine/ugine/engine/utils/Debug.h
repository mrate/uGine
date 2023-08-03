//#pragma once
//
//#include <entt/entt.hpp>
//
//#include <format>
//#include <string>
//
//namespace ugine::utils {
//
//struct Debug {
//    std::string name;
//    std::string file;
//    int line;
//};
//
//void OnDebugAttached(const Debug& debug);
//void OnDebugDetached(const Debug& debug);
//void OnDebugTrace(const Debug& debug);
//void OnDebugAttach(GameObjectRegistry& reg, GameObjectHandle e);
//void OnDebugDetach(GameObjectRegistry& reg, GameObjectHandle e);
//
//#ifdef _DEBUG
//#define DEBUG_ATTACH(val, name, ...) val._DebugAttach(ugine::Debug{ std::format(name, __VA_ARGS__), __FILE__, __LINE__ })
//#define DEBUG_DETACH(val, name) val._DebugDetach(ugine::Debug{ name, __FILE__, __LINE__ })
//#define DEBUG_TRACE(val) OnDebugTrace(val._DebugInfo())
//#else
//#define DEBUG_ATTACH(val, name, ...)
//#define DEBUG_DETACH(val, name)
//#define DEBUG_TRACE(val)
//#endif
//
//} // namespace ugine::utils
