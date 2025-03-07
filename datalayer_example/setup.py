# Available at setup time due to pyproject.toml
"""Module to generate an installation for the datalayer example."""
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = '0.0.1'

# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
ext_modules = [
    Pybind11Extension(
        'datalayer_example',
        ['pythonBinding.cpp'],
        # Example: passing in the version to the compiled code
        cxx_std=20,
        define_macros=[('VERSION_INFO', __version__)],
    ),
]

setup(
    name='datalayer_example',
    version=__version__,
    author='Stephan Kantelberg',
    author_email='stephan.kantelberg@zuehlke.com',
    description='A test project using pybind11',
    long_description='',
    ext_modules=ext_modules,
    #extras_require={""},
    # Currently, build_ext only provides an optional "highest supported C++
    # level" feature, but in the future it may provide more features.
    cmdclass={'build_ext': build_ext},
    zip_safe=False,
    python_requires='>=3.7',
    include_dirs=['include']
)
