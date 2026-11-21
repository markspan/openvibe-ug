% tuto4_square_eeg_Initialize.m
% -------------------------------
% UG fork: new file, not in upstream OpenViBE. See UG.md, 2.57.
%
% Called when the scenario starts. The input's header is not known yet, so which
% channels are EEG is worked out with the first chunk (see the Process function).
%
function box_out = tuto4_square_eeg_Initialize(box_in)

	disp('Square EEG: initialized.')

	box_in.user_data.is_eeg = [];

	box_out = box_in;
end
