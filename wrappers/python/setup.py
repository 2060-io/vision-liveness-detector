from setuptools import setup, find_packages
import os
import platform
import sys

setup(
    name='liveness_detector',
    version='0.1.0',
    description='Liveness detector server launcher with precompiled binaries',
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
            'liveness-detector-launcher=liveness_detector.server_launcher:main',  # Assuming `main` is the entry point in `server_launcher.py`
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