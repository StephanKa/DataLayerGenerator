"""Conanfile module for the DataLayerGenerator project."""
import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy


class DataLayerGeneratorConan(ConanFile):
    """Header-only C++20 data layer framework with compile-time code generator."""

    name = 'datalayer-generator'
    version = '0.0.1'
    description = 'Header-only C++20 data layer framework with compile-time Python/Jinja2 code generation'
    url = 'https://github.com/your-org/DataLayerGenerator'
    homepage = 'https://github.com/your-org/DataLayerGenerator'
    license = 'Unlicense'
    topics = ('embedded', 'code-generation', 'header-only', 'c++20', 'data-layer')

    # Header-only: no compiler/build_type/arch needed for the library itself,
    # but we keep them for the test executable and optional pybind11 module.
    package_type = 'header-library'
    settings = 'os', 'compiler', 'build_type', 'arch'

    options = {
        'with_fmt': [True, False],
        'with_file_persistence': [True, False],
        'with_pybind11': [True, False],
    }
    default_options = {
        'with_fmt': True,
        'with_file_persistence': True,
        'with_pybind11': True,
        'fmt/*:header_only': True,
    }

    # Sources that must be present when packaging from source
    exports_sources = (
        'src/include/*',
        'src/generator/*',
        'src/template/*',
        'cmake/DataLayerGeneratorHelpers.cmake',
        'LICENSE',
    )

    # Header-only: the source tree IS the package — no build step required
    no_copy_source = True

    def layout(self):
        """Define the package layout using the standard CMake project structure."""
        cmake_layout(self)

    def requirements(self):
        """Declare runtime dependencies based on enabled options."""
        if self.options.with_fmt:
            self.requires('fmt/12.1.0')
        if self.options.with_pybind11:
            self.requires('pybind11/3.0.1')

    def build_requirements(self):
        """Dependencies only needed when building the project itself (tests, example)."""
        if not self.settings.get_safe('arch') == 'armv7':
            self.test_requires('catch2/3.14.0')

    def generate(self):
        """Generate build system files. Only needed when building the example/tests."""
        tc = CMakeToolchain(self)
        tc.user_presets_path = None
        tc.generate()

    def build(self):
        """Build the example executable and/or tests (not invoked when consumed as a library)."""
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        """Install headers, generator tool, templates, and CMake helpers into the package folder."""
        # License
        copy(self, 'LICENSE',
             src=self.source_folder,
             dst=os.path.join(self.package_folder, 'licenses'))

        # C++ framework headers
        copy(self, '*.h',
             src=os.path.join(self.source_folder, 'src', 'include'),
             dst=os.path.join(self.package_folder, 'include'))

        # Python code generator
        copy(self, '*',
             src=os.path.join(self.source_folder, 'src', 'generator'),
             dst=os.path.join(self.package_folder, 'res', 'generator'),
             excludes=['__pycache__', '*.pyc'])

        # Jinja2 templates
        copy(self, '*',
             src=os.path.join(self.source_folder, 'src', 'template'),
             dst=os.path.join(self.package_folder, 'res', 'template'))

        # CMake helper function for consumers
        copy(self, 'DataLayerGeneratorHelpers.cmake',
             src=os.path.join(self.source_folder, 'cmake'),
             dst=os.path.join(self.package_folder, 'lib', 'cmake', 'DataLayerGenerator'))

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
