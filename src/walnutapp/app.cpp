#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"

Walnut::Application *Walnut::CreateApplication(int argc, char **argv) {
  Walnut::ApplicationSpecification spec;
  spec.Name = "Walnut Example";
  spec.CustomTitlebar = true;

  Walnut::Application *app = new Walnut::Application(spec);
  app->SetMenubarCallback([app]() {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit")) {
        app->Close();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {
        // gameLayer->ShowAboutModal();
      }
      ImGui::EndMenu();
    }
  });
  return app;
}
