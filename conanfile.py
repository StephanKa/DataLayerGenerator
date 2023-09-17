"""Conanfile module for this project."""
import conans.model.requires
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain


class ProjectConan(ConanFile):
    """This class defines the handling for different setups."""

    settings = 'os', 'compiler', 'build_type', 'arch'
    default_options = {'fmt/*:header_only': True}
    generators = 'CMakeDeps', 'CMakeToolchain'

    def configure(self):
        """Override the configure method and defines different requirements for different architectures."""
        if self.settings.get_safe('arch') == 'armv7':
            self.requires = conans.model.requires.Requirements(['fmt/10.1.0'])
        else:
            self.requires = conans.model.requires.Requirements(['catch2/3.4.0', 'fmt/10.1.0', 'pybind11/2.10.4'])

    def build(self):
        """Override the build method."""
        cmake = CMakeToolchain(self)
        cmake.configure()
        cmake.build()
