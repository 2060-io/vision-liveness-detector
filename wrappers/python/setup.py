from setuptools import setup, find_packages
import os
import platform
import sys

from packaging.version import Version

# Try to read version string from env variable
version_string = os.environ.get("RELEASE_VERSION", "0.0.0.dev0")
version = Version(version_string)

# Read the README.md for long description
here = os.path.abspath(os.path.dirname(__file__))
readme_path = os.path.join(here, "README.md")
with open(readme_path, encoding="utf-8") as f:
    long_description = f.read()

setup(
    name='liveness_detector',
    version=str(version),
    description='Liveness detector server launcher with precompiled binaries',
    long_description=long_description,
    long_description_content_type="text/markdown",
    author='2060.io',
    author_email='r@2060.io',
    packages=find_packages(include=['liveness_detector', 'liveness_detector.*']),
    package_data={
        "": [
                "server/livenessDetectorServer",
                "server/livenessDetectorServer.exe",
                "server/lib/*",
            ]
        },
    include_package_data=True,
    entry_points={
        'console_scripts': [
            'liveness-detector-launcher=liveness_detector.server_launcher:main',
        ],
    },
    classifiers=[
        'Programming Language :: Python :: 3',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.6',
    install_requires=[
        # List required Python dependencies here
    ],
)