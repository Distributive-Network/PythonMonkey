from setuptools import setup, Extension


setup(
    name='greet',
    version='1.0',
    description='Python Package with Hello World C Extension',
    ext_modules=[
        Extension(
            'greet',
            sources=['hello.cpp'],
            py_limited_api=True)
    ],
)