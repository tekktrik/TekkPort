# SPDX-FileCopyrightText: 2022 Alec Delaney
#
# SPDX-License-Identifier: MIT

"""A setuptools based setup module.

See:
https://packaging.python.org/en/latest/distributing.html
https://github.com/pypa/sampleproject
"""

from setuptools import setup, Extension

# To use a consistent encoding
from codecs import open
from os import path

here = path.abspath(path.dirname(__file__))

# Get the long description from the README file
with open(path.join(here, "README.rst"), encoding="utf-8") as f:
    long_description = f.read()

module = Extension(
    "parallel64",
    [
        "parallel64/__init__.c",
        "parallel64/portio.c",
        "parallel64/_BasePort.c",
    ],
)

setup(
    name="parallel64",
    version="1.0.1",
    setup_requires=["setuptools_scm"],
    description="Base Library for the Portal-style libraries.",
    long_description=long_description,
    long_description_content_type="text/x-rst",
    # Author details
    author="Alec Delaney",
    author_email="tekktrik@gmail.com",
    # The project's main homepage.
    url="https://github.com/tekktrik/parallel64",
    # Choose your license
    license="MIT",
    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Topic :: System :: Hardware",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
    ],
    # What does your project relate to?
    keywords=["parallel", "port", "spp", "epp", "ecp", "gpio"],
    # You can just specify the packages manually here if your project is
    # simple. Or you can use find_packages().
    #packages=["_parallel64"],
    ext_modules=[module],
)
