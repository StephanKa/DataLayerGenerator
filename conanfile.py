"""Conanfile module for the DataLayerGenerator project."""
import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class DataLayerGeneratorConan(ConanFile):
    """Header-only C++20/23 data layer framework with compile-time code generator."""

    name = 'datalayer-generator'
    version = '0.0.1'
    description = 'Header-only C++20/23 data layer framework with compile-time Python/Jinja2 code generation'
    url = 'https://github.com/your-org/DataLayerGenerator'
    homepage = 'https://github.com/your-org/DataLayerGenerator'
    license = 'Unlicense'
    topics = ('embedded', 'code-generation', 'header-only', 'c++23', 'data-layer')

    # Header-only: settings are retained for option compatibility with consumers.
    package_type = 'header-library'
    settings = 'os', 'compiler', 'build_type', 'arch'

    options = {
        'with_fmt': [True, False],
        'with_file_persistence': [True, False],
    }
    default_options = {
        'with_fmt': True,
        'with_file_persistence': True,
        'fmt/*:header_only': True,
    }

    # Sources that must be present when packaging from source
    exports_sources = (
        'CMakeLists.txt',
        'templates/*',
        'cmake/*',
        'src/CMakeLists.txt',
        'src/include/*',
        'src/generator/*',
        'src/model/*',
        'src/template/*',
        'src/main.cpp',
        'src/embedded.cpp',
        'src/linkerscript/*',
        'LICENSE',
    )

    # Keep the source tree in place because CMake installs the package resources from it.
    no_copy_source = True

    def layout(self):
        """Define the package layout using the standard CMake project structure."""
        cmake_layout(self)

    def requirements(self):
        """Declare runtime dependencies based on enabled options."""
        if self.options.with_fmt:
            self.requires('fmt/12.1.0')

    def generate(self):
        """Generate build system files for the CMake install step."""
        tc = CMakeToolchain(self)
        tc.user_presets_path = None
        tc.variables['BUILD_EXAMPLES'] = False
        tc.variables['ENABLE_TESTING'] = False
        tc.variables['ENABLE_DOCS'] = False
        tc.variables['ENABLE_PYBIND11'] = False
        tc.variables['ENABLE_FMT'] = self.options.with_fmt
        tc.variables['ENABLE_FILE_PERSISTENCE'] = self.options.with_file_persistence
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        """Configure and install the header-only CMake package."""
        cmake = CMake(self)
        cmake.configure()
        cmake.install()

    def package_info(self):
        """Populate cpp_info so consuming CMake projects find the library and its compile definitions."""
        # CMake package / target names
        self.cpp_info.set_property('cmake_file_name', 'DataLayerGenerator')
        self.cpp_info.set_property('cmake_target_name', 'DataLayerGenerator::DataLayerGenerator')

        # Inject the helper cmake module so consumers get generate_datalayer()
        self.cpp_info.set_property(
            'cmake_build_modules',
            [os.path.join('lib', 'cmake', 'DataLayerGenerator', 'DataLayerGeneratorHelpers.cmake')],
        )

        # Header-only: no compiled artifacts
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = ['include']

        # Propagate compile definitions driven by options
        if self.options.with_fmt:
            self.cpp_info.defines.append('USE_FMT')
        if self.options.with_file_persistence:
            self.cpp_info.defines.append('USE_FILE_PERSISTENCE')
