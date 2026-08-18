file(COPY signals DESTINATION ${BUILD_DATADIR}/scenarios)
file(COPY bci-examples DESTINATION ${BUILD_DATADIR}/scenarios)
install(DIRECTORY signals      DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
install(DIRECTORY bci-examples DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
