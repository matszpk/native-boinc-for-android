#! /usr/bin/env python

from distutils.core import setup

setup(name = 'Boinc',
      version = '6.10.58',
      description = 'Python API for BOINC',
      url = 'http://boinc.berkeley.edu',
      packages = ['Boinc'],
      py_modules = ['boinc_path_config']
     )
