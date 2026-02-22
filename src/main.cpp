#include "application.hpp"
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

int main(int argc, char *argv[]) {

  zephyr::Application application;

  application.init();
  application.run();

  return 0;
}
