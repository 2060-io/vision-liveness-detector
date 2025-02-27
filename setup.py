from setuptools import setup, find_packages
import os

# Collecting the binaries for distribution
def collect_binaries(directory):
    return [os.path.join('bin', f) for f in os.listdir(directory) if os.path.isfile(os.path.join(directory, f))]

# Package data for the compiled binaries
package_data = {
    'liveness_detector': collect_binaries('liveness_detector/bin'),
}

setup(
    name='liveness-detector',
    version='0.1.0',
    description='Liveness detector server launcher with precompiled binaries',
    author='Your Name',
    author_email='your.email@example.com',
    packages=find_packages(include=['liveness_detector', 'liveness_detector.*']),
    package_data=package_data,
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
        # Specify external dependencies here
    ],
)