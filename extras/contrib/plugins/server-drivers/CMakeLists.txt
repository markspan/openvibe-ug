# Acquisition server driver plugins contributed by third parties.
#
# Listed explicitly: this is the manifest of drivers ported to the AbstractDriver interface. The
# other folders here are unported legacy sources targeting the retired ovasIDriver/GTK interface and
# build nothing.
add_subdirectory(gtec-gipsa)
# UG fork: upstream ships the eego driver's source but does not build it
add_subdirectory(eemagine-eego)
