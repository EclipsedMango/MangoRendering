//
// #ifndef MANGORENDERING_CORE_H
// #define MANGORENDERING_CORE_H
//
// #include "../Nodes/Node3d.h"
// #include "Window.h"
//
// class Core {
// public:
//     explicit Core(Node3d* scene);
//     ~Core();
//
//     void Init();
//     void InitRenderer();
//     void InitImGui() const;
//
//     static void BeginImGuiFrame();
//     static void EndImGuiFrame();
//
//     void Process() const;
//
//     static void TickNodes(Node3d* node, float deltaTime);
//     static void ProcessNodes(Node3d* node);
//
//     void ChangeScene(Node3d* scene);
//
// private:
//     Node3d* m_currentScene {};
//     Window* m_activeWindow {};
// };
//
//
// #endif //MANGORENDERING_CORE_H