#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_ADD_INCLUDEDIRS := .
COMPONENT_EMBED_TXTFILES := ${PROJECT_PATH}/certificates/firmware_upgrade/root_ca.pem
