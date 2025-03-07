"""Conanfile module for this project."""
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain


class ProjectConan(ConanFile):
    """This class defines the handling for different setups."""

    settings = 'os', 'compiler', 'build_type', 'arch'
    default_options = {'fmt/*:header_only': True}
    generators = 'CMakeDeps', 'CMakeToolchain'

    def requirements(self):
        """Override the requirements method and define packages depending on the arch type."""
        if self.settings.get_safe('arch') == 'armv7':
            self.requires('fmt/11.1.4')
        else:
            self.requires('catch2/3.8.0')
            self.requires('fmt/11.1.4')
            self.requires('pybind11/2.13.6')

    def configure(self):
        """Override the configure method and defines different requirements for different architectures."""
        cmake = CMakeToolchain(self)
        cmake.user_presets_path = None

    def build(self):
        """Override the build method."""
        cmake = CMakeToolchain(self)
        cmake.configure()
        cmake.build()
