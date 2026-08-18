
INCLUDE("FindThirdPartyBrainmasterCodeMakerAPI")
INCLUDE("FindThirdPartyGMobiLabPlusAPI")
INCLUDE("FindThirdPartyGUSBampCAPI")
INCLUDE("FindThirdPartyMitsar")

INCLUDE("FindThirdPartyGNEEDaccessAPI")

target_link_libraries(${PROJECT_NAME}
                      eemagine-eego-sdk
                      sdk-gtec-unicorn
)