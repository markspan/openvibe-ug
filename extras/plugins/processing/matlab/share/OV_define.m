% OV_MATLAB toolkit
% OV_define.m
% -------------------------------
% Author : Laurent Bonnet (INRIA)
% Date   : 28/07/2011

% This file is meant to be executed prior to any user defined script.
% It declare all stim IDs as  variable for use  by the user.
% MEANT TO BE CALLED BY OPENVIBE
% 
% Stimulation codes 
% Originally from openvibe-toolkit/ovtk_defines.h
%
% UG fork: IDs are read with sscanf('%lx'), which gives the exact uint64. uint64(hex2dec(...)) went through a double,
% rounded every ID above 2^53 and warned about each one. See UG.md, 2.59.


function OV_define()

% STREAM TYPES:
    global OV_TypeId_EBMLStream;
    OV_TypeId_EBMLStream = sscanf('434F65872EFD2B7E', '%lx');
    global OV_TypeId_ChannelLocalisation;
    OV_TypeId_ChannelLocalisation = sscanf('013DF452A3A8879A', '%lx');
    global OV_TypeId_ExperimentationInformation;                                % deprecated
    OV_TypeId_ExperimentationInformation = sscanf('403488E7565D70B6', '%lx'); % deprecated
    global OV_TypeId_ExperimentInfo;
    OV_TypeId_ExperimentInfo = sscanf('403488E7565D70B6', '%lx');
    global OV_TypeId_Stimulations;
    OV_TypeId_Stimulations = sscanf('6F752DD0082A321E', '%lx');
    global OV_TypeId_StreamedMatrix;
    OV_TypeId_StreamedMatrix = sscanf('544A003E6DCBA5F6', '%lx');
    global OV_TypeId_FeatureVector;
    OV_TypeId_FeatureVector = sscanf('17341935152FF448', '%lx');
    global OV_TypeId_Signal;
    OV_TypeId_Signal = sscanf('5BA36127195FEAE1', '%lx');
    global OV_TypeId_Spectrum;
    OV_TypeId_Spectrum = sscanf('1F261C0A593BF6BD', '%lx');

% SETTING TYPES:
	global OV_TypeId_Boolean;
    OV_TypeId_Boolean = sscanf('2CDB2F0B12F231EA', '%lx');
	global OV_TypeId_Integer;
    OV_TypeId_Integer = sscanf('007DEEF92F3E95C6', '%lx');
	global OV_TypeId_Float;
    OV_TypeId_Float = sscanf('512A166F5C3EF83F', '%lx');
	global OV_TypeId_String;
    OV_TypeId_String = sscanf('79A9EDEB245D83FC', '%lx');
	global OV_TypeId_Filename;
    OV_TypeId_Filename = sscanf('330306DD74A95F98', '%lx');
	global OV_TypeId_Script;
    OV_TypeId_Script = sscanf('B0D0DB4549CBC34A', '%lx');
	global OV_TypeId_Stimulation;
    OV_TypeId_Stimulation = sscanf('2C132D6E44AB0D97', '%lx');
	global OV_TypeId_LogLevel;
    OV_TypeId_LogLevel = sscanf('A88B36670871638C', '%lx');
	global OV_TypeId_Color;
    OV_TypeId_Color = sscanf('7F45A2A97DB12219', '%lx');
	global OV_TypeId_ColorGradient;
    OV_TypeId_ColorGradient = sscanf('3D3C7C7FEF0E7129', '%lx');
	
	
% STIM CODES:
    global OVTK_StimulationId_LabelStart;
     OVTK_StimulationId_LabelStart                         = sscanf('00008100', '%lx');
    global OVTK_StimulationId_LabelEnd;
     OVTK_StimulationId_LabelEnd                           = sscanf('000081ff', '%lx');
     
    global OVTK_StimulationId_NumberStart;
     OVTK_StimulationId_NumberStart                         = sscanf('00000000', '%lx');
    global OVTK_StimulationId_NumberEnd;
     OVTK_StimulationId_NumberEnd                           = sscanf('000000ff', '%lx');
end
