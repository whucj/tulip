TULIP_INSTALL_PYTHON_FILES(tulip/plugins ${CMAKE_CURRENT_SOURCE_DIR}/import
                                         ${CMAKE_CURRENT_SOURCE_DIR}/color
                                         ${CMAKE_CURRENT_SOURCE_DIR}/layout)

TULIP_INSTALL_PYTHON_FILES(tulip/plugins/general ${CMAKE_CURRENT_SOURCE_DIR}/general/SquarifiedTreeMapHelper.py)
TULIP_INSTALL_PYTHON_FILES(tulipgui/plugins/general ${CMAKE_CURRENT_SOURCE_DIR}/general/H3LayoutHelper.py)

ADD_CUSTOM_TARGET(copyTulipPythonPlugins ALL
                                         COMMAND ${CMAKE_COMMAND} -E make_directory ${TULIP_PYTHON_FOLDER}/plugins/color
                                         COMMAND ${CMAKE_COMMAND} -E make_directory ${TULIP_PYTHON_FOLDER}/plugins/general
                                         COMMAND ${CMAKE_COMMAND} -E make_directory ${TULIP_PYTHON_FOLDER}/plugins/import
                                         COMMAND ${CMAKE_COMMAND} -E make_directory ${TULIP_PYTHON_FOLDER}/plugins/layout
                                         COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/color ${TULIP_PYTHON_FOLDER}/plugins/color
                                         COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/general ${TULIP_PYTHON_FOLDER}/plugins/general
                                         COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/import ${TULIP_PYTHON_FOLDER}/plugins/import
                                         COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/layout ${TULIP_PYTHON_FOLDER}/plugins/layout)

IF(TULIP_ACTIVATE_PYTHON_WHEEL_TARGET)
  ADD_DEPENDENCIES(wheel copyTulipPythonPlugins)
ENDIF(TULIP_ACTIVATE_PYTHON_WHEEL_TARGET)
