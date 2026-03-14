CMAKE_MINIMUM_REQUIRED( VERSION 3.24 )

ADD_SUBDIRECTORY( other )

ADD_TEST( NAME PyTest COMMAND ${PYTHON_VENV_EXE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )

ADD_TEST( NAME PySampleCodeTest COMMAND ${PYTHON_VENV_EXE} -m unittest openvsp.tests.test_vsp_api  )

ADD_TEST( NAME PyDegenGeomTest COMMAND ${PYTHON_VENV_EXE} -m unittest openvsp.tests.test_degen_geom  )

ADD_TEST( NAME PyVSPAEROTest
    COMMAND ${PYTHON_VENV_EXE} -m pytest tests/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/python_api/packages/pyVSPAERO )

ADD_TEST( NAME PyVSPAEROViewerTest
    COMMAND ${PYTHON_VENV_EXE} -m pytest tests/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/python_api/packages/pyVSPAERO_viewer )
