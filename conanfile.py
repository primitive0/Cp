from conan import ConanFile
from conan.tools.cmake import cmake_layout


class CPrimeConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    requires = (
        "catch2/3.13.0",
        "utfcpp/4.0.9",
        "magic_enum/0.9.7",
        "boost/1.91.0",
        "cxxopts/3.3.1",
    )

    default_options = {
        "boost/*:header_only": True,
    }

    generators = (
        "CMakeToolchain",
        "CMakeDeps",
    )

    def layout(self):
        cmake_layout(self)
        self.folders.build = "build"
        self.folders.generators = "build/generators"
