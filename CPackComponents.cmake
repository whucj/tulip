SET(CPACK_COMPONENT_GROUP_THIRDPARTY_DESCRIPTION "Thirdparty libraries.")
SET(CPACK_COMPONENT_GROUP_LIBRARIES_DESCRIPTION "Tulip framework dynamic libraries.")
SET(CPACK_COMPONENT_GROUP_HEADERS_DESCRIPTION "Tulip framework C++ headers.")
SET(CPACK_COMPONENT_GROUP_SOFTWARE_DESCRIPTION "Tulip softwares.")
SET(CPACK_COMPONENT_GROUP_PLUGINS_DESCRIPTION "Tulip base plugins set.")
SET(CPACK_COMPONENT_GROUP_EXTRAS_DESCRIPTION "Tulip extra files and documentation.")

#thirdparty
DEFINE_COMPONENT(ftgl "FTGL" "A library to render freetype fonts in openGL scenes." ""  ${THIRDPARTY_GROUP_NAME})
DEFINE_COMPONENT(qscintilla2 "QScintilla 2" "A library to provide source code edition facility in Qt." ""  ${THIRDPARTY_GROUP_NAME})
DEFINE_COMPONENT(sip "SIP library" "A library providing python bindings for C++." ""  ${THIRDPARTY_GROUP_NAME})

# library/tulip
DEFINE_COMPONENT(libtulip "Core library" "Tulip core library provides a framework for huge graph manipulation." ""  ${LIBS_GROUP_NAME})
DEFINE_COMPONENT(libtulip_dev "Core library - Development files" "Tulip core library provides a framework for huge graph manipulation." "libtulip"  ${HEADERS_GROUP_NAME})

# library/tulip-ogl
DEFINE_COMPONENT(libtulip_ogl "OpenGL library" "Tulip OpenGL provides a library for 3D visualization of graphs created with the Tulip core library. " "libtulip;ftgl" ${LIBS_GROUP_NAME})
DEFINE_COMPONENT(libtulip_ogl_dev "OpenGL libary - Development files" "Tulip OpenGL provides a library for for 3D visualization of graphs created with the Tulip core library." "libtulip_dev;libtulip_ogl"  ${HEADERS_GROUP_NAME})

#library/tulip-qt
DEFINE_COMPONENT(libtulip_qt "Qt library" "Tulip Qt provides a library for the Qt4 framework helping the design of graphical interfaces to integrate Tulip 3D visualizations into a Qt application." "libtulip;libtulip_ogl"  ${LIBS_GROUP_NAME})
DEFINE_COMPONENT(libtulip_qt_dev "Qt library - Development files" "Tulip Qt provides a library for the Qt4 framework helping the design of graphical interfaces to integrate Tulip 3D visualizations into a Qt application." "libtulip_qt;libtulip_ogl_dev" ${HEADERS_GROUP_NAME})

#Extra features
DEFINE_COMPONENT(tulip_ogdf "OGDF library bridge" "Tulip OGDF provides a bridge to use the powerful OGDF library (Open Graph Drawing Framework) inside Tulip." "libtulip" ${LIBS_GROUP_NAME})
DEFINE_COMPONENT(tulip_python "Python bindings" "Python bindings for Tulip." "sip;qscintilla2"  ${LIBS_GROUP_NAME})

# plugins
DEFINE_COMPONENT(tulip_plugins "Tulip Base plugins" "Base Tulip Plugins from trunk" "libtulip;libtulip_ogl;libtulip_qt"  ${PLUGINS_GROUP_NAME})
DEFINE_COMPONENT(tulip "Tulip software" "The main Tulip software. Provides a complete interface and a set of tools to easily create, manage and visualize huge graphs in 3D scenes." "libtulip;libtulip_ogl;libtulip_qt;tulip_plugins" ${SOFTWARE_GROUP_NAME})

# doc/
# must be always present because it may be installed by hand
# even it is not generated
DEFINE_COMPONENT(tulip_doc "Framework documentation" "Manuals and doxygen for the Tulip framework." "tulip" ${EXTRAS_GROUP_NAME})

IF(SPHINX_FOUND)
DEFINE_COMPONENT(tulip_python_doc "Python bindings documentation" "Manual and API description for the Tulip Python bindings." "tulip_python"  ${EXTRAS_GROUP_NAME})
ENDIF(SPHINX_FOUND)

IF(LINUX)
  SET(META_DEPS "ftgl;qscintilla2;gzstream;sip;libtulip;libtulip_ogl;libtulip_qt;tulip;tulip_plugins")
  # meta package (Linux only)
  IF(GENERATE_DOC)
    SET(META_DEPS "${META_DEPS};tulip-doc")
  ENDIF()
  DEFINE_COMPONENT(tulip_all "Meta package" "Meta package containing tulip application, libraries, documentation and base plugins" "${META_DEPS}" "" ${EXTRAS_GROUP_NAME})

  SET(CPACK_DEBIAN_EXCLUDE_COMPONENTS "ftgl;sip;qscintilla2;tulip_all")
  
  SET(CPACK_UBUNTU_DISTRIBUTION_RELEASES lucid maverick natty)

  SET(CPACK_UBUNTU_COMMON_BUILD_DEPENDS "libgl1-mesa-dev;libglu1-mesa-dev;libglew1.5-dev;libftgl-dev;libjpeg62-dev;libpng12-dev;libxml2-dev;libqt4-dev;libqt4-opengl-dev;zlib1g-dev;libstdc++6;python-dev;python-sip-dev;libqscintilla2-dev;doxygen;gcj-jdk;libxml-commons-resolver1.1-java-gcj;qt4-dev-tools;python-sphinx;docbook-xsl;graphviz;texlive-font-utils")

  SET(CPACK_COMPONENT_LIBTULIP_UBUNTU_COMMON_DEPENDS "zlib1g, libstdc++6")
  SET(CPACK_COMPONENT_LIBTULIP-DEV_UBUNTU_COMMON_DEPENDS "libtulip, zlib1g-dev, libstdc++6")
  SET(CPACK_COMPONENT_LIBTULIP-OGL_UBUNTU_COMMON_DEPENDS "libtulip, libgl1-mesa-glx, libglu1-mesa, libglew1.5, libftgl2, libjpeg62, libpng12-0, libxml2")
  SET(CPACK_COMPONENT_LIBTULIP-OGL-DEV_UBUNTU_COMMON_DEPENDS "libtulip-dev, libtulip-ogl, libgl1-mesa-dev, libglu1-mesa-dev, libglew1.5-dev, libftgl-dev, libjpeg62-dev, libpng12-dev, libxml2-dev")
  SET(CPACK_COMPONENT_LIBTULIP-QT_UBUNTU_COMMON_DEPENDS "libtulip, libtulip-ogl, libqt4-core, libqt4-gui, libqt4-opengl")
  SET(CPACK_COMPONENT_LIBTULIP-QT-DEV_UBUNTU_COMMON_DEPENDS "libtulip-qt, libtulip-ogl-dev, libqt4-dev, libqt4-opengl-dev")
  SET(CPACK_COMPONENT_TULIP-OGDF_UBUNTU_COMMON_DEPENDS "tulip, libtulip") 
  SET(CPACK_COMPONENT_TULIP-PYTHON_UBUNTU_COMMON_DEPENDS "tulip, python, python-sip, libqscintilla2-5")
  SET(CPACK_COMPONENT_TULIP-PLUGINS_UBUNTU_COMMON_DEPENDS "tulip, libtulip, libtulip-ogl, libtulip-qt")
  SET(CPACK_COMPONENT_TULIP_UBUNTU_COMMON_DEPENDS "libtulip, libtulip-ogl, libtulip-qt, tulip-plugins")
  SET(CPACK_COMPONENT_TULIP-DOC_UBUNTU_COMMON_DEPENDS "tulip, qt4-dev-tools")
  SET(CPACK_COMPONENT_TULIP-PYTHON-DOC_UBUNTU_COMMON_DEPENDS "tulip-python")  

  SET(CPACK_LUCID_BUILD_DEPENDS ${CPACK_UBUNTU_COMMON_BUILD_DEPENDS})
  SET(CPACK_MAVERICK_BUILD_DEPENDS ${CPACK_UBUNTU_COMMON_BUILD_DEPENDS})
  SET(CPACK_NATTY_BUILD_DEPENDS ${CPACK_UBUNTU_COMMON_BUILD_DEPENDS})

  SET(CPACK_COMPONENT_LIBTULIP_LUCID_DEPENDS ${CPACK_COMPONENT_LIBTULIP_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-DEV_LUCID_DEPENDS ${CPACK_COMPONENT_LIBTULIP-DEV_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-OGL_LUCID_DEPENDS ${CPACK_COMPONENT_LIBTULIP-OGL_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-OGL-DEV_LUCID_DEPENDS ${CPACK_COMPONENT_LIBTULIP-OGL-DEV_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-QT_LUCID_DEPENDS ${CPACK_COMPONENT_LIBTULIP-QT_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-QT-DEV_LUCID_DEPENDS ${CPACK_COMPONENT_LIBTULIP-QT-DEV_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-OGDF_LUCID_DEPENDS ${CPACK_COMPONENT_LIBTULIP-OGDF_UBUNTU_COMMON_DEPENDS}) 
  SET(CPACK_COMPONENT_TULIP-PYTHON_LUCID_DEPENDS ${CPACK_COMPONENT_TULIP-PYTHON_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-PLUGINS_LUCID_DEPENDS ${CPACK_COMPONENT_TULIP-PLUGINS_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP_LUCID_DEPENDS ${CPACK_COMPONENT_TULIP_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-DOC_LUCID_DEPENDS ${CPACK_COMPONENT_TULIP-DOC_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-PYTHON-DOC_LUCID_DEPENDS ${CPACK_COMPONENT_TULIP-PYTHON-DOC_UBUNTU_COMMON_DEPENDS})

  SET(CPACK_COMPONENT_LIBTULIP_MAVERICK_DEPENDS ${CPACK_COMPONENT_LIBTULIP_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-DEV_MAVERICK_DEPENDS ${CPACK_COMPONENT_LIBTULIP-DEV_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-OGL_MAVERICK_DEPENDS ${CPACK_COMPONENT_LIBTULIP-OGL_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-OGL-DEV_MAVERICK_DEPENDS ${CPACK_COMPONENT_LIBTULIP-OGL-DEV_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-QT_MAVERICK_DEPENDS ${CPACK_COMPONENT_LIBTULIP-QT_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-QT-DEV_MAVERICK_DEPENDS ${CPACK_COMPONENT_LIBTULIP-QT-DEV_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-OGDF_MAVERICK_DEPENDS ${CPACK_COMPONENT_LIBTULIP-OGDF_UBUNTU_COMMON_DEPENDS}) 
  SET(CPACK_COMPONENT_TULIP-PYTHON_MAVERICK_DEPENDS ${CPACK_COMPONENT_TULIP-PYTHON_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-PLUGINS_MAVERICK_DEPENDS ${CPACK_COMPONENT_TULIP-PLUGINS_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP_MAVERICK_DEPENDS ${CPACK_COMPONENT_TULIP_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-DOC_MAVERICK_DEPENDS ${CPACK_COMPONENT_TULIP-DOC_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-PYTHON-DOC_MAVERICK_DEPENDS ${CPACK_COMPONENT_TULIP-PYTHON-DOC_UBUNTU_COMMON_DEPENDS})

  SET(CPACK_COMPONENT_LIBTULIP_NATTY_DEPENDS ${CPACK_COMPONENT_LIBTULIP_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-DEV_NATTY_DEPENDS ${CPACK_COMPONENT_LIBTULIP-DEV_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-OGL_NATTY_DEPENDS ${CPACK_COMPONENT_LIBTULIP-OGL_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-OGL-DEV_NATTY_DEPENDS ${CPACK_COMPONENT_LIBTULIP-OGL-DEV_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-QT_NATTY_DEPENDS ${CPACK_COMPONENT_LIBTULIP-QT_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_LIBTULIP-QT-DEV_NATTY_DEPENDS ${CPACK_COMPONENT_LIBTULIP-QT-DEV_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-OGDF_NATTY_DEPENDS ${CPACK_COMPONENT_LIBTULIP-OGDF_UBUNTU_COMMON_DEPENDS}) 
  SET(CPACK_COMPONENT_TULIP-PYTHON_NATTY_DEPENDS ${CPACK_COMPONENT_TULIP-PYTHON_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-PLUGINS_NATTY_DEPENDS ${CPACK_COMPONENT_TULIP-PLUGINS_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP_NATTY_DEPENDS ${CPACK_COMPONENT_TULIP_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-DOC_NATTY_DEPENDS ${CPACK_COMPONENT_TULIP-DOC_UBUNTU_COMMON_DEPENDS})
  SET(CPACK_COMPONENT_TULIP-PYTHON-DOC_NATTY_DEPENDS ${CPACK_COMPONENT_TULIP-PYTHON-DOC_UBUNTU_COMMON_DEPENDS})
  

ENDIF()
