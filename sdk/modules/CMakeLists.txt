# Add all the subdirs as projects of the named branch
OV_ADD_PROJECTS("MODULES")

# Sort target into directories for better visualization in IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(MODULES_FOLDER Modules)