#pragma once

// Boxes
//---------------------------------------------------------------------------------------------------
#define OVP_ClassId_Algorithm_AcquisitionDecoder                                                OpenViBE::CIdentifier(0x1E0812B7, 0x3F686DD4)
#define OVP_ClassId_Algorithm_AcquisitionDecoderDesc                                            OpenViBE::CIdentifier(0xA01599B0, 0x7F51631A)
#define OVP_Algorithm_AcquisitionDecoder_OutputParameterId_BufferDuration                       OpenViBE::CIdentifier(0x7527D6E5, 0xB7A70339)
#define OVP_Algorithm_AcquisitionDecoder_OutputParameterId_ExperimentInfoStream                 OpenViBE::CIdentifier(0xA7F1D539, 0xEC708539)
#define OVP_Algorithm_AcquisitionDecoder_OutputParameterId_SignalStream                         OpenViBE::CIdentifier(0x42C0D7BD, 0xBBCEA3F3)
#define OVP_Algorithm_AcquisitionDecoder_OutputParameterId_StimulationStream                    OpenViBE::CIdentifier(0x08FC3C12, 0x86A07BF7)
#define OVP_Algorithm_AcquisitionDecoder_OutputParameterId_ChannelLocalisationStream            OpenViBE::CIdentifier(0x4EB92F81, 0x6ECDA6B9)
#define OVP_Algorithm_AcquisitionDecoder_OutputParameterId_ChannelUnitsStream                   OpenViBE::CIdentifier(0x11B93981, 0x6E5DA9B0)

#define OVP_ClassId_Algorithm_AcquisitionEncoder                                                OpenViBE::CIdentifier(0xF9FD2FB5, 0xDF0B3B2C)
#define OVP_ClassId_Algorithm_AcquisitionEncoderDesc                                            OpenViBE::CIdentifier(0xE3E0D9EB, 0x4D4EBA00)
#define OVP_Algorithm_AcquisitionEncoder_InputParameterId_BufferDuration                        OpenViBE::CIdentifier(0xAFA07097, 0x1145B59B)
#define OVP_Algorithm_AcquisitionEncoder_InputParameterId_ExperimentInfoStream                  OpenViBE::CIdentifier(0x38755128, 0xCB0C908A)
#define OVP_Algorithm_AcquisitionEncoder_InputParameterId_SignalStream                          OpenViBE::CIdentifier(0x4ED9D929, 0x6DF5B2B6)
#define OVP_Algorithm_AcquisitionEncoder_InputParameterId_StimulationStream                     OpenViBE::CIdentifier(0xCDE202AD, 0xF4864EC9)
#define OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelLocalisationStream             OpenViBE::CIdentifier(0x2CF786E5, 0x520714A1)
#define OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelUnitsStream                    OpenViBE::CIdentifier(0x25DD84B4, 0x528524CA)

#define OVP_ClassId_Algorithm_ChannelLocalisationDecoder                                        OpenViBE::CIdentifier(0x8222F065, 0xB05D35CF)
#define OVP_ClassId_Algorithm_ChannelLocalisationDecoderDesc                                    OpenViBE::CIdentifier(0x713A29FD, 0xA5A95E2C)
#define OVP_Algorithm_ChannelLocalisationDecoder_OutputParameterId_Dynamic                      OpenViBE::CIdentifier(0xD20991FD, 0xA3153651)

#define OVP_ClassId_Algorithm_ChannelUnitsDecoder                                               OpenViBE::CIdentifier(0x5F973DDF, 0x4A582DAF)
#define OVP_ClassId_Algorithm_ChannelUnitsDecoderDesc                                           OpenViBE::CIdentifier(0x2D59257D, 0x3B1915DA)
#define OVP_Algorithm_ChannelUnitsDecoder_OutputParameterId_Dynamic                             OpenViBE::CIdentifier(0x31CF1C7A, 0x17475323)

#define OVP_ClassId_Algorithm_ChannelUnitsEncoder                                               OpenViBE::CIdentifier(0x2CA034FD, 0x5C051E86)
#define OVP_ClassId_Algorithm_ChannelUnitsEncoderDesc                                           OpenViBE::CIdentifier(0x08696DFC, 0x6D415262)
#define OVP_Algorithm_ChannelUnitsEncoder_InputParameterId_Dynamic                              OpenViBE::CIdentifier(0x615F03B9, 0x4F6A320A)

#define OVP_ClassId_BoxAlgorithm_DecoderAlgorithmTest                                           OpenViBE::CIdentifier(0x3C2EF355, 0xFE495C3D)
#define OVP_ClassId_BoxAlgorithm_DecoderAlgorithmTestDesc                                       OpenViBE::CIdentifier(0xE5176EB9, 0xD6E47D7F)

#define OVP_ClassId_Algorithm_EBMLBaseDecoder                                                   OpenViBE::CIdentifier(0xFD30C96D, 0x8245A8F8)
#define OVP_ClassId_Algorithm_EBMLBaseDecoderDesc                                               OpenViBE::CIdentifier(0x4F701AC9, 0xDFBE912E)
#define OVP_Algorithm_EBMLDecoder_InputParameterId_MemoryBufferToDecode                         OpenViBE::CIdentifier(0x2F98EA3C, 0xFB0BE096)
#define OVP_Algorithm_EBMLDecoder_OutputTriggerId_ReceivedHeader                                OpenViBE::CIdentifier(0x815234BF, 0xAABAE5F2)
#define OVP_Algorithm_EBMLDecoder_OutputTriggerId_ReceivedBuffer                                OpenViBE::CIdentifier(0xAA2738BF, 0xF7FE9FC3)
#define OVP_Algorithm_EBMLDecoder_OutputTriggerId_ReceivedEnd                                   OpenViBE::CIdentifier(0xC4AA114C, 0x628C2D77)

#define OVP_ClassId_Algorithm_EBMLBaseEncoder                                                   OpenViBE::CIdentifier(0x4272C178, 0x3FE84927)
#define OVP_ClassId_Algorithm_EBMLBaseEncoderDesc                                               OpenViBE::CIdentifier(0x47A9E701, 0x7C57BF3C)
#define OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer                         OpenViBE::CIdentifier(0xA3D8B171, 0xF8734734)
#define OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader                                   OpenViBE::CIdentifier(0x878EAF60, 0xF9D5303F)
#define OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer                                   OpenViBE::CIdentifier(0x1B7076FD, 0x449BC70A)
#define OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd                                      OpenViBE::CIdentifier(0x3FC23508, 0x806753D8)
#define OVP_Algorithm_EBMLEncoder_OutputTriggerId_MemoryBufferUpdated                           OpenViBE::CIdentifier(0xD46C7462, 0xD3407E5F)

#define OVP_ClassId_Algorithm_ExperimentInfoDecoder                                             OpenViBE::CIdentifier(0x6FA7D52B, 0x80E2ABD6)
#define OVP_ClassId_Algorithm_ExperimentInfoDecoderDesc                                         OpenViBE::CIdentifier(0x0F37CA61, 0x8A77F44E)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_ExperimentID                      OpenViBE::CIdentifier(0x40259641, 0x478C73DE)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_ExperimentDate                    OpenViBE::CIdentifier(0xBC0266A2, 0x9C2935F1)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectID                         OpenViBE::CIdentifier(0x97C5D20D, 0x203E65B3)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectName                       OpenViBE::CIdentifier(0x3D3826EA, 0xE8883815)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectAge                        OpenViBE::CIdentifier(0xC36C6B08, 0x5227380A)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectGender                     OpenViBE::CIdentifier(0x7D5059E8, 0xE4D8B38D)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_LaboratoryID                      OpenViBE::CIdentifier(0xE761D3D4, 0x44BA1EBF)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_LaboratoryName                    OpenViBE::CIdentifier(0x5CA80FA5, 0x774F01CB)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_TechnicianID                      OpenViBE::CIdentifier(0xC8ECFBBC, 0x0DCDA310)
#define OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_TechnicianName                    OpenViBE::CIdentifier(0xB8A94B68, 0x389393D9)

#define OVP_ClassId_BoxAlgorithm_EncoderAlgorithmTest                                           OpenViBE::CIdentifier(0x87D18C62, 0xF2DAF779)
#define OVP_ClassId_BoxAlgorithm_EncoderAlgorithmTestDesc                                       OpenViBE::CIdentifier(0x95E27325, 0x6893A519)

#define OVP_ClassId_Algorithm_ExperimentInfoEncoder                                             OpenViBE::CIdentifier(0x56B354FE, 0xBF175468)
#define OVP_ClassId_Algorithm_ExperimentInfoEncoderDesc                                         OpenViBE::CIdentifier(0x8CC2C754, 0x61665FDA)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentID                       OpenViBE::CIdentifier(0x40259641, 0x478C73DE)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentDate                     OpenViBE::CIdentifier(0xBC0266A2, 0x9C2935F1)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectID                          OpenViBE::CIdentifier(0x97C5D20D, 0x203E65B3)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectName                        OpenViBE::CIdentifier(0x3D3826EA, 0xE8883815)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectAge                         OpenViBE::CIdentifier(0xC36C6B08, 0x5227380A)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectGender                      OpenViBE::CIdentifier(0x7D5059E8, 0xE4D8B38D)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryID                       OpenViBE::CIdentifier(0xE761D3D4, 0x44BA1EBF)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryName                     OpenViBE::CIdentifier(0x5CA80FA5, 0x774F01CB)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianID                       OpenViBE::CIdentifier(0xC8ECFBBC, 0x0DCDA310)
#define OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianName                     OpenViBE::CIdentifier(0xB8A94B68, 0x389393D9)

#define OVP_ClassId_Algorithm_FeatureVectorDecoder                                              OpenViBE::CIdentifier(0xC2689ECC, 0x43B335C1)
#define OVP_ClassId_Algorithm_FeatureVectorDecoderDesc                                          OpenViBE::CIdentifier(0xAB0AE561, 0xF181E34F)

#define OVP_ClassId_Algorithm_FeatureVectorEncoder                                              OpenViBE::CIdentifier(0x7EBE049D, 0xF777A602)
#define OVP_ClassId_Algorithm_FeatureVectorEncoderDesc                                          OpenViBE::CIdentifier(0xC249527B, 0x89EE1996)

#define OVP_ClassId_Algorithm_MasterAcquisitionEncoder                                          OpenViBE::CIdentifier(0x2D15E00B, 0x51414EB6)
#define OVP_ClassId_Algorithm_MasterAcquisitionEncoderDesc                                      OpenViBE::CIdentifier(0xE6EC841D, 0x9E75A8FB)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectID                       OpenViBE::CIdentifier(0xD5BB5231, 0x59389B72)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectAge                      OpenViBE::CIdentifier(0x9EF355E4, 0xC8531112)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectGender                   OpenViBE::CIdentifier(0xA9056AE3, 0x57FE6AF0)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SignalMatrix                    OpenViBE::CIdentifier(0xE9AC8077, 0xE369A51D)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SignalSampling                  OpenViBE::CIdentifier(0xB84AD0CA, 0x4F316DD3)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_StimulationSet                  OpenViBE::CIdentifier(0x5B728D37, 0xFD088887)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_BufferDuration                  OpenViBE::CIdentifier(0xE1FC7385, 0x586A4F3F)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_ChannelLocalisation             OpenViBE::CIdentifier(0x227E13F0, 0x206B44F9)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_ChannelUnits                    OpenViBE::CIdentifier(0x740060C2, 0x7D2B4F57)

#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_EncodeChannelLocalisationData   OpenViBE::CIdentifier(0x26EE1F81, 0x3DB00D5D)
#define OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_EncodeChannelUnitData           OpenViBE::CIdentifier(0x19DC533C, 0x56301D0B)

#define OVP_ClassId_Algorithm_SignalDecoder                                                     OpenViBE::CIdentifier(0x7237C149, 0x0CA66DA7)
#define OVP_ClassId_Algorithm_SignalDecoderDesc                                                 OpenViBE::CIdentifier(0xF1547D89, 0x49FFD0C2)
#define OVP_Algorithm_SignalDecoder_OutputParameterId_Sampling                                  OpenViBE::CIdentifier(0x363D8D79, 0xEEFB912C)

#define OVP_ClassId_Algorithm_SignalEncoder                                                     OpenViBE::CIdentifier(0xC488AD3C, 0xEB2E36BF)
#define OVP_ClassId_Algorithm_SignalEncoderDesc                                                 OpenViBE::CIdentifier(0x90AC1E0F, 0x01518200)
#define OVP_Algorithm_SignalEncoder_InputParameterId_Sampling                                   OpenViBE::CIdentifier(0x998710FF, 0x2C5CCA82)

#define OVP_ClassId_Algorithm_SpectrumDecoder                                                   OpenViBE::CIdentifier(0x128202DB, 0x449FC7A6)
#define OVP_ClassId_Algorithm_SpectrumDecoderDesc                                               OpenViBE::CIdentifier(0x54D18EE8, 0x5DBD913A)
#define OVP_Algorithm_SpectrumDecoder_OutputParameterId_FrequencyAbscissa                       OpenViBE::CIdentifier(0x14A572E4, 0x5C405C8E)
#define OVP_Algorithm_SpectrumDecoder_OutputParameterId_Sampling                                OpenViBE::CIdentifier(0x68442C12, 0x0D9A46DE)


#define OVP_ClassId_Algorithm_SpectrumEncoder                                                   OpenViBE::CIdentifier(0xB3E252DB, 0xC3214498)
#define OVP_ClassId_Algorithm_SpectrumEncoderDesc                                               OpenViBE::CIdentifier(0xD6182973, 0x122CE114)
#define OVP_Algorithm_SpectrumEncoder_InputParameterId_FrequencyAbscissa                        OpenViBE::CIdentifier(0x05C91BD6, 0x2D8C4083)
#define OVP_Algorithm_SpectrumEncoder_InputParameterId_Sampling                                 OpenViBE::CIdentifier(0x02D25E1B, 0x76A1019B)

#define OVP_ClassId_Algorithm_StimulationDecoder                                                OpenViBE::CIdentifier(0xC8807F2B, 0x0813C5B1)
#define OVP_ClassId_Algorithm_StimulationDecoderDesc                                            OpenViBE::CIdentifier(0x391A615B, 0x71CD888A)
#define OVP_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet                       OpenViBE::CIdentifier(0xF46D0C19, 0x47306BEA)

#define OVP_ClassId_Algorithm_StimulationEncoder                                                OpenViBE::CIdentifier(0x6E86F7D5, 0xA4668108)
#define OVP_ClassId_Algorithm_StimulationEncoderDesc                                            OpenViBE::CIdentifier(0x9B994B50, 0x52C3F06A)
#define OVP_Algorithm_StimulationEncoder_InputParameterId_StimulationSet                        OpenViBE::CIdentifier(0x8565254C, 0x3A49268E)

#define OVP_ClassId_Algorithm_StreamedMatrixDecoder                                             OpenViBE::CIdentifier(0x7359D0DB, 0x91784B21)
#define OVP_ClassId_Algorithm_StreamedMatrixDecoderDesc                                         OpenViBE::CIdentifier(0x384529D5, 0xD8E0A728)
#define OVP_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix                            OpenViBE::CIdentifier(0x79EF3123, 0x35E3EA4D)

#define OVP_ClassId_Algorithm_StreamedMatrixEncoder                                             OpenViBE::CIdentifier(0x5CB32C71, 0x576F00A6)
#define OVP_ClassId_Algorithm_StreamedMatrixEncoderDesc                                         OpenViBE::CIdentifier(0xEEEFE060, 0x646EE8AB)
#define OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix                             OpenViBE::CIdentifier(0xA3E9E5B0, 0xAE756303)

#define OVP_ClassId_Algorithm_StreamStructureDecoder                                            OpenViBE::CIdentifier(0xA7EF3E8B, 0x4CF70B74)
#define OVP_ClassId_Algorithm_StreamStructureDecoderDesc                                        OpenViBE::CIdentifier(0x2E361099, 0xCBE828A7)

#define OVP_ClassId_Algorithm_ChannelLocalisationEncoder                                        OpenViBE::CIdentifier(0xC4AA738A, 0x2368C0EA)
#define OVP_ClassId_Algorithm_ChannelLocalisationEncoderDesc                                    OpenViBE::CIdentifier(0x3F7B49A3, 0x2B8F861A)
#define OVP_Algorithm_ChannelLocalisationEncoder_InputParameterId_Dynamic                       OpenViBE::CIdentifier(0xCF5DD4F8, 0xC2FF2878)
